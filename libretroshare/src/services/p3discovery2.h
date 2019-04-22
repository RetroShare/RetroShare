/*******************************************************************************
 * libretroshare/src/services: p3discovery2.h                                  *
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

// Discovery2: Improved discovery service.

#include <memory>

#include "retroshare/rsdisc.h"

#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"

#include "pqi/pqiservicemonitor.h"
#include "rsitems/rsdiscovery2items.h"
#include "services/p3service.h"
#include "pqi/authgpg.h"
#include "gxs/rsgixs.h"

#ifndef RS_DEBUG_P3DISCOVERY
#	define RS_DEBUG_P3DISCOVERY 4
#endif

class p3ServiceControl;

typedef RsPgpId PGPID;
typedef RsPeerId SSLID;

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

	void mergeFriendList(const std::set<PGPID> &friends);

	std::set<PGPID> mFriendSet;
	std::map<SSLID, DiscSslInfo> mSslIds;
};


class p3discovery2 : public RsDisc, public p3Service, public pqiServiceMonitor,
        public AuthGPGService
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

	/// @see RsDisc
	bool sendInvite(
	        const RsPeerId& inviteId, const RsPeerId& toSslId,
	        std::string& errorMsg = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

	/// @see RsDisc
	bool requestInvite(
	        const RsPeerId& inviteId, const RsPeerId& toSslId,
	        std::string& errorMsg = RS_DEFAULT_STORAGE_PARAM(std::string)
	        ) override;

        /************* from AuthGPService ****************/
virtual AuthGPGOperation *getGPGOperation();
virtual void setGPGOperation(AuthGPGOperation *operation);


private:

	PGPID getPGPId(const SSLID &id);

	int  handleIncoming();
	void updatePgpFriendList();

	void addFriend(const SSLID &sslId);
	void removeFriend(const SSLID &sslId);

	void updatePeerAddresses(const RsDiscContactItem *item);
	void updatePeerAddressList(const RsDiscContactItem *item);

	void sendOwnContactInfo(const SSLID &sslid);
	void recvOwnContactInfo(const SSLID &fromId, const RsDiscContactItem *item);

	void sendPGPList(const SSLID &toId);
	void processPGPList(const SSLID &fromId, const RsDiscPgpListItem *item);

	void processContactInfo(const SSLID &fromId, const RsDiscContactItem *info);

	void requestPGPCertificate(const PGPID &aboutId, const SSLID &toId);

	void recvPGPCertificateRequest(
	        const RsPeerId& fromId, const RsDiscPgpListItem* item );

	void sendPGPCertificate(const PGPID &aboutId, const SSLID &toId);
	void recvPGPCertificate(const SSLID &fromId, RsDiscPgpCertItem *item);
	void recvIdentityList(const RsPeerId& pid,const std::list<RsGxsId>& ids);

	bool setPeerVersion(const SSLID &peerId, const std::string &version);

	void recvInvite(std::unique_ptr<RsGossipDiscoveryInviteItem> inviteItem);

	void rsEventsHandler(const RsEvent& event);
	RsEventsHandlerId_t mRsEventsHandle;


	p3PeerMgr *mPeerMgr;
	p3LinkMgr *mLinkMgr;
	p3NetMgr  *mNetMgr;
	p3ServiceControl *mServiceCtrl;
    RsGixs *mGixs ;

	/* data */
	RsMutex mDiscMtx;

	void updatePeers_locked(const SSLID &aboutId);
	void sendContactInfo_locked(const PGPID &aboutId, const SSLID &toId);

	rstime_t mLastPgpUpdate;

	std::map<PGPID, DiscPgpInfo> mFriendList;
	std::map<SSLID, DiscPeerInfo> mLocationMap;

	std::list<RsDiscPgpCertItem *> mPendingDiscPgpCertInList;
	std::list<RsDiscPgpCertItem *> mPendingDiscPgpCertOutList;

protected:
#if defined(RS_DEBUG_P3DISCOVERY) && RS_DEBUG_P3DISCOVERY == 1
	using Dbg1 = RsDbg;
	using Dbg2 = RsNoDbg;
	using Dbg3 = RsNoDbg;
#elif defined(RS_DEBUG_P3DISCOVERY) && RS_DEBUG_P3DISCOVERY == 2
	using Dbg1 = RsDbg;
	using Dbg2 = RsDbg;
	using Dbg3 = RsNoDbg;
#elif defined(RS_DEBUG_P3DISCOVERY) && RS_DEBUG_P3DISCOVERY >= 3
	using Dbg1 = RsDbg;
	using Dbg2 = RsDbg;
	using Dbg3 = RsDbg;
#else // RS_DEBUG_P3DISCOVERY
	using Dbg1 = RsNoDbg;
	using Dbg2 = RsNoDbg;
	using Dbg3 = RsNoDbg;
#endif // RS_DEBUG_P3DISCOVERY
};
