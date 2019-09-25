/*******************************************************************************
 * libretroshare/src/pqi: p3linkmgr.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2011 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef MRK_PQI_LINK_MANAGER_HEADER
#define MRK_PQI_LINK_MANAGER_HEADER

#include "pqi/pqimonitor.h"
#include "pqi/pqiipset.h"
#include "util/rstime.h"
#include "pqi/pqiassist.h"

#include "pqi/p3cfgmgr.h"

#include "util/rsthreads.h"

class ExtAddrFinder ;
class DNSResolver ;


/* order of attempts ... */
const uint32_t RS_NET_CONN_TCP_ALL             = 0x00ff;
const uint32_t RS_NET_CONN_UDP_ALL             = 0x0f00;
 
const uint32_t RS_NET_CONN_TCP_LOCAL           = 0x0001;
const uint32_t RS_NET_CONN_TCP_EXTERNAL        = 0x0002;
const uint32_t RS_NET_CONN_TCP_UNKNOW_TOPOLOGY = 0x0004;
const uint32_t RS_NET_CONN_TCP_HIDDEN_TOR      = 0x0008;
const uint32_t RS_NET_CONN_TCP_HIDDEN_I2P      = 0x0010;

const uint32_t RS_NET_CONN_UDP_DHT_SYNC        = 0x0100;
const uint32_t RS_NET_CONN_UDP_PEER_SYNC       = 0x0200; /* coming soon */

// These are set in pqipersongroup.
const uint32_t RS_TCP_STD_TIMEOUT_PERIOD	= 5; /* 5 seconds! */
const uint32_t RS_TCP_HIDDEN_TIMEOUT_PERIOD	= 30; /* 30 seconds! */
const uint32_t RS_UDP_STD_TIMEOUT_PERIOD	= 80; /* 80 secs, allows UDP TTL to get to 40! - Plenty of time (30+80) = 110 secs */

class peerAddrInfo
{
	public:
	peerAddrInfo(); /* init */

	bool 		found;
	uint32_t 	type;
	pqiIpAddrSet	addrs;
	rstime_t		ts;
};

class peerConnectAddress
{
	public:
	peerConnectAddress(); /* init */

	struct sockaddr_storage addr;
	uint32_t delay;  /* to stop simultaneous connects */
	uint32_t period; /* UDP only */
	uint32_t type;
	uint32_t flags;  /* CB FLAGS defined in pqimonitor.h */
	rstime_t ts;
	
	// Extra Parameters for Relay connections.
	struct sockaddr_storage proxyaddr; 
	struct sockaddr_storage srcaddr;
	uint32_t bandwidth;

	// Extra Parameters for Proxy/Hidden connection.
	std::string domain_addr;
	uint16_t    domain_port;
};

class peerConnectState
{
	public:
	peerConnectState(); /* init */

	RsPeerId id;

	/***** Below here not stored permanently *****/

	bool dhtVisible;

	uint32_t connecttype;  // RS_NET_CONN_TCP_ALL / RS_NET_CONN_UDP_ALL
	bool actAsServer;
	rstime_t lastavailable;
	rstime_t lastattempt;

	std::string name;

	uint32_t    state;
	uint32_t    actions;
	uint32_t    linkType;

	uint32_t		source; /* most current source */
	peerAddrInfo	dht;
	peerAddrInfo	disc;
	peerAddrInfo	peer;

	struct sockaddr_storage connectaddr; // current connection address. Can be local or external.

	/* a list of connect attempts to make (in order) */
	bool inConnAttempt;
	peerConnectAddress currentConnAddrAttempt;
	std::list<peerConnectAddress> connAddrs;

	/* information about denial */
	bool wasDeniedConnection;
        rstime_t deniedTS;
	bool deniedInConnAttempt; /* is below valid */
	peerConnectAddress deniedConnectionAttempt;
};

class p3tunnel; 
class RsPeerGroupItem_deprecated;
struct RsGroupInfo;

class p3PeerMgr;
class p3NetMgr;

class p3PeerMgrIMPL;
class p3NetMgrIMPL;

std::string textPeerConnectState(peerConnectState &state);

/*******
 * Virtual Interface to allow testing
 *
 */

class p3LinkMgr: public pqiConnectCb
{
	public:

