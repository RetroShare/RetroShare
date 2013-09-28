/*
 * libretroshare/src/services: p3discovery2.cc
 *
 * Services for RetroShare.
 *
 * Copyright 2004-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include "services/p3discovery2.h"
#include "util/rsversion.h"

#include "retroshare/rsiface.h"


// Interface pointer.
RsDisc *rsDisc = NULL;

#define P3DISC_DEBUG	1



bool populateContactInfo(const peerState &detail, RsDiscContactItem *pkt)
{
	pkt->clear();

	pkt->pgpId = detail.gpg_id;
	pkt->sslId = detail.id;
	pkt->location = detail.location;
	pkt->version = "";
	pkt->netMode = detail.netMode;
	pkt->vs_disc = detail.vs_disc;
	pkt->vs_dht = detail.vs_dht;
	
	pkt->lastContact = time(NULL);

	if (detail.hiddenNode)
	{
		pkt->isHidden = true;
		pkt->hiddenAddr = detail.hiddenDomain;
		pkt->hiddenPort = detail.hiddenPort;
	}
	else
	{
		pkt->isHidden = false;

		pkt->localAddrV4.addr = detail.localaddr;
		pkt->extAddrV4.addr = detail.serveraddr;
		sockaddr_storage_clear(pkt->localAddrV6.addr);
		sockaddr_storage_clear(pkt->extAddrV6.addr);

		pkt->dyndns = detail.dyndns;
		detail.ipAddrs.mLocal.loadTlv(pkt->localAddrList);
		detail.ipAddrs.mExt.loadTlv(pkt->extAddrList);
	}

	return true;
}

void DiscPgpInfo::mergeFriendList(const std::list<PGPID> &friends)
{
	std::list<PGPID>::const_iterator it;
	for(it = friends.begin(); it != friends.end(); it++)
	{
		mFriendSet.insert(*it);
	}
}


p3discovery2::p3discovery2(p3PeerMgr *peerMgr, p3LinkMgr *linkMgr, p3NetMgr *netMgr)
:p3Service(RS_SERVICE_TYPE_DISC), mPeerMgr(peerMgr), mLinkMgr(linkMgr), mNetMgr(netMgr), 
	mDiscMtx("p3discovery2")
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	addSerialType(new RsDiscSerialiser());

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::p3discovery2()";
	std::cerr << std::endl;
#endif

	mLastPgpUpdate = 0;
	
	// Add self into PGP FriendList.
	mFriendList[AuthGPG::getAuthGPG()->getGPGOwnId()] = DiscPgpInfo();
	
	return;
}

p3discovery2::~p3discovery2()
{
	return;
	
}


void p3discovery2::addFriend(const SSLID &sslId)
{
	PGPID pgpId = getPGPId(sslId);

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<PGPID, DiscPgpInfo>::iterator it;
	it = mFriendList.find(pgpId);
	if (it == mFriendList.end())
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::addFriend() adding pgp entry: " << pgpId;
		std::cerr << std::endl;
#endif

		mFriendList[pgpId] = DiscPgpInfo();

		it = mFriendList.find(pgpId);
	}


	/* now add SSLID */

	std::map<SSLID, DiscSslInfo>::iterator sit;
	sit = it->second.mSslIds.find(sslId);
	if (sit == it->second.mSslIds.end())
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::addFriend() adding ssl entry: " << sslId;
		std::cerr << std::endl;
#endif

		it->second.mSslIds[sslId] = DiscSslInfo();
		sit = it->second.mSslIds.find(sslId);
	}

	/* update Settings from peerMgr */
	peerState detail;
	if (mPeerMgr->getFriendNetStatus(sit->first, detail)) 
	{
		sit->second.mDiscStatus = detail.vs_disc;		
	}
	else
	{
		sit->second.mDiscStatus = RS_VS_DISC_OFF;
	}
}

void p3discovery2::removeFriend(const SSLID &sslId)
{
	PGPID pgpId = getPGPId(sslId);

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<PGPID, DiscPgpInfo>::iterator it;
	it = mFriendList.find(pgpId);
	if (it == mFriendList.end())
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::removeFriend() missing pgp entry: " << pgpId;
		std::cerr << std::endl;
#endif
		return;
	}

	std::map<SSLID, DiscSslInfo>::iterator sit;
	sit = it->second.mSslIds.find(sslId);
	if (sit == it->second.mSslIds.end())
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::addFriend() missing ssl entry: " << sslId;
		std::cerr << std::endl;
#endif
		return;
	}

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::addFriend() removing ssl entry: " << sslId;
	std::cerr << std::endl;
#endif
	it->second.mSslIds.erase(sit);

	if (it->second.mSslIds.empty())
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::addFriend() pgpId now has no sslIds";
		std::cerr << std::endl;
#endif
		/* pgp peer without any ssl entries -> check if they are still a real friend */
		if (!(AuthGPG::getAuthGPG()->isGPGAccepted(pgpId)))
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::addFriend() pgpId is no longer a friend, removing";
			std::cerr << std::endl;
#endif
			mFriendList.erase(it);
		}
	}
}


PGPID p3discovery2::getPGPId(const SSLID &id)
{
	PGPID pgpId;
	mPeerMgr->getGpgId(id, pgpId);
	return pgpId;
}


int p3discovery2::tick()
{
	return handleIncoming();
}

int p3discovery2::handleIncoming()
{
	RsItem *item = NULL;

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::handleIncoming()" << std::endl;
#endif

	int nhandled = 0;
	// While messages read
	while(NULL != (item = recvItem()))
	{
		RsDiscPgpListItem *pgplist = NULL;
		RsDiscPgpCertItem *pgpcert = NULL;
		RsDiscContactItem *contact = NULL;
		nhandled++;

#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::handleIncoming() Received Message!" << std::endl;
		item -> print(std::cerr);
		std::cerr  << std::endl;
#endif

		if (NULL != (contact = dynamic_cast<RsDiscContactItem *> (item))) 
		{
			if (item->PeerId() == contact->sslId) /* self describing */
			{
				recvOwnContactInfo(item->PeerId(), contact);
			}
			else if (rsPeers->servicePermissionFlags_sslid(item->PeerId()) & RS_SERVICE_PERM_DISCOVERY)
			{
				processContactInfo(item->PeerId(), contact);
			}
			else
			{
				/* not allowed */
				delete item;
			}
			continue;
		}

		/* any other packets should be dropped if they don't have permission */
		if(!(rsPeers->servicePermissionFlags_sslid(item->PeerId()) & RS_SERVICE_PERM_DISCOVERY))
		{
			delete item;
			continue;
		}

		if (NULL != (pgpcert = dynamic_cast<RsDiscPgpCertItem *> (item)))	
		{
			recvPGPCertificate(item->PeerId(), pgpcert);
		}
		else if (NULL != (pgplist = dynamic_cast<RsDiscPgpListItem *> (item)))	
		{
			/* two types */
			if (pgplist->mode == DISC_PGP_LIST_MODE_FRIENDS)
			{
				processPGPList(pgplist->PeerId(), pgplist);
				
			}
			else if (pgplist->mode == DISC_PGP_LIST_MODE_GETCERT)
			{
				recvPGPCertificateRequest(pgplist->PeerId(), pgplist);
			}
			else
			{
                		delete item ;
			}
		}
		else
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::handleIncoming() Unknown Received Message!" << std::endl;
			item -> print(std::cerr);
			std::cerr  << std::endl;
#endif
			delete item;
		}
	}

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::handleIncoming() finished." << std::endl;
#endif

	return nhandled;
}



void p3discovery2::sendOwnContactInfo(const SSLID &sslid)
{

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::sendOwnContactInfo()";
	std::cerr << std::endl;
#endif
	peerState detail;
	if (mPeerMgr->getOwnNetStatus(detail)) 
	{
		RsDiscContactItem *pkt = new RsDiscContactItem();
		populateContactInfo(detail, pkt);
		pkt->version = RsUtil::retroshareVersion();

		pkt->PeerId(sslid);

#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::sendOwnContactInfo() sending:" << std::endl;
		pkt -> print(std::cerr);
		std::cerr  << std::endl;
#endif
		sendItem(pkt);
	}
}


