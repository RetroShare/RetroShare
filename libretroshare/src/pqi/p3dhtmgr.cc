/*
 * libretroshare/src/pqi: p3dhtmgr.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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

#include "pqi/p3dhtmgr.h"
#include "pqi/p3connmgr.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

#include "util/rsprint.h"

#define DHT_DEBUG 1
/*****
 * #define P3DHTMGR_USE_LOCAL_UDP_CONN 1  // For Testing only 
 ****/

/**** DHT State Variables ****
 * TODO:
 * (1) notify call in.
 * (2) publish/search parameters.
 * (3) callback.
 * (4) example
 *
 */

/**** DHT State Variables ****/

#define DHT_STATE_OFF		0
#define DHT_STATE_INIT		1
#define DHT_STATE_CHECK_PEERS	2
#define DHT_STATE_FIND_STUN	3
#define DHT_STATE_ACTIVE	4

/* TIMEOUTS (Key ones in .h) */
#define DHT_RESTART_PERIOD	300  /* 5 min */
#define DHT_DEFAULT_PERIOD	300  /* Default period if no work to do */
#define DHT_MIN_PERIOD 		1    /* to ensure we don't get too many requests */

#define DHT_DEFAULT_WAITTIME	1    /* Std sleep break period */


void printDhtPeerEntry(dhtPeerEntry *ent, std::ostream &out);

/* Interface class for DHT data */

dhtPeerEntry::dhtPeerEntry()
	:state(DHT_PEER_INIT), lastTS(0), 
	 notifyPending(0), notifyTS(0), 
	 type(DHT_ADDR_INVALID)
{
	laddr.sin_addr.s_addr = 0;
	laddr.sin_port = 0;
	laddr.sin_family = 0;

	raddr.sin_addr.s_addr = 0;
	raddr.sin_port = 0;
	raddr.sin_family = 0;

	return;
}

p3DhtMgr::p3DhtMgr(std::string id, pqiConnectCb *cb)
	:connCb(cb), mStunRequired(true) 
{
	/* setup own entry */
	dhtMtx.lock(); /* LOCK MUTEX */

	ownEntry.id = id;
	ownEntry.state = DHT_PEER_INIT;
	ownEntry.type = DHT_ADDR_INVALID;
	ownEntry.lastTS = 0;

	ownEntry.notifyPending = 0;
	ownEntry.notifyTS = 0;

	ownEntry.hash1 = RsUtil::HashId(id, false);
	ownEntry.hash2 = RsUtil::HashId(id, true);

	mDhtModifications = true;
	mDhtOn = false;
	mDhtState  = DHT_STATE_OFF;

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	return;
}

void    p3DhtMgr::setDhtOn(bool on)
{
	dhtMtx.lock(); /* LOCK MUTEX */

	mDhtModifications = true;
	mDhtOn = on;

	dhtMtx.unlock(); /* UNLOCK MUTEX */
}

bool    p3DhtMgr::getDhtOn()
{
	dhtMtx.lock(); /* LOCK MUTEX */

	bool on = mDhtOn;

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	return on;
}

bool    p3DhtMgr::getDhtActive()
{
	dhtMtx.lock(); /* LOCK MUTEX */

	bool act = dhtActive();

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	return act;
}

/******************************** PEER MANAGEMENT **********************************
 *
 */
	/* set key data */
bool p3DhtMgr::setExternalInterface(
			struct sockaddr_in laddr, 
			struct sockaddr_in raddr, 
			uint32_t type)
{
	dhtMtx.lock(); /* LOCK MUTEX */

	mDhtModifications = true;
	ownEntry.laddr = laddr;
	ownEntry.raddr = raddr;
	ownEntry.type = type;
	ownEntry.state = DHT_PEER_ADDR_KNOWN; /* will force republish */

#ifdef DHT_DEBUG
        std::cerr << "p3DhtMgr::setExternalInterface()";
        std::cerr << " laddr: " << inet_ntoa(ownEntry.laddr.sin_addr);
	std::cerr << " lport: " << ntohs(ownEntry.laddr.sin_port);
        std::cerr << " raddr: " << inet_ntoa(ownEntry.raddr.sin_addr);
	std::cerr << " rport: " << ntohs(ownEntry.raddr.sin_port);
        std::cerr << " type: " << ownEntry.type;
        std::cerr << " state: " << ownEntry.state;
	std::cerr << std::endl;
#endif

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	return true;
}


	/* add / remove peers */
