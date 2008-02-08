/*
 * libretroshare/src/pqi: p3connmgr.cc
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

#include "pqi/p3connmgr.h"
#include "tcponudp/tou.h"
#include "util/rsprint.h"

#include "serialiser/rsconfigitems.h"

/* Network setup States */

const uint32_t RS_NET_UNKNOWN = 	0x0001;
const uint32_t RS_NET_UPNP_INIT = 	0x0002;
const uint32_t RS_NET_UPNP_SETUP =  	0x0003;
const uint32_t RS_NET_UDP_SETUP =   	0x0004;
const uint32_t RS_NET_DONE =    	0x0005;


/* Stun modes (TODO) */
const uint32_t RS_STUN_DHT =      	0x0001;
const uint32_t RS_STUN_DONE =      	0x0002;
const uint32_t RS_STUN_LIST_MIN =      	100;

const uint32_t MAX_UPNP_INIT = 		10; /* seconds UPnP timeout */

#define CONN_DEBUG 1


void  printConnectState(peerConnectState &peer);

peerConnectAddress::peerConnectAddress()
	:type(0), ts(0)
{
	sockaddr_clear(&addr);
}


peerAddrInfo::peerAddrInfo()
	:found(false), type(0), ts(0)
{
	sockaddr_clear(&laddr);
	sockaddr_clear(&raddr);
}

peerConnectState::peerConnectState()
	:id("unknown"), 
	 netMode(RS_NET_MODE_UNKNOWN), visState(RS_VIS_STATE_STD), 
	 lastcontact(0),

	 name("nameless"), state(0), actions(0), 
	 source(0), 
	 inConnAttempt(0), connAttemptTS(0)
{
	sockaddr_clear(&localaddr);
	sockaddr_clear(&serveraddr);

	return;
}


p3ConnectMgr::p3ConnectMgr(p3AuthMgr *am)
	:p3Config(CONFIG_TYPE_PEERS), 
	mAuthMgr(am), mDhtMgr(NULL), mUpnpMgr(NULL), mNetStatus(RS_NET_UNKNOWN), 
	mStunStatus(0), mStatusChanged(false)
{
	mUpnpAddrValid = false;
	mStunAddrValid = false;

	/* setup basics of own state */
	if (am)
	{
		ownState.id = mAuthMgr->OwnId();
		ownState.name = mAuthMgr->getName(ownState.id);
		ownState.netMode = RS_NET_MODE_UDP;
	}

	return;
}


void p3ConnectMgr::setOwnNetConfig(uint32_t netMode, uint32_t visState)
{
	ownState.netMode = netMode;
	ownState.visState = visState;

	/* if we've started up - then tweak Dht On/Off */
	if (mNetStatus != RS_NET_UNKNOWN)
	{
		mDhtMgr->setDhtOn(!(ownState.visState & RS_VIS_STATE_NODHT));
	}

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
}


/***** Framework / initial implementation for a connection manager.
 *
 * This needs a state machine for Initialisation.
 *
 * Network state:
 *   RS_NET_UNKNOWN
 *   RS_NET_EXT_UNKNOWN * forwarded port (but Unknown Ext IP) *
 *   RS_NET_EXT_KNOWN   * forwarded port with known IP/Port. *
 *
 *   RS_NET_UPNP_CHECK  * checking for UPnP *
 *   RS_NET_UPNP_KNOWN  * confirmed UPnP ext Ip/port *
 *
 *   RS_NET_UDP_UNKNOWN * not Ext/UPnP - to determine Ext IP/Port *
 *   RS_NET_UDP_KNOWN   * have Stunned for Ext Addr *
 *
 *  Transitions:
 *
 *  RS_NET_UNKNOWN -(config)-> RS_NET_EXT_UNKNOWN 
 *  RS_NET_UNKNOWN -(config)-> RS_NET_UPNP_UNKNOWN  
 *  RS_NET_UNKNOWN -(config)-> RS_NET_UDP_UNKNOWN
 *              
 *  RS_NET_EXT_UNKNOWN -(DHT(ip)/Stun)-> RS_NET_EXT_KNOWN
 *
 *  RS_NET_UPNP_UNKNOWN -(Upnp)-> RS_NET_UPNP_KNOWN
 *  RS_NET_UPNP_UNKNOWN -(timout/Upnp)-> RS_NET_UDP_UNKNOWN
 *
 *  RS_NET_UDP_UNKNOWN -(stun)-> RS_NET_UDP_KNOWN
 *
 *
 * STUN state:
 * 	RS_STUN_INIT * done nothing *
 * 	RS_STUN_DHT  * looking up peers *
 * 	RS_STUN_DONE * found active peer and stunned *
 *
 *
 * Steps.
 *******************************************************************
 * (1) Startup.
 * 	- UDP port setup.
 * 	- DHT setup.
 * 	- Get Stun Keys -> add to DHT.
 *	- Feedback from DHT -> ask UDP to stun.
 *
 * (1) determine Network mode.
 *	If external Port.... Done: 
 * (2) 
 *******************************************************************
 * Stable operation:
 * (1) tick and check peers.
 * (2) handle callback.
 * (3) notify of new/failed connections.
 *
 *
 */

void p3ConnectMgr::netStartup()
{
	/* startup stuff */

	/* StunInit gets a list of peers, and asks the DHT to find them...
	 * This is needed for all systems so startup straight away 
	 */

	loadConfiguration();
	netDhtInit();
	netUdpInit();
	netStunInit();

	/* decide which net setup mode we're going into 
	 */

	connMtx.lock();   /*   LOCK MUTEX */

	mNetInitTS = time(NULL);

	switch(ownState.netMode)
	{
		case RS_NET_MODE_UPNP:
			mNetStatus = RS_NET_UPNP_INIT;
			break;

		case RS_NET_MODE_EXT:  /* v similar to UDP */
		case RS_NET_MODE_UDP:
		default:
			mNetStatus = RS_NET_UDP_SETUP;
			break;

	}
	connMtx.unlock(); /* UNLOCK MUTEX */
}


void p3ConnectMgr::tick()
{
	netTick();
	tickMonitors();

}

void p3ConnectMgr::netTick()
{

#ifdef CONN_DEBUG
	//std::cerr << "p3ConnectMgr::netTick()" << std::endl;
#endif

	connMtx.lock();   /*   LOCK MUTEX */

	uint32_t netStatus = mNetStatus;

	connMtx.unlock(); /* UNLOCK MUTEX */

	switch(netStatus)
	{
		case RS_NET_UNKNOWN:

#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::netTick() STATUS: UNKNOWN" << std::endl;
#endif
 			/*  RS_NET_UNKNOWN -(config)-> RS_NET_EXT_INIT
 			 *  RS_NET_UNKNOWN -(config)-> RS_NET_UPNP_INIT
 			 *  RS_NET_UNKNOWN -(config)-> RS_NET_UDP_INIT
			 */
			netStartup();
			break;

		case RS_NET_UPNP_INIT:
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::netTick() STATUS: UPNP_INIT" << std::endl;
#endif
			netUpnpInit();
			break;

		case RS_NET_UPNP_SETUP:
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::netTick() STATUS: UPNP_SETUP" << std::endl;
#endif
			netUpnpCheck();
			break;

		case RS_NET_UDP_SETUP:
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::netTick() STATUS: UDP_SETUP" << std::endl;
#endif
			netUdpCheck();
			break;

		case RS_NET_DONE:
#ifdef CONN_DEBUG
			//std::cerr << "p3ConnectMgr::netTick() STATUS: DONE" << std::endl;
#endif
		default:
			break;
	}

	return;
}


