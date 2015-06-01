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

#include "pqi/pqiassist.h"

#include "pqi/p3cfgmgr.h"

#include "util/rsthreads.h"

class ExtAddrFinder ;
class DNSResolver ;


/* order of attempts ... */
const uint32_t RS_NET_CONN_TCP_ALL             = 0x000f;
const uint32_t RS_NET_CONN_UDP_ALL             = 0x00f0;
 
const uint32_t RS_NET_CONN_TCP_LOCAL           = 0x0001;
const uint32_t RS_NET_CONN_TCP_EXTERNAL        = 0x0002;
const uint32_t RS_NET_CONN_TCP_UNKNOW_TOPOLOGY = 0x0004;
const uint32_t RS_NET_CONN_TCP_HIDDEN 	       = 0x0008;

const uint32_t RS_NET_CONN_UDP_DHT_SYNC        = 0x0010;
const uint32_t RS_NET_CONN_UDP_PEER_SYNC       = 0x0020; /* coming soon */

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
	time_t		ts;
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
	time_t ts;
	
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
	time_t lastavailable;
	time_t lastattempt;

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
        time_t deniedTS;
	bool deniedInConnAttempt; /* is below valid */
	peerConnectAddress deniedConnectionAttempt;

};

class p3tunnel; 
class RsPeerGroupItem;
class RsGroupInfo;

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

virtual void 	notifyDeniedConnection(const RsPgpId& gpgid,const RsPeerId& sslid,const std::string& sslcn,const struct sockaddr_storage &addr, bool incoming) = 0;

	/* Network Addresses */
virtual bool 	setLocalAddress(const struct sockaddr_storage &addr) = 0;
virtual bool 	getLocalAddress(struct sockaddr_storage &addr) = 0;

	/************* DEPRECIATED FUNCTIONS (TO REMOVE) ********/

virtual void	getFriendList(std::list<RsPeerId> &ssl_peers) = 0; // ONLY used by p3peers.cc USE p3PeerMgr instead.
virtual bool	getFriendNetStatus(const RsPeerId &id, peerConnectState &state) = 0; // ONLY used by p3peers.cc

virtual bool 	checkPotentialAddr(const struct sockaddr_storage &addr, time_t age)=0;

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

virtual void 	notifyDeniedConnection(const RsPgpId& gpgid,const RsPeerId& sslid,const std::string& sslcn,const struct sockaddr_storage &addr, bool incoming);

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

void 	tick();

	/* THIS COULD BE ADDED TO INTERFACE */
void    setFriendVisibility(const RsPeerId &id, bool isVisible);

    void disconnectFriend(const RsPeerId& id) ;

	/* add/remove friends */
virtual int 	addFriend(const RsPeerId &ssl_id, bool isVisible);
int 	removeFriend(const RsPeerId &ssl_id);

void 	printPeerLists(std::ostream &out);

virtual bool checkPotentialAddr(const struct sockaddr_storage &addr, time_t age);
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
void  	locked_ConnectAttempt_ProxyAddress(peerConnectState *peer, const struct sockaddr_storage &proxy_addr, const std::string &domain_addr, uint16_t domain_port);

bool  	locked_ConnectAttempt_Complete(peerConnectState *peer);

bool  	locked_CheckPotentialAddr(const struct sockaddr_storage &addr, time_t age);
bool 	addAddressIfUnique(std::list<peerConnectAddress> &addrList, peerConnectAddress &pca, bool pushFront);


private:
	// These should have there own Mutex Protection,
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

	std::list<RsPeerGroupItem *> groupList;
	uint32_t lastGroupId;

	/* relatively static list of banned ip addresses */
	std::list<struct sockaddr_storage> mBannedIpList;
};

#endif // MRK_PQI_LINK_MANAGER_HEADER