bool p3DhtMgr::findPeer(std::string id)
{
	RsStackMutex stack(dhtMtx); /***** LOCK MUTEX *****/

	mDhtModifications = true;

	std::map<std::string, dhtPeerEntry>::iterator it;
	it = peers.find(id);
	if (it != peers.end())
	{
		return true;
	}

	/* if they are not in the list -> add */
	dhtPeerEntry ent;
	ent.id = id;
	ent.state = DHT_PEER_INIT;
	ent.type = DHT_ADDR_INVALID;
	ent.lastTS = 0;

	ent.notifyPending = 0;
	ent.notifyTS = 0;

	/* fill in hashes */
	ent.hash1 = RsUtil::HashId(id, false);
	ent.hash2 = RsUtil::HashId(id, true);

	/* other fields don't matter */

	peers[id] = ent;

	return true;
}

bool p3DhtMgr::dropPeer(std::string id)
{
	RsStackMutex stack(dhtMtx); /***** LOCK MUTEX *****/

	mDhtModifications = true;

	/* once we are connected ... don't worry about them anymore */
	std::map<std::string, dhtPeerEntry>::iterator it;
	it = peers.find(id);
	if (it == peers.end())
	{
		return false;
	}

	/* remove */
	peers.erase(it);

	return true;
}

	/* post DHT key saying we should connect */
bool p3DhtMgr::notifyPeer(std::string id) 
{
	RsStackMutex stack(dhtMtx); /***** LOCK MUTEX *****/
#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::notifyPeer() " << id << std::endl;
#endif

	std::map<std::string, dhtPeerEntry>::iterator it;
	it = peers.find(id);
	if (it == peers.end())
	{
		return false;
	}

	time_t now = time(NULL);

	if (now - it->second.notifyTS < 2 * DHT_NOTIFY_PERIOD)
	{
		/* drop the notify (too soon) */
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::notifyPeer() TO SOON - DROPPING" << std::endl;
#endif
		return false;
	}

	it->second.notifyPending = RS_CONNECT_ACTIVE;
	it->second.notifyTS = time(NULL);

	/* Trigger search if not found! */
	if (it->second.state != DHT_PEER_FOUND)
	{
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::notifyPeer() PEER NOT FOUND - Trigger search" << std::endl;
#endif
		it->second.lastTS = 0;
	}

	mDhtModifications = true; /* no wait! */

	return true;
}

	/* extract current peer status */
bool p3DhtMgr::getPeerStatus(std::string id, 
			struct sockaddr_in &laddr, 
			struct sockaddr_in &raddr, 
			uint32_t &type, uint32_t &state)
{
	dhtMtx.lock(); /* LOCK MUTEX */

	std::map<std::string, dhtPeerEntry>::iterator it;
	it = peers.find(id);

	bool found = it != peers.end();
	if (found)
	{
		laddr = it->second.laddr;
		raddr = it->second.raddr;
		type = it->second.type;
		state = it->second.type;
	}

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	return found;
}

/********************************* STUN HANDLING  **********************************
 * add / cleanup / stun.
 *
 */

	/* stun */
bool p3DhtMgr::addStun(std::string id)
{
	dhtMtx.lock(); /* LOCK MUTEX */

	mDhtModifications = true;

	std::list<std::string>::iterator it;
	it = std::find(stunIds.begin(), stunIds.end(), id);
	if (it != stunIds.end())
	{
		dhtMtx.unlock(); /* UNLOCK MUTEX */
		return false;
	}
	stunIds.push_back(id);

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	return true;
}

bool p3DhtMgr::doneStun()
{
	dhtMtx.lock(); /* LOCK MUTEX */

	mDhtModifications = true;

	/* clear up */
	stunIds.clear();

	mStunRequired = false;

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	return true;
}


