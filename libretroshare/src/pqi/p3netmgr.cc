/*******************************************************************************
 * libretroshare/src/pqi: p3netmgr.cc                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2007-2011  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2015-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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
#include "util/rstime.h"
#include <vector>

#include "pqi/p3netmgr.h"

#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"

#include "util/rsnet.h"
#include "util/rsrandom.h"
#include "util/rsdebug.h"

#include "util/extaddrfinder.h"
#include "util/dnsresolver.h"


struct RsLog::logInfo p3netmgrzoneInfo = {RsLog::Default, "p3netmgr"};
#define p3netmgrzone &p3netmgrzoneInfo

#include "rsitems/rsconfigitems.h"

#include "retroshare/rsiface.h"
#include "retroshare/rsconfig.h"
#include "retroshare/rsbanlist.h"

/* Network setup States */

const uint32_t RS_NET_NEEDS_RESET = 	0x0000;
const uint32_t RS_NET_UNKNOWN = 	0x0001;
const uint32_t RS_NET_UPNP_INIT = 	0x0002;
const uint32_t RS_NET_UPNP_SETUP =  	0x0003;
const uint32_t RS_NET_EXT_SETUP =  	0x0004;
const uint32_t RS_NET_DONE =    	0x0005;
const uint32_t RS_NET_LOOPBACK =    	0x0006;
//const uint32_t RS_NET_DOWN =    	0x0007;

/* Stun modes (TODO) */
//const uint32_t RS_STUN_DHT =      	0x0001;
//const uint32_t RS_STUN_DONE =      	0x0002;
//const uint32_t RS_STUN_LIST_MIN =      	100;
//const uint32_t RS_STUN_FOUND_MIN =     	10;

const uint32_t MAX_UPNP_INIT = 		60; /* seconds UPnP timeout */
const uint32_t MAX_UPNP_COMPLETE = 	600; /* 10 min... seems to take a while */
//const uint32_t MAX_NETWORK_INIT =	70; /* timeout before network reset */

//const uint32_t MIN_TIME_BETWEEN_NET_RESET = 		5;

/****
 * #define NETMGR_DEBUG 1
 * #define NETMGR_DEBUG_RESET 1
 * #define NETMGR_DEBUG_TICK 1
 * #define NETMGR_DEBUG_STATEBOX 1
 ***/
// #define NETMGR_DEBUG 1
// #define NETMGR_DEBUG_RESET 1
// #define NETMGR_DEBUG_TICK 1
// #define NETMGR_DEBUG_STATEBOX 1

pqiNetStatus::pqiNetStatus() :
    mExtAddrOk(false), mExtAddrStableOk(false), mUpnpOk(false), mDhtOk(false),
    mDhtNetworkSize(0), mDhtRsNetworkSize(0), mResetReq(false)
{
	sockaddr_storage_clear(mLocalAddr);
	sockaddr_storage_clear(mExtAddr);
}



void pqiNetStatus::print(std::ostream &out)
{
	out << "pqiNetStatus: ";
        out << " mExtAddrOk: " << mExtAddrOk;
        out << " mExtAddrStableOk: " << mExtAddrStableOk;
	out << std::endl;
        out << " mUpnpOk: " << mUpnpOk;
        out << " mDhtOk: " << mDhtOk;
        out << " mResetReq: " << mResetReq;
        out << std::endl;
	out << "mDhtNetworkSize: " << mDhtNetworkSize << " mDhtRsNetworkSize: " << mDhtRsNetworkSize;
        out << std::endl;
	out << "mLocalAddr: " << sockaddr_storage_tostring(mLocalAddr) << " ";
	out << "mExtAddr: " << sockaddr_storage_tostring(mExtAddr) << " ";
        out << std::endl;
}


p3NetMgrIMPL::p3NetMgrIMPL() : mPeerMgr(nullptr), mLinkMgr(nullptr),
    mNetMtx("p3NetMgr"), mNetStatus(RS_NET_UNKNOWN), mStatusChanged(false),
    mDoNotNetCheckUntilTs(0)
{

	{
		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/

		mNetMode = RS_NET_MODE_UDP;

		mUseExtAddrFinder = true;
		mExtAddrFinder = new ExtAddrFinder();
		mNetInitTS = 0;
	
		mNetFlags = pqiNetStatus();
		mOldNetFlags = pqiNetStatus();

		mOldNatType = RSNET_NATTYPE_UNKNOWN;
		mOldNatHole = RSNET_NATHOLE_UNKNOWN;
		sockaddr_storage_clear(mLocalAddr);
		sockaddr_storage_clear(mExtAddr);

		// force to IPv4 for the moment.
		mLocalAddr.ss_family = AF_INET;
		mExtAddr.ss_family = AF_INET;

		// default to full.
		mVsDisc = RS_VS_DISC_FULL;
		mVsDht = RS_VS_DHT_FULL;

	}
	
#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgr() Startup" << std::endl;
#endif

	rslog(RSL_WARNING, p3netmgrzone, "p3NetMgr() Startup, resetting network");
	netReset();

	return;
}

void p3NetMgrIMPL::setManagers(p3PeerMgr *peerMgr, p3LinkMgr *linkMgr)
{
	mPeerMgr = peerMgr;
	mLinkMgr = linkMgr;
}

#ifdef RS_USE_DHT_STUNNER
void p3NetMgrIMPL::setAddrAssist(pqiAddrAssist *dhtStun, pqiAddrAssist *proxyStun)
{
	mDhtStunner = dhtStun;
	mProxyStunner = proxyStun;
}
#endif // RS_USE_DHT_STUNNER


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

void p3NetMgrIMPL::netReset()
{
#ifdef NETMGR_DEBUG_RESET
	std::cerr << "p3NetMgrIMPL::netReset() Called" << std::endl;
#endif
	rslog(RSL_ALERT, p3netmgrzone, "p3NetMgr::netReset() Called");

	shutdown(); /* blocking shutdown call */

	// Will initiate a new call for determining the external ip.
	if (mUseExtAddrFinder)
	{
#ifdef NETMGR_DEBUG_RESET
		std::cerr << "p3NetMgrIMPL::netReset() restarting AddrFinder" << std::endl;
#endif
		mExtAddrFinder->reset() ;
	}
	else
	{
#ifdef NETMGR_DEBUG_RESET
		std::cerr << "p3NetMgrIMPL::netReset() ExtAddrFinder Disabled" << std::endl;
#endif
	}

#ifdef NETMGR_DEBUG_RESET
	std::cerr << "p3NetMgrIMPL::netReset() resetting NetStatus" << std::endl;
#endif

	/* reset tcp network - if necessary */
	{
		/* NOTE: nNetListeners should be protected via the Mutex.
		* HOWEVER, as we NEVER change this list - once its setup
		* we can get away without it - and assume its constant.
		* 
		* NB: (*it)->reset_listener must be out of the mutex, 
		*      as it calls back to p3ConnMgr.
		*/

		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/

		struct sockaddr_storage iaddr = mLocalAddr;
		
#ifdef NETMGR_DEBUG_RESET
		std::cerr << "p3NetMgrIMPL::netReset() resetting listeners" << std::endl;
#endif
		std::list<pqiNetListener *>::const_iterator it;
		for(it = mNetListeners.begin(); it != mNetListeners.end(); ++it)
		{
			(*it)->resetListener(iaddr);
#ifdef NETMGR_DEBUG_RESET
			std::cerr << "p3NetMgrIMPL::netReset() reset listener" << std::endl;
#endif
		}
	}

	{
		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
        	mNetStatus = RS_NET_UNKNOWN;
		netStatusReset_locked();
	}

	updateNetStateBox_reset();

#ifdef NETMGR_DEBUG_RESET
	std::cerr << "p3NetMgrIMPL::netReset() done" << std::endl;
#endif
}


