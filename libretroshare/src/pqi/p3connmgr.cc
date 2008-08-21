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
#include "pqi/p3dhtmgr.h" // Only need it for constants.
#include "tcponudp/tou.h"

#include "util/rsprint.h"
#include "util/rsdebug.h"
const int p3connectzone = 3431;

#include "serialiser/rsconfigitems.h"
#include "pqi/pqinotify.h"

#include <sstream>

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
const uint32_t RS_STUN_FOUND_MIN =     	10;

const uint32_t MAX_UPNP_INIT = 		10; /* seconds UPnP timeout */

/****
 * #define CONN_DEBUG 1
 ***/
/****
 * #define P3CONNMGR_NO_TCP_CONNECTIONS 1
 ***/
/****
 * #define P3CONNMGR_NO_AUTO_CONNECTION 1
 ***/

#define CONN_DEBUG 1
const uint32_t P3CONNMGR_TCP_DEFAULT_DELAY = 2; /* 2 Seconds? is it be enough! */
const uint32_t P3CONNMGR_UDP_DHT_DELAY     = DHT_NOTIFY_PERIOD + 60; /* + 1 minute for DHT POST */
const uint32_t P3CONNMGR_UDP_PROXY_DELAY   = 30;  /* 30 seconds (NOT IMPLEMENTED YET!) */

#define MAX_AVAIL_PERIOD (2 * DHT_NOTIFY_PERIOD)  // If we haven't connected in 2 DHT periods.
#define MIN_RETRY_PERIOD (DHT_CHECK_PERIOD + 120) // just over DHT CHECK_PERIOD

void  printConnectState(peerConnectState &peer);

peerConnectAddress::peerConnectAddress()
	:delay(0), period(0), type(0), ts(0)
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
	 connecttype(0),
	 lastavailable(0),
	 lastattempt(0),
	 name("nameless"), state(0), actions(0), 
	 source(0), 
	 inConnAttempt(0)
{
	sockaddr_clear(&localaddr);
	sockaddr_clear(&serveraddr);

	return;
}


p3ConnectMgr::p3ConnectMgr(p3AuthMgr *am)
	:p3Config(CONFIG_TYPE_PEERS), 
	mAuthMgr(am), mNetStatus(RS_NET_UNKNOWN), 
	mStunStatus(0), mStunFound(0), mStunMoreRequired(true), 
	mStatusChanged(false)
{
	mUpnpAddrValid = false;
	mStunAddrValid = false;
	mStunAddrStable = false;

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
	/* only change TRY flags */

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::setOwnNetConfig()" << std::endl;
	std::cerr << "Existing netMode: " << ownState.netMode << " vis: " << ownState.visState;
	std::cerr << std::endl;
	std::cerr << "Input netMode: " << netMode << " vis: " << visState;
	std::cerr << std::endl;
#endif
	ownState.netMode &= ~(RS_NET_MODE_TRYMODE);

#ifdef CONN_DEBUG
	std::cerr << "After Clear netMode: " << ownState.netMode << " vis: " << ownState.visState;
	std::cerr << std::endl;
#endif

	switch(netMode & RS_NET_MODE_ACTUAL)
	{
		case RS_NET_MODE_EXT:
			ownState.netMode |= RS_NET_MODE_TRY_EXT;
			break;
		case RS_NET_MODE_UPNP:
			ownState.netMode |= RS_NET_MODE_TRY_UPNP;
			break;
		default:
		case RS_NET_MODE_UDP:
			ownState.netMode |= RS_NET_MODE_TRY_UDP;
			break;
	}

	ownState.visState = visState;

#ifdef CONN_DEBUG
	std::cerr << "Final netMode: " << ownState.netMode << " vis: " << ownState.visState;
	std::cerr << std::endl;
#endif

	/* if we've started up - then tweak Dht On/Off */
	if (mNetStatus != RS_NET_UNKNOWN)
	{
		enableNetAssistFirewall(!(ownState.visState & RS_VIS_STATE_NODHT));
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
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netStartup()" << std::endl;
#endif

	loadConfiguration();
	netDhtInit();
	netUdpInit();
	netStunInit();

	/* decide which net setup mode we're going into 
	 */

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	mNetInitTS = time(NULL);

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netStartup() tou_stunkeepalive() enabled" << std::endl;
#endif
	tou_stunkeepalive(1);

	ownState.netMode &= ~(RS_NET_MODE_ACTUAL);

	switch(ownState.netMode & RS_NET_MODE_TRYMODE)
	{

		case RS_NET_MODE_TRY_EXT:  /* v similar to UDP */
			ownState.netMode |= RS_NET_MODE_EXT;
			mNetStatus = RS_NET_UDP_SETUP;
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netStartup() disabling stunkeepalive() cos EXT" << std::endl;
#endif
			tou_stunkeepalive(0);
			mStunMoreRequired = false; /* only need to validate address (EXT) */

			break;

		case RS_NET_MODE_TRY_UDP:
			ownState.netMode |= RS_NET_MODE_UDP;
			mNetStatus = RS_NET_UDP_SETUP;
			break;

		case RS_NET_MODE_TRY_UPNP:
		default:
			/* Force it here (could be default!) */
			ownState.netMode |= RS_NET_MODE_TRY_UPNP;
			ownState.netMode |= RS_NET_MODE_UDP;      /* set to UDP, upgraded is UPnP is Okay */
			mNetStatus = RS_NET_UPNP_INIT;
			break;
	}
}


void p3ConnectMgr::tick()
{
	netTick();
	statusTick();
	tickMonitors();

}

bool p3ConnectMgr::shutdown() /* blocking shutdown call */
{
	connMtx.lock();   /*   LOCK MUTEX */

	bool upnpActive = ownState.netMode & RS_NET_MODE_UPNP;

	connMtx.unlock(); /* UNLOCK MUTEX */

	if (upnpActive)
	{
		netAssistFirewallShutdown();
	}
	netAssistConnectShutdown();

	return true;
}


void    p3ConnectMgr::statusTick()
{
	/* iterate through peers ... 
	 * 	if been available for long time ... remove flag
	 * 	if last attempt a while - retryConnect. 
	 * 	etc.
	 */

#ifdef CONN_DEBUG
	//std::cerr << "p3ConnectMgr::statusTick()" << std::endl;
#endif
	std::list<std::string> retryIds;
	std::list<std::string>::iterator it2;

	time_t now = time(NULL);
	time_t oldavail = now - MAX_AVAIL_PERIOD;
	time_t retry = now - MIN_RETRY_PERIOD;

      {
      	RsStackMutex stack(connMtx);  /******   LOCK MUTEX ******/
        std::map<std::string, peerConnectState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		if (it->second.state & RS_PEER_S_CONNECTED)
		{
			continue;
		}

		if ((it->second.state & RS_PEER_S_ONLINE) &&
			(it->second.lastavailable < oldavail))
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::statusTick() ONLINE TIMEOUT for: ";
			std::cerr << it->first;
			std::cerr << std::endl;
#endif
			it->second.state &= (~RS_PEER_S_ONLINE);
		}

		if (it->second.lastattempt < retry)
		{
			retryIds.push_back(it->first);
		}
	}
      }