void p3DhtMgr::run()
{
	/* 
	 *
	 */

	while(1)
	{
		checkDHTStatus();


#ifdef DHT_DEBUG
		status(std::cerr);
#endif

		dhtMtx.lock(); /* LOCK MUTEX */

		uint32_t dhtState = mDhtState;

		dhtMtx.unlock(); /* UNLOCK MUTEX */

		int period = 60; /* default wait */
		switch(dhtState)
		{
			case DHT_STATE_INIT:
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::run() state = INIT -> wait" << std::endl;
#endif
				period = 10;
				break;
			case DHT_STATE_CHECK_PEERS:
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::run() state = CHECK_PEERS -> do stuff" << std::endl;
#endif
				checkPeerDHTKeys();
				checkStunState();
				period = DHT_MIN_PERIOD;
				break;
			case DHT_STATE_FIND_STUN:
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::run() state = FIND_STUN -> do stuff" << std::endl;
#endif
				doStun();
				checkPeerDHTKeys(); /* keep on going - as we could be in this state for a while */
				checkStunState();
				period = DHT_MIN_PERIOD;
				break;
			case DHT_STATE_ACTIVE:
			{
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::run() state = ACTIVE -> do stuff" << std::endl;
#endif
				period = checkOwnDHTKeys();
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::run() checkOwnDHTKeys() period: " << period << std::endl;
#endif
				int tmpperiod = checkNotifyDHT();
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::run() checkNotifyDHT() period: " << tmpperiod << std::endl;
#endif
				int tmpperiod2 = checkPeerDHTKeys();
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::run() checkPeerDHTKeys() period: " << tmpperiod2 << std::endl;
#endif
				if (tmpperiod < period)
					period = tmpperiod;
				if (tmpperiod2 < period)
					period = tmpperiod2;
			}
				break;
			default:
			case DHT_STATE_OFF:
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::run() state = OFF -> wait" << std::endl;
#endif
				period = 60;
				break;
		}

#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::run() sleeping for: " << period << std::endl;
#endif

		/* Breakable sleep loop */

		bool toBreak = false;
		int waittime = 1;
		int i;
		for(i = 0; i < period; i += waittime)
		{
			if (period-i > DHT_DEFAULT_WAITTIME)
			{
				waittime = DHT_DEFAULT_WAITTIME;
			}
			else
			{
				waittime = period-i;
			}

			dhtMtx.lock(); /* LOCK MUTEX */

			if (mDhtModifications)
			{
				mDhtModifications = false;
				toBreak = true;
			}

			dhtMtx.unlock(); /* UNLOCK MUTEX */

			if (toBreak)
			{
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::run() breaking sleep" << std::endl;
#endif

				break; /* speed up config modifications */
			}


			sleep(waittime);
		}
	}
}


int p3DhtMgr::checkOwnDHTKeys()
{
	int repubPeriod = 10000;
	time_t now = time(NULL);

	/* in order of importance:
	 * (1) Check for Own Key publish.
	 * (2) Check for notification requests
	 * (3) Check for Peers
	 */

#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::checkOwnDHTKeys()" << std::endl;
#endif

	dhtMtx.lock(); /* LOCK MUTEX */

	dhtPeerEntry peer = ownEntry;

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	/* publish ourselves if necessary */
	if (peer.state >= DHT_PEER_ADDR_KNOWN)
	{
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::checkOwnDHTKeys() OWN ADDR KNOWN" << std::endl;
#endif
		if ((peer.state < DHT_PEER_PUBLISHED) ||
		    (now - peer.lastTS > DHT_PUBLISH_PERIOD))
		{
#ifdef DHT_DEBUG
			std::cerr << "p3DhtMgr::checkOwnDHTKeys() OWN ADDR REPUB" << std::endl;
#endif

#ifdef DHT_DEBUG
	        	std::cerr << "PUBLISH: ";
	        	std::cerr << " hash1: " << RsUtil::BinToHex(peer.hash1);
	        	std::cerr << " laddr: " << inet_ntoa(peer.laddr.sin_addr);
			std::cerr << ":" << ntohs(peer.laddr.sin_port);
	        	std::cerr << "   raddr: " << inet_ntoa(peer.raddr.sin_addr);
			std::cerr << ":" << ntohs(peer.raddr.sin_port);
	        	std::cerr << "   type: " << peer.type;
			std::cerr << std::endl;
#endif

			/* publish own key */
			if (dhtPublish(peer.hash1, peer.laddr, peer.raddr, peer.type, ""))
			{
				dhtMtx.lock(); /* LOCK MUTEX */

				ownEntry.lastTS = now;
				ownEntry.state = DHT_PEER_PUBLISHED;

				dhtMtx.unlock(); /* UNLOCK MUTEX */
			}

			/* restart immediately */
			repubPeriod = DHT_MIN_PERIOD;
			return repubPeriod;
		}
		else
		{
			if (now - peer.lastTS < DHT_PUBLISH_PERIOD)
		    	{
		  		repubPeriod = DHT_PUBLISH_PERIOD - 
					(now - peer.lastTS);
			}
			else
			{
				repubPeriod = 10;
			}
#ifdef DHT_DEBUG
			std::cerr << "p3DhtMgr::checkOwnDHTKeys() repub in: ";
			std::cerr << repubPeriod << std::endl;
#endif
		}
			

		/* check for connect requests */
		//if ((peer.state == DHT_PEER_PUBLISHED) &&
		//	(!(peer.type & RS_NET_CONN_TCP_EXTERNAL))) 
		if (peer.state == DHT_PEER_PUBLISHED) 
		{
			if (now - peer.notifyTS >= DHT_NOTIFY_PERIOD)
			{
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::checkOwnDHTKeys() check for Notify (rep=0)";
				std::cerr << std::endl;
#endif
				if (dhtSearch(peer.hash1, DHT_MODE_NOTIFY))
				{
					dhtMtx.lock(); /* LOCK MUTEX */

					ownEntry.notifyTS = now;

					dhtMtx.unlock(); /* UNLOCK MUTEX */
				}

				/* restart immediately */
				repubPeriod = DHT_MIN_PERIOD;
				return repubPeriod;
			}
			else
			{
		  		repubPeriod = DHT_NOTIFY_PERIOD - 
					(now - peer.notifyTS);
				if (repubPeriod < DHT_MIN_PERIOD)
				{
					repubPeriod = DHT_MIN_PERIOD;
				}
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::checkOwnDHTKeys() check Notify in: ";
				std::cerr << repubPeriod << std::endl;
#endif
			}
		}
		else
		{
			if (peer.state != DHT_PEER_PUBLISHED)
			{
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::checkOwnDHTKeys() No Notify until Published";
				std::cerr << std::endl;
#endif
			}
			else if (peer.type & RS_NET_CONN_TCP_EXTERNAL)
			{
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::checkOwnDHTKeys() No Notify because have Ext Addr";
				std::cerr << std::endl;
#endif
			}
			else
			{
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::checkOwnDHTKeys() No Notify: Unknown Reason";
				std::cerr << std::endl;
#endif
			}
		}

	}
	else
	{
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::checkOwnDHTKeys() PEER ADDR UNKNOWN" << std::endl;
#endif
		repubPeriod = 10;
	}
	return repubPeriod;
}


