/*******************************************************************************
 * libretroshare/src/services: p3discovery2.cc                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2013  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "services/p3discovery2.h"
#include "pqi/p3peermgr.h"
#include "retroshare/rsversion.h"
#include "retroshare/rsiface.h"
#include "rsserver/p3face.h"
#include "util/rsdebug.h"
#include "pqi/authssl.h"


/*extern*/ RsDisc* rsDisc = nullptr;

RsDisc::~RsDisc() {};

/****
 * #define P3DISC_DEBUG	1
 ****/

static bool populateContactInfo( const peerState &detail,
                                 RsDiscContactItem *pkt,
                                 bool include_ip_information )
{
	pkt->clear();

	pkt->pgpId = detail.gpg_id;
	pkt->sslId = detail.id;
	pkt->location = detail.location;
	pkt->version = "";
	pkt->netMode = detail.netMode;
	pkt->vs_disc = detail.vs_disc;
	pkt->vs_dht = detail.vs_dht;

	pkt->lastContact = time(nullptr);

	if (detail.hiddenNode)
	{
		pkt->isHidden = true;
		pkt->hiddenAddr = detail.hiddenDomain;
		pkt->hiddenPort = detail.hiddenPort;
	}
	else
	{
		pkt->isHidden = false;

		if(include_ip_information)
		{
			pkt->localAddrV4.addr = detail.localaddr;
			pkt->extAddrV4.addr = detail.serveraddr;
			sockaddr_storage_clear(pkt->localAddrV6.addr);
			sockaddr_storage_clear(pkt->extAddrV6.addr);

			pkt->dyndns = detail.dyndns;
			detail.ipAddrs.mLocal.loadTlv(pkt->localAddrList);
			detail.ipAddrs.mExt.loadTlv(pkt->extAddrList);
		}
		else
		{
			sockaddr_storage_clear(pkt->localAddrV6.addr);
			sockaddr_storage_clear(pkt->extAddrV6.addr);
			sockaddr_storage_clear(pkt->localAddrV4.addr);
			sockaddr_storage_clear(pkt->extAddrV4.addr);
		}
	}

	return true;
}

void DiscPgpInfo::mergeFriendList(const std::set<PGPID> &friends)
{
    std::set<PGPID>::const_iterator it;
	for(it = friends.begin(); it != friends.end(); ++it)
	{
		mFriendSet.insert(*it);
	}
}


p3discovery2::p3discovery2(
        p3PeerMgr* peerMgr, p3LinkMgr* linkMgr, p3NetMgr* netMgr,
        p3ServiceControl* sc, RsGixs* gixs ) :
    p3Service(), mPeerMgr(peerMgr), mLinkMgr(linkMgr), mNetMgr(netMgr),
    mServiceCtrl(sc), mGixs(gixs), mDiscMtx("p3discovery2"), mLastPgpUpdate(0)
{
	Dbg3() << __PRETTY_FUNCTION__ << std::endl;

	RS_STACK_MUTEX(mDiscMtx);
	addSerialType(new RsDiscSerialiser());

	// Add self into PGP FriendList.
	mFriendList[AuthGPG::getAuthGPG()->getGPGOwnId()] = DiscPgpInfo();

	rsEvents->registerEventsHandler(
	            [this](const RsEvent& event){ rsEventsHandler(event); },
	            mRsEventsHandle );
}


const std::string DISCOVERY_APP_NAME = "disc";
const uint16_t DISCOVERY_APP_MAJOR_VERSION  =       1;
const uint16_t DISCOVERY_APP_MINOR_VERSION  =       0;
const uint16_t DISCOVERY_MIN_MAJOR_VERSION  =       1;
const uint16_t DISCOVERY_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3discovery2::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_TYPE_DISC,
                DISCOVERY_APP_NAME,
                DISCOVERY_APP_MAJOR_VERSION,
                DISCOVERY_APP_MINOR_VERSION,
                DISCOVERY_MIN_MAJOR_VERSION,
                DISCOVERY_MIN_MINOR_VERSION);
}