#ifndef P3CONNMGR_NO_AUTO_CONNECTION 

        for(it2 = retryIds.begin(); it2 != retryIds.end(); it2++)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::statusTick() RETRY TIMEOUT for: ";
		std::cerr << *it2;
		std::cerr << std::endl;
#endif
		/* retry it! */
		retryConnectTCP(*it2);
	}

#endif

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
			stunCheck(); /* Keep on stunning until its happy */
		default:
			break;
	}

	return;
}


void p3ConnectMgr::netUdpInit()
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netUdpInit()" << std::endl;
#endif
	connMtx.lock();   /*   LOCK MUTEX */

	struct sockaddr_in iaddr = ownState.localaddr;

	connMtx.unlock(); /* UNLOCK MUTEX */

	/* open our udp port */
	tou_init((struct sockaddr *) &iaddr, sizeof(iaddr));
}


void p3ConnectMgr::netDhtInit()
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netDhtInit()" << std::endl;
#endif
	connMtx.lock();   /*   LOCK MUTEX */

	uint32_t vs = ownState.visState;

	connMtx.unlock(); /* UNLOCK MUTEX */

	enableNetAssistFirewall(!(vs & RS_VIS_STATE_NODHT));
}


void p3ConnectMgr::netUpnpInit()
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netUpnpInit()" << std::endl;
#endif
	uint16_t eport, iport;

	connMtx.lock();   /*   LOCK MUTEX */

	/* get the ports from the configuration */

	mNetStatus = RS_NET_UPNP_SETUP;
	iport = ntohs(ownState.localaddr.sin_port);
	eport = ntohs(ownState.serveraddr.sin_port);
	if ((eport < 1000) || (eport > 30000))
	{
		eport = iport;
	}

	connMtx.unlock(); /* UNLOCK MUTEX */

	netAssistFirewallPorts(iport, eport);
	enableNetAssistFirewall(true);
}

void p3ConnectMgr::netUpnpCheck()
{
	/* grab timestamp */
	connMtx.lock();   /*   LOCK MUTEX */

	time_t delta = time(NULL) - mNetInitTS;

	connMtx.unlock(); /* UNLOCK MUTEX */

	struct sockaddr_in extAddr;
	int upnpState = netAssistFirewallActive();

	if ((upnpState < 0) ||
	   ((upnpState == 0) && (delta > MAX_UPNP_INIT)))
	{
		/* fallback to UDP startup */
		connMtx.lock();   /*   LOCK MUTEX */

		/* UPnP Failed us! */
		mUpnpAddrValid = false;
		mNetStatus = RS_NET_UDP_SETUP;
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netUpnpCheck() enabling stunkeepalive() cos UDP" << std::endl;
#endif
		tou_stunkeepalive(1);

		connMtx.unlock(); /* UNLOCK MUTEX */
	}
	else if ((upnpState > 0) &&
		netAssistExtAddress(extAddr))
	{
		/* switch to UDP startup */
		connMtx.lock();   /*   LOCK MUTEX */

		mUpnpAddrValid = true;
		mUpnpExtAddr = extAddr;
		mNetStatus = RS_NET_UDP_SETUP;
		/* Fix netMode & Clear others! */
		ownState.netMode = RS_NET_MODE_TRY_UPNP | RS_NET_MODE_UPNP; 
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netUpnpCheck() disabling stunkeepalive() cos uPnP" << std::endl;
#endif
		tou_stunkeepalive(0);
		mStunMoreRequired = false; /* only need to validate address (UPNP) */

		connMtx.unlock(); /* UNLOCK MUTEX */
	}
}

void p3ConnectMgr::netUdpCheck()
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netUdpCheck()" << std::endl;
#endif
	if (udpExtAddressCheck() || (mUpnpAddrValid)) 
	{
		bool extValid = false;
		bool extAddrStable = false;
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
			extAddrStable = true;
		}
		else if (mStunAddrValid)
		{
			extValid = true;
			extAddr = mStunExtAddr;
			extAddrStable = mStunAddrStable;
		}

		if (extValid)
		{
			ownState.serveraddr = extAddr;
			mode = RS_NET_CONN_TCP_LOCAL;

			if (!extAddrStable)
			{
#ifdef CONN_DEBUG
				std::cerr << "p3ConnectMgr::netUdpCheck() UDP Unstable :( ";
				std::cerr <<  std::endl;
				std::cerr << "p3ConnectMgr::netUdpCheck() We are unreachable";
				std::cerr <<  std::endl;
				std::cerr << "netMode =>  RS_NET_MODE_UNREACHABLE";
				std::cerr <<  std::endl;
#endif
				ownState.netMode &= ~(RS_NET_MODE_ACTUAL);
				ownState.netMode |= RS_NET_MODE_UNREACHABLE;
				tou_stunkeepalive(0);
				mStunMoreRequired = false; /* no point -> unreachable (EXT) */

				/* send a system warning message */
				pqiNotify *notify = getPqiNotify();
				if (notify)
				{
					std::string title = 
						"Warning: Bad Firewall Configuration";

					std::string msg;
					msg +=  "               **** WARNING ****     \n";
					msg +=  "Retroshare has detected that you are behind";
					msg +=  " a restrictive Firewall\n";
					msg +=  "\n";
					msg +=  "You cannot connect to other firewalled peers\n";
					msg +=  "\n";
					msg +=  "You can fix this by:\n";
					msg +=  "   (1) opening an External Port\n";
					msg +=  "   (2) enabling UPnP, or\n";
					msg +=  "   (3) get a new (approved) Firewall/Router\n";

					notify->AddSysMessage(0, RS_SYS_WARNING, title, msg);
				}

			}
			else if (mUpnpAddrValid  || (ownState.netMode & RS_NET_MODE_EXT))
			{
				mode |= RS_NET_CONN_TCP_EXTERNAL;
				mode |= RS_NET_CONN_UDP_DHT_SYNC;
			}
			else // if (extAddrStable)
			{
				/* Check if extAddr == intAddr (Not Firewalled) */
				if ((0 == inaddr_cmp(iaddr, extAddr)) &&
					isExternalNet(&(extAddr.sin_addr)))
				{
					mode |= RS_NET_CONN_TCP_EXTERNAL;
				}

				mode |= RS_NET_CONN_UDP_DHT_SYNC;
			}

			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}

		connMtx.unlock(); /* UNLOCK MUTEX */

		if (extValid)
		{
			netAssistSetAddress(iaddr, extAddr, mode);
		}
		else
		{
			/* mode = 0 for error */
			netAssistSetAddress(iaddr, extAddr, mode);
		}

		/* flag unreachables! */
		if ((extValid) && (!extAddrStable))
		{
			netUnreachableCheck();
		}
	}
}

