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
#include "pqi/p3authmgr.h"

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

const uint32_t RS_NET_CONN_TCP_LOCAL 		= 0x0001;
const uint32_t RS_NET_CONN_TCP_EXTERNAL 	= 0x0002;
const uint32_t RS_NET_CONN_UDP_DHT_SYNC 	= 0x0010;
const uint32_t RS_NET_CONN_UDP_PEER_SYNC 	= 0x0020; /* coming soon */

/* extra flags */
// not sure if needed yet.
//const uint32_t RS_NET_CONN_PEERAGE 		= 0x0f00;
//const uint32_t RS_NET_CONN_SERVER		= 0x0100; /* TCP only */
//const uint32_t RS_NET_CONN_PEER			= 0x0200; /* all UDP */


/* flags of peerStatus */
const uint32_t RS_NET_FLAGS_USE_DISC		= 0x0001;
const uint32_t RS_NET_FLAGS_USE_DHT			= 0x0002;
const uint32_t RS_NET_FLAGS_ONLINE			= 0x0004;
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
	struct sockaddr_in laddr, raddr;
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

	uint32_t netMode; /* EXT / UPNP / UDP / INVALID */
	uint32_t visState; /* STD, GRAY, DARK */	

	struct sockaddr_in localaddr, serveraddr;

        time_t lastcontact; 

	/***** Below here not stored permanently *****/

	uint32_t connecttype;  // RS_NET_CONN_TCP_ALL / RS_NET_CONN_UDP_ALL
        time_t lastavailable;
	time_t lastattempt;

	std::string name;

	uint32_t    state;
	uint32_t    actions;

	uint32_t		source; /* most current source */
	peerAddrInfo		dht;
	peerAddrInfo		disc;
	peerAddrInfo		peer;

	/* a list of connect attempts to make (in order) */
	bool inConnAttempt;
	peerConnectAddress currentConnAddr;
	std::list<peerConnectAddress> connAddrs;

};


class p3ConnectMgr: public pqiConnectCb, public p3Config
{
	public:

	p3ConnectMgr(p3AuthMgr *authMgr); 

void 	tick();

	/*************** Setup ***************************/
void	addNetAssistConnect(uint32_t type, pqiNetAssistConnect *);
void	addNetAssistFirewall(uint32_t type, pqiNetAssistFirewall *);

bool	checkNetAddress(); /* check our address is sensible */

	/*************** External Control ****************/
bool	shutdown(); /* blocking shutdown call */

bool	retryConnect(std::string id);

bool    getUPnPState();
bool	getUPnPEnabled();
bool	getDHTEnabled();

bool  getIPServersEnabled() { return use_extr_addr_finder ;}
void  setIPServersEnabled(bool b) ;
void  getIPServersList(std::list<std::string>& ip_servers) ;

bool	getNetStatusOk();
bool	getNetStatusUpnpOk();
bool	getNetStatusDhtOk();
bool	getNetStatusExtOk();
bool	getNetStatusUdpOk();
bool	getNetStatusTcpOk();
bool	getNetResetReq();

void 	setOwnNetConfig(uint32_t netMode, uint32_t visState);
bool 	setLocalAddress(std::string id, struct sockaddr_in addr);
bool 	setExtAddress(std::string id, struct sockaddr_in addr);

bool 	setNetworkMode(std::string id, uint32_t netMode);
bool 	setVisState(std::string id, uint32_t visState);

	/* add/remove friends */
bool 	addFriend(std::string id, uint32_t netMode = RS_NET_MODE_UDP, 
	   uint32_t visState = RS_VIS_STATE_STD , time_t lastContact = 0);

bool	removeFriend(std::string);
bool	addNeighbour(std::string);

	/*************** External Control ****************/

	/* access to network details (called through Monitor) */
const std::string getOwnId();
bool	getOwnNetStatus(peerConnectState &state);

bool	isFriend(std::string id);
bool	isOnline(std::string id);
bool	getFriendNetStatus(std::string id, peerConnectState &state);
bool	getOthersNetStatus(std::string id, peerConnectState &state);

void	getOnlineList(std::list<std::string> &peers);
void	getFriendList(std::list<std::string> &peers);
void	getOthersList(std::list<std::string> &peers);


