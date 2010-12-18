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

#include "pqi/authssl.h"
#include "pqi/p3connmgr.h"
#include "pqi/p3dhtmgr.h" // Only need it for constants.
#include "tcponudp/tou.h"
#include "tcponudp/extaddrfinder.h"
#include "util/rsnet.h"
#include "pqi/authgpg.h"


#include "util/rsprint.h"
#include "util/rsdebug.h"
const int p3connectzone = 3431;

#include "serialiser/rsconfigitems.h"
#include "pqi/pqinotify.h"
#include "retroshare/rsiface.h"

#include <sstream>

/* Network setup States */

const uint32_t RS_NET_NEEDS_RESET = 	0x0000;
const uint32_t RS_NET_UNKNOWN = 	0x0001;
const uint32_t RS_NET_UPNP_INIT = 	0x0002;
const uint32_t RS_NET_UPNP_SETUP =  	0x0003;
const uint32_t RS_NET_EXT_SETUP =  	0x0004;
const uint32_t RS_NET_DONE =    	0x0005;
const uint32_t RS_NET_LOOPBACK =    	0x0006;
const uint32_t RS_NET_DOWN =    	0x0007;

/* Stun modes (TODO) */
const uint32_t RS_STUN_DHT =      	0x0001;
const uint32_t RS_STUN_DONE =      	0x0002;
const uint32_t RS_STUN_LIST_MIN =      	100;
const uint32_t RS_STUN_FOUND_MIN =     	10;

const uint32_t MAX_UPNP_INIT = 		60; /* seconds UPnP timeout */
const uint32_t MAX_UPNP_COMPLETE = 	600; /* 10 min... seems to take a while */
const uint32_t MAX_NETWORK_INIT =	70; /* timeout before network reset */

const uint32_t MIN_TIME_BETWEEN_NET_RESET = 		5;

const uint32_t PEER_IP_CONNECT_STATE_MAX_LIST_SIZE =     	4;

/****
 * #define CONN_DEBUG 1
 * #define CONN_DEBUG_RESET 1
 * #define CONN_DEBUG_TICK 1
 ***/

/****
 * #define P3CONNMGR_NO_TCP_CONNECTIONS 1
 ***/
/****
 * #define P3CONNMGR_NO_AUTO_CONNECTION 1
 ***/

const uint32_t P3CONNMGR_TCP_DEFAULT_DELAY = 3; /* 2 Seconds? is it be enough! */
const uint32_t P3CONNMGR_UDP_DEFAULT_DELAY = 3; /* 2 Seconds? is it be enough! */

const uint32_t P3CONNMGR_TCP_DEFAULT_PERIOD = 10;
const uint32_t P3CONNMGR_UDP_DEFAULT_PERIOD = 40; 

#define MAX_AVAIL_PERIOD 230 //times a peer stay in available state when not connected
#define MIN_RETRY_PERIOD 140

#define MAX_RANDOM_ATTEMPT_OFFSET	6 // seconds.

void  printConnectState(std::ostream &out, peerConnectState &peer);

peerConnectAddress::peerConnectAddress()
	:delay(0), period(0), type(0), ts(0)
{
	sockaddr_clear(&addr);
}


peerAddrInfo::peerAddrInfo()
	:found(false), type(0), ts(0)
{
}

peerConnectState::peerConnectState()
	:id("unknown"), 
         gpg_id("unknown"),
	 netMode(RS_NET_MODE_UNKNOWN), visState(RS_VIS_STATE_STD), 
	 lastcontact(0),
	 connecttype(0),
	 lastavailable(0),
         lastattempt(0),
         name(""), location(""),
         state(0), actions(0),
	 source(0), 
	 inConnAttempt(0)
{
	sockaddr_clear(&currentlocaladdr);
	sockaddr_clear(&currentserveraddr);

	return;
}

std::string textPeerConnectState(peerConnectState &state)
{
	std::ostringstream out;
	out << "Id: " << state.id << std::endl;
	out << "NetMode: " << state.netMode << std::endl;
	out << "VisState: " << state.visState << std::endl;
	out << "laddr: " << rs_inet_ntoa(state.currentlocaladdr.sin_addr)
		<< ":" << ntohs(state.currentlocaladdr.sin_port) << std::endl;
	out << "eaddr: " << rs_inet_ntoa(state.currentserveraddr.sin_addr)
		<< ":" << ntohs(state.currentserveraddr.sin_port) << std::endl;

	std::string output = out.str();
	return output;
}


pqiNetStatus::pqiNetStatus()
	:mLocalAddrOk(false), mExtAddrOk(false), mExtAddrStableOk(false), 
	mUpnpOk(false), mDhtOk(false), mResetReq(false)
{
        mDhtNetworkSize = 0;
        mDhtRsNetworkSize = 0;

	sockaddr_clear(&mLocalAddr);
	sockaddr_clear(&mExtAddr);
	return;
}



void pqiNetStatus::print(std::ostream &out)
{
	out << "pqiNetStatus: ";
	out << "mLocalAddrOk: " << mLocalAddrOk; 
        out << " mExtAddrOk: " << mExtAddrOk;
        out << " mExtAddrStableOk: " << mExtAddrStableOk;
	out << std::endl;
        out << " mUpnpOk: " << mUpnpOk;
        out << " mDhtOk: " << mDhtOk;
        out << " mResetReq: " << mResetReq;
        out << std::endl;
	out << "mDhtNetworkSize: " << mDhtNetworkSize << " mDhtRsNetworkSize: " << mDhtRsNetworkSize;
        out << std::endl;
	out << "mLocalAddr: " << rs_inet_ntoa(mLocalAddr.sin_addr) << ":" << ntohs(mLocalAddr.sin_port) << " ";
	out << "mExtAddr: " << rs_inet_ntoa(mExtAddr.sin_addr) << ":" << ntohs(mExtAddr.sin_port) << " ";
	out << " NetOk: " << NetOk();
        out << std::endl;
}


p3ConnectMgr::p3ConnectMgr()
	:p3Config(CONFIG_TYPE_PEERS), 
        mNetStatus(RS_NET_UNKNOWN),
	mStatusChanged(false)
{

	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		/* setup basics of own state */
		mOwnState.id = AuthSSL::getAuthSSL()->OwnId();
		mOwnState.gpg_id = AuthGPG::getAuthGPG()->getGPGOwnId();
		mOwnState.name = AuthGPG::getAuthGPG()->getGPGOwnName();
		mOwnState.location = AuthSSL::getAuthSSL()->getOwnLocation();
		mOwnState.netMode = RS_NET_MODE_UDP;
		// user decided.
		//mOwnState.netMode |= RS_NET_MODE_TRY_UPNP;
	
		mUseExtAddrFinder = true;
		mAllowTunnelConnection = false;
		mExtAddrFinder = new ExtAddrFinder;
		mNetInitTS = 0;
	        mRetryPeriod = MIN_RETRY_PERIOD;
	
		mNetFlags = pqiNetStatus();
		mOldNetFlags = pqiNetStatus();

		lastGroupId = 1;

		/* setup Banned Ip Address - static for now 
		 */

		struct in_addr bip;
		memset(&bip, 0, sizeof(bip));
		bip.s_addr = 1;

		mBannedIpList.push_back(bip);
	}
	
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr() Startup" << std::endl;
#endif

	netReset();

	return;
}

bool  p3ConnectMgr::getIPServersEnabled() 
{ 
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
	return mUseExtAddrFinder;
}

void  p3ConnectMgr::getIPServersList(std::list<std::string>& ip_servers) 
{ 
	mExtAddrFinder->getIPServersList(ip_servers);
}

void p3ConnectMgr::setIPServersEnabled(bool b)
{
	bool changed = false;
	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
		if (mUseExtAddrFinder != b)
			changed = true;
		mUseExtAddrFinder = b;
	}

	if (changed)
	{
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr: setIPServers to " << b << std::endl ; 
#endif
}

void p3ConnectMgr::setTunnelConnection(bool b)
{
	bool changed = false;
	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		if (mAllowTunnelConnection != b)
			changed = true;

        	mAllowTunnelConnection = b;
	}

	if (changed)
	{
        	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}
}

bool p3ConnectMgr::getTunnelConnection()
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
	return mAllowTunnelConnection;
}

void p3ConnectMgr::setOwnNetConfig(uint32_t netMode, uint32_t visState)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
	/* only change TRY flags */

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::setOwnNetConfig()" << std::endl;
	std::cerr << "Existing netMode: " << mOwnState.netMode << " vis: " << mOwnState.visState;
	std::cerr << std::endl;
	std::cerr << "Input netMode: " << netMode << " vis: " << visState;
	std::cerr << std::endl;
#endif
	mOwnState.netMode &= ~(RS_NET_MODE_TRYMODE);

#ifdef CONN_DEBUG
	std::cerr << "After Clear netMode: " << mOwnState.netMode << " vis: " << mOwnState.visState;
	std::cerr << std::endl;
#endif

	switch(netMode & RS_NET_MODE_ACTUAL)
	{
		case RS_NET_MODE_EXT:
			mOwnState.netMode |= RS_NET_MODE_TRY_EXT;
			break;
		case RS_NET_MODE_UPNP:
			mOwnState.netMode |= RS_NET_MODE_TRY_UPNP;
			break;
		default:
		case RS_NET_MODE_UDP:
			mOwnState.netMode |= RS_NET_MODE_TRY_UDP;
			break;
	}

	mOwnState.visState = visState;

#ifdef CONN_DEBUG
	std::cerr << "Final netMode: " << mOwnState.netMode << " vis: " << mOwnState.visState;
	std::cerr << std::endl;
#endif

	/* if we've started up - then tweak Dht On/Off */
	if (mNetStatus != RS_NET_UNKNOWN)
	{
                enableNetAssistConnect(!(mOwnState.visState & RS_VIS_STATE_NODHT));
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

/* Called to reseet the whole network stack. this call is 
 * triggered by udp stun address tracking.
 *
 * must:
 * 	- reset UPnP and DHT.
 * 	- 
 */

void p3ConnectMgr::netReset()
{
	//don't do a net reset if the MIN_TIME_BETWEEN_NET_RESET is not reached
#if 0
	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
		time_t delta = time(NULL) - mNetInitTS;
		#ifdef CONN_DEBUG_RESET
			std::cerr << "p3ConnectMgr time since last reset : " << delta << std::endl;
		#endif
		if (delta < (time_t)MIN_TIME_BETWEEN_NET_RESET) 
		{
		    mNetStatus = RS_NET_NEEDS_RESET;
		    #ifdef CONN_DEBUG_RESET
			    std::cerr << "p3ConnectMgr::netStartup() don't do a net reset if the MIN_TIME_BETWEEN_NET_RESET is not reached" << std::endl;
		    #endif
		    return;
		}
	}
#endif

#ifdef CONN_DEBUG_RESET
	std::cerr << "p3ConnectMgr::netReset() Called" << std::endl;
#endif

	shutdown(); /* blocking shutdown call */

	// Will initiate a new call for determining the external ip.
	if (mUseExtAddrFinder)
	{
#ifdef CONN_DEBUG_RESET
		std::cerr << "p3ConnectMgr::netReset() restarting AddrFinder" << std::endl;
#endif
		mExtAddrFinder->reset() ;
	}
	else
	{
#ifdef CONN_DEBUG_RESET
		std::cerr << "p3ConnectMgr::netReset() ExtAddrFinder Disabled" << std::endl;
#endif
	}

#ifdef CONN_DEBUG_RESET
	std::cerr << "p3ConnectMgr::netReset() resetting NetStatus" << std::endl;
#endif

	/* reset udp network - handled by tou_init! */
	/* reset tcp network - if necessary */
	{
		/* NOTE: nNetListeners should be protected via the Mutex.
		* HOWEVER, as we NEVER change this list - once its setup
		* we can get away without it - and assume its constant.
		* 
		* NB: (*it)->reset_listener must be out of the mutex, 
		*      as it calls back to p3ConnMgr.
		*/

		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		struct sockaddr_in iaddr = mOwnState.currentlocaladdr;
		
#ifdef CONN_DEBUG_RESET
		std::cerr << "p3ConnectMgr::netReset() resetting listeners" << std::endl;
#endif
		std::list<pqiNetListener *>::const_iterator it;
		for(it = mNetListeners.begin(); it != mNetListeners.end(); it++)
		{
			(*it)->resetListener(iaddr);
#ifdef CONN_DEBUG_RESET
			std::cerr << "p3ConnectMgr::netReset() reset listener" << std::endl;
#endif
		}
	}

	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
        	mNetStatus = RS_NET_UNKNOWN;
		netStatusReset_locked();
	}

	/* check Network Address. This happens later */
	//checkNetAddress();