        p3LinkMgr() { return; }
virtual ~p3LinkMgr() { return; }


virtual const 	RsPeerId& getOwnId() = 0;
virtual bool  	isOnline(const RsPeerId &ssl_id) = 0;
virtual void  	getOnlineList(std::list<RsPeerId> &ssl_peers) = 0;
virtual bool  	getPeerName(const RsPeerId &ssl_id, std::string &name) = 0;
virtual uint32_t getLinkType(const RsPeerId &ssl_id) = 0;

	/**************** handle monitors *****************/
virtual void	addMonitor(pqiMonitor *mon) = 0;
virtual void	removeMonitor(pqiMonitor *mon) = 0;

	/****************** Connections *******************/
virtual bool	connectAttempt(const RsPeerId &id, struct sockaddr_storage &raddr,
					struct sockaddr_storage &proxyaddr, struct sockaddr_storage &srcaddr,
					uint32_t &delay, uint32_t &period, uint32_t &type, uint32_t &flags, uint32_t &bandwidth,
					std::string &domain_addr, uint16_t &domain_port) = 0;
	
virtual bool 	connectResult(const RsPeerId &id, bool success, bool isIncomingConnection, uint32_t flags, const struct sockaddr_storage &remote_peer_address) = 0;
virtual bool	retryConnect(const RsPeerId &id) = 0;

	/* Network Addresses */
virtual bool 	setLocalAddress(const struct sockaddr_storage &addr) = 0;
virtual bool 	getLocalAddress(struct sockaddr_storage &addr) = 0;

	/************* DEPRECIATED FUNCTIONS (TO REMOVE) ********/

virtual void	getFriendList(std::list<RsPeerId> &ssl_peers) = 0; // ONLY used by p3peers.cc USE p3PeerMgr instead.
virtual bool	getFriendNetStatus(const RsPeerId &id, peerConnectState &state) = 0; // ONLY used by p3peers.cc

	virtual bool checkPotentialAddr(const sockaddr_storage& addr) = 0;

	/************* DEPRECIATED FUNCTIONS (TO REMOVE) ********/
virtual int 	addFriend(const RsPeerId &ssl_id, bool isVisible) = 0;
	/******* overloaded from pqiConnectCb *************/
// THESE MUSTn't BE specfied HERE - as overloaded from pqiConnectCb.
//virtual void    peerStatus(std::string id, const pqiIpAddrSet &addrs, 
//                        uint32_t type, uint32_t flags, uint32_t source) = 0;
//virtual void    peerConnectRequest(std::string id, const struct sockaddr_storage &raddr,
//                        const struct sockaddr_storage &proxyaddr,  const struct sockaddr_storage &srcaddr,
//                        uint32_t source, uint32_t flags, uint32_t delay, uint32_t bandwidth) = 0;

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

};



class p3LinkMgrIMPL: public p3LinkMgr
{
	public:

/************************************************************************************************/
/* EXTERNAL INTERFACE */
/************************************************************************************************/

virtual const 	RsPeerId& getOwnId();
virtual bool  	isOnline(const RsPeerId &ssl_id);
virtual void  	getOnlineList(std::list<RsPeerId> &ssl_peers);
virtual bool  	getPeerName(const RsPeerId &ssl_id, std::string &name);
virtual uint32_t getLinkType(const RsPeerId &ssl_id);


	/**************** handle monitors *****************/
virtual void	addMonitor(pqiMonitor *mon);
virtual void	removeMonitor(pqiMonitor *mon);

	/****************** Connections *******************/
virtual bool	connectAttempt(const RsPeerId &id, struct sockaddr_storage &raddr,
					struct sockaddr_storage &proxyaddr, struct sockaddr_storage &srcaddr,
					uint32_t &delay, uint32_t &period, uint32_t &type, uint32_t &flags, uint32_t &bandwidth, 
					std::string &domain_addr, uint16_t &domain_port);
	
virtual bool 	connectResult(const RsPeerId &id, bool success, bool isIncomingConnection, uint32_t flags, const struct sockaddr_storage &remote_peer_address);
virtual bool	retryConnect(const RsPeerId &id);

	/* Network Addresses */
virtual bool 	setLocalAddress(const struct sockaddr_storage &addr);
virtual bool 	getLocalAddress(struct sockaddr_storage &addr);