void p3ConnectMgr::netUnreachableCheck()
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netUnreachableCheck()" << std::endl;
#endif
        std::map<std::string, peerConnectState>::iterator it;

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		/* get last contact detail */
		if (it->second.state & RS_PEER_S_CONNECTED)
		{
#ifdef CONN_DEBUG
			std::cerr << "NUC() Ignoring Connected Peer" << std::endl;
#endif
			continue;
		}

		peerAddrInfo details;
		switch(it->second.source)
		{
			case RS_CB_DHT:
				details = it->second.dht;
#ifdef CONN_DEBUG
				std::cerr << "NUC() Using DHT data" << std::endl;
#endif
				break;
			case RS_CB_DISC:
				details = it->second.disc;
#ifdef CONN_DEBUG
				std::cerr << "NUC() Using DISC data" << std::endl;
#endif
				break;
			case RS_CB_PERSON:
				details = it->second.peer;
#ifdef CONN_DEBUG
				std::cerr << "NUC() Using PEER data" << std::endl;
#endif
				break;
			default:
				continue;
				break;
		}

		std::cerr << "NUC() Peer: " << it->first << std::endl;

		/* Determine Reachability (only advisory) */
		// if (ownState.netMode == RS_NET_MODE_UNREACHABLE) // MUST BE TRUE!
		{
			if (details.type & RS_NET_CONN_TCP_EXTERNAL) 
			{
				/* reachable! */
				it->second.state &= (~RS_PEER_S_UNREACHABLE);
#ifdef CONN_DEBUG
				std::cerr << "NUC() Peer EXT TCP - reachable" << std::endl;
#endif
			}
			else 
			{
				/* unreachable */
				it->second.state |= RS_PEER_S_UNREACHABLE;
#ifdef CONN_DEBUG
				std::cerr << "NUC() Peer !EXT TCP - unreachable" << std::endl;
#endif
			}
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
	uint8_t stable;

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::udpExtAddressCheck()" << std::endl;
#endif

	if (0 < tou_extaddr((struct sockaddr *) &addr, &len, &stable))
	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/


		/* update UDP information */
		mStunExtAddr = addr;
		mStunAddrValid = true;
		mStunAddrStable = (stable != 0);

#ifdef CONN_DEBUG
        	std::cerr << "p3ConnectMgr::udpExtAddressCheck() Got ";
	        std::cerr << " addr: " << inet_ntoa(mStunExtAddr.sin_addr);
		std::cerr << ":" << ntohs(mStunExtAddr.sin_port);
		std::cerr << " stable: " << mStunAddrStable;
		std::cerr << std::endl;
#endif

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

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	netAssistStun(true);

	/* push stun list to DHT */
	std::list<std::string>::iterator it;
	for(it = mStunList.begin(); it != mStunList.end(); it++)
	{
		netAssistAddStun(*it);
	}
	mStunStatus = RS_STUN_DHT;
	mStunFound = 0;
	mStunMoreRequired = true;
}

bool p3ConnectMgr::stunCheck()
{
	/* check if we've got a Stun result */
	bool stunOk = false;

#ifdef CONN_DEBUG
	//std::cerr << "p3ConnectMgr::stunCheck()" << std::endl;
#endif

      {
      	RsStackMutex stack(connMtx); /********* LOCK STACK MUTEX ******/

	/* if DONE -> return */
	if (mStunStatus == RS_STUN_DONE)
	{
		return true;
	}

	if (mStunFound >= RS_STUN_FOUND_MIN)
	{
		mStunMoreRequired = false;
	}
	stunOk = (!mStunMoreRequired);
      }


	if (udpExtAddressCheck() && (stunOk))
	{
		/* set external UDP address */
		netAssistStun(false);

		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		mStunStatus = RS_STUN_DONE;

		return true;
	}
	return false;
}

void    p3ConnectMgr::stunStatus(std::string id, struct sockaddr_in raddr, uint32_t type, uint32_t flags)
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::stunStatus()";
	std::cerr << " id: " << RsUtil::BinToHex(id) << " raddr: " << inet_ntoa(raddr.sin_addr);
	std::cerr << ":" << ntohs(raddr.sin_port);
	std::cerr << std::endl;
#endif

	connMtx.lock();   /*   LOCK MUTEX */

	bool stillStunning = (mStunStatus == RS_STUN_DHT);

	connMtx.unlock(); /* UNLOCK MUTEX */

	/* only useful if they have an exposed TCP/UDP port */
	if (type & RS_NET_CONN_TCP_EXTERNAL) 
	{
		if (stillStunning)
		{
			connMtx.lock();   /*   LOCK MUTEX */
			mStunFound++;
			connMtx.unlock(); /* UNLOCK MUTEX */
		
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::stunStatus() Sending to UDP" << std::endl;
#endif
			/* push to the UDP */
			udpStunPeer(id, raddr);

		}

		/* push to the stunCollect */
		stunCollect(id, raddr, flags);
	}
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
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

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
		if ((flags & RS_STUN_ONLINE) || (flags & RS_STUN_FRIEND))
		{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::stunCollect() Id added to Front" << std::endl;
#endif
			/* push to the front */
			mStunList.push_front(id);

			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}
		else if (mStunList.size() < RS_STUN_LIST_MIN)
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::stunCollect() Id added to Back" << std::endl;
#endif
			/* push to the front */
			mStunList.push_back(id);

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

}

