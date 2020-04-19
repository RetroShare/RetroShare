/*******************************************************************************
 * RetroShare gossip discovery service implementation                          *
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

#include "gossipdiscovery/p3gossipdiscovery.h"
#include "pqi/p3peermgr.h"
#include "retroshare/rsversion.h"
#include "retroshare/rsiface.h"
#include "rsserver/p3face.h"
#include "util/rsdebug.h"
#include "retroshare/rspeers.h"

/****
 * #define P3DISC_DEBUG	1
 ****/

/*extern*/ std::shared_ptr<RsGossipDiscovery> rsGossipDiscovery(nullptr);

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

void DiscPgpInfo::mergeFriendList(const std::set<RsPgpId> &friends)
{
    std::set<RsPgpId>::const_iterator it;
	for(it = friends.begin(); it != friends.end(); ++it)
	{
		mFriendSet.insert(*it);
	}
}


p3discovery2::p3discovery2(
        p3PeerMgr* peerMgr, p3LinkMgr* linkMgr, p3NetMgr* netMgr,
        p3ServiceControl* sc, RsGixs* gixs ) :
    p3Service(), mRsEventsHandle(0), mPeerMgr(peerMgr), mLinkMgr(linkMgr),
    mNetMgr(netMgr), mServiceCtrl(sc), mGixs(gixs), mDiscMtx("p3discovery2"),
    mLastPgpUpdate(0)
{
	Dbg3() << __PRETTY_FUNCTION__ << std::endl;

	RS_STACK_MUTEX(mDiscMtx);
	addSerialType(new RsDiscSerialiser());

	// Add self into PGP FriendList.
	mFriendList[AuthGPG::getAuthGPG()->getGPGOwnId()] = DiscPgpInfo();

	if(rsEvents)
		rsEvents->registerEventsHandler(
                    RsEventType::GOSSIP_DISCOVERY,
		            [this](std::shared_ptr<const RsEvent> event)
		{
			rsEventsHandler(*event);
		}, mRsEventsHandle ); // mRsEventsHandle is zeroed in initializer list
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

void p3discovery2::addFriend(const RsPeerId &sslId)
{
	RsPgpId pgpId = getPGPId(sslId);

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<RsPgpId, DiscPgpInfo>::iterator it;
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


	/* now add RsPeerId */

	std::map<RsPeerId, DiscSslInfo>::iterator sit;
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

void p3discovery2::removeFriend(const RsPeerId &sslId)
{
	RsPgpId pgpId = getPGPId(sslId);

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<RsPgpId, DiscPgpInfo>::iterator it;
	it = mFriendList.find(pgpId);
	if (it == mFriendList.end())
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::removeFriend() missing pgp entry: " << pgpId;
		std::cerr << std::endl;
#endif
		return;
	}

	std::map<RsPeerId, DiscSslInfo>::iterator sit;
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

RsPgpId p3discovery2::getPGPId(const RsPeerId &id)
{
	RsPgpId pgpId;
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
		RsDiscPgpListItem*      pgplist  = nullptr;
		RsDiscPgpKeyItem*       pgpkey   = nullptr;
		RsDiscPgpCertItem*      pgpcert	 = nullptr;	 // deprecated, hanlde for retro compability
		RsDiscContactItem*      contact  = nullptr;
		RsDiscIdentityListItem* gxsidlst = nullptr;

		++nhandled;

		Dbg4() << __PRETTY_FUNCTION__ << " Received item: " << std::endl
		       << *item << std::endl;

		if((contact = dynamic_cast<RsDiscContactItem *>(item)) != nullptr)
		{
			if (item->PeerId() == contact->sslId)
				recvOwnContactInfo(item->PeerId(), contact);
			else
                processContactInfo(item->PeerId(), contact);
		}
		else if( (gxsidlst = dynamic_cast<RsDiscIdentityListItem *>(item)) != nullptr )
		{
			recvIdentityList(item->PeerId(),gxsidlst->ownIdentityList);
			delete item;
		}
		else if((pgpkey = dynamic_cast<RsDiscPgpKeyItem *>(item)) != nullptr)
			recvPGPCertificate(item->PeerId(), pgpkey);
		else if((pgpcert = dynamic_cast<RsDiscPgpCertItem *>(item)) != nullptr)
			// sink
			delete pgpcert;
		else if((pgplist = dynamic_cast<RsDiscPgpListItem *>(item)) != nullptr)
		{
			if (pgplist->mode == RsGossipDiscoveryPgpListMode::FRIENDS)
				processPGPList(pgplist->PeerId(), pgplist);
			else if (pgplist->mode == RsGossipDiscoveryPgpListMode::GETCERT)
				recvPGPCertificateRequest(pgplist->PeerId(), pgplist);
			else delete item;
		}
		else
		{
			RsWarn() << __PRETTY_FUNCTION__ << " Received unknown item type " << (int)item->PacketSubType() << "! " << std::endl ;
            RsWarn() << item << std::endl;
			delete item;
		}
	}

	return nhandled;
}

void p3discovery2::sendOwnContactInfo(const RsPeerId &sslid)
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
		 * anyway. Furthermore, a Tor node is not supposed to have any mean to send the IPs of his friend nodes
 		 * to other nodes. This would be a very serious security risk.    */

		populateContactInfo(detail, pkt, !rsPeers->isHiddenNode(sslid));

		/* G10h4ck: sending IP information also to hidden nodes has proven very
		 *   helpful in the usecase of non hidden nodes, that share a common
		 *   hidden trusted node, to discover each other IP.
		 *   Advanced/corner case non hidden node users that want to hide their
		 *   IP to a specific hidden ~trusted~ node can do it through the
		 *   permission matrix. Disabling this instead will make life more
		 *   difficult for average user, that moreover whould have no way to
		 *   revert an hardcoded policy. */

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

void p3discovery2::recvOwnContactInfo(const RsPeerId &fromId, const RsDiscContactItem *item)
{
    std::unique_ptr<const RsDiscContactItem> pitem(item); // ensures that item will be destroyed whichever door we leave through

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
	// Check that the "own" ID sent corresponds to the one we think it should be.
	// Some of these checks may look superfluous but it's better to risk to check twice than not check at all.

    // was obtained using a short invite. , and that the friend is marked as "ignore PGP validation" because it
	RsPeerDetails det ;
	if(!rsPeers->getPeerDetails(fromId,det))
	{
		RsErr() << "(EE) Cannot obtain details from " << fromId << " who is supposed to be a friend! Dropping the info." << std::endl;
		return;
	}
	if(det.gpg_id != item->pgpId)
	{
		RsErr() << "(EE) peer " << fromId << " sent own details with PGP key ID " << item->pgpId << " which does not match the known key id " << det.gpg_id << ". Dropping the info." << std::endl;
		return;
	}

	// Peer Own Info replaces the existing info, because the
	// peer is the primary source of his own IPs.

	mPeerMgr->setNetworkMode(fromId, item->netMode);
	mPeerMgr->setLocation(fromId, item->location);
	mPeerMgr->setVisState(fromId, item->vs_disc, item->vs_dht);

	setPeerVersion(fromId, item->version);

	updatePeerAddresses(item);

    // if the peer is not validated, we stop the exchange here

    if(det.skip_pgp_signature_validation)
    {
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::recvOwnContactInfo() missing PGP key  " << item->pgpId << " from short invite friend " << fromId << ". Requesting it." << std::endl;
#endif
		requestPGPCertificate(det.gpg_id, fromId);
        return;
	}

	// This information will be sent out to online peers, at the receipt of their PGPList.
	// It is important that PGPList is received after the OwnContactItem.
	// This should happen, but is not enforced by the protocol.

	// Start peer list exchange, if discovery is enabled

    peerState ps;
    mPeerMgr->getOwnNetStatus(ps);

    if(ps.vs_disc != RS_VS_DISC_OFF)
		sendPGPList(fromId);

	// Update mDiscStatus.
	RS_STACK_MUTEX(mDiscMtx);

	RsPgpId pgpId = getPGPId(fromId);
	std::map<RsPgpId, DiscPgpInfo>::iterator it = mFriendList.find(pgpId);
	if (it != mFriendList.end())
	{
		std::map<RsPeerId, DiscSslInfo>::iterator sit = it->second.mSslIds.find(fromId);
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
void p3discovery2::sendPGPList(const RsPeerId &toId)
{
	updatePgpFriendList();

	RS_STACK_MUTEX(mDiscMtx);


#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::sendPGPList() to " << toId << std::endl;
#endif

	RsDiscPgpListItem *pkt = new RsDiscPgpListItem();

	pkt->mode = RsGossipDiscoveryPgpListMode::FRIENDS;

	for(auto it = mFriendList.begin(); it != mFriendList.end(); ++it)
	{
        // Check every friend, and only send his PGP key if the friend tells that he wants discovery. Because this action over profiles depends on a node information,
        // we check each node of a given progile and only send the profile key if at least one node allows it.

        for(auto it2(it->second.mSslIds.begin());it2!=it->second.mSslIds.end();++it2)
            if(it2->second.mDiscStatus != RS_VS_DISC_OFF)
            {
				pkt->pgpIdSet.ids.insert(it->first);
                break;
            }
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
	
    std::list<RsPgpId> pgpList;
	std::set<RsPgpId> pgpSet;

	std::set<RsPgpId>::iterator sit;
	std::list<RsPgpId>::iterator lit;
	std::map<RsPgpId, DiscPgpInfo>::iterator it;
	
	RsPgpId ownPgpId = AuthGPG::getAuthGPG()->getGPGOwnId();
    AuthGPG::getAuthGPG()->getGPGAcceptedList(pgpList);
	pgpList.push_back(ownPgpId);
	
	// convert to set for ordering.
	for(lit = pgpList.begin(); lit != pgpList.end(); ++lit)
	{
		pgpSet.insert(*lit);
	}
	
	std::list<RsPgpId> pgpToAdd;
	std::list<RsPgpId> pgpToRemove;
	

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

void p3discovery2::processPGPList(const RsPeerId &fromId, const RsDiscPgpListItem *item)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::processPGPList() from " << fromId;
	std::cerr << std::endl;
#endif

	RsPgpId fromPgpId = getPGPId(fromId);
	auto it = mFriendList.find(fromPgpId);
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
		requestUnknownPgpCerts = false;
	
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
		std::set<RsPgpId>::const_iterator fit;
		for(fit = item->pgpIdSet.ids.begin(); fit != item->pgpIdSet.ids.end(); ++fit)
		{
			if (!AuthGPG::getAuthGPG()->isGPGId(*fit))
			{
#ifdef P3DISC_DEBUG
				std::cerr << "p3discovery2::processPGPList() requesting certificate for PgpId: " << *fit;
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
void p3discovery2::updatePeers_locked(const RsPeerId &aboutId)
{

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::updatePeers_locked() about " << aboutId;
	std::cerr << std::endl;
#endif

	RsPgpId aboutPgpId = getPGPId(aboutId);

	auto ait = mFriendList.find(aboutPgpId);
	if (ait == mFriendList.end())
	{

#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::updatePeers_locked() PgpId is not a friend: " << aboutPgpId;
		std::cerr << std::endl;
#endif
		return;
	}

	std::set<RsPgpId> mutualFriends;
	std::set<RsPeerId> onlineFriends;
	
	const std::set<RsPgpId> &friendSet = ait->second.mFriendSet;

	for(auto fit = friendSet.begin(); fit != friendSet.end(); ++fit)
	{

#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::updatePeer_locked() checking their friend: " << *fit;
		std::cerr  << std::endl;
#endif

		auto ffit = mFriendList.find(*fit);

		if (ffit == mFriendList.end())
		{

#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::updatePeer_locked() Ignoring not our friend";
			std::cerr  << std::endl;
#endif
			// Not our friend, or we have no Locations (SSL) for this RsPgpId (same difference)
			continue;
		}

		if (ffit->second.mFriendSet.find(aboutPgpId) != ffit->second.mFriendSet.end())
		{

#ifdef P3DISC_DEBUG
			std::cerr << "p3discovery2::updatePeer_locked() Adding as Mutual Friend";
			std::cerr  << std::endl;
#endif
			mutualFriends.insert(*fit);

			for(auto mit = ffit->second.mSslIds.begin(); mit != ffit->second.mSslIds.end(); ++mit)
			{
				RsPeerId sslid = mit->first;
				if (mServiceCtrl->isPeerConnected(getServiceInfo().mServiceType, sslid))
				{
					// TODO IGNORE if sslid == aboutId, or sslid == ownId.
#ifdef P3DISC_DEBUG
					std::cerr << "p3discovery2::updatePeer_locked() Adding Online RsPeerId: " << sslid;
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
	for(auto fit = mutualFriends.begin(); fit != mutualFriends.end(); ++fit)
		sendContactInfo_locked(*fit, aboutId);

#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::updatePeer_locked() Updating Online Peers about " << aboutId;
	std::cerr  << std::endl;
#endif
	// update Other Peers about aboutPgpId.
	for(auto sit = onlineFriends.begin(); sit != onlineFriends.end(); ++sit)
	{
		// This could be more efficient, and only be specific about aboutId.
		// but we'll leave it like this for the moment.
		sendContactInfo_locked(aboutPgpId, *sit);
	}
}

void p3discovery2::sendContactInfo_locked(const RsPgpId &aboutId, const RsPeerId &toId)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::sendContactInfo_locked() aboutPGPId: " << aboutId << " toId: " << toId;
	std::cerr << std::endl;
#endif
	std::map<RsPgpId, DiscPgpInfo>::const_iterator it;
	it = mFriendList.find(aboutId);
	if (it == mFriendList.end())
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::sendContactInfo_locked() ERROR aboutId is not a friend";
		std::cerr << std::endl;
#endif
		return;
	}

	std::map<RsPeerId, DiscSslInfo>::const_iterator sit;
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
			std::cerr << "p3discovery2::sendContactInfo_locked() RsPeerId Hidden";
			std::cerr << std::endl;
#endif
		}
	}
}

void p3discovery2::processContactInfo(const RsPeerId &fromId, const RsDiscContactItem *item)
{
	(void) fromId; // remove unused parameter warnings, debug only

	RS_STACK_MUTEX(mDiscMtx);

    // This case is the node fromId sending information about ourselves to us. There's one good use of this:
    // read the IP information the friend knows about us, and use it to extimate our external address.

	if (item->sslId == rsPeers->getOwnId())
	{
		if(sockaddr_storage_isExternalNet(item->currentConnectAddress.addr))
			mPeerMgr->addCandidateForOwnExternalAddress(item->PeerId(), item->currentConnectAddress.addr);

		delete item;
		return;
	}

	auto it = mFriendList.find(item->pgpId);	// is this the PGP id one of our friends?

	if (it == mFriendList.end())	// no it's not.
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3discovery2::processContactInfo(" << fromId << ") RsPgpId: ";
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
			mNetMgr->netAssistKnownPeer(item->sslId, item->extAddrV4.addr, NETASSIST_KNOWN_PEER_FOF | NETASSIST_KNOWN_PEER_OFFLINE);
		}
        delete item;
		return;
	}

    // The peer the discovery info is about is a friend. We gather the nodes for that profile into the local structure and notify p3peerMgr.

    if(!rsPeers->isGPGAccepted(item->pgpId))  // this is an additional check, because the friendship previously depends on the local cache. We need
		return ;                              // fresh information here.

	bool should_notify_discovery = false;

	DiscSslInfo& sslInfo(it->second.mSslIds[item->sslId]);	// This line inserts the entry while not removing already existing data
    														// do not remove it!
	if (!mPeerMgr->isFriend(item->sslId))
	{
		should_notify_discovery = true;

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

		// This code is a bit dangerous: we add a friend without the insurance that the PGP key that will validate this friend actually has
		// the supplied PGP id. Of course, because it comes from a friend,  we should trust that friend. Anyway, it is important that
		// when connecting the handshake is always doen w.r.t. the known PGP key, and not the one that is indicated in the certificate issuer field.

		mPeerMgr->addFriend( item->sslId, item->pgpId, item->netMode,
		                     RS_VS_DISC_OFF, RS_VS_DHT_FULL,
		                     time(NULL) - RS_PEER_OFFLINE_NO_DISC - 1,
		                     RS_NODE_PERM_ALL );

		updatePeerAddresses(item);
	}
	updatePeerAddressList(item);

	RsServer::notify()->notifyListChange(NOTIFY_LIST_NEIGHBOURS, NOTIFY_TYPE_MOD);

	if(should_notify_discovery)
		RsServer::notify()->notifyDiscInfoChanged();

    delete item;
}

/* we explictly request certificates, instead of getting them all the time
 */
void p3discovery2::requestPGPCertificate(const RsPgpId &aboutId, const RsPeerId &toId)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::requestPGPCertificate() aboutId: " << aboutId << " to: " << toId;
	std::cerr << std::endl;
#endif
	
	RsDiscPgpListItem *pkt = new RsDiscPgpListItem();
	
	pkt->mode = RsGossipDiscoveryPgpListMode::GETCERT;
    pkt->pgpIdSet.ids.insert(aboutId);
	pkt->PeerId(toId);
	
#ifdef P3DISC_DEBUG
	std::cerr << "p3discovery2::requestPGPCertificate() sending:" << std::endl;
	pkt->print(std::cerr);
	std::cerr  << std::endl;
#endif

	sendItem(pkt);
}

void p3discovery2::recvPGPCertificateRequest( const RsPeerId& fromId, const RsDiscPgpListItem* item )
{
#ifdef P3DISC_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " from " << fromId << std::endl;
#endif
    peerState ps;
    mPeerMgr->getOwnNetStatus(ps);

    if(ps.vs_disc == RS_VS_DISC_OFF)
    {
        std::cerr << "(WW) refusing PGP certificate request from " << fromId << " because discovery is OFF" << std::endl;
		return;
    }

	RsPgpId ownPgpId = AuthGPG::getAuthGPG()->getGPGOwnId();
	for(const RsPgpId& pgpId : item->pgpIdSet.ids)
		if (pgpId == ownPgpId)
			sendPGPCertificate(pgpId, fromId);
		else if(ps.vs_disc != RS_VS_DISC_OFF && AuthGPG::getAuthGPG()->isGPGAccepted(pgpId))
			sendPGPCertificate(pgpId, fromId);
		else
            std::cerr << "(WW) not sending certificate " << pgpId << " asked by friend " << fromId << " because this either this cert is not a friend, or discovery is off" << std::endl;

	delete item;
}


void p3discovery2::sendPGPCertificate(const RsPgpId &aboutId, const RsPeerId &toId)
{
    RsDiscPgpKeyItem *pgp_key_item = new RsDiscPgpKeyItem;

	pgp_key_item->PeerId(toId);
    pgp_key_item->pgpKeyId = aboutId;
    unsigned char *bin_data;
    size_t bin_len;

	if(!AuthGPG::getAuthGPG()->exportPublicKey(aboutId,bin_data,bin_len,false,true))
    {
        std::cerr << "(EE) cannot export public key " << aboutId << " requested by peer " << toId << std::endl;
		return ;
    }

    pgp_key_item->bin_data = bin_data;
	pgp_key_item->bin_len = bin_len;

    sendItem(pgp_key_item);
}

void p3discovery2::recvPGPCertificate(const RsPeerId& fromId, RsDiscPgpKeyItem* item )
{
    // 1 - check that the cert structure is valid.

	RsPgpId cert_pgp_id;
	std::string cert_name;
	std::list<RsPgpId> cert_signers;

	if(!AuthGPG::getAuthGPG()->getGPGDetailsFromBinaryBlock( (unsigned char*)item->bin_data,item->bin_len, cert_pgp_id, cert_name, cert_signers ))
	{
		std::cerr << "(EE) cannot parse own PGP key sent by " << fromId << std::endl;
		return;
	}

	if(cert_pgp_id != item->pgpKeyId)
	{
		std::cerr << "(EE) received a PGP key from " << fromId << " which ID (" << cert_pgp_id << ") is different from the one anounced in the packet (" << item->pgpKeyId << ")!" << std::endl;
		return;
	}

    // 2 - check if the peer who is sending us a cert is already validated

    RsPeerDetails det;
    if(!rsPeers->getPeerDetails(fromId,det))
    {
        std::cerr << "(EE) cannot get peer details from friend " << fromId << ": this is very wrong!"<< std::endl;
        return;
    }

    // We treat own pgp keys right away when they are sent by a friend for which we dont have it. This way we can keep the skip_pgg_signature_validation consistent

    if(det.skip_pgp_signature_validation)
    {
#ifdef P3DISC_DEBUG
		std::cerr << __PRETTY_FUNCTION__ << " Received own full certificate from short-invite friend " << fromId << std::endl;
#endif
        // do some extra checks. Dont remove them. They cost nothing as compared to what they could avoid.

		if(item->pgpKeyId != det.gpg_id)
        {
			std::cerr << "(EE) received a PGP key with ID " << item->pgpKeyId << " from non validated peer " << fromId << ", which should only be allowed to send his own key " << det.gpg_id << std::endl;
			return;
		}
	}
	RsPgpId tmp_pgp_id;
	std::string error_string;

#ifdef P3DISC_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << "Received PGP key " << cert_pgp_id << " from from friend " << fromId << ". Adding to keyring." << std::endl;
#endif
	// now that will add the key *and* set the skip_signature_validation flag at once
	rsPeers->loadPgpKeyFromBinaryData((unsigned char*)item->bin_data,item->bin_len, tmp_pgp_id,error_string);	// no error should occur at this point because we called loadDetailsFromStringCert() already
	delete item;

    // Make sure we allow connections after the key is added. This is not the case otherwise. We only do that if the peer is non validated peer, since
    // otherwise the connection should already be accepted. This only happens when the short invite peer sends its own PGP key.

    if(det.skip_pgp_signature_validation)
		AuthGPG::getAuthGPG()->AllowConnection(det.gpg_id,true);
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
		
	std::map<RsPgpId, DiscPgpInfo>::const_iterator it;
	RsPgpId pgp_id = getPGPId(id);
		
	it = mFriendList.find(pgp_id);
	if (it == mFriendList.end())
	{
		// ERROR.
		return false;
	}

	// For each of their friends that we know, grab that set of RsPeerId.
	const std::set<RsPgpId> &friendSet = it->second.mFriendSet;
	std::set<RsPgpId>::const_iterator fit;
	for(fit = friendSet.begin(); fit != friendSet.end(); ++fit)
	{
		it = mFriendList.find(*fit);
		if (it == mFriendList.end())
		{
			continue;
		}

		std::map<RsPeerId, DiscSslInfo>::const_iterator sit;
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
	sendCount = 0;//mPendingDiscPgpCertOutList.size();
	recvCount = 0;//mPendingDiscPgpCertInList.size();

	return true;
}

bool p3discovery2::getDiscPgpFriends(const RsPgpId &pgp_id, std::list<RsPgpId> &proxyPgpIds)
{
	/* find id -> and extract the neighbour_of ids */
		
	if(pgp_id == rsPeers->getGPGOwnId()) // SSL id			// This is treated appart, because otherwise we don't receive any disc info about us
		return rsPeers->getGPGAcceptedList(proxyPgpIds) ;
		
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
		
	std::map<RsPgpId, DiscPgpInfo>::const_iterator it;
	it = mFriendList.find(pgp_id);
	if (it == mFriendList.end())
	{
		// ERROR.
		return false;
	}
	
	std::set<RsPgpId>::const_iterator fit;
	for(fit = it->second.mFriendSet.begin(); fit != it->second.mFriendSet.end(); ++fit)
	{
		proxyPgpIds.push_back(*fit);
	}
	return true;
}

bool p3discovery2::getPeerVersion(const RsPeerId &peerId, std::string &version)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<RsPeerId, DiscPeerInfo>::const_iterator it;
	it = mLocationMap.find(peerId);
	if (it == mLocationMap.end())
	{
		// MISSING.
		return false;
	}
	
	version = it->second.mVersion;
	return true;
}

bool p3discovery2::setPeerVersion(const RsPeerId &peerId, const std::string &version)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<RsPeerId, DiscPeerInfo>::iterator it;
	it = mLocationMap.find(peerId);
	if (it == mLocationMap.end())
	{
		mLocationMap[peerId] = DiscPeerInfo();
		it = mLocationMap.find(peerId);
	}

	it->second.mVersion = version;
	return true;
}

void p3discovery2::rsEventsHandler(const RsEvent& event)
{
	Dbg3() << __PRETTY_FUNCTION__ << " " << static_cast<uint32_t>(event.mType) << std::endl;
}


/*************************************************************************************/
/*			AuthGPGService						     */
/*************************************************************************************/
// AuthGPGOperation *p3discovery2::getGPGOperation()
// {
// 	{
// 		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
//
// 		/* process disc reply in list */
// 		if (!mPendingDiscPgpCertInList.empty()) {
// 			RsDiscPgpCertItem *item = mPendingDiscPgpCertInList.front();
// 			mPendingDiscPgpCertInList.pop_front();
//
// 			return new AuthGPGOperationLoadOrSave(true, item->pgpId, item->pgpCert, item);
// 		}
// 	}
//
// 	return NULL;
// }

// void p3discovery2::setGPGOperation(AuthGPGOperation *operation)
// {
// 	AuthGPGOperationLoadOrSave *loadOrSave = dynamic_cast<AuthGPGOperationLoadOrSave*>(operation);
// 	if (loadOrSave)
// 	{
// 		RsDiscPgpCertItem *item = (RsDiscPgpCertItem *) loadOrSave->m_userdata;
// 		if (!item)
// 		{
// 			return;
// 		}
//
// 		if (loadOrSave->m_load)
// 		{
//
// #ifdef P3DISC_DEBUG
// 			std::cerr << "p3discovery2::setGPGOperation() Loaded Cert" << std::endl;
// 			item->print(std::cerr, 5);
// 			std::cerr  << std::endl;
// #endif
// 			// It has already been processed by PGP.
// 			delete item;
// 		}
// 		else
// 		{
// 			// Attaching Certificate.
// 			item->pgpCert = loadOrSave->m_certGpg;
//
// #ifdef P3DISC_DEBUG
// 			std::cerr << "p3discovery2::setGPGOperation() Sending Message:" << std::endl;
// 			item->print(std::cerr, 5);
// #endif
//
// 			// Send off message
// 			sendItem(item);
// 		}
// 		return;
// 	}
//
// 	/* ignore other operations */
// }
