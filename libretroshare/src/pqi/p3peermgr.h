/*
 * libretroshare/src/pqi: p3peermgr.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2011 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
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

#ifndef MRK_PQI_PEER_MANAGER_HEADER
#define MRK_PQI_PEER_MANAGER_HEADER

#include "pqi/pqimonitor.h"
#include "pqi/pqiipset.h"
#include "pqi/pqiassist.h"

#include "pqi/p3cfgmgr.h"

#include "util/rsthreads.h"

	/* RS_VIS_STATE_XXXX
	 * determines how public this peer wants to be...
	 *
	 * STD = advertise to Peers / DHT checking etc 
	 * GRAY = share with friends / but not DHT 
	 * DARK = hidden from all 
	 * BROWN? = hidden from friends / but on DHT
	 */

const uint32_t RS_VIS_STATE_NODISC = 0x0001;
const uint32_t RS_VIS_STATE_NODHT  = 0x0002;

const uint32_t RS_VIS_STATE_STD    = 0x0000;
const uint32_t RS_VIS_STATE_GRAY   = RS_VIS_STATE_NODHT;
const uint32_t RS_VIS_STATE_DARK   = RS_VIS_STATE_NODISC | RS_VIS_STATE_NODHT;
const uint32_t RS_VIS_STATE_BROWN  = RS_VIS_STATE_NODISC;



	/* Startup Modes (confirmed later) */
const uint32_t RS_NET_MODE_TRYMODE =    0x00f0;

const uint32_t RS_NET_MODE_TRY_EXT  =   0x0010;
const uint32_t RS_NET_MODE_TRY_UPNP =   0x0020;
const uint32_t RS_NET_MODE_TRY_UDP  =   0x0040;

	/* Actual State */
const uint32_t RS_NET_MODE_ACTUAL =      0x000f;

const uint32_t RS_NET_MODE_UNKNOWN =     0x0000;
const uint32_t RS_NET_MODE_EXT =         0x0001;
const uint32_t RS_NET_MODE_UPNP =        0x0002;
const uint32_t RS_NET_MODE_UDP =         0x0004;
const uint32_t RS_NET_MODE_UNREACHABLE = 0x0008;


/* flags of peerStatus */
const uint32_t RS_NET_FLAGS_USE_DISC		= 0x0001;
const uint32_t RS_NET_FLAGS_USE_DHT		= 0x0002;
const uint32_t RS_NET_FLAGS_ONLINE		= 0x0004;
const uint32_t RS_NET_FLAGS_EXTERNAL_ADDR	= 0x0008;
const uint32_t RS_NET_FLAGS_STABLE_UDP		= 0x0010;
const uint32_t RS_NET_FLAGS_TRUSTS_ME 		= 0x0020;

class peerState
{
	public:
	peerState(); /* init */

	std::string id;
	std::string gpg_id;

	uint32_t netMode; /* EXT / UPNP / UDP / INVALID */
	uint32_t visState; /* STD, GRAY, DARK */	

        struct sockaddr_in localaddr;
        struct sockaddr_in serveraddr;
        std::string dyndns;

        time_t lastcontact;

	/* list of addresses from various sources */
	pqiIpAddrSet ipAddrs;

	std::string location;
	std::string name;

};

class RsPeerGroupItem;
class RsGroupInfo;

std::string textPeerState(peerState &state);

class p3LinkMgr;
class p3NetMgr;

class p3LinkMgrIMPL;
class p3NetMgrIMPL;

class p3PeerMgr
{
	public:

        p3PeerMgr() { return; }
virtual ~p3PeerMgr() { return; }

virtual bool 	addFriend(const std::string &ssl_id, const std::string &gpg_id, uint32_t netMode = RS_NET_MODE_UDP,
	   uint32_t visState = RS_VIS_STATE_STD , time_t lastContact = 0) = 0;
virtual bool	removeFriend(const std::string &ssl_id) = 0;

virtual bool	isFriend(const std::string &ssl_id) = 0;



	/******************** Groups **********************/
	/* This is solely used by p3peers - makes sense   */

virtual bool    addGroup(RsGroupInfo &groupInfo) = 0;
virtual bool    editGroup(const std::string &groupId, RsGroupInfo &groupInfo) = 0;
virtual bool    removeGroup(const std::string &groupId) = 0;
virtual bool    getGroupInfo(const std::string &groupId, RsGroupInfo &groupInfo) = 0;
virtual bool    getGroupInfoList(std::list<RsGroupInfo> &groupInfoList) = 0;
virtual bool    assignPeersToGroup(const std::string &groupId, const std::list<std::string> &peerIds, bool assign) = 0;


	/**************** Set Net Info ****************/
	/*
	 * These functions are used by:
	 * 1) p3linkmgr 
	 * 2) p3peers - reasonable
	 * 3) p3disc  - reasonable
	 */

virtual bool 	setLocalAddress(const std::string &id, struct sockaddr_in addr) = 0;
virtual bool 	setExtAddress(const std::string &id, struct sockaddr_in addr) = 0;
virtual bool    setDynDNS(const std::string &id, const std::string &dyndns) = 0;

virtual bool 	setNetworkMode(const std::string &id, uint32_t netMode) = 0;
virtual bool 	setVisState(const std::string &id, uint32_t visState) = 0;

virtual bool    setLocation(const std::string &pid, const std::string &location) = 0;

virtual bool    updateCurrentAddress(const std::string& id, const pqiIpAddress &addr) = 0;
virtual bool    updateLastContact(const std::string& id) = 0;
virtual bool    updateAddressList(const std::string& id, const pqiIpAddrSet &addrs) = 0;