/********************************  Network Status  *********************************
 * Configuration Loading / Saving.
 */


void p3ConnectMgr::addMonitor(pqiMonitor *mon)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

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
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

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
	bool doStatusChange = false;
	std::list<pqipeer> actionList;
        std::map<std::string, peerConnectState>::iterator it;

      {
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	if (mStatusChanged)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::tickMonitors() StatusChanged! List:" << std::endl;
#endif
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

				/* notify GUI */
				if (peer.actions & RS_PEER_CONNECTED)
				{
					pqiNotify *notify = getPqiNotify();
					if (notify)
					{
						notify->AddPopupMessage(RS_POPUP_CONNECT, 
							peer.id, "Peer Online: ");


						notify->AddFeedItem(RS_FEED_ITEM_PEER_CONNECT, peer.id, "", "");
					}
				}
#if 0
				if (peer.actions & RS_PEER_DISCONNECTED)
				{
					pqiNotify *notify = getPqiNotify();
					if (notify)
					{
						notify->AddFeedItem(RS_FEED_ITEM_PEER_DISCONNECT, peer.id, "", "");


					}
				}
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
		doStatusChange = true;
	
	}
      } /****** UNLOCK STACK MUTEX ******/

	/* NOTE - clients is accessed without mutex protection!!!!
	 * At the moment this is okay - as they are only added at the start.
	 * IF this changes ---- must fix with second Mutex.
	 */

	if (doStatusChange)
	{
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
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
	state = ownState;
	return true;
}

bool p3ConnectMgr::isFriend(std::string id)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
	return (mFriendList.end() != mFriendList.find(id));
}

bool p3ConnectMgr::isOnline(std::string id)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() != (it = mFriendList.find(id)))
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::isOnline(" << id;
		std::cerr << ") is Friend, Online: ";
		std::cerr << (it->second.state & RS_PEER_S_CONNECTED);
		std::cerr << std::endl;
#endif
		return (it->second.state & RS_PEER_S_CONNECTED);
	}
	else
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::isOnline(" << id;
		std::cerr << ") is Not Friend";
		std::cerr << std::endl;
		std::cerr << "p3ConnectMgr::isOnline() OwnId: ";
		std::cerr << mAuthMgr->OwnId();
		std::cerr << std::endl;
#endif
		/* not a friend */
	}

	return false;
}

bool p3ConnectMgr::getFriendNetStatus(std::string id, peerConnectState &state)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
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
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
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
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
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
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		peers.push_back(it->first);
	}
	return;
}


void p3ConnectMgr::getOthersList(std::list<std::string> &peers)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
	for(it = mOthersList.begin(); it != mOthersList.end(); it++)
	{
		peers.push_back(it->first);
	}
	return;
}



bool p3ConnectMgr::connectAttempt(std::string id, struct sockaddr_in &addr, 
                                uint32_t &delay, uint32_t &period, uint32_t &type)

{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::connectAttempt() FAILED Not in FriendList!";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif

		return false;
	}

	if (it->second.connAddrs.size() < 1)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::connectAttempt() FAILED No ConnectAddresses";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif
		return false;
	}
	
	it->second.lastattempt = time(NULL);  /* time of last connect attempt */
	it->second.inConnAttempt = true;
	it->second.currentConnAddr = it->second.connAddrs.front();
	it->second.connAddrs.pop_front();

	addr = it->second.currentConnAddr.addr;
	delay = it->second.currentConnAddr.delay;
	period = it->second.currentConnAddr.period;
	type = it->second.currentConnAddr.type;

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::connectAttempt() Success: ";
	std::cerr << " id: " << id;
	std::cerr << std::endl;
	std::cerr << " laddr: " << inet_ntoa(addr.sin_addr);
	std::cerr << " lport: " << ntohs(addr.sin_port);
	std::cerr << " delay: " << delay;
	std::cerr << " period: " << period;
	std::cerr << " type: " << type;
	std::cerr << std::endl;
#endif

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
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
        std::map<std::string, peerConnectState>::iterator it;
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::connectResult() Failed, missing Friend ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif
		return false;
	}


	it->second.inConnAttempt = false;

	if (success)
	{
		/* remove other attempts */
		it->second.connAddrs.clear();
		netAssistFriend(id, false);

		/* update address (will come through from DISC) */

#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::connectResult() Connect!: ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
		std::cerr << " Success: " << success;
		std::cerr << " flags: " << flags;
		std::cerr << std::endl;
#endif


		/* change state */
		it->second.state |= RS_PEER_S_CONNECTED;
		it->second.actions |= RS_PEER_CONNECTED;
		mStatusChanged = true;
		it->second.lastcontact = time(NULL);  /* time of connect */
		it->second.connecttype = flags;

		return true;
	}

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::connectResult() Disconnect/Fail: ";
	std::cerr << " id: " << id;
	std::cerr << std::endl;
	std::cerr << " Success: " << success;
	std::cerr << " flags: " << flags;
	std::cerr << std::endl;