void p3ConnectMgr::netUdpInit()
{
	connMtx.lock();   /*   LOCK MUTEX */

	struct sockaddr_in iaddr = ownState.localaddr;

	connMtx.unlock(); /* UNLOCK MUTEX */

	/* open our udp port */
	tou_init((struct sockaddr *) &iaddr, sizeof(iaddr));
}


void p3ConnectMgr::netDhtInit()
{
	connMtx.lock();   /*   LOCK MUTEX */

	uint32_t vs = ownState.visState;

	connMtx.unlock(); /* UNLOCK MUTEX */

	mDhtMgr->setDhtOn(!(vs & RS_VIS_STATE_NODHT));
}


void p3ConnectMgr::netUpnpInit()
{
	uint16_t eport, iport;

	connMtx.lock();   /*   LOCK MUTEX */

	/* get the ports from the configuration */

	mNetStatus = RS_NET_UPNP_SETUP;

	connMtx.unlock(); /* UNLOCK MUTEX */

	mUpnpMgr->setInternalPort(iport);
	mUpnpMgr->setExternalPort(eport);

	mUpnpMgr->enableUPnP(true);
}

void p3ConnectMgr::netUpnpCheck()
{
	/* grab timestamp */
	connMtx.lock();   /*   LOCK MUTEX */

	time_t delta = time(NULL) - mNetInitTS;

	connMtx.unlock(); /* UNLOCK MUTEX */

	struct sockaddr_in extAddr;
	int upnpState = mUpnpMgr->getUPnPActive();

	if ((upnpState < 0) ||
	   ((upnpState == 0) && (delta > MAX_UPNP_INIT)))
	{
		/* fallback to UDP startup */
		connMtx.lock();   /*   LOCK MUTEX */

		mUpnpAddrValid = false;
		mNetStatus = RS_NET_UDP_SETUP;
		ownState.netMode = RS_NET_MODE_UDP; /* UPnP Failed us! */

		connMtx.unlock(); /* UNLOCK MUTEX */
	}
	else if ((upnpState > 0) &&
		mUpnpMgr->getExternalAddress(extAddr))
	{
		/* switch to UDP startup */
		connMtx.lock();   /*   LOCK MUTEX */

		mUpnpAddrValid = true;
		mUpnpExtAddr = extAddr;
		mNetStatus = RS_NET_UDP_SETUP;

		connMtx.unlock(); /* UNLOCK MUTEX */
	}
}

void p3ConnectMgr::netUdpCheck()
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netUdpCheck()" << std::endl;
#endif
	if (stunCheck())
	{
		bool extValid = false;
		struct sockaddr_in extAddr;
		uint32_t mode = 0;



		connMtx.lock();   /*   LOCK MUTEX */

		mNetStatus = RS_NET_DONE;

		/* get the addr from the configuration */
		struct sockaddr_in iaddr = ownState.localaddr;

		if (mUpnpAddrValid)
		{
			extValid = true;
			extAddr = mUpnpExtAddr;
		}
		else if (mStunAddrValid)
		{
			extValid = true;
			extAddr = mStunExtAddr;
		}

		if (extValid)
		{
			ownState.serveraddr = extAddr;
			mode = RS_NET_CONN_TCP_LOCAL | RS_NET_CONN_UDP_DHT_SYNC;
			if ((ownState.netMode == RS_NET_MODE_UPNP) || 
				(ownState.netMode == RS_NET_MODE_EXT))
			{
				mode |= RS_NET_CONN_TCP_EXTERNAL;
			}
			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}

		connMtx.unlock(); /* UNLOCK MUTEX */

		if (extValid)
		{
			mDhtMgr->setExternalInterface(iaddr, extAddr, mode);
		}
		else
		{
			mDhtMgr->setExternalInterface(iaddr, extAddr, RS_NET_MODE_ERROR);
		}
	}
}


/*******************************  UDP MAINTAINANCE  ********************************
 * Interaction with the UDP is mainly for determining the External Port.
 *
 */

bool p3ConnectMgr::udpInternalAddress(struct sockaddr_in iaddr)
{
	return false; 
}
	
bool p3ConnectMgr::udpExtAddressCheck()
{
	/* three possibilities:
	 * (1) not found yet.
	 * (2) Found!
	 * (3) bad udp (port switching).
	 */
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::udpExtAddressCheck()" << std::endl;
#endif

	if (0 < tou_extaddr((struct sockaddr *) &addr, &len))
	{
		/* update UDP information */
		connMtx.lock();   /*   LOCK MUTEX */

		mStunExtAddr = addr;
		mStunAddrValid = true;


#ifdef CONN_DEBUG
        	std::cerr << "p3ConnectMgr::udpExtAddressCheck() Got ";
	        std::cerr << " addr: " << inet_ntoa(mStunExtAddr.sin_addr);
		std::cerr << " port: " << ntohs(mStunExtAddr.sin_port);
		std::cerr << std::endl;
#endif


		connMtx.unlock(); /* UNLOCK MUTEX */

		return true;
	}
	return false; 
}
	
void p3ConnectMgr::udpStunPeer(std::string id, struct sockaddr_in &addr)
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::udpStunPeer()" << std::endl;
#endif
	/* add it into udp stun list */
	tou_stunpeer((struct sockaddr *) &addr, sizeof(addr), id.c_str());
}

/********************************** STUN SERVERS ***********************************
 * We maintain a list of stun servers. This is initialised with a set of random keys.
 *
 * This is gradually rolled over with time. We update with friends/friends of friends, 
 * and the lists that they provide (part of AutoDisc).
 *
 * max 100 entries?
 */

void p3ConnectMgr::netStunInit()
{
	stunInit();
}

void p3ConnectMgr::stunInit()
{

	connMtx.lock();   /*   LOCK MUTEX */

	/* push stun list to DHT */
	std::list<std::string>::iterator it;
	for(it = mStunList.begin(); it != mStunList.end(); it++)
	{
		mDhtMgr->addStun(*it);
	}
	mStunStatus = RS_STUN_DHT;

	connMtx.unlock(); /* UNLOCK MUTEX */
}

bool p3ConnectMgr::stunCheck()
{
	/* check if we've got a Stun result */

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::stunCheck()" << std::endl;
#endif


	if (udpExtAddressCheck())
	{
		/* set external UDP address */
		mDhtMgr->doneStun();

		connMtx.lock();   /*   LOCK MUTEX */

		mStunStatus = RS_STUN_DONE;

		connMtx.unlock(); /* UNLOCK MUTEX */

		return true;
	}
	return false;
}