p3discovery2::~p3discovery2()
{ rsEvents->unregisterEventsHandler(mRsEventsHandle); }

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
	RsItem* item = nullptr;

	int nhandled = 0;
	// While messages read
	while(nullptr != (item = recvItem()))
	{
		RsDiscPgpListItem* pgplist = nullptr;
		RsDiscPgpCertItem* pgpcert = nullptr;
		RsDiscContactItem* contact = nullptr;
		RsDiscIdentityListItem* gxsidlst = nullptr;
		RsGossipDiscoveryInviteItem* invite = nullptr;
		RsGossipDiscoveryInviteRequestItem* inviteReq = nullptr;

		++nhandled;

#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::handleIncoming() Received Message!" << std::endl;
		item -> print(std::cerr);
		std::cerr  << std::endl;
#endif

		if((contact = dynamic_cast<RsDiscContactItem *>(item)))
		{
			if (item->PeerId() == contact->sslId)
				recvOwnContactInfo(item->PeerId(), contact);
			else processContactInfo(item->PeerId(), contact);
		}
		else if((gxsidlst = dynamic_cast<RsDiscIdentityListItem *>(item)))
		{
			recvIdentityList(item->PeerId(),gxsidlst->ownIdentityList);
			delete item;
		}
		else if((pgpcert = dynamic_cast<RsDiscPgpCertItem *>(item)))
			recvPGPCertificate(item->PeerId(), pgpcert);
		else if((pgplist = dynamic_cast<RsDiscPgpListItem *>(item)))
		{
			if (pgplist->mode == DISC_PGP_LIST_MODE_FRIENDS)
				processPGPList(pgplist->PeerId(), pgplist);
			else if (pgplist->mode == DISC_PGP_LIST_MODE_GETCERT)
				recvPGPCertificateRequest(pgplist->PeerId(), pgplist);
			else delete item;
		}
		else if((invite = dynamic_cast<RsGossipDiscoveryInviteItem*>(item)))
			recvInvite(std::unique_ptr<RsGossipDiscoveryInviteItem>(invite));
		else if((inviteReq = dynamic_cast<RsGossipDiscoveryInviteRequestItem*>(item)))
		{
			sendInvite(inviteReq->mInviteId, item->PeerId());
			delete item;
		}
		else
		{
			RsErr() << __PRETTY_FUNCTION__ << " Received unknown item type! "
			        << std::endl << item << std::endl;
			delete item;
		}
	}

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
		/* Cyril: we dont send our own IP to an hidden node. It will not use it
		 *   anyway. */
		populateContactInfo(detail, pkt, !rsPeers->isHiddenNode(sslid));
		/* G10h4ck: sending IP information also to hidden nodes has proven very
		 *   helpful in the usecase of non hidden nodes, that share a common
		 *   hidden trusted node, to discover each other IP.
		 *   Advanced/corner case non hidden node users that want to hide their
		 *   IP to a specific hidden ~trusted~ node can do it through the
		 *   permission matrix. Disabling this instead will make life more
		 *   difficult for average user, that moreover whould have no way to
		 *   revert an hardcoded policy. */
		//populateContactInfo(detail, pkt, true);

		pkt->version = RS_HUMAN_READABLE_VERSION;
		pkt->PeerId(sslid);

#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::sendOwnContactInfo() sending:" << std::endl;
		pkt -> print(std::cerr);
		std::cerr  << std::endl;
#endif
		sendItem(pkt);

        RsDiscIdentityListItem *pkt2 = new RsDiscIdentityListItem();

        rsIdentity->getOwnIds(pkt2->ownIdentityList,true);
        pkt2->PeerId(sslid) ;

#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::sendOwnContactInfo() sending own signed identity list:" << std::endl;
        for(auto it(pkt2->ownIdentityList.begin());it!=pkt2->ownIdentityList.end();++it)
            std::cerr << "  identity: " << (*it).toStdString() << std::endl;