#ifdef CONN_DEBUG_RESET
	std::cerr << "p3ConnectMgr::netReset() done" << std::endl;
#endif
}


/* to allow resets of network stuff */
void    p3ConnectMgr::addNetListener(pqiNetListener *listener)
{
        RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
        mNetListeners.push_back(listener);
}

void p3ConnectMgr::netStatusReset_locked()
{
	//std::cerr << "p3ConnectMgr::netStatusReset()" << std::endl;;

	mNetFlags = pqiNetStatus();
	//oldNetFlags = pqiNetStatus();
	//IndicateConfigChanged();
}

void p3ConnectMgr::netStartup()
{
	/* startup stuff */

	/* StunInit gets a list of peers, and asks the DHT to find them...
	 * This is needed for all systems so startup straight away 
	 */
#ifdef CONN_DEBUG_RESET
	std::cerr << "p3ConnectMgr::netStartup()" << std::endl;
#endif

        netDhtInit(); 
	netUdpInit();

	/* decide which net setup mode we're going into 
	 */


	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	mNetInitTS = time(NULL);
	netStatusReset_locked();

#ifdef CONN_DEBUG_RESET
	std::cerr << "p3ConnectMgr::netStartup() resetting mNetInitTS / Status" << std::endl;
#endif
	mOwnState.netMode &= ~(RS_NET_MODE_ACTUAL);

	switch(mOwnState.netMode & RS_NET_MODE_TRYMODE)
	{

		case RS_NET_MODE_TRY_EXT:  /* v similar to UDP */
#ifdef CONN_DEBUG_RESET
			std::cerr << "p3ConnectMgr::netStartup() TRY_EXT mode";
			std::cerr << std::endl;
#endif
			mOwnState.netMode |= RS_NET_MODE_EXT;
			mNetStatus = RS_NET_EXT_SETUP;
			break;

		case RS_NET_MODE_TRY_UDP:
#ifdef CONN_DEBUG_RESET
			std::cerr << "p3ConnectMgr::netStartup() TRY_UDP mode";
			std::cerr << std::endl;
#endif
			mOwnState.netMode |= RS_NET_MODE_UDP;
			mNetStatus = RS_NET_EXT_SETUP;
			break;

		default: // Fall through.

#ifdef CONN_DEBUG_RESET
			std::cerr << "p3ConnectMgr::netStartup() UNKNOWN mode";
			std::cerr << std::endl;
#endif

		case RS_NET_MODE_TRY_UPNP:
#ifdef CONN_DEBUG_RESET
			std::cerr << "p3ConnectMgr::netStartup() TRY_UPNP mode";
			std::cerr << std::endl;
#endif
			/* Force it here (could be default!) */
			mOwnState.netMode |= RS_NET_MODE_TRY_UPNP;
			mOwnState.netMode |= RS_NET_MODE_UDP;      /* set to UDP, upgraded is UPnP is Okay */
			mNetStatus = RS_NET_UPNP_INIT;
			break;
	}
}


void p3ConnectMgr::tick()
{
	netTick();
	statusTick();
	tickMonitors();

	static time_t last_friends_check = time(NULL) ;
	static const time_t INTERVAL_BETWEEN_LOCATION_CLEANING = 600 ; // Remove unused locations every 10 minutes.

	time_t now = time(NULL) ;

	if(now > last_friends_check + INTERVAL_BETWEEN_LOCATION_CLEANING && rsPeers != NULL)
	{
		std::cerr << "p3ConnectMgr::tick(): cleaning unused locations." << std::endl ;

		rsPeers->cleanUnusedLocations() ;
		last_friends_check = now ;
	}
}

bool p3ConnectMgr::shutdown() /* blocking shutdown call */
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::shutdown()";
	std::cerr << std::endl;
#endif
	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
		mNetStatus = RS_NET_UNKNOWN;
		mNetInitTS = time(NULL);
		netStatusReset_locked();
	}
	netAssistFirewallShutdown();
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

#ifdef CONN_DEBUG_TICK
	std::cerr << "p3ConnectMgr::statusTick()" << std::endl;
#endif
	std::list<std::string> retryIds;
	std::list<std::string>::iterator it2;
        //std::list<std::string> dummyToRemove;

      {
	time_t now = time(NULL);
	time_t oldavail = now - MAX_AVAIL_PERIOD;
        time_t retry = now - mRetryPeriod;

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
#ifdef CONN_DEBUG_TICK
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
#ifdef CONN_DEBUG_TICK
		std::cerr << "p3ConnectMgr::statusTick() RETRY TIMEOUT for: ";
		std::cerr << *it2;
		std::cerr << std::endl;
#endif
		/* retry it! */
		retryConnect(*it2);
	}

#endif

}


void p3ConnectMgr::netTick()
{

#ifdef CONN_DEBUG_TICK
	std::cerr << "p3ConnectMgr::netTick()" << std::endl;
#endif

	// Check whether we are stuck on loopback. This happens if RS starts when
	// the computer is not yet connected to the internet. In such a case we
	// periodically check for a local net address.
	//
	checkNetAddress() ;
	networkConsistencyCheck(); /* check consistency. If not consistent, do a reset inside  networkConsistencyCheck() */

	connMtx.lock();   /*   LOCK MUTEX */

	uint32_t netStatus = mNetStatus;
	time_t   age = time(NULL) - mNetInitTS;

	connMtx.unlock(); /* UNLOCK MUTEX */
        /* start tcp network - if necessary */

	switch(netStatus)
	{
		case RS_NET_NEEDS_RESET:

#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
			std::cerr << "p3ConnectMgr::netTick() STATUS: NEEDS_RESET" << std::endl;
#endif
			netReset();
			break;

		case RS_NET_UNKNOWN:
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
			std::cerr << "p3ConnectMgr::netTick() STATUS: UNKNOWN" << std::endl;
#endif

			/* add a small delay to stop restarting straight after a RESET 
			 * This is so can we shutdown cleanly
			 */
#define STARTUP_DELAY 5
			if (age < STARTUP_DELAY)
			{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
				std::cerr << "p3ConnectMgr::netTick() Delaying Startup" << std::endl;
#endif
			}
			else
			{
				netStartup();
			}

			break;

		case RS_NET_UPNP_INIT:
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
			std::cerr << "p3ConnectMgr::netTick() STATUS: UPNP_INIT" << std::endl;
#endif
			netUpnpInit();
			break;

		case RS_NET_UPNP_SETUP:
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
			std::cerr << "p3ConnectMgr::netTick() STATUS: UPNP_SETUP" << std::endl;
#endif
			netUpnpCheck();
			break;


		case RS_NET_EXT_SETUP:
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
			std::cerr << "p3ConnectMgr::netTick() STATUS: EXT_SETUP" << std::endl;
#endif
			netExtCheck();
			//netDhtInit();
			break;

		case RS_NET_DONE:
#ifdef CONN_DEBUG_TICK
			std::cerr << "p3ConnectMgr::netTick() STATUS: DONE" << std::endl;
#endif

			break;

		case RS_NET_LOOPBACK:
                        //don't do a shutdown because a client in a computer without local network might be usefull for debug.
                        //shutdown();
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
                        std::cerr << "p3ConnectMgr::netTick() STATUS: RS_NET_LOOPBACK" << std::endl;
#endif
		default:
			break;
	}

	return;
}


void p3ConnectMgr::netUdpInit()
{
	// All functionality has been moved elsewhere (pqiNetListener interface)
#if 0
#if defined(CONN_DEBUG_RESET)
	std::cerr << "p3ConnectMgr::netUdpInit() Does nothing!" << std::endl;
#endif
	connMtx.lock();   /*   LOCK MUTEX */

	struct sockaddr_in iaddr = mOwnState.currentlocaladdr;

	connMtx.unlock(); /* UNLOCK MUTEX */

	/* udp port now controlled by udpstack (from libbitdht) */
	mUdpStack->resetAddress(iaddr);
	/* open our udp port */
	tou_init((struct sockaddr *) &iaddr, sizeof(iaddr));
#endif

}


void p3ConnectMgr::netDhtInit()
{
#if defined(CONN_DEBUG_RESET)
	std::cerr << "p3ConnectMgr::netDhtInit()" << std::endl;
#endif
	connMtx.lock();   /*   LOCK MUTEX */

	uint32_t vs = mOwnState.visState;

	connMtx.unlock(); /* UNLOCK MUTEX */

        enableNetAssistConnect(!(vs & RS_VIS_STATE_NODHT));
}


void p3ConnectMgr::netUpnpInit()
{
#if defined(CONN_DEBUG_RESET)
	std::cerr << "p3ConnectMgr::netUpnpInit()" << std::endl;
#endif
	uint16_t eport, iport;

	connMtx.lock();   /*   LOCK MUTEX */

	/* get the ports from the configuration */

	mNetStatus = RS_NET_UPNP_SETUP;
	iport = ntohs(mOwnState.currentlocaladdr.sin_port);
	eport = ntohs(mOwnState.currentserveraddr.sin_port);
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

#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
		std::cerr << "p3ConnectMgr::netUpnpCheck() age: " << delta << std::endl;
#endif

	connMtx.unlock(); /* UNLOCK MUTEX */

	struct sockaddr_in extAddr;
	int upnpState = netAssistFirewallActive();

	if (((upnpState == 0) && (delta > (time_t)MAX_UPNP_INIT)) ||
	    ((upnpState > 0) && (delta > (time_t)MAX_UPNP_COMPLETE)))
	{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
		std::cerr << "p3ConnectMgr::netUpnpCheck() ";
		std::cerr << "Upnp Check failed." << std::endl;
#endif
		/* fallback to UDP startup */
		connMtx.lock();   /*   LOCK MUTEX */

		/* UPnP Failed us! */
		mNetStatus = RS_NET_EXT_SETUP;
		mNetFlags.mUpnpOk = false;

		connMtx.unlock(); /* UNLOCK MUTEX */
	}
	else if ((upnpState > 0) && netAssistExtAddress(extAddr))
	{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
		std::cerr << "p3ConnectMgr::netUpnpCheck() ";
		std::cerr << "Upnp Check success state: " << upnpState << std::endl;
#endif
		/* switch to UDP startup */
		connMtx.lock();   /*   LOCK MUTEX */

		/* Set Net Status flags ....
		 * we now have external upnp address. Golden!
		 * don't set netOk flag until have seen some traffic.
		 */
		if (isValidNet(&(extAddr.sin_addr)))
		{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
			std::cerr << "p3ConnectMgr::netUpnpCheck() ";
			std::cerr << "UpnpAddr: " << rs_inet_ntoa(extAddr.sin_addr);
			std::cerr << ":" << ntohs(extAddr.sin_port);
			std::cerr << std::endl;
#endif
			mNetFlags.mUpnpOk = true;
			mNetFlags.mExtAddr = extAddr;
			mNetFlags.mExtAddrOk = true;
			mNetFlags.mExtAddrStableOk = true;

			mNetStatus = RS_NET_EXT_SETUP;
			/* Fix netMode & Clear others! */
			mOwnState.netMode = RS_NET_MODE_TRY_UPNP | RS_NET_MODE_UPNP; 
		}
		connMtx.unlock(); /* UNLOCK MUTEX */
	}
	else
	{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
		std::cerr << "p3ConnectMgr::netUpnpCheck() ";
		std::cerr << "Upnp Check Continues: status: " << upnpState << std::endl;
#endif
	}

}


