/*******************************************************************************
 * libretroshare/src/pqi: p3linkmgr.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2011 by Robert Fernie.                                       *
 * Copyright (C) 2015-2018  Gioacchino Mazzurco <gio@eigenlab.org>             *
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

#include "pqi/p3linkmgr.h"

#include "pqi/p3peermgr.h"
#include "pqi/p3netmgr.h"

#include "rsserver/p3face.h"
#include "pqi/authssl.h"
#include "pqi/p3dhtmgr.h" // Only need it for constants.
#include "tcponudp/tou.h"
#include "util/extaddrfinder.h"
#include "util/dnsresolver.h"
#include "util/rsnet.h"
#include "pqi/authgpg.h"


#include "util/rsprint.h"
#include "util/rsdebug.h"
#include "util/rsstring.h"

#include "rsitems/rsconfigitems.h"

#include "retroshare/rsiface.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsdht.h"
#include "retroshare/rsbanlist.h"

/* Network setup States */

static struct RsLog::logInfo p3connectzoneInfo = {RsLog::Default, "p3connect"};
#define p3connectzone &p3connectzoneInfo

/****
 * #define LINKMGR_DEBUG 1
 * #define LINKMGR_DEBUG_LOG	1
 * #define LINKMGR_DEBUG_CONNFAIL 1
 * #define LINKMGR_DEBUG_ACTIONS  1
 * #define LINKMGR_DEBUG_LINKTYPE	1
 ***/

/****
 * #define DISABLE_UDP_CONNECTIONS	1		
 ***/

/****
 * #define P3CONNMGR_NO_TCP_CONNECTIONS 1
 ***/
/****
 * #define P3CONNMGR_NO_AUTO_CONNECTION 1
 ***/

//#define P3CONNMGR_NO_TCP_CONNECTIONS 1

const uint32_t P3CONNMGR_TCP_DEFAULT_DELAY = 3; /* 2 Seconds? is it be enough! */
//const uint32_t P3CONNMGR_UDP_DEFAULT_DELAY = 3; /* 2 Seconds? is it be enough! */

const uint32_t P3CONNMGR_TCP_DEFAULT_PERIOD = 10;
const uint32_t P3CONNMGR_UDP_DEFAULT_PERIOD = 30;  // this represents how long it stays at the default TTL (4), before rising.

#define MAX_AVAIL_PERIOD 230 //times a peer stay in available state when not connected
#define MIN_RETRY_PERIOD 140

#define MAX_RANDOM_ATTEMPT_OFFSET	6 // seconds.

void  printConnectState(std::ostream &out, peerConnectState &peer);

peerConnectAddress::peerConnectAddress()
	:delay(0), period(0), type(0), flags(0), ts(0), bandwidth(0), domain_port(0)
{
	sockaddr_storage_clear(addr);
	sockaddr_storage_clear(proxyaddr);
	sockaddr_storage_clear(srcaddr);
}


peerAddrInfo::peerAddrInfo()
	:found(false), type(0), ts(0)
{
}

peerConnectState::peerConnectState()
	: connecttype(0),
	 lastavailable(0),
         lastattempt(0),
         name(""),
         state(0), actions(0),
	 source(0), 
	 inConnAttempt(0), 
	 wasDeniedConnection(false), deniedTS(false), deniedInConnAttempt(false)
{
}

std::string textPeerConnectState(peerConnectState &state)
{
    return "Id: " + state.id.toStdString() + "\n";
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


p3LinkMgrIMPL::p3LinkMgrIMPL(p3PeerMgrIMPL *peerMgr, p3NetMgrIMPL *netMgr)
	:mPeerMgr(peerMgr), mNetMgr(netMgr), mLinkMtx("p3LinkMgr"),mStatusChanged(false)
{

	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/
	
		mDNSResolver = new DNSResolver();
		mRetryPeriod = MIN_RETRY_PERIOD;
	
		/* setup Banned Ip Address - static for now 
		 */

		struct sockaddr_storage bip;
		sockaddr_storage_clear(bip);
        struct sockaddr_in *addr = (struct sockaddr_in *) &bip;
		addr->sin_family = AF_INET;
		addr->sin_addr.s_addr = 1;
		addr->sin_port = htons(0);
		
		mBannedIpList.push_back(bip);
	}
	
#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgr() Startup" << std::endl;
#endif

	return;
}

p3LinkMgrIMPL::~p3LinkMgrIMPL()
{
	delete(mDNSResolver);
}

bool    p3LinkMgrIMPL::setLocalAddress(const struct sockaddr_storage &addr)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/
	mLocalAddress = addr;

	return true ;
}

bool p3LinkMgrIMPL::getLocalAddress(struct sockaddr_storage &addr)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/
	addr = mLocalAddress;
	return true;
}


bool    p3LinkMgrIMPL::isOnline(const RsPeerId &ssl_id)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

        std::map<RsPeerId, peerConnectState>::iterator it;
	it = mFriendList.find(ssl_id);
	if (it == mFriendList.end())
	{
		return false;
	}

	if (it->second.state & RS_PEER_S_CONNECTED)
	{
		return true;
	}
	return false;
}



uint32_t p3LinkMgrIMPL::getLinkType(const RsPeerId &ssl_id)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

        std::map<RsPeerId, peerConnectState>::iterator it;
	it = mFriendList.find(ssl_id);
	if (it == mFriendList.end())
	{
		return 0;
	}

	if (it->second.state & RS_PEER_S_CONNECTED)
	{
		return it->second.linkType;
	}
	return 0;
}



void    p3LinkMgrIMPL::getOnlineList(std::list<RsPeerId> &ssl_peers)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

        std::map<RsPeerId, peerConnectState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
	{
		if (it->second.state & RS_PEER_S_CONNECTED)
		{
			ssl_peers.push_back(it->first);
		}
	}
	return;
}

void    p3LinkMgrIMPL::getFriendList(std::list<RsPeerId> &ssl_peers)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	std::map<RsPeerId, peerConnectState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
	{
		ssl_peers.push_back(it->first);
	}
	return;
}

bool    p3LinkMgrIMPL::getPeerName(const RsPeerId &ssl_id, std::string &name)
{
	return mPeerMgr->getPeerName(ssl_id, name);
}


bool    p3LinkMgrIMPL::getFriendNetStatus(const RsPeerId &id, peerConnectState &state)
{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

        std::map<RsPeerId, peerConnectState>::iterator it;
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
		return false;
	}

	state = it->second;

	return true;
}



void    p3LinkMgrIMPL::setFriendVisibility(const RsPeerId &id, bool isVisible)
{
	/* set visibility */
	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/
	
		std::map<RsPeerId, peerConnectState>::iterator it;
		it = mFriendList.find(id);
		if (it == mFriendList.end())
		{
			/* */
			std::cerr << "p3LinkMgrIMPL::setFriendVisibility() ERROR peer unknown: " << id;
			std::cerr << std::endl;
			return;
		}

		if (it->second.dhtVisible == isVisible)
		{
			/* no change in state */
			return;
		}
	
		it->second.dhtVisible = isVisible;
	
		if (it->second.state & RS_PEER_S_CONNECTED)
		{
			/* dont worry about it */
			return;
		}
	}

	/* switch the NetAssistOn/Off */
	mNetMgr->netAssistFriend(id, isVisible);
}
	

void p3LinkMgrIMPL::tick()
{
	statusTick();
	tickMonitors();
}