void p3NetMgrIMPL::netStatusReset_locked()
{
	//std::cerr << "p3NetMgrIMPL::netStatusReset()" << std::endl;;

	mNetFlags = pqiNetStatus();
}


bool p3NetMgrIMPL::shutdown() /* blocking shutdown call */
{
#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgrIMPL::shutdown()";
	std::cerr << std::endl;
#endif
	{
		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
		mNetStatus = RS_NET_UNKNOWN;
		mNetInitTS = time(NULL);
		netStatusReset_locked();
	}
	netAssistFirewallShutdown();
	netAssistConnectShutdown();

	return true;
}









void p3NetMgrIMPL::netStartup()
{
	/* startup stuff */

	/* StunInit gets a list of peers, and asks the DHT to find them...
	 * This is needed for all systems so startup straight away 
	 */
#ifdef NETMGR_DEBUG_RESET
	std::cerr << "p3NetMgrIMPL::netStartup()" << std::endl;
#endif

        netDhtInit(); 

	/* decide which net setup mode we're going into 
	 */


	RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/

	mNetInitTS = time(NULL);
	netStatusReset_locked();

#ifdef NETMGR_DEBUG_RESET
	std::cerr << "p3NetMgrIMPL::netStartup() resetting mNetInitTS / Status" << std::endl;
#endif
	mNetMode &= ~(RS_NET_MODE_ACTUAL);

	switch(mNetMode & RS_NET_MODE_TRYMODE)
	{

		case RS_NET_MODE_TRY_EXT:  /* v similar to UDP */
#ifdef NETMGR_DEBUG_RESET
			std::cerr << "p3NetMgrIMPL::netStartup() TRY_EXT mode";
			std::cerr << std::endl;
#endif
			mNetMode |= RS_NET_MODE_EXT;
			mNetStatus = RS_NET_EXT_SETUP;
			break;

		case RS_NET_MODE_TRY_UDP:
#ifdef NETMGR_DEBUG_RESET
			std::cerr << "p3NetMgrIMPL::netStartup() TRY_UDP mode";
			std::cerr << std::endl;
#endif
			mNetMode |= RS_NET_MODE_UDP;
			mNetStatus = RS_NET_EXT_SETUP;
			break;

		case RS_NET_MODE_TRY_LOOPBACK:
			std::cerr << "p3NetMgrIMPL::netStartup() TRY_LOOPBACK mode";
			std::cerr << std::endl;
			mNetMode |= RS_NET_MODE_HIDDEN;
			mNetStatus = RS_NET_LOOPBACK;
			break;

		default: // Fall through.

#ifdef NETMGR_DEBUG_RESET
			std::cerr << "p3NetMgrIMPL::netStartup() UNKNOWN mode";
			std::cerr << std::endl;
#endif

		case RS_NET_MODE_TRY_UPNP:
#ifdef NETMGR_DEBUG_RESET
			std::cerr << "p3NetMgrIMPL::netStartup() TRY_UPNP mode";
			std::cerr << std::endl;
#endif
			/* Force it here (could be default!) */
			mNetMode |= RS_NET_MODE_TRY_UPNP;
			mNetMode |= RS_NET_MODE_UDP;      /* set to UDP, upgraded is UPnP is Okay */
			mNetStatus = RS_NET_UPNP_INIT;
			break;
	}
}


void p3NetMgrIMPL::tick()
{
	rstime_t now = time(nullptr);
	rstime_t dontCheckNetUntil;
	{ RS_STACK_MUTEX(mNetMtx); dontCheckNetUntil = mDoNotNetCheckUntilTs; }

	if(now >= dontCheckNetUntil) netStatusTick();

	uint32_t netStatus; { RS_STACK_MUTEX(mNetMtx); netStatus = mNetStatus; }
	switch (netStatus)
	{
	case RS_NET_LOOPBACK:
		if(dontCheckNetUntil <= now)
		{
			RS_STACK_MUTEX(mNetMtx);
			mDoNotNetCheckUntilTs = now + 30;
		}
		break;
	default:
		netAssistTick();
		updateNetStateBox_temporal();
#ifdef RS_USE_DHT_STUNNER
		if (mDhtStunner) mDhtStunner->tick();
		if (mProxyStunner) mProxyStunner->tick();
#endif // RS_USE_DHT_STUNNER
		break;
	}
}

#define STARTUP_DELAY 5


void p3NetMgrIMPL::netStatusTick()
{

#ifdef NETMGR_DEBUG_TICK
	std::cerr << "p3NetMgrIMPL::netTick()" << std::endl;

	std::cerr << "p3NetMgrIMPL::netTick() mNetMode: " << std::hex << mNetMode;
	std::cerr << " ACTUALMODE: " << (mNetMode & RS_NET_MODE_ACTUAL);
	std::cerr << " TRYMODE: " << (mNetMode & RS_NET_MODE_TRYMODE);
	std::cerr << std::endl;
#endif

	// Check whether we are stuck on loopback. This happens if RS starts when
	// the computer is not yet connected to the internet. In such a case we
	// periodically check for a local net address.
	//
	checkNetAddress() ;

	uint32_t netStatus = 0;
	rstime_t   age = 0;
	{
		RsStackMutex stack(mNetMtx);   /************** LOCK MUTEX ***************/

		netStatus = mNetStatus;
		age = time(NULL) - mNetInitTS;

	}

	switch(netStatus)
	{
		case RS_NET_NEEDS_RESET:

#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netTick() STATUS: NEEDS_RESET" << std::endl;
#endif
			rslog(RSL_WARNING, p3netmgrzone, "p3NetMgr::netTick() RS_NET_NEEDS_RESET, resetting network");
	
			netReset();
			break;

		case RS_NET_UNKNOWN:
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netTick() STATUS: UNKNOWN" << std::endl;
#endif

			/* add a small delay to stop restarting straight after a RESET 
			 * This is so can we shutdown cleanly
			 */
			if (age < STARTUP_DELAY)
			{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
				std::cerr << "p3NetMgrIMPL::netTick() Delaying Startup" << std::endl;
#endif
			}
			else
			{
				netStartup();
			}

			break;

		case RS_NET_UPNP_INIT:
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netTick() STATUS: UPNP_INIT" << std::endl;
#endif
			netUpnpInit();
			break;

		case RS_NET_UPNP_SETUP:
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netTick() STATUS: UPNP_SETUP" << std::endl;
#endif
			netUpnpCheck();
			break;


		case RS_NET_EXT_SETUP:
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netTick() STATUS: EXT_SETUP" << std::endl;
#endif
			netExtCheck();
			break;

		case RS_NET_DONE:
#ifdef NETMGR_DEBUG_TICK
			std::cerr << "p3NetMgrIMPL::netTick() STATUS: DONE" << std::endl;
#endif

			break;

		case RS_NET_LOOPBACK:
                        //don't do a shutdown because a client in a computer without local network might be usefull for debug.
                        //shutdown();
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
                        std::cerr << "p3NetMgrIMPL::netTick() STATUS: RS_NET_LOOPBACK" << std::endl;
#endif
		default:
			break;
	}

	return;
}