int p3DhtMgr::checkPeerDHTKeys()
{
	/* now loop through the peers */

#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::checkPeerDHTKeys()" << std::endl;
#endif

	dhtMtx.lock(); /* LOCK MUTEX */

	/* iterate through and find min time and suitable candidate */
        std::map<std::string, dhtPeerEntry>::iterator it,pit;
	time_t now = time(NULL);
	uint32_t period = 0;
	uint32_t repeatPeriod = 6000;

	pit = peers.end();
	time_t pTS = now;
	
	for(it = peers.begin(); it != peers.end(); it++)
	{
		time_t delta = now - it->second.lastTS;
		if (it->second.state < DHT_PEER_FOUND)
		{
			period = DHT_SEARCH_PERIOD;
		}
		else /* (it->second.state == DHT_PEER_FOUND) */
		{
			period = DHT_CHECK_PERIOD;
		}
#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::checkPeerDHTKeys() Peer: " << it->second.id;
	std::cerr << " Period: " << period;
	std::cerr << " Delta: " << delta;
	std::cerr << std::endl;
#endif


		if ((unsigned) delta >= period)
		{
			if (it->second.lastTS < pTS)
			{
				pit = it;
				pTS = it->second.lastTS;
			}
			repeatPeriod = DHT_MIN_PERIOD; 
		}
		else if (period - delta < repeatPeriod)
		{
			repeatPeriod = period - delta;
		}
	}

	/* now have - peer to handle, and period to next call */

	if (pit == peers.end())
	{
		dhtMtx.unlock(); /* UNLOCK MUTEX */
		return repeatPeriod;
	}

	/* update timestamp 
	 * clear FOUND or INIT state.
	 * */

	pit->second.lastTS = now;
	pit->second.state = DHT_PEER_SEARCH;

	dhtPeerEntry peer = (pit->second);

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	/* now search for the peer */
	dhtSearch(peer.hash1, DHT_MODE_SEARCH);

	/* results come through callback */
	return repeatPeriod;
}


int p3DhtMgr::checkNotifyDHT()
{
#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::checkNotifyDHT()" << std::endl;
#endif
	/* now loop through the peers */
	uint32_t notifyType = 0;
	dhtPeerEntry peer;
	dhtPeerEntry  own;

      {
	RsStackMutex stack(dhtMtx); /***** LOCK MUTEX *****/

	/* iterate through and find min time and suitable candidate */
        std::map<std::string, dhtPeerEntry>::iterator it;
	time_t now = time(NULL);
	int repeatPeriod = DHT_DEFAULT_PERIOD; 

	/* find the first with a notify flag */
	for(it = peers.begin(); it != peers.end(); it++)
	{
		if (it->second.notifyPending)
		{
			if (it->second.state == DHT_PEER_FOUND)
			{
				notifyType = it->second.notifyPending;
				break;
			}

			/* if very old drop it */
			if (now - it->second.notifyTS > DHT_NOTIFY_PERIOD)
			{
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::checkNotifyDHT() Dropping OLD Notify: ";
				std::cerr << it->first << std::endl;
#endif
				it->second.notifyPending = 0;
			}
		}
	}

	/* now have - peer to handle */
	if (it == peers.end())
	{
		return repeatPeriod;
	}

#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::checkNotifyDHT() Notify From: ";
	std::cerr << it->first << std::endl;
#endif

	/* update timestamp */
	it->second.notifyTS = now;
	it->second.notifyPending = 0;

	peer = (it->second);
	own  = ownEntry;

      } /******* UNLOCK ******/

        if (notifyType == RS_CONNECT_ACTIVE)
	{
		/* publish notification (publish Our Id) 
		 * We publish the connection attempt on peers hash, 
		 * using our alternative hash..
		 * */
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::checkNotifyDHT() Posting Active Notify";
		std::cerr << std::endl;
#endif

		dhtNotify(peer.hash1, own.hash2, "");
	}

	/* feedback to say we started it! */
#ifdef P3DHTMGR_USE_LOCAL_UDP_CONN
	connCb->peerConnectRequest(peer.id, peer.laddr, RS_CB_DHT);
#else
	connCb->peerConnectRequest(peer.id, peer.raddr, RS_CB_DHT);
#endif

	return DHT_MIN_PERIOD; 
}