void p3discovery2::recvOwnContactInfo(const SSLID &fromId, const RsDiscContactItem *item)
{

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::recvOwnContactInfo()";
	std::cerr << std::endl;

	std::cerr << "Info sent by the peer itself -> updating self info:" << std::endl;
	std::cerr << "  -> vs_disc      : " << item->vs_disc << std::endl;
	std::cerr << "  -> vs_dht       : " << item->vs_dht << std::endl;
	std::cerr << "  -> network mode : " << item->netMode << std::endl;
	std::cerr << "  -> location     : " << item->location << std::endl;
	std::cerr << std::endl;
#endif

	// Peer Own Info replaces the existing info, because the
	// peer is the primary source of his own IPs.

	mPeerMgr->setNetworkMode(fromId, item->netMode);
	mPeerMgr->setLocation(fromId, item->location);
	mPeerMgr->setVisState(fromId, item->vs_disc, item->vs_dht);

	setPeerVersion(fromId, item->version);
	updatePeerAddresses(item);

	// This information will be sent out to online peers, at the receipt of their PGPList.
	// It is important that PGPList is received after the OwnContactItem.
	// This should happen, but is not enforced by the protocol.

	// start peer list exchange.
	sendPGPList(fromId);

	// Update mDiscStatus.
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	PGPID pgpId = getPGPId(fromId);
	std::map<PGPID, DiscPgpInfo>::iterator it = mFriendList.find(pgpId);
	if (it != mFriendList.end())
	{
		std::map<SSLID, DiscSslInfo>::iterator sit = it->second.mSslIds.find(fromId);
		if (sit != it->second.mSslIds.end())
		{
			sit->second.mDiscStatus = item->vs_disc;
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::recvOwnContactInfo()";
			std::cerr << "updating mDiscStatus to: " << sit->second.mDiscStatus;
			std::cerr << std::endl;
#endif
		}
		else
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::recvOwnContactInfo()";
			std::cerr << " ERROR missing SSL Entry: " << fromId;
			std::cerr << std::endl;
#endif
		}
	}
	else
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::recvOwnContactInfo()";
		std::cerr << " ERROR missing PGP Entry: " << pgpId;
		std::cerr << std::endl;
#endif
	}

	// cleanup.
	delete item;
}

void p3discovery2::updatePeerAddresses(const RsDiscContactItem *item)
{
	if (item->isHidden)
	{
		mPeerMgr->setHiddenDomainPort(item->sslId, item->hiddenAddr, item->hiddenPort);
	}
	else
	{
		mPeerMgr->setLocalAddress(item->sslId, item->localAddrV4.addr);
		mPeerMgr->setExtAddress(item->sslId, item->extAddrV4.addr);
		mPeerMgr->setDynDNS(item->sslId, item->dyndns);

		updatePeerAddressList(item);
	}
}


void p3discovery2::updatePeerAddressList(const RsDiscContactItem *item)
{
	if (item->isHidden)
	{
	}
	else
	{
		pqiIpAddrSet addrsFromPeer;	
		addrsFromPeer.mLocal.extractFromTlv(item->localAddrList);
		addrsFromPeer.mExt.extractFromTlv(item->extAddrList);

#ifdef P3DISC_DEBUG
		std::cerr << "Setting address list to peer " << item->sslId << ", to be:" << std::endl ;

		std::string addrstr;
		addrsFromPeer.printAddrs(addrstr);
		std::cerr << addrstr;
		std::cerr << std::endl;
#endif
		mPeerMgr->updateAddressList(item->sslId, addrsFromPeer);
	}
}


// Starts the Discovery process.
// should only be called it DISC2_STATUS_NOT_HIDDEN(OwnInfo.status).
void p3discovery2::sendPGPList(const SSLID &toId)
{
	updatePgpFriendList();
	
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/


#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::sendPGPList() to " << toId;
	std::cerr << std::endl;
#endif

	RsDiscPgpListItem *pkt = new RsDiscPgpListItem();

	pkt->mode = DISC_PGP_LIST_MODE_FRIENDS;

	std::map<PGPID, DiscPgpInfo>::const_iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		pkt->pgpIdSet.ids.push_back(it->first);
	}

	pkt->PeerId(toId);

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::sendPGPList() sending:" << std::endl;
	pkt->print(std::cerr);
	std::cerr  << std::endl;
#endif

	sendItem(pkt);
}

void p3discovery2::updatePgpFriendList()
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::updatePgpFriendList()";
	std::cerr << std::endl;
#endif
	
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

#define PGP_MAX_UPDATE_PERIOD 300
	
	if (time(NULL) < mLastPgpUpdate + PGP_MAX_UPDATE_PERIOD )
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::updatePgpFriendList() Already uptodate";
		std::cerr << std::endl;
#endif
		return;
	}
	
	mLastPgpUpdate = time(NULL);
	
	std::list<PGPID> pgpList;
	std::set<PGPID> pgpSet;

	std::set<PGPID>::iterator sit;
	std::list<PGPID>::iterator lit;
	std::map<PGPID, DiscPgpInfo>::iterator it;
	
	PGPID ownPgpId = AuthGPG::getAuthGPG()->getGPGOwnId();
	AuthGPG::getAuthGPG()->getGPGAcceptedList(pgpList);
	pgpList.push_back(ownPgpId);
	
	// convert to set for ordering.
	for(lit = pgpList.begin(); lit != pgpList.end(); lit++)
	{
		pgpSet.insert(*lit);
	}
	
	std::list<PGPID> pgpToAdd;
	std::list<PGPID> pgpToRemove;
	

	sit = pgpSet.begin();
	it = mFriendList.begin();
	while (sit != pgpSet.end() && it != mFriendList.end())
	{
		if (*sit < it->first)
		{
			/* to add */
			pgpToAdd.push_back(*sit);
			++sit;
		}
		else if (it->first < *sit)
		{
			/* to remove */
			pgpToRemove.push_back(it->first);
			++it;
		}
		else 
		{
			/* same - okay */
			++sit;
			++it;
		}
	}
	
	/* more to add? */
	for(; sit != pgpSet.end(); sit++)
	{
		pgpToAdd.push_back(*sit);
	}
	
	for(; it != mFriendList.end(); it++)
	{
		/* more to remove */
		pgpToRemove.push_back(it->first);		
	}
	
	for(lit = pgpToRemove.begin(); lit != pgpToRemove.end(); lit++)
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::updatePgpFriendList() Removing pgpId: " << *lit;
		std::cerr << std::endl;
#endif
		
		it = mFriendList.find(*lit);
		mFriendList.erase(it);
	}

	for(lit = pgpToAdd.begin(); lit != pgpToAdd.end(); lit++)
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::updatePgpFriendList() Adding pgpId: " << *lit;
		std::cerr << std::endl;
#endif

		mFriendList[*lit] = DiscPgpInfo();
	}	

	/* finally install the pgpList on our own entry */
	DiscPgpInfo &ownInfo = mFriendList[ownPgpId];
	ownInfo.mergeFriendList(pgpList);

}




void p3discovery2::processPGPList(const SSLID &fromId, const RsDiscPgpListItem *item)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::processPGPList() from " << fromId;
	std::cerr << std::endl;
#endif

	std::map<PGPID, DiscPgpInfo>::iterator it;
	PGPID fromPgpId = getPGPId(fromId);	
	it = mFriendList.find(fromPgpId);
	if (it == mFriendList.end())
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::processPGPList() is not friend: " << fromId;
		std::cerr << std::endl;
#endif

		// cleanup.
		delete item;
		return;
	}

	bool requestUnknownPgpCerts = true;
	peerState pstate;
	mPeerMgr->getOwnNetStatus(pstate);
	if (pstate.vs_disc != RS_VS_DISC_FULL)
	{
		requestUnknownPgpCerts = false;
	}	
	
	uint32_t linkType = mLinkMgr->getLinkType(fromId);
	if ((linkType & RS_NET_CONN_SPEED_TRICKLE) || 
		(linkType & RS_NET_CONN_SPEED_LOW))
	{
		std::cerr << "p3discovery2::processPGPList() Not requesting Certificates from: " << fromId;
		std::cerr << " (low bandwidth)" << std::endl;
		requestUnknownPgpCerts = false;
	}

	if (requestUnknownPgpCerts)
	{
		std::list<PGPID>::const_iterator fit;
		for(fit = item->pgpIdSet.ids.begin(); fit != item->pgpIdSet.ids.end(); fit++)
		{
			if (!AuthGPG::getAuthGPG()->isGPGId(*fit))
			{
#ifdef P3DISC_DEBUG
				std::cerr << "p3discovery2::processPGPList() requesting PgpId: " << *fit;
				std::cerr << " from SslId: " << fromId;
				std::cerr << std::endl;
#endif
				requestPGPCertificate(*fit, fromId);
			}
		}
	}

	it->second.mergeFriendList(item->pgpIdSet.ids);
	updatePeers_locked(fromId);

	// cleanup.
	delete item;
}


/*
 *      -> Update Other Peers about B.
 *      -> Update B about Other Peers.
 */
