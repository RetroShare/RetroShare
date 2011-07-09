/*
 * libretroshare/src/pqi: p3linkmgr.cc
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

#include "pqi/p3linkmgr.h"

#include "pqi/p3peermgr.h"
#include "pqi/p3netmgr.h"

#include "pqi/authssl.h"
#include "pqi/p3dhtmgr.h" // Only need it for constants.
#include "tcponudp/tou.h"
#include "util/extaddrfinder.h"
#include "util/dnsresolver.h"
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
	 lastcontact(0),
	 connecttype(0),
	 lastavailable(0),
         lastattempt(0),
         name(""),
         state(0), actions(0),
	 source(0), 
	 inConnAttempt(0)
{
	//sockaddr_clear(&currentlocaladdr);
	//sockaddr_clear(&currentserveraddr);

	return;
}

std::string textPeerConnectState(peerConnectState &state)
{
	std::ostringstream out;
	out << "Id: " << state.id << std::endl;


	std::string output = out.str();
	return output;
}



/*********
 * NOTES:
 *
 * p3LinkMgr doesn't store anything. All configuration is handled by p3PeerMgr.
 * 
 * p3LinkMgr recvs the Discovery / Dht / Status updates.... tries the address.
 * at success the address is pushed to p3PeerMgr for storage.
 *
 */


p3LinkMgr::p3LinkMgr(p3PeerMgr *peerMgr, p3NetMgr *netMgr)
	:mPeerMgr(peerMgr), mNetMgr(netMgr), mLinkMtx("p3LinkMgr"),mStatusChanged(false)
{

	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

		/* setup basics of own state */
		mOwnState.id = AuthSSL::getAuthSSL()->OwnId();
		mOwnState.name = AuthGPG::getAuthGPG()->getGPGOwnName();

		// user decided.
		//mOwnState.netMode |= RS_NET_MODE_TRY_UPNP;
	
		mAllowTunnelConnection = false;
		mDNSResolver = new DNSResolver();
		mRetryPeriod = MIN_RETRY_PERIOD;
	
		lastGroupId = 1;

		/* setup Banned Ip Address - static for now 
		 */

		struct in_addr bip;
		memset(&bip, 0, sizeof(bip));
		bip.s_addr = 1;

		mBannedIpList.push_back(bip);
	}
	
#ifdef CONN_DEBUG
	std::cerr << "p3LinkMgr() Startup" << std::endl;
#endif

	return;
}

void p3LinkMgr::setTunnelConnection(bool b)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/
	mAllowTunnelConnection = b;
}

bool p3LinkMgr::getTunnelConnection()
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/
	return mAllowTunnelConnection;
}


void    p3LinkMgr::getOnlineList(std::list<std::string> &ssl_peers)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

        std::map<std::string, peerConnectState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		if (it->second.state & RS_PEER_S_CONNECTED)
		{
			ssl_peers.push_back(it->first);
		}
	}
	return;
}

void    p3LinkMgr::getFriendList(std::list<std::string> &ssl_peers)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

        std::map<std::string, peerConnectState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		ssl_peers.push_back(it->first);
	}
	return;


}

int     p3LinkMgr::getFriendCount()
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	return mFriendList.size();


}

int     p3LinkMgr::getOnlineCount()
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	int count = 0;

        std::map<std::string, peerConnectState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		if (it->second.state & RS_PEER_S_CONNECTED)
		{
			count++;
		}
	}

	return count;

}






void p3LinkMgr::tick()
{
	netTick();
	statusTick();
	tickMonitors();
}

bool p3LinkMgr::shutdown() /* blocking shutdown call */
{
#ifdef CONN_DEBUG
	std::cerr << "p3LinkMgr::shutdown() NOOP";
	std::cerr << std::endl;
#endif

	return true;
}


void    p3LinkMgr::statusTick()
{
	/* iterate through peers ... 
	 * 	if been available for long time ... remove flag
	 * 	if last attempt a while - retryConnect. 
	 * 	etc.
	 */

#ifdef CONN_DEBUG_TICK
	std::cerr << "p3LinkMgr::statusTick()" << std::endl;
#endif
	std::list<std::string> retryIds;
	std::list<std::string>::iterator it2;
        //std::list<std::string> dummyToRemove;

      {
	time_t now = time(NULL);
	time_t oldavail = now - MAX_AVAIL_PERIOD;
        time_t retry = now - mRetryPeriod;

      	RsStackMutex stack(mLinkMtx);  /******   LOCK MUTEX ******/
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
			std::cerr << "p3LinkMgr::statusTick() ONLINE TIMEOUT for: ";
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
		std::cerr << "p3LinkMgr::statusTick() RETRY TIMEOUT for: ";
		std::cerr << *it2;
		std::cerr << std::endl;
#endif
		/* retry it! */
		retryConnect(*it2);
	}

#endif

}


