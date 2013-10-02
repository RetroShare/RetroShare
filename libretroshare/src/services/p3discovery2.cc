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

/****
 * #define P3DISC_DEBUG	1
 ****/

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
			std::cerr << "p3discovery2::handleIncoming() Unknown Received Message!" << std::endl;
			item -> print(std::cerr);
			std::cerr  << std::endl;
#endif
			delete item;
		}
	}

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::handleIncoming() finished." << std::endl;
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
		std::cerr << "p3discovery2::sendPGPCertificate() Not sending Certificates to: " << toId;
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
		std::cerr << "p3discovery2::recvPGPCertificate() Not Loading Certificates as in MINIMAL MODE";
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

						  
