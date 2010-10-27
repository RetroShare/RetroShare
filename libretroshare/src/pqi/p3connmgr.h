/*
 * libretroshare/src/pqi: p3connmgr.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#ifndef MRK_PQI_CONNECTION_MANAGER_HEADER
#define MRK_PQI_CONNECTION_MANAGER_HEADER

#include "pqi/pqimonitor.h"
#include "pqi/pqiipset.h"

//#include "pqi/p3dhtmgr.h"
//#include "pqi/p3upnpmgr.h"
#include "pqi/pqiassist.h"

#include "pqi/p3cfgmgr.h"

#include "util/rsthreads.h"

class ExtAddrFinder ;

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


/* order of attempts ... */
const uint32_t RS_NET_CONN_TCP_ALL 		= 0x000f;
const uint32_t RS_NET_CONN_UDP_ALL 		= 0x00f0;
const uint32_t RS_NET_CONN_TUNNEL 		= 0x0f00;

const uint32_t RS_NET_CONN_TCP_LOCAL 		= 0x0001;
const uint32_t RS_NET_CONN_TCP_EXTERNAL 	= 0x0002;
const uint32_t RS_NET_CONN_TCP_UNKNOW_TOPOLOGY	= 0x0004;
const uint32_t RS_NET_CONN_UDP_DHT_SYNC 	= 0x0010;
const uint32_t RS_NET_CONN_UDP_PEER_SYNC 	= 0x0020; /* coming soon */

/* extra flags */
// not sure if needed yet.
//const uint32_t RS_NET_CONN_PEERAGE 		= 0x0f00;
//const uint32_t RS_NET_CONN_SERVER		= 0x0100; /* TCP only */
//const uint32_t RS_NET_CONN_PEER			= 0x0200; /* all UDP */


/* flags of peerStatus */
const uint32_t RS_NET_FLAGS_USE_DISC		= 0x0001;
const uint32_t RS_NET_FLAGS_USE_DHT		= 0x0002;
const uint32_t RS_NET_FLAGS_ONLINE		= 0x0004;
const uint32_t RS_NET_FLAGS_EXTERNAL_ADDR	= 0x0008;
const uint32_t RS_NET_FLAGS_STABLE_UDP		= 0x0010;
const uint32_t RS_NET_FLAGS_TRUSTS_ME 		= 0x0020;

const uint32_t RS_TCP_STD_TIMEOUT_PERIOD	= 5; /* 5 seconds! */

class peerAddrInfo
{
	public:
	peerAddrInfo(); /* init */

	bool 		found;
	uint32_t 	type;
	pqiIpAddrSet	addrs;
	time_t		ts;
};

class peerConnectAddress
{
	public:
	peerConnectAddress(); /* init */

	struct sockaddr_in addr;
	uint32_t delay;  /* to stop simultaneous connects */
	uint32_t period; /* UDP only */
	uint32_t type;
	time_t ts;
};

class peerConnectState
{
	public:
	peerConnectState(); /* init */

	std::string id;
	std::string gpg_id;

	uint32_t netMode; /* EXT / UPNP / UDP / INVALID */
	uint32_t visState; /* STD, GRAY, DARK */	

	struct sockaddr_in localaddr, serveraddr;

        //used to store current ip (for config and connection management)
	struct sockaddr_in currentlocaladdr;             /* Mandatory */
	struct sockaddr_in currentserveraddr;            /* Mandatory */
        std::string dyndns;

        time_t lastcontact; 

	/* list of addresses from various sources */
	pqiIpAddrSet ipAddrs;

	/***** Below here not stored permanently *****/

	uint32_t connecttype;  // RS_NET_CONN_TCP_ALL / RS_NET_CONN_UDP_ALL
        time_t lastavailable;
	time_t lastattempt;

	std::string name;
        std::string location;

	uint32_t    state;
	uint32_t    actions;

	uint32_t		source; /* most current source */
	peerAddrInfo		dht;
	peerAddrInfo		disc;
	peerAddrInfo		peer;

	/* a list of connect attempts to make (in order) */
	bool inConnAttempt;
	peerConnectAddress currentConnAddrAttempt;
	std::list<peerConnectAddress> connAddrs;


};

class pqiNetStatus
{
	public:

	pqiNetStatus();

        bool mLocalAddrOk;     // Local address is not loopback.
        bool mExtAddrOk;       // have external address.
        bool mExtAddrStableOk; // stable external address.
        bool mUpnpOk;          // upnp is ok.
        bool mDhtOk;           // dht is ok.