/********************************  Network Status  *********************************
 * Configuration Loading / Saving.
 */

void p3LinkMgr::addMonitor(pqiMonitor *mon)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	std::list<pqiMonitor *>::iterator it;
	it = std::find(clients.begin(), clients.end(), mon);
	if (it != clients.end())
	{
		return;
	}

	mon->setLinkMgr(this);
	clients.push_back(mon);
	return;
}

void p3LinkMgr::removeMonitor(pqiMonitor *mon)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	std::list<pqiMonitor *>::iterator it;
	it = std::find(clients.begin(), clients.end(), mon);
	if (it == clients.end())
	{
		return;
	}
	(*it)->setLinkMgr(NULL);
	clients.erase(it);

	return;
}

void p3LinkMgr::tickMonitors()
{
	bool doStatusChange = false;
	std::list<pqipeer> actionList;
        std::map<std::string, peerConnectState>::iterator it;

      {
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	if (mStatusChanged)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3LinkMgr::tickMonitors() StatusChanged! List:" << std::endl;
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


const std::string p3LinkMgr::getOwnId()
{
                return AuthSSL::getAuthSSL()->OwnId();
}


bool p3LinkMgr::connectAttempt(const std::string &id, struct sockaddr_in &addr,
                                uint32_t &delay, uint32_t &period, uint32_t &type)

{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<std::string, peerConnectState>::iterator it;
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
#ifdef CONN_DEBUG
		std::cerr << "p3LinkMgr::connectAttempt() FAILED Not in FriendList! id: " << id << std::endl;
#endif

		return false;
	}

	if (it->second.connAddrs.size() < 1)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3LinkMgr::connectAttempt() FAILED No ConnectAddresses id: " << id << std::endl;
#endif
		return false;
        }


        if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef CONN_DEBUG
                std::cerr << "p3LinkMgr::connectAttempt() Already FLAGGED as connected!!!!"  << std::endl;
                std::cerr << "p3LinkMgr::connectAttempt() But allowing anyway!!!"  << std::endl;
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
        	std::cerr << "p3LinkMgr::connectAttempt() found an address: id: " << id << std::endl;
		std::cerr << " laddr: " << rs_inet_ntoa(addr.sin_addr) << " lport: " << ntohs(addr.sin_port) << " delay: " << delay << " period: " << period;
		std::cerr << " type: " << type << std::endl;
#endif
        if (addr.sin_addr.s_addr == 0 || addr.sin_port == 0) {
#ifdef CONN_DEBUG
        	std::cerr << "p3LinkMgr::connectAttempt() WARNING: address or port is null" << std::endl;
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

bool p3LinkMgr::connectResult(const std::string &id, bool success, uint32_t flags, struct sockaddr_in remote_peer_address)
{
	bool should_netAssistFriend_false = false ;
	bool should_netAssistFriend_true  = false ;
	bool updatePeerAddr = false;
	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

		rslog(RSL_WARNING, p3connectzone, "p3LinkMgr::connectResult() called Connect!: id: " + id);
		if (success) 
		{
			rslog(RSL_WARNING, p3connectzone, "p3LinkMgr::connectResult() called with SUCCESS.");
		} else 
		{
			rslog(RSL_WARNING, p3connectzone, "p3LinkMgr::connectResult() called with FAILED.");
		}

		if (id == getOwnId()) 
		{
#ifdef CONN_DEBUG
			rslog(RSL_WARNING, p3connectzone, "p3LinkMgr::connectResult() Failed, connecting to own id: ");
#endif
			return false;
		}
		/* check for existing */
		std::map<std::string, peerConnectState>::iterator it;
		it = mFriendList.find(id);
		if (it == mFriendList.end())
		{
#ifdef CONN_DEBUG
			std::cerr << "p3LinkMgr::connectResult() Failed, missing Friend " << " id: " << id << std::endl;
#endif
			return false;
		}

		if (success)
		{
			/* update address (should also come through from DISC) */

#ifdef CONN_DEBUG
			std::cerr << "p3LinkMgr::connectResult() Connect!: id: " << id << std::endl;
			std::cerr << " Success: " << success << " flags: " << flags << std::endl;
#endif

			rslog(RSL_WARNING, p3connectzone, "p3LinkMgr::connectResult() Success");

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

			// THIS TEST IS A Bit BAD XXX, we should update their address anyway...
			// This means we only update connections that we've made.. so maybe not too bad?
			
			if ((it->second.inConnAttempt) &&
					(it->second.currentConnAddrAttempt.addr.sin_addr.s_addr 
					 == remote_peer_address.sin_addr.s_addr) &&
					(it->second.currentConnAddrAttempt.addr.sin_port 
					 == remote_peer_address.sin_port))
			{				
				updatePeerAddr = true;
#ifdef CONN_DEBUG
				std::cerr << "p3LinkMgr::connectResult() adding current peer address in list." << std::endl;
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
			std::cerr << "p3LinkMgr::connectResult() Disconnect/Fail: id: " << id << std::endl;
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
	
	if (updatePeerAddr)
	{
		pqiIpAddress raddr;
		raddr.mAddr = remote_peer_address;
		raddr.mSeenTime = time(NULL);
		raddr.mSrc = 0;

		mPeerMgr->updateCurrentAddress(id, raddr);
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


void    p3LinkMgr::peerStatus(std::string id, const pqiIpAddrSet &addrs,
                       uint32_t type, uint32_t flags, uint32_t source)
{
	/* HACKED UP FIX ****/

        std::map<std::string, peerConnectState>::iterator it;
	bool isFriend = true;

	time_t now = time(NULL);

	peerAddrInfo details;
	details.type    = type;
	details.found   = true;
	details.addrs = addrs;
	details.ts      = now;
	
	bool updateNetConfig = (source == RS_CB_PERSON);	
	uint32_t peerVisibility = 0;
	uint32_t peerNetMode = 0;

	uint32_t ownNetMode = mNetMgr->getNetworkMode();
	
	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	{
		/* Log */
		std::ostringstream out;
		out << "p3LinkMgr::peerStatus()" << " id: " << id;
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
			std::cerr << "p3LinkMgr::peerStatus() Peer Not Found - Ignore" << std::endl;
#endif
			return;
		}
#ifdef CONN_DEBUG
		std::cerr << "p3LinkMgr::peerStatus() Peer is in mOthersList" << std::endl;
#endif
	}

#ifdef CONN_DEBUG
	std::cerr << "p3LinkMgr::peerStatus() Current Peer State:" << std::endl;
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
		 * 
		 * One problem with these states... is that we will never find them via DHT
		 * if these flags are switched off here... If we get a connection attempt via DHT
		 * we should switch the DHT search back on.
		 */

		peerNetMode = 0; //it->second.netMode &= (~RS_NET_MODE_ACTUAL); /* clear actual flags */
		if (flags & RS_NET_FLAGS_EXTERNAL_ADDR)
		{
			peerNetMode = RS_NET_MODE_EXT;
		}
		else if (flags & RS_NET_FLAGS_STABLE_UDP)
		{
			peerNetMode = RS_NET_MODE_UDP;
		}
		else
		{
			peerNetMode = RS_NET_MODE_UNREACHABLE;
		}


		/* always update VIS status */
		if (flags & RS_NET_FLAGS_USE_DISC)
		{
			peerVisibility &= (~RS_VIS_STATE_NODISC);
		}
		else
		{
			peerVisibility |= RS_VIS_STATE_NODISC;
		}

		if (flags & RS_NET_FLAGS_USE_DHT)
		{
			peerVisibility &= (~RS_VIS_STATE_NODHT);
		}
		else
		{
			peerVisibility |= RS_VIS_STATE_NODHT;
		}

		
	}

	/* Determine Reachability (only advisory) */
	if (ownNetMode & RS_NET_MODE_UDP)
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
	else if (ownNetMode & RS_NET_MODE_UNREACHABLE)
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
		std::cerr << "p3LinkMgr::peerStatus() NOT FRIEND " << " id: " << id << std::endl;
#endif

		{
			rslog(RSL_WARNING, p3connectzone, "p3LinkMgr::peerStatus() NO CONNECT (not friend)");
		}
		return;
	}

	/* if already connected -> done */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3LinkMgr::peerStatus() PEER ONLINE ALREADY " << " id: " << id << std::endl;
#endif
		{
			/* Log */
			rslog(RSL_WARNING, p3connectzone, "p3LinkMgr::peerStatus() NO CONNECT (already connected!)");
		}

		return;
	}


      } /****** STACK UNLOCK MUTEX *******/
	
	bool newAddrs = mPeerMgr->updateAddressList(id, addrs);
	if (updateNetConfig)
	{
		mPeerMgr -> setVisState(id, peerVisibility);
		mPeerMgr -> setNetworkMode(id, peerNetMode);
	}

#ifdef CONN_DEBUG
	std::cerr << "p3LinkMgr::peerStatus()" << " id: " << id;
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
	std::cerr << "p3LinkMgr::peerStatus() Resulting Peer State:" << std::endl;
	printConnectState(std::cerr, it->second);
	std::cerr << std::endl;
#endif

}

void    p3LinkMgr::peerConnectRequest(std::string id, struct sockaddr_in raddr,
                       							uint32_t source)
{
#ifdef CONN_DEBUG
	std::cerr << "p3LinkMgr::peerConnectRequest() id: " << id << " raddr: " << rs_inet_ntoa(raddr.sin_addr) << ":" << ntohs(raddr.sin_port);
	std::cerr << " source: " << source << std::endl;
#endif
	{
		/* Log */
		std::ostringstream out;
		out << "p3LinkMgr::peerConnectRequest() id: " << id << " raddr: " << rs_inet_ntoa(raddr.sin_addr);
		out << ":" << ntohs(raddr.sin_port) << " source: " << source;
		rslog(RSL_WARNING, p3connectzone, out.str());
	}

	/******************** TCP PART *****************************/

#ifdef CONN_DEBUG
	std::cerr << "p3LinkMgr::peerConnectRequest() Try TCP first" << std::endl;
#endif

	if (source == RS_CB_DHT)
	{
		std::cerr << "p3LinkMgr::peerConnectRequest() source DHT ==> retryConnectUDP()" << std::endl;
		retryConnectUDP(id, raddr);
		return;
	}
	else
	{	// IS THIS USED???
		std::cerr << "p3LinkMgr::peerConnectRequest() source OTHER ==> retryConnect()" << std::endl;

		retryConnect(id);
		return;
	}
}


/*******************************************************************/
/*******************************************************************/
       /*************** External Control ****************/
bool   p3LinkMgr::retryConnect(const std::string &id)
{
	/* push all available addresses onto the connect addr stack */
#ifdef CONN_DEBUG
	std::cerr << "p3LinkMgr::retryConnect() id: " << id << std::endl;
#endif

#ifndef P3CONNMGR_NO_TCP_CONNECTIONS

	retryConnectTCP(id);

#endif  // P3CONNMGR_NO_TCP_CONNECTIONS

	return true;
}



bool   p3LinkMgr::retryConnectUDP(const std::string &id, struct sockaddr_in &rUdpAddr)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	/* push all available addresses onto the connect addr stack */
#ifdef CONN_DEBUG
	std::cerr << "p3LinkMgr::retryConnectTCP() id: " << id << std::endl;
#endif

        if (id == getOwnId()) {
            #ifdef CONN_DEBUG
            rslog(RSL_WARNING, p3connectzone, "p3LinkMgr::retryConnectUDP() Failed, connecting to own id: ");
            #endif
            return false;
        }

        /* look up the id */
        std::map<std::string, peerConnectState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
#ifdef CONN_DEBUG
               std::cerr << "p3LinkMgr::retryConnectUDP() Peer is not Friend" << std::endl;
#endif
		return false;
	}

	/* if already connected -> done */
        if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef CONN_DEBUG
		std::cerr << "p3LinkMgr::retryConnectUDP() Peer Already Connected" << std::endl;