void p3ConnectMgr::stunStatus(std::string id, struct sockaddr_in addr, uint32_t flags)
{
	std::cerr << "p3ConnectMgr::stunStatus()";
	std::cerr << " id: " << RsUtil::BinToHex(id) << " addr: " << inet_ntoa(addr.sin_addr);
	std::cerr << " port: " << ntohs(addr.sin_port);
	std::cerr << std::endl;

	connMtx.lock();   /*   LOCK MUTEX */

	bool stillStunning = (mStunStatus == RS_STUN_DHT);

	connMtx.unlock(); /* UNLOCK MUTEX */

	if (stillStunning)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::stunStatus() Sending to UDP" << std::endl;
#endif
		/* push to the UDP */
		udpStunPeer(id, addr);
	}

	/* push to the stunCollect */
	stunCollect(id, addr, flags);
}

/* FLAGS 

ONLINE
EXT 
UPNP
UDP
FRIEND
FRIEND_OF_FRIEND
OTHER

*/

void p3ConnectMgr::stunCollect(std::string id, struct sockaddr_in addr, uint32_t flags)
{
	/* if peer is online - move to the top */
	connMtx.lock();   /*   LOCK MUTEX */

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::stunCollect() id: " << RsUtil::BinToHex(id) << std::endl;
#endif

	std::list<std::string>::iterator it;
	it = std::find(mStunList.begin(), mStunList.end(), id);
	if (it == mStunList.end())
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::stunCollect() Id not in List" << std::endl;
#endif
		/* add it in:
		 * if FRIEND / ONLINE or if list is short.
		 */
		if ((flags & RS_STUN_ONLINE) || (flags & RS_STUN_FRIEND) 
			|| (mStunList.size() < RS_STUN_LIST_MIN))
		{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::stunCollect() Id added to Front" << std::endl;
#endif
			/* push to the front */
			mStunList.push_front(id);

			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}
	}
	else
	{
		/* if they're online ... move to the front 
		 */
		if (flags & RS_STUN_ONLINE)
		{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::stunCollect() Id moved to Front" << std::endl;
#endif
			/* move to front */
			mStunList.erase(it);
			mStunList.push_front(id);

			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}
	}

	connMtx.unlock(); /* UNLOCK MUTEX */
}

/********************************  Network Status  *********************************
 * Configuration Loading / Saving.
 */


void p3ConnectMgr::addMonitor(pqiMonitor *mon)
{
	/* 
	 */
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	std::list<pqiMonitor *>::iterator it;
	it = std::find(clients.begin(), clients.end(), mon);
	if (it != clients.end())
	{
		return;
	}

	mon->setConnectionMgr(this);
	clients.push_back(mon);
	return;
}

void p3ConnectMgr::removeMonitor(pqiMonitor *mon)
{
	/* 
	 */
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	std::list<pqiMonitor *>::iterator it;
	it = std::find(clients.begin(), clients.end(), mon);
	if (it == clients.end())
	{
		return;
	}
	(*it)->setConnectionMgr(NULL);
	clients.erase(it);

	return;
}


void p3ConnectMgr::tickMonitors()
{
	std::list<pqipeer> actionList;
        std::map<std::string, peerConnectState>::iterator it;

	if (mStatusChanged)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::tickMonitors() StatusChanged! List:" << std::endl;
#endif
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
		/* assemble list */
		for(it = mFriendList.begin(); it != mFriendList.end(); it++)
		{
			if (it->second.actions)
			{
				/* add in */
				pqipeer peer;
				peer.id = it->second.id;
				peer.name = it->second.name;
				peer.state = it->second.state;
				peer.actions = it->second.actions;

				/* reset action */
				it->second.actions = 0;

				actionList.push_back(peer);

#ifdef CONN_DEBUG
				std::cerr << "Friend: " << peer.name;
				std::cerr << " Id: " << peer.id;
				std::cerr << " State: " << peer.state;
				if (peer.state & RS_PEER_S_FRIEND)
					std::cerr << " S:RS_PEER_S_FRIEND";
				if (peer.state & RS_PEER_S_ONLINE)
					std::cerr << " S:RS_PEER_S_ONLINE";
				if (peer.state & RS_PEER_S_CONNECTED)
					std::cerr << " S:RS_PEER_S_CONNECTED";
				std::cerr << " Actions: " << peer.actions;
				if (peer.actions & RS_PEER_NEW)
					std::cerr << " A:RS_PEER_NEW";
				if (peer.actions & RS_PEER_MOVED)
					std::cerr << " A:RS_PEER_MOVED";
				if (peer.actions & RS_PEER_CONNECTED)
					std::cerr << " A:RS_PEER_CONNECTED";
				if (peer.actions & RS_PEER_DISCONNECTED)
					std::cerr << " A:RS_PEER_DISCONNECTED";
				if (peer.actions & RS_PEER_CONNECT_REQ)
					std::cerr << " A:RS_PEER_CONNECT_REQ";

				std::cerr << std::endl;
#endif
			}
		}
		/* do the Others as well! */
		for(it = mOthersList.begin(); it != mOthersList.end(); it++)
		{
			if (it->second.actions)
			{
				/* add in */
				pqipeer peer;
				peer.id = it->second.id;
				peer.name = it->second.name;
				peer.state = it->second.state;
				peer.actions = it->second.actions;

				/* reset action */
				it->second.actions = 0;

#ifdef CONN_DEBUG
				std::cerr << "Other: " << peer.name;
				std::cerr << " Id: " << peer.id;
				std::cerr << " State: " << peer.state;
				if (peer.state & RS_PEER_S_FRIEND)
					std::cerr << " S:RS_PEER_S_FRIEND";
				if (peer.state & RS_PEER_S_ONLINE)
					std::cerr << " S:RS_PEER_S_ONLINE";
				if (peer.state & RS_PEER_S_CONNECTED)
					std::cerr << " S:RS_PEER_S_CONNECTED";
				std::cerr << " Actions: " << peer.actions;
				if (peer.actions & RS_PEER_NEW)
					std::cerr << " A:RS_PEER_NEW";
				if (peer.actions & RS_PEER_MOVED)
					std::cerr << " A:RS_PEER_MOVED";
				if (peer.actions & RS_PEER_CONNECTED)
					std::cerr << " A:RS_PEER_CONNECTED";
				if (peer.actions & RS_PEER_DISCONNECTED)
					std::cerr << " A:RS_PEER_DISCONNECTED";
				if (peer.actions & RS_PEER_CONNECT_REQ)
					std::cerr << " A:RS_PEER_CONNECT_REQ";

				std::cerr << std::endl;
#endif

				actionList.push_back(peer);
			}
		}
		mStatusChanged = false;
	
#ifdef CONN_DEBUG
		std::cerr << "Sending to " << clients.size() << " monitorClients";
		std::cerr << std::endl;
#endif
	
		/* send to all monitors */
		std::list<pqiMonitor *>::iterator mit;
		for(mit = clients.begin(); mit != clients.end(); mit++)
		{
			(*mit)->statusChange(actionList);
		}
	}
}