int p3DhtMgr::doStun()
{
#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::doStun()" << std::endl;
#endif
	if (stunIds.size() < 1)
		return 0;

	/* pop the front one */
	std::string activeStunId = stunIds.front();

	stunIds.pop_front();
	/* Don't recycle -> otherwise can end in an infinite (pointless) loop! */
	//stunIds.push_back(activeStunId);

	/* look it up! */
	dhtSearch(activeStunId, DHT_MODE_SEARCH);

	return 1;
}



int p3DhtMgr::checkStunState()
{
#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::checkStunState()" << std::endl;
#endif
	dhtMtx.lock(); /* LOCK MUTEX */

	/* now loop through the peers */
	if (!mStunRequired)
	{
		mDhtState = DHT_STATE_ACTIVE;
	}

	if (mDhtState == DHT_STATE_CHECK_PEERS)
	{
		/* check that they have all be searched for */
		std::map<std::string, dhtPeerEntry>::iterator it;
		for(it = peers.begin(); it != peers.end(); it++)
		{
			if (it->second.state == DHT_PEER_INIT)
			{
				break;
			}
		}

		if (it == peers.end())
		{
			/* we've checked all peers */
			mDhtState = DHT_STATE_FIND_STUN;
		}
	}
	else if (mDhtState == DHT_STATE_FIND_STUN)
	{
		/* if we run out of stun peers -> just go to active */
		if (stunIds.size() < 1)
		{
			std::cerr << "WARNING: out of Stun Peers - without getting id" << std::endl;
			mDhtState = DHT_STATE_ACTIVE;
		}
	}

	dhtMtx.unlock(); /* UNLOCK MUTEX */
	return 1;
}

void p3DhtMgr::checkDHTStatus()
{
	dhtMtx.lock(); /* LOCK MUTEX */

	bool isActive   = (mDhtState != DHT_STATE_OFF);

	bool toShutdown = false;
	bool toStartup  = false;

#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::checkDhtStatus() mDhtState: " << mDhtState << std::endl;
	std::cerr << "p3DhtMgr::checkDhtStatus() mDhtOn   : " << mDhtOn << std::endl;
#endif

	if ((isActive) && (!mDhtOn))
	{
#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::checkDhtStatus() Active & !mDhtOn -> toShutdown" << std::endl;
#endif
		toShutdown = true;
	}

	if ((!isActive) && (mDhtOn))
	{
#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::checkDhtStatus() !Active & mDhtOn -> toStartup" << std::endl;
#endif
		toStartup = true;
	}

	/* restart if it has shutdown */
	if (isActive && mDhtOn)
	{
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::checkDhtStatus() Active & mDhtOn" << std::endl;
#endif
		if (dhtActive())
		{
#ifdef DHT_DEBUG
			std::cerr << "p3DhtMgr::checkDhtStatus() dhtActive() = true" << std::endl;
#endif
			if (mDhtState == DHT_STATE_INIT)
			{
				mDhtState = DHT_STATE_CHECK_PEERS;
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::checkDhtStatus() Switching to CHECK PEERS" << std::endl;
#endif
			}
		}
		else
		{
#ifdef DHT_DEBUG
			std::cerr << "p3DhtMgr::checkDhtStatus() dhtActive() = false" << std::endl;
#endif
			if (mDhtActiveTS - time(NULL) > DHT_RESTART_PERIOD)
			{
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::checkDhtStatus() restart Period..." << std::endl;
#endif
				toShutdown = true;
				toStartup = true;
			}
		}
	}

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	if (toShutdown)
	{
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::checkDhtStatus() toShutdown = true -> shutdown()" << std::endl;
#endif
		if (shutdown())
		{
			clearDhtData();

			dhtMtx.lock(); /* LOCK MUTEX */

			mDhtState  = DHT_STATE_OFF;
#ifdef DHT_DEBUG
			std::cerr << "p3DhtMgr::checkDhtStatus() mDhtState -> OFF" << std::endl;
#endif

			dhtMtx.unlock(); /* UNLOCK MUTEX */
		}
	}

	if (toStartup)
	{
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::checkDhtStatus() toStartup = true -> init()" << std::endl;
#endif
		if (init())
		{

			dhtMtx.lock(); /* LOCK MUTEX */

			mDhtState  = DHT_STATE_INIT;
			mDhtActiveTS = time(NULL);

#ifdef DHT_DEBUG
			std::cerr << "p3DhtMgr::checkDhtStatus() mDhtState -> INIT" << std::endl;
#endif


			dhtMtx.unlock(); /* UNLOCK MUTEX */
		}
	}
}