void p3ConnectMgr::netExtCheck()
{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
	std::cerr << "p3ConnectMgr::netExtCheck()" << std::endl;
#endif
	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
		bool isStable = false;
		struct sockaddr_in tmpip ;

			/* check for External Address */
		/* in order of importance */
		/* (1) UPnP -> which handles itself */
		if (!mNetFlags.mExtAddrOk)
		{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
			std::cerr << "p3ConnectMgr::netExtCheck() Ext Not Ok" << std::endl;
#endif

			/* net Assist */
			if (netAssistExtAddress(tmpip))
			{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
				std::cerr << "p3ConnectMgr::netExtCheck() Ext supplied from netAssistExternalAddress()" << std::endl;
#endif
				if (isValidNet(&(tmpip.sin_addr)))
				{
					// must be stable???
					isStable = true;
					mNetFlags.mExtAddr = tmpip;
					mNetFlags.mExtAddrOk = true;
					mNetFlags.mExtAddrStableOk = isStable;
				}	
				else
				{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
					std::cerr << "p3ConnectMgr::netExtCheck() Bad Address supplied from netAssistExternalAddress()" << std::endl;
#endif
				}
			}

		}

		/* otherwise ask ExtAddrFinder */
		if (!mNetFlags.mExtAddrOk)
		{
			/* ExtAddrFinder */
			if (mUseExtAddrFinder)
			{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
				std::cerr << "p3ConnectMgr::netExtCheck() checking ExtAddrFinder" << std::endl;
#endif
				bool extFinderOk = mExtAddrFinder->hasValidIP(&(tmpip.sin_addr));
				if (extFinderOk)
				{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
					std::cerr << "p3ConnectMgr::netExtCheck() Ext supplied by ExtAddrFinder" << std::endl;
#endif
					/* best guess at port */
					tmpip.sin_port = mNetFlags.mLocalAddr.sin_port;
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
					std::cerr << "p3ConnectMgr::netExtCheck() ";
					std::cerr << "ExtAddr: " << rs_inet_ntoa(tmpip.sin_addr);
					std::cerr << ":" << ntohs(tmpip.sin_port);
					std::cerr << std::endl;
#endif

					mNetFlags.mExtAddr = tmpip;
					mNetFlags.mExtAddrOk = true;
					mNetFlags.mExtAddrStableOk = isStable;

					/* XXX HACK TO FIX */
#warning "ALLOWING ExtAddrFinder -> ExtAddrStableOk = true (which it is not normally)"
					mNetFlags.mExtAddrStableOk = true;

				}
			}
		}
				
		/* any other sources ??? */
		
		/* finalise address */
		if (mNetFlags.mExtAddrOk)
		{

#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
			std::cerr << "p3ConnectMgr::netExtCheck() ";
			std::cerr << "ExtAddr: " << rs_inet_ntoa(mNetFlags.mExtAddr.sin_addr);
			std::cerr << ":" << ntohs(mNetFlags.mExtAddr.sin_port);
			std::cerr << std::endl;
#endif
			//update ip address list
			mOwnState.currentserveraddr = mNetFlags.mExtAddr;

			pqiIpAddress addrInfo;
			addrInfo.mAddr = mNetFlags.mExtAddr;
			addrInfo.mSeenTime = time(NULL);
			addrInfo.mSrc = 0;
			mOwnState.ipAddrs.mExt.updateIpAddressList(addrInfo);

			mNetStatus = RS_NET_DONE;
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
			std::cerr << "p3ConnectMgr::netExtCheck() Ext Ok: RS_NET_DONE" << std::endl;
#endif

			if (!mNetFlags.mExtAddrStableOk)
			{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
				std::cerr << "p3ConnectMgr::netUdpCheck() UDP Unstable :( ";
				std::cerr <<  std::endl;
				std::cerr << "p3ConnectMgr::netUdpCheck() We are unreachable";
				std::cerr <<  std::endl;
				std::cerr << "netMode =>  RS_NET_MODE_UNREACHABLE";
				std::cerr <<  std::endl;
#endif
				mOwnState.netMode &= ~(RS_NET_MODE_ACTUAL);
				mOwnState.netMode |= RS_NET_MODE_UNREACHABLE;

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
			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}

		if (mNetFlags.mExtAddrOk)
		{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
			std::cerr << "p3ConnectMgr::netExtCheck() setting netAssistSetAddress()" << std::endl;
#endif
			netAssistSetAddress(mNetFlags.mLocalAddr, mNetFlags.mExtAddr, mOwnState.netMode);
		}
#if 0
		else
		{
			std::cerr << "p3ConnectMgr::netExtCheck() setting ERR netAssistSetAddress(0)" << std::endl;
			/* mode = 0 for error */
			netAssistSetAddress(mNetFlags.mLocalAddr, mNetFlags.mExtAddr, mOwnState.netMode);
		}
#endif

		/* flag unreachables! */
		if ((mNetFlags.mExtAddrOk) && (!mNetFlags.mExtAddrStableOk))
		{
#if defined(CONN_DEBUG_TICK) || defined(CONN_DEBUG_RESET)
			std::cerr << "p3ConnectMgr::netExtCheck() Ext Unstable - Unreachable Check" << std::endl;
#endif
			connMtx.unlock(); /* UNLOCK MUTEX */
			netUnreachableCheck();
		}
	}
}


void p3ConnectMgr::networkConsistencyCheck()
{
	return;
}

#if 0

void p3ConnectMgr::networkConsistencyCheck()
{
	time_t delta;
#ifdef CONN_DEBUG_TICK
	delta = time(NULL) - mNetInitTS;
	std::cerr << "p3ConnectMgr::networkConsistencyCheck() time since last reset : " << delta << std::endl;
#endif

	bool doNetReset = false;
	//if one of the flag is degrated from true to false during last tick, let's do a reset
#ifdef CONN_DEBUG_TICK
	std::cerr << "p3ConnectMgr::networkConsistencyCheck() net flags : " << std::endl;
	std::cerr << "	oldnetFlagLocalOk : " << oldnetFlagLocalOk << ". netFlagLocalOk : " << netFlagLocalOk << "." << std::endl;
	std::cerr << "	oldnetFlagUpnpOk : " << oldnetFlagUpnpOk << ". netFlagUpnpOk : " << netFlagUpnpOk << "." << std::endl;
	std::cerr << "	oldnetFlagDhtOk : " << oldnetFlagDhtOk << ". netFlagDhtOk : " << netFlagDhtOk << "." << std::endl;
	std::cerr << "	oldnetFlagStunOk : " << oldnetFlagStunOk << ". netFlagStunOk : " << netFlagStunOk << "." << std::endl;
	std::cerr << "	oldnetFlagExtraAddressCheckOk : " << oldnetFlagExtraAddressCheckOk << ". netFlagExtraAddressCheckOk : " << netFlagExtraAddressCheckOk << "." << std::endl;
#endif
	if ( !netFlagLocalOk
			|| (!netFlagUpnpOk && oldnetFlagUpnpOk)
			|| (!netFlagDhtOk && oldnetFlagDhtOk)
			|| (!netFlagStunOk && oldnetFlagStunOk)
			|| (!netFlagExtraAddressCheckOk && oldnetFlagExtraAddressCheckOk)
		) {
#ifdef CONN_DEBUG_TICK
		std::cerr << "p3ConnectMgr::networkConsistencyCheck() A net flag went down." << std::endl;
#endif

		//don't do a normal shutdown for upnp as it might hang up.
		//With a 0 port it will just dereference and not attemps to communicate for shutting down upnp session.
		netAssistFirewallPorts(0, 0);

		doNetReset = true;
	}

	bool haveReliableIP = false ;

	{
		RsStackMutex stck(connMtx) ;

		//storing old flags
		mOldNetFlags = mNetFlags;

		if (!doNetReset) 
		{	// Set an external address. if ip adresses are different, let's use the stun address, then 
			// the extaddrfinder and then the upnp address.
			//
			struct sockaddr_in extAddr;

			if (!mOwnState.dyndns.empty () && getIPAddressFromString (mOwnState.dyndns.c_str (), &extAddr.sin_addr))
			{
#ifdef CONN_DEBUG_TICK
				std::cerr << "p3ConnectMgr::networkConsistencyCheck() using getIPAddressFromString for mOwnState.serveraddr." << std::endl;
#endif
				extAddr.sin_port = mOwnState.currentserveraddr.sin_port;
				mOwnState.currentserveraddr = extAddr;
				haveReliableIP = true ;
			}		
			else if (getExtFinderExtAddress(extAddr)) 
			{
				netExtFinderAddressCheck(); //so we put the extra address flag ok.
#ifdef CONN_DEBUG_TICK
				std::cerr << "p3ConnectMgr::networkConsistencyCheck() using getExtFinderExtAddress for mOwnState.serveraddr." << std::endl;
#endif
				mOwnState.currentserveraddr = extAddr;
				haveReliableIP = true ;
			}
			else if (getUpnpExtAddress(extAddr)) 
			{
#ifdef CONN_DEBUG_TICK
				std::cerr << "p3ConnectMgr::networkConsistencyCheck() using getUpnpExtAddress for mOwnState.serveraddr." << std::endl;
#endif
				mOwnState.currentserveraddr = extAddr;
				haveReliableIP = true ;
			} 
			else 
			{
				//try to extract ext address from our own ip address list
				IpAddressTimed extractedAddress;

				if (peerConnectState::extractExtAddress(mOwnState.getIpAddressList(), extractedAddress)) 
					mOwnState.currentserveraddr = extractedAddress.ipAddr;

				//check if a peer is connected, if yes don't do a net reset
				bool is_connected = false;
				std::map<std::string, peerConnectState>::iterator it;
				for(it = mFriendList.begin(); it != mFriendList.end() && !is_connected; it++)
				{
					/* get last contact detail */
					is_connected = it->second.state & RS_PEER_S_CONNECTED;
				}
#ifdef CONN_DEBUG_TICK
				if (is_connected) 
					std::cerr << "p3ConnectMgr::networkConsistencyCheck() not doing a net reset because a peer is connected." << std::endl;
				else 
					std::cerr << "p3ConnectMgr::networkConsistencyCheck() no peer is connected." << std::endl;
#endif
				doNetReset = !is_connected;
			}
		}
	} /* UNLOCK MUTEX */

	if (haveReliableIP)
	{
		//extAddr found,update ip address A list
		pqiIpAddress addr;
		addr.mAddr = mOwnState.currentserveraddr;
		addr.mSeenTime = time(NULL);
		addr.mSrc = OWN_ADDRESS;

		mOwnState.ipAddrs.updateExtAddrs(addr);
	}

	//let's do a net reset
	if (doNetReset) 
	{
		//don't do a reset it if the network init is not finished
		delta = time(NULL) - mNetInitTS;
		if (delta > MAX_NETWORK_INIT) {
#ifdef CONN_DEBUG_TICK
			std::cerr << "p3ConnectMgr::networkConsistencyCheck() doing a net reset." << std::endl;
#endif
#ifdef CONN_DEBUG_RESET
			std::cerr << "p3ConnectMgr::networkConsistencyCheck() Calling NetReset" << std::endl;
#endif
			netReset();
		} 
		else 
		{
#ifdef CONN_DEBUG_TICK
			std::cerr << "p3ConnectMgr::networkConsistencyCheck() reset delayed : p3ConnectMgr time since last reset : " << delta;
			std::cerr << ". Cannot reset before : " <<  MAX_NETWORK_INIT << " sec" << std::endl;
#endif

#ifdef CONN_DEBUG_RESET
			std::cerr << "p3ConnectMgr::networkConsistencyCheck() Reset PENDING" << std::endl;
#endif
		}
	}
}

#endif

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
		// if (mOwnState.netMode == RS_NET_MODE_UNREACHABLE) // MUST BE TRUE!
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
				std::cerr << "Friend: " << peer.name << " Id: " << peer.id << " State: " << peer.state;
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
						notify->AddPopupMessage(RS_POPUP_CONNECT, peer.id,"", "Online: ");
						notify->AddFeedItem(RS_FEED_ITEM_PEER_CONNECT, peer.id, "", "");
					}
				}
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
				std::cerr << "Other: " << peer.name << " Id: " << peer.id << " State: " << peer.state;
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
		std::cerr << "Sending to " << clients.size() << " monitorClients" << std::endl;
#endif
	
		/* send to all monitors */
		std::list<pqiMonitor *>::iterator mit;
		for(mit = clients.begin(); mit != clients.end(); mit++)
		{
			(*mit)->statusChange(actionList);
		}
	}

#ifdef WINDOWS_SYS
///////////////////////////////////////////////////////////
// hack for too many connections

	/* notify all monitors */
	std::list<pqiMonitor *>::iterator mit;
	for(mit = clients.begin(); mit != clients.end(); mit++) {
		(*mit)->statusChanged();
	}

///////////////////////////////////////////////////////////
#endif
}


const std::string p3ConnectMgr::getOwnId()
{
                return AuthSSL::getAuthSSL()->OwnId();
}


bool p3ConnectMgr::getOwnNetStatus(peerConnectState &state)
{
        RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
        state = mOwnState;
	return true;
}