void p3NetMgrIMPL::netDhtInit()
{
#if defined(NETMGR_DEBUG_RESET)
	std::cerr << "p3NetMgrIMPL::netDhtInit()" << std::endl;
#endif
	
	uint32_t vs = 0;
	{
		RsStackMutex stack(mNetMtx); /*********** LOCKED MUTEX ************/
		vs = mVsDht;
	}
	
	enableNetAssistConnect(vs != RS_VS_DHT_OFF);
}


void p3NetMgrIMPL::netUpnpInit()
{
#if defined(NETMGR_DEBUG_RESET)
	std::cerr << "p3NetMgrIMPL::netUpnpInit()" << std::endl;
#endif
	uint16_t eport, iport;

	mNetMtx.lock();   /*   LOCK MUTEX */

	/* get the ports from the configuration */

	mNetStatus = RS_NET_UPNP_SETUP;
	iport = sockaddr_storage_port(mLocalAddr);
	eport = sockaddr_storage_port(mExtAddr);
	if ((eport < 1000) || (eport > 30000))
	{
		eport = iport;
	}

	mNetMtx.unlock(); /* UNLOCK MUTEX */

	netAssistFirewallPorts(iport, eport);
	enableNetAssistFirewall(true);
}

void p3NetMgrIMPL::netUpnpCheck()
{
	/* grab timestamp */
	mNetMtx.lock();   /*   LOCK MUTEX */

	rstime_t delta = time(NULL) - mNetInitTS;

#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
		std::cerr << "p3NetMgrIMPL::netUpnpCheck() age: " << delta << std::endl;
#endif

	mNetMtx.unlock(); /* UNLOCK MUTEX */

	struct sockaddr_storage extAddr;
	int upnpState = netAssistFirewallActive();

	if (((upnpState == 0) && (delta > (rstime_t)MAX_UPNP_INIT)) ||
	    ((upnpState > 0) && (delta > (rstime_t)MAX_UPNP_COMPLETE)))
	{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
		std::cerr << "p3NetMgrIMPL::netUpnpCheck() ";
		std::cerr << "Upnp Check failed." << std::endl;
#endif
		/* fallback to UDP startup */
		mNetMtx.lock();   /*   LOCK MUTEX */

		/* UPnP Failed us! */
		mNetStatus = RS_NET_EXT_SETUP;
		mNetFlags.mUpnpOk = false;

		mNetMtx.unlock(); /* UNLOCK MUTEX */
	}
	else if ((upnpState > 0) && netAssistExtAddress(extAddr))
	{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
		std::cerr << "p3NetMgrIMPL::netUpnpCheck() ";
		std::cerr << "Upnp Check success state: " << upnpState << std::endl;
#endif
		/* switch to UDP startup */
		mNetMtx.lock();   /*   LOCK MUTEX */

		/* Set Net Status flags ....
		 * we now have external upnp address. Golden!
		 * don't set netOk flag until have seen some traffic.
		 */
		if (sockaddr_storage_isValidNet(extAddr))
		{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netUpnpCheck() ";
			std::cerr << "UpnpAddr: " << sockaddr_storage_tostring(extAddr);
			std::cerr << std::endl;
#endif
			mNetFlags.mUpnpOk = true;
			mNetFlags.mExtAddr = extAddr;
			mNetFlags.mExtAddrOk = true;
			mNetFlags.mExtAddrStableOk = true;

			mNetStatus = RS_NET_EXT_SETUP;
			/* Fix netMode & Clear others! */
			mNetMode = RS_NET_MODE_TRY_UPNP | RS_NET_MODE_UPNP; 
		}
		mNetMtx.unlock(); /* UNLOCK MUTEX */
	}
	else
	{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
		std::cerr << "p3NetMgrIMPL::netUpnpCheck() ";
		std::cerr << "Upnp Check Continues: status: " << upnpState << std::endl;
#endif
	}

}

class ZeroInt
{
    public:
        ZeroInt() { n=0; }
        uint32_t n ;
};