const std::string p3ConnectMgr::getOwnId()
{
	if (mAuthMgr)
	{
		return mAuthMgr->OwnId();
	}
	else
	{
		std::string nullStr;
		return nullStr;
	}
}


bool p3ConnectMgr::getOwnNetStatus(peerConnectState &state)
{
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	state = ownState;
	return true;
}

bool p3ConnectMgr::isFriend(std::string id)
{
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	return (mFriendList.end() != mFriendList.find(id));
}

bool p3ConnectMgr::getFriendNetStatus(std::string id, peerConnectState &state)
{
	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
		return false;
	}

	state = it->second;
	return true;
}


bool p3ConnectMgr::getOthersNetStatus(std::string id, peerConnectState &state)
{
	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	it = mOthersList.find(id);
	if (it == mOthersList.end())
	{
		return false;
	}

	state = it->second;
	return true;
}


void p3ConnectMgr::getOnlineList(std::list<std::string> &peers)
{
	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		if (it->second.state & RS_PEER_S_CONNECTED)
		{
			peers.push_back(it->first);
		}
	}
	return;
}

void p3ConnectMgr::getFriendList(std::list<std::string> &peers)
{
	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		peers.push_back(it->first);
	}
	return;
}


void p3ConnectMgr::getOthersList(std::list<std::string> &peers)
{
	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	for(it = mOthersList.begin(); it != mOthersList.end(); it++)
	{
		peers.push_back(it->first);
	}
	return;
}



bool p3ConnectMgr::connectAttempt(std::string id, struct sockaddr_in &addr, uint32_t &type)
{
	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
		
		std::cerr << "p3ConnectMgr::connectAttempt() FAILED Not in FriendList!";
		std::cerr << " id: " << id;
		std::cerr << std::endl;

		return false;
	}

	if (it->second.connAddrs.size() < 1)
	{
		std::cerr << "p3ConnectMgr::connectAttempt() FAILED No ConnectAddresses";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
		return false;
	}
	
	it->second.inConnAttempt = true;
	it->second.currentConnAddr = it->second.connAddrs.front();
	it->second.connAddrs.pop_front();

	addr = it->second.currentConnAddr.addr;
	type = it->second.currentConnAddr.type;

	std::cerr << "p3ConnectMgr::connectAttempt() Success: ";
	std::cerr << " id: " << id;
	std::cerr << std::endl;
	std::cerr << " laddr: " << inet_ntoa(addr.sin_addr);
	std::cerr << " lport: " << ntohs(addr.sin_port);
	std::cerr << " type: " << type;
	std::cerr << std::endl;

	return true;
}


/****************************
 * Update state,
 * trigger retry if necessary,
 *
 * remove from DHT?
 *
 */

bool p3ConnectMgr::connectResult(std::string id, bool success, uint32_t flags)
{
	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
		return false;
	}

	std::cerr << "p3ConnectMgr::connectAttempt() Success: ";
	std::cerr << " id: " << id;
	std::cerr << std::endl;
	std::cerr << " Success: " << success;
	std::cerr << " flags: " << flags;
	std::cerr << std::endl;

	it->second.inConnAttempt = false;

	if (success)
	{
		/* remove other attempts */
		it->second.connAddrs.clear();
		mDhtMgr->dropPeer(id);

		/* update address (will come through from DISC) */

	std::cerr << "p3ConnectMgr::connectAttempt() Success: ";
	std::cerr << " id: " << id;
	std::cerr << std::endl;
	std::cerr << " Success: " << success;
	std::cerr << " flags: " << flags;
	std::cerr << std::endl;


		/* change state */
		it->second.state |= RS_PEER_S_CONNECTED;
		it->second.actions |= RS_PEER_CONNECTED;
		mStatusChanged = true;
		it->second.lastcontact = time(NULL);  /* time of connect */

		return true;
	}

	/* if currently connected -> flag as failed */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
		it->second.state &= (~RS_PEER_S_CONNECTED);
		it->second.actions |= RS_PEER_DISCONNECTED;

		it->second.lastcontact = time(NULL);  /* time of disconnect */

		mDhtMgr->findPeer(id);
		if (it->second.visState & RS_VIS_STATE_NODHT)
		{
			/* hidden from DHT world */
		}
		else
		{
			mDhtMgr->findPeer(id);
		}

	}
		

	if (it->second.connAddrs.size() < 1)
	{
		return true;
	}
	

	it->second.actions |= RS_PEER_CONNECT_REQ;
	mStatusChanged = true;

	return true;
}




/******************************** Feedback ......  *********************************
 * From various sources
 */