#endif

	/* if currently connected -> flag as failed */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
		it->second.state &= (~RS_PEER_S_CONNECTED);
		it->second.actions |= RS_PEER_DISCONNECTED;

		it->second.lastcontact = time(NULL);  /* time of disconnect */

		netAssistFriend(id, true);
		if (it->second.visState & RS_VIS_STATE_NODHT)
		{
			/* hidden from DHT world */
		}
		else
		{
			//netAssistFriend(id, true);
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
        std::map<std::string, peerConnectState>::iterator it;
	bool isFriend = true;

	time_t now = time(NULL);

	peerAddrInfo details;
	details.type    = type;
	details.found   = true;
	details.laddr   = laddr;
	details.raddr   = raddr;
	details.ts      = now;


      {
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

#ifdef CONN_DEBUG
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
#endif
	{
		/* Log */
		std::ostringstream out;
		out << "p3ConnectMgr::peerStatus()";
		out << " id: " << id;
		out << " laddr: " << inet_ntoa(laddr.sin_addr);
		out << " lport: " << ntohs(laddr.sin_port);
		out << " raddr: " << inet_ntoa(raddr.sin_addr);
		out << " rport: " << ntohs(raddr.sin_port);
		out << " type: " << type;
		out << " flags: " << flags;
		out << " source: " << source;
		rslog(RSL_WARNING, p3connectzone, out.str());
	}

	/* look up the id */
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
		/* check Others list */
		isFriend = false;
		it = mOthersList.find(id);
		if (it == mOthersList.end())
		{
			/* not found - ignore */
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::peerStatus() Peer Not Found - Ignore";
			std::cerr << std::endl;
#endif
			return;
		}
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() Peer is in mOthersList";
		std::cerr << std::endl;
#endif
	}

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::peerStatus() Current Peer State:" << std::endl;
	printConnectState(it->second);
	std::cerr << std::endl;
#endif

	/* update the status */

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
		it->second.lastavailable = now;
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
			it->second.lastavailable = now;
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
		it->second.lastavailable = now;

		/* must be online to recv info (should be connected too!) 
		 * but no need for action as should be connected already 
		 */

		it->second.netMode &= (~RS_NET_MODE_ACTUAL); /* clear actual flags */
		if (flags & RS_NET_FLAGS_EXTERNAL_ADDR)
		{
			it->second.netMode = RS_NET_MODE_EXT;
		}
		else if (flags & RS_NET_FLAGS_STABLE_UDP)
		{
			it->second.netMode = RS_NET_MODE_UDP;
		}
		else
		{
			it->second.netMode = RS_NET_MODE_UNREACHABLE;
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

	/* Determine Reachability (only advisory) */
	if (ownState.netMode & RS_NET_MODE_UDP)
	{
		if ((details.type & RS_NET_CONN_UDP_DHT_SYNC) ||
		    (details.type & RS_NET_CONN_TCP_EXTERNAL)) 
		{
			/* reachable! */
			it->second.state &= (~RS_PEER_S_UNREACHABLE);
		}
		else 
		{
			/* unreachable */
			it->second.state |= RS_PEER_S_UNREACHABLE;
		}
	}
	else if (ownState.netMode & RS_NET_MODE_UNREACHABLE)
	{
		if (details.type & RS_NET_CONN_TCP_EXTERNAL) 
		{
			/* reachable! */
			it->second.state &= (~RS_PEER_S_UNREACHABLE);
		}
		else 
		{
			/* unreachable */
			it->second.state |= RS_PEER_S_UNREACHABLE;
		}
	}
	else
	{
		it->second.state &= (~RS_PEER_S_UNREACHABLE);
	}

	if (!isFriend)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() NOT FRIEND ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif

		{
			/* Log */
			std::ostringstream out;
			out << "p3ConnectMgr::peerStatus() NO CONNECT (not friend)";
			rslog(RSL_WARNING, p3connectzone, out.str());
		}
		return;
	}

	/* if already connected -> done */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() PEER ONLINE ALREADY ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif
		{
			/* Log */
			std::ostringstream out;
			out << "p3ConnectMgr::peerStatus() NO CONNECT (already connected!)";
			rslog(RSL_WARNING, p3connectzone, out.str());
		}

		return;
	}


	/* are the addresses different? */

#ifdef CONN_DEBUG
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
#endif

#ifndef P3CONNMGR_NO_AUTO_CONNECTION 

#ifndef P3CONNMGR_NO_TCP_CONNECTIONS

	/* add in attempts ... local(TCP), remote(TCP) 
	 * udp must come from notify 
	 */

	/* determine delay (for TCP connections) 
	 * this is to ensure that simultaneous connections don't occur
	 * (which can fail).
	 * easest way is to compare ids ... and delay one of them
	 */
	
	uint32_t tcp_delay = 0;
	if (id > ownState.id)
	{
		tcp_delay = P3CONNMGR_TCP_DEFAULT_DELAY;
	}

	/* if address is same -> try local */
	if ((isValidNet(&(details.laddr.sin_addr))) &&
		(sameNet(&(ownState.localaddr.sin_addr), &(details.laddr.sin_addr))))

	{
		/* add the local address */
		peerConnectAddress pca;
		pca.ts = now;
		pca.delay = tcp_delay;
		pca.period = 0;
		pca.type = RS_NET_CONN_TCP_LOCAL;
		pca.addr = details.laddr;

#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() ADDING TCP_LOCAL ADDR: ";
		std::cerr << " id: " << id;
		std::cerr << " laddr: " << inet_ntoa(pca.addr.sin_addr);
		std::cerr << " lport: " << ntohs(pca.addr.sin_port);
		std::cerr << " delay: " << pca.delay;
		std::cerr << " period: " << pca.period;
		std::cerr << " type: " << pca.type;
		std::cerr << " source: " << source;
		std::cerr << std::endl;
#endif
		{
			/* Log */
			std::ostringstream out;
			out << "p3ConnectMgr::peerStatus() PushBack Local TCP Address: ";
			out << " id: " << id;
			out << " laddr: " << inet_ntoa(pca.addr.sin_addr);
			out << ":" << ntohs(pca.addr.sin_port);
			out << " type: " << pca.type;
			out << " delay: " << pca.delay;
			out << " period: " << pca.period;
			out << " ts: " << pca.ts;
			out << " source: " << source;
			rslog(RSL_WARNING, p3connectzone, out.str());
		}
	
		it->second.connAddrs.push_back(pca);
	}
	else
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() Not adding Local Connect (Diff Network)";
		std::cerr << " id: " << id;
		std::cerr << " laddr: " << inet_ntoa(details.laddr.sin_addr);
		std::cerr << ": " << ntohs(details.laddr.sin_port);
		std::cerr << " own.laddr: " << inet_ntoa(ownState.localaddr.sin_addr);
		std::cerr << ": " << ntohs(ownState.localaddr.sin_port);
		std::cerr << std::endl;