void    p3LinkMgrIMPL::statusTick()
{
	/* iterate through peers ... 
	 * 	if been available for long time ... remove flag
	 * 	if last attempt a while - retryConnect. 
	 * 	etc.
	 */

#ifdef LINKMGR_DEBUG_TICK
	std::cerr << "p3LinkMgrIMPL::statusTick()" << std::endl;
#endif
	std::list<RsPeerId> retryIds;
        //std::list<std::string> dummyToRemove;

      {
	rstime_t now = time(NULL);
	rstime_t oldavail = now - MAX_AVAIL_PERIOD;
        rstime_t retry = now - mRetryPeriod;

      	RsStackMutex stack(mLinkMtx);  /******   LOCK MUTEX ******/
        std::map<RsPeerId, peerConnectState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
	{
		if (it->second.state & RS_PEER_S_CONNECTED)
		{
			continue;
		}

		if ((it->second.state & RS_PEER_S_ONLINE) &&
			(it->second.lastavailable < oldavail))
		{
#ifdef LINKMGR_DEBUG_TICK
			std::cerr << "p3LinkMgrIMPL::statusTick() ONLINE TIMEOUT for: ";
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
	std::list<RsPeerId>::iterator it2;
	for(it2 = retryIds.begin(); it2 != retryIds.end(); ++it2)
	{
#ifdef LINKMGR_DEBUG_TICK
		std::cerr << "p3LinkMgrIMPL::statusTick() RETRY TIMEOUT for: ";
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

void p3LinkMgrIMPL::addMonitor(pqiMonitor *mon)
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

void p3LinkMgrIMPL::removeMonitor(pqiMonitor *mon)
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

void p3LinkMgrIMPL::tickMonitors()
{
	bool doStatusChange = false;
	std::list<pqipeer> actionList;
        std::map<RsPeerId, peerConnectState>::iterator it;

      {
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	if (mStatusChanged)
	{
#ifdef LINKMGR_DEBUG_ACTIONS
		std::cerr << "p3LinkMgrIMPL::tickMonitors() StatusChanged! List:" << std::endl;
#endif
		/* assemble list */
		for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
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

#ifdef LINKMGR_DEBUG_ACTIONS
				std::cerr << "Friend: " << peer.name << " Id: " << peer.id.toStdString() << " State: " << peer.state;
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
					p3Notify *notify = RsServer::notify();
					if (notify)
					{
						notify->AddPopupMessage(RS_POPUP_CONNECT, peer.id.toStdString(),"", "Online: ");
						notify->AddFeedItem(RS_FEED_ITEM_PEER_CONNECT, peer.id.toStdString());
					}
				}
			}
		}

		/* do the Others as well! */
		for(it = mOthersList.begin(); it != mOthersList.end(); ++it)
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

#ifdef LINKMGR_DEBUG_ACTIONS
				std::cerr << "Other: " << peer.name << " Id: " << peer.id.toStdString() << " State: " << peer.state;
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
#ifdef LINKMGR_DEBUG_ACTIONS
		std::cerr << "Sending to " << clients.size() << " monitorClients" << std::endl;
#endif
	
		/* send to all monitors */
		std::list<pqiMonitor *>::iterator mit;
		for(mit = clients.begin(); mit != clients.end(); ++mit)
		{
			(*mit)->statusChange(actionList);
		}
	}

#ifdef WINDOWS_SYS
///////////////////////////////////////////////////////////
// hack for too many connections

	/* notify all monitors */
	std::list<pqiMonitor *>::iterator mit;
	for(mit = clients.begin(); mit != clients.end(); ++mit) {
		(*mit)->statusChanged();
	}

///////////////////////////////////////////////////////////
#endif

	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

		/* Now Cleanup OthersList (served its purpose (MOVE Action)) */
		mOthersList.clear();
	}

}


const RsPeerId& p3LinkMgrIMPL::getOwnId()
{
	return AuthSSL::getAuthSSL()->OwnId();
}


bool p3LinkMgrIMPL::connectAttempt(const RsPeerId &id, struct sockaddr_storage &raddr,
								   struct sockaddr_storage &proxyaddr,
								   struct sockaddr_storage &srcaddr,
								   uint32_t &delay, uint32_t &period, uint32_t &type, uint32_t &flags, uint32_t &bandwidth,
								   std::string &domain_addr, uint16_t &domain_port)

{
	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<RsPeerId, peerConnectState>::iterator it;
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::connectAttempt() FAILED Not in FriendList! id: " << id << std::endl;
#endif

		return false;
	}

	if (it->second.connAddrs.size() < 1)
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::connectAttempt() FAILED No ConnectAddresses id: " << id << std::endl;
#endif
		return false;
        }


        if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef LINKMGR_DEBUG
                std::cerr << "p3LinkMgrIMPL::connectAttempt() Already FLAGGED as connected!!!!"  << std::endl;
                std::cerr << "p3LinkMgrIMPL::connectAttempt() But allowing anyway!!!"  << std::endl;
#endif

		rslog(RSL_WARNING, p3connectzone, "p3LinkMgrIMPL::connectAttempt() ERROR ALREADY CONNECTED");

         }

#ifdef LINKMGR_DEBUG_LOG
	rslog(RSL_WARNING, p3connectzone, "p3LinkMgrIMPL::connectAttempt() called id: " + id.toStdString());
#endif

        it->second.lastattempt = time(NULL); 
        it->second.inConnAttempt = true;
        it->second.currentConnAddrAttempt = it->second.connAddrs.front();
	it->second.connAddrs.pop_front();

	raddr = it->second.currentConnAddrAttempt.addr;
	delay = it->second.currentConnAddrAttempt.delay;
	period = it->second.currentConnAddrAttempt.period;
	type = it->second.currentConnAddrAttempt.type;
	flags = it->second.currentConnAddrAttempt.flags;

	proxyaddr = it->second.currentConnAddrAttempt.proxyaddr;
	srcaddr = it->second.currentConnAddrAttempt.srcaddr;
	bandwidth = it->second.currentConnAddrAttempt.bandwidth;

	domain_addr = it->second.currentConnAddrAttempt.domain_addr;
	domain_port = it->second.currentConnAddrAttempt.domain_port;

	/********* Setup LinkType parameters **********/

#define TRICKLE_LIMIT		2001		// 2kb
#define LOW_BANDWIDTH_LIMIT	5001		// 5kb

#ifdef LINKMGR_DEBUG_LINKTYPE
	std::cerr << "p3LinkMgrIMPL::connectAttempt() Setting up LinkType" << std::endl;
	std::cerr << "p3LinkMgrIMPL::connectAttempt() type = " << type << std::endl;