bool p3ConnectMgr::isFriend(std::string id)
{
#ifdef CONN_DEBUG
                std::cerr << "p3ConnectMgr::isFriend(" << id << ") called" << std::endl;
#endif
        RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
        bool ret = (mFriendList.end() != mFriendList.find(id));
#ifdef CONN_DEBUG
                std::cerr << "p3ConnectMgr::isFriend(" << id << ") returning : " << ret << std::endl;
#endif
        return ret;
}

bool p3ConnectMgr::isOnline(std::string id)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() != (it = mFriendList.find(id)))
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::isOnline(" << id << ") is Friend, Online: " << (it->second.state & RS_PEER_S_CONNECTED) << std::endl;
#endif
		return (it->second.state & RS_PEER_S_CONNECTED);
	}
	else
	{
#ifdef CONN_DEBUG
                std::cerr << "p3ConnectMgr::isOnline(" << id << ") is Not Friend" << std::endl << "p3ConnectMgr::isOnline() OwnId: " << AuthSSL::getAuthSSL()->OwnId() << std::endl;
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


#if 0
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
#endif


bool p3ConnectMgr::getPeerCount (unsigned int *pnFriendCount, unsigned int *pnOnlineCount, bool ssl)
{
	if (ssl) {
		/* count ssl id's */

		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		if (pnFriendCount) *pnFriendCount = mFriendList.size();
		if (pnOnlineCount) {
			*pnOnlineCount = 0;

			std::map<std::string, peerConnectState>::iterator it;
			for (it = mFriendList.begin(); it != mFriendList.end(); it++) {
				if (it->second.state & RS_PEER_S_CONNECTED) {
					(*pnOnlineCount)++;
				}
			}
		}
	} else {
		/* count gpg id's */

		if (pnFriendCount) *pnFriendCount = 0;
		if (pnOnlineCount) *pnOnlineCount = 0;

		if (pnFriendCount || pnOnlineCount) {
			std::list<std::string> gpgIds;
			if (AuthGPG::getAuthGPG()->getGPGAcceptedList(gpgIds) == false) {
				return false;
			}

			/* add own id */
			gpgIds.push_back(AuthGPG::getAuthGPG()->getGPGOwnId());

			std::list<std::string> gpgOnlineIds = gpgIds;

			RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

			std::list<std::string>::iterator gpgIt;

			/* check ssl id's */
			std::map<std::string, peerConnectState>::iterator it;
			for (it = mFriendList.begin(); it != mFriendList.end(); it++) {
				if (pnFriendCount && gpgIds.size()) {
					gpgIt = std::find(gpgIds.begin(), gpgIds.end(), it->second.gpg_id);
					if (gpgIt != gpgIds.end()) {
						(*pnFriendCount)++;
						gpgIds.erase(gpgIt);
					}
				}

				if (pnOnlineCount && gpgOnlineIds.size()) {
					if (it->second.state & RS_PEER_S_CONNECTED) {
						gpgIt = std::find(gpgOnlineIds.begin(), gpgOnlineIds.end(), it->second.gpg_id);
						if (gpgIt != gpgOnlineIds.end()) {
							(*pnOnlineCount)++;
							gpgOnlineIds.erase(gpgIt);
						}
					}
				}
			}
		}
	}

	return true;
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
		std::cerr << "p3ConnectMgr::connectAttempt() FAILED Not in FriendList! id: " << id << std::endl;
#endif

		return false;
	}

	if (it->second.connAddrs.size() < 1)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::connectAttempt() FAILED No ConnectAddresses id: " << id << std::endl;
#endif
		return false;
        }


        if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef CONN_DEBUG
                std::cerr << "p3ConnectMgr::connectAttempt() Already FLAGGED as connected!!!!"  << std::endl;
                std::cerr << "p3ConnectMgr::connectAttempt() But allowing anyway!!!"  << std::endl;
#endif
         }

        it->second.lastattempt = time(NULL); 
        it->second.inConnAttempt = true;
        it->second.currentConnAddrAttempt = it->second.connAddrs.front();
	it->second.connAddrs.pop_front();

	addr = it->second.currentConnAddrAttempt.addr;
	delay = it->second.currentConnAddrAttempt.delay;
	period = it->second.currentConnAddrAttempt.period;
	type = it->second.currentConnAddrAttempt.type;


#ifdef CONN_DEBUG
        	std::cerr << "p3ConnectMgr::connectAttempt() found an address: id: " << id << std::endl;
		std::cerr << " laddr: " << rs_inet_ntoa(addr.sin_addr) << " lport: " << ntohs(addr.sin_port) << " delay: " << delay << " period: " << period;
		std::cerr << " type: " << type << std::endl;
#endif
        if (addr.sin_addr.s_addr == 0 || addr.sin_port == 0) {
#ifdef CONN_DEBUG
        	std::cerr << "p3ConnectMgr::connectAttempt() WARNING: address or port is null" << std::endl;
        	std::cerr << " type: " << type << std::endl;
#endif
        }

	return true;
}


/****************************
 * Update state,
 * trigger retry if necessary,
 *
 * remove from DHT?
 *
 */

bool p3ConnectMgr::connectResult(std::string id, bool success, uint32_t flags, struct sockaddr_in remote_peer_address)
{
	bool should_netAssistFriend_false = false ;
	bool should_netAssistFriend_true  = false ;
	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		rslog(RSL_WARNING, p3connectzone, "p3ConnectMgr::connectResult() called Connect!: id: " + id);
		if (success) {
			rslog(RSL_WARNING, p3connectzone, "p3ConnectMgr::connectResult() called with SUCCESS.");
		} else {
			rslog(RSL_WARNING, p3connectzone, "p3ConnectMgr::connectResult() called with FAILED.");
		}

		if (id == getOwnId()) {
#ifdef CONN_DEBUG
			rslog(RSL_WARNING, p3connectzone, "p3ConnectMgr::connectResult() Failed, connecting to own id: ");
#endif
			return false;
		}
		/* check for existing */
		std::map<std::string, peerConnectState>::iterator it;
		it = mFriendList.find(id);
		if (it == mFriendList.end())
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::connectResult() Failed, missing Friend " << " id: " << id << std::endl;
#endif
			return false;
		}

		if (success)
		{
			/* update address (should also come through from DISC) */

#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::connectResult() Connect!: id: " << id << std::endl;
			std::cerr << " Success: " << success << " flags: " << flags << std::endl;
#endif

			rslog(RSL_WARNING, p3connectzone, "p3ConnectMgr::connectResult() Success");

			/* change state */
			it->second.state |= RS_PEER_S_CONNECTED;
			it->second.actions |= RS_PEER_CONNECTED;
			it->second.lastcontact = time(NULL);  /* time of connect */
			it->second.connecttype = flags;


			/* only update the peer's address if we were in a connect attempt.
			 * Otherwise, they connected to us, and the address will be a
			 * random port of their outgoing TCP socket
			 *
			 * NB even if we received the connection, the IP address is likely to okay.
			 */

			//used to send back to the peer it's own ext address
			//it->second.currentserveraddr = remote_peer_address;

			if ((it->second.inConnAttempt) &&
					(it->second.currentConnAddrAttempt.addr.sin_addr.s_addr 
					 == remote_peer_address.sin_addr.s_addr) &&
					(it->second.currentConnAddrAttempt.addr.sin_port 
					 == remote_peer_address.sin_port))
			{
				pqiIpAddress raddr;
				raddr.mAddr = remote_peer_address;
				raddr.mSeenTime = time(NULL);
				raddr.mSrc = 0;
				if (isPrivateNet(&(remote_peer_address.sin_addr)))
				{
					it->second.ipAddrs.updateLocalAddrs(raddr);
					it->second.currentlocaladdr = remote_peer_address;
				}
				else
				{
					it->second.ipAddrs.updateExtAddrs(raddr);
					it->second.currentserveraddr = remote_peer_address;
				}
#ifdef CONN_DEBUG
				std::cerr << "p3ConnectMgr::connectResult() adding current peer address in list." << std::endl;
				it->second.ipAddrs.printAddrs(std::cerr);
#endif
			}

			/* remove other attempts */
			it->second.inConnAttempt = false;
			it->second.connAddrs.clear();
			should_netAssistFriend_false = true ;
			mStatusChanged = true;
			return true;
		}
		else
		{
			it->second.inConnAttempt = false;

#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::connectResult() Disconnect/Fail: id: " << id << std::endl;
			std::cerr << " Success: " << success << " flags: " << flags << std::endl;
#endif

			/* if currently connected -> flag as failed */
			if (it->second.state & RS_PEER_S_CONNECTED)
			{
				it->second.state &= (~RS_PEER_S_CONNECTED);
				it->second.actions |= RS_PEER_DISCONNECTED;
				mStatusChanged = true;

				it->second.lastcontact = time(NULL);  /* time of disconnect */

				should_netAssistFriend_true = true ;
			}

			if (it->second.connAddrs.size() >= 1)
			{
				it->second.actions |= RS_PEER_CONNECT_REQ;
				mStatusChanged = true;
			}
		}
	}
	if(should_netAssistFriend_true)
		netAssistFriend(id,true) ;
	if(should_netAssistFriend_false)
		netAssistFriend(id,false) ;

	return true;
}

/******************************** Feedback ......  *********************************
 * From various sources
 */


void    p3ConnectMgr::peerStatus(std::string id, const pqiIpAddrSet &addrs,
                       uint32_t type, uint32_t flags, uint32_t source)
{
	/* HACKED UP FIX ****/

        std::map<std::string, peerConnectState>::iterator it;
	bool isFriend = true;
	bool newAddrs;

	time_t now = time(NULL);

	peerAddrInfo details;
	details.type    = type;
	details.found   = true;
	details.addrs = addrs;
	details.ts      = now;

      {
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	{
		/* Log */
		std::ostringstream out;
		out << "p3ConnectMgr::peerStatus()" << " id: " << id;
		out << " type: " << type << " flags: " << flags;
		out << " source: " << source;
		out << std::endl;
		addrs.printAddrs(out);
		
		rslog(RSL_WARNING, p3connectzone, out.str());
#ifdef CONN_DEBUG
		std::cerr << out.str();
#endif
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
			std::cerr << "p3ConnectMgr::peerStatus() Peer Not Found - Ignore" << std::endl;
#endif
			return;
		}
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() Peer is in mOthersList" << std::endl;
#endif
	}

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::peerStatus() Current Peer State:" << std::endl;
	printConnectState(std::cerr, it->second);
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

		/* if we are recieving these - the dht is definitely up.
		 */
		mNetFlags.mDhtOk = true;
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
	if (mOwnState.netMode & RS_NET_MODE_UDP)
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
	else if (mOwnState.netMode & RS_NET_MODE_UNREACHABLE)
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
		std::cerr << "p3ConnectMgr::peerStatus() NOT FRIEND " << " id: " << id << std::endl;
#endif

		{
			rslog(RSL_WARNING, p3connectzone, "p3ConnectMgr::peerStatus() NO CONNECT (not friend)");
		}
		return;
	}

	/* if already connected -> done */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::peerStatus() PEER ONLINE ALREADY " << " id: " << id << std::endl;
#endif
		{
			/* Log */
			rslog(RSL_WARNING, p3connectzone, "p3ConnectMgr::peerStatus() NO CONNECT (already connected!)");
		}

		return;
	}

	newAddrs = it->second.ipAddrs.updateAddrs(addrs);
      } /****** STACK UNLOCK MUTEX *******/

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::peerStatus()" << " id: " << id;
	std::cerr << " type: " << type << " flags: " << flags;
	std::cerr << " source: " << source << std::endl;
	std::cerr << " addrs: " << std::endl;
	addrs.printAddrs(std::cerr);
	std::cerr << std::endl;

#endif

#ifndef P3CONNMGR_NO_AUTO_CONNECTION 

#ifndef P3CONNMGR_NO_TCP_CONNECTIONS
	if (newAddrs)
	{
		retryConnectTCP(id);
	}
    
#endif  // P3CONNMGR_NO_TCP_CONNECTIONS


#else 
#endif  // P3CONNMGR_NO_AUTO_CONNECTION 

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::peerStatus() Resulting Peer State:" << std::endl;
	printConnectState(std::cerr, it->second);
	std::cerr << std::endl;
#endif

}

void    p3ConnectMgr::peerConnectRequest(std::string id, struct sockaddr_in raddr,
                       							uint32_t source)
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::peerConnectRequest() id: " << id << " raddr: " << rs_inet_ntoa(raddr.sin_addr) << ":" << ntohs(raddr.sin_port);
	std::cerr << " source: " << source << std::endl;