#endif
	}


	if ((details.type & RS_NET_CONN_TCP_EXTERNAL) &&
	   (isValidNet(&(details.raddr.sin_addr))))

	{
		/* add the remote address */
		peerConnectAddress pca;
		pca.ts = now;
		pca.delay = tcp_delay;
		pca.period = 0;
		pca.type = RS_NET_CONN_TCP_EXTERNAL;
		pca.addr = details.raddr;

#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() ADDING TCP_REMOTE ADDR: ";
		std::cerr << " id: " << id;
		std::cerr << " raddr: " << inet_ntoa(pca.addr.sin_addr);
		std::cerr << " rport: " << ntohs(pca.addr.sin_port);
		std::cerr << " delay: " << pca.delay;
		std::cerr << " period: " << pca.period;
		std::cerr << " type: " << pca.type;
		std::cerr << " source: " << source;
		std::cerr << std::endl;
#endif
		{
			/* Log */
			std::ostringstream out;
			out << "p3ConnectMgr::peerStatus() PushBack Remote TCP Address: ";
			out << " id: " << id;
			out << " raddr: " << inet_ntoa(pca.addr.sin_addr);
			out << ":" << ntohs(pca.addr.sin_port);
			out << " type: " << pca.type;
			out << " delay: " << pca.delay;
			out << " period: " << pca.period;
			out << " ts: " << pca.ts;
			out << " source: " << source;
			rslog(RSL_WARNING, p3connectzone, out.str());
		}

		it->second.connAddrs.push_back(pca);
	}
	else
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() Not adding Remote Connect (Type != E or Invalid Network)";
		std::cerr << " id: " << id;
		std::cerr << " raddr: " << inet_ntoa(details.raddr.sin_addr);
		std::cerr << ": " << ntohs(details.raddr.sin_port);
		std::cerr << " type: " << details.type;
		std::cerr << std::endl;
#endif
	}

#endif  // P3CONNMGR_NO_TCP_CONNECTIONS

      } /****** STACK UNLOCK MUTEX *******/

	/* notify if they say we can, or we cannot connect ! */
	if (details.type & RS_NET_CONN_UDP_DHT_SYNC) 
	{
		retryConnectNotify(id);
	}
#else 
      } // P3CONNMGR_NO_AUTO_CONNECTION /****** STACK UNLOCK MUTEX *******/
#endif  // P3CONNMGR_NO_AUTO_CONNECTION 

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	if (it->second.inConnAttempt)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() ALREADY IN CONNECT ATTEMPT: ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;

		/*  -> it'll automatically use the addresses */

		std::cerr << "p3ConnectMgr::peerStatus() Resulting Peer State:" << std::endl;
		printConnectState(it->second);
		std::cerr << std::endl;
#endif

		return;
	}


	/* start a connection attempt */
	if (it->second.connAddrs.size() > 0)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() Started CONNECT ATTEMPT! ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif

		it->second.actions |= RS_PEER_CONNECT_REQ;
		mStatusChanged = true;
	}
	else
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() No addr suitable for CONNECT ATTEMPT! ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif
	}

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::peerStatus() Resulting Peer State:" << std::endl;
	printConnectState(it->second);
	std::cerr << std::endl;
#endif

}

void    p3ConnectMgr::peerConnectRequest(std::string id, struct sockaddr_in raddr,
                       							uint32_t source)
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::peerConnectRequest()";
	std::cerr << " id: " << id;
	std::cerr << " raddr: " << inet_ntoa(raddr.sin_addr);
	std::cerr << ":" << ntohs(raddr.sin_port);
	std::cerr << " source: " << source;
	std::cerr << std::endl;
#endif
	{
		/* Log */
		std::ostringstream out;
		out << "p3ConnectMgr::peerConnectRequest()";
		out << " id: " << id;
		out << " raddr: " << inet_ntoa(raddr.sin_addr);
		out << ":" << ntohs(raddr.sin_port);
		out << " source: " << source;
		rslog(RSL_WARNING, p3connectzone, out.str());
	}

	/******************** TCP PART *****************************/

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::peerConnectRequest() Try TCP first";
	std::cerr << std::endl;
#endif

	retryConnectTCP(id);

	/******************** UDP PART *****************************/

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	if (ownState.netMode & RS_NET_MODE_UNREACHABLE)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerConnectRequest() Unreachable - no UDP connection";
		std::cerr << std::endl;
#endif
		return;
	}

	/* look up the id */
        std::map<std::string, peerConnectState>::iterator it;
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
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::peerConnectRequest() Peer Not Found - Ignore";
			std::cerr << std::endl;
#endif
			return;
		}
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerConnectRequest() Peer is in mOthersList - Ignore";
		std::cerr << std::endl;
#endif
		return;
	}

	/* if already connected -> done */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerConnectRequest() Already connected - Ignore";
		std::cerr << std::endl;
#endif
		return;
	}


	time_t now = time(NULL);
	/* this is a UDP connection request (DHT only for the moment!) */
	if (isValidNet(&(raddr.sin_addr)))
	{
		/* add the remote address */
		peerConnectAddress pca;
		pca.ts = now;
		pca.type = RS_NET_CONN_UDP_DHT_SYNC;
		pca.delay = 0;

		if (source == RS_CB_DHT)
		{
			pca.period = P3CONNMGR_UDP_DHT_DELAY; 
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::peerConnectRequest() source = DHT ";
			std::cerr << std::endl;
#endif
		}
		else if (source == RS_CB_PROXY)
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::peerConnectRequest() source = PROXY ";
			std::cerr << std::endl;
#endif
			pca.period = P3CONNMGR_UDP_PROXY_DELAY; 
		}
		else
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::peerConnectRequest() source = UNKNOWN ";
			std::cerr << std::endl;
#endif
			/* error! */
			pca.period = P3CONNMGR_UDP_PROXY_DELAY; 
		}

#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerConnectRequest() period = " << pca.period;
		std::cerr << std::endl;
#endif

		pca.addr = raddr;

		{
			/* Log */
			std::ostringstream out;
			out << "p3ConnectMgr::peerConnectRequest() PushBack UDP Address: ";
			out << " id: " << id;
			out << " raddr: " << inet_ntoa(pca.addr.sin_addr);
			out << ":" << ntohs(pca.addr.sin_port);
			out << " type: " << pca.type;
			out << " delay: " << pca.delay;
			out << " period: " << pca.period;
			out << " ts: " << pca.ts;
			rslog(RSL_WARNING, p3connectzone, out.str());
		}

		/* push to the back ... TCP ones should be tried first */
		it->second.connAddrs.push_back(pca);
	}

	if (it->second.inConnAttempt)
	{
		/*  -> it'll automatically use the addresses */
		return;
	}

	/* start a connection attempt */
	if (it->second.connAddrs.size() > 0)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerConnectRequest() Started CONNECT ATTEMPT! ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif

		it->second.actions |= RS_PEER_CONNECT_REQ;
		mStatusChanged = true;
	}
	else
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerConnectRequest() No addr suitable for CONNECT ATTEMPT! ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif
	}
}



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

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/


        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() != mFriendList.find(id))
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::addFriend() Already Exists";
		std::cerr << std::endl;