void    p3DhtMgr::clearDhtData()
{
	std::cerr << "p3DhtMgr::clearDhtData() DUMMY FN" << std::endl;
}


/****************************** REAL DHT INTERFACE *********************************
 * publish/search/result.
 *
 * dummy implementation for testing.
 */

int  p3DhtMgr::status(std::ostream &out)
{
	dhtMtx.lock(); /* LOCK MUTEX */

	out << "p3DhtMgr::status() ************************************" << std::endl;
	out << "mDhtState: " << mDhtState << std::endl;
	out << "mDhtOn   : " << mDhtOn << std::endl;
	out << "dhtActive: " << dhtActive() << std::endl;

	/* now own state */
	out << "OWN DETAILS -------------------------------------------" << std::endl;
	printDhtPeerEntry(&ownEntry, out);
	out << "OWN DETAILS END----------------------------------------" << std::endl;

	/* now peers states */
	std::map<std::string, dhtPeerEntry>::iterator it;
	out << "PEER DETAILS ------------------------------------------" << std::endl;
	for(it = peers.begin(); it != peers.end(); it++)
	{
		printDhtPeerEntry(&(it->second), out);
	}
	out << "PEER DETAILS END---------------------------------------" << std::endl;
		
	/* now stun states */
	out << "STUN DETAILS ------------------------------------------" << std::endl;
	out << "Available Stun Ids: " << stunIds.size() << std::endl;
	out << "STUN DETAILS END---------------------------------------" << std::endl;


	out << "p3DhtMgr::status() END ********************************" << std::endl;

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	return 0;
}





bool p3DhtMgr::init()
{
	std::cerr << "p3DhtMgr::init() DUMMY FN" << std::endl;
	return true;	
}

bool p3DhtMgr::shutdown()
{
	std::cerr << "p3DhtMgr::shutdown() DUMMY FN" << std::endl;
	return true;	
}

bool p3DhtMgr::dhtActive()
{
	std::cerr << "p3DhtMgr::dhtActive() DUMMY FN" << std::endl;
	return true;	
}

bool p3DhtMgr::publishDHT(std::string key, std::string value, uint32_t ttl)
{
	std::cerr << "p3DhtMgr::publishDHT() DUMMY FN" << std::endl;
	return false;
}

bool p3DhtMgr::searchDHT(std::string idhash)
{
	std::cerr << "p3DhtMgr::searchDHT() DUMMY FN" << std::endl;
	return false;
}




/****************************** INTERMEDIATE DHT INTERFACE *********************************
 * publish/search/result.
 *
 * Take the 'real' parameters and create the key/value parameters for the real dht.
 */


bool p3DhtMgr::dhtPublish(std::string idhash, 
		struct sockaddr_in &laddr, struct sockaddr_in &raddr, 
		uint32_t type, std::string sign)
{
	/* ... goes and searches */
#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::dhtPublish()" << std::endl;

       	std::cerr << "PUBLISHing: idhash: " << RsUtil::BinToHex(idhash);
       	std::cerr << " laddr: " << inet_ntoa(laddr.sin_addr);
	std::cerr << ":" << ntohs(laddr.sin_port);
       	std::cerr << "   raddr: " << inet_ntoa(raddr.sin_addr);
	std::cerr << ":" << ntohs(raddr.sin_port);
       	std::cerr << "   type: " << type;
       	std::cerr << "   sign: " << sign;
	std::cerr << std::endl;
#endif

	/* Create a Value from addresses and type */
        /* to store the ip address and flags */

	std::ostringstream out;
	out << "RSDHT:" << std::setw(2) << std::setfill('0') << DHT_MODE_SEARCH << ": ";
	out << "IPL="   << inet_ntoa(laddr.sin_addr) << ":"  << ntohs(laddr.sin_port) << ", ";
	out << "IPE="   << inet_ntoa(raddr.sin_addr) << ":"  << ntohs(raddr.sin_port) << ", ";
	out << "type="  << std::setw(4) << std::setfill('0') << std::hex << type << ", ";

/*******
	char valuearray[1024];
	snprintf(valuearray, 1024, "RSDHT:%02d: IPL=%s:%d, IPE=%s:%d, type=%04X,",
			DHT_MODE_SEARCH,
	                inet_ntoa(laddr.sin_addr),
	                ntohs(laddr.sin_port),
	                inet_ntoa(raddr.sin_addr),
	                ntohs(raddr.sin_port),
	                type);

	std::string value = valuearray;
******/

	std::string value = out.str();

#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::dhtPublish()" << std::endl;

       	std::cerr << "PUBLISH: key: " << RsUtil::BinToHex(idhash);
       	std::cerr << " value: " << value;
	std::cerr << std::endl;
#endif

	/* call to the real DHT */
	return publishDHT(idhash, value, DHT_TTL_PUBLISH);
}