#endif

	it->second.linkType = 0;
	if (type & RS_NET_CONN_TCP_ALL)
	{
#ifdef LINKMGR_DEBUG_LINKTYPE
		std::cerr << "p3LinkMgrIMPL::connectAttempt() type & TCP_ALL => TCP_UNKNOWN" << std::endl;
#endif
		it->second.linkType |= RS_NET_CONN_TRANS_TCP_UNKNOWN;
	}
	else if (type & RS_NET_CONN_UDP_ALL)
	{
		if (flags & RS_CB_FLAG_MODE_UDP_DIRECT)
		{
#ifdef LINKMGR_DEBUG_LINKTYPE
			std::cerr << "p3LinkMgrIMPL::connectAttempt() type & UDP_ALL && flags & DIRECT => UDP_DIRECT" << std::endl;
#endif
			it->second.linkType |= RS_NET_CONN_TRANS_UDP_DIRECT;
		}
		else if (flags & RS_CB_FLAG_MODE_UDP_PROXY)
		{
#ifdef LINKMGR_DEBUG_LINKTYPE
			std::cerr << "p3LinkMgrIMPL::connectAttempt() type & UDP_ALL && flags & PROXY => UDP_PROXY" << std::endl;
#endif
			it->second.linkType |= RS_NET_CONN_TRANS_UDP_PROXY;
		}
		else if (flags & RS_CB_FLAG_MODE_UDP_RELAY)
		{
#ifdef LINKMGR_DEBUG_LINKTYPE
			std::cerr << "p3LinkMgrIMPL::connectAttempt() type & UDP_ALL && flags & RELAY => UDP_RELAY" << std::endl;
#endif
			it->second.linkType |= RS_NET_CONN_TRANS_UDP_RELAY;
		}
		else 
		{
#ifdef LINKMGR_DEBUG_LINKTYPE
			std::cerr << "p3LinkMgrIMPL::connectAttempt() type & UDP_ALL && else => UDP_UNKNOWN" << std::endl;
#endif
			it->second.linkType |= RS_NET_CONN_TRANS_UDP_UNKNOWN;
		}
	}
	else
	{
#ifdef LINKMGR_DEBUG_LINKTYPE
		std::cerr << "p3LinkMgrIMPL::connectAttempt() else => TRANS_UNKNOWN" << std::endl;
#endif
		it->second.linkType |= RS_NET_CONN_TRANS_UNKNOWN;
	}

	if ((type & RS_NET_CONN_UDP_ALL) && (flags & RS_CB_FLAG_MODE_UDP_RELAY))
	{
		if (bandwidth < TRICKLE_LIMIT)
		{
#ifdef LINKMGR_DEBUG_LINKTYPE
			std::cerr << "p3LinkMgrIMPL::connectAttempt() flags & RELAY && band < TRICKLE => SPEED_TRICKLE" << std::endl;
#endif
			it->second.linkType |= RS_NET_CONN_SPEED_TRICKLE;
		}
		else if (bandwidth < LOW_BANDWIDTH_LIMIT)
		{
#ifdef LINKMGR_DEBUG_LINKTYPE
			std::cerr << "p3LinkMgrIMPL::connectAttempt() flags & RELAY && band < LOW => SPEED_LOW" << std::endl;
#endif
			it->second.linkType |= RS_NET_CONN_SPEED_LOW;
		}
		else
		{
#ifdef LINKMGR_DEBUG_LINKTYPE
			std::cerr << "p3LinkMgrIMPL::connectAttempt() flags & RELAY && else => SPEED_NORMAL" << std::endl;
#endif
			it->second.linkType |= RS_NET_CONN_SPEED_NORMAL;
		}
	}
	else
	{ 
#ifdef LINKMGR_DEBUG_LINKTYPE
		std::cerr << "p3LinkMgrIMPL::connectAttempt() else => SPEED_NORMAL" << std::endl;
#endif
		it->second.linkType |= RS_NET_CONN_SPEED_NORMAL;
	}

	uint32_t connType = mPeerMgr->getConnectionType(id);
	it->second.linkType |= connType;

#ifdef LINKMGR_DEBUG_LINKTYPE
	std::cerr << "p3LinkMgrIMPL::connectAttempt() connType: " << connType << std::endl;
	std::cerr << "p3LinkMgrIMPL::connectAttempt() final LinkType: " << it->second.linkType << std::endl;


        std::cerr << "p3LinkMgrIMPL::connectAttempt() found an address: id: " << id << std::endl;
	std::cerr << " laddr: " << sockaddr_storage_tostring(raddr) << " delay: " << delay << " period: " << period;
	std::cerr << " type: " << type << std::endl;
	std::cerr << "p3LinkMgrIMPL::connectAttempt() set LinkType to: " << it->second.linkType << std::endl;
#endif


	/********* Setup LinkType parameters **********/

#ifdef LINKMGR_DEBUG
        	std::cerr << "p3LinkMgrIMPL::connectAttempt() found an address: id: " << id << std::endl;
        std::cerr << " laddr: " << sockaddr_storage_tostring(raddr) << " delay: " << delay << " period: " << period;
		std::cerr << " type: " << type << std::endl;
        	std::cerr << "p3LinkMgrIMPL::connectAttempt() set LinkType to: " << it->second.linkType << std::endl;
#endif
        if (sockaddr_storage_isnull(raddr)) {
#ifdef LINKMGR_DEBUG
        	std::cerr << "p3LinkMgrIMPL::connectAttempt() WARNING: address or port is null" << std::endl;
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

bool p3LinkMgrIMPL::connectResult(const RsPeerId &id, bool success, bool isIncomingConnection, uint32_t flags, const struct sockaddr_storage &remote_peer_address)
{
	bool doDhtAssist = false ;
	bool updatePeerAddr = false;
	bool updateLastContact = false;

	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/


		if (id == getOwnId()) 
		{
			rslog(RSL_ALERT, p3connectzone, "p3LinkMgrIMPL::connectResult() ERROR Trying to Connect to OwnId: " + id.toStdString());

			return false;
		}
		/* check for existing */
		std::map<RsPeerId, peerConnectState>::iterator it;
		it = mFriendList.find(id);
		if (it == mFriendList.end())
		{
			rslog(RSL_ALERT, p3connectzone, "p3LinkMgrIMPL::connectResult() ERROR Missing Friend: " + id.toStdString());

#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::connectResult() ERROR, missing Friend " << " id: " << id << std::endl;
#endif
			return false;
		}

		/* now we can tell if we think we were connected - proper point to log */

		{
			std::string out = "p3LinkMgrIMPL::connectResult() id: " + id.toStdString();
			if (success) 
			{
				out += " SUCCESS ";
				if (it->second.state & RS_PEER_S_CONNECTED)
				{
					out += " WARNING: State says: Already Connected";
				}
			} 
			else 
			{
				if (it->second.state & RS_PEER_S_CONNECTED)
				{
					out += " FAILURE OF THE CONNECTION (Was Connected)";
				}
				else
				{
					out += " FAILED ATTEMPT (Not Connected)";
				}
			}
#ifdef LINKMGR_DEBUG_LOG
			rslog(RSL_WARNING, p3connectzone, out);
#endif
		}



		if (success)
		{
			/* update address (should also come through from DISC) */
#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::connectResult() Connect!: id: " << id << std::endl;
            std::cerr << " Success: " << success << " flags: " << flags << ", remote IP = " <<  sockaddr_storage_iptostring(remote_peer_address) << std::endl;
#endif

#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::connectResult() Connect!: id: " << id << std::endl;
			std::cerr << " Success: " << success << " flags: " << flags << std::endl;

			rslog(RSL_WARNING, p3connectzone, "p3LinkMgrIMPL::connectResult() Success");
#endif

			/* change state */
			it->second.state |= RS_PEER_S_CONNECTED;
			it->second.actions |= RS_PEER_CONNECTED;
			it->second.connecttype = flags;
			it->second.connectaddr = remote_peer_address;

			it->second.actAsServer = isIncomingConnection;

			updateLastContact = true; /* time of connect */

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
					(sockaddr_storage_same(it->second.currentConnAddrAttempt.addr, remote_peer_address)))
			{				
				updatePeerAddr = true;
#ifdef LINKMGR_DEBUG
				std::cerr << "p3LinkMgrIMPL::connectResult() adding current peer address in list." << std::endl;
#endif
			}

			/* remove other attempts */
			it->second.inConnAttempt = false;
			it->second.connAddrs.clear();
			mStatusChanged = true;
		}
		else
		{
#ifdef LINKMGR_DEBUG_CONNFAIL
			std::cerr << "p3LinkMgrIMPL::connectResult() Disconnect/Fail: flags: " << flags << " id: " << id;
			std::cerr << std::endl;

			if (it->second.inConnAttempt)
			{
				std::cerr << "p3LinkMgrIMPL::connectResult() Likely Connect Fail, as inConnAttempt Flag is set";
				std::cerr << std::endl;
			}
			if (it->second.state & RS_PEER_S_CONNECTED)
			{
				std::cerr << "p3LinkMgrIMPL::connectResult() Likely DISCONNECT, as state set to Connected";
				std::cerr << std::endl;
			}
#endif

			it->second.inConnAttempt = false;

#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::connectResult() Disconnect/Fail: id: " << id << std::endl;
			std::cerr << " Success: " << success << " flags: " << flags << std::endl;
#endif

			/* if currently connected -> flag as failed */
			if (it->second.state & RS_PEER_S_CONNECTED)
			{
				it->second.state &= (~RS_PEER_S_CONNECTED);
				it->second.actions |= RS_PEER_DISCONNECTED;
				mStatusChanged = true;

				updateLastContact = true; /* time of disconnect */
			}

			if (it->second.connAddrs.size() >= 1)
			{
				it->second.actions |= RS_PEER_CONNECT_REQ;
				mStatusChanged = true;
			}

			if (it->second.dhtVisible)
			{
				doDhtAssist = true;
			}
		}
	}
	
	if (updatePeerAddr)
	{
		pqiIpAddress raddr;
		raddr.mAddr = remote_peer_address;
		raddr.mSeenTime = time(NULL);
		raddr.mSrc = 0;

#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::connectResult() Success and we initiated connection... Updating Address";
		std::cerr << std::endl;
#endif
		mPeerMgr->updateCurrentAddress(id, raddr);
	}

	if (updateLastContact)
	{
		mPeerMgr->updateLastContact(id);
	}


	/* inform NetAssist of result. This is slightly duplicating below, as we switch it on/off
	 * in a second anyway. However, the FAILURE of UDP connection, must be informed.
 	 * 	
	 * actually the way the DHT works at the moment, both forms of feedback are required.
	 * this handles connection requests, the other searches. As they are independent... do both.
	 */

	if (flags == RS_NET_CONN_UDP_ALL)
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::connectResult() Sending Feedback for UDP connection";
		std::cerr << std::endl;