#endif
	{
		/* Log */
		std::ostringstream out;
		out << "p3ConnectMgr::peerConnectRequest() id: " << id << " raddr: " << rs_inet_ntoa(raddr.sin_addr);
		out << ":" << ntohs(raddr.sin_port) << " source: " << source;
		rslog(RSL_WARNING, p3connectzone, out.str());
	}

	/******************** TCP PART *****************************/

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::peerConnectRequest() Try TCP first" << std::endl;
#endif

	if (source == RS_CB_DHT)
	{
		std::cerr << "p3ConnectMgr::peerConnectRequest() source DHT ==> retryConnectUDP()" << std::endl;
		retryConnectUDP(id, raddr);
		return;
	}
	else
	{	// IS THIS USED???
		std::cerr << "p3ConnectMgr::peerConnectRequest() source OTHER ==> retryConnect()" << std::endl;

		retryConnect(id);
		return;
	}
}


/*******************************************************************/
/*******************************************************************/

bool p3ConnectMgr::addFriend(std::string id, std::string gpg_id, uint32_t netMode, uint32_t visState, time_t lastContact)
{
	bool should_netAssistFriend_true = false ;
	bool should_netAssistFriend_false = false ;
	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		//set a new retry period, so the more frinds we have the less we launch conection attempts
		mRetryPeriod = MIN_RETRY_PERIOD + (mFriendList.size() * 2);

		if (id == AuthSSL::getAuthSSL()->OwnId()) {
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::addFriend() cannot add own id as a friend." << std::endl;
#endif
			/* (1) already exists */
			return false;
		}
		/* so four possibilities
		 * (1) already exists as friend -> do nothing.
		 * (2) is in others list -> move over.
		 * (3) is non-existant -> create new one.
		 */

#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::addFriend() " << id << "; gpg_id : " << gpg_id << std::endl;
#endif

		std::map<std::string, peerConnectState>::iterator it;
		if (mFriendList.end() != mFriendList.find(id))
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::addFriend() Already Exists" << std::endl;
#endif
			/* (1) already exists */
			return true;
		}

		//Authentication is now tested at connection time, we don't store the ssl cert anymore
		//
		if (!AuthGPG::getAuthGPG()->isGPGAccepted(gpg_id) &&  gpg_id != AuthGPG::getAuthGPG()->getGPGOwnId())
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::addFriend() gpg is not accepted" << std::endl;
#endif
			/* no auth */
			return false;
		}


		/* check if it is in others */
		if (mOthersList.end() != (it = mOthersList.find(id)))
		{
			/* (2) in mOthersList -> move over */
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::addFriend() Move from Others" << std::endl;
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
				should_netAssistFriend_false = true ;
			}
			else
			{
				should_netAssistFriend_true = true ;
			}

			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}
		else
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::addFriend() Creating New Entry" << std::endl;
#endif

			/* create a new entry */
			peerConnectState pstate;

			pstate.id = id;
			pstate.gpg_id = gpg_id;
			pstate.name = AuthGPG::getAuthGPG()->getGPGName(gpg_id);

			pstate.state = RS_PEER_S_FRIEND;
			pstate.actions = RS_PEER_NEW;
			pstate.visState = visState;
			pstate.netMode = netMode;
			pstate.lastcontact = lastContact;

			/* addr & timestamps -> auto cleared */

			mFriendList[id] = pstate;

			mStatusChanged = true;

			/* expect it to be a standard DHT */
			should_netAssistFriend_true = true ;

			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}
	}
	if(should_netAssistFriend_true)
		netAssistFriend(id,true) ;
	if(should_netAssistFriend_false)
		netAssistFriend(id,false) ;

	return true;
}


bool p3ConnectMgr::removeFriend(std::string id)
{

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::removeFriend() for id : " << id << std::endl;
	std::cerr << "p3ConnectMgr::removeFriend() mFriendList.size() : " << mFriendList.size() << std::endl;
#endif

	netAssistFriend(id, false);

	std::list<std::string> toRemove;

	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		/* move to othersList */
		bool success = false;
		std::map<std::string, peerConnectState>::iterator it;
		//remove ssl and gpg_ids
		for(it = mFriendList.begin(); it != mFriendList.end(); it++)
		{
			if (it->second.id == id || it->second.gpg_id == id) {
#ifdef CONN_DEBUG
				std::cerr << "p3ConnectMgr::removeFriend() friend found in the list." << id << std::endl;
#endif
				peerConnectState peer = it->second;

				toRemove.push_back(it->second.id);

				peer.state &= (~RS_PEER_S_FRIEND);
				peer.state &= (~RS_PEER_S_CONNECTED);
				peer.state &= (~RS_PEER_S_ONLINE);
				peer.actions = RS_PEER_MOVED;
				peer.inConnAttempt = false;
				mOthersList[id] = peer;
				mStatusChanged = true;

				success = true;
			}
		}

		std::list<std::string>::iterator toRemoveIt;
		for(toRemoveIt = toRemove.begin(); toRemoveIt != toRemove.end(); toRemoveIt++) {
			if (mFriendList.end() != (it = mFriendList.find(*toRemoveIt))) {
				mFriendList.erase(it);
			}
		}

#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::removeFriend() new mFriendList.size() : " << mFriendList.size() << std::endl;
#endif
	}

	/* remove id from all groups */
	std::list<std::string> peerIds;
	peerIds.push_back(id);

	assignPeersToGroup("", peerIds, false);

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

        return !toRemove.empty();
}



#if 0
bool p3ConnectMgr::addNeighbour(std::string id)
{

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::addNeighbour() not implemented anymore." << id << std::endl;
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
        if (!AuthSSL::getAuthSSL()->isAuthenticated(id))
	{
		/* no auth */
		return false;
	}

	/* get details from AuthMgr */
        sslcert detail;
        if (!AuthSSL::getAuthSSL()->getCertDetails(id, detail))
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

        return false;
}

#endif

/*******************************************************************/
/*******************************************************************/
       /*************** External Control ****************/
bool   p3ConnectMgr::retryConnect(std::string id)
{
	/* push all available addresses onto the connect addr stack */
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::retryConnect() id: " << id << std::endl;
#endif

#ifndef P3CONNMGR_NO_TCP_CONNECTIONS

	retryConnectTCP(id);

#endif  // P3CONNMGR_NO_TCP_CONNECTIONS

	return true;
}



bool   p3ConnectMgr::retryConnectUDP(std::string id, struct sockaddr_in &rUdpAddr)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* push all available addresses onto the connect addr stack */
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::retryConnectTCP() id: " << id << std::endl;
#endif

        if (id == getOwnId()) {
            #ifdef CONN_DEBUG
            rslog(RSL_WARNING, p3connectzone, "p3ConnectMgr::retryConnectUDP() Failed, connecting to own id: ");
            #endif
            return false;
        }

        /* look up the id */
        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
#ifdef CONN_DEBUG
               std::cerr << "p3ConnectMgr::retryConnectUDP() Peer is not Friend" << std::endl;
#endif
		return false;
	}

	/* if already connected -> done */
        if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectUDP() Peer Already Connected" << std::endl;
#endif
                if (it->second.connecttype & RS_NET_CONN_TUNNEL) {
#ifdef CONN_DEBUG
                    std::cerr << "p3ConnectMgr::retryConnectUDP() Peer Connected through a tunnel connection, let's try a normal connection." << std::endl;
#endif
                } else {
#ifdef CONN_DEBUG
                    	std::cerr << "p3ConnectMgr::retryConnectUDP() Peer Connected no more connection attempts" << std::endl;
#endif
                    return false;
                }
	}

	/* Explicit Request to start the UDP connection */
	if (isValidNet(&(rUdpAddr.sin_addr)))
	{
#ifdef CONN_DEBUG
		std::cerr << "Adding udp connection attempt: ";
		std::cerr << "Addr: " << rs_inet_ntoa(rUdpAddr.sin_addr);
		std::cerr << ":" << ntohs(rUdpAddr.sin_port);
		std::cerr << std::endl;
#endif
		peerConnectAddress pca;
		pca.addr = rUdpAddr;
		pca.type = RS_NET_CONN_UDP_PEER_SYNC;
		pca.delay = P3CONNMGR_UDP_DEFAULT_DELAY;
		pca.ts = time(NULL);
		pca.period = P3CONNMGR_UDP_DEFAULT_PERIOD;

		addAddressIfUnique(it->second.connAddrs, pca);
	}

	/* finish it off */
	return locked_ConnectAttempt_Complete(&(it->second));
}





bool   p3ConnectMgr::retryConnectTCP(std::string id)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* push all available addresses onto the connect addr stack...
	 * with the following exceptions:
   	 *   - check local address, see if it is the same network as us
	     - check address age. don't add old ones
	 */

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::retryConnectTCP() id: " << id << std::endl;
#endif

        if (id == getOwnId()) {
            #ifdef CONN_DEBUG
            rslog(RSL_WARNING, p3connectzone, "p3ConnectMgr::retryConnectTCP() Failed, connecting to own id: ");
            #endif
            return false;
        }

        /* look up the id */
        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
#ifdef CONN_DEBUG
               std::cerr << "p3ConnectMgr::retryConnectTCP() Peer is not Friend" << std::endl;
#endif
		return false;
	}

	/* if already connected -> done */
        if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::retryConnectTCP() Peer Already Connected" << std::endl;
#endif
                if (it->second.connecttype & RS_NET_CONN_TUNNEL) {
#ifdef CONN_DEBUG
                    std::cerr << "p3ConnectMgr::retryConnectTCP() Peer Connected through a tunnel connection, let's try a normal connection." << std::endl;
#endif
                } else {
#ifdef CONN_DEBUG
                    	std::cerr << "p3ConnectMgr::retryConnectTCP() Peer Connected no more connection attempts" << std::endl;
#endif
                    return false;
                }
	}

	/* UDP automatically searches -> no need to push start */

	locked_ConnectAttempt_CurrentAddresses(&(it->second));
	locked_ConnectAttempt_HistoricalAddresses(&(it->second));
	locked_ConnectAttempt_AddDynDNS(&(it->second));
	locked_ConnectAttempt_AddTunnel(&(it->second));

	/* finish it off */
	return locked_ConnectAttempt_Complete(&(it->second));
}


#define MAX_TCP_ADDR_AGE	(3600 * 24 * 14) // two weeks in seconds.

bool  p3ConnectMgr::locked_CheckPotentialAddr(struct sockaddr_in *addr, time_t age)
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::locked_CheckPotentialAddr("; 
	std::cerr << rs_inet_ntoa(addr->sin_addr);
	std::cerr << ":" << ntohs(addr->sin_port);
	std::cerr << ", " << age << ")";
	std::cerr << std::endl;
#endif

	/*
	 * if it is old - quick rejection 
	 */
	if (age > MAX_TCP_ADDR_AGE)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::locked_CheckPotentialAddr() REJECTING - TOO OLD"; 
		std::cerr << std::endl;
#endif
		return false;
	}

	bool isValid = isValidNet(&(addr->sin_addr));
//	bool isLoopback = isLoopbackNet(&(addr->sin_addr));
//	bool isPrivate = isPrivateNet(&(addr->sin_addr));
	bool isExternal = isExternalNet(&(addr->sin_addr));

	/* if invalid - quick rejection */
	if (!isValid)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::locked_CheckPotentialAddr() REJECTING - INVALID";
		std::cerr << std::endl;
#endif
		return false;
	}

	/* if it is on the ban list - ignore */
	/* checks - is it the dreaded 1.0.0.0 */

	std::list<struct in_addr>::const_iterator it;
	for(it = mBannedIpList.begin(); it != mBannedIpList.end(); it++)
	{
		if (it->s_addr == addr->sin_addr.s_addr)
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::locked_CheckPotentialAddr() REJECTING - ON BANNED IPLIST"; 
			std::cerr << std::endl;
#endif
			return false;
		}
	}


	/* if it is an external address, we'll accept it.
	 * - even it is meant to be a local address.
	 */
	if (isExternal)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::locked_CheckPotentialAddr() ACCEPTING - EXTERNAL"; 
		std::cerr << std::endl;