void    p3ConnectMgr::peerStatus(std::string id, 
			struct sockaddr_in laddr, struct sockaddr_in raddr,
                       uint32_t type, uint32_t flags, uint32_t source)
{
	std::cerr << "p3ConnectMgr::peerStatus()";
	std::cerr << " id: " << id;
	std::cerr << " laddr: " << inet_ntoa(laddr.sin_addr);
	std::cerr << " lport: " << ntohs(laddr.sin_port);
	std::cerr << " raddr: " << inet_ntoa(raddr.sin_addr);
	std::cerr << " rport: " << ntohs(raddr.sin_port);
	std::cerr << " type: " << type;
	std::cerr << " flags: " << flags;
	std::cerr << " source: " << source;
	std::cerr << std::endl;

	/* look up the id */
        std::map<std::string, peerConnectState>::iterator it;
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	bool isFriend = true;
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
		/* check Others list */
		isFriend = false;
		it = mOthersList.find(id);
		if (it == mOthersList.end())
		{
			/* not found - ignore */
			std::cerr << "p3ConnectMgr::peerStatus() Peer Not Found - Ignore";
			std::cerr << std::endl;
			return;
		}
		std::cerr << "p3ConnectMgr::peerStatus() Peer is in mOthersList";
		std::cerr << std::endl;
	}

	std::cerr << "p3ConnectMgr::peerStatus() Current Peer State:" << std::endl;
	printConnectState(it->second);
	std::cerr << std::endl;

	/* update the status */

	peerAddrInfo details;
	details.type    = type;
	details.found   = true;
	details.laddr   = laddr;
	details.raddr   = raddr;
	details.ts      = time(NULL);

	/* if source is DHT */
	if (source == RS_CB_DHT)
	{
		/* DHT can tell us about
		 * 1) connect type (UDP/TCP/etc)
		 * 2) local/external address
		 */
		it->second.source = RS_CB_DHT;
		it->second.dht = details;

		/* If we get a info -> then they are online */
		it->second.state |= RS_PEER_S_ONLINE;
	}
	else if (source == RS_CB_DISC)
	{
		/* DISC can tell us about
		 * 1) connect type (UDP/TCP/etc)
		 * 2) local/external addresses
		 */
		it->second.source = RS_CB_DISC;
		it->second.disc = details;

		if (flags & RS_NET_FLAGS_ONLINE)
		{
			it->second.actions |= RS_PEER_ONLINE;
			it->second.state |= RS_PEER_S_ONLINE;
			mStatusChanged = true;
		}

		/* not updating VIS status??? */
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}
	else if (source == RS_CB_PERSON)
	{
		/* PERSON can tell us about
		 * 1) online / offline
		 * 2) connect address
		 * -> update all!
		 */

		it->second.source = RS_CB_PERSON;
		it->second.peer = details;

		it->second.localaddr = laddr;
		it->second.serveraddr = raddr;
		it->second.state |= RS_PEER_S_ONLINE;

		/* must be online to recv info (should be connected too!) 
		 * but no need for action as should be connected already 
		 */

		if (flags & RS_NET_FLAGS_EXTERNAL_ADDR)
		{
			it->second.netMode = RS_NET_MODE_EXT;
		}
		else
		{
			it->second.netMode = RS_NET_MODE_UDP;
		}

		/* always update VIS status */
		if (flags & RS_NET_FLAGS_USE_DISC)
		{
			it->second.visState &= (~RS_VIS_STATE_NODISC);
		}
		else
		{
			it->second.visState |= RS_VIS_STATE_NODISC;
		}

		if (flags & RS_NET_FLAGS_USE_DHT)
		{
			it->second.visState &= (~RS_VIS_STATE_NODHT);
		}
		else
		{
			it->second.visState |= RS_VIS_STATE_NODHT;
		}
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}

	if (!isFriend)
	{
		std::cerr << "p3ConnectMgr::peerStatus() NOT FRIEND ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;

		return;
	}

	/* if already connected -> done */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
		std::cerr << "p3ConnectMgr::peerStatus() PEER ONLINE ALREADY ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;

		return;
	}


	/* are the addresses different? */

	std::cerr << "p3ConnectMgr::peerStatus()";
	std::cerr << " id: " << id;
	std::cerr << " laddr: " << inet_ntoa(laddr.sin_addr);
	std::cerr << " lport: " << ntohs(laddr.sin_port);
	std::cerr << " raddr: " << inet_ntoa(raddr.sin_addr);
	std::cerr << " rport: " << ntohs(raddr.sin_port);
	std::cerr << " type: " << type;
	std::cerr << " flags: " << flags;
	std::cerr << " source: " << source;
	std::cerr << std::endl;
	

	time_t now = time(NULL);
	/* add in attempts ... local(TCP), remote(TCP) 
	 * udp must come from notify 
	 */

	/* if address is same -> try local */
	if ((isValidNet(&(details.laddr.sin_addr))) &&
		(sameNet(&(ownState.localaddr.sin_addr), &(details.laddr.sin_addr))))

	{
		/* add the local address */
		peerConnectAddress pca;
		pca.ts = now;
		pca.type = RS_NET_CONN_TCP_LOCAL;
		pca.addr = details.laddr;

		std::cerr << "p3ConnectMgr::peerStatus() ADDING TCP_LOCAL ADDR: ";
		std::cerr << " id: " << id;
		std::cerr << " laddr: " << inet_ntoa(pca.addr.sin_addr);
		std::cerr << " lport: " << ntohs(pca.addr.sin_port);
		std::cerr << " type: " << pca.type;
		std::cerr << " source: " << source;
		std::cerr << std::endl;
	
		it->second.connAddrs.push_back(pca);
	}
	else
	{
		std::cerr << "p3ConnectMgr::peerStatus() Not adding Local Connect (Diff Network)";
		std::cerr << " id: " << id;
		std::cerr << " laddr: " << inet_ntoa(details.laddr.sin_addr);
		std::cerr << ": " << ntohs(details.laddr.sin_port);
		std::cerr << " own.laddr: " << inet_ntoa(ownState.localaddr.sin_addr);
		std::cerr << ": " << ntohs(ownState.localaddr.sin_port);
		std::cerr << std::endl;
	}


	if ((details.type & RS_NET_CONN_TCP_EXTERNAL) &&
	   (isValidNet(&(details.raddr.sin_addr))))

	{
		/* add the remote address */
		peerConnectAddress pca;
		pca.ts = now;
		pca.type = RS_NET_CONN_TCP_EXTERNAL;
		pca.addr = details.raddr;

		std::cerr << "p3ConnectMgr::peerStatus() ADDING TCP_REMOTE ADDR: ";
		std::cerr << " id: " << id;
		std::cerr << " laddr: " << inet_ntoa(pca.addr.sin_addr);
		std::cerr << " lport: " << ntohs(pca.addr.sin_port);
		std::cerr << " type: " << pca.type;
		std::cerr << " source: " << source;
		std::cerr << std::endl;

		it->second.connAddrs.push_back(pca);
	}
	else
	{
		std::cerr << "p3ConnectMgr::peerStatus() Not adding Remote Connect (Type != E or Invalid Network)";
		std::cerr << " id: " << id;
		std::cerr << " raddr: " << inet_ntoa(details.raddr.sin_addr);
		std::cerr << ": " << ntohs(details.raddr.sin_port);
		std::cerr << " type: " << details.type;
		std::cerr << std::endl;
	}

	if (it->second.inConnAttempt)
	{
		std::cerr << "p3ConnectMgr::peerStatus() ALREADY IN CONNECT ATTEMPT: ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
		/*  -> it'll automatically use the addresses */
		return;
	}

	std::cerr << "p3ConnectMgr::peerStatus() Started CONNECT ATTEMPT! ";
	std::cerr << " id: " << id;
	std::cerr << std::endl;


	/* start a connection attempt */
	it->second.actions |= RS_PEER_CONNECT_REQ;
	mStatusChanged = true;
}


void    p3ConnectMgr::peerConnectRequest(std::string id, uint32_t type)
{
	std::cerr << "p3ConnectMgr::peerConnectRequest()";
	std::cerr << " id: " << id;
	std::cerr << " type: " << type;
	std::cerr << std::endl;

	/* look up the id */
        std::map<std::string, peerConnectState>::iterator it;
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	bool isFriend = true;
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
		/* check Others list */
		isFriend = false;
		it = mOthersList.find(id);
		if (it == mOthersList.end())
		{
			/* not found - ignore */
			std::cerr << "p3ConnectMgr::peerStatus() Peer Not Found - Ignore";
			std::cerr << std::endl;
			return;
		}
		std::cerr << "p3ConnectMgr::peerStatus() Peer is in mOthersList";
		std::cerr << std::endl;
	}

	/* if already connected -> done */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
		return;
	}

	time_t now = time(NULL);
	/* this is a UDP connection request (DHT only for the moment!) */
	if (isValidNet(&(it->second.dht.raddr.sin_addr)))

	{
		/* add the remote address */
		peerConnectAddress pca;
		pca.ts = now;
		pca.type = RS_NET_CONN_UDP_DHT_SYNC;
		pca.addr = it->second.dht.raddr;

		/* add to the start of list -> so handled next! */
		//it->second.connAddrs.push_front(pca);
		/* push to the back ... TCP ones should be tried first */
		it->second.connAddrs.push_back(pca);
	}


	if (it->second.inConnAttempt)
	{
		/*  -> it'll automatically use the addresses */
		return;
	}

	/* start a connection attempt */
	it->second.actions |= RS_PEER_CONNECT_REQ;
	mStatusChanged = true;
}




