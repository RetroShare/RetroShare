/*
 * libretroshare/src/services: p3discovery2.h
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

#ifndef MRK_SERVICES_DISCOVERY2_H
#define MRK_SERVICES_DISCOVERY2_H

// Discovery2: Improved discovery service.

#include "retroshare/rsdisc.h"

#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"

#include "pqi/pqiservicemonitor.h"
#include "serialiser/rsdiscovery2items.h"
#include "services/p3service.h"
#include "pqi/authgpg.h"

class p3ServiceControl;


typedef RsPgpId PGPID;
typedef RsPeerId SSLID;

class DiscSslInfo
{
	public:
	DiscSslInfo() { mDiscStatus = 0; }
	uint16_t mDiscStatus;
};

class DiscPeerInfo
{
	public:
	DiscPeerInfo() {}

	std::string mVersion;
	//uint32_t mStatus;
};

class DiscPgpInfo
{
	public:
	DiscPgpInfo() {}

void    mergeFriendList(const std::list<PGPID> &friends);

	//PGPID mPgpId;
	std::set<PGPID> mFriendSet;
	std::map<SSLID, DiscSslInfo> mSslIds;
};



class p3discovery2: public RsDisc, public p3Service, public pqiServiceMonitor, public AuthGPGService
{
	public:

	p3discovery2(p3PeerMgr *peerMgr, p3LinkMgr *linkMgr, p3NetMgr *netMgr, p3ServiceControl *sc);
virtual ~p3discovery2();

virtual RsServiceInfo getServiceInfo();

	/************* from pqiServiceMonitor *******************/
	virtual void statusChange(const std::list<pqiServicePeer> &plist);
	/************* from pqiServiceMonitor *******************/
	
	int	tick();
	
	/* external interface */
virtual bool    getDiscFriends(const RsPeerId &id, std::list<RsPeerId> &friends);
virtual bool    getDiscPgpFriends(const RsPgpId &pgpid, std::list<RsPgpId> &gpg_friends);
virtual bool    getPeerVersion(const RsPeerId &id, std::string &version);
virtual bool    getWaitingDiscCount(unsigned int *sendCount, unsigned int *recvCount);

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
	void recvPGPCertificateRequest(const SSLID &fromId, const RsDiscPgpListItem *item);
	void sendPGPCertificate(const PGPID &aboutId, const SSLID &toId);
	void recvPGPCertificate(const SSLID &fromId, RsDiscPgpCertItem *item);

	bool setPeerVersion(const SSLID &peerId, const std::string &version);

	private:

	p3PeerMgr *mPeerMgr;
	p3LinkMgr *mLinkMgr;
	p3NetMgr  *mNetMgr;
	p3ServiceControl *mServiceCtrl;

	/* data */
	RsMutex mDiscMtx;

	void updatePeers_locked(const SSLID &aboutId);
	void sendContactInfo_locked(const PGPID &aboutId, const SSLID &toId);

	time_t mLastPgpUpdate;

	std::map<PGPID, DiscPgpInfo> mFriendList;
	std::map<SSLID, DiscPeerInfo> mLocationMap;

	std::list<RsDiscPgpCertItem *> mPendingDiscPgpCertInList;
	std::list<RsDiscPgpCertItem *> mPendingDiscPgpCertOutList;
};


#endif // MRK_SERVICES_DISCOVERY2_H