#endif
		return true;
	}


	/* get here, it is private or loopback 
	 *  - can only connect to these addresses if we are on the same subnet.
	    - check net against our local address.
	 */

	std::cerr << "p3ConnectMgr::locked_CheckPotentialAddr() Checking sameNet against: "; 
	std::cerr << rs_inet_ntoa(mOwnState.currentlocaladdr.sin_addr);
	std::cerr << ")";
	std::cerr << std::endl;

	if (sameNet(&(mOwnState.currentlocaladdr.sin_addr), &(addr->sin_addr)))
	{
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::locked_CheckPotentialAddr() ACCEPTING - PRIVATE & sameNET"; 
		std::cerr << std::endl;
#endif
		return true;
	}

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::locked_CheckPotentialAddr() REJECTING - PRIVATE & !sameNET"; 
	std::cerr << std::endl;
#endif

	/* else it fails */
	return false;

}


void  p3ConnectMgr::locked_ConnectAttempt_CurrentAddresses(peerConnectState *peer)
{
	// Just push all the addresses onto the stack.
	/* try "current addresses" first */
	if (locked_CheckPotentialAddr(&(peer->currentlocaladdr), 0))
	{
#ifdef CONN_DEBUG
		std::cerr << "Adding tcp connection attempt: ";
		std::cerr << "Current Local Addr: " << rs_inet_ntoa(peer->currentlocaladdr.sin_addr);
		std::cerr << ":" << ntohs(peer->currentlocaladdr.sin_port);
		std::cerr << std::endl;
#endif
		peerConnectAddress pca;
		pca.addr = peer->currentlocaladdr;
		pca.type = RS_NET_CONN_TCP_LOCAL;
		pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
		pca.ts = time(NULL);
		pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;

		addAddressIfUnique(peer->connAddrs, pca);
	}

	if (locked_CheckPotentialAddr(&(peer->currentserveraddr), 0))
	{
#ifdef CONN_DEBUG
		std::cerr << "Adding tcp connection attempt: ";
		std::cerr << "Current Ext Addr: " << rs_inet_ntoa(peer->currentserveraddr.sin_addr);
		std::cerr << ":" << ntohs(peer->currentserveraddr.sin_port);
		std::cerr << std::endl;
#endif
		peerConnectAddress pca;
		pca.addr = peer->currentserveraddr;
		pca.type = RS_NET_CONN_TCP_EXTERNAL;
		pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
		pca.ts = time(NULL);
		pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;

		addAddressIfUnique(peer->connAddrs, pca);
	}
}


void  p3ConnectMgr::locked_ConnectAttempt_HistoricalAddresses(peerConnectState *peer)
{
	/* now try historical addresses */
	/* try local addresses first */
	std::list<pqiIpAddress>::iterator ait;
	time_t now = time(NULL);

	for(ait = peer->ipAddrs.mLocal.mAddrs.begin(); 
		ait != peer->ipAddrs.mLocal.mAddrs.end(); ait++)
	{
		if (locked_CheckPotentialAddr(&(ait->mAddr), now - ait->mSeenTime))
		{

#ifdef CONN_DEBUG
			std::cerr << "Adding tcp connection attempt: ";
			std::cerr << "Local Addr: " << rs_inet_ntoa(ait->mAddr.sin_addr);
			std::cerr << ":" << ntohs(ait->mAddr.sin_port);
			std::cerr << std::endl;
#endif

			peerConnectAddress pca;
			pca.addr = ait->mAddr;
			pca.type = RS_NET_CONN_TCP_LOCAL;
			pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
			pca.ts = time(NULL);
			pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;
	
			addAddressIfUnique(peer->connAddrs, pca);
		}
	}

	for(ait = peer->ipAddrs.mExt.mAddrs.begin(); 
		ait != peer->ipAddrs.mExt.mAddrs.end(); ait++)
	{
		if (locked_CheckPotentialAddr(&(ait->mAddr), now - ait->mSeenTime))
		{
	
#ifdef CONN_DEBUG
			std::cerr << "Adding tcp connection attempt: ";
			std::cerr << "Ext Addr: " << rs_inet_ntoa(ait->mAddr.sin_addr);
			std::cerr << ":" << ntohs(ait->mAddr.sin_port);
			std::cerr << std::endl;
#endif
			peerConnectAddress pca;
			pca.addr = ait->mAddr;
			pca.type = RS_NET_CONN_TCP_EXTERNAL;
			pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
			pca.ts = time(NULL);
			pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;
	
			addAddressIfUnique(peer->connAddrs, pca);
		}
	}
}


void  p3ConnectMgr::locked_ConnectAttempt_AddDynDNS(peerConnectState *peer)
{
	/* try dyndns address too */
        if (!peer->dyndns.empty()) {
            struct in_addr addr;
            u_short port = peer->currentserveraddr.sin_port ? peer->currentserveraddr.sin_port : peer->currentlocaladdr.sin_port;
#ifdef CONN_DEBUG
            std::cerr << "Looking up DynDNS address" << std::endl;
#endif
            if (port) {
                if (getIPAddressFromString (peer->dyndns.c_str (), &addr))
		{
#ifdef CONN_DEBUG
			std::cerr << "Adding tcp connection attempt: ";
			std::cerr << "DynDNS Addr: " << rs_inet_ntoa(addr);
			std::cerr << ":" << ntohs(port);
			std::cerr << std::endl;
#endif
                        peerConnectAddress pca;
                        pca.addr.sin_family = AF_INET;
                        pca.addr.sin_addr.s_addr = addr.s_addr;
                        pca.addr.sin_port = port;
                        pca.type = RS_NET_CONN_TCP_EXTERNAL;
                        //for the delay, we add a random time and some more time when the friend list is big
                        pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
                        pca.ts = time(NULL);
                        pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;

			/* check address validity */
			if (locked_CheckPotentialAddr(&(pca.addr), 0))
			{
				addAddressIfUnique(peer->connAddrs, pca);
			}
                }
            }
        }
}


void  p3ConnectMgr::locked_ConnectAttempt_AddTunnel(peerConnectState *peer)
{
        if (!(peer->state & RS_PEER_S_CONNECTED) && mAllowTunnelConnection)
        {
#ifdef CONN_DEBUG
		std::cerr << "Adding TUNNEL Connection Attempt";
		std::cerr << std::endl;
#endif
            peerConnectAddress pca;
            pca.type = RS_NET_CONN_TUNNEL;
            pca.ts = time(NULL);
            pca.period = 0;

            sockaddr_clear(&pca.addr);

	    addAddressIfUnique(peer->connAddrs, pca);
        }
}


bool  p3ConnectMgr::addAddressIfUnique(std::list<peerConnectAddress> &addrList, peerConnectAddress &pca)
{
	/* iterate through the list, and make sure it isn't already 
	 * in the list 
	 */

	std::list<peerConnectAddress>::iterator it;
	for(it = addrList.begin(); it != addrList.end(); it++)
	{
		if ((pca.addr.sin_addr.s_addr == it->addr.sin_addr.s_addr)	&&
			(pca.addr.sin_port == it->addr.sin_port)	&&
			(pca.type == it->type))
		{
			/* already */
			return false;
		}
	}

	addrList.push_back(pca);

	return true;
}



bool  p3ConnectMgr::locked_ConnectAttempt_Complete(peerConnectState *peer)
{

	/* flag as last attempt to prevent loop */
	//add a random perturbation between 0 and 2 sec.
        peer->lastattempt = time(NULL) + rand() % MAX_RANDOM_ATTEMPT_OFFSET; 

        if (peer->inConnAttempt) {
                /*  -> it'll automatically use the addresses we added */
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::locked_ConnectAttempt_Complete() Already in CONNECT ATTEMPT";
		std::cerr << std::endl;
            	std::cerr << "p3ConnectMgr::locked_ConnectAttempt_Complete() Remaining ConnAddr Count: " << peer->connAddrs.size();
	    	std::cerr << std::endl;
#endif
		return true;
	}

	/* start a connection attempt */
        if (peer->connAddrs.size() > 0) 
	{
            #ifdef CONN_DEBUG
            std::ostringstream out;
            out << "p3ConnectMgr::locked_ConnectAttempt_Complete() Started CONNECT ATTEMPT! " ;
	    out << std::endl;
            out << "p3ConnectMgr::locked_ConnectAttempt_Complete() ConnAddr Count: " << peer->connAddrs.size();
            rslog(RSL_DEBUG_ALERT, p3connectzone, out.str());
	    std::cerr << out.str() << std::endl;

            #endif

            peer->actions |= RS_PEER_CONNECT_REQ;
            mStatusChanged = true;
	    return true; 
        } 
	else 
	{
            #ifdef CONN_DEBUG
            std::ostringstream out;
            out << "p3ConnectMgr::locked_ConnectAttempt_Complete() No addr in the connect attempt list. Not suitable for CONNECT ATTEMPT! ";
            rslog(RSL_DEBUG_ALERT, p3connectzone, out.str());
	    std::cerr << out.str() << std::endl;
            #endif
	    return false;
	}
	return false;
}

/**********************************************************************
 **********************************************************************
 ******************** External Setup **********************************
 **********************************************************************
 **********************************************************************/


bool    p3ConnectMgr::setLocalAddress(std::string id, struct sockaddr_in addr)
{
	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		bool changed = false;
		{
			RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
			if (mOwnState.currentlocaladdr.sin_addr.s_addr != addr.sin_addr.s_addr ||
			    mOwnState.currentlocaladdr.sin_port != addr.sin_port)
			{
				changed = true;
			}

			mOwnState.currentlocaladdr = addr;
		}

		if (changed)
		{
			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
			netReset();
			#ifdef CONN_DEBUG_RESET
			std::cerr << "p3ConnectMgr::setLocalAddress() Calling NetReset" << std::endl;
			#endif
		}
		return true;
	}

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
	/* check if it is a friend */
	std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
			#ifdef CONN_DEBUG
					std::cerr << "p3ConnectMgr::setLocalAddress() cannot add addres info : peer id not found in friend list  id: " << id << std::endl;
			#endif
			return false;
		}
	}

	/* "it" points to peer */
	it->second.currentlocaladdr = addr;

#if 0
	//update ip address list
	IpAddressTimed ipAddressTimed;
	ipAddressTimed.ipAddr = addr;
	ipAddressTimed.seenTime = time(NULL);
	it->second.updateIpAddressList(ipAddressTimed);
#endif

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return true;
}

bool    p3ConnectMgr::setExtAddress(std::string id, struct sockaddr_in addr)
{
	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
		mOwnState.currentserveraddr = addr;
		return true;
	}

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
	/* check if it is a friend */
	std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::setLocalAddress() cannot add addres info : peer id not found in friend list  id: " << id << std::endl;
#endif
			return false;
		}
	}

	/* "it" points to peer */
	it->second.currentserveraddr = addr;

#if 0
	//update ip address list
	IpAddressTimed ipAddressTimed;
	ipAddressTimed.ipAddr = addr;
	ipAddressTimed.seenTime = time(NULL);
	it->second.updateIpAddressList(ipAddressTimed);
#endif

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return true;
}


bool p3ConnectMgr::setDynDNS(std::string id, std::string dyndns)
{
    if (id == AuthSSL::getAuthSSL()->OwnId())
    {
        RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
        mOwnState.dyndns = dyndns;
        return true;
    }

    RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
    /* check if it is a friend */
    std::map<std::string, peerConnectState>::iterator it;
    if (mFriendList.end() == (it = mFriendList.find(id)))
    {
            if (mOthersList.end() == (it = mOthersList.find(id)))
            {
                    #ifdef CONN_DEBUG
                                    std::cerr << "p3ConnectMgr::setDynDNS() cannot add dyn dns info : peer id not found in friend list  id: " << id << std::endl;
                    #endif
                    return false;
            }
    }

    /* "it" points to peer */
    it->second.dyndns = dyndns;

    IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

    return true;
}

bool    p3ConnectMgr::updateAddressList(const std::string& id, const pqiIpAddrSet &addrs)
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::setAddressList() called for id : " << id << std::endl;
#endif

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* check if it is our own ip */
	if (id == getOwnId()) 
	{
		mOwnState.ipAddrs.updateAddrs(addrs);
		return true;
	}

	/* check if it is a friend */
	std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
            if (mOthersList.end() == (it = mOthersList.find(id)))
            {
#ifdef CONN_DEBUG
				std::cerr << "p3ConnectMgr::setLocalAddress() cannot add addres info : peer id not found in friend list. id: " << id << std::endl;
#endif
                    return false;
            }
	}

	/* "it" points to peer */
	it->second.ipAddrs.updateAddrs(addrs);
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::setLocalAddress() Updated Address for: " << id;
	std::cerr << std::endl;
	it->second.ipAddrs.printAddrs(std::cerr);
	std::cerr << std::endl;
