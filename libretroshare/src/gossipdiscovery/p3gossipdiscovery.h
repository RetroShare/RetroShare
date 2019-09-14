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
	DiscSslInfo() : mDiscStatus(0) {}
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