void p3NetMgrIMPL::netExtCheck()
{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
	std::cerr << "p3NetMgrIMPL::netExtCheck()" << std::endl;
#endif
	bool netSetupDone = false;

	{
		RS_STACK_MUTEX(mNetMtx);

		bool isStable = false;
		sockaddr_storage tmpip;

		std::map<sockaddr_storage,ZeroInt> address_votes;

		/* check for External Address */
		/* in order of importance */
		/* (1) UPnP -> which handles itself */
		{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netExtCheck() Ext Not Ok" << std::endl;
#endif
			/* net Assist */
			if ( netAssistExtAddress(tmpip) &&
			     sockaddr_storage_isValidNet(tmpip) &&
			     sockaddr_storage_ipv6_to_ipv4(tmpip) )
			{
				if( !rsBanList ||
				        rsBanList->isAddressAccepted(
				            tmpip, RSBANLIST_CHECKING_FLAGS_BLACKLIST ) )
				{
					// must be stable???
					isStable = true;
					mNetFlags.mExtAddrOk = true;
					mNetFlags.mExtAddrStableOk = isStable;
					address_votes[tmpip].n++ ;
					std::cerr << __PRETTY_FUNCTION__ << " NetAssistAddress "
					          << " reported external address "
					          << sockaddr_storage_iptostring(tmpip)
					          << std::endl;
				}
				else
					std::cerr << "(SS) netAssisExternalAddress returned banned "
					          << "own IP " << sockaddr_storage_iptostring(tmpip)
					          << " (banned). Rejecting." << std::endl;
			}
		}

#ifdef ALLOW_DHT_STUNNER
        // (cyril) I disabled this because it's pretty dangerous. The DHT can report a wrong address quite easily
        // if the other DHT peers are not collaborating.
        
		/* Next ask the DhtStunner */
		{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netExtCheck() Ext Not Ok, Checking DhtStunner" << std::endl;
#endif
			uint8_t isstable = 0;
			struct sockaddr_storage tmpaddr;
			sockaddr_storage_clear(tmpaddr);

			if (mDhtStunner)
			{
				/* input network bits */
				if (mDhtStunner->getExternalAddr(tmpaddr, isstable))
				{
					if((rsBanList == NULL) || rsBanList->isAddressAccepted(tmpaddr,RSBANLIST_CHECKING_FLAGS_BLACKLIST))
					{
						// must be stable???
						isStable = (isstable == 1);
						//mNetFlags.mExtAddr = tmpaddr;
						mNetFlags.mExtAddrOk = true;
						mNetFlags.mExtAddrStableOk = isStable;

						address_votes[tmpaddr].n++ ;
#ifdef	NETMGR_DEBUG_STATEBOX
						std::cerr << "p3NetMgrIMPL::netExtCheck() From DhtStunner: ";
						std::cerr << sockaddr_storage_tostring(tmpaddr);
						std::cerr << " Stable: " << (uint32_t) isstable;
						std::cerr << std::endl;
#endif
					}
					else
						std::cerr << "(SS) DHTStunner returned wrong own IP " << sockaddr_storage_iptostring(tmpaddr) << " (banned). Rejecting." << std::endl;
				}
			}
		}
#endif

		/* ask ExtAddrFinder */
		{
			/* ExtAddrFinder */
			if (mUseExtAddrFinder)
			{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
				std::cerr << "p3NetMgrIMPL::netExtCheck() checking ExtAddrFinder" << std::endl;
#endif
				bool extFinderOk = mExtAddrFinder->hasValidIP(tmpip);
				if (extFinderOk && sockaddr_storage_ipv6_to_ipv4(tmpip))
				{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
					std::cerr << "p3NetMgrIMPL::netExtCheck() Ext supplied by ExtAddrFinder" << std::endl;
#endif
					sockaddr_storage_setport(tmpip, guessNewExtPort());

#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
					std::cerr << "p3NetMgrIMPL::netExtCheck() ";
					std::cerr << "ExtAddr: " << sockaddr_storage_tostring(tmpip);
					std::cerr << std::endl;
#endif

					mNetFlags.mExtAddrOk = true;

					address_votes[tmpip].n++ ;

					/* XXX HACK TO FIX drbob: ALLOWING
					 * ExtAddrFinder -> ExtAddrStableOk = true
					 * (which it is not normally) */
					mNetFlags.mExtAddrStableOk = true;

					std::cerr << __PRETTY_FUNCTION__ << " ExtAddrFinder "
					          << " reported external address "
					          << sockaddr_storage_iptostring(tmpip)
					          << std::endl;
				}
			}
		}

		/* also ask peer mgr. */
		if (mPeerMgr)
		{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netExtCheck() checking mPeerMgr" << std::endl;
#endif
			uint8_t isstable; // unused
			sockaddr_storage tmpaddr;

			if ( mPeerMgr->getExtAddressReportedByFriends(tmpaddr, isstable) &&
			     sockaddr_storage_ipv6_to_ipv4(tmpaddr) )
			{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
				std::cerr << "p3NetMgrIMPL::netExtCheck() Ext supplied by ExtAddrFinder" << std::endl;
#endif
				sockaddr_storage_setport(tmpaddr, guessNewExtPort());

#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
				std::cerr << "p3NetMgrIMPL::netExtCheck() ";
				std::cerr << "ExtAddr: " << sockaddr_storage_tostring(tmpip);
				std::cerr << std::endl;
#endif

				mNetFlags.mExtAddrOk = true;
				mNetFlags.mExtAddrStableOk = isstable;

				address_votes[tmpaddr].n++;

				std::cerr << __PRETTY_FUNCTION__ << " PeerMgr reported external"
				          << " address "
				          << sockaddr_storage_iptostring(tmpaddr) << std::endl;
			}
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			else
				std::cerr << "  No reliable address returned." << std::endl;
#endif
		}

		/* any other sources ??? */

		/* finalise address */
		if (mNetFlags.mExtAddrOk)
		{
			// look at votes.

			std::cerr << "Figuring out ext addr from voting:" << std::endl;
			uint32_t admax = 0 ;

			for(std::map<sockaddr_storage,ZeroInt>::const_iterator it(address_votes.begin());it!=address_votes.end();++it)
			{
				std::cerr << "  Vote: " << sockaddr_storage_iptostring(it->first) << " : " << it->second.n << " votes." ;

				if(it->second.n > admax)
				{
					mNetFlags.mExtAddr = it->first ;
					admax = it->second.n ;

					std::cerr << " Kept!" << std::endl;
				}
				else
					std::cerr << " Discarded." << std::endl;
			}

#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netExtCheck() ";
			std::cerr << "ExtAddr: " << sockaddr_storage_tostring(mNetFlags.mExtAddr);
			std::cerr << std::endl;
#endif
			//update ip address list
			mExtAddr = mNetFlags.mExtAddr;

			mNetStatus = RS_NET_DONE;
			netSetupDone = true;

#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netExtCheck() Ext Ok: RS_NET_DONE" << std::endl;
#endif



			if (!mNetFlags.mExtAddrStableOk)
			{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
				std::cerr << "p3NetMgrIMPL::netUdpCheck() UDP Unstable :( ";
				std::cerr <<  std::endl;
				std::cerr << "p3NetMgrIMPL::netUdpCheck() We are unreachable";
				std::cerr <<  std::endl;
				std::cerr << "netMode =>  RS_NET_MODE_UNREACHABLE";
				std::cerr <<  std::endl;
#endif

				// Due to the new UDP connections - we can still connect some of the time!
				// So limit warning!

				//mNetMode &= ~(RS_NET_MODE_ACTUAL);
				//mNetMode |= RS_NET_MODE_UNREACHABLE;

				/* send a system warning message */
				//pqiNotify *notify = getPqiNotify();
				//if (notify)
				{
					//std::string title =
					//                "Warning: Bad Firewall Configuration";

					std::string msg;
					msg +=  "               **** WARNING ****     \n";
					msg +=  "Retroshare has detected that you are behind";
					msg +=  " a restrictive Firewall\n";
					msg +=  "\n";
					msg +=  "You will have limited connectivity to other firewalled peers\n";
					msg +=  "\n";
					msg +=  "You can fix this by:\n";
					msg +=  "   (1) opening an External Port\n";
					msg +=  "   (2) enabling UPnP, or\n";
					msg +=  "   (3) get a new (approved) Firewall/Router\n";

					//notify->AddSysMessage(0, RS_SYS_WARNING, title, msg);

					std::cerr << msg << std::endl;
				}

			}

		}

		if (mNetFlags.mExtAddrOk)
		{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netExtCheck() setting netAssistSetAddress()" << std::endl;
#endif
			netAssistSetAddress(mNetFlags.mLocalAddr, mNetFlags.mExtAddr, mNetMode);
		}

		/* flag unreachables! */
		if ((mNetFlags.mExtAddrOk) && (!mNetFlags.mExtAddrStableOk))
		{
#if defined(NETMGR_DEBUG_TICK) || defined(NETMGR_DEBUG_RESET)
			std::cerr << "p3NetMgrIMPL::netExtCheck() Ext Unstable - Unreachable Check" << std::endl;
#endif
		}
	}

	if (netSetupDone)
	{
		std::cerr << "p3NetMgrIMPL::netExtCheck() netSetupDone" << std::endl;

		/* Setup NetStateBox with this info */
		updateNetStateBox_startup();

		/* update PeerMgr with correct info */
		if (mPeerMgr)
		{
			mPeerMgr->UpdateOwnAddress(mLocalAddr, mExtAddr);
		}

		/* inform DHT about our external address */
		RsPeerId fakeId;
		netAssistKnownPeer(fakeId, mExtAddr, NETASSIST_KNOWN_PEER_SELF | NETASSIST_KNOWN_PEER_ONLINE);

		std::cerr << __PRETTY_FUNCTION__ << " Network Setup Complete"
		          << std::endl;
	}
}