void p3discovery2::updatePeers_locked(const SSLID &aboutId)
{

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::updatePeers_locked() about " << aboutId;
	std::cerr << std::endl;
#endif

	PGPID aboutPgpId = getPGPId(aboutId);

	std::map<PGPID, DiscPgpInfo>::const_iterator ait;
	ait = mFriendList.find(aboutPgpId);
	if (ait == mFriendList.end())
	{

#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::updatePeers_locked() PgpId is not a friend: " << aboutPgpId;
		std::cerr << std::endl;
#endif
		return;
	}

	std::set<PGPID> mutualFriends;
	std::set<SSLID> onlineFriends;
	std::set<SSLID>::const_iterator sit;
	
	const std::set<PGPID> &friendSet = ait->second.mFriendSet;
	std::set<PGPID>::const_iterator fit;
	for(fit = friendSet.begin(); fit != friendSet.end(); fit++)
	{

#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::updatePeer_locked() checking their friend: " << *fit;
		std::cerr  << std::endl;
#endif

		std::map<PGPID, DiscPgpInfo>::const_iterator ffit;
		ffit = mFriendList.find(*fit);
		if (ffit == mFriendList.end())
		{

#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::updatePeer_locked() Ignoring not our friend";
			std::cerr  << std::endl;
#endif
			// Not our friend, or we have no Locations (SSL) for this PGPID (same difference)
			continue;
		}

		if (ffit->second.mFriendSet.find(aboutPgpId) != ffit->second.mFriendSet.end())
		{

#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::updatePeer_locked() Adding as Mutual Friend";
			std::cerr  << std::endl;
#endif
			mutualFriends.insert(*fit);

			std::map<SSLID, DiscSslInfo>::const_iterator mit;
			for(mit = ffit->second.mSslIds.begin();
					mit != ffit->second.mSslIds.end(); mit++)
			{
				SSLID sslid = mit->first;
				if (mLinkMgr->isOnline(sslid))
				{
					// TODO IGNORE if sslid == aboutId, or sslid == ownId.
#ifdef P3DISC_DEBUG
					std::cerr << "p3discovery2::updatePeer_locked() Adding Online SSLID: " << sslid;
					std::cerr  << std::endl;
#endif
					onlineFriends.insert(sslid);
				}
			}
		}
	}

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::updatePeer_locked() Updating " << aboutId << " about Mutual Friends";
	std::cerr  << std::endl;
#endif
	// update aboutId about Other Peers.
	for(fit = mutualFriends.begin(); fit != mutualFriends.end(); fit++)
	{
		sendContactInfo_locked(*fit, aboutId);
	}

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::updatePeer_locked() Updating Online Peers about " << aboutId;
	std::cerr  << std::endl;
#endif
	// update Other Peers about aboutPgpId.
	for(sit = onlineFriends.begin(); sit != onlineFriends.end(); sit++)
	{
		// This could be more efficient, and only be specific about aboutId.
		// but we'll leave it like this for the moment.
		sendContactInfo_locked(aboutPgpId, *sit);
	}
}


void p3discovery2::sendContactInfo_locked(const PGPID &aboutId, const SSLID &toId)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::sendContactInfo_locked() aboutPGPId: " << aboutId << " toId: " << toId;
	std::cerr << std::endl;
#endif
	if (!(rsPeers->servicePermissionFlags_sslid(toId) & RS_SERVICE_PERM_DISCOVERY))
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::sendContactInfo_locked() discovery disabled for SSLID: " << toId;
		std::cerr << std::endl;
#endif
		return;
	}

	std::map<PGPID, DiscPgpInfo>::const_iterator it;
	it = mFriendList.find(aboutId);
	if (it == mFriendList.end())
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::sendContactInfo_locked() ERROR aboutId is not a friend";
		std::cerr << std::endl;
#endif
		return;
	}

	std::map<SSLID, DiscSslInfo>::const_iterator sit;
	for(sit = it->second.mSslIds.begin(); sit != it->second.mSslIds.end(); sit++)
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::sendContactInfo_locked() related sslId: " << sit->first;
		std::cerr << std::endl;
#endif

		if ((sit->first == rsPeers->getOwnId()) || (sit->first == toId))
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::processContactInfo() not sending info on self or theirself";
			std::cerr << std::endl;
#endif		
			continue;
		}

		if (sit->second.mDiscStatus != RS_VS_DISC_OFF)
		{
			peerState detail;
			if (mPeerMgr->getFriendNetStatus(sit->first, detail)) 
			{
				RsDiscContactItem *pkt = new RsDiscContactItem();
				populateContactInfo(detail, pkt);
				pkt->PeerId(toId);

#ifdef P3DISC_DEBUG
				std::cerr << "p3discovery2::sendContactInfo_locked() Sending";
				std::cerr << std::endl;
				pkt->print(std::cerr);
				std::cerr  << std::endl;
#endif
				sendItem(pkt);
			}
			else
			{
#ifdef P3DISC_DEBUG
				std::cerr << "p3discovery2::sendContactInfo_locked() No Net Status";
				std::cerr << std::endl;
#endif
			}
		}
		else
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::sendContactInfo_locked() SSLID Hidden";
			std::cerr << std::endl;
#endif
		}
	}
}


void p3discovery2::processContactInfo(const SSLID &fromId, const RsDiscContactItem *item)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	if (item->sslId == rsPeers->getOwnId())
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::processContactInfo(" << fromId << ") PGPID: ";
		std::cerr << item->pgpId << " Ignoring Info on self";
		std::cerr << std::endl;
#endif		
		return;
	}


	/* */
	std::map<PGPID, DiscPgpInfo>::iterator it;
	it = mFriendList.find(item->pgpId);
	if (it == mFriendList.end())
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::processContactInfo(" << fromId << ") PGPID: ";
		std::cerr << item->pgpId << " Not Friend.";
		std::cerr << std::endl;
		std::cerr << "p3discovery2::processContactInfo(" << fromId << ") THIS SHOULD NEVER HAPPEN!";
		std::cerr << std::endl;
#endif		
		
		/* THESE ARE OUR FRIEND OF FRIENDS ... pass this information along to NetMgr & DHT...
		 * as we can track FOF and use them as potential Proxies / Relays
		 */

		if (!item->isHidden)
		{
			/* add into NetMgr and non-search, so we can detect connect attempts */
			mNetMgr->netAssistFriend(item->sslId,false);

			/* inform NetMgr that we know this peer */
			mNetMgr->netAssistKnownPeer(item->sslId, item->extAddrV4.addr,
				NETASSIST_KNOWN_PEER_FOF | NETASSIST_KNOWN_PEER_OFFLINE);
		}
		return;
	}

	bool should_notify_discovery = false;
	std::map<SSLID, DiscSslInfo>::iterator sit;
	sit = it->second.mSslIds.find(item->sslId);
	if (sit == it->second.mSslIds.end())
	{
		/* insert! */
		DiscSslInfo sslInfo;
		it->second.mSslIds[item->sslId] = sslInfo;
		sit = it->second.mSslIds.find(item->sslId);

		should_notify_discovery = true;

		if (!mPeerMgr->isFriend(item->sslId))
		{
			// Add with no disc by default. If friend already exists, it will do nothing
			// NO DISC is important - otherwise, we'll just enter a nasty loop, 
			// where every addition triggers requests, then they are cleaned up, and readded...

			// This way we get their addresses, but don't advertise them until we get a
			// connection.
#ifdef P3DISC_DEBUG
			std::cerr << "--> Adding to friends list " << item->sslId << " - " << item->pgpId << std::endl;
#endif
			mPeerMgr->addFriend(item->sslId, item->pgpId, item->netMode, RS_VS_DISC_OFF, RS_VS_DHT_FULL,(time_t)0,RS_SERVICE_PERM_ALL); 
			updatePeerAddresses(item);
		}
	}

	updatePeerAddressList(item);


	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_NEIGHBOURS, NOTIFY_TYPE_MOD);

	if(should_notify_discovery)
		rsicontrol->getNotify().notifyDiscInfoChanged();
}



/* we explictly request certificates, instead of getting them all the time
 */
void p3discovery2::requestPGPCertificate(const PGPID &aboutId, const SSLID &toId)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::requestPGPCertificate() aboutId: " << aboutId << " to: " << toId;
	std::cerr << std::endl;
#endif
	
	RsDiscPgpListItem *pkt = new RsDiscPgpListItem();
	
	pkt->mode = DISC_PGP_LIST_MODE_GETCERT;
	pkt->pgpIdSet.ids.push_back(aboutId);		
	pkt->PeerId(toId);
	
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::requestPGPCertificate() sending:" << std::endl;
	pkt->print(std::cerr);
	std::cerr  << std::endl;
#endif

	sendItem(pkt);
}

 /* comment */
void p3discovery2::recvPGPCertificateRequest(const SSLID &fromId, const RsDiscPgpListItem *item)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::recvPPGPCertificateRequest() from " << fromId;
	std::cerr << std::endl;
#endif

	std::list<std::string>::const_iterator it;
	for(it = item->pgpIdSet.ids.begin(); it != item->pgpIdSet.ids.end(); it++)
	{
		// NB: This doesn't include own certificates? why not.
		// shouldn't be a real problem. Peer must have own PgpCert already.
		if (AuthGPG::getAuthGPG()->isGPGAccepted(*it))
		{
			sendPGPCertificate(*it, fromId);
		}
	}
	delete item;
}