bool p3DhtMgr::dhtNotify(std::string idhash, std::string ownIdHash, std::string sign)
{
	std::cerr << "p3DhtMgr::dhtNotify()" << std::endl;

	std::ostringstream value;
	value << "RSDHT:" << std::setw(2) << std::setfill('0') << DHT_MODE_NOTIFY << ":";
	value << ownIdHash;

	/* call to the real DHT */
	return publishDHT(idhash, value.str(), DHT_TTL_NOTIFY);
}

bool p3DhtMgr::dhtSearch(std::string idhash, uint32_t mode)
{
	std::cerr << "p3DhtMgr::dhtSearch()" << std::endl;

	/* call to the real DHT */
	return searchDHT(idhash);
}





/****************************** DHT FEEDBACK INTERFACE *********************************
 * Two functions...
 * (1) The interpretation function.
 * (2) callback handling.
 *
 */

bool p3DhtMgr::resultDHT(std::string key, std::string value)
{
	/* so .... two possibilities.
	 *
	 * RSDHT:01: IPL ... IPE ... TYPE ... 	(advertising)
	 * RSDHT:02: HASH 			(connect request)
	 *
	 */

	/* variables for dhtResult() call */
	struct sockaddr_in laddr;
	struct sockaddr_in raddr; 
	std::string sign;


	int32_t  reqType;
	uint32_t loc;
	if (1 > sscanf(value.c_str(), "RSDHT:%d: %n", &reqType, &loc))
	{
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::resultDHT() Not RSDHT msg -> discarding" << std::endl;
#endif
		/* failed */
		return false;
	}


	dhtMtx.lock(); /* LOCK MUTEX */
	std::string ownhash = ownEntry.hash1;
	dhtMtx.unlock(); /* UNLOCK MUTEX */

	switch(reqType)
	{
		case DHT_MODE_NOTIFY:
		{
#ifdef DHT_DEBUG
			std::cerr << "p3DhtMgr::resultDHT() NOTIFY msg" << std::endl;
#endif

			if (ownhash != key)
			{
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::resultDHT() NOTIFY msg not for us -> discarding" << std::endl;
#endif
				return false;
			}

			/* get the hash */
			std::string notifyHash = value.substr(loc);
#ifdef DHT_DEBUG
			std::cerr << "p3DhtMgr::resultDHT() NOTIFY msg HASH:-> 0x" << RsUtil::BinToHex(notifyHash) << "<-" << std::endl;
#endif
			/* call out */
			dhtResultNotify(notifyHash);

			break;
		}

		case DHT_MODE_SEARCH:
		{

			if (ownhash == key)
			{
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::resultDHT() SEARCH msg is OWN PUBLISH -> discarding" << std::endl;
#endif
				return false;
			}

			uint32_t a1,b1,c1,d1,e1;
			uint32_t a2,b2,c2,d2,e2;
			uint32_t flags;

			if (sscanf(&((value.c_str())[loc]), " IPL=%d.%d.%d.%d:%d, IPE=%d.%d.%d.%d:%d, type=%x", 
				&a1, &b1, &c1, &d1, &e1, &a2, &b2, &c2, &d2, &e2, &flags) != 11)
			{
				/* failed to scan */
#ifdef DHT_DEBUG
				std::cerr << "p3DhtMgr::resultDHT() SEARCH parse failed of:" << (&((value.c_str())[loc]));
				std::cerr << std::endl;
#endif
				return false;
			}

                        std::ostringstream out1;
			out1 << a1 << "." << b1 << "." << c1 << "." << d1;
			inet_aton(out1.str().c_str(), &(laddr.sin_addr));
			laddr.sin_port = htons(e1);
			laddr.sin_family = AF_INET;

                        std::ostringstream out2;
			out2 << a2 << "." << b2 << "." << c2 << "." << d2;
			inet_aton(out2.str().c_str(), &(raddr.sin_addr));
			raddr.sin_port = htons(e2);
			raddr.sin_family = AF_INET;

#ifdef DHT_DEBUG
			std::cerr << "p3DhtMgr::resultDHT() SEARCH laddr: " << out1.str() << ":" << e1;
			std::cerr << " raddr: " << out2.str() << ":" << e2;
			std::cerr << " flags: " << flags;
			std::cerr << std::endl;
#endif

			return dhtResultSearch(key, laddr, raddr, flags, sign);

			break;
		}

		default:
		
			return false;
			break;
	}

	return false;
}