/**********************************************************************************************
 ************************************** Interfaces    *****************************************
 **********************************************************************************************/

bool p3NetMgrIMPL::checkNetAddress()
{
	bool addrChanged = false;
	bool validAddr = false;
	
	sockaddr_storage prefAddr;
	sockaddr_storage oldAddr;

	if (mNetMode & RS_NET_MODE_TRY_LOOPBACK)
	{
		RsInfo() << __PRETTY_FUNCTION__ <<" network mode set to LOOPBACK,"
		         << " forcing  address to 127.0.0.1" << std::endl;

		sockaddr_storage_ipv4_aton(prefAddr, "127.0.0.1");
		validAddr = true;
	}
	else
	{
		/* TODO: Sat Oct 24 15:51:24 CEST 2015 The fact of having just one local
		 *  address is a flawed assumption, this should be redesigned as soon as
		 *  possible. It will require complete reenginering of the network layer
		 *  code. */

		/* For retro-compatibility strictly accept only IPv4 addresses here,
		 * IPv6 addresses are handled in a retro-compatible manner in
		 * p3PeerMgrIMPL::UpdateOwnAddress */
		std::vector<sockaddr_storage> addrs;
		if (getLocalAddresses(addrs))
		{
			for (auto it = addrs.begin(); it != addrs.end(); ++it)
			{
				sockaddr_storage& addr(*it);
				if( sockaddr_storage_isValidNet(addr) &&
				    !sockaddr_storage_isLoopbackNet(addr) &&
				    !sockaddr_storage_isLinkLocalNet(addr) &&
				    sockaddr_storage_ipv6_to_ipv4(addr) )
				{
					prefAddr = addr;
					validAddr = true;
					break;
				}
			}

			/* If no satisfactory local address has been found yet relax and
			 * accept also link local addresses */
			if(!validAddr) for (auto it = addrs.begin(); it!=addrs.end(); ++it)
			{
				sockaddr_storage& addr(*it);
				if( sockaddr_storage_isValidNet(addr) &&
				    !sockaddr_storage_isLoopbackNet(addr) &&
				    sockaddr_storage_ipv6_to_ipv4(addr) )
				{
					prefAddr = addr;
					validAddr = true;
					break;
				}
			}

			/* If no satisfactory local address has been found yet relax and
			 * accept also loopback addresses */
			if(!validAddr) for (auto it = addrs.begin(); it!=addrs.end(); ++it)
			{
				sockaddr_storage& addr(*it);
				if( sockaddr_storage_isValidNet(addr) &&
				    sockaddr_storage_ipv6_to_ipv4(addr) )
				{
					prefAddr = addr;
					validAddr = true;
					break;
				}
			}
		}
	}

	if (!validAddr)
	{
		RsErr() << __PRETTY_FUNCTION__ << " no valid local network address "
		        <<" found. Report to developers." << std::endl;
		print_stacktrace();

		return false;
	}
	
	/* check addresses */
	{ RS_STACK_MUTEX(mNetMtx);

		sockaddr_storage_copy(mLocalAddr, oldAddr);
		addrChanged = !sockaddr_storage_sameip(prefAddr, mLocalAddr);

		// update address.
		sockaddr_storage_copyip(mLocalAddr, prefAddr);
		sockaddr_storage_copy(mLocalAddr, mNetFlags.mLocalAddr);

		if(sockaddr_storage_isLoopbackNet(mLocalAddr))
			mNetStatus = RS_NET_LOOPBACK;

		// Check if local port is valid, reset it if not
		if (!sockaddr_storage_port(mLocalAddr))
		{
			/* Using same port as external may make some NAT happier */
			uint16_t port = sockaddr_storage_port(mExtAddr);

			/* Avoid to automatically set a local port to a reserved one < 1024
			 * that needs special permissions or root access.
			 * This do not impede the user to set a reserved port manually,
			 * which make sense in some cases. */
			while (port < 1025)
				port = static_cast<uint16_t>(RsRandom::random_u32());

			sockaddr_storage_setport(mLocalAddr, htons(port));
			addrChanged = true;

			RsWarn() << __PRETTY_FUNCTION__ << " local port was 0, corrected "
			         <<"to: " << port << std::endl;
		}
	} // RS_STACK_MUTEX(mNetMtx);

	if (addrChanged)
	{
		RsInfo() << __PRETTY_FUNCTION__ << " local address changed, resetting"
		         <<" network." << std::endl;
		
		if (mPeerMgr) mPeerMgr->UpdateOwnAddress(mLocalAddr, mExtAddr);

		netReset();
	}

	return true;
}


/**********************************************************************************************
 ************************************** Interfaces    *****************************************
 **********************************************************************************************/

/* to allow resets of network stuff */
void    p3NetMgrIMPL::addNetListener(pqiNetListener *listener)
{
        RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
        mNetListeners.push_back(listener);
}



bool    p3NetMgrIMPL::setLocalAddress(const struct sockaddr_storage &addr)
{
	bool changed = false;
	{
		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
		if (!sockaddr_storage_same(mLocalAddr, addr))
		{
			changed = true;
		}

		mLocalAddr = addr;
	}

	if (changed)
	{
#ifdef NETMGR_DEBUG_RESET
		std::cerr << "p3NetMgrIMPL::setLocalAddress() Calling NetReset" << std::endl;
#endif
		rslog(RSL_WARNING, p3netmgrzone, "p3NetMgr::setLocalAddress() local address changed, resetting network");
		netReset();
	}
	return true;
}
bool    p3NetMgrIMPL::getExtAddress(struct sockaddr_storage& addr)
{
        RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/

        if(mNetFlags.mExtAddrOk)
        {
            addr = mNetFlags.mExtAddr ;
            return true ;
        }
        else
            return false ;
}

bool    p3NetMgrIMPL::setExtAddress(const struct sockaddr_storage &addr)
{
	bool changed = false;
	{
		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
		if (!sockaddr_storage_same(mExtAddr, addr))
		{
			changed = true;
		}

		mExtAddr = addr;
	}

	if (changed)
	{
#ifdef NETMGR_DEBUG_RESET
		std::cerr << "p3NetMgrIMPL::setExtAddress() Calling NetReset" << std::endl;
#endif
		rslog(RSL_WARNING, p3netmgrzone, "p3NetMgr::setExtAddress() ext address changed, resetting network");
		netReset();
	}
	return true;
}