#endif

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return true;
}

bool    p3ConnectMgr::setNetworkMode(std::string id, uint32_t netMode)
{
	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		uint32_t visState;
		uint32_t oldNetMode;
		{
			RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
			visState = mOwnState.visState;
			oldNetMode = mOwnState.netMode;
		}

		setOwnNetConfig(netMode, visState);

		if ((netMode & RS_NET_MODE_ACTUAL) != (oldNetMode & RS_NET_MODE_ACTUAL)) {
			#ifdef CONN_DEBUG_RESET
			std::cerr << "p3ConnectMgr::setNetworkMode() Calling NetReset" << std::endl;
			#endif
			netReset();
		}
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

bool    p3ConnectMgr::setLocation(std::string id, std::string location)
{
        RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

#ifdef CONN_DEBUG
        std::cerr << "p3ConnectMgr::setLocation() called for id : " << id << "; with location " << location << std::endl;
#endif
        if (id == AuthSSL::getAuthSSL()->OwnId())
        {
                mOwnState.location = location;
                return true;
        }

        /* check if it is a friend */
        std::map<std::string, peerConnectState>::iterator it;
        if (mFriendList.end() == (it = mFriendList.find(id))) {
            return false;
        } else {
            it->second.location = location;
            return true;
        }
}

bool    p3ConnectMgr::setVisState(std::string id, uint32_t visState)
{
	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		uint32_t netMode;
		{
			RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
			netMode = mOwnState.netMode;
		}
		setOwnNetConfig(netMode, visState);
		return true;
	}

	bool dht_state ;
	bool isFriend = false;
	{
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
		else
		{
			isFriend = true;
		}

		/* "it" points to peer */
		it->second.visState = visState;
		dht_state = it->second.visState & RS_VIS_STATE_NODHT ;
	}
	if(isFriend)
	{
		/* toggle DHT state */
		if(dht_state)
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
	bool addrChanged = false;
	bool validAddr = false;
	
	struct in_addr prefAddr;
	struct sockaddr_in oldAddr;

	validAddr = getPreferredInterface(prefAddr);

	/* if we don't have a valid address - reset */
	if (!validAddr)
	{
#ifdef CONN_DEBUG_RESET
		std::cerr << "p3ConnectMgr::checkNetAddress() no Valid Network Address, resetting network." << std::endl;
		std::cerr << std::endl;
#endif
		netReset();
		IndicateConfigChanged();
		return false;
	}
	
	
	/* check addresses */
	
	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
		
		oldAddr = mOwnState.currentlocaladdr;
		addrChanged = (prefAddr.s_addr != mOwnState.currentlocaladdr.sin_addr.s_addr);

#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::checkNetAddress()";
		std::cerr << std::endl;
		std::cerr << "Current Local: " << rs_inet_ntoa(mOwnState.currentlocaladdr.sin_addr);
		std::cerr << ":" << ntohs(mOwnState.currentlocaladdr.sin_port);
		std::cerr << std::endl;
		std::cerr << "Current Preferred: " << rs_inet_ntoa(prefAddr);
		std::cerr << std::endl;
#endif
		
#ifdef CONN_DEBUG_RESET
		if (addrChanged)
		{
			std::cerr << "p3ConnectMgr::checkNetAddress() Address Changed!";
			std::cerr << std::endl;
			std::cerr << "Current Local: " << rs_inet_ntoa(mOwnState.currentlocaladdr.sin_addr);
			std::cerr << ":" << ntohs(mOwnState.currentlocaladdr.sin_port);
			std::cerr << std::endl;
			std::cerr << "Current Preferred: " << rs_inet_ntoa(prefAddr);
			std::cerr << std::endl;
		}
#endif
		
		// update address.
		mOwnState.currentlocaladdr.sin_addr = prefAddr;
	
	
		mNetFlags.mLocalAddr = mOwnState.currentlocaladdr;

		if(isLoopbackNet(&(mOwnState.currentlocaladdr.sin_addr)))
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::checkNetAddress() laddr: Loopback" << std::endl;
#endif
			mNetFlags.mLocalAddrOk = false;
			mNetStatus = RS_NET_LOOPBACK;
		}
		else if (!isValidNet(&mOwnState.currentlocaladdr.sin_addr))
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::checkNetAddress() laddr: invalid" << std::endl;
#endif
			mNetFlags.mLocalAddrOk = false;
		}
		else
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::checkNetAddress() laddr okay" << std::endl;
#endif
			mNetFlags.mLocalAddrOk = true;
		}


		int port = ntohs(mOwnState.currentlocaladdr.sin_port);
		if ((port < PQI_MIN_PORT) || (port > PQI_MAX_PORT))
		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::checkNetAddress() Correcting Port to DEFAULT" << std::endl;
#endif
			mOwnState.currentlocaladdr.sin_port = htons(PQI_DEFAULT_PORT);
			addrChanged = true;
		}

		/* if localaddr = serveraddr, then ensure that the ports
		 * are the same (modify server)... this mismatch can
		 * occur when the local port is changed....
		 */
		if (mOwnState.currentlocaladdr.sin_addr.s_addr == mOwnState.currentserveraddr.sin_addr.s_addr)
		{
			mOwnState.currentserveraddr.sin_port = mOwnState.currentlocaladdr.sin_port;
		}

		// ensure that address family is set, otherwise windows Barfs.
		mOwnState.currentlocaladdr.sin_family = AF_INET;
		mOwnState.currentserveraddr.sin_family = AF_INET;

		//update ip address list
		pqiIpAddress addrInfo;
		addrInfo.mAddr = mOwnState.currentlocaladdr;
		addrInfo.mSeenTime = time(NULL);
		addrInfo.mSrc = 0;
		mOwnState.ipAddrs.mLocal.updateIpAddressList(addrInfo);

#ifdef CONN_DEBUG_TICK
		std::cerr << "p3ConnectMgr::checkNetAddress() Final Local Address: " << rs_inet_ntoa(mOwnState.currentlocaladdr.sin_addr);
		std::cerr << ":" << ntohs(mOwnState.currentlocaladdr.sin_port) << std::endl;
		std::cerr << "p3ConnectMgr::checkNetAddress() Addres History: ";
		std::cerr << std::endl;
		mOwnState.ipAddrs.printAddrs(std::cerr);
		std::cerr << std::endl;
#endif
	}

	if (addrChanged)
	{
#ifdef CONN_DEBUG_RESET
		std::cerr << "p3ConnectMgr::checkNetAddress() local address changed, resetting network." << std::endl;
		std::cerr << std::endl;
#endif
		netReset();
		IndicateConfigChanged();
	}

	return 1;
}



/**********************************************************************
 **********************************************************************
 ******************** p3Config functions ******************************
 **********************************************************************
 **********************************************************************/

        /* Key Functions to be overloaded for Full Configuration */

RsSerialiser *p3ConnectMgr::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsPeerConfigSerialiser());
	rss->addSerialType(new RsGeneralConfigSerialiser()) ;

	return rss;
}


bool p3ConnectMgr::saveList(bool &cleanup, std::list<RsItem *>& saveData)
{
	/* create a list of current peers */
	cleanup = false;

	connMtx.lock(); /****** MUTEX LOCKED *******/ 

	RsPeerNetItem *item = new RsPeerNetItem();
	item->clear();

	item->pid = getOwnId();
        item->gpg_id = mOwnState.gpg_id;
        item->location = mOwnState.location;
	if (mOwnState.netMode & RS_NET_MODE_TRY_EXT)
	{
		item->netMode = RS_NET_MODE_EXT;
	}
	else if (mOwnState.netMode & RS_NET_MODE_TRY_UPNP)
	{
		item->netMode = RS_NET_MODE_UPNP;
	}
	else
	{
		item->netMode = RS_NET_MODE_UDP;
	}

	item->visState = mOwnState.visState;
	item->lastContact = mOwnState.lastcontact;

        item->currentlocaladdr = mOwnState.currentlocaladdr;
        item->currentremoteaddr = mOwnState.currentserveraddr;
        item->dyndns = mOwnState.dyndns;
        mOwnState.ipAddrs.mLocal.loadTlv(item->localAddrList);
        mOwnState.ipAddrs.mExt.loadTlv(item->extAddrList);

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::saveList() Own Config Item:" << std::endl;
	item->print(std::cerr, 10);
	std::cerr << std::endl;
#endif

	saveData.push_back(item);
	saveCleanupList.push_back(item);

	/* iterate through all friends and save */
        std::map<std::string, peerConnectState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		item = new RsPeerNetItem();
		item->clear();

		item->pid = it->first;
                item->gpg_id = (it->second).gpg_id;
                item->location = (it->second).location;
                item->netMode = (it->second).netMode;
		item->visState = (it->second).visState;
		item->lastContact = (it->second).lastcontact;
		item->currentlocaladdr = (it->second).currentlocaladdr;
                item->currentremoteaddr = (it->second).currentserveraddr;
                item->dyndns = (it->second).dyndns;
                (it->second).ipAddrs.mLocal.loadTlv(item->localAddrList);
                (it->second).ipAddrs.mExt.loadTlv(item->extAddrList);

		saveData.push_back(item);
		saveCleanupList.push_back(item);
#ifdef CONN_DEBUG
		std::cerr << "p3ConnectMgr::saveList() Peer Config Item:" << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
#endif
	}

	// Now save config for network digging strategies
	
	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;

	RsTlvKeyValue kv;
	kv.key = "USE_EXTR_IP_FINDER" ;
	kv.value = (mUseExtAddrFinder)?"TRUE":"FALSE" ;
	vitem->tlvkvs.pairs.push_back(kv) ;

        #ifdef CONN_DEBUG
	std::cout << "Pushing item for use_extr_addr_finder = " << mUseExtAddrFinder << std::endl ;
        #endif
	saveData.push_back(vitem);
	saveCleanupList.push_back(vitem);

                // Now save config for network digging strategies

        RsConfigKeyValueSet *vitem2 = new RsConfigKeyValueSet ;

        RsTlvKeyValue kv2;
        kv2.key = "ALLOW_TUNNEL_CONNECTION" ;
        kv2.value = (mAllowTunnelConnection)?"TRUE":"FALSE" ;
        vitem2->tlvkvs.pairs.push_back(kv2) ;

        #ifdef CONN_DEBUG
        std::cout << "Pushing item for allow_tunnel_connection = " << mAllowTunnelConnection << std::endl ;
        #endif
        saveData.push_back(vitem2);
	saveCleanupList.push_back(vitem2);

	/* save groups */

	std::list<RsPeerGroupItem *>::iterator groupIt;
	for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
		saveData.push_back(*groupIt); // no delete
	}

	return true;
}

void    p3ConnectMgr::saveDone()
{
	/* clean up the save List */
	std::list<RsItem *>::iterator it;
	for(it = saveCleanupList.begin(); it != saveCleanupList.end(); it++)
	{
		delete (*it);
	}

	saveCleanupList.clear();

	/* unlock mutex */
	connMtx.unlock(); /****** MUTEX UNLOCKED *******/
}