#endif
                if (it->second.connecttype & RS_NET_CONN_TUNNEL) {
#ifdef CONN_DEBUG
                    std::cerr << "p3LinkMgr::retryConnectUDP() Peer Connected through a tunnel connection, let's try a normal connection." << std::endl;
#endif
                } else {
#ifdef CONN_DEBUG
                    	std::cerr << "p3LinkMgr::retryConnectUDP() Peer Connected no more connection attempts" << std::endl;
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





bool   p3LinkMgr::retryConnectTCP(const std::string &id)
{
	/* Check if we should retry first */
	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/
	
		/* push all available addresses onto the connect addr stack...
		 * with the following exceptions:
	   	 *   - check local address, see if it is the same network as us
		     - check address age. don't add old ones
		 */
	
#ifdef CONN_DEBUG
		std::cerr << "p3LinkMgr::retryConnectTCP() id: " << id << std::endl;
#endif
	
		if (id == getOwnId()) 
		{
#ifdef CONN_DEBUG
			rslog(RSL_WARNING, p3connectzone, "p3LinkMgr::retryConnectTCP() Failed, connecting to own id: ");
#endif
			return false;
		}
	
	        /* look up the id */
	        std::map<std::string, peerConnectState>::iterator it;
		if (mFriendList.end() == (it = mFriendList.find(id)))
		{
#ifdef CONN_DEBUG
			std::cerr << "p3LinkMgr::retryConnectTCP() Peer is not Friend" << std::endl;
#endif
			return false;
		}
	
		/* if already connected -> done */
		if (it->second.state & RS_PEER_S_CONNECTED)
		{
#ifdef CONN_DEBUG
			std::cerr << "p3LinkMgr::retryConnectTCP() Peer Already Connected" << std::endl;
#endif
			if (it->second.connecttype & RS_NET_CONN_TUNNEL) 
			{
#ifdef CONN_DEBUG
				std::cerr << "p3LinkMgr::retryConnectTCP() Peer Connected through a tunnel connection, let's try a normal connection." << std::endl;
#endif
			} 
			else 
			{
#ifdef CONN_DEBUG
				std::cerr << "p3LinkMgr::retryConnectTCP() Peer Connected no more connection attempts" << std::endl;
#endif
				return false;
			}
		}
	} /****** END of LOCKED ******/

	/* If we reach here, must retry .... extract the required info from p3PeerMgr */

	struct sockaddr_in lAddr;
	struct sockaddr_in eAddr;
	pqiIpAddrSet histAddrs;
	std::string dyndns;

	if (mPeerMgr->setConnectAddresses(id, lAddr, eAddr, histAddrs, dyndns))
	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	        std::map<std::string, peerConnectState>::iterator it;
		if (mFriendList.end() != (it = mFriendList.find(id)))
		{

			locked_ConnectAttempt_CurrentAddresses(&(it->second), &lAddr, &eAddr);
			locked_ConnectAttempt_HistoricalAddresses(&(it->second), histAddrs);

			uint16_t dynPort = ntohs(eAddr.sin_port);
			if (!dynPort)
				dynPort = ntohs(lAddr.sin_port);
			if (dynPort)
			{
				locked_ConnectAttempt_AddDynDNS(&(it->second), dyndns, dynPort);
			}

			//locked_ConnectAttempt_AddTunnel(&(it->second));
	
			/* finish it off */
			return locked_ConnectAttempt_Complete(&(it->second));
		}
	}

	return false;

}