	/**************** handle monitors *****************/
void	addMonitor(pqiMonitor *mon);
void	removeMonitor(pqiMonitor *mon);

	/******* overloaded from pqiConnectCb *************/
virtual void    peerStatus(std::string id, 
			struct sockaddr_in laddr, struct sockaddr_in raddr,
                        uint32_t type, uint32_t flags, uint32_t source);
virtual void    peerConnectRequest(std::string id, 
			struct sockaddr_in raddr, uint32_t source);
virtual void    stunStatus(std::string id, struct sockaddr_in raddr, uint32_t type, uint32_t flags);

	/****************** Connections *******************/
bool 	connectAttempt(std::string id, struct sockaddr_in &addr, 
				uint32_t &delay, uint32_t &period, uint32_t &type);
bool 	connectResult(std::string id, bool success, uint32_t flags);


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
virtual bool netAssistAddStun(std::string id);
virtual bool netAssistStun(bool on);
virtual bool netAssistNotify(std::string id);
virtual bool netAssistSetAddress( struct sockaddr_in &laddr,
                                        struct sockaddr_in &eaddr,
					uint32_t mode);


	/* Internal Functions */
void 	statusTick();
void 	netTick();
void 	netStartup();

	/* startup the bits */
void 	netDhtInit();
void 	netUdpInit();
void 	netStunInit();

void 	netStatusReset();


void	netInit();

void 	netExtInit();
void 	netExtCheck();

void 	netUpnpInit();
void 	netUpnpCheck();

void 	netUdpCheck();
void    netUnreachableCheck();

	/* Udp / Stun functions */
bool 	udpInternalAddress(struct sockaddr_in iaddr);
bool 	udpExtAddressCheck();
void 	udpStunPeer(std::string id, struct sockaddr_in &addr);

void 	stunInit();
bool 	stunCheck();
void 	stunCollect(std::string id, struct sockaddr_in addr, uint32_t flags);
bool    addBootstrapStunPeers();

	/* monitor control */
void 	tickMonitors();

	/* connect attempts */
bool	retryConnectTCP(std::string id);
bool	retryConnectNotify(std::string id);

	/* temporary for testing */
virtual void 	loadConfiguration() { return; }

	protected:
/*****************************************************************/
/***********************  p3config  ******************************/
        /* Key Functions to be overloaded for Full Configuration */
	virtual RsSerialiser *setupSerialiser();
	virtual std::list<RsItem *> saveList(bool &cleanup);
	virtual bool    loadList(std::list<RsItem *> load);
/*****************************************************************/


#if 0

void 	setupOwnNetConfig(RsPeerConfigItem *item);
void 	addPeer(RsPeerConfigItem *item);

#endif

private:

	p3AuthMgr *mAuthMgr;

	std::map<uint32_t, pqiNetAssistFirewall *> mFwAgents;
	std::map<uint32_t, pqiNetAssistConnect  *> mDhts;

	RsMutex connMtx; /* protects below */

	time_t   mNetInitTS;
	uint32_t mNetStatus;

	uint32_t mStunStatus;
	uint32_t mStunFound;
	bool 	 mStunMoreRequired;

	bool     mStatusChanged;

	std::list<pqiMonitor *> clients;

	ExtAddrFinder *mExtAddrFinder ;
	bool use_extr_addr_finder ;

	/* external Address determination */
	bool mUpnpAddrValid, mStunAddrValid;
	bool mStunAddrStable;
	struct sockaddr_in mUpnpExtAddr;
	struct sockaddr_in mStunExtAddr;

	/* network status flags (read by rsiface) */
	bool netFlagOk;
	bool netFlagUpnpOk;
	bool netFlagDhtOk;
	bool netFlagExtOk;
	bool netFlagUdpOk;
	bool netFlagTcpOk;
	bool netFlagResetReq;


	/* these are protected for testing */
protected:

void addPeer(std::string id, std::string name); /* tmp fn */

	peerConnectState ownState;

	std::list<std::string> mStunList;
	std::map<std::string, peerConnectState> mFriendList;
	std::map<std::string, peerConnectState> mOthersList;
};

#endif // MRK_PQI_CONNECTION_MANAGER_HEADER