void p3discovery2::sendPGPCertificate(const PGPID &aboutId, const SSLID &toId)
{

	/* for Relay Connections (and other slow ones) we don't want to 
	 * to waste bandwidth sending certificates. So don't add it.
	 */

	uint32_t linkType = mLinkMgr->getLinkType(toId);
	if ((linkType & RS_NET_CONN_SPEED_TRICKLE) || 
		(linkType & RS_NET_CONN_SPEED_LOW))
	{
		std::cerr << "p3disc::sendPGPCertificate() Not sending Certificates to: " << toId;
		std::cerr << " (low bandwidth)" << std::endl;
		return;
	}

	RsDiscPgpCertItem *item = new RsDiscPgpCertItem();
	item->pgpId = aboutId;
	item->PeerId(toId);
	
	
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::sendPGPCertificate() queuing for Cert generation:" << std::endl;
	item->print(std::cerr);
	std::cerr  << std::endl;
#endif

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
	/* queue it! */
	
	mPendingDiscPgpCertOutList.push_back(item);
}

						  
void p3discovery2::recvPGPCertificate(const SSLID &fromId, RsDiscPgpCertItem *item)
{
	
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::recvPGPCertificate() queuing for Cert loading" << std::endl;
	std::cerr  << std::endl;
#endif


	/* should only happen if in FULL Mode */
	peerState pstate;
	mPeerMgr->getOwnNetStatus(pstate);
	if (pstate.vs_disc != RS_VS_DISC_FULL)
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::recvPGPCertificate() Not Loading Certificates as in MINIMAL MODE";
		std::cerr << std::endl;
#endif

		delete item;
	}	

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
	/* push this back to be processed by pgp when possible */

	mPendingDiscPgpCertInList.push_back(item);
}
						  
				  
						  
						  

        /************* from pqiMonitor *******************/
void p3discovery2::statusChange(const std::list<pqipeer> &plist)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::statusChange()" << std::endl;
#endif

	std::list<pqipeer>::const_iterator pit;
	for(pit =  plist.begin(); pit != plist.end(); pit++) 
	{
		if ((pit->state & RS_PEER_S_FRIEND) && (pit->actions & RS_PEER_CONNECTED)) 
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::statusChange() Starting Disc with: " << pit->id << std::endl;
#endif
			sendOwnContactInfo(pit->id);
		} 
		else if (!(pit->state & RS_PEER_S_FRIEND) && (pit->actions & RS_PEER_MOVED)) 
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::statusChange() Removing Friend: " << pit->id << std::endl;
#endif
			removeFriend(pit->id);
		}
		else if ((pit->state & RS_PEER_S_FRIEND) && (pit->actions & RS_PEER_NEW)) 
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::statusChange() Adding Friend: " << pit->id << std::endl;
#endif
			addFriend(pit->id);
		}
	}
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::statusChange() finished." << std::endl;
#endif
	return;
}

		

	/*************************************************************************************/
	/*			   Extracting Network Graph Details			     */
	/*************************************************************************************/
bool p3discovery2::getDiscFriends(const std::string& id, std::list<std::string> &proxyIds)
{
	// This is treated appart, because otherwise we don't receive any disc info about us
	if(id == rsPeers->getOwnId()) // SSL id	
		return rsPeers->getFriendList(proxyIds) ;
		
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
		
	std::map<PGPID, DiscPgpInfo>::const_iterator it;
	PGPID pgp_id = getPGPId(id);
		
	it = mFriendList.find(pgp_id);
	if (it == mFriendList.end())
	{
		// ERROR.
		return false;
	}

	// For each of their friends that we know, grab that set of SSLIDs.
	const std::set<PGPID> &friendSet = it->second.mFriendSet;
	std::set<PGPID>::const_iterator fit;
	for(fit = friendSet.begin(); fit != friendSet.end(); fit++)
	{
		it = mFriendList.find(*fit);
		if (it == mFriendList.end())
		{
			continue;
		}

		std::map<SSLID, DiscSslInfo>::const_iterator sit;
		for(sit = it->second.mSslIds.begin();
				sit != it->second.mSslIds.end(); sit++)
		{
			proxyIds.push_back(sit->first);
		}
	}
	return true;
		
}
						  
						  
bool p3discovery2::getDiscPgpFriends(const PGPID &pgp_id, std::list<PGPID> &proxyPgpIds)
{
	/* find id -> and extract the neighbour_of ids */
		
	if(pgp_id == rsPeers->getGPGOwnId()) // SSL id			// This is treated appart, because otherwise we don't receive any disc info about us
		return rsPeers->getGPGAcceptedList(proxyPgpIds) ;
		
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
		
	std::map<PGPID, DiscPgpInfo>::const_iterator it;
	it = mFriendList.find(pgp_id);
	if (it == mFriendList.end())
	{
		// ERROR.
		return false;
	}
	
	std::set<PGPID>::const_iterator fit;
	for(fit = it->second.mFriendSet.begin(); fit != it->second.mFriendSet.end(); fit++)
	{
		proxyPgpIds.push_back(*fit);
	}
	return true;
}
						  
						  
bool p3discovery2::getPeerVersion(const SSLID &peerId, std::string &version)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<SSLID, DiscPeerInfo>::const_iterator it;
	it = mLocationMap.find(peerId);
	if (it == mLocationMap.end())
	{
		// MISSING.
		return false;
	}
	
	version = it->second.mVersion;
	return true;
}
						  
						  
bool p3discovery2::setPeerVersion(const SSLID &peerId, const std::string &version)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<SSLID, DiscPeerInfo>::iterator it;
	it = mLocationMap.find(peerId);
	if (it == mLocationMap.end())
	{
		mLocationMap[peerId] = DiscPeerInfo();
		it = mLocationMap.find(peerId);
	}

	it->second.mVersion = version;
	return true;
}
						  

bool p3discovery2::getWaitingDiscCount(unsigned int *sendCount, unsigned int *recvCount)
{
	if (sendCount == NULL && recvCount == NULL) {
		/* Nothing to do */
		return false;
	}

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	if (sendCount) {
		*sendCount = mPendingDiscPgpCertOutList.size();
	}

	if (recvCount) {
		*recvCount = mPendingDiscPgpCertInList.size();
	}
	return true;
}



/*************************************************************************************/
/*			AuthGPGService						     */
/*************************************************************************************/
AuthGPGOperation *p3discovery2::getGPGOperation()
{
	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		/* process disc reply in list */
		if (!mPendingDiscPgpCertInList.empty()) {
			RsDiscPgpCertItem *item = mPendingDiscPgpCertInList.front();
			mPendingDiscPgpCertInList.pop_front();

			return new AuthGPGOperationLoadOrSave(true, item->pgpId, item->pgpCert, item);
		}
	}

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		/* process disc reply in list */
		if (!mPendingDiscPgpCertOutList.empty()) {
			RsDiscPgpCertItem *item = mPendingDiscPgpCertOutList.front();
			mPendingDiscPgpCertOutList.pop_front();

			return new AuthGPGOperationLoadOrSave(false, item->pgpId, "", item);
		}
	}
	return NULL;
}

void p3discovery2::setGPGOperation(AuthGPGOperation *operation)
{
	AuthGPGOperationLoadOrSave *loadOrSave = dynamic_cast<AuthGPGOperationLoadOrSave*>(operation);
	if (loadOrSave) 
	{
		RsDiscPgpCertItem *item = (RsDiscPgpCertItem *) loadOrSave->m_userdata;
		if (!item)
		{
			return;
		}

		if (loadOrSave->m_load) 
		{
	
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::setGPGOperation() Loaded Cert" << std::endl;
			item->print(std::cerr, 5);
			std::cerr  << std::endl;
#endif
			// It has already been processed by PGP.
			delete item;
		} 
		else 
		{
			// Attaching Certificate.
			item->pgpCert = loadOrSave->m_certGpg;

#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::setGPGOperation() Sending Message:" << std::endl;
			item->print(std::cerr, 5);
#endif

			// Send off message
			sendItem(item);
		}
		return;
	}

	/* ignore other operations */
}

						  

#if 0
/***************************************************************************************/
/***************************************************************************************/
/************** OLD CODE ***************************/
/***************************************************************************************/
/***************************************************************************************/


#include "retroshare/rsiface.h"
#include "retroshare/rspeers.h"
#include "services/p3disc.h"

#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"

#include "pqi/authssl.h"
#include "pqi/authgpg.h"

#include <iostream>
#include <errno.h>
#include <cmath>

const uint32_t AUTODISC_LDI_SUBTYPE_PING = 0x01;
const uint32_t AUTODISC_LDI_SUBTYPE_RPLY = 0x02;

#include "util/rsdebug.h"
#include "util/rsprint.h"
#include "util/rsversion.h"