#define MAX_TCP_ADDR_AGE	(3600 * 24 * 14) // two weeks in seconds.

bool  p3LinkMgr::locked_CheckPotentialAddr(const struct sockaddr_in *addr, time_t age)
{
#ifdef CONN_DEBUG
	std::cerr << "p3LinkMgr::locked_CheckPotentialAddr("; 
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
		std::cerr << "p3LinkMgr::locked_CheckPotentialAddr() REJECTING - TOO OLD"; 
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
		std::cerr << "p3LinkMgr::locked_CheckPotentialAddr() REJECTING - INVALID";
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
			std::cerr << "p3LinkMgr::locked_CheckPotentialAddr() REJECTING - ON BANNED IPLIST"; 
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
		std::cerr << "p3LinkMgr::locked_CheckPotentialAddr() ACCEPTING - EXTERNAL"; 
		std::cerr << std::endl;
#endif
		return true;
	}


	/* get here, it is private or loopback 
	 *  - can only connect to these addresses if we are on the same subnet.
	    - check net against our local address.
	 */

	std::cerr << "p3LinkMgr::locked_CheckPotentialAddr() Checking sameNet against: "; 
	std::cerr << rs_inet_ntoa(mLocalAddress.sin_addr);
	std::cerr << ")";
	std::cerr << std::endl;

	if (sameNet(&(mLocalAddress.sin_addr), &(addr->sin_addr)))
	{
#ifdef CONN_DEBUG
		std::cerr << "p3LinkMgr::locked_CheckPotentialAddr() ACCEPTING - PRIVATE & sameNET"; 
		std::cerr << std::endl;
#endif
		return true;
	}