	struct sockaddr_in mLocalAddr; // percieved ext addr.
	struct sockaddr_in mExtAddr; // percieved ext addr.

	bool mResetReq; // Not Used yet!.

	void print(std::ostream &out);

	bool NetOk() // minimum to believe network is okay.`
	{
		return (mLocalAddrOk && mExtAddrOk);
	}
};

class p3tunnel; 
class RsPeerGroupItem;
class RsGroupInfo;

std::string textPeerConnectState(peerConnectState &state);


class p3ConnectMgr: public pqiConnectCb, public p3Config
{
	public:

        p3ConnectMgr();

void 	tick();

	/*************** Setup ***************************/
void	addNetAssistConnect(uint32_t type, pqiNetAssistConnect *);
void	addNetAssistFirewall(uint32_t type, pqiNetAssistFirewall *);

void    addNetListener(pqiNetListener *listener);

bool	checkNetAddress(); /* check our address is sensible */

	/*************** External Control ****************/
bool	shutdown(); /* blocking shutdown call */

bool	retryConnect(std::string id);

bool    getUPnPState();
bool	getUPnPEnabled();
bool	getDHTEnabled();

bool  getIPServersEnabled();
void  setIPServersEnabled(bool b) ;
void  getIPServersList(std::list<std::string>& ip_servers) ;

void 	setTunnelConnection(bool b);
bool 	getTunnelConnection();

bool	getNetStatusLocalOk();
bool	getNetStatusUpnpOk();
bool	getNetStatusDhtOk();
bool	getNetStatusStunOk();
bool	getNetStatusExtraAddressCheckOk();

bool 	getUpnpExtAddress(struct sockaddr_in &addr);
bool 	getExtFinderAddress(struct sockaddr_in &addr);
void 	getNetStatus(pqiNetStatus &status);

void 	setOwnNetConfig(uint32_t netMode, uint32_t visState);
bool 	setLocalAddress(std::string id, struct sockaddr_in addr);
bool 	setExtAddress(std::string id, struct sockaddr_in addr);
bool    setDynDNS(std::string id, std::string dyndns);
bool    updateAddressList(const std::string& id, const pqiIpAddrSet &addrs);

bool 	setNetworkMode(std::string id, uint32_t netMode);
bool 	setVisState(std::string id, uint32_t visState);

bool    setLocation(std::string pid, std::string location);//location is shown in the gui to differentiate ssl certs

	/* add/remove friends */
bool 	addFriend(std::string ssl_id, std::string gpg_id, uint32_t netMode = RS_NET_MODE_UDP,
	   uint32_t visState = RS_VIS_STATE_STD , time_t lastContact = 0);

bool	removeFriend(std::string ssl_id);
bool	addNeighbour(std::string);

	/*************** External Control ****************/

	/* access to network details (called through Monitor) */
const std::string getOwnId();
bool	getOwnNetStatus(peerConnectState &state);

bool	isFriend(std::string ssl_id);
bool	isOnline(std::string ssl_id);
bool	getFriendNetStatus(std::string id, peerConnectState &state);
bool	getOthersNetStatus(std::string id, peerConnectState &state);

void	getOnlineList(std::list<std::string> &ssl_peers);
void	getFriendList(std::list<std::string> &ssl_peers);
//void	getOthersList(std::list<std::string> &peers); /deprecated
bool    getPeerCount (unsigned int *pnFriendCount, unsigned int *pnOnlineCount, bool ssl);


	/**************** handle monitors *****************/
void	addMonitor(pqiMonitor *mon);
void	removeMonitor(pqiMonitor *mon);

	/******* overloaded from pqiConnectCb *************/
virtual void    peerStatus(std::string id, const pqiIpAddrSet &addrs, 
                        uint32_t type, uint32_t flags, uint32_t source);
virtual void    peerConnectRequest(std::string id, 
			struct sockaddr_in raddr, uint32_t source);
//virtual void    stunStatus(std::string id, struct sockaddr_in raddr, uint32_t type, uint32_t flags);

	/****************** Connections *******************/
bool 	connectAttempt(std::string id, struct sockaddr_in &addr, 
				uint32_t &delay, uint32_t &period, uint32_t &type);
bool 	connectResult(std::string id, bool success, uint32_t flags, struct sockaddr_in remote_peer_address);