const int pqidisczone = 2482;

//static int convertTDeltaToTRange(double tdelta);
//static int convertTRangeToTDelta(int trange);

// Operating System specific includes.
#include "pqi/pqinetwork.h"

/* DISC FLAGS */

const uint32_t P3DISC_FLAGS_USE_DISC 		= 0x0001;
const uint32_t P3DISC_FLAGS_USE_DHT 		= 0x0002;
const uint32_t P3DISC_FLAGS_EXTERNAL_ADDR	= 0x0004;
const uint32_t P3DISC_FLAGS_STABLE_UDP	 	= 0x0008;
const uint32_t P3DISC_FLAGS_PEER_ONLINE 	= 0x0010;
const uint32_t P3DISC_FLAGS_OWN_DETAILS 	= 0x0020;
const uint32_t P3DISC_FLAGS_PEER_TRUSTS_ME 	= 0x0040;
const uint32_t P3DISC_FLAGS_ASK_VERSION		= 0x0080;


/*****
 * #define P3DISC_DEBUG 	1
 ****/

//#define P3DISC_DEBUG 	1

/******************************************************************************************
 ******************************  NEW DISCOVERY  *******************************************
 ******************************************************************************************
 *****************************************************************************************/

p3disc::p3disc(p3PeerMgr *pm, p3LinkMgr *lm, p3NetMgr *nm, pqipersongrp *pqih)
        :p3Service(RS_SERVICE_TYPE_DISC),
				 p3Config(CONFIG_TYPE_P3DISC),
		  mPeerMgr(pm), mLinkMgr(lm), mNetMgr(nm), 
		  mPqiPersonGrp(pqih), mDiscMtx("p3disc")
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	addSerialType(new RsDiscSerialiser());

	mLastSentHeartbeatTime = time(NULL);
	mDiscEnabled = true;

	//add own version to versions map
	versions[AuthSSL::getAuthSSL()->OwnId()] = RsUtil::retroshareVersion();
#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::p3disc() setup";
	std::cerr << std::endl;
#endif

	return;
}

int p3disc::tick()
{
        //send a heartbeat to all connected peers
	time_t hbTime;
	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
		hbTime = mLastSentHeartbeatTime;
	}

	if (time(NULL) - hbTime > HEARTBEAT_REPEAT_TIME) 
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::tick() sending heartbeat to all peers" << std::endl;
#endif

		std::list<std::string> peers;
		std::list<std::string>::const_iterator pit;

		mLinkMgr->getOnlineList(peers);
		for (pit = peers.begin(); pit != peers.end(); ++pit) 
		{
			sendHeartbeat(*pit);
		}

		/* check our Discovery flag */
		peerState detail;
		mPeerMgr->getOwnNetStatus(detail);

		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		mDiscEnabled = (!(detail.visState & RS_VIS_STATE_NODISC));
		mLastSentHeartbeatTime = time(NULL);
	}

	return handleIncoming();
}

int p3disc::handleIncoming()
{
	RsItem *item = NULL;

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::handleIncoming()" << std::endl;
#endif

	int nhandled = 0;
	// While messages read
	while(NULL != (item = recvItem()))
	{
		RsDiscAskInfo *inf = NULL;
		RsDiscReply *dri = NULL;
		RsDiscVersion *dvi = NULL;
		RsDiscHeartbeat *dta = NULL;

#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::handleIncoming() Received Message!" << std::endl;
		item -> print(std::cerr);
		std::cerr  << std::endl;
#endif

		// if discovery reply then respond if haven't already.
		if (NULL != (dri = dynamic_cast<RsDiscReply *> (item)))	
		{
			if(rsPeers->servicePermissionFlags_sslid(item->PeerId()) & RS_SERVICE_PERM_DISCOVERY)
				recvDiscReply(dri);
            else
                delete item ;
		}
		else if (NULL != (dvi = dynamic_cast<RsDiscVersion *> (item))) 
		{
			recvPeerVersionMsg(dvi);
			nhandled++;
			delete item;
		}
		else if (NULL != (inf = dynamic_cast<RsDiscAskInfo *> (item))) /* Ping */ 
		{
			if(rsPeers->servicePermissionFlags_sslid(item->PeerId()) & RS_SERVICE_PERM_DISCOVERY)
				recvAskInfo(inf);

			nhandled++;
			delete item;
		}
		else if (NULL != (dta = dynamic_cast<RsDiscHeartbeat *> (item))) 
		{
			recvHeartbeatMsg(dta);
			nhandled++ ;
			delete item;
		}
		else
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::handleIncoming() Unknown Received Message!" << std::endl;
			item -> print(std::cerr);
			std::cerr  << std::endl;
#endif
			delete item;
		}
	}

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::handleIncoming() finished." << std::endl;
#endif

	return nhandled;
}




        /************* from pqiMonitor *******************/
void p3disc::statusChange(const std::list<pqipeer> &plist)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::statusChange()" << std::endl;
#endif

	std::list<pqipeer>::const_iterator pit;
	/* if any have switched to 'connected' then we notify */
	for(pit =  plist.begin(); pit != plist.end(); pit++) 
	{
		if ((pit->state & RS_PEER_S_FRIEND) && (pit->actions & RS_PEER_CONNECTED)) 
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::statusChange() Starting Disc with: " << pit->id << std::endl;
#endif
			sendOwnVersion(pit->id);

			if(rsPeers->servicePermissionFlags_sslid(pit->id) & RS_SERVICE_PERM_DISCOVERY)
				sendAllInfoToJustConnectedPeer(pit->id);

			sendJustConnectedPeerInfoToAllPeer(pit->id);
		} 
		else if (!(pit->state & RS_PEER_S_FRIEND) && (pit->actions & RS_PEER_MOVED)) 
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::statusChange() Removing Friend: " << pit->id << std::endl;
#endif
			removeFriend(pit->id);
		}
		else if ((pit->state & RS_PEER_S_FRIEND) && (pit->actions & RS_PEER_NEW)) 
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::statusChange() Adding Friend: " << pit->id << std::endl;
#endif
			askInfoToAllPeers(pit->id);
		}
	}
#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::statusChange() finished." << std::endl;
#endif
	return;
}

void p3disc::sendAllInfoToJustConnectedPeer(const std::string &id)
{
	/* get a peer lists */

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() id: " << id << std::endl;
#endif

	std::list<std::string> friendIds;
	std::list<std::string>::iterator it;
	std::set<std::string> gpgIds;
	std::set<std::string>::iterator git;

	/* We send our full friends list - if we have Discovery Enabled */
	if (mDiscEnabled)
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() Discovery Enabled, sending Friend List" << std::endl;
#endif
		mLinkMgr->getFriendList(friendIds);

		/* send them a list of all friend's details */
		for(it = friendIds.begin(); it != friendIds.end(); it++) 
		{
			/* get details */
			peerState detail;
			if (!mPeerMgr->getFriendNetStatus(*it, detail)) 
			{
				/* major error! */
#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() No Info, Skipping: " << *it;
				std::cerr << std::endl;
#endif
				continue;
			}
	
			if (!(detail.visState & RS_VIS_STATE_NODISC)) 
			{
#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() Adding GPGID: " << detail.gpg_id;
				std::cerr << " (SSLID: " << *it << ")";
				std::cerr << std::endl;
#endif
				gpgIds.insert(detail.gpg_id);
			}
			else
			{
#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() DISC OFF for GPGID: " << detail.gpg_id;
				std::cerr << " (SSLID: " << *it << ")";
				std::cerr << std::endl;
#endif
			}
		}
	}

	//add own info, this info is sent whether discovery is enabled - or not.
	gpgIds.insert(rsPeers->getGPGOwnId());

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		/* refresh with new list */
		std::list<std::string> &idList = mSendIdList[id];
		idList.clear();
		for(git = gpgIds.begin(); git != gpgIds.end(); git++)
		{
			idList.push_back(*git);
		}
	}

        #ifdef P3DISC_DEBUG
	std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() finished." << std::endl;
        #endif
}