#endif
		sendItem(pkt2);
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
	RS_STACK_MUTEX(mDiscMtx);

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

void p3discovery2::recvIdentityList(const RsPeerId& pid,const std::list<RsGxsId>& ids)
{
    std::list<RsPeerId> peers;
    peers.push_back(pid);

#ifdef P3DISC_DEBUG
    std::cerr << "p3discovery2::recvIdentityList(): from peer " << pid << ": " << ids.size() << " identities" << std::endl;
#endif

    RsIdentityUsage use_info(RS_SERVICE_TYPE_DISC,RsIdentityUsage::IDENTITY_DATA_UPDATE);

	for(auto it(ids.begin());it!=ids.end();++it)
    {
#ifdef P3DISC_DEBUG
        std::cerr << "  identity: " << (*it).toStdString() << std::endl;
#endif
		mGixs->requestKey(*it,peers,use_info) ;
    }
}

void p3discovery2::updatePeerAddresses(const RsDiscContactItem *item)
{
	if (item->isHidden)
		mPeerMgr->setHiddenDomainPort(item->sslId, item->hiddenAddr,
		                              item->hiddenPort);
	else
	{
		mPeerMgr->setDynDNS(item->sslId, item->dyndns);
		updatePeerAddressList(item);
	}
}

void p3discovery2::updatePeerAddressList(const RsDiscContactItem *item)
{
	if (item->isHidden)
	{
	}
	else if(!mPeerMgr->isHiddenNode(rsPeers->getOwnId()))
	{
		/* Cyril: we don't store IP addresses if we're a hidden node.
		 * Normally they should not be sent to us, except for old peers. */
		/* G10h4ck: sending IP information also to hidden nodes has proven very
		 *   helpful in the usecase of non hidden nodes, that share a common
		 *   hidden trusted node, to discover each other IP.
		 *   Advanced/corner case non hidden node users that want to hide their
		 *   IP to a specific hidden ~trusted~ node can do it through the
		 *   permission matrix. Disabling this instead will make life more
		 *   difficult for average user, that moreover whould have no way to
		 *   revert an hardcoded policy. */
		pqiIpAddrSet addrsFromPeer;
		addrsFromPeer.mLocal.extractFromTlv(item->localAddrList);
		addrsFromPeer.mExt.extractFromTlv(item->extAddrList);

#ifdef P3DISC_DEBUG
		std::cerr << "Setting address list to peer " << item->sslId
		          << ", to be:" << std::endl ;

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

	RS_STACK_MUTEX(mDiscMtx);


#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::sendPGPList() to " << toId << std::endl;
#endif

	RsDiscPgpListItem *pkt = new RsDiscPgpListItem();

	pkt->mode = DISC_PGP_LIST_MODE_FRIENDS;

	std::map<PGPID, DiscPgpInfo>::const_iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
	{
        pkt->pgpIdSet.ids.insert(it->first);
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
	
	RS_STACK_MUTEX(mDiscMtx);

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
	for(lit = pgpList.begin(); lit != pgpList.end(); ++lit)
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
	for(; sit != pgpSet.end(); ++sit)
	{
		pgpToAdd.push_back(*sit);
	}
	
	for(; it != mFriendList.end(); ++it)
	{
		/* more to remove */
		pgpToRemove.push_back(it->first);		
	}
	
	for(lit = pgpToRemove.begin(); lit != pgpToRemove.end(); ++lit)
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::updatePgpFriendList() Removing pgpId: " << *lit;
		std::cerr << std::endl;
#endif
		
		it = mFriendList.find(*lit);
		mFriendList.erase(it);
	}

	for(lit = pgpToAdd.begin(); lit != pgpToAdd.end(); ++lit)
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::updatePgpFriendList() Adding pgpId: " << *lit;
		std::cerr << std::endl;
#endif

		mFriendList[*lit] = DiscPgpInfo();
	}	

	/* finally install the pgpList on our own entry */
	DiscPgpInfo &ownInfo = mFriendList[ownPgpId];
    ownInfo.mergeFriendList(pgpSet);

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
		std::set<PGPID>::const_iterator fit;
		for(fit = item->pgpIdSet.ids.begin(); fit != item->pgpIdSet.ids.end(); ++fit)
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
	for(fit = friendSet.begin(); fit != friendSet.end(); ++fit)
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
					mit != ffit->second.mSslIds.end(); ++mit)
			{
				SSLID sslid = mit->first;
				if (mServiceCtrl->isPeerConnected(getServiceInfo().mServiceType, sslid))
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
	for(fit = mutualFriends.begin(); fit != mutualFriends.end(); ++fit)
	{
		sendContactInfo_locked(*fit, aboutId);
	}

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::updatePeer_locked() Updating Online Peers about " << aboutId;
	std::cerr  << std::endl;
#endif
	// update Other Peers about aboutPgpId.
	for(sit = onlineFriends.begin(); sit != onlineFriends.end(); ++sit)
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
	for(sit = it->second.mSslIds.begin(); sit != it->second.mSslIds.end(); ++sit)
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::sendContactInfo_locked() related sslId: " << sit->first;
		std::cerr << std::endl;
#endif

        if (sit->first == rsPeers->getOwnId())
        {
            // sending info of toId to himself will be used by toId to check that the IP it is connected as is the same
            // as its external IP.
#ifdef P3DISC_DEBUG
            std::cerr << "p3discovery2::processContactInfo() not sending info on self";
			std::cerr << std::endl;
#endif		
			continue;
		}

		if (sit->second.mDiscStatus != RS_VS_DISC_OFF)
		{
			peerState detail;
            peerConnectState detail2;

            if (mPeerMgr->getFriendNetStatus(sit->first, detail))
			{
				RsDiscContactItem *pkt = new RsDiscContactItem();
				populateContactInfo(detail, pkt,!mPeerMgr->isHiddenNode(toId));// never send IPs to an hidden node. The node will not use them anyway.
				pkt->PeerId(toId);

                // send to each peer its own connection address.

                if(sit->first == toId && mLinkMgr->getFriendNetStatus(sit->first,detail2))
                    pkt->currentConnectAddress.addr = detail2.connectaddr;
                else
                    sockaddr_storage_clear(pkt->currentConnectAddress.addr) ;

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
	(void) fromId; // remove unused parameter warnings, debug only

	RS_STACK_MUTEX(mDiscMtx);

	if (item->sslId == rsPeers->getOwnId())
	{
		if(sockaddr_storage_isExternalNet(item->currentConnectAddress.addr))
			mPeerMgr->addCandidateForOwnExternalAddress(
			            item->PeerId(), item->currentConnectAddress.addr);

		delete item;
		return;
	}

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
		
		/* THESE ARE OUR FRIEND OF FRIENDS ... pass this information along to
		 * NetMgr & DHT...
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
        delete item;
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
		//sit = it->second.mSslIds.find(item->sslId);

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
            // We pass RS_NODE_PERM_ALL because the PGP id is already a friend, so we should keep the existing
            // permission flags. Therefore the mask needs to be 0xffff.

			// set last seen to RS_PEER_OFFLINE_NO_DISC minus 1 so that it won't be shared with other friends
			// until a first connection is established

			mPeerMgr->addFriend( item->sslId, item->pgpId, item->netMode,
			                     RS_VS_DISC_OFF, RS_VS_DHT_FULL,
			                     time(NULL) - RS_PEER_OFFLINE_NO_DISC - 1,
			                     RS_NODE_PERM_ALL );
			updatePeerAddresses(item);
		}
	}

	updatePeerAddressList(item);


	RsServer::notify()->notifyListChange(NOTIFY_LIST_NEIGHBOURS, NOTIFY_TYPE_MOD);

	if(should_notify_discovery)
		RsServer::notify()->notifyDiscInfoChanged();

    delete item;
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
    pkt->pgpIdSet.ids.insert(aboutId);
	pkt->PeerId(toId);
	
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::requestPGPCertificate() sending:" << std::endl;
	pkt->print(std::cerr);
	std::cerr  << std::endl;
#endif

	sendItem(pkt);
}

void p3discovery2::recvPGPCertificateRequest(
        const RsPeerId& fromId, const RsDiscPgpListItem* item )
{
#ifdef P3DISC_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " from " << fromId << std::endl;
#endif

	RsPgpId ownPgpId = AuthGPG::getAuthGPG()->getGPGOwnId();
	for(const RsPgpId& pgpId : item->pgpIdSet.ids)
		if (pgpId == ownPgpId || AuthGPG::getAuthGPG()->isGPGAccepted(pgpId))
			sendPGPCertificate(pgpId, fromId);
	delete item;
}


void p3discovery2::sendPGPCertificate(const PGPID &aboutId, const SSLID &toId)
{
	RsDiscPgpCertItem* item = new RsDiscPgpCertItem();
	item->pgpId = aboutId;
	item->PeerId(toId);

#ifdef P3DISC_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " queuing for Cert generation:"
	          << std::endl;
	item->print(std::cerr);
	std::cerr  << std::endl;
#endif

	{
		RS_STACK_MUTEX(mDiscMtx);
		mPendingDiscPgpCertOutList.push_back(item);
	}
}

void p3discovery2::recvPGPCertificate(
        const SSLID& /*fromId*/, RsDiscPgpCertItem* item )
{
#ifdef P3DISC_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " queuing for Cert loading" << std::endl;
#endif
	/* push this back to be processed by pgp when possible */
	RS_STACK_MUTEX(mDiscMtx);
	mPendingDiscPgpCertInList.push_back(item);
}

        /************* from pqiServiceMonitor *******************/
void p3discovery2::statusChange(const std::list<pqiServicePeer> &plist)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::statusChange()" << std::endl;
#endif

	std::list<pqiServicePeer>::const_iterator pit;
	for(pit =  plist.begin(); pit != plist.end(); ++pit)
	{
		if (pit->actions & RS_SERVICE_PEER_CONNECTED) 
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::statusChange() Starting Disc with: " << pit->id << std::endl;
#endif
			sendOwnContactInfo(pit->id);
		} 
		else if (pit->actions & RS_SERVICE_PEER_DISCONNECTED) 
		{
			std::cerr << "p3discovery2::statusChange() Disconnected: " << pit->id << std::endl;
		}

		if (pit->actions & RS_SERVICE_PEER_NEW)
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::statusChange() Adding Friend: " << pit->id << std::endl;
#endif
			addFriend(pit->id);
		}
		else if (pit->actions & RS_SERVICE_PEER_REMOVED)
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::statusChange() Removing Friend: " << pit->id << std::endl;
#endif
			removeFriend(pit->id);
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
bool p3discovery2::getDiscFriends(const RsPeerId& id, std::list<RsPeerId> &proxyIds)
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
	for(fit = friendSet.begin(); fit != friendSet.end(); ++fit)
	{
		it = mFriendList.find(*fit);
		if (it == mFriendList.end())
		{
			continue;
		}

		std::map<SSLID, DiscSslInfo>::const_iterator sit;
		for(sit = it->second.mSslIds.begin();
				sit != it->second.mSslIds.end(); ++sit)
		{
			proxyIds.push_back(sit->first);
		}
	}
	return true;

}