bool p3DhtMgr::dhtResultNotify(std::string idhash)
{
	RsStackMutex stack(dhtMtx); /***** LOCK MUTEX *****/

#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::dhtResultNotify() from idhash: ";
	std::cerr << RsUtil::BinToHex(idhash) << std::endl;
#endif
	std::map<std::string, dhtPeerEntry>::iterator it;
	time_t now = time(NULL);

	/* if notify - we must match on the second hash */
	for(it = peers.begin(); (it != peers.end()) && ((it->second).hash2 != idhash); it++);

	/* update data */
	std::string peerid;

	if (it != peers.end())
	{
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::dhtResult() NOTIFY for id: " << it->first << std::endl;
#endif
		/* delay callback -> if they are not found */
		it->second.notifyTS      = now;
		it->second.notifyPending = RS_CONNECT_PASSIVE;
		mDhtModifications = true; /* no wait! */

		if (it->second.state != DHT_PEER_FOUND)
		{
			/* flag for immediate search */
			it->second.lastTS = 0;
		}
	}
	else
	{
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::dhtResult() unknown NOTIFY ";
		std::cerr << RsUtil::BinToHex(idhash) << std::endl;
#endif
	}

	return true;
}


bool p3DhtMgr::dhtResultSearch(std::string idhash, 
		struct sockaddr_in &laddr, struct sockaddr_in &raddr, 
		uint32_t type, std::string sign)
{
	dhtMtx.lock(); /* LOCK MUTEX */

#ifdef DHT_DEBUG
	std::cerr << "p3DhtMgr::dhtResultSearch() for idhash: ";
	std::cerr << RsUtil::BinToHex(idhash) << std::endl;
#endif
	std::map<std::string, dhtPeerEntry>::iterator it;
	bool doCb = false;
	bool doStun = false;
	uint32_t stunFlags = 0;
	time_t now = time(NULL);

	dhtPeerEntry ent;

	/* if search - we must match on the second hash */
	for(it = peers.begin(); (it != peers.end()) && ((it->second).hash1 != idhash); it++);

	/* update data */
	if (it != peers.end())
	{
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::dhtResult() SEARCH for id: " << it->first << std::endl;
#endif
		it->second.lastTS = now;

		/* update info .... always */
		it->second.state = DHT_PEER_FOUND;
		it->second.laddr  = laddr;
		it->second.raddr  = raddr;
		it->second.type  = type;

		/* Do callback all the time */
		ent = it->second;
		doCb = true;

		if (it->second.notifyPending)
		{
			/* no wait if we have pendingNotification */
			mDhtModifications = true; 
		}

		/* do Stun always */
		doStun = true;
		stunFlags = RS_STUN_FRIEND | RS_STUN_ONLINE;
	}
	else
	{
#ifdef DHT_DEBUG
		std::cerr << "p3DhtMgr::dhtResult() SEARCH(stun) for idhash: ";
		std::cerr << RsUtil::BinToHex(idhash) << std::endl;
#endif
		/* stun result? */
		doStun = true;
		stunFlags = RS_STUN_ONLINE;
	}

	dhtMtx.unlock(); /* UNLOCK MUTEX */

	if (doCb)
	{
		connCb->peerStatus(ent.id, ent.laddr, ent.raddr, 
				ent.type, 0, RS_CB_DHT);
	}

	if (doStun)
	{
		connCb->stunStatus(idhash, raddr, type, stunFlags);
	}
			
	return true;
}



/******************************** AUX FUNCTIONS **********************************
 *
 */

void printDhtPeerEntry(dhtPeerEntry *ent, std::ostream &out)
{
	
	out << "DhtEntry: ID: " << ent->id;
	out << " State: " << ent->state;
	out << " lastTS: " << ent->lastTS;
	out << " notifyPending: " << ent->notifyPending;
	out << " notifyTS: " << ent->notifyTS;
	out << std::endl;
	out << " laddr: " << inet_ntoa(ent->laddr.sin_addr) << ":" << ntohs(ent->laddr.sin_port);
	out << " raddr: " << inet_ntoa(ent->raddr.sin_addr) << ":" << ntohs(ent->raddr.sin_port);
	out << " type: " << ent->type;
	out << std::endl;
	out << " hash1: " << RsUtil::BinToHex(ent->hash1);
	out << std::endl;
	out << " hash2: " << RsUtil::BinToHex(ent->hash2);
	out << std::endl;
	return;
}