void p3disc::sendJustConnectedPeerInfoToAllPeer(const std::string &connectedPeerId)
{

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() connectedPeerId : " << connectedPeerId << std::endl;
#endif

	/* only ask info if discovery is on */
	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
		if (!mDiscEnabled)
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() Disc Disabled => NULL OP" << std::endl;
#endif
			return;
		}
	}

	/* get details */
	peerState detail;
	if (!mPeerMgr->getFriendNetStatus(connectedPeerId, detail)) 
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() No NetStatus => FAILED" << std::endl;
#endif
		return;
	}

	if (detail.visState & RS_VIS_STATE_NODISC)
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() Peer Disc Discable => NULL OP" << std::endl;
#endif
		return;
	}


	std::string gpg_connectedPeerId = rsPeers->getGPGId(connectedPeerId);
	std::list<std::string> onlineIds;

	/* get a peer lists */
	rsPeers->getOnlineList(onlineIds);

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		/* append gpg id's of all friend's to the sending list */

		std::list<std::string>::iterator it;
		for (it = onlineIds.begin(); it != onlineIds.end(); it++) 
			if(rsPeers->servicePermissionFlags_sslid(*it) & RS_SERVICE_PERM_DISCOVERY)
			{
				std::list<std::string> &idList = mSendIdList[*it];

				if (std::find(idList.begin(), idList.end(), gpg_connectedPeerId) == idList.end()) 
				{
#ifdef P3DISC_DEBUG
					std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() adding to queue for: ";
					std::cerr << *it << std::endl;
#endif
					idList.push_back(gpg_connectedPeerId);
				}
				else
				{
#ifdef P3DISC_DEBUG
					std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() already in queue for: ";
					std::cerr << *it << std::endl;
#endif
				}
			}
	}
}


bool    isDummyFriend(const std::string &id) 
{
        bool ret = (id.substr(0,5) == "dummy");
	return ret;
}

 /* (dest (to), source (cert)) */
RsDiscReply *p3disc::createDiscReply(const std::string &to, const std::string &about)
{

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::createDiscReply() called. Sending details of: " << about << " to: " << to << std::endl;
#endif

	std::string aboutGpgId = rsPeers->getGPGId(about);
	if (aboutGpgId.empty()) {
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::createDiscReply() no info about this id" << std::endl;
#endif
		return NULL;
	}


	// Construct a message
	RsDiscReply *di = new RsDiscReply();

	// Fill the message
	// Set Target as input cert.
	di -> PeerId(to);
	di -> aboutId = aboutGpgId;

	// set the ip addresse list.
	std::list<std::string> sslChilds;
	rsPeers->getAssociatedSSLIds(aboutGpgId, sslChilds);
	bool shouldWeSendGPGKey = false;//the GPG key is send only if we've got a valid friend with DISC enabled

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
		if (!mDiscEnabled)
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::createDiscReply() Disc Disabled, removing all friend SSL Ids";
			std::cerr << std::endl;
#endif
			sslChilds.clear();
		}
	}

	std::list<std::string>::iterator it;
	for (it = sslChilds.begin(); it != sslChilds.end(); it++) 
	{
		/* skip dummy ones - until they are removed fully */
		if (isDummyFriend(*it))
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::createDiscReply() Skipping Dummy Child SSL Id:" << *it;
			std::cerr << std::endl;
#endif
			continue;
		}

		peerState detail;
		if (!mPeerMgr->getFriendNetStatus(*it, detail) 
			|| detail.visState & RS_VIS_STATE_NODISC)
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::createDiscReply() Skipping cos No Details or NODISC flag id: " << *it;
			std::cerr << std::endl;
#endif
			continue;
		}

#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::createDiscReply() Found Child SSL Id:" << *it;
		std::cerr << std::endl;
#endif

#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::createDiscReply() Adding Child SSL Id Details";
		std::cerr << std::endl;
#endif
		shouldWeSendGPGKey = true;

		RsPeerNetItem rsPeerNetItem ;
		rsPeerNetItem.clear();

		rsPeerNetItem.pid = detail.id;
		rsPeerNetItem.gpg_id = detail.gpg_id;
		rsPeerNetItem.location = detail.location;
		rsPeerNetItem.netMode = detail.netMode;
		rsPeerNetItem.visState = detail.visState;
		rsPeerNetItem.lastContact = detail.lastcontact;
		rsPeerNetItem.currentlocaladdr = detail.localaddr;
		rsPeerNetItem.currentremoteaddr = detail.serveraddr;
		rsPeerNetItem.dyndns = detail.dyndns;
		detail.ipAddrs.mLocal.loadTlv(rsPeerNetItem.localAddrList);
		detail.ipAddrs.mExt.loadTlv(rsPeerNetItem.extAddrList);


		di->rsPeerList.push_back(rsPeerNetItem);
	}


	//send own details
	if (about == rsPeers->getGPGOwnId()) 
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::createDiscReply() Adding Own Id Details";
		std::cerr << std::endl;
#endif
		peerState detail;
		if (mPeerMgr->getOwnNetStatus(detail)) 
		{
			shouldWeSendGPGKey = true;
			RsPeerNetItem rsPeerNetItem ;
			rsPeerNetItem.clear();
			rsPeerNetItem.pid = detail.id;
			rsPeerNetItem.gpg_id = detail.gpg_id;
			rsPeerNetItem.location = detail.location;
			rsPeerNetItem.netMode = detail.netMode;
			rsPeerNetItem.visState = detail.visState;
			rsPeerNetItem.lastContact = time(NULL);
			rsPeerNetItem.currentlocaladdr = detail.localaddr;
			rsPeerNetItem.currentremoteaddr = detail.serveraddr;
			rsPeerNetItem.dyndns = detail.dyndns;
			detail.ipAddrs.mLocal.loadTlv(rsPeerNetItem.localAddrList);
			detail.ipAddrs.mExt.loadTlv(rsPeerNetItem.extAddrList);

			di->rsPeerList.push_back(rsPeerNetItem);
		}
	}

	if (!shouldWeSendGPGKey) {
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::createDiscReply() GPG key should not be send, no friend with disc on found about it." << std::endl;
#endif
		// cleanup!
		delete di;
		return NULL;
	}

	return di;
}

void p3disc::sendOwnVersion(std::string to)
{
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::sendOwnVersion() Sending rs version to: " << to << std::endl;
#endif

	/* only ask info if discovery is on */
	if (!mDiscEnabled)
	{
#ifdef P3DISC_DEBUG
	        std::cerr << "p3disc::sendOwnVersion() Disc Disabled => NULL OP" << std::endl;
#endif
		return;
	}


	RsDiscVersion *di = new RsDiscVersion();
	di->PeerId(to);
	di->version = RsUtil::retroshareVersion();

	/* send the message */
	sendItem(di);

#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::sendOwnVersion() finished." << std::endl;
#endif
}

void p3disc::sendHeartbeat(std::string to)
{
    {
                std::string out = "p3disc::sendHeartbeat() to : " + to;
#ifdef P3DISC_DEBUG
                std::cerr << out << std::endl;
#endif
        rslog(RSL_WARNING, pqidisczone, out);
        }


        RsDiscHeartbeat *di = new RsDiscHeartbeat();
        di->PeerId(to);

        /* send the message */
        sendItem(di);

#ifdef P3DISC_DEBUG
        std::cerr << "Sent tick Message" << std::endl;
#endif
}

void p3disc::askInfoToAllPeers(std::string about)
{

#ifdef P3DISC_DEBUG
        std::cerr <<"p3disc::askInfoToAllPeers() about " << about << std::endl;
#endif

	// We Still Ask even if Disc isn't Enabled... if they want to give us the info ;)


        peerState connectState;
        if (!mPeerMgr->getFriendNetStatus(about, connectState) || (connectState.visState & RS_VIS_STATE_NODISC)) 
		{
#ifdef P3DISC_DEBUG
            std::cerr << "p3disc::askInfoToAllPeers() friend disc is off, don't send the request." << std::endl;
#endif
            return;
        }

        std::string aboutGpgId = rsPeers->getGPGId(about);
        if (aboutGpgId == "") 
		{
#ifdef P3DISC_DEBUG
            std::cerr << "p3disc::askInfoToAllPeers() no gpg id found" << std::endl;
#endif
        }

        std::list<std::string> onlineIds;
        std::list<std::string>::iterator it;

        rsPeers->getOnlineList(onlineIds);

        /* ask info to trusted friends */
        for(it = onlineIds.begin(); it != onlineIds.end(); it++) 
		{
                RsDiscAskInfo *di = new RsDiscAskInfo();
                di->PeerId(*it);
                di->gpg_id = aboutGpgId; 
                sendItem(di);
#ifdef P3DISC_DEBUG
                std::cerr << "p3disc::askInfoToAllPeers() question sent to : " << *it << std::endl;
#endif
        }
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::askInfoToAllPeers() finished." << std::endl;
#endif
}