bool p3discovery2::getWaitingDiscCount(size_t &sendCount, size_t &recvCount)
{
	RS_STACK_MUTEX(mDiscMtx);
	sendCount = mPendingDiscPgpCertOutList.size();
	recvCount = mPendingDiscPgpCertInList.size();

	return true;
}

bool p3discovery2::sendInvite(
        const RsPeerId& inviteId, const RsPeerId& toSslId,
        std::string& errorMsg )
{
	RsPeers& mPeers = *rsPeers;

	std::string&& invite = mPeers.GetRetroshareInvite(inviteId);

	if(invite.empty())
	{
		errorMsg = "Failure generating invite for peer: " +
		        inviteId.toStdString() + " are you sure is a friend?";
		RsErr() << __PRETTY_FUNCTION__ << " " << errorMsg << std::endl;
		return false;
	}

	RsGossipDiscoveryInviteItem* item = new RsGossipDiscoveryInviteItem;
	item->PeerId(toSslId);
	item->mInvite = mPeers.GetRetroshareInvite(inviteId, true, true);

	return sendItem(item);
}

bool p3discovery2::requestInvite(
        const RsPeerId& inviteId, const RsPeerId& toSslId,
        std::string& /*errorMsg*/ )
{
	Dbg2() << __PRETTY_FUNCTION__ << " inviteId: " << inviteId
	       << " toSslId: " << toSslId << std::endl;

	RsGossipDiscoveryInviteRequestItem* item =
	        new RsGossipDiscoveryInviteRequestItem;
	item->PeerId(toSslId);
	item->mInviteId = inviteId;

	return sendItem(item);
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
	for(fit = it->second.mFriendSet.begin(); fit != it->second.mFriendSet.end(); ++fit)
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

void p3discovery2::recvInvite(
        std::unique_ptr<RsGossipDiscoveryInviteItem> inviteItem )
{
	RsPeerDetails dt; uint32_t eCode;
	if( rsPeers
	        && rsPeers->loadDetailsFromStringCert(
	            inviteItem->mInvite, dt, eCode )
	        && rsPeers->isFriendPendingPgp(dt.id) )
	{ // notify we got a pendig PGP
		typedef RsGossipDiscoveryPendingPgpFriendInviteReceivedEvent Evt_t;

		// Ensure rsEvents is not deleted while we use it
		std::shared_ptr<RsEvents> lockedRsEvents = rsEvents;
		if(lockedRsEvents)
			lockedRsEvents->postEvent(
			            std::unique_ptr<Evt_t>(new Evt_t(inviteItem->mInvite)) );
	}
}

void p3discovery2::rsEventsHandler(const RsEvent& event)
{
	switch(event.mType)
	{
	case RsEventType::AUTHSSL_CONNECTION_AUTENTICATION:
	{
		using Evt_t = RsAuthSslConnectionAutenticationEvent;
		const Evt_t& evt = static_cast<const Evt_t&>(event);
		if(evt.mIsPendingPpg)
		{
			Dbg1() << __PRETTY_FUNCTION__
			       << " AUTHSSL_CONNECTION_AUTENTICATION event: " << event
			       << std::endl;

			const RsPeerId& sslId = evt.mSslId;
			RsThread::async([sslId]()
			{ /* Make sure the connection is ready before requesting invite. */
				auto timeout = std::chrono::system_clock::now() +
				        std::chrono::seconds(15);
				while ( rsDisc && rsPeers &&
				        std::chrono::system_clock::now() < timeout )
				{
					if(rsPeers->isOnline(sslId))
					{
						// Need to wait a bit more even if online
						//std::this_thread::sleep_for(std::chrono::seconds(2));
						if(rsDisc && rsDisc->requestInvite(sslId, sslId))
							return; // RsThread::async
					}

					Dbg3() << __PRETTY_FUNCTION__ << " waiting for: "
					       << sslId << " to complete connection" << std::endl;

					std::this_thread::sleep_for(std::chrono::milliseconds(200));
				}

				RsErr() << __PRETTY_FUNCTION__ << " timeout requesting pending "
				        << " PGP peer: " << sslId << " invite." << std::endl;
			});
		}
		break;
	}
	default: break;
	}
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

RsGossipDiscoveryPendingPgpFriendInviteReceivedEvent::
RsGossipDiscoveryPendingPgpFriendInviteReceivedEvent(const std::string& invite) :
    RsEvent(RsEventType::GOSSIP_DISCOVERY_PENDIG_PGP_CERT_RECEIVED),
    mInvite(invite) {}
