/*
 * libretroshare/src/pqi: p3linkmgr.h
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

#ifndef MRK_PQI_LINK_MANAGER_HEADER
#define MRK_PQI_LINK_MANAGER_HEADER

#include "pqi/pqimonitor.h"
#include "pqi/pqiipset.h"

//#include "pqi/p3dhtmgr.h"
//#include "pqi/p3upnpmgr.h"
#include "pqi/pqiassist.h"

#include "pqi/p3cfgmgr.h"

#include "util/rsthreads.h"

class ExtAddrFinder ;
class DNSResolver ;

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
	//std::string gpg_id;

	//uint32_t netMode; /* EXT / UPNP / UDP / INVALID */
	//uint32_t visState; /* STD, GRAY, DARK */	

	//struct sockaddr_in localaddr, serveraddr;

        //used to store current ip (for config and connection management)
	//struct sockaddr_in currentlocaladdr;             /* Mandatory */
	//struct sockaddr_in currentserveraddr;            /* Mandatory */
        //std::string dyndns;


	/* list of addresses from various sources */
	//pqiIpAddrSet ipAddrs;

	/***** Below here not stored permanently *****/

	bool dhtVisible;

        //time_t lastcontact; 

	uint32_t connecttype;  // RS_NET_CONN_TCP_ALL / RS_NET_CONN_UDP_ALL
        time_t lastavailable;
	time_t lastattempt;

	std::string name;

        //std::string location;

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

class p3tunnel; 
class RsPeerGroupItem;
class RsGroupInfo;

class p3PeerMgr;
class p3NetMgr;

std::string textPeerConnectState(peerConnectState &state);


class p3LinkMgr: public pqiConnectCb
{
	public:

        p3LinkMgr(p3PeerMgr *peerMgr, p3NetMgr *netMgr);

void 	tick();

	/*************** Setup ***************************/
//void	addNetAssistConnect(uint32_t type, pqiNetAssistConnect *);
//void	addNetAssistFirewall(uint32_t type, pqiNetAssistFirewall *);

//void    addNetListener(pqiNetListener *listener);

//bool	checkNetAddress(); /* check our address is sensible */

	/*************** External Control ****************/
bool	shutdown(); /* blocking shutdown call */

bool	retryConnect(const std::string &id);

void 	setTunnelConnection(bool b);
bool 	getTunnelConnection();

void    setFriendVisibility(const std::string &id, bool isVisible);

//void 	setOwnNetConfig(uint32_t netMode, uint32_t visState);
//bool 	setLocalAddress(const std::string &id, struct sockaddr_in addr);
//bool 	setExtAddress(const std::string &id, struct sockaddr_in addr);
//bool    setDynDNS(const std::string &id, const std::string &dyndns);
//bool    updateAddressList(const std::string& id, const pqiIpAddrSet &addrs);

//bool 	setNetworkMode(const std::string &id, uint32_t netMode);
//bool 	setVisState(const std::string &id, uint32_t visState);

//bool    setLocation(const std::string &pid, const std::string &location);//location is shown in the gui to differentiate ssl certs

	/* add/remove friends */
int 	addFriend(const std::string &ssl_id, bool isVisible);
int 	removeFriend(const std::string &ssl_id);

	/*************** External Control ****************/

	/* access to network details (called through Monitor) */
const std::string getOwnId();
bool	getOwnNetStatus(peerConnectState &state);

bool 	setLocalAddress(struct sockaddr_in addr);
struct sockaddr_in getLocalAddress();

bool	isOnline(const std::string &ssl_id);
bool	getFriendNetStatus(const std::string &id, peerConnectState &state);
//bool	getOthersNetStatus(const std::string &id, peerConnectState &state);

void	getOnlineList(std::list<std::string> &ssl_peers);
void	getFriendList(std::list<std::string> &ssl_peers);
int 	getOnlineCount();
int 	getFriendCount();


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
bool 	connectAttempt(const std::string &id, struct sockaddr_in &addr,
				uint32_t &delay, uint32_t &period, uint32_t &type);
bool 	connectResult(const std::string &id, bool success, uint32_t flags, struct sockaddr_in remote_peer_address);

protected:
	/****************** Internal Interface *******************/

//virtual bool enableNetAssistConnect(bool on);
//virtual bool netAssistConnectEnabled();
//virtual bool netAssistConnectActive();
//virtual bool netAssistConnectShutdown();
//virtual bool netAssistConnectStats(uint32_t &netsize, uint32_t &localnetsize);

		/* Assist Connect */
//virtual bool netAssistFriend(std::string id, bool on);
//virtual bool netAssistSetAddress( struct sockaddr_in &laddr,
//                                        struct sockaddr_in &eaddr,
//					uint32_t mode);


	/* Internal Functions */
void 	statusTick();

	/* monitor control */
void 	tickMonitors();

	/* connect attempts UDP */
bool    retryConnectUDP(const std::string &id, struct sockaddr_in &rUdpAddr);

	/* connect attempts TCP */
bool	retryConnectTCP(const std::string &id);

void 	locked_ConnectAttempt_CurrentAddresses(peerConnectState *peer, struct sockaddr_in *localAddr, struct sockaddr_in *serverAddr);
void 	locked_ConnectAttempt_HistoricalAddresses(peerConnectState *peer, const pqiIpAddrSet &ipAddrs);
void 	locked_ConnectAttempt_AddDynDNS(peerConnectState *peer, std::string dyndns, uint16_t dynPort);
void 	locked_ConnectAttempt_AddTunnel(peerConnectState *peer);

bool  	locked_ConnectAttempt_Complete(peerConnectState *peer);

bool  	locked_CheckPotentialAddr(const struct sockaddr_in *addr, time_t age);
bool 	addAddressIfUnique(std::list<peerConnectAddress> &addrList, 
					peerConnectAddress &pca);


private:
	// These should have there own Mutex Protection,
	//p3tunnel *mP3tunnel;
	DNSResolver *mDNSResolver ;

	p3PeerMgr *mPeerMgr;
	p3NetMgr  *mNetMgr;

	RsMutex mLinkMtx; /* protects below */

        uint32_t mRetryPeriod;

	bool     mStatusChanged;

	struct sockaddr_in mLocalAddress;

	std::list<pqiMonitor *> clients;

	bool mAllowTunnelConnection;

	/* external Address determination */
	//bool mUpnpAddrValid, mStunAddrValid;
	//struct sockaddr_in mUpnpExtAddr;

	//peerConnectState mOwnState;

	std::map<std::string, peerConnectState> mFriendList;
	std::map<std::string, peerConnectState> mOthersList;

	std::list<RsPeerGroupItem *> groupList;
	uint32_t lastGroupId;

	/* relatively static list of banned ip addresses */
	std::list<struct in_addr> mBannedIpList;
};

#endif // MRK_PQI_LINK_MANAGER_HEADER