#endif
		if (success)
		{
#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::connectResult() UDP Update CONNECTED to: " << id;
			std::cerr << std::endl;
#endif

			mNetMgr->netAssistStatusUpdate(id, NETMGR_DHT_FEEDBACK_CONNECTED);
		}
		else
		{
#ifdef LINKMGR_DEBUG

			std::cerr << "p3LinkMgrIMPL::connectResult() UDP Update FAILED to: " << id;
			std::cerr << std::endl;
#endif

			/* have no differentiation between failure and closed? */
			mNetMgr->netAssistStatusUpdate(id, NETMGR_DHT_FEEDBACK_CONN_FAILED);
		}
	}


	if (success)
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::connectResult() Success switching off DhtAssist for friend: " << id;
		std::cerr << std::endl;
#endif
		/* always switch it off now */
		mNetMgr->netAssistFriend(id,false);

		/* inform NetMgr that we know this peers address: but only if external address */
		if (sockaddr_storage_isExternalNet(remote_peer_address))
		{
			mNetMgr->netAssistKnownPeer(id,remote_peer_address, 
				NETASSIST_KNOWN_PEER_FRIEND | NETASSIST_KNOWN_PEER_ONLINE);
		}
	}
	else
	{
		if (doDhtAssist)
		{
#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::connectResult() Fail, Enabling DhtAssist for: " << id;
			std::cerr << std::endl;
#endif
			mNetMgr->netAssistFriend(id,true) ;
		}
		else
		{
#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::connectResult() Fail, No DhtAssist, as No DHT visibility for: " << id;
			std::cerr << std::endl;
#endif
		}

		/* inform NetMgr that this peer is offline */
		mNetMgr->netAssistKnownPeer(id,remote_peer_address, NETASSIST_KNOWN_PEER_FRIEND | NETASSIST_KNOWN_PEER_OFFLINE);
	}

	return success;
}

/******************************** Feedback ......  *********************************
 * From various sources
 */

void    p3LinkMgrIMPL::peerStatus(const RsPeerId& id, const pqiIpAddrSet &addrs,
                       uint32_t type, uint32_t flags, uint32_t source)
{
	/* HACKED UP FIX ****/

        std::map<RsPeerId, peerConnectState>::iterator it;
	bool isFriend = true;

	rstime_t now = time(NULL);

	peerAddrInfo details;
	details.type    = type;
	details.found   = true;
	details.addrs = addrs;
	details.ts      = now;
	
	bool updateNetConfig = (source == RS_CB_PERSON);	
	uint32_t peer_vs_disc = 0;
	uint32_t peer_vs_dht = 0;
	uint32_t peerNetMode = 0;

	uint32_t ownNetMode = mNetMgr->getNetworkMode();
	
	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	{
		/* Log */
		std::string out = "p3LinkMgrIMPL::peerStatus() id: " + id.toStdString();
		rs_sprintf_append(out, " type: %lu flags: %lu source: %lu\n", type, flags, source);
		addrs.printAddrs(out);
		
		rslog(RSL_WARNING, p3connectzone, out);
#ifdef LINKMGR_DEBUG
		std::cerr << out << std::endl;
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
#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::peerStatus() Peer Not Found - Ignore" << std::endl;
#endif
			return;
		}
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::peerStatus() Peer is in mOthersList" << std::endl;
#endif
	}

#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::peerStatus() Current Peer State:" << std::endl;
	printConnectState(std::cerr, it->second);
	std::cerr << std::endl;
#endif

	/* update the status */

	/* if source is DHT */
	if (source == RS_CB_DHT)
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::peerStatus() Update From DHT:";
		std::cerr << std::endl;
#endif
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
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::peerStatus() Update From DISC:";
		std::cerr << std::endl;
#endif
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
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::peerStatus() Update From PERSON:";
		std::cerr << std::endl;
#endif

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
			peer_vs_disc = RS_VS_DISC_FULL;
		}
		else
		{
			peer_vs_disc = RS_VS_DISC_OFF;
		}

		if (flags & RS_NET_FLAGS_USE_DHT)
		{
			peer_vs_dht = RS_VS_DHT_FULL;
		}
		else
		{
			peer_vs_dht = RS_VS_DHT_OFF;
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
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::peerStatus() NOT FRIEND " << " id: " << id << std::endl;
#endif

		{
			rslog(RSL_WARNING, p3connectzone, "p3LinkMgrIMPL::peerStatus() NO CONNECT (not friend)");
		}
		return;
	}

	/* if already connected -> done */
	if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::peerStatus() PEER ONLINE ALREADY " << " id: " << id << std::endl;
#endif
		{
			/* Log */
			rslog(RSL_WARNING, p3connectzone, "p3LinkMgrIMPL::peerStatus() NO CONNECT (already connected!)");
		}

		return;
	}


      } /****** STACK UNLOCK MUTEX *******/
	
	bool newAddrs = mPeerMgr->updateAddressList(id, addrs);
	if (updateNetConfig)
	{
		mPeerMgr -> setVisState(id, peer_vs_disc, peer_vs_dht);
		mPeerMgr -> setNetworkMode(id, peerNetMode);
	}