void p3disc::recvPeerDetails(RsDiscReply *item, const std::string &certGpgId)
{

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::recvPeerFriendMsg() From: " << item->PeerId() << " About " << item->aboutId << std::endl;
#endif

	if (certGpgId.empty()) 
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::recvPeerFriendMsg() gpg cert Id is empty - cert not transmitted" << std::endl;
#endif
	}
	else if (item->aboutId == "" || item->aboutId != certGpgId) 
	{
		std::cerr << "p3disc::recvPeerFriendMsg() Error : about id is not the same as gpg id." << std::endl;
		return;
	}

	bool should_notify_discovery = false ;
	std::string item_gpg_id = rsPeers->getGPGId(item->PeerId()) ;

	for (std::list<RsPeerNetItem>::iterator pit = item->rsPeerList.begin(); pit != item->rsPeerList.end(); pit++) 
	{
		if(isDummyFriend(pit->pid)) 
		{
			continue;
		}

		bool new_info = false;
		addDiscoveryData(item->PeerId(), pit->pid,item_gpg_id,
				item->aboutId, pit->currentlocaladdr, pit->currentremoteaddr, 0, time(NULL),new_info);

		if(new_info)
			should_notify_discovery = true ;

#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::recvPeerFriendMsg() Peer Config Item:" << std::endl;
		pit->print(std::cerr, 10);
		std::cerr << std::endl;
#endif

		if (pit->pid != rsPeers->getOwnId())
		{
			// Apparently, the connect manager won't add a friend if the gpg id is not
			// trusted. However, this should be tested here for consistency and security 
			// in case of modifications in mConnMgr.
			//

			// Check if already friend.
			if(AuthGPG::getAuthGPG()->isGPGAccepted(pit->gpg_id) ||  pit->gpg_id == AuthGPG::getAuthGPG()->getGPGOwnId())
			{
				if (!mPeerMgr->isFriend(pit->pid))
				{
					// Add with no disc by default. If friend already exists, it will do nothing
					// NO DISC is important - otherwise, we'll just enter a nasty loop, 
					// where every addition triggers requests, then they are cleaned up, and readded...

					// This way we get their addresses, but don't advertise them until we get a
					// connection.
#ifdef P3DISC_DEBUG
					std::cerr << "--> Adding to friends list " << pit->pid << " - " << pit->gpg_id << std::endl;
#endif
					mPeerMgr->addFriend(pit->pid, pit->gpg_id, pit->netMode, RS_VIS_STATE_NODISC,(time_t)0,RS_SERVICE_PERM_ALL); 
				}
			}

			/* skip if not one of our peers */
			if (!mPeerMgr->isFriend(pit->pid))
			{
				/* THESE ARE OUR FRIEND OF FRIENDS ... pass this information along to NetMgr & DHT...
				 * as we can track FOF and use them as potential Proxies / Relays
				 */

				/* add into NetMgr and non-search, so we can detect connect attempts */
				mNetMgr->netAssistFriend(pit->pid,false);

				/* inform NetMgr that we know this peer */
				mNetMgr->netAssistKnownPeer(pit->pid, pit->currentremoteaddr, 
						NETASSIST_KNOWN_PEER_FOF | NETASSIST_KNOWN_PEER_OFFLINE);

				continue;
			}

			if (item->PeerId() == pit->pid) 
			{
#ifdef P3DISC_DEBUG
				std::cerr << "Info sent by the peer itself -> updating self info:" << std::endl;
				std::cerr << "  -> current local  addr = " << pit->currentlocaladdr << std::endl;
				std::cerr << "  -> current remote addr = " << pit->currentremoteaddr << std::endl;
				std::cerr << "  -> visState            =  " << std::hex << pit->visState << std::dec;
				std::cerr << "   -> network mode: " << pit->netMode << std::endl;
				std::cerr << "   ->     location: " << pit->location << std::endl;
				std::cerr << std::endl;
#endif

				bool peerDataChanged = false;

				// When the peer sends his own list of IPs, the info replaces the existing info, because the
				// peer is the primary source of his own IPs.
				if (mPeerMgr->setNetworkMode(pit->pid, pit->netMode)) {
					peerDataChanged = true;
				}
				if (mPeerMgr->setLocation(pit->pid, pit->location)) {
					peerDataChanged = true;
				}
				if (mPeerMgr->setLocalAddress(pit->pid, pit->currentlocaladdr)) {
					peerDataChanged = true;
				}
				if (mPeerMgr->setExtAddress(pit->pid, pit->currentremoteaddr)) {
					peerDataChanged = true;
				}
				if (mPeerMgr->setVisState(pit->pid, pit->visState)) {
					peerDataChanged = true;
				}
				if (mPeerMgr->setDynDNS(pit->pid, pit->dyndns)) {
					peerDataChanged = true;
				}

				if (peerDataChanged == true)
				{
					// inform all connected peers of change
					sendJustConnectedPeerInfoToAllPeer(pit->pid);
				}
			}

			// always update historical address list... this should be enough to let us connect.

			pqiIpAddrSet addrsFromPeer;	
			addrsFromPeer.mLocal.extractFromTlv(pit->localAddrList);
			addrsFromPeer.mExt.extractFromTlv(pit->extAddrList);

#ifdef P3DISC_DEBUG
			std::cerr << "Setting address list to peer " << pit->pid << ", to be:" << std::endl ;

			addrsFromPeer.printAddrs(std::cerr);
			std::cerr << std::endl;
#endif
			mPeerMgr->updateAddressList(pit->pid, addrsFromPeer);

		}
#ifdef P3DISC_DEBUG
		else
		{
			std::cerr << "Skipping info about own id " << pit->pid << std::endl ;
		}
#endif

	}

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_NEIGHBOURS, NOTIFY_TYPE_MOD);

	if(should_notify_discovery)
		rsicontrol->getNotify().notifyDiscInfoChanged();

	/* cleanup (handled by caller) */
}

void p3disc::recvPeerVersionMsg(RsDiscVersion *item)
{
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::recvPeerVersionMsg() From: " << item->PeerId();
        std::cerr << std::endl;
#endif

        // dont need protection
        versions[item->PeerId()] = item->version;

        return;
}

void p3disc::recvHeartbeatMsg(RsDiscHeartbeat *item)
{
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::recvHeartbeatMsg() From: " << item->PeerId();
        std::cerr << std::endl;
#endif

        mPqiPersonGrp->tagHeartbeatRecvd(item->PeerId());

        return;
}

void p3disc::recvAskInfo(RsDiscAskInfo *item) 
{
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::recvAskInfo() From: " << item->PeerId();
        std::cerr << std::endl;
#endif
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	/* only provide info if discovery is on */
	if (!mDiscEnabled)
	{
#ifdef P3DISC_DEBUG
        	std::cerr << "p3disc::recvAskInfo() Disc Disabled => NULL OP";
        	std::cerr << std::endl;
#endif
		return;
	}

        std::list<std::string> &idList = mSendIdList[item->PeerId()];

        if (std::find(idList.begin(), idList.end(), item->gpg_id) == idList.end()) {
            idList.push_back(item->gpg_id);
        }
}

void p3disc::recvDiscReply(RsDiscReply *dri)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::recvDiscReply() From: " << dri->PeerId() << " About: " << dri->aboutId;
	std::cerr << std::endl;
#endif

	/* search pending item and remove it, when already exist */
	std::list<RsDiscReply*>::iterator it;
	for (it = mPendingDiscReplyInList.begin(); it != mPendingDiscReplyInList.end(); it++) 
	{
		if ((*it)->PeerId() == dri->PeerId() && (*it)->aboutId == dri->aboutId) 
		{
			delete (*it);
			mPendingDiscReplyInList.erase(it);
			break;
		}
	}

	// add item to list for later process

	if(mDiscEnabled || dri->aboutId == rsPeers->getGPGId(dri->PeerId()))
		mPendingDiscReplyInList.push_back(dri); // no delete
	else
		delete dri ;
}




void p3disc::removeFriend(std::string /*ssl_id*/)
{
}

/*************************************************************************************/
/*			AuthGPGService						     */
/*************************************************************************************/
AuthGPGOperation *p3disc::getGPGOperation()
{
	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		/* process disc reply in list */
		if (mPendingDiscReplyInList.empty() == false) {
			RsDiscReply *item = mPendingDiscReplyInList.front();

			return new AuthGPGOperationLoadOrSave(true, item->aboutId, item->certGPG, item);
		}
	}

	/* process disc reply out list */

	std::string destId;
	std::string srcId;

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		while (!mSendIdList.empty()) 
		{
			std::map<std::string, std::list<std::string> >::iterator it = mSendIdList.begin();

			if (!it->second.empty() && mLinkMgr->isOnline(it->first)) {
				std::string gpgId = it->second.front();
				it->second.pop_front();

				destId = it->first;
				srcId = gpgId;

				break;
			} else {
				/* peer is not online anymore ... try next */
				mSendIdList.erase(it);
			}
		}
	}

	if (!destId.empty() && !srcId.empty()) {
		RsDiscReply *item = createDiscReply(destId, srcId);
		if (item) {
			return new AuthGPGOperationLoadOrSave(false, item->aboutId, "", item);
		}
	}

	return NULL;
}