bool    p3NetMgrIMPL::setNetworkMode(uint32_t netMode)
{
	uint32_t oldNetMode;
	{
		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
		/* only change TRY flags */

		oldNetMode = mNetMode;

#ifdef NETMGR_DEBUG
        std::cerr << "p3NetMgrIMPL::setNetworkMode()";
        std::cerr << " Existing netMode: " << mNetMode;
        std::cerr << " Input netMode: " << netMode;
        std::cerr << std::endl;
#endif
		mNetMode &= ~(RS_NET_MODE_TRYMODE);

		switch(netMode & RS_NET_MODE_ACTUAL)
		{
			case RS_NET_MODE_EXT:
				mNetMode |= RS_NET_MODE_TRY_EXT;
				break;
			case RS_NET_MODE_UPNP:
				mNetMode |= RS_NET_MODE_TRY_UPNP;
				break;
			case RS_NET_MODE_HIDDEN:
				mNetMode |= RS_NET_MODE_TRY_LOOPBACK; 
				break;
			default:
			case RS_NET_MODE_UDP:
				mNetMode |= RS_NET_MODE_TRY_UDP;
				break;
		}
	}


	if ((netMode & RS_NET_MODE_ACTUAL) != (oldNetMode & RS_NET_MODE_ACTUAL)) 
	{
#ifdef NETMGR_DEBUG_RESET
		std::cerr << "p3NetMgrIMPL::setNetworkMode() Calling NetReset" << std::endl;
#endif
		rslog(RSL_WARNING, p3netmgrzone, "p3NetMgr::setNetworkMode() Net Mode changed, resetting network");
		netReset();
	}
	return true;
}


bool    p3NetMgrIMPL::setVisState(uint16_t vs_disc, uint16_t vs_dht)
{
	RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
	mVsDisc = vs_disc;
    mVsDht = vs_dht;

	/* if we've started up - then tweak Dht On/Off */
	if (mNetStatus != RS_NET_UNKNOWN)
	{
		enableNetAssistConnect(mVsDht != RS_VS_DHT_OFF);
	}

	return true;
}


/**********************************************************************************************
 ************************************** Interfaces    *****************************************
 **********************************************************************************************/

void p3NetMgrIMPL::addNetAssistFirewall(uint32_t id, pqiNetAssistFirewall *fwAgent)
{
	mFwAgents[id] = fwAgent;
}


bool p3NetMgrIMPL::enableNetAssistFirewall(bool on)
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); ++it)
	{
		(it->second)->enable(on);
	}
	return true;
}


bool p3NetMgrIMPL::netAssistFirewallEnabled()
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); ++it)
	{
		if ((it->second)->getEnabled())
		{
			return true;
		}
	}
	return false;
}

bool p3NetMgrIMPL::netAssistFirewallActive()
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); ++it)
	{
		if ((it->second)->getActive())
		{
			return true;
		}
	}
	return false;
}

bool p3NetMgrIMPL::netAssistFirewallShutdown()
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); ++it)
	{
		(it->second)->shutdown();
	}
	return true;
}

bool p3NetMgrIMPL::netAssistFirewallPorts(uint16_t iport, uint16_t eport)
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); ++it)
	{
		(it->second)->setInternalPort(iport);
		(it->second)->setExternalPort(eport);
	}
	return true;
}


bool p3NetMgrIMPL::netAssistExtAddress(struct sockaddr_storage &extAddr)
{
	std::map<uint32_t, pqiNetAssistFirewall *>::iterator it;
	for(it = mFwAgents.begin(); it != mFwAgents.end(); ++it)
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


void p3NetMgrIMPL::addNetAssistConnect(uint32_t id, pqiNetAssistConnect *dht)
{
	mDhts[id] = dht;
}

bool p3NetMgrIMPL::enableNetAssistConnect(bool on)
{

#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgrIMPL::enableNetAssistConnect(" << on << ")";
	std::cerr << std::endl;
#endif

	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		(it->second)->enable(on);
	}
	return true;
}

bool p3NetMgrIMPL::netAssistConnectEnabled()
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		if ((it->second)->getEnabled())
		{
#ifdef NETMGR_DEBUG
			std::cerr << "p3NetMgrIMPL::netAssistConnectEnabled() YES";
			std::cerr << std::endl;
#endif

			return true;
		}
	}

#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgrIMPL::netAssistConnectEnabled() NO";
	std::cerr << std::endl;
#endif

	return false;
}

bool p3NetMgrIMPL::netAssistConnectActive()
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		if ((it->second)->getActive())

		{
#ifdef NETMGR_DEBUG
			std::cerr << "p3NetMgrIMPL::netAssistConnectActive() ACTIVE";
			std::cerr << std::endl;
#endif

			return true;
		}
	}

#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgrIMPL::netAssistConnectActive() INACTIVE";
	std::cerr << std::endl;
#endif

	return false;
}

bool p3NetMgrIMPL::netAssistConnectStats(uint32_t &netsize, uint32_t &localnetsize)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		if (((it->second)->getActive()) && ((it->second)->getNetworkStats(netsize, localnetsize)))

		{
#ifdef NETMGR_DEBUG
			std::cerr << "p3NetMgrIMPL::netAssistConnectStats(";
			std::cerr << netsize << ", " << localnetsize << ")";
			std::cerr << std::endl;
#endif

			return true;
		}
	}

#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgrIMPL::netAssistConnectStats() INACTIVE";
	std::cerr << std::endl;
#endif

	return false;
}

bool p3NetMgrIMPL::netAssistConnectShutdown()
{
#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgrIMPL::netAssistConnectShutdown()";
	std::cerr << std::endl;
#endif

	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		(it->second)->shutdown();
	}
	return true;
}

bool p3NetMgrIMPL::netAssistFriend(const RsPeerId &id, bool on)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;

#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgrIMPL::netAssistFriend(" << id << ", " << on << ")";
	std::cerr << std::endl;
#endif

	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		if (on)
			(it->second)->findPeer(id);
		else
			(it->second)->dropPeer(id);
	}
	return true;
}


bool p3NetMgrIMPL::netAssistKnownPeer(const RsPeerId &id, const struct sockaddr_storage &addr, uint32_t flags)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;

#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgrIMPL::netAssistKnownPeer(" << id << ")";
	std::cerr << std::endl;
#endif

	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		(it->second)->addKnownPeer(id, addr, flags);
	}
	return true;
}

bool p3NetMgrIMPL::netAssistBadPeer(const struct sockaddr_storage &addr, uint32_t reason, uint32_t flags, uint32_t age)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;

#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgrIMPL::netAssistBadPeer(" << sockaddr_storage_iptostring(addr) << ")";
	std::cerr << std::endl;
#endif

	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		(it->second)->addBadPeer(addr, reason, flags, age);
	}
	return true;
}


bool p3NetMgrIMPL::netAssistAttach(bool on)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;

#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgrIMPL::netAssistAttach(" << on << ")";
	std::cerr << std::endl;
#endif

	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		(it->second)->setAttachMode(on);
	}
	return true;
}



bool p3NetMgrIMPL::netAssistStatusUpdate(const RsPeerId &id, int state)
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;

#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgrIMPL::netAssistStatusUpdate(" << id << ", " << state << ")";
	std::cerr << std::endl;
#endif

	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		(it->second)->ConnectionFeedback(id, state);
	}
	return true;
}


bool p3NetMgrIMPL::netAssistSetAddress( const struct sockaddr_storage & /*laddr*/,
					const struct sockaddr_storage & /*eaddr*/,
					uint32_t /*mode*/)
{
#if 0
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		(it->second)->setExternalInterface(laddr, eaddr, mode);
	}