#ifdef CONN_DEBUG
	std::cerr << "p3LinkMgr::locked_CheckPotentialAddr() REJECTING - PRIVATE & !sameNET"; 
	std::cerr << std::endl;
#endif

	/* else it fails */
	return false;

}


void  p3LinkMgr::locked_ConnectAttempt_CurrentAddresses(peerConnectState *peer, struct sockaddr_in *localAddr, struct sockaddr_in *serverAddr)
{
	// Just push all the addresses onto the stack.
	/* try "current addresses" first */
	if ((localAddr) && (locked_CheckPotentialAddr(localAddr, 0)))
	{
#ifdef CONN_DEBUG
		std::cerr << "Adding tcp connection attempt: ";
		std::cerr << "Current Local Addr: " << rs_inet_ntoa(localAddr->sin_addr);
		std::cerr << ":" << ntohs(localAddr->sin_port);
		std::cerr << std::endl;
#endif
		peerConnectAddress pca;
		pca.addr = *localAddr;
		pca.type = RS_NET_CONN_TCP_LOCAL;
		pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
		pca.ts = time(NULL);
		pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;

		addAddressIfUnique(peer->connAddrs, pca);
	}

	if ((serverAddr) && (locked_CheckPotentialAddr(serverAddr, 0)))
	{
#ifdef CONN_DEBUG
		std::cerr << "Adding tcp connection attempt: ";
		std::cerr << "Current Ext Addr: " << rs_inet_ntoa(serverAddr->sin_addr);
		std::cerr << ":" << ntohs(serverAddr->sin_port);
		std::cerr << std::endl;
#endif
		peerConnectAddress pca;
		pca.addr = *serverAddr;
		pca.type = RS_NET_CONN_TCP_EXTERNAL;
		pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
		pca.ts = time(NULL);
		pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;

		addAddressIfUnique(peer->connAddrs, pca);
	}
}


