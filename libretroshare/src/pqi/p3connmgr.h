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
#include "pqi/p3dhtmgr.h"
#include "pqi/p3upnpmgr.h"
#include "pqi/p3authmgr.h"

#include "pqi/p3cfgmgr.h"

#include "util/rsthreads.h"

const uint32_t RS_VIS_STATE_NODISC = 0x0001;
const uint32_t RS_VIS_STATE_NODHT  = 0x0002;

const uint32_t RS_VIS_STATE_STD    = 0x0000;
const uint32_t RS_VIS_STATE_GRAY   = RS_VIS_STATE_NODHT;
const uint32_t RS_VIS_STATE_DARK   = RS_VIS_STATE_NODISC | RS_VIS_STATE_NODHT;
const uint32_t RS_VIS_STATE_BROWN  = RS_VIS_STATE_NODISC;

const uint32_t RS_NET_MODE_UNKNOWN =    0x0000;
const uint32_t RS_NET_MODE_EXT =        0x0001;
const uint32_t RS_NET_MODE_UPNP =       0x0002;
const uint32_t RS_NET_MODE_UDP =        0x0003;
const uint32_t RS_NET_MODE_ERROR =      0x0004;


/* order of attempts ... */
const uint32_t RS_NET_CONN_TCP_ALL 		= 0x000f;
const uint32_t RS_NET_CONN_UDP_ALL 		= 0x00f0;

const uint32_t RS_NET_CONN_TCP_LOCAL 		= 0x0001;
const uint32_t RS_NET_CONN_TCP_EXTERNAL 	= 0x0002;
const uint32_t RS_NET_CONN_UDP_DHT_SYNC 	= 0x0010;


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
	uint32_t type;
	time_t ts;
};

class peerConnectState
{
	public:
	peerConnectState(); /* init */

	std::string id;
	std::string name;

	uint32_t    state;
	uint32_t    actions;

	uint32_t netMode; /* EXT / UPNP / UDP / INVALID */

	/* Fix this up! */

        // public for the moment.
	struct sockaddr_in lastaddr, localaddr, serveraddr;

	/* determines how public this peer wants to be...
	 *
	 * STD = advertise to Peers / DHT checking etc 
	 * GRAY = share with friends / but not DHT 
	 * DARK = hidden from all 
	 * BROWN? = hidden from friends / but on DHT
	 */

	uint32_t visState; /* STD, GRAY, DARK */	

	uint32_t		source; /* most current source */
	peerAddrInfo		dht;
	peerAddrInfo		disc;
	peerAddrInfo		peer;

	/* a list of connect attempts to make (in order) */
	bool inConnAttempt;
	time_t connAttemptTS;
	peerConnectAddress currentConnAddr;
	std::list<peerConnectAddress> connAddrs;


	/* stuff here un-used at the moment */


        time_t lc_timestamp; // last connect timestamp
        time_t lr_timestamp; // last receive timestamp

	time_t nc_timestamp; // next connect timestamp.
	time_t nc_timeintvl; // next connect time interval.
};


class p3ConnectMgr: public pqiConnectCb, public p3Config
{
	public:

	p3ConnectMgr(p3AuthMgr *authMgr); 

void 	tick();

	/*************** Setup ***************************/
void	setDhtMgr(p3DhtMgr *dmgr) 	{ mDhtMgr = dmgr; }
void	setUpnpMgr(p3UpnpMgr *umgr)	{ mUpnpMgr = umgr; }
bool	checkNetAddress(); /* check our address is sensible */

	/*************** External Control ****************/
bool	retryConnect(std::string id);

bool    getUPnPState();
bool	getUPnPEnabled();
bool	getDHTEnabled();

bool 	setLocalAddress(std::string id, struct sockaddr_in addr);
bool 	setExtAddress(std::string id, struct sockaddr_in addr);
bool 	setNetworkMode(std::string id, uint32_t netMode);

	/* add/remove friends */
bool	addFriend(std::string);
bool	removeFriend(std::string);
bool	addNeighbour(std::string);

	/*************** External Control ****************/

	/* access to network details (called through Monitor) */
const std::string getOwnId();
bool	getOwnNetStatus(peerConnectState &state);

bool	isFriend(std::string id);
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
                        uint32_t type, uint32_t mode, uint32_t source);
virtual void    peerConnectRequest(std::string id, uint32_t type);
virtual void    stunStatus(std::string id, struct sockaddr_in addr, uint32_t flags);

	/****************** Connections *******************/
bool 	connectAttempt(std::string id, struct sockaddr_in &addr, uint32_t &type);
bool 	connectResult(std::string id, bool success, uint32_t flags);


protected:

	/* Internal Functions */
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

void 	netUdpCheck();

	/* Udp / Stun functions */
bool 	udpInternalAddress(struct sockaddr_in iaddr);
bool 	udpExtAddressCheck();
void 	udpStunPeer(std::string id, struct sockaddr_in &addr);

void 	stunInit();
bool 	stunCheck();
void 	stunCollect(std::string id, struct sockaddr_in addr, uint32_t flags);

	/* monitor control */
void 	tickMonitors();


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
	p3DhtMgr *mDhtMgr;
	p3UpnpMgr *mUpnpMgr;

	RsMutex connMtx; /* protects below */

	time_t   mNetInitTS;
	uint32_t mNetStatus;
	uint32_t mStunStatus;
	bool     mStatusChanged;

	std::list<pqiMonitor *> clients;


	/* external Address determination */
	bool mUpnpAddrValid, mStunAddrValid;
	struct sockaddr_in mUpnpExtAddr;
	struct sockaddr_in mStunExtAddr;


	/* these are protected for testing */
protected:

void addPeer(std::string id, std::string name); /* tmp fn */

	peerConnectState ownState;

	std::list<std::string> mStunList;
	std::map<std::string, peerConnectState> mFriendList;
	std::map<std::string, peerConnectState> mOthersList;
};

#endif // MRK_PQI_CONNECTION_MANAGER_HEADER