#endif
	return true;
}

void p3NetMgrIMPL::netAssistTick()
{
	std::map<uint32_t, pqiNetAssistConnect *>::iterator it;
	for(it = mDhts.begin(); it != mDhts.end(); ++it)
	{
		(it->second)->tick();
	}

	std::map<uint32_t, pqiNetAssistFirewall *>::iterator fit;
	for(fit = mFwAgents.begin(); fit != mFwAgents.end(); ++fit)
	{
		(fit->second)->tick();
	}
	return;
}



/**********************************************************************
 **********************************************************************
 ******************** Network State ***********************************
 **********************************************************************
 **********************************************************************/

bool    p3NetMgrIMPL::getUPnPState()
{
	return netAssistFirewallActive();
}

bool	p3NetMgrIMPL::getUPnPEnabled()
{
	return netAssistFirewallEnabled();
}

bool	p3NetMgrIMPL::getDHTEnabled()
{
	return netAssistConnectEnabled();
}


void	p3NetMgrIMPL::getNetStatus(pqiNetStatus &status)
{
	/* cannot lock local stack, then call DHT... as this can cause lock up */
	/* must extract data... then update mNetFlags */

	bool dhtOk = netAssistConnectActive();
	uint32_t netsize = 0, rsnetsize = 0;
	netAssistConnectStats(netsize, rsnetsize);

	RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/

	/* quick update of the stuff that can change! */
	mNetFlags.mDhtOk = dhtOk;
	mNetFlags.mDhtNetworkSize = netsize;
	mNetFlags.mDhtRsNetworkSize = rsnetsize;

	status = mNetFlags;
}









/**********************************************************************************************
 ************************************** ExtAddrFinder *****************************************
 **********************************************************************************************/

bool  p3NetMgrIMPL::getIPServersEnabled() 
{ 
	RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
	return mUseExtAddrFinder;
}

void  p3NetMgrIMPL::getIPServersList(std::list<std::string>& ip_servers) 
{ 
	mExtAddrFinder->getIPServersList(ip_servers);
}

void p3NetMgrIMPL::setIPServersEnabled(bool b)
{
	{
		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
		mUseExtAddrFinder = b;
	}

#ifdef NETMGR_DEBUG
	std::cerr << "p3NetMgr: setIPServers to " << b << std::endl ; 
#endif

}



/**********************************************************************************************
 ************************************** NetStateBox  ******************************************
 **********************************************************************************************/

uint32_t p3NetMgrIMPL::getNetStateMode()
{
	RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
	return mNetStateBox.getNetStateMode();
}

uint32_t p3NetMgrIMPL::getNetworkMode()
{
	RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
	return mNetStateBox.getNetworkMode();
}

uint32_t p3NetMgrIMPL::getNatTypeMode()
{
	RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
	return mNetStateBox.getNatTypeMode();
}

uint32_t p3NetMgrIMPL::getNatHoleMode()
{
	RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
	return mNetStateBox.getNatHoleMode();
}

uint32_t p3NetMgrIMPL::getConnectModes()
{
	RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
	return mNetStateBox.getConnectModes();
}


/* These are the regular updates from Dht / Stunners */
void p3NetMgrIMPL::updateNetStateBox_temporal()
{
#ifdef	NETMGR_DEBUG_STATEBOX
	std::cerr << "p3NetMgrIMPL::updateNetStateBox_temporal() ";
	std::cerr << std::endl;
#endif
	struct sockaddr_storage tmpaddr;
	sockaddr_storage_clear(tmpaddr);

#ifdef RS_USE_DHT_STUNNER
	uint8_t isstable = 0;

	if (mDhtStunner)
	{

        	/* input network bits */
       		if (mDhtStunner->getExternalAddr(tmpaddr, isstable))
		{
			RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
			mNetStateBox.setAddressStunDht(tmpaddr, isstable);
			
#ifdef	NETMGR_DEBUG_STATEBOX
			std::cerr << "p3NetMgrIMPL::updateNetStateBox_temporal() DhtStunner: ";
			std::cerr << sockaddr_storage_tostring(tmpaddr);
			std::cerr << " Stable: " << (uint32_t) isstable;
			std::cerr << std::endl;
#endif
			
		}
	}

	if (mProxyStunner)
	{

        	/* input network bits */
       		if (mProxyStunner->getExternalAddr(tmpaddr, isstable))
		{
			RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
			mNetStateBox.setAddressStunProxy(tmpaddr, isstable);

#ifdef	NETMGR_DEBUG_STATEBOX
			std::cerr << "p3NetMgrIMPL::updateNetStateBox_temporal() ProxyStunner: ";
			std::cerr << sockaddr_storage_tostring(tmpaddr);
			std::cerr << " Stable: " << (uint32_t) isstable;
			std::cerr << std::endl;
#endif
		
		}
	}
#endif // RS_USE_DHT_STUNNER


	{
		bool dhtOn = netAssistConnectEnabled();
		bool dhtActive = netAssistConnectActive();

		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/
		mNetStateBox.setDhtState(dhtOn, dhtActive);
	}


	/* now we check if a WebIP address is required? */

#ifdef	NETMGR_DEBUG_STATEBOX
	{
		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/

		uint32_t netstate = mNetStateBox.getNetStateMode();
		uint32_t netMode = mNetStateBox.getNetworkMode();
		uint32_t natType = mNetStateBox.getNatTypeMode();
		uint32_t natHole = mNetStateBox.getNatHoleMode();
		uint32_t connect = mNetStateBox.getConnectModes();

		std::string netstatestr = NetStateNetStateString(netstate);
		std::string connectstr = NetStateConnectModesString(connect);
		std::string natholestr = NetStateNatHoleString(natHole);
		std::string nattypestr = NetStateNatTypeString(natType);
		std::string netmodestr = NetStateNetworkModeString(netMode);

		std::cerr << "p3NetMgrIMPL::updateNetStateBox_temporal() NetStateBox Thinking";
		std::cerr << std::endl;
		std::cerr << "\tNetState: " << netstatestr;
		std::cerr << std::endl;
		std::cerr << "\tConnectModes: " << connectstr;
		std::cerr << std::endl;
		std::cerr << "\tNetworkMode: " << netmodestr;
		std::cerr << std::endl;
		std::cerr << "\tNatHole: " << natholestr;
		std::cerr << std::endl;
		std::cerr << "\tNatType: " << nattypestr;
		std::cerr << std::endl;

	}
#endif
	
	updateNatSetting();

}

#define NET_STUNNER_PERIOD_FAST		(-1)	// default of Stunner.
#define NET_STUNNER_PERIOD_SLOW		(120) 	// This needs to be as small Routers will allow... try 2 minutes.
// FOR TESTING ONLY.
//#define NET_STUNNER_PERIOD_SLOW		(60) 	// 3 minutes.