#endif
		/* (1) already exists */
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
			netAssistFriend(id, false);
		}
		else
		{
			netAssistFriend(id, true);
		}

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
	netAssistFriend(id, true);

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return true;
}


bool p3ConnectMgr::removeFriend(std::string id)
{

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::removeFriend() " << id;
	std::cerr << std::endl;
#endif

	netAssistFriend(id, false);

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

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

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == mFriendList.find(id))
	{
		/* (1) already exists */
		return false;
	}

	if (mOthersList.end() == mOthersList.find(id))
	{
		/* (2) already exists */
		return true;
	}

	/* check with the AuthMgr if its valid */
	if (!mAuthMgr->isValid(id))
	{
		/* no auth */
		return false;
	}

	/* get details from AuthMgr */
	pqiAuthDetails detail;
	if (!mAuthMgr->getDetails(id, detail))
	{
		/* no details */
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

	return true;
}


/*******************************************************************/
/*******************************************************************/
       /*************** External Control ****************/
bool   p3ConnectMgr::retryConnect(std::string id)
{
	retryConnectTCP(id);
	retryConnectNotify(id);

	return true;
}


bool   p3ConnectMgr::retryConnectTCP(std::string id)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* push addresses onto stack */
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::retryConnectTCP()";
	std::cerr << " id: " << id;
	std::cerr << std::endl;
#endif

	/* look up the id */
        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectTCP() Peer is not Friend";
		std::cerr << std::endl;
#endif
		return false;
	}

	/* if already connected -> done */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectTCP() Peer Already Connected";
		std::cerr << std::endl;
#endif
		return true;
	}

	/* are the addresses different? */

	time_t now = time(NULL);
        std::list<peerConnectAddress>::iterator cit;

	/* add in attempts ... local(TCP), remote(TCP) 
	 */

#ifndef P3CONNMGR_NO_TCP_CONNECTIONS

	/* if address is same -> try local */
	if ((isValidNet(&(it->second.localaddr.sin_addr))) &&
		(sameNet(&(ownState.localaddr.sin_addr), 
			&(it->second.localaddr.sin_addr))))

	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectTCP() Local Address Valid: ";
		std::cerr << inet_ntoa(it->second.localaddr.sin_addr);
		std::cerr << ":" << ntohs(it->second.localaddr.sin_port);
		std::cerr << std::endl;
#endif

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
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::retryConnectTCP() Adding Local Addr to Queue";
			std::cerr << std::endl;
#endif

			/* add the local address */
			peerConnectAddress pca;
			pca.ts = now;
			pca.type = RS_NET_CONN_TCP_LOCAL;
			pca.addr = it->second.localaddr;
	
			{
				/* Log */
				std::ostringstream out;
				out << "p3ConnectMgr::retryConnectTCP() PushBack Local TCP Address: ";
				out << " id: " << id;
				out << " raddr: " << inet_ntoa(pca.addr.sin_addr);
				out << ":" << ntohs(pca.addr.sin_port);
				out << " type: " << pca.type;
				out << " delay: " << pca.delay;
				out << " period: " << pca.period;
				out << " ts: " << pca.ts;
				rslog(RSL_WARNING, p3connectzone, out.str());
			}
	
			it->second.connAddrs.push_back(pca);
		}
		else
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::retryConnectTCP() Local Addr already in Queue";
			std::cerr << std::endl;
#endif
		}
	}

	/* otherwise try external ... (should check flags) */
	//if ((isValidNet(&(it->second.serveraddr.sin_addr))) && 
	//		(it->second.netMode = RS_NET_MODE_EXT))
	
	/* always try external */
	if (isValidNet(&(it->second.serveraddr.sin_addr)))
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectTCP() Ext Address Valid: ";
		std::cerr << inet_ntoa(it->second.serveraddr.sin_addr);
		std::cerr << ":" << ntohs(it->second.serveraddr.sin_port);
		std::cerr << std::endl;
#endif


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
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::retryConnectTCP() Adding Ext Addr to Queue";
			std::cerr << std::endl;
#endif

			/* add the remote address */
			peerConnectAddress pca;
			pca.ts = now;
			pca.type = RS_NET_CONN_TCP_EXTERNAL;
			pca.addr = it->second.serveraddr;

			{
				/* Log */
				std::ostringstream out;
				out << "p3ConnectMgr::retryConnectTCP() PushBack Ext TCP Address: ";
				out << " id: " << id;
				out << " raddr: " << inet_ntoa(pca.addr.sin_addr);
				out << ":" << ntohs(pca.addr.sin_port);
				out << " type: " << pca.type;
				out << " delay: " << pca.delay;
				out << " period: " << pca.period;
				out << " ts: " << pca.ts;
				rslog(RSL_WARNING, p3connectzone, out.str());
			}

			it->second.connAddrs.push_back(pca);
		}
		else
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::retryConnectTCP() Ext Addr already in Queue";
			std::cerr << std::endl;
#endif
		}
	}

#endif // P3CONNMGR_NO_TCP_CONNECTIONS

	/* flag as last attempt to prevent loop */
	it->second.lastattempt = time(NULL);  

	if (it->second.inConnAttempt)
	{
		/*  -> it'll automatically use the addresses */
		return true;
	}

	/* start a connection attempt */
	if (it->second.connAddrs.size() > 0)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectTCP() Started CONNECT ATTEMPT! ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif

		it->second.actions |= RS_PEER_CONNECT_REQ;
		mStatusChanged = true;
	}
	else
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectTCP() No addr suitable for CONNECT ATTEMPT! ";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif
	}
	return true; 
}


bool   p3ConnectMgr::retryConnectNotify(std::string id)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* push addresses onto stack */
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::retryConnectNotify()";
	std::cerr << " id: " << id;
	std::cerr << std::endl;
#endif

	/* look up the id */
        std::map<std::string, peerConnectState>::iterator it;

	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectNotify() Peer is not Friend";
		std::cerr << std::endl;
#endif
		return false;
	}

	/* if already connected -> done */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectNotify() Peer Already Connected";
		std::cerr << std::endl;