//void    p3ConnectMgr::stunStatus(std::string id, struct sockaddr_in addr)

/*******************************************************************/
/*******************************************************************/

bool p3ConnectMgr::addFriend(std::string id, uint32_t netMode, uint32_t visState, time_t lastContact)
{
	/* so three possibilities 
	 * (1) already exists as friend -> do nothing.
	 * (2) is in others list -> move over.
	 * (3) is non-existant -> create new one.
	 */

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::addFriend() " << id;
	std::cerr << std::endl;
#endif

	connMtx.lock();   /*   LOCK MUTEX */


        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() != mFriendList.find(id))
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::addFriend() Already Exists";
		std::cerr << std::endl;
#endif
		/* (1) already exists */

		connMtx.unlock(); /* UNLOCK MUTEX */
		return true;
	}

	/* check with the AuthMgr if its authorised */
	if (!mAuthMgr->isAuthenticated(id))
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::addFriend() Failed Authentication";
		std::cerr << std::endl;
#endif
		/* no auth */

		connMtx.unlock(); /* UNLOCK MUTEX */
		return false;
	}

	/* check if it is in others */
	if (mOthersList.end() != (it = mOthersList.find(id)))
	{
		/* (2) in mOthersList -> move over */
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::addFriend() Move from Others";
		std::cerr << std::endl;
#endif

		mFriendList[id] = it->second;
		mOthersList.erase(it);

		it = mFriendList.find(id);

		/* setup state */
		it->second.state = RS_PEER_S_FRIEND;
		it->second.actions = RS_PEER_NEW;

		/* setup connectivity parameters */
		it->second.visState = visState;
		it->second.netMode  = netMode;
		it->second.lastcontact = lastContact;

		mStatusChanged = true;

		/* add peer to DHT (if not dark) */
		if (it->second.visState & RS_VIS_STATE_NODHT)
		{
			/* hidden from DHT world */
			mDhtMgr->dropPeer(id);
		}
		else
		{
			mDhtMgr->findPeer(id);
		}

		connMtx.unlock(); /* UNLOCK MUTEX */

		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

		return true;
	}

	/* get details from AuthMgr */
	pqiAuthDetails detail;
	if (!mAuthMgr->getDetails(id, detail))
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::addFriend() Failed to get Details";
		std::cerr << std::endl;
#endif
		/* ERROR: no details */

		connMtx.unlock(); /* UNLOCK MUTEX */
		return false;
	}


#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::addFriend() Creating New Entry";
	std::cerr << std::endl;
#endif

	/* create a new entry */
	peerConnectState pstate;

	pstate.id = id;
	pstate.name = detail.name;

	pstate.state = RS_PEER_S_FRIEND;
	pstate.actions = RS_PEER_NEW;
	pstate.visState = visState;
	pstate.netMode = netMode;
	pstate.lastcontact = lastContact;

	/* addr & timestamps -> auto cleared */

	mFriendList[id] = pstate;

	mStatusChanged = true;

	/* expect it to be a standard DHT */
	mDhtMgr->findPeer(id);

	connMtx.unlock(); /* UNLOCK MUTEX */

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return true;
}


bool p3ConnectMgr::removeFriend(std::string id)
{

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::removeFriend() " << id;
	std::cerr << std::endl;
#endif

	mDhtMgr->dropPeer(id);

	connMtx.lock();   /*   LOCK MUTEX */

	/* move to othersList */
	bool success = false;
        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() != (it = mFriendList.find(id)))
	{

		peerConnectState peer = it->second;

		mFriendList.erase(it);

		peer.state &= (~RS_PEER_S_FRIEND);
		peer.state &= (~RS_PEER_S_CONNECTED);
		peer.state &= (~RS_PEER_S_ONLINE);
		peer.actions = RS_PEER_MOVED;
		peer.inConnAttempt = false;
		mOthersList[id] = peer;
		mStatusChanged = true;

		success = true;
	}

	connMtx.unlock(); /* UNLOCK MUTEX */

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return success;
}



bool p3ConnectMgr::addNeighbour(std::string id)
{

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::addNeighbour() " << id;
	std::cerr << std::endl;
#endif

	/* so three possibilities 
	 * (1) already exists as friend -> do nothing.
	 * (2) already in others list -> do nothing.
	 * (3) is non-existant -> create new one.
	 */

	connMtx.lock();   /*   LOCK MUTEX */

        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == mFriendList.find(id))
	{
		/* (1) already exists */
		connMtx.unlock(); /* UNLOCK MUTEX */
		return false;
	}

	if (mOthersList.end() == mOthersList.find(id))
	{
		/* (2) already exists */
		connMtx.unlock(); /* UNLOCK MUTEX */
		return true;
	}

	/* check with the AuthMgr if its valid */
	if (!mAuthMgr->isValid(id))
	{
		/* no auth */
		connMtx.unlock(); /* UNLOCK MUTEX */
		return false;
	}

	/* get details from AuthMgr */
	pqiAuthDetails detail;
	if (!mAuthMgr->getDetails(id, detail))
	{
		/* no details */
		connMtx.unlock(); /* UNLOCK MUTEX */
		return false;
	}

	/* create a new entry */
	peerConnectState pstate;

	pstate.id = id;
	pstate.name = detail.name;

	pstate.state = 0;
	pstate.actions = 0; //RS_PEER_NEW;
	pstate.visState = RS_VIS_STATE_STD;
	pstate.netMode = RS_NET_MODE_UNKNOWN;

	/* addr & timestamps -> auto cleared */

	mOthersList[id] = pstate;

	// Nothing to notify anyone about... as no new information
	//mStatusChanged = true;

	connMtx.unlock(); /* UNLOCK MUTEX */

	return true;
}


/*******************************************************************/
/*******************************************************************/
       /*************** External Control ****************/