void p3NetMgrIMPL::updateNatSetting()
{
	bool updateRefreshRate = false;
	uint32_t natType = RSNET_NATTYPE_UNKNOWN;
	uint32_t natHole = RSNET_NATHOLE_UNKNOWN;
	{
		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/

		natType = mNetStateBox.getNatTypeMode();
		natHole = mNetStateBox.getNatHoleMode();
		if ((natType != mOldNatType) || (natHole != mOldNatHole))
		{
			mOldNatType = natType;
			mOldNatHole = natHole;
			updateRefreshRate = true;
			
#ifdef	NETMGR_DEBUG_STATEBOX
			std::cerr << "p3NetMgrIMPL::updateNetStateBox_temporal() NatType Change!";
			std::cerr << "\tNatType: " << NetStateNatTypeString(natType);
			std::cerr << "\tNatHole: " << NetStateNatHoleString(natHole);

			std::cerr << std::endl;
#endif
			
			
		}
	}

	
	// MUST also use this chance to set ATTACH flag for DHT.
	if (updateRefreshRate)
	{
#ifdef	NETMGR_DEBUG_STATEBOX
		std::cerr << "p3NetMgrIMPL::updateNetStateBox_temporal() Updating Refresh Rate, based on changed NatType";
		std::cerr << std::endl;
#endif
		
#ifdef RS_USE_DHT_STUNNER
		switch(natType)
		{
			case RSNET_NATTYPE_RESTRICTED_CONE: 
			{
				if ((natHole == RSNET_NATHOLE_NONE) || (natHole == RSNET_NATHOLE_UNKNOWN))
				{
					mProxyStunner->setRefreshPeriod(NET_STUNNER_PERIOD_FAST);
				}
				else 
				{
					mProxyStunner->setRefreshPeriod(NET_STUNNER_PERIOD_SLOW);
				}
				break;
			}
			case RSNET_NATTYPE_NONE:
			case RSNET_NATTYPE_UNKNOWN:
			case RSNET_NATTYPE_SYMMETRIC:
			case RSNET_NATTYPE_DETERM_SYM:
			case RSNET_NATTYPE_FULL_CONE:
			case RSNET_NATTYPE_OTHER:

				mProxyStunner->setRefreshPeriod(NET_STUNNER_PERIOD_SLOW);
				break;
		}
#endif // RS_USE_DHT_STUNNER

		/* This controls the Attach mode of the DHT... 
		 * which effectively makes the DHT "attach" to Open Nodes.
		 * So that messages can get through.
		 * We only want to be attached - if we don't have a stable DHT port.
		 */
		if ((natHole == RSNET_NATHOLE_NONE) || (natHole == RSNET_NATHOLE_UNKNOWN))
		{
			switch(natType)
			{
				/* switch to attach mode if we have a bad firewall */
				case RSNET_NATTYPE_UNKNOWN:
				case RSNET_NATTYPE_SYMMETRIC:
				case RSNET_NATTYPE_RESTRICTED_CONE: 
				case RSNET_NATTYPE_DETERM_SYM:
				case RSNET_NATTYPE_OTHER:
					netAssistAttach(true);

				break;
				/* switch off attach mode if we have a nice firewall */
				case RSNET_NATTYPE_NONE:
				case RSNET_NATTYPE_FULL_CONE:
					netAssistAttach(false);
				break;
			}
		}
		else
		{
			// Switch off Firewall Mode (Attach)
			netAssistAttach(false);
		}
	}
}



void p3NetMgrIMPL::updateNetStateBox_startup()
{
#ifdef	NETMGR_DEBUG_STATEBOX
	std::cerr << "p3NetMgrIMPL::updateNetStateBox_startup() ";
	std::cerr << std::endl;
#endif
	{
		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/

		/* fill in the data */
		struct sockaddr_storage tmpip;
	
		/* net Assist */
		if (netAssistExtAddress(tmpip))
		{

#ifdef	NETMGR_DEBUG_STATEBOX
			std::cerr << "p3NetMgrIMPL::updateNetStateBox_startup() ";
			std::cerr << "Ext supplied from netAssistExternalAddress()";
			std::cerr << std::endl;
#endif

			if (sockaddr_storage_isValidNet(tmpip))
			{
#ifdef	NETMGR_DEBUG_STATEBOX
				std::cerr << "p3NetMgrIMPL::updateNetStateBox_startup() ";
				std::cerr << "netAssist Returned: " << sockaddr_storage_tostring(tmpip);
				std::cerr << std::endl;
#endif
				mNetStateBox.setAddressUPnP(true, tmpip);
			}
			else
			{
				mNetStateBox.setAddressUPnP(false, tmpip);
#ifdef	NETMGR_DEBUG_STATEBOX
				std::cerr << "p3NetMgrIMPL::updateNetStateBox_startup() ";
				std::cerr << "ERROR Bad Address supplied from netAssistExternalAddress()";
				std::cerr << std::endl;
#endif
			}
		}
		else
		{
#ifdef	NETMGR_DEBUG_STATEBOX
			std::cerr << "p3NetMgrIMPL::updateNetStateBox_startup() ";
			std::cerr << " netAssistExtAddress() is not active";
			std::cerr << std::endl;
#endif
			mNetStateBox.setAddressUPnP(false, tmpip);
		}


		/* ExtAddrFinder */
		if (mUseExtAddrFinder)
		{
			bool extFinderOk = mExtAddrFinder->hasValidIP(tmpip);
			if (extFinderOk)
			{
				sockaddr_storage_setport(tmpip, guessNewExtPort());

#ifdef	NETMGR_DEBUG_STATEBOX
				std::cerr << "p3NetMgrIMPL::updateNetStateBox_startup() ";
				std::cerr << "ExtAddrFinder Returned: " << sockaddr_storage_iptostring(tmpip);
				std::cerr << std::endl;
#endif

				mNetStateBox.setAddressWebIP(true, tmpip);
			}
			else
			{
				mNetStateBox.setAddressWebIP(false, tmpip);
#ifdef	NETMGR_DEBUG_STATEBOX
				std::cerr << "p3NetMgrIMPL::updateNetStateBox_startup() ";
				std::cerr << " ExtAddrFinder hasn't found an address yet";
				std::cerr << std::endl;
#endif
			}
		}
		else
		{
#ifdef	NETMGR_DEBUG_STATEBOX
			std::cerr << "p3NetMgrIMPL::updateNetStateBox_startup() ";
			std::cerr << " ExtAddrFinder is not active";
			std::cerr << std::endl;
#endif
			mNetStateBox.setAddressWebIP(false, tmpip);
		}


		/* finally - if the user has set Forwarded, pass it on */
		if (mNetMode & RS_NET_MODE_TRY_EXT)
		{
			mNetStateBox.setPortForwarded(true, 0); // Port unknown for now.
		}
	}
}

void p3NetMgrIMPL::updateNetStateBox_reset()
{
	{
		RsStackMutex stack(mNetMtx); /****** STACK LOCK MUTEX *******/

		mNetStateBox.reset();

		mOldNatHole = RSNET_NATHOLE_UNKNOWN;
		mOldNatType = RSNET_NATTYPE_UNKNOWN;

	}
}

p3NetMgr::~p3NetMgr() = default;
pqiNetAssist::~pqiNetAssist() = default;
pqiNetAssistPeerShare::~pqiNetAssistPeerShare() = default;
pqiNetAssistConnect::~pqiNetAssistConnect() = default;