#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::peerStatus()" << " id: " << id;
	std::cerr << " type: " << type << " flags: " << flags;
	std::cerr << " source: " << source << std::endl;
	std::cerr << " addrs: " << std::endl;
	std::string out;
	addrs.printAddrs(out);
	std::cerr << out << std::endl;

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

#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::peerStatus() Resulting Peer State:" << std::endl;
	printConnectState(std::cerr, it->second);
	std::cerr << std::endl;
#endif

}

/* This has become very unwieldy - as extra arguments are required for UDP connections */
void p3LinkMgrIMPL::peerConnectRequest(
        const RsPeerId& id, const sockaddr_storage &raddr,
        const sockaddr_storage &proxyaddr, const sockaddr_storage &srcaddr,
        uint32_t source, uint32_t flags, uint32_t delay, uint32_t bandwidth )
{
#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::peerConnectRequest() id: " << id;
	std::cerr << " raddr: " << sockaddr_storage_tostring(raddr);
	std::cerr << " proxyaddr: " << sockaddr_storage_tostring(proxyaddr);
	std::cerr << " srcaddr: " << sockaddr_storage_tostring(srcaddr);
	std::cerr << " source: " << source;
	std::cerr << " flags: " << flags;
	std::cerr << " delay: " << delay;
	std::cerr << " bandwidth: " << bandwidth;
	std::cerr << std::endl;
#endif
	{
		/* Log */
		std::string out = "p3LinkMgrIMPL::peerConnectRequest() id: " + id.toStdString();
		out += " raddr: ";
		out += sockaddr_storage_tostring(raddr);
		out += " proxyaddr: ";
		out += sockaddr_storage_tostring(proxyaddr);
		out += " srcaddr: ";
		out += sockaddr_storage_tostring(srcaddr);

		rs_sprintf_append(out, " source: %lu", source);
		rs_sprintf_append(out, " flags: %lu", flags);
		rs_sprintf_append(out, " delay: %lu", delay);
		rs_sprintf_append(out, " bandwidth: %lu", bandwidth);
		
		rslog(RSL_WARNING, p3connectzone, out);
	}

	/******************** TCP PART *****************************/

#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::peerConnectRequest() (From DHT Only)" << std::endl;
#endif

	if (source == RS_CB_DHT)
	{

		if (flags & RS_CB_FLAG_MODE_TCP)
		{

#ifndef P3CONNMGR_NO_TCP_CONNECTIONS

#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::peerConnectRequest() DHT says Online ==> so try TCP";
			std::cerr << std::endl;
#endif
			{
				RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	        		std::map<RsPeerId, peerConnectState>::iterator it;
				if (mFriendList.end() == (it = mFriendList.find(id)))
				{
#ifdef LINKMGR_DEBUG
					std::cerr << "p3LinkMgrIMPL::peerConnectRequest() ERROR Peer is not Friend" << std::endl;
#endif
					return;
				}
	
				/* if already connected -> done */
				if (it->second.state & RS_PEER_S_CONNECTED)
				{
#ifdef LINKMGR_DEBUG
					std::cerr << "p3LinkMgrIMPL::peerConnectRequest() ERROR Peer Already Connected" << std::endl;
#endif
					return;
				} 
				/* setup specific attempt for DHT found address. */
				locked_ConnectAttempt_SpecificAddress(&(it->second), raddr);
			}

			retryConnect(id);

#endif

		}
		else
		{
			/* UDP Attempt! */
#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::peerConnectRequest() DHT says CONNECT ==> tryConnectUDP()";
			std::cerr << std::endl;
#endif
			tryConnectUDP(id, raddr, proxyaddr, srcaddr, flags, delay, bandwidth);

		}
		return;

	}
	else
	{	// IS THIS USED???
		std::cerr << "p3LinkMgrIMPL::peerConnectRequest() ERROR source OTHER ==> NOOP" << std::endl;
		std::cerr << std::endl;

		return;
	}
}


/*******************************************************************/
/*******************************************************************/
       /*************** External Control ****************/
bool   p3LinkMgrIMPL::retryConnect(const RsPeerId &id)
{
	/* push all available addresses onto the connect addr stack */
#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::retryConnect() id: " << id << std::endl;
#endif

#ifndef P3CONNMGR_NO_TCP_CONNECTIONS

	retryConnectTCP(id);

#endif  // P3CONNMGR_NO_TCP_CONNECTIONS

	return true;
}



bool   p3LinkMgrIMPL::tryConnectUDP(const RsPeerId &id, const struct sockaddr_storage &rUdpAddr, 
									const struct sockaddr_storage &proxyaddr, const struct sockaddr_storage &srcaddr,
									uint32_t flags, uint32_t delay, uint32_t bandwidth)

{

#ifdef DISABLE_UDP_CONNECTIONS

	std::cerr << "p3LinkMgrIMPL::tryConnectUDP() CONNECTIONS DISABLED FOR NOW... id: " << id << std::endl;

	std::cerr << "p3LinkMgrIMPL::tryConnectUDP() PARAMS id: " << id;
	std::cerr << " raddr: " << rs_inet_ntoa(rUdpAddr.sin_addr) << ":" << ntohs(rUdpAddr.sin_port);
	std::cerr << " proxyaddr: " << rs_inet_ntoa(proxyaddr.sin_addr) << ":" << ntohs(proxyaddr.sin_port);
	std::cerr << " srcaddr: " << rs_inet_ntoa(srcaddr.sin_addr) << ":" << ntohs(srcaddr.sin_port);
	std::cerr << " flags: " << flags;
	std::cerr << " delay: " << delay;
	std::cerr << " bandwidth: " << bandwidth;
	std::cerr << std::endl;

	mNetMgr->netAssistStatusUpdate(id, NETMGR_DHT_FEEDBACK_CONN_FAILED);

	return false;	
#endif

	if (mPeerMgr->isHidden())
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::tryConnectUDP() isHidden(): no connection attempts for : " << id;
		std::cerr << std::endl;
#endif
		return false;
	}


	RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

	/* push all available addresses onto the connect addr stack */
#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::tryConnectUDP() id: " << id << std::endl;
#endif

        if (id == getOwnId()) {
            #ifdef LINKMGR_DEBUG
            rslog(RSL_WARNING, p3connectzone, "p3LinkMgrIMPL::retryConnectUDP() Failed, connecting to own id: ");
            #endif
            return false;
        }

        /* look up the id */
        std::map<RsPeerId, peerConnectState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
#ifdef LINKMGR_DEBUG
               std::cerr << "p3LinkMgrIMPL::retryConnectUDP() Peer is not Friend" << std::endl;
#endif
		return false;
	}

	/* if already connected -> done */
        if (it->second.state & RS_PEER_S_CONNECTED)
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::retryConnectUDP() Peer Already Connected" << std::endl;
#endif
		return false;
	}

	/* Explicit Request to start the UDP connection */
	if (sockaddr_storage_isValidNet(rUdpAddr))
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "Adding udp connection attempt: ";
		std::cerr << "Addr: " << sockaddr_storage_tostring(rUdpAddr);
		std::cerr << std::endl;