bool   p3ConnectMgr::retryConnect(std::string id)
{
	/* push addresses onto stack */
	std::cerr << "p3ConnectMgr::retryConnect()";
	std::cerr << " id: " << id;
	std::cerr << std::endl;

	/* look up the id */
        std::map<std::string, peerConnectState>::iterator it;
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */

	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		std::cerr << "p3ConnectMgr::retryConnect() Peer is not Friend";
		std::cerr << std::endl;
		return false;
	}

	/* if already connected -> done */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
		std::cerr << "p3ConnectMgr::retryConnect() Peer Already Connected";
		std::cerr << std::endl;
		return true;
	}

	/* are the addresses different? */

	time_t now = time(NULL);
        std::list<peerConnectAddress>::iterator cit;

	/* add in attempts ... local(TCP), remote(TCP) 
	 */

	/* if address is same -> try local */
	if ((isValidNet(&(it->second.localaddr.sin_addr))) &&
		(sameNet(&(ownState.localaddr.sin_addr), 
			&(it->second.localaddr.sin_addr))))

	{
		std::cerr << "p3ConnectMgr::retryConnect() Local Address Valid: ";
		std::cerr << inet_ntoa(it->second.localaddr.sin_addr);
		std::cerr << ":" << ntohs(it->second.localaddr.sin_port);
		std::cerr << std::endl;

		bool localExists = false;
		if ((it->second.inConnAttempt) && 
			(it->second.currentConnAddr.type == RS_NET_CONN_TCP_LOCAL))
		{
			localExists = true;
		}

		for(cit = it->second.connAddrs.begin();
			(!localExists) && (cit != it->second.connAddrs.begin()); cit++)
		{
			if (cit->type == RS_NET_CONN_TCP_LOCAL)
			{
				localExists = true;
			}
		}

		/* check if there is a local one on there already */

		if (!localExists)
		{
			std::cerr << "p3ConnectMgr::retryConnect() Adding Local Addr to Queue";
			std::cerr << std::endl;

			/* add the local address */
			peerConnectAddress pca;
			pca.ts = now;
			pca.type = RS_NET_CONN_TCP_LOCAL;
			pca.addr = it->second.localaddr;

			it->second.connAddrs.push_back(pca);
		}
		else
		{
			std::cerr << "p3ConnectMgr::retryConnect() Local Addr already in Queue";
			std::cerr << std::endl;
		}
	}

	/* otherwise try external ... (should check flags) */
	if ((isValidNet(&(it->second.serveraddr.sin_addr))) && (1))
	//	(it->second.netMode & RS_NET_CONN_TCP_EXTERNAL))
	{
		std::cerr << "p3ConnectMgr::retryConnect() Ext Address Valid (+EXT Flag): ";
		std::cerr << inet_ntoa(it->second.serveraddr.sin_addr);
		std::cerr << ":" << ntohs(it->second.serveraddr.sin_port);
		std::cerr << std::endl;


		bool remoteExists = false;
		if ((it->second.inConnAttempt) && 
			(it->second.currentConnAddr.type == RS_NET_CONN_TCP_EXTERNAL))
		{
			remoteExists = true;
		}

		for(cit = it->second.connAddrs.begin();
			(!remoteExists) && (cit != it->second.connAddrs.begin()); cit++)
		{
			if (cit->type == RS_NET_CONN_TCP_EXTERNAL)
			{
				remoteExists = true;
			}
		}

		/* check if there is a local one on there already */

		if (!remoteExists)
		{
			std::cerr << "p3ConnectMgr::retryConnect() Adding Ext Addr to Queue";
			std::cerr << std::endl;

			/* add the remote address */
			peerConnectAddress pca;
			pca.ts = now;
			pca.type = RS_NET_CONN_TCP_EXTERNAL;
			pca.addr = it->second.serveraddr;

			it->second.connAddrs.push_back(pca);
		}
		else
		{
			std::cerr << "p3ConnectMgr::retryConnect() Ext Addr already in Queue";
			std::cerr << std::endl;
		}
	}

	if (it->second.inConnAttempt)
	{
		/*  -> it'll automatically use the addresses */
		return true;
	}

	
	/* start a connection attempt (only if we stuck something on the queue) */
	if (it->second.connAddrs.size() > 0)
	{
		it->second.actions |= RS_PEER_CONNECT_REQ;
		mStatusChanged = true;
	}

	return true; 
}



bool    p3ConnectMgr::setLocalAddress(std::string id, struct sockaddr_in addr)
{
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	if (id == mAuthMgr->OwnId())
	{
		ownState.localaddr = addr;
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		return true;
	}

	/* check if it is a friend */
        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
			return false;
		}
	}

	/* "it" points to peer */
	it->second.localaddr = addr;
	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return true;
}

bool    p3ConnectMgr::setExtAddress(std::string id, struct sockaddr_in addr)
{
	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	if (id == mAuthMgr->OwnId())
	{
		ownState.serveraddr = addr;
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		return true;
	}

	/* check if it is a friend */
        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
			return false;
		}
	}

	/* "it" points to peer */
	it->second.serveraddr = addr;
	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return true;
}

bool    p3ConnectMgr::setNetworkMode(std::string id, uint32_t netMode)
{
	if (id == mAuthMgr->OwnId())
	{
		uint32_t visState = ownState.visState;
		setOwnNetConfig(netMode, visState);

		return true;
	}

	/* check if it is a friend */
        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
			return false;
		}
	}

	/* "it" points to peer */
	it->second.netMode = netMode;
	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/


	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	return false;
}

bool    p3ConnectMgr::setVisState(std::string id, uint32_t visState)
{
	if (id == mAuthMgr->OwnId())
	{
		uint32_t netMode = ownState.netMode;
		setOwnNetConfig(netMode, visState);

		return true;
	}

	/* check if it is a friend */
        std::map<std::string, peerConnectState>::iterator it;
	bool isFriend = false;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
			return false;
		}
	}
	else
	{
		isFriend = true;
	}

	/* "it" points to peer */
	it->second.visState = visState;
	if (isFriend)
	{
		/* toggle DHT state */
		if (it->second.visState & RS_VIS_STATE_NODHT)
		{
			/* hidden from DHT world */
			mDhtMgr->dropPeer(id);
		}
		else
		{
			mDhtMgr->findPeer(id);
		}
	}

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/


	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	return false;
}




bool p3ConnectMgr::getUPnPState()
{
	return mUpnpMgr->getUPnPActive();
}

bool p3ConnectMgr::getUPnPEnabled()
{
	return mUpnpMgr->getUPnPEnabled();
}

bool p3ConnectMgr::getDHTEnabled()
{
	return mDhtMgr->getDhtOn();
}


/*******************************************************************/