void  p3LinkMgr::locked_ConnectAttempt_HistoricalAddresses(peerConnectState *peer, const pqiIpAddrSet &ipAddrs)
{
	/* now try historical addresses */
	/* try local addresses first */
	std::list<pqiIpAddress>::const_iterator ait;
	time_t now = time(NULL);

	for(ait = ipAddrs.mLocal.mAddrs.begin(); 
		ait != ipAddrs.mLocal.mAddrs.end(); ait++)
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

	for(ait = ipAddrs.mExt.mAddrs.begin(); 
		ait != ipAddrs.mExt.mAddrs.end(); ait++)
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


void  p3LinkMgr::locked_ConnectAttempt_AddDynDNS(peerConnectState *peer, std::string dyndns, uint16_t port)
{
	/* try dyndns address too */
	if (!dyndns.empty()) 
	{
		struct in_addr addr;
#ifdef CONN_DEBUG
		std::cerr << "Looking up DynDNS address" << std::endl;
#endif
		if (port) 
		{
			if(mDNSResolver->getIPAddressFromString(dyndns, addr))
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
				pca.addr.sin_port = htons(port);
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


void  p3LinkMgr::locked_ConnectAttempt_AddTunnel(peerConnectState *peer)
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


bool  p3LinkMgr::addAddressIfUnique(std::list<peerConnectAddress> &addrList, peerConnectAddress &pca)
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



bool  p3LinkMgr::locked_ConnectAttempt_Complete(peerConnectState *peer)
{

	/* flag as last attempt to prevent loop */
	//add a random perturbation between 0 and 2 sec.
	peer->lastattempt = time(NULL) + rand() % MAX_RANDOM_ATTEMPT_OFFSET; 

	if (peer->inConnAttempt) 
	{
                /*  -> it'll automatically use the addresses we added */
#ifdef CONN_DEBUG
		std::cerr << "p3LinkMgr::locked_ConnectAttempt_Complete() Already in CONNECT ATTEMPT";
		std::cerr << std::endl;
		std::cerr << "p3LinkMgr::locked_ConnectAttempt_Complete() Remaining ConnAddr Count: " << peer->connAddrs.size();
		std::cerr << std::endl;
#endif
		return true;
	}

	/* start a connection attempt */
	if (peer->connAddrs.size() > 0) 
	{
#ifdef CONN_DEBUG
		std::ostringstream out;
		out << "p3LinkMgr::locked_ConnectAttempt_Complete() Started CONNECT ATTEMPT! " ;
	    out << std::endl;
		out << "p3LinkMgr::locked_ConnectAttempt_Complete() ConnAddr Count: " << peer->connAddrs.size();
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
		out << "p3LinkMgr::locked_ConnectAttempt_Complete() No addr in the connect attempt list. Not suitable for CONNECT ATTEMPT! ";
		rslog(RSL_DEBUG_ALERT, p3connectzone, out.str());
	    std::cerr << out.str() << std::endl;
#endif
	    return false;
	}
	return false;
}