bool  p3ConnectMgr::loadList(std::list<RsItem *>& load)
{

        if (load.size() == 0) {
            std::cerr << "p3ConnectMgr::loadList() list is empty, it may be a configuration problem."  << std::endl;
            return false;
        }

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::loadList() Item Count: " << load.size() << std::endl;
#endif

	std::string ownId = getOwnId();

	/* load the list of peers */
	std::list<RsItem *>::iterator it;
	for(it = load.begin(); it != load.end(); it++)
	{
		RsPeerNetItem *pitem = dynamic_cast<RsPeerNetItem *>(*it);
		if (pitem)
		{
			if (pitem->pid == ownId)
			{
#ifdef CONN_DEBUG
				std::cerr << "p3ConnectMgr::loadList() Own Config Item:" << std::endl;
				pitem->print(std::cerr, 10);
				std::cerr << std::endl;
#endif
				/* add ownConfig */
                                setOwnNetConfig(pitem->netMode, pitem->visState);
                                mOwnState.gpg_id = AuthGPG::getAuthGPG()->getGPGOwnId();
                                mOwnState.location = AuthSSL::getAuthSSL()->getOwnLocation();
			}
			else
			{
#ifdef CONN_DEBUG
				std::cerr << "p3ConnectMgr::loadList() Peer Config Item:" << std::endl;
				pitem->print(std::cerr, 10);
				std::cerr << std::endl;
#endif
				/* ************* */
                                addFriend(pitem->pid, pitem->gpg_id, pitem->netMode, pitem->visState, pitem->lastContact);
                                setLocation(pitem->pid, pitem->location);
                        }
                        setLocalAddress(pitem->pid, pitem->currentlocaladdr);
                        setExtAddress(pitem->pid, pitem->currentremoteaddr);
                        setDynDNS (pitem->pid, pitem->dyndns);

			/* convert addresses */
			pqiIpAddrSet addrs;
			addrs.mLocal.extractFromTlv(pitem->localAddrList);
			addrs.mExt.extractFromTlv(pitem->extAddrList);
                        updateAddressList(pitem->pid, addrs);

			delete(*it);

			continue;
		}

		RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet *>(*it) ;
		if (vitem)
		{
			RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::loadList() General Variable Config Item:" << std::endl;
			vitem->print(std::cerr, 10);
			std::cerr << std::endl;
#endif
			std::list<RsTlvKeyValue>::iterator kit;
			for(kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); kit++) {
				if(kit->key == "USE_EXTR_IP_FINDER") {
					mUseExtAddrFinder = (kit->value == "TRUE");
					std::cerr << "setting use_extr_addr_finder to " << mUseExtAddrFinder << std::endl ;
				} else if (kit->key == "ALLOW_TUNNEL_CONNECTION") {
					mAllowTunnelConnection = (kit->value == "TRUE");
					std::cerr << "setting allow_tunnel_connection to " << mAllowTunnelConnection << std::endl ;
				}
			}

			delete(*it);

			continue;
		}

		RsPeerGroupItem *gitem = dynamic_cast<RsPeerGroupItem *>(*it) ;
		if (gitem)
		{
			RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::loadList() Peer group item:" << std::endl;
			gitem->print(std::cerr, 10);
			std::cerr << std::endl;
#endif

			groupList.push_back(gitem); // don't delete

			if ((gitem->flag & RS_GROUP_FLAG_STANDARD) == 0) {
				/* calculate group id */
				uint32_t groupId = atoi(gitem->id.c_str());
				if (groupId > lastGroupId) {
					lastGroupId = groupId;
				}
			}

			continue;
		}

		delete (*it);
	}

	{
		/* set missing groupIds */

		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		/* Standard groups */
		const int standardGroupCount = 5;
		const char *standardGroup[standardGroupCount] = { RS_GROUP_ID_FRIENDS, RS_GROUP_ID_FAMILY, RS_GROUP_ID_COWORKERS, RS_GROUP_ID_OTHERS, RS_GROUP_ID_FAVORITES };
		bool foundStandardGroup[standardGroupCount] = { false, false, false, false, false };

		std::list<RsPeerGroupItem *>::iterator groupIt;
		for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
			if ((*groupIt)->flag & RS_GROUP_FLAG_STANDARD) {
				int i;
				for (i = 0; i < standardGroupCount; i++) {
					if ((*groupIt)->id == standardGroup[i]) {
						foundStandardGroup[i] = true;
						break;
					}
				}
				
				if (i >= standardGroupCount) {
					/* No more a standard group, remove the flag standard */
					(*groupIt)->flag &= ~RS_GROUP_FLAG_STANDARD;
				}
			} else {
				uint32_t groupId = atoi((*groupIt)->id.c_str());
				if (groupId == 0) {
					std::ostringstream out;
					out << (lastGroupId++);
					(*groupIt)->id = out.str();
				}
			}
		}
		
		/* Initialize standard groups */
		for (int i = 0; i < standardGroupCount; i++) {
			if (foundStandardGroup[i] == false) {
				RsPeerGroupItem *gitem = new RsPeerGroupItem;
				gitem->id = standardGroup[i];
				gitem->name = standardGroup[i];
				gitem->flag |= RS_GROUP_FLAG_STANDARD;
				groupList.push_back(gitem);
			}
		}
	}

	return true;
}



void  printConnectState(std::ostream &out, peerConnectState &peer)
{

	out << "Friend: " << peer.name << " Id: " << peer.id << " State: " << peer.state;
	if (peer.state & RS_PEER_S_FRIEND)
		out << " S:RS_PEER_S_FRIEND";
	if (peer.state & RS_PEER_S_ONLINE)
		out << " S:RS_PEER_S_ONLINE";
	if (peer.state & RS_PEER_S_CONNECTED)
		out << " S:RS_PEER_S_CONNECTED";
	out << " Actions: " << peer.actions;
	if (peer.actions & RS_PEER_NEW)
		out << " A:RS_PEER_NEW";
	if (peer.actions & RS_PEER_MOVED)
		out << " A:RS_PEER_MOVED";
	if (peer.actions & RS_PEER_CONNECTED)
		out << " A:RS_PEER_CONNECTED";
	if (peer.actions & RS_PEER_DISCONNECTED)
		out << " A:RS_PEER_DISCONNECTED";
	if (peer.actions & RS_PEER_CONNECT_REQ)
		out << " A:RS_PEER_CONNECT_REQ";

	out << std::endl;
	return;
}




/**********************************************************************
 **********************************************************************
 ******************** Interfaces    ***********************************
 **********************************************************************
 **********************************************************************/



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

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::enableNetAssistConnect(" << on << ")";
	std::cerr << std::endl;
#endif

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
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::netAssistConnectEnabled() YES";
			std::cerr << std::endl;
#endif

			return true;
		}
	}

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netAssistConnectEnabled() NO";
	std::cerr << std::endl;
#endif

	return false;
}

bool p3ConnectMgr::netAssistConnectActive()
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		if ((it->second)->getActive())

		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::netAssistConnectActive() ACTIVE";
			std::cerr << std::endl;
#endif

			return true;
		}
	}

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netAssistConnectActive() INACTIVE";
	std::cerr << std::endl;
#endif

	return false;
}

bool p3ConnectMgr::netAssistConnectStats(uint32_t &netsize, uint32_t &localnetsize)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		if (((it->second)->getActive()) && ((it->second)->getNetworkStats(netsize, localnetsize)))

		{
#ifdef CONN_DEBUG
			std::cerr << "p3ConnectMgr::netAssistConnectStats(";
			std::cerr << netsize << ", " << localnetsize << ")";
			std::cerr << std::endl;
#endif

			return true;
		}
	}

#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netAssistConnectStats() INACTIVE";
	std::cerr << std::endl;
#endif

	return false;
}

bool p3ConnectMgr::netAssistConnectShutdown()
{
#ifdef CONN_DEBUG
	std::cerr << "p3ConnectMgr::netAssistConnectShutdown()";
	std::cerr << std::endl;
#endif

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
	std::list<pqiNetAssistConnect*> toFind ;
	std::list<pqiNetAssistConnect*> toDrop ;

	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/
		for(it = mDhts.begin(); it != mDhts.end(); it++)
		{
			if (on)
				toFind.push_back(it->second) ;
			else
				toDrop.push_back(it->second) ;
		}
	}

	for(std::list<pqiNetAssistConnect*>::const_iterator it(toFind.begin());it!=toFind.end();++it)
		(*it)->findPeer(id) ;
	for(std::list<pqiNetAssistConnect*>::const_iterator it(toDrop.begin());it!=toDrop.end();++it)
		(*it)->dropPeer(id) ;

	return true;
}


bool p3ConnectMgr::netAssistSetAddress( struct sockaddr_in &laddr,
					struct sockaddr_in &eaddr,
					uint32_t mode)
{
#if 0
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); it++)
	{
		(it->second)->setExternalInterface(laddr, eaddr, mode);
	}
#endif
	return true;
}

/**********************************************************************
 **********************************************************************
 ******************** Network State ***********************************
 **********************************************************************
 **********************************************************************/

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


void	p3ConnectMgr::getNetStatus(pqiNetStatus &status)
{
	/* cannot lock local stack, then call DHT... as this can cause lock up */
	/* must extract data... then update mNetFlags */

	bool dhtOk = netAssistConnectActive();
	uint32_t netsize, rsnetsize;
	netAssistConnectStats(netsize, rsnetsize);

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	/* quick update of the stuff that can change! */
	mNetFlags.mDhtOk = dhtOk;
	mNetFlags.mDhtNetworkSize = netsize;
	mNetFlags.mDhtRsNetworkSize = rsnetsize;

	status = mNetFlags;
}

/**********************************************************************
 **********************************************************************
 ************************** Groups ************************************
 **********************************************************************
 **********************************************************************/

bool p3ConnectMgr::addGroup(RsGroupInfo &groupInfo)
{
	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		RsPeerGroupItem *groupItem = new RsPeerGroupItem;
		groupItem->set(groupInfo);

		std::ostringstream out;
		out << (++lastGroupId);
		groupItem->id = out.str();

		// remove standard flag
		groupItem->flag &= ~RS_GROUP_FLAG_STANDARD;

		groupItem->PeerId(getOwnId());

		groupList.push_back(groupItem);
	}

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_ADD);

	IndicateConfigChanged();

	return true;
}

bool p3ConnectMgr::editGroup(const std::string &groupId, RsGroupInfo &groupInfo)
{
	if (groupId.empty()) {
		return false;
	}

	bool changed = false;

	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		std::list<RsPeerGroupItem*>::iterator groupIt;
		for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
			if ((*groupIt)->id == groupId) {
				break;
			}
		}

		if (groupIt != groupList.end()) {
			if ((*groupIt)->flag & RS_GROUP_FLAG_STANDARD) {
				// can't edit standard groups
			} else {
				changed = true;
				(*groupIt)->set(groupInfo);
			}
		}
	}

	if (changed) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_MOD);

		IndicateConfigChanged();
	}

	return changed;
}

bool p3ConnectMgr::removeGroup(const std::string &groupId)
{
	if (groupId.empty()) {
		return false;
	}

	bool changed = false;

	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		std::list<RsPeerGroupItem*>::iterator groupIt;
		for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
			if ((*groupIt)->id == groupId) {
				break;
			}
		}

		if (groupIt != groupList.end()) {
			if ((*groupIt)->flag & RS_GROUP_FLAG_STANDARD) {
				// can't remove standard groups
			} else {
				changed = true;
				delete(*groupIt);
				groupList.erase(groupIt);
			}
		}
	}

	if (changed) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_DEL);

		IndicateConfigChanged();
	}

	return changed;
}

bool p3ConnectMgr::getGroupInfo(const std::string &groupId, RsGroupInfo &groupInfo)
{
	if (groupId.empty()) {
		return false;
	}

	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	std::list<RsPeerGroupItem*>::iterator groupIt;
	for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
		if ((*groupIt)->id == groupId) {
			(*groupIt)->get(groupInfo);

			return true;
		}
	}

	return false;
}

bool p3ConnectMgr::getGroupInfoList(std::list<RsGroupInfo> &groupInfoList)
{
	RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

	std::list<RsPeerGroupItem*>::iterator groupIt;
	for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
		RsGroupInfo groupInfo;
		(*groupIt)->get(groupInfo);
		groupInfoList.push_back(groupInfo);
	}

	return true;
}

// groupId == "" && assign == false -> remove from all groups
bool p3ConnectMgr::assignPeersToGroup(const std::string &groupId, const std::list<std::string> &peerIds, bool assign)
{
	if (groupId.empty() && assign == true) {
		return false;
	}

	if (peerIds.empty()) {
		return false;
	}

	bool changed = false;

	{
		RsStackMutex stack(connMtx); /****** STACK LOCK MUTEX *******/

		std::list<RsPeerGroupItem*>::iterator groupIt;
		for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
			if (groupId.empty() || (*groupIt)->id == groupId) {
				RsPeerGroupItem *groupItem = *groupIt;

				std::list<std::string>::const_iterator peerIt;
				for (peerIt = peerIds.begin(); peerIt != peerIds.end(); peerIt++) {
					std::list<std::string>::iterator peerIt1 = std::find(groupItem->peerIds.begin(), groupItem->peerIds.end(), *peerIt);
					if (assign) {
						if (peerIt1 == groupItem->peerIds.end()) {
							groupItem->peerIds.push_back(*peerIt);
							changed = true;
						}
					} else {
						if (peerIt1 != groupItem->peerIds.end()) {
							groupItem->peerIds.erase(peerIt1);
							changed = true;
						}
					}
				}

				if (groupId.empty() == false) {
					break;
				}
			}
		}
	}

	if (changed) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_MOD);

		IndicateConfigChanged();
	}

	return changed;
}