void p3disc::setGPGOperation(AuthGPGOperation *operation)
{
	AuthGPGOperationLoadOrSave *loadOrSave = dynamic_cast<AuthGPGOperationLoadOrSave*>(operation);
	if (loadOrSave) {
		if (loadOrSave->m_load) {
			/* search in pending in list */
			RsDiscReply *item = NULL;

			{
				RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

				std::list<RsDiscReply*>::iterator it;
				it = std::find(mPendingDiscReplyInList.begin(), mPendingDiscReplyInList.end(), loadOrSave->m_userdata);
				if (it != mPendingDiscReplyInList.end()) {
					item = *it;
					mPendingDiscReplyInList.erase(it);
				}
			}

			if (item) {
				recvPeerDetails(item, loadOrSave->m_certGpgId);
				delete item;
			}
		} else {
			RsDiscReply *item = (RsDiscReply*) loadOrSave->m_userdata;

			if (item) 
			{
// It is okay to send an empty certificate! - This is to reduce the network load at connection time.
// Hopefully, we'll get the stripped down certificates working soon!... even then still be okay to send null.
#if 0 
				if (loadOrSave->m_certGpg.empty()) 
				{
#ifdef P3DISC_DEBUG
					std::cerr << "p3disc::setGPGOperation() don't send details because the gpg cert is not good" << std::endl;
#endif
					delete item;
					return;
				}
#endif

				/* for Relay Connections (and other slow ones) we don't want to 
				 * to waste bandwidth sending certificates. So don't add it.
				 */

				uint32_t linkType = mLinkMgr->getLinkType(item->PeerId());
				if ((linkType & RS_NET_CONN_SPEED_TRICKLE) || 
					(linkType & RS_NET_CONN_SPEED_LOW))
				{
					std::cerr << "p3disc::setGPGOperation() Send DiscReply Packet to: ";
					std::cerr << item->PeerId();
					std::cerr << " without Certificate (low bandwidth)" << std::endl;
				}
				else
				{
					// Attaching Certificate.
					item->certGPG = loadOrSave->m_certGpg;
				}

#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::setGPGOperation() About to Send Message:" << std::endl;
				item->print(std::cerr, 5);
#endif

				// Send off message
				sendItem(item);

#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::cbkGPGOperationSave() discovery reply sent." << std::endl;
#endif
			}
		}
		return;
	}

	/* ignore other operations */
}

/*************************************************************************************/
/*				Storing Network Graph				     */
/*************************************************************************************/
int	p3disc::addDiscoveryData(const std::string& fromId, const std::string& aboutId,const std::string& from_gpg_id,const std::string& about_gpg_id, const struct sockaddr_in& laddr, const struct sockaddr_in& raddr, uint32_t flags, time_t ts,bool& new_info)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	new_info = false ;

	gpg_neighbors[from_gpg_id].insert(about_gpg_id) ;

#ifdef P3DISC_DEBUG
	std::cerr << "Adding discovery data " << fromId << " - " << aboutId << std::endl ;
#endif
	/* Store Network information */
	std::map<std::string, autoneighbour>::iterator it;
	if (neighbours.end() == (it = neighbours.find(aboutId)))
	{
		/* doesn't exist */
		autoneighbour an;

		/* an data */
		an.id = aboutId;
		an.validAddrs = false;
		an.discFlags = 0;
		an.ts = 0;

		neighbours[aboutId] = an;

		it = neighbours.find(aboutId);
		new_info = true ;
	}

	/* it always valid */

	/* just update packet */

	autoserver as;

	as.id = fromId;
	as.localAddr = laddr;
	as.remoteAddr = raddr;
	as.discFlags = flags;
	as.ts = ts;

	bool authDetails = (as.id == it->second.id);

	/* KEY decision about address */
	if ((authDetails) ||
		((!(it->second.authoritative)) && (as.ts > it->second.ts)))
	{
		/* copy details to an */
		it->second.authoritative = authDetails;
		it->second.ts = as.ts;
		it->second.validAddrs = true;
		it->second.localAddr = as.localAddr;
		it->second.remoteAddr = as.remoteAddr;
		it->second.discFlags = as.discFlags;
	}

	if(it->second.neighbour_of.find(fromId) == it->second.neighbour_of.end())
	{
		(it->second).neighbour_of[fromId] = as;
		new_info =true ;
	}

	/* do we update network address info??? */
	return 1;

}



/*************************************************************************************/
/*			   Extracting Network Graph Details			     */
/*************************************************************************************/
bool p3disc::potentialproxies(const std::string& id, std::list<std::string> &proxyIds)
{
	/* find id -> and extract the neighbour_of ids */

	if(id == rsPeers->getOwnId()) // SSL id			// This is treated appart, because otherwise we don't receive any disc info about us
		return rsPeers->getFriendList(proxyIds) ;

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, autoneighbour>::iterator it;
	std::map<std::string, autoserver>::iterator sit;
	if (neighbours.end() == (it = neighbours.find(id)))
	{
		return false;
	}

	for(sit = it->second.neighbour_of.begin(); sit != it->second.neighbour_of.end(); sit++)
	{
		proxyIds.push_back(sit->first);
	}
	return true;
}


bool p3disc::potentialGPGproxies(const std::string& gpg_id, std::list<std::string> &proxyGPGIds)
{
	/* find id -> and extract the neighbour_of ids */

	if(gpg_id == rsPeers->getGPGOwnId()) // SSL id			// This is treated appart, because otherwise we don't receive any disc info about us
		return rsPeers->getGPGAcceptedList(proxyGPGIds) ;

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, std::set<std::string> >::iterator it = gpg_neighbors.find(gpg_id) ;

	if(it == gpg_neighbors.end())
		return false;

	for(std::set<std::string>::const_iterator sit(it->second.begin()); sit != it->second.end(); ++sit)
		proxyGPGIds.push_back(*sit);

	return true;
}

void p3disc::getversions(std::map<std::string, std::string> &versions)
{
	versions = this->versions;
}

void p3disc::getWaitingDiscCount(unsigned int *sendCount, unsigned int *recvCount)
{
	if (sendCount == NULL && recvCount == NULL) {
		/* Nothing to do */
		return;
	}

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	if (sendCount) {
		*sendCount = 0;

		std::map<std::string, std::list<std::string> >::iterator it;
		for (it = mSendIdList.begin(); it != mSendIdList.end(); it++) {
			*sendCount += it->second.size();
		}
	}

	if (recvCount) {
		*recvCount = mPendingDiscReplyInList.size();
	}
}

#ifdef UNUSED_CODE
int p3disc::idServers()
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, autoneighbour>::iterator nit;
	std::map<std::string, autoserver>::iterator sit;
	int cts = time(NULL);

	std::string out = "::::AutoDiscovery Neighbours::::\n";
	for(nit = neighbours.begin(); nit != neighbours.end(); nit++)
	{
		out += "Neighbour: " + (nit->second).id + "\n";
		rs_sprintf_append(out, "-> LocalAddr: %s:%u\n", rs_inet_ntoa(nit->second.localAddr.sin_addr).c_str(), ntohs(nit->second.localAddr.sin_port));
		rs_sprintf_append(out, "-> RemoteAddr: %s:%u\n", rs_inet_ntoa(nit->second.remoteAddr.sin_addr).c_str(), ntohs(nit->second.remoteAddr.sin_port));
		rs_sprintf_append(out, "  Last Contact: %ld sec ago\n", cts - (nit->second.ts));

		rs_sprintf_append(out, " -->DiscFlags: 0x%x\n", nit->second.discFlags);

		for(sit = (nit->second.neighbour_of).begin();
				sit != (nit->second.neighbour_of).end(); sit++)
		{
			out += "\tConnected via: " + (sit->first) + "\n";
			rs_sprintf_append(out, "\t\tLocalAddr: %s:%u\n", rs_inet_ntoa(sit->second.localAddr.sin_addr).c_str(), ntohs(sit->second.localAddr.sin_port));
			rs_sprintf_append(out, "\t\tRemoteAddr: %s:%u\n", rs_inet_ntoa(sit->second.remoteAddr.sin_addr).c_str(), ntohs(sit->second.remoteAddr.sin_port));

			rs_sprintf_append(out, "\t\tLast Contact: %ld sec ago\n", cts - (sit->second.ts));
			rs_sprintf_append(out, "\t\tDiscFlags: 0x%x\n", sit->second.discFlags);
		}
	}

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::idServers()" << std::endl;
	std::cerr << out;
#endif

	return 1;
}
#endif

// tdelta     -> trange.
// -inf...<0	   0 (invalid)
//    0.. <9       1
//    9...<99      2
//   99...<999	   3
//  999...<9999    4
//  etc...

//int convertTDeltaToTRange(double tdelta)
//{
//	if (tdelta < 0)
//		return 0;
//	int trange = 1 + (int) log10(tdelta + 1.0);
//	return trange;
//
//}

// trange     -> tdelta
// -inf...0	  -1 (invalid)
//    1            8
//    2           98
//    3          998
//    4         9998
//  etc...

//int convertTRangeToTDelta(int trange)
//{
//	if (trange <= 0)
//		return -1;
//
//	return (int) (pow(10.0, trange) - 1.5); // (int) xxx98.5 -> xxx98
//}

#endif // #if 0