		// THIS MUST ONLY BE CALLED BY NETMGR!!!!
virtual bool    UpdateOwnAddress(const struct sockaddr_in &mLocalAddr, const struct sockaddr_in &mExtAddr) = 0;

	/**************** Net Status Info ****************/
	/*
	 * MUST RATIONALISE THE DATA FROM THESE FUNCTIONS
	 * These functions are used by:
	 * 1) p3face-config ... to remove!
	 * 2) p3peers - reasonable
	 * 3) p3disc  - reasonable
	 */

virtual bool	getOwnNetStatus(peerState &state) = 0;
virtual bool	getFriendNetStatus(const std::string &id, peerState &state) = 0;
virtual bool	getOthersNetStatus(const std::string &id, peerState &state) = 0;


        /************* DEPRECIATED FUNCTIONS (TO REMOVE) ********/

		// Single Use Function... shouldn't be here. used by p3serverconfig.cc
virtual bool 	haveOnceConnected() = 0;


/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/


};


class p3PeerMgrIMPL: public p3PeerMgr, public p3Config
{
	public:

/************************************************************************************************/
/* EXTERNAL INTERFACE */
/************************************************************************************************/

virtual bool 	addFriend(const std::string &ssl_id, const std::string &gpg_id, uint32_t netMode = RS_NET_MODE_UDP,
	   uint32_t visState = RS_VIS_STATE_STD , time_t lastContact = 0);
virtual bool	removeFriend(const std::string &ssl_id);

virtual bool	isFriend(const std::string &ssl_id);


	/******************** Groups **********************/
	/* This is solely used by p3peers - makes sense   */

virtual bool    addGroup(RsGroupInfo &groupInfo);
virtual bool    editGroup(const std::string &groupId, RsGroupInfo &groupInfo);
virtual bool    removeGroup(const std::string &groupId);
virtual bool    getGroupInfo(const std::string &groupId, RsGroupInfo &groupInfo);
virtual bool    getGroupInfoList(std::list<RsGroupInfo> &groupInfoList);
virtual bool    assignPeersToGroup(const std::string &groupId, const std::list<std::string> &peerIds, bool assign);


	/**************** Set Net Info ****************/
	/*
	 * These functions are used by:
	 * 1) p3linkmgr 
	 * 2) p3peers - reasonable
	 * 3) p3disc  - reasonable
	 */

virtual bool 	setLocalAddress(const std::string &id, struct sockaddr_in addr);
virtual bool 	setExtAddress(const std::string &id, struct sockaddr_in addr);
virtual bool    setDynDNS(const std::string &id, const std::string &dyndns);

virtual bool 	setNetworkMode(const std::string &id, uint32_t netMode);
virtual bool 	setVisState(const std::string &id, uint32_t visState);

virtual bool    setLocation(const std::string &pid, const std::string &location);

virtual bool    updateCurrentAddress(const std::string& id, const pqiIpAddress &addr);
virtual bool    updateLastContact(const std::string& id);
virtual bool    updateAddressList(const std::string& id, const pqiIpAddrSet &addrs);


		// THIS MUST ONLY BE CALLED BY NETMGR!!!!
virtual bool    UpdateOwnAddress(const struct sockaddr_in &mLocalAddr, const struct sockaddr_in &mExtAddr);
	/**************** Net Status Info ****************/
	/*
	 * MUST RATIONALISE THE DATA FROM THESE FUNCTIONS
	 * These functions are used by:
	 * 1) p3face-config ... to remove!
	 * 2) p3peers - reasonable
	 * 3) p3disc  - reasonable
	 */

virtual bool	getOwnNetStatus(peerState &state);
virtual bool	getFriendNetStatus(const std::string &id, peerState &state);
virtual bool	getOthersNetStatus(const std::string &id, peerState &state);


        /************* DEPRECIATED FUNCTIONS (TO REMOVE) ********/

		// Single Use Function... shouldn't be here. used by p3serverconfig.cc
virtual bool 	haveOnceConnected();


/************************************************************************************************/
/* Extra IMPL Functions (used by p3LinkMgr, p3NetMgr + Setup) */
/************************************************************************************************/

        p3PeerMgrIMPL();

void	setManagers(p3LinkMgrIMPL *linkMgr, p3NetMgrIMPL  *netMgr);

void 	tick();

const std::string getOwnId();
void 	setOwnNetworkMode(uint32_t netMode);
void 	setOwnVisState(uint32_t visState);

int 	getConnectAddresses(const std::string &id, 
				struct sockaddr_in &lAddr, struct sockaddr_in &eAddr, 
				pqiIpAddrSet &histAddrs, std::string &dyndns);

protected:
	/* Internal Functions */
void    printPeerLists(std::ostream &out);

	protected:
/*****************************************************************/
/***********************  p3config  ******************************/
        /* Key Functions to be overloaded for Full Configuration */
	virtual RsSerialiser *setupSerialiser();
	virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
	virtual void saveDone();
	virtual bool    loadList(std::list<RsItem *>& load);
/*****************************************************************/

	/* other important managers */

	p3LinkMgrIMPL *mLinkMgr;
	p3NetMgrIMPL  *mNetMgr;


private:
	RsMutex mPeerMtx; /* protects below */

	bool     mStatusChanged;

	std::list<pqiMonitor *> clients;

	peerState mOwnState;

	std::map<std::string, peerState> mFriendList;
	std::map<std::string, peerState> mOthersList;

	std::list<RsPeerGroupItem *> groupList;
	uint32_t lastGroupId;

	std::list<RsItem *> saveCleanupList; /* TEMPORARY LIST WHEN SAVING */
};

#endif // MRK_PQI_PEER_MANAGER_HEADER