bool 	p3ConnectMgr::checkNetAddress()
{
	std::cerr << "p3ConnectMgr::checkNetAddress()";
	std::cerr << std::endl;

	std::list<std::string> addrs = getLocalInterfaces();
	std::list<std::string>::iterator it;

	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */
	bool found = false;
	for(it = addrs.begin(); (!found) && (it != addrs.end()); it++)
	{
		std::cerr << "p3ConnectMgr::checkNetAddress() Local Interface: " << *it;
		std::cerr << std::endl;

		if ((*it) == inet_ntoa(ownState.localaddr.sin_addr))
		{
			std::cerr << "p3ConnectMgr::checkNetAddress() Matches Existing Address! FOUND = true";
			std::cerr << std::endl;
			found = true;
		}
	}
	/* check that we didn't catch 0.0.0.0 - if so go for prefered */
	if ((found) && (ownState.localaddr.sin_addr.s_addr == 0))
	{
		found = false;
	}

	if (!found)
	{
		ownState.localaddr.sin_addr = getPreferredInterface();

		std::cerr << "p3ConnectMgr::checkNetAddress() Local Address Not Found: Using Preferred Interface: ";
		std::cerr << inet_ntoa(ownState.localaddr.sin_addr);
		std::cerr << std::endl;
	
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	}
	if ((isPrivateNet(&(ownState.localaddr.sin_addr))) ||
		(isLoopbackNet(&(ownState.localaddr.sin_addr))))
	{
		/* firewalled */
		//own_cert -> Firewalled(true);
	}
	else
	{
		//own_cert -> Firewalled(false);
	}

	int port = ntohs(ownState.localaddr.sin_port);
	if ((port < PQI_MIN_PORT) || (port > PQI_MAX_PORT))
	{
		ownState.localaddr.sin_port = htons(PQI_DEFAULT_PORT);
	}

	/* if localaddr = serveraddr, then ensure that the ports
	 * are the same (modify server)... this mismatch can 
	 * occur when the local port is changed....
	 */

	if (ownState.localaddr.sin_addr.s_addr == 
			ownState.serveraddr.sin_addr.s_addr)
	{
		ownState.serveraddr.sin_port = 
			ownState.localaddr.sin_port;
	}

	// ensure that address family is set, otherwise windows Barfs.
	ownState.localaddr.sin_family = AF_INET;
	ownState.serveraddr.sin_family = AF_INET;

	std::cerr << "p3ConnectMgr::checkNetAddress() Final Local Address: ";
	std::cerr << inet_ntoa(ownState.localaddr.sin_addr);
	std::cerr << ":" << ntohs(ownState.localaddr.sin_port);
	std::cerr << std::endl;
  
	return 1;
}


/************************* p3config functions **********************/
/*******************************************************************/
        /* Key Functions to be overloaded for Full Configuration */

RsSerialiser *p3ConnectMgr::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsPeerConfigSerialiser());

	return rss;
}


std::list<RsItem *> p3ConnectMgr::saveList(bool &cleanup)
{
	/* create a list of current peers */
	std::list<RsItem *> saveData;
	cleanup = true;

	connMtx.lock();   /*   LOCK MUTEX */
	connMtx.unlock(); /* UNLOCK MUTEX */

	RsPeerNetItem *item = new RsPeerNetItem();
	item->clear();

	item->pid = getOwnId();
	item->netMode = ownState.netMode;
	item->visState = ownState.visState;
	item->lastContact = ownState.lastcontact;
	item->localaddr = ownState.localaddr;
	item->remoteaddr = ownState.serveraddr;

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::saveList() Own Config Item:";
	std::cerr << std::endl;
	item->print(std::cerr, 10);
	std::cerr << std::endl;
#endif

	saveData.push_back(item);

	/* iterate through all friends and save */
        std::map<std::string, peerConnectState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		item = new RsPeerNetItem();
		item->clear();

		item->pid = it->first;
		item->netMode = (it->second).netMode;
		item->visState = (it->second).visState;
		item->lastContact = (it->second).lastcontact;
		item->localaddr = (it->second).localaddr;
		item->remoteaddr = (it->second).serveraddr;

		saveData.push_back(item);
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::saveList() Peer Config Item:";
		std::cerr << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
#endif
	}

	RsPeerStunItem *sitem = new RsPeerStunItem();

	std::list<std::string>::iterator sit;
	for(sit = mStunList.begin(); sit != mStunList.end(); sit++)
	{
		sitem->stunList.ids.push_back(*sit);
	}
	saveData.push_back(sitem);

	return saveData;
}

bool  p3ConnectMgr::loadList(std::list<RsItem *> load)
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::loadList() Item Count: " << load.size();
	std::cerr << std::endl;
#endif

	/* load the list of peers */
	std::list<RsItem *>::iterator it;
	for(it = load.begin(); it != load.end(); it++)
	{
		RsPeerNetItem *pitem = dynamic_cast<RsPeerNetItem *>(*it);
		RsPeerStunItem *sitem = dynamic_cast<RsPeerStunItem *>(*it);

		if (pitem)
		{
			if (pitem->pid == getOwnId())
			{
#ifdef CONN_DEBUG
				std::cerr << "p3ConnectMgr::loadList() Own Config Item:";
				std::cerr << std::endl;
				pitem->print(std::cerr, 10);
				std::cerr << std::endl;
#endif
				/* add ownConfig */
				setOwnNetConfig(pitem->netMode, pitem->visState);
				setLocalAddress(pitem->pid, pitem->localaddr);
				setExtAddress(pitem->pid, pitem->remoteaddr);
			}
			else
			{
#ifdef CONN_DEBUG
				std::cerr << "p3ConnectMgr::loadList() Peer Config Item:";
				std::cerr << std::endl;
				pitem->print(std::cerr, 10);
				std::cerr << std::endl;
#endif
				/* ************* */
				addFriend(pitem->pid, pitem->netMode, pitem->visState, pitem->lastContact);
				setLocalAddress(pitem->pid, pitem->localaddr);
				setExtAddress(pitem->pid, pitem->remoteaddr);
			}
		}
		else if (sitem)
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::loadList() Stun Config Item:";
			std::cerr << std::endl;
			sitem->print(std::cerr, 10);
			std::cerr << std::endl;
#endif
			std::list<std::string>::iterator sit;
			for(sit = sitem->stunList.ids.begin();
				sit != sitem->stunList.ids.end(); sit++)
			{
				mStunList.push_back(*sit);
			}
		}

		connMtx.lock();   /*   LOCK MUTEX */
		connMtx.unlock(); /* UNLOCK MUTEX */
		delete (*it);
	}
	return true;
}



void  printConnectState(peerConnectState &peer)
{

#ifdef CONN_DEBUG
				std::cerr << "Friend: " << peer.name;
				std::cerr << " Id: " << peer.id;
				std::cerr << " State: " << peer.state;
				if (peer.state & RS_PEER_S_FRIEND)
					std::cerr << " S:RS_PEER_S_FRIEND";
				if (peer.state & RS_PEER_S_ONLINE)
					std::cerr << " S:RS_PEER_S_ONLINE";
				if (peer.state & RS_PEER_S_CONNECTED)
					std::cerr << " S:RS_PEER_S_CONNECTED";
				std::cerr << " Actions: " << peer.actions;
				if (peer.actions & RS_PEER_NEW)
					std::cerr << " A:RS_PEER_NEW";
				if (peer.actions & RS_PEER_MOVED)
					std::cerr << " A:RS_PEER_MOVED";
				if (peer.actions & RS_PEER_CONNECTED)
					std::cerr << " A:RS_PEER_CONNECTED";
				if (peer.actions & RS_PEER_DISCONNECTED)
					std::cerr << " A:RS_PEER_DISCONNECTED";
				if (peer.actions & RS_PEER_CONNECT_REQ)
					std::cerr << " A:RS_PEER_CONNECT_REQ";

				std::cerr << std::endl;
#endif
	return;
}