#endif
		return true;
	}

	/* flag as last attempt to prevent loop */
	it->second.lastattempt = time(NULL);  

	if (ownState.netMode & RS_NET_MODE_UNREACHABLE)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectNotify() UNREACHABLE so no Notify!";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif
	}
	else
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectNotify() Notifying Peer";
		std::cerr << " id: " << id;
		std::cerr << std::endl;
#endif
		{
			/* Log */
			std::ostringstream out;
			out  << "p3ConnectMgr::retryConnectNotify() Notifying Peer";
			out  << " id: " << id;
			rslog(RSL_WARNING, p3connectzone, out.str());
		}

		/* attempt UDP connection */
		netAssistNotify(id);
	}

	return true; 
}





bool    p3ConnectMgr::setLocalAddress(std::string id, struct sockaddr_in addr)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

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
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/


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

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

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

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

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
			netAssistFriend(id, false);
		}
		else
		{
			netAssistFriend(id, true);
		}
	}

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return false;
}




/*******************************************************************/

bool 	p3ConnectMgr::checkNetAddress()
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::checkNetAddress()";
	std::cerr << std::endl;
#endif

	std::list<std::string> addrs = getLocalInterfaces();
	std::list<std::string>::iterator it;

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	bool found = false;
	for(it = addrs.begin(); (!found) && (it != addrs.end()); it++)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::checkNetAddress() Local Interface: " << *it;
		std::cerr << std::endl;
#endif

		if ((*it) == inet_ntoa(ownState.localaddr.sin_addr))
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::checkNetAddress() Matches Existing Address! FOUND = true";
			std::cerr << std::endl;
#endif
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

#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::checkNetAddress() Local Address Not Found: Using Preferred Interface: ";
		std::cerr << inet_ntoa(ownState.localaddr.sin_addr);
		std::cerr << std::endl;
#endif
	
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

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::checkNetAddress() Final Local Address: ";
	std::cerr << inet_ntoa(ownState.localaddr.sin_addr);
	std::cerr << ":" << ntohs(ownState.localaddr.sin_port);
	std::cerr << std::endl;
#endif
  
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

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	RsPeerNetItem *item = new RsPeerNetItem();
	item->clear();

	item->pid = getOwnId();
	if (ownState.netMode & RS_NET_MODE_TRY_EXT)
	{
		item->netMode = RS_NET_MODE_EXT;
	}
	else if (ownState.netMode & RS_NET_MODE_TRY_UPNP)
	{
		item->netMode = RS_NET_MODE_UPNP;
	}
	else
	{
		item->netMode = RS_NET_MODE_UDP;
	}

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
	uint32_t count = 0;
	for(sit = mStunList.begin(); (sit != mStunList.end()) && 
			(count < RS_STUN_LIST_MIN); sit++, count++)
	{
		sitem->stunList.ids.push_back(*sit);
	}

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::saveList() Peer Stun Item:";
	std::cerr << std::endl;
	sitem->print(std::cerr, 10);
	std::cerr << std::endl;
#endif

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
			RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
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



bool  p3ConnectMgr::addBootstrapStunPeers()
{
	std::string id;
	struct sockaddr_in dummyaddr;
	uint32_t flags = 0;

	/* only use the Bootstrap system now */

	return true;
}

/************************ INTERFACES ***********************/


void p3ConnectMgr::addNetAssistFirewall(uint32_t id, pqiNetAssistFirewall *fwAgent)
{
	mFwAgents[id] = fwAgent;
}


bool p3ConnectMgr::enableNetAssistFirewall(bool on)
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); it++)
	{
		(it->second)->enable(on);
	}
	return true;
}


bool p3ConnectMgr::netAssistFirewallEnabled()
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); it++)
	{
		if ((it->second)->getEnabled())
		{
			return true;
		}
	}
	return false;
}

bool p3ConnectMgr::netAssistFirewallActive()
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); it++)
	{
		if ((it->second)->getActive())
		{
			return true;
		}
	}
	return false;
}

bool p3ConnectMgr::netAssistFirewallShutdown()
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); it++)
	{
		(it->second)->shutdown();
	}
	return true;
}

bool p3ConnectMgr::netAssistFirewallPorts(uint16_t iport, uint16_t eport)
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); it++)
	{
		(it->second)->setInternalPort(iport);
		(it->second)->setExternalPort(eport);
	}
	return true;
}


bool p3ConnectMgr::netAssistExtAddress(struct sockaddr_in &extAddr)
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); it++)
	{
		if ((it->second)->getActive())
		{
			if ((it->second)->getExternalAddress(extAddr))
			{
				return true;
			}
		}
	}
	return false;
}


void p3ConnectMgr::addNetAssistConnect(uint32_t id, pqiNetAssistConnect *dht)
{
	mDhts[id] = dht;
}

bool p3ConnectMgr::enableNetAssistConnect(bool on)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		(it->second)->enable(on);
	}
	return true;
}

bool p3ConnectMgr::netAssistConnectEnabled()
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		if ((it->second)->getEnabled())
		{
			return true;
		}
	}
	return false;
}

bool p3ConnectMgr::netAssistConnectActive()
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		if ((it->second)->getActive())

		{
			return true;
		}
	}
	return false;
}

bool p3ConnectMgr::netAssistConnectShutdown()
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		(it->second)->shutdown();
	}
	return true;
}

bool p3ConnectMgr::netAssistFriend(std::string id, bool on)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		if (on)
		{
			(it->second)->findPeer(id);
		}
		else
		{
			(it->second)->dropPeer(id);
		}
	}
	return true;
}

bool p3ConnectMgr::netAssistAddStun(std::string id)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		(it->second)->addStun(id);
	}
	return true;
}


bool p3ConnectMgr::netAssistStun(bool on)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		(it->second)->enableStun(on);
	}
	return true;
}

bool p3ConnectMgr::netAssistNotify(std::string id)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		(it->second)->notifyPeer(id);
	}
	return true;
}


bool p3ConnectMgr::netAssistSetAddress( struct sockaddr_in &laddr,
					struct sockaddr_in &eaddr,
					uint32_t mode)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		(it->second)->setExternalInterface(laddr, eaddr, mode);
	}
	return true;
}


bool    p3ConnectMgr::getUPnPState()
{
	return netAssistFirewallActive();
}

bool	p3ConnectMgr::getUPnPEnabled()
{
	return netAssistFirewallEnabled();
}

bool	p3ConnectMgr::getDHTEnabled()
{
	return netAssistConnectEnabled();
}