	/******************** Groups **********************/
bool    addGroup(RsGroupInfo &groupInfo);
bool    editGroup(const std::string &groupId, RsGroupInfo &groupInfo);
bool    removeGroup(const std::string &groupId);
bool    getGroupInfo(const std::string &groupId, RsGroupInfo &groupInfo);
bool    getGroupInfoList(std::list<RsGroupInfo> &groupInfoList);
bool    assignPeersToGroup(const std::string &groupId, const std::list<std::string> &peerIds, bool assign);


protected:
	/****************** Internal Interface *******************/
virtual bool enableNetAssistFirewall(bool on);
virtual bool netAssistFirewallEnabled();
virtual bool netAssistFirewallActive();
virtual bool netAssistFirewallShutdown();

virtual bool enableNetAssistConnect(bool on);
virtual bool netAssistConnectEnabled();
virtual bool netAssistConnectActive();
virtual bool netAssistConnectShutdown();

/* Assist Firewall */
bool netAssistExtAddress(struct sockaddr_in &extAddr);
bool netAssistFirewallPorts(uint16_t iport, uint16_t eport);

		/* Assist Connect */
virtual bool netAssistFriend(std::string id, bool on);
virtual bool netAssistSetAddress( struct sockaddr_in &laddr,
                                        struct sockaddr_in &eaddr,
					uint32_t mode);


	/* Internal Functions */
void 	netReset();

void 	statusTick();
void 	netTick();
void 	netStartup();

	/* startup the bits */
void 	netDhtInit();
void 	netUdpInit();
void 	netStunInit();



void	netInit();

void 	netExtInit();
void 	netExtCheck();

void 	netUpnpInit();
void 	netUpnpCheck();

void    netUnreachableCheck();

void 	networkConsistencyCheck();

	/* monitor control */
void 	tickMonitors();

	/* connect attempts UDP */
bool    retryConnectUDP(std::string id, struct sockaddr_in &rUdpAddr);

	/* connect attempts TCP */
bool	retryConnectTCP(std::string id);

void 	locked_ConnectAttempt_CurrentAddresses(peerConnectState *peer);
void 	locked_ConnectAttempt_HistoricalAddresses(peerConnectState *peer);
void 	locked_ConnectAttempt_AddDynDNS(peerConnectState *peer);
void 	locked_ConnectAttempt_AddTunnel(peerConnectState *peer);

bool  	locked_ConnectAttempt_Complete(peerConnectState *peer);

bool  	locked_CheckPotentialAddr(struct sockaddr_in *addr, time_t age);
bool 	addAddressIfUnique(std::list<peerConnectAddress> &addrList, 
					peerConnectAddress &pca);


	protected:
/*****************************************************************/
/***********************  p3config  ******************************/
        /* Key Functions to be overloaded for Full Configuration */
	virtual RsSerialiser *setupSerialiser();
	virtual std::list<RsItem *> saveList(bool &cleanup);
	virtual void saveDone();
	virtual bool    loadList(std::list<RsItem *> load);
/*****************************************************************/

//void 	setupOwnNetConfig(RsPeerConfigItem *item);
//void 	addPeer(RsPeerConfigItem *item);

private:
	// These should have there own Mutex Protection,
	//p3tunnel *mP3tunnel;
	ExtAddrFinder *mExtAddrFinder ;

	/* These are considered static from a MUTEX perspective */
	std::map<uint32_t, pqiNetAssistFirewall *> mFwAgents;
	std::map<uint32_t, pqiNetAssistConnect  *> mDhts;

        std::list<pqiNetListener *> mNetListeners;


	RsMutex connMtx; /* protects below */

void 	netStatusReset_locked();

        uint32_t mRetryPeriod;

	time_t   mNetInitTS;
	uint32_t mNetStatus;

	bool     mStatusChanged;

	std::list<pqiMonitor *> clients;

	bool mUseExtAddrFinder;
	bool mAllowTunnelConnection;

	/* external Address determination */
	//bool mUpnpAddrValid, mStunAddrValid;
	//struct sockaddr_in mUpnpExtAddr;

	/* network status flags (read by rsiface) */
	pqiNetStatus mNetFlags;
	pqiNetStatus mOldNetFlags;

	peerConnectState mOwnState;

	std::map<std::string, peerConnectState> mFriendList;
	std::map<std::string, peerConnectState> mOthersList;

	std::list<RsPeerGroupItem *> groupList;
	uint32_t lastGroupId;

	std::list<RsItem *> saveCleanupList; /* TEMPORARY LIST WHEN SAVING */


	/* relatively static list of banned ip addresses */
	std::list<struct in_addr> mBannedIpList;
};

#endif // MRK_PQI_CONNECTION_MANAGER_HEADER