	/******* overloaded from pqiConnectCb *************/
virtual void    peerStatus(const RsPeerId& id, const pqiIpAddrSet &addrs, 
                        uint32_t type, uint32_t flags, uint32_t source);
virtual void    peerConnectRequest(const RsPeerId& id, const struct sockaddr_storage &raddr,
                        const struct sockaddr_storage &proxyaddr,  const struct sockaddr_storage &srcaddr, 
                        uint32_t source, uint32_t flags, uint32_t delay, uint32_t bandwidth);


	/************* DEPRECIATED FUNCTIONS (TO REMOVE) ********/

virtual void	getFriendList(std::list<RsPeerId> &ssl_peers); // ONLY used by p3peers.cc USE p3PeerMgr instead.
virtual bool	getFriendNetStatus(const RsPeerId &id, peerConnectState &state); // ONLY used by p3peers.cc

/************************************************************************************************/
/* Extra IMPL Functions (used by p3PeerMgr, p3NetMgr + Setup) */
/************************************************************************************************/

        p3LinkMgrIMPL(p3PeerMgrIMPL *peerMgr, p3NetMgrIMPL *netMgr);
        virtual ~p3LinkMgrIMPL();

void 	tick();

	/* THIS COULD BE ADDED TO INTERFACE */
void    setFriendVisibility(const RsPeerId &id, bool isVisible);

    void disconnectFriend(const RsPeerId& id) ;

	/* add/remove friends */
virtual int 	addFriend(const RsPeerId &ssl_id, bool isVisible);
int 	removeFriend(const RsPeerId &ssl_id);

void 	printPeerLists(std::ostream &out);

    virtual bool checkPotentialAddr(const sockaddr_storage& addr);

protected:
	/* THESE CAN PROBABLY BE REMOVED */
//bool	shutdown(); /* blocking shutdown call */
//bool	getOwnNetStatus(peerConnectState &state);


protected:
	/****************** Internal Interface *******************/

	/* Internal Functions */
void 	statusTick();

	/* monitor control */
void 	tickMonitors();

	/* connect attempts UDP */
bool   tryConnectUDP(const RsPeerId &id, const struct sockaddr_storage &rUdpAddr, 
							const struct sockaddr_storage &proxyaddr, const struct sockaddr_storage &srcaddr,
							uint32_t flags, uint32_t delay, uint32_t bandwidth);

	/* connect attempts TCP */
bool	retryConnectTCP(const RsPeerId &id);

void 	locked_ConnectAttempt_SpecificAddress(peerConnectState *peer, const struct sockaddr_storage &remoteAddr);
void 	locked_ConnectAttempt_CurrentAddresses(peerConnectState *peer, const struct sockaddr_storage &localAddr, const struct sockaddr_storage &serverAddr);
void 	locked_ConnectAttempt_HistoricalAddresses(peerConnectState *peer, const pqiIpAddrSet &ipAddrs);
void 	locked_ConnectAttempt_AddDynDNS(peerConnectState *peer, std::string dyndns, uint16_t dynPort);
void 	locked_ConnectAttempt_AddTunnel(peerConnectState *peer);
void  	locked_ConnectAttempt_ProxyAddress(peerConnectState *peer, const uint32_t type, const struct sockaddr_storage &proxy_addr, const std::string &domain_addr, uint16_t domain_port);

bool  	locked_ConnectAttempt_Complete(peerConnectState *peer);

    bool locked_CheckPotentialAddr(const sockaddr_storage& addr);

bool 	addAddressIfUnique(std::list<peerConnectAddress> &addrList, peerConnectAddress &pca, bool pushFront);


private:
	// These should have their own Mutex Protection,
	//p3tunnel *mP3tunnel;
	DNSResolver *mDNSResolver ;

	p3PeerMgrIMPL *mPeerMgr;
	p3NetMgrIMPL  *mNetMgr;

	RsMutex mLinkMtx; /* protects below */

        uint32_t mRetryPeriod;

	bool     mStatusChanged;

	struct sockaddr_storage mLocalAddress;

	std::list<pqiMonitor *> clients;

	bool mAllowTunnelConnection;

	/* external Address determination */
	//bool mUpnpAddrValid, mStunAddrValid;
	//struct sockaddr_in mUpnpExtAddr;

	//peerConnectState mOwnState;

	std::map<RsPeerId, peerConnectState> mFriendList;
	std::map<RsPeerId, peerConnectState> mOthersList;

	/* relatively static list of banned ip addresses */
	std::list<struct sockaddr_storage> mBannedIpList;
};

#endif // MRK_PQI_LINK_MANAGER_HEADER