#endif
		peerConnectAddress pca;
		pca.addr = rUdpAddr;
		pca.type = RS_NET_CONN_UDP_PEER_SYNC;
		pca.delay = delay; 
		pca.ts = time(NULL);
		pca.period = P3CONNMGR_UDP_DEFAULT_PERIOD;
		pca.flags = flags;
		
		pca.proxyaddr = proxyaddr;
		pca.srcaddr = srcaddr;
		pca.bandwidth = bandwidth;

		// Push address to the front... so it happens quickly (before any timings are lost).
		addAddressIfUnique(it->second.connAddrs, pca, true);
	}

	/* finish it off */
	return locked_ConnectAttempt_Complete(&(it->second));
}




/* push all available addresses onto the connect addr stack...
 * with the following exceptions:
 *  - id is our own
 *  - id is not our friend
 *  - id is already connected
 *  - id is hidden but of an unkown type
 *  - we are hidden but id is not
 */
bool p3LinkMgrIMPL::retryConnectTCP(const RsPeerId &id)
{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::retryConnectTCP() id: " << id << std::endl;
#endif

	if (id == getOwnId()) return false;

	{
		RS_STACK_MUTEX(mLinkMtx);
		std::map<RsPeerId, peerConnectState>::iterator it = mFriendList.find(id);
		if ( it == mFriendList.end() ) return false;
		if ( it->second.state & RS_PEER_S_CONNECTED ) return false;
	}

	// Extract the required info from p3PeerMgr

	// first possibility - is it a hidden peer
	if (mPeerMgr->isHiddenPeer(id))
	{
		/* check for valid hidden type */
		uint32_t type = mPeerMgr->getHiddenType(id);
		if ( type & (~RS_HIDDEN_TYPE_MASK) ) return false;

		/* then we just have one connect attempt via the Proxy */
		struct sockaddr_storage proxy_addr;
		std::string domain_addr;
		uint16_t domain_port;
		if ( mPeerMgr->getProxyAddress(id, proxy_addr, domain_addr, domain_port) )
		{
			RS_STACK_MUTEX(mLinkMtx);
			std::map<RsPeerId, peerConnectState>::iterator it = mFriendList.find(id);
			if (it != mFriendList.end())
			{
				locked_ConnectAttempt_ProxyAddress(&(it->second), type, proxy_addr, domain_addr, domain_port);
				return locked_ConnectAttempt_Complete(&(it->second));
			}
		}

		return false;
	}

	if (mPeerMgr->isHidden()) return false;

	struct sockaddr_storage lAddr;
	struct sockaddr_storage eAddr;
	pqiIpAddrSet histAddrs;
	std::string dyndns;
	if (mPeerMgr->getConnectAddresses(id, lAddr, eAddr, histAddrs, dyndns))
	{
		RS_STACK_MUTEX(mLinkMtx);

		std::map<RsPeerId, peerConnectState>::iterator it = mFriendList.find(id);
		if ( it != mFriendList.end() )
		{
			locked_ConnectAttempt_CurrentAddresses(&(it->second), lAddr, eAddr);

			uint16_t dynPort = 0;
			if (!sockaddr_storage_isnull(eAddr)) dynPort = sockaddr_storage_port(eAddr);
			if (!dynPort && !sockaddr_storage_isnull(lAddr))
				dynPort = sockaddr_storage_port(lAddr);
			if (dynPort)
				locked_ConnectAttempt_AddDynDNS(&(it->second), dyndns, dynPort);

			locked_ConnectAttempt_HistoricalAddresses(&(it->second), histAddrs);

			// finish it off
			return locked_ConnectAttempt_Complete(&(it->second));
		}
		else
			std::cerr << "p3LinkMgrIMPL::retryConnectTCP() ERROR failed to find friend data : " << id << std::endl;
	}
	else
		std::cerr << "p3LinkMgrIMPL::retryConnectTCP() ERROR failed to get addresses from PeerMgr for: " << id << std::endl;

	return false;
}


#define MAX_TCP_ADDR_AGE	(3600 * 24 * 14) // two weeks in seconds.

bool p3LinkMgrIMPL::locked_CheckPotentialAddr(const struct sockaddr_storage &addr, rstime_t age)
{
#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::locked_CheckPotentialAddr("; 
	std::cerr << sockaddr_storage_tostring(addr);
	std::cerr << ", " << age << ")";
	std::cerr << std::endl;
#endif

	/*
	 * if it is old - quick rejection 
	 */
	if (age > MAX_TCP_ADDR_AGE)
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::locked_CheckPotentialAddr() REJECTING - TOO OLD"; 
		std::cerr << std::endl;
#endif
		return false;
	}

	/* if invalid - quick rejection */
	if ( ! sockaddr_storage_isValidNet(addr) )
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::locked_CheckPotentialAddr() REJECTING - INVALID";
		std::cerr << std::endl;
#endif
		return false;
	}

    std::list<struct sockaddr_storage>::const_iterator it;
	for(it = mBannedIpList.begin(); it != mBannedIpList.end(); ++it)
    {
#ifdef LINKMGR_DEBUG
        std::cerr << "Checking IP w.r.t. banned IP " << sockaddr_storage_iptostring(*it) << std::endl;
#endif

		if (sockaddr_storage_sameip(*it, addr))
		{
#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::locked_CheckPotentialAddr() REJECTING - ON BANNED IPLIST"; 
			std::cerr << std::endl;
#endif
			return false;
		}
	}

    if(rsBanList != NULL && !rsBanList->isAddressAccepted(addr, RSBANLIST_CHECKING_FLAGS_BLACKLIST))
    {
#ifdef LINKMGR_DEBUG
        std::cerr << "p3LinkMgrIMPL::locked_CheckPotentialAddr() adding to local Banned IPList";
        std::cerr << std::endl;
#endif
        return false ;
    }

	return true;
}


void  p3LinkMgrIMPL::locked_ConnectAttempt_SpecificAddress(peerConnectState *peer, const struct sockaddr_storage &remoteAddr)
{
#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_SpecificAddresses()";
	std::cerr << std::endl;
#endif
	if (locked_CheckPotentialAddr(remoteAddr, 0))
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_SpecificAddresses() ";
		std::cerr << "Adding tcp connection attempt: ";
		std::cerr << "Addr: " << sockaddr_storage_tostring(remoteAddr);
		std::cerr << std::endl;
#endif
		peerConnectAddress pca;
		pca.addr = remoteAddr;
		pca.type = RS_NET_CONN_TCP_EXTERNAL;
		pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
		pca.ts = time(NULL);
		pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;
		sockaddr_storage_clear(pca.proxyaddr);
		sockaddr_storage_clear(pca.srcaddr);
		pca.bandwidth = 0;
		
		addAddressIfUnique(peer->connAddrs, pca, false);
	}
}


void  p3LinkMgrIMPL::locked_ConnectAttempt_CurrentAddresses(peerConnectState *peer, const struct sockaddr_storage &localAddr, const struct sockaddr_storage &serverAddr)
{
#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_CurrentAddresses()";
	std::cerr << std::endl;
#endif
	// Just push all the addresses onto the stack.
	/* try "current addresses" first */
	if (locked_CheckPotentialAddr(localAddr, 0))
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_CurrentAddresses() ";
		std::cerr << "Adding tcp connection attempt: ";
		std::cerr << "Current Local Addr: " << sockaddr_storage_tostring(localAddr);
		std::cerr << std::endl;
#endif
		peerConnectAddress pca;
		pca.addr = localAddr;
		pca.type = RS_NET_CONN_TCP_LOCAL;
		pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
		pca.ts = time(NULL);
		pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;
		sockaddr_storage_clear(pca.proxyaddr);
		sockaddr_storage_clear(pca.srcaddr);
		pca.bandwidth = 0;
		
		addAddressIfUnique(peer->connAddrs, pca, false);
	}

	if (locked_CheckPotentialAddr(serverAddr, 0))
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_CurrentAddresses() ";
		std::cerr << "Adding tcp connection attempt: ";
		std::cerr << "Current Ext Addr: " << sockaddr_storage_tostring(serverAddr);
		std::cerr << std::endl;
