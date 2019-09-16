/*******************************************************************************
 * RetroShare gossip discovery service implementation                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2013  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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
#pragma once

//
// p3GossipDiscovery is reponsible for facilitating the circulation of public keys between friend nodes.
//
// The service locally holds a cache that stores:
//     * the list of friend profiles, in each of which the list of locations with their own discovery flag (which means whether they allow discovery or not)
//     * the list of friend nodes, with their version number
//
// Data flow
// =========
//
//  statusChange(std::list<pqiServicePeer>&)         // called by pqiMonitor when peers are added,removed, or recently connected
//       |
//       +---- sendOwnContactInfo(RsPeerId)          // [On connection]      sends own PgpId, discovery flag, list of own signed GxsIds
//       |               |
//       |               +---->[to friend]
//       |
//       +---- addFriend() / removeFriend()          // [New/Removed friend] updates the list of friends, along with their own discovery flag
//
//  tick()
//   |
//   +------ handleIncoming()
//                  |
//                  +-- recvOwnContactInfo(RsPeerId)   // update location, IP addresses of a peer.
//                  |            |
//                  |            +------(if the peer has short_invite flag)
//                  |            |                      |
//                  |            |                      +---------requestPGPKey()->[to friend]      // requests the full PGP public key, so as to be
//                  |            |                                                                  // able to validate connections.
//                  |            |
//                  |            +------(if disc != RS_VS_DISC_OFF)
//                  |                                   |
//                  |                                   +---------sendPgpList()->[to friend]        // sends own list of friend profiles for which at least one location
//                  |                                                                               // accepts discovery
//                  +-- processContactInfo(item->PeerId(), contact);
//                  |
//                  +-- recvIdentityList(Gxs Identity List)
//                  |
//                  +-- recvPGPCertificate(item->PeerId(), pgpkey);
//                  |
//                  +-- processPGPList(pgplist->PeerId(), pgplist);
//                  |
//                  +-- recvPGPCertificateRequest(pgplist->PeerId(), pgplist);
//
// Notes:
//    * Tor nodes never send their own IP, and normal nodes never send their IP to Tor nodes either.
//      A Tor node may accidentally know the IP of a normal node when it adds its certificate. However, the IP is dropped and not saved in this case.
//      Generally speaking, no IP information should leave or transit through a Tor node.
//
//    * the decision to call recvOwnContactInfo() or processContactInfo() depends on whether the item's peer id is the one the info is about. This is
//      a bit unsafe. We should probably have to different items here especially if the information is not exactly the same.
//
#include <memory>

#include "retroshare/rsgossipdiscovery.h"
#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"
#include "pqi/pqiservicemonitor.h"
#include "gossipdiscovery/gossipdiscoveryitems.h"
#include "services/p3service.h"
#include "pqi/authgpg.h"
#include "gxs/rsgixs.h"

class p3ServiceControl;

struct DiscSslInfo
{
	DiscSslInfo() : mDiscStatus(RS_VS_DISC_OFF) {}	// default is to not allow discovery, until the peer tells about it
	uint16_t mDiscStatus;
};

struct DiscPeerInfo
{
	DiscPeerInfo() {}

	std::string mVersion;
};

struct DiscPgpInfo
{
	DiscPgpInfo() {}

	void mergeFriendList(const std::set<RsPgpId> &friends);

	std::set<RsPgpId> mFriendSet;
	std::map<RsPeerId, DiscSslInfo> mSslIds;
};


class p3discovery2 :
        public RsGossipDiscovery, public p3Service, public pqiServiceMonitor
        //public AuthGPGService
{
public:

	p3discovery2( p3PeerMgr* peerMgr, p3LinkMgr* linkMgr, p3NetMgr* netMgr,
	              p3ServiceControl* sc, RsGixs* gixs );
	virtual ~p3discovery2();

virtual RsServiceInfo getServiceInfo();

	/************* from pqiServiceMonitor *******************/
	virtual void statusChange(const std::list<pqiServicePeer> &plist);
	/************* from pqiServiceMonitor *******************/
	
	int	tick();
	
	/* external interface */
	bool getDiscFriends(const RsPeerId &id, std::list<RsPeerId> &friends);
	bool getDiscPgpFriends(const RsPgpId &pgpid, std::list<RsPgpId> &gpg_friends);
	bool getPeerVersion(const RsPeerId &id, std::string &version);
	bool getWaitingDiscCount(size_t &sendCount, size_t &recvCount);

    /************* from AuthGPService ****************/
	// virtual AuthGPGOperation *getGPGOperation();
	// virtual void setGPGOperation(AuthGPGOperation *operation);


private:

	RsPgpId getPGPId(const RsPeerId &id);

	int  handleIncoming();
	void updatePgpFriendList();

	void addFriend(const RsPeerId &sslId);
	void removeFriend(const RsPeerId &sslId);

	void updatePeerAddresses(const RsDiscContactItem *item);
	void updatePeerAddressList(const RsDiscContactItem *item);

	void sendOwnContactInfo(const RsPeerId &sslid);
	void recvOwnContactInfo(const RsPeerId &fromId, const RsDiscContactItem *item);

	void sendPGPList(const RsPeerId &toId);
	void processPGPList(const RsPeerId &fromId, const RsDiscPgpListItem *item);

	void processContactInfo(const RsPeerId &fromId, const RsDiscContactItem *info);

    // send/recv information

	void requestPGPCertificate(const RsPgpId &aboutId, const RsPeerId &toId);
	void recvPGPCertificateRequest(const RsPeerId& fromId, const RsDiscPgpListItem* item );
	void sendPGPCertificate(const RsPgpId &aboutId, const RsPeerId &toId);
	void recvPGPCertificate(const RsPeerId &fromId, RsDiscPgpKeyItem *item);
	void recvIdentityList(const RsPeerId& pid,const std::list<RsGxsId>& ids);

	bool setPeerVersion(const RsPeerId &peerId, const std::string &version);

	void rsEventsHandler(const RsEvent& event);
	RsEventsHandlerId_t mRsEventsHandle;

	p3PeerMgr *mPeerMgr;
	p3LinkMgr *mLinkMgr;
	p3NetMgr  *mNetMgr;
	p3ServiceControl *mServiceCtrl;
	RsGixs* mGixs;

	/* data */
	RsMutex mDiscMtx;

	void updatePeers_locked(const RsPeerId &aboutId);
	void sendContactInfo_locked(const RsPgpId &aboutId, const RsPeerId &toId);

	rstime_t mLastPgpUpdate;

	std::map<RsPgpId, DiscPgpInfo> mFriendList;
	std::map<RsPeerId, DiscPeerInfo> mLocationMap;

// This was used to async the receiving of PGP keys, mainly because PGPHandler cross-checks all signatures, so receiving these keys in large loads can be costly
// Because discovery is not running in the main thread, there's no reason to re-async this into another process (e.g. AuthGPG)
//
//	std::list<RsDiscPgpCertItem *> mPendingDiscPgpCertInList;

protected:
	RS_SET_CONTEXT_DEBUG_LEVEL(1)
};