#endif
		peerConnectAddress pca;
		pca.addr = serverAddr;
		pca.type = RS_NET_CONN_TCP_EXTERNAL;
		pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
		pca.ts = time(NULL);
		pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;
		sockaddr_storage_clear(pca.proxyaddr);
		sockaddr_storage_clear(pca.srcaddr);
		pca.bandwidth = 0;
		
		addAddressIfUnique(peer->connAddrs, pca, false);
	}
}


void  p3LinkMgrIMPL::locked_ConnectAttempt_HistoricalAddresses(peerConnectState *peer, const pqiIpAddrSet &ipAddrs)
{
	/* now try historical addresses */
	/* try local addresses first */
	std::list<pqiIpAddress>::const_iterator ait;
	rstime_t now = time(NULL);

#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_HistoricalAddresses()";
	std::cerr << std::endl;
#endif
    for(ait = ipAddrs.mLocal.mAddrs.begin();  ait != ipAddrs.mLocal.mAddrs.end(); ++ait)
	{
		if (locked_CheckPotentialAddr(ait->mAddr, now - ait->mSeenTime))
		{

#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_HistoricalAddresses() ";
			std::cerr << "Adding tcp connection attempt: ";
			std::cerr << "Local Addr: " << sockaddr_storage_tostring(ait->mAddr);
			std::cerr << std::endl;
#endif

			peerConnectAddress pca;
			pca.addr = ait->mAddr;
			pca.type = RS_NET_CONN_TCP_LOCAL;
			pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
			pca.ts = time(NULL);
			pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;
			sockaddr_storage_clear(pca.proxyaddr);
			sockaddr_storage_clear(pca.srcaddr);
			pca.bandwidth = 0;
			
			addAddressIfUnique(peer->connAddrs, pca, false);
		}
	}

	for(ait = ipAddrs.mExt.mAddrs.begin(); 
		ait != ipAddrs.mExt.mAddrs.end(); ++ait)
	{
		if (locked_CheckPotentialAddr(ait->mAddr, now - ait->mSeenTime))
		{
	
#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_HistoricalAddresses() ";
			std::cerr << "Adding tcp connection attempt: ";
			std::cerr << "Ext Addr: " << sockaddr_storage_tostring(ait->mAddr);
			std::cerr << std::endl;
#endif
			peerConnectAddress pca;
			pca.addr = ait->mAddr;
			pca.type = RS_NET_CONN_TCP_EXTERNAL;
			pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
			pca.ts = time(NULL);
			pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;
			sockaddr_storage_clear(pca.proxyaddr);
			sockaddr_storage_clear(pca.srcaddr);
			pca.bandwidth = 0;
			
			addAddressIfUnique(peer->connAddrs, pca, false);
		}
	}
}


void  p3LinkMgrIMPL::locked_ConnectAttempt_AddDynDNS(peerConnectState *peer, std::string dyndns, uint16_t port)
{
	/* try dyndns address too */
	struct sockaddr_storage addr;
	if (!dyndns.empty() && port) 
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_AddDynDNS() Looking up DynDNS address: " << dyndns << std::endl;
#endif
		if(mDNSResolver->getIPAddressFromString(dyndns, addr))
		{
#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_AddDynDNS() ";
			std::cerr << "Adding tcp connection attempt: ";
			std::cerr << "DynDNS Addr: " << sockaddr_storage_iptostring(addr);
			std::cerr << ":" << port;
			std::cerr << std::endl;
#endif
			peerConnectAddress pca;
			sockaddr_storage_copyip(pca.addr, addr);
			sockaddr_storage_setport(pca.addr, port);
			pca.type = RS_NET_CONN_TCP_EXTERNAL;
			//for the delay, we add a random time and some more time when the friend list is big
			pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
			pca.ts = time(NULL);
			pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;

			sockaddr_storage_clear(pca.proxyaddr);
			sockaddr_storage_clear(pca.srcaddr);
			pca.bandwidth = 0;
			
			/* check address validity */
			if (locked_CheckPotentialAddr(pca.addr, 0))
			{
				addAddressIfUnique(peer->connAddrs, pca, true);
			}
		}
		else
		{
#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_AddDynDNS() DNSResolver hasn't found addr yet";
			std::cerr << std::endl;
#endif
		}
	}
	else
	{
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_AddDynDNS() Address(" << dyndns << ") or Port(" << port << ") NULL ignoring";
		std::cerr << std::endl;
#endif
	}
}


void  p3LinkMgrIMPL::locked_ConnectAttempt_ProxyAddress(peerConnectState *peer, const uint32_t type, const struct sockaddr_storage &proxy_addr, const std::string &domain_addr, uint16_t domain_port)
{
#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_ProxyAddress() trying address: " << domain_addr << ":" << domain_port << std::endl;
#endif
	peerConnectAddress pca;
	pca.addr = proxy_addr;

	switch (type) {
	case RS_HIDDEN_TYPE_TOR:
		pca.type = RS_NET_CONN_TCP_HIDDEN_TOR;
		break;
	case RS_HIDDEN_TYPE_I2P:
		pca.type = RS_NET_CONN_TCP_HIDDEN_I2P;
		break;
	case RS_HIDDEN_TYPE_UNKNOWN:
	default:
		/**** THIS CASE SHOULD NOT BE TRIGGERED - since this function is called with a valid hidden type only ****/
		std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_ProxyAddress() hidden type of addr: " << domain_addr << " is unkown -> THIS SHOULD NEVER HAPPEN!" << std::endl;
		std::cerr << " - peer : " << peer->id << "(" << peer->name << ")" << std::endl;
		std::cerr << " - proxy: " << sockaddr_storage_tostring(proxy_addr) << std::endl;
		std::cerr << " - addr : " << domain_addr << ":" << domain_port << std::endl;
		pca.type = RS_NET_CONN_TCP_UNKNOW_TOPOLOGY;
	}

	//for the delay, we add a random time and some more time when the friend list is big
	pca.delay = P3CONNMGR_TCP_DEFAULT_DELAY;
	pca.ts = time(NULL);
	pca.period = P3CONNMGR_TCP_DEFAULT_PERIOD;

	sockaddr_storage_clear(pca.proxyaddr);
	sockaddr_storage_clear(pca.srcaddr);
	pca.bandwidth = 0;

	pca.domain_addr = domain_addr;
	pca.domain_port = domain_port;
			
	/* check address validity */
	if (locked_CheckPotentialAddr(pca.addr, 0))
	{
		addAddressIfUnique(peer->connAddrs, pca, true);
	}
}


bool  p3LinkMgrIMPL::addAddressIfUnique(std::list<peerConnectAddress> &addrList, peerConnectAddress &pca, bool pushFront)
{
	/* iterate through the list, and make sure it isn't already 
	 * in the list 
	 */
#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::addAddressIfUnique() Checking Address: " << sockaddr_storage_iptostring(pca.addr);
	std::cerr << std::endl;
#endif

	std::list<peerConnectAddress>::iterator it;
	for(it = addrList.begin(); it != addrList.end(); ++it)
	{
		if (sockaddr_storage_same(pca.addr, it->addr) &&
			(pca.type == it->type))
		{
#ifdef LINKMGR_DEBUG
			std::cerr << "p3LinkMgrIMPL::addAddressIfUnique() Discarding Duplicate Address";
			std::cerr << std::endl;
#endif
			/* already */
			return false;
		}
	}

#ifdef LINKMGR_DEBUG
	std::cerr << "p3LinkMgrIMPL::addAddressIfUnique() Adding New Address";
	std::cerr << std::endl;
#endif

	if (pushFront)
	{
		addrList.push_front(pca);
	}
	else
	{
		addrList.push_back(pca);
	}

	return true;
}



bool  p3LinkMgrIMPL::locked_ConnectAttempt_Complete(peerConnectState *peer)
{

	/* flag as last attempt to prevent loop */
	//add a random perturbation between 0 and 2 sec.
	peer->lastattempt = time(NULL) + rand() % MAX_RANDOM_ATTEMPT_OFFSET; 

	if (peer->inConnAttempt) 
	{
                /*  -> it'll automatically use the addresses we added */
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_Complete() Already in CONNECT ATTEMPT";
		std::cerr << std::endl;
		std::cerr << "p3LinkMgrIMPL::locked_ConnectAttempt_Complete() Remaining ConnAddr Count: " << peer->connAddrs.size();
		std::cerr << std::endl;
#endif
		return true;
	}

	/* start a connection attempt */
	if (peer->connAddrs.size() > 0) 
	{
#ifdef LINKMGR_DEBUG
		std::string out = "p3LinkMgrIMPL::locked_ConnectAttempt_Complete() Started CONNECT ATTEMPT!\n" ;
		rs_sprintf_append(out, "p3LinkMgrIMPL::locked_ConnectAttempt_Complete() ConnAddr Count: %u", peer->connAddrs.size());
		rslog(RSL_DEBUG_ALERT, p3connectzone, out);
		std::cerr << out << std::endl;
#endif

		peer->actions |= RS_PEER_CONNECT_REQ;
		mStatusChanged = true;
	    return true; 
	} 
	else 
	{
#ifdef LINKMGR_DEBUG
		std::string out = "p3LinkMgrIMPL::locked_ConnectAttempt_Complete() No addr in the connect attempt list. Not suitable for CONNECT ATTEMPT!";
		rslog(RSL_DEBUG_ALERT, p3connectzone, out);
		std::cerr << out << std::endl;
#endif
	    return false;
	}
	return false;
}


/***********************************************************************************************************
 ************************************* Handling of Friends *************************************************
 ***********************************************************************************************************/

int p3LinkMgrIMPL::addFriend(const RsPeerId &id, bool isVisible)
{
#ifdef LINKMGR_DEBUG_LOG
	rslog(RSL_WARNING, p3connectzone, "p3LinkMgr::addFriend() id: " + id.toStdString());
#endif
	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/
	
#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::addFriend(" << id << "," << isVisible << ")";
		std::cerr << std::endl;
#endif

	        std::map<RsPeerId, peerConnectState>::iterator it;
		it = mFriendList.find(id);
	
		if (it != mFriendList.end())
		{
			std::cerr << "p3LinkMgrIMPL::addFriend() ERROR, friend already exists : " << id;
			std::cerr << std::endl;
			return 0;
		}
	
		peerConnectState pcs;
		pcs.dhtVisible = isVisible;
		pcs.id = id;
		pcs.name = "NoName";
		pcs.state = RS_PEER_S_FRIEND;
		pcs.actions = RS_PEER_NEW;
		pcs.linkType = RS_NET_CONN_SPEED_UNKNOWN ;
	
		mFriendList[id] = pcs;

		mStatusChanged = true;
	}
	
	mNetMgr->netAssistFriend(id, isVisible);

	return 1;
}


int p3LinkMgrIMPL::removeFriend(const RsPeerId &id)
{
	rslog(RSL_WARNING, p3connectzone, "p3LinkMgr::removeFriend() id: " + id.toStdString());

	{
		RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/
		
		#ifdef LINKMGR_DEBUG
		std::cerr << "p3LinkMgrIMPL::removeFriend(" << id << ")";
		std::cerr << std::endl;
		#endif
		
		std::map<RsPeerId, peerConnectState>::iterator it;
		it = mFriendList.find(id);
		
		if (it == mFriendList.end())
		{
			std::cerr << "p3LinkMgrIMPL::removeFriend() ERROR, friend not there : " << id;
			std::cerr << std::endl;
			return 0;
		}
		
		/* Move to OthersList (so remove can be handled via the action) */
		peerConnectState peer = it->second;
		
		peer.state &= (~RS_PEER_S_FRIEND);
		peer.state &= (~RS_PEER_S_CONNECTED);
		peer.state &= (~RS_PEER_S_ONLINE);
		peer.actions = RS_PEER_MOVED;
		peer.inConnAttempt = false;
		mOthersList[id] = peer;

		mStatusChanged = true;
		
		mFriendList.erase(it);
	}
		
	mNetMgr->netAssistFriend(id, false);

	return 1;
}

void p3LinkMgrIMPL::disconnectFriend(const RsPeerId& id)
{
    std::list<pqiMonitor*> disconnect_clients ;

    {
        RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

        disconnect_clients = clients ;

        std::cerr << "Disconnecting friend " << id << std::endl;

        std::map<RsPeerId, peerConnectState>::iterator it;
        it = mFriendList.find(id);

        if (it == mFriendList.end())
        {
            std::cerr << "p3LinkMgrIMPL::removeFriend() ERROR, friend not there : " << id;
            std::cerr << std::endl;
            return ;
        }

        /* Move to OthersList (so remove can be handled via the action) */
        peerConnectState peer = it->second;

        peer.state &= (~RS_PEER_S_CONNECTED);
        peer.state &= (~RS_PEER_S_ONLINE);
        peer.actions = RS_PEER_DISCONNECTED;
        peer.inConnAttempt = false;
    }

    for(std::list<pqiMonitor*>::const_iterator it(disconnect_clients.begin());it!=disconnect_clients.end();++it)
        (*it)->disconnectPeer(id) ;
}

void p3LinkMgrIMPL::printPeerLists(std::ostream &out)
{
        {
                RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

                out << "p3LinkMgrIMPL::printPeerLists() Friend List";
                out << std::endl;


                std::map<RsPeerId, peerConnectState>::iterator it;
                for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
                {
                        out << "\t SSL ID: " << it->second.id.toStdString();
                        out << "\t State: " << it->second.state;
                        out << std::endl;
                }

                out << "p3LinkMgrIMPL::printPeerLists() Others List";
                out << std::endl;
                for(it = mOthersList.begin(); it != mOthersList.end(); ++it)
                {
                        out << "\t SSL ID: " << it->second.id.toStdString();
                        out << "\t State: " << it->second.state;
                }
        }

    return;
}

bool p3LinkMgrIMPL::checkPotentialAddr(const sockaddr_storage &addr, rstime_t age)
{
    RsStackMutex stack(mLinkMtx); /****** STACK LOCK MUTEX *******/

    return locked_CheckPotentialAddr(addr,age) ;
}


void  printConnectState(std::ostream &out, peerConnectState &peer)
{

        out << "Friend: " << peer.name << " Id: " << peer.id.toStdString() << " State: " << peer.state;
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



