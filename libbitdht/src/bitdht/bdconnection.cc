/*
 * bitdht/bdconnection.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2011 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */

#include <algorithm>
#include "bitdht/bdiface.h"

#include "bitdht/bdnode.h"
#include "bitdht/bdquerymgr.h"
#include "bitdht/bdconnection.h"
#include "bitdht/bdmsgs.h"
#include "bitdht/bdstddht.h"
#include "util/bdnet.h"
#include "util/bdrandom.h"

/*
 * #define DEBUG_NODE_CONNECTION		1 
 * #define DEBUG_NODE_CONNECTION_EXTRA		1 
 */

#define DEBUG_NODE_CONNECTION		1 


#define BITDHT_CR_PAUSE_BASE_PERIOD 5
#define BITDHT_CR_PAUSE_RND_PERIOD  15

#define MAX_NUM_RETRIES 3

uint32_t createConnectionErrorCode(uint32_t userProvided, uint32_t fallback, uint32_t point);

/************************************************************************************************************
******************************************** Connection Config **********************************************
************************************************************************************************************/

bdConnectManager::bdConnectManager(bdNodeId *ownId, bdSpace *space, bdQueryManager *qmgr, bdDhtFunctions *fns, bdNodePublisher *pub)
	:mOwnId(*ownId), mNodeSpace(space), mQueryMgr(qmgr), mFns(fns), mPub(pub)
{
	defaultConnectionOptions();
}
	



void bdConnectManager::defaultConnectionOptions()
{
	/* by default we want to help people proxy connections.
	 * As this involves no interaction at higher levels, 
	 * we want ALL BitDHT clients to support - unless explicitly disabled.
	 */

	setConnectionOptions(BITDHT_CONNECT_MODE_PROXY, 
			BITDHT_CONNECT_OPTION_AUTOPROXY);
}

void bdConnectManager::setConnectionOptions(uint32_t allowedModes, uint32_t flags)
{
	mConfigAllowedModes = allowedModes;
	mConfigAutoProxy = false;

	if (flags & BITDHT_CONNECT_OPTION_AUTOPROXY)
	{
		mConfigAutoProxy = true;
	}

}

void bdConnectManager::shutdownConnections()
{
	std::cerr << "bdConnectManager::shutdownConnections() Brutal Shutdown of the following connections: ";
	std::cerr << std::endl;

	printConnections();

	mConnectionRequests.clear();
	mConnections.clear();

}

void bdConnectManager::printConnections()
{
	std::cerr << "bdConnectManager::printConnections()";
	std::cerr << std::endl;

	std::map<bdNodeId, bdConnectionRequest>::iterator it;
	for(it = mConnectionRequests.begin(); it != mConnectionRequests.end(); it++)
	{
		std::cerr << "bdConnectManager::printConnections() Connect Request:";
		std::cerr << std::endl;
		std::cerr << it->second;
		std::cerr << std::endl;
	}

	std::map<bdProxyTuple, bdConnection>::iterator tit;
	for(tit = mConnections.begin(); tit != mConnections.end(); tit++)
	{
		std::cerr << "bdConnectManager::printConnections() ConnectAttempt:";
		std::cerr << std::endl;
		std::cerr << tit->second;
		std::cerr << std::endl;
	}
}






/************************************************************************************************************
****************************************** Connection Initiation ********************************************
************************************************************************************************************/


/* This is called to initialise a connection.
 * the callback could be with regard to:
 * a Direct EndPoint.
 * a Proxy Proxy, or an Proxy EndPoint.
 * a Relay Proxy, or an Relay EndPoint.
 *
 * We have two alternatives:
 *  1) Direct Endpoint.
 *  2) Using a Proxy.
 */

int bdConnectManager::requestConnection(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode, uint32_t start)
{
	/* check if connection obj already exists */
#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdConnectManager::requestConnection() Mode: " << mode;
	std::cerr << " Start: " << start;
	std::cerr << " Target: ";
	mFns->bdPrintNodeId(std::cerr, target);
	std::cerr << " Local NetAddress: " << inet_ntoa(laddr->sin_addr);
        std::cerr << ":" << ntohs(laddr->sin_port);
	std::cerr << std::endl;
#endif

	if (!start)
	{
		return killConnectionRequest(laddr, target, mode);
	}

	if (!(mConfigAllowedModes & mode))
	{
		/* MODE not supported */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::requestConnection() Mode Not Supported";
		std::cerr << std::endl;
#endif
		return 0;
	}

	// Seems like a dumb one, but the testing picked it up.
	if (*target == mOwnId)
	{
		/* MODE not supported */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::requestConnection() WARNING Not allowing connection to self";
		std::cerr << std::endl;
#endif
		return 0;
	}

	if (mode == BITDHT_CONNECT_MODE_DIRECT)
	{
		return requestConnection_direct(laddr, target);
	}
	else
	{
		return requestConnection_proxy(laddr, target, mode);
	}
}

int bdConnectManager::checkExistingConnectionAttempt(bdNodeId *target)
{
	std::map<bdNodeId, bdConnectionRequest>::iterator it;
	it = mConnectionRequests.find(*target);
	if (it != mConnectionRequests.end())
	{
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::checkExistingConnectAttempt() Found Existing Connection!";
		std::cerr << std::endl;
#endif
		return 1;
	}
	return 0;
}


int bdConnectManager::killConnectionRequest(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode)
{
	/* check if connection obj already exists */
#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdConnectManager::killConnectionRequest() Mode: " << mode;
	std::cerr << " Target: ";
	mFns->bdPrintNodeId(std::cerr, target);
	std::cerr << " Local NetAddress: " << inet_ntoa(laddr->sin_addr);
        std::cerr << ":" << ntohs(laddr->sin_port);
	std::cerr << std::endl;
#endif

	std::map<bdNodeId, bdConnectionRequest>::iterator it;
	it = mConnectionRequests.find(*target);
	if (it == mConnectionRequests.end())
	{
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::killConnectionRequest() ERROR Request not there!";
		std::cerr << std::endl;
#endif
		return 0;
	}

	std::cerr << "bdConnectManager::killConnectionRequest() Flagging Connection Request as DONE";
	std::cerr << std::endl;

	time_t now = time(NULL);
	it->second.mState = BITDHT_CONNREQUEST_DONE;
	it->second.mStateTS = now;
	it->second.mErrCode = BITDHT_CONNECT_ERROR_SOURCE_START |
				BITDHT_CONNECT_ERROR_USER;
	
	
	
	return 1;
}




#define MIN_START_DIRECT_COUNT 		1
#define MIN_START_PROXY_COUNT 		10
#define CONNECT_NUM_PROXY_ATTEMPTS	10


int bdConnectManager::requestConnection_direct(struct sockaddr_in *laddr, bdNodeId *target)
{

#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdConnectManager::requestConnection_direct()";
	std::cerr << std::endl;
#endif
	/* create a bdConnect, and put into the queue */
	int mode = BITDHT_CONNECT_MODE_DIRECT;
	bdConnectionRequest connreq;
	
	if (checkExistingConnectionAttempt(target))
	{
		return 0;
	}

	connreq.setupDirectConnection(laddr, target);

	mQueryMgr->result(target, connreq.mGoodProxies);

	/* now look in the bdSpace as well */
	if (connreq.mGoodProxies.size() < MIN_START_DIRECT_COUNT)
	{
		int number = CONNECT_NUM_PROXY_ATTEMPTS;
		int with_flag = BITDHT_PEER_STATUS_DHT_ENGINE_VERSION;
		std::list<bdId> matchIds;
		std::list<bdId>::iterator it;
		std::list<bdId>::iterator pit;
		mNodeSpace->find_node(target, number, matchIds, with_flag);

		/* merge lists (costly should use sets or something) */
		for(it = matchIds.begin(); it != matchIds.end(); it++)
		{
			pit = std::find(connreq.mGoodProxies.begin(), connreq.mGoodProxies.end(), *it);
			if (pit != connreq.mGoodProxies.end())
			{
				connreq.mGoodProxies.push_back(*it);
			}
		}
	}

	/* Actually if we lots of ids at this point... its likely that something is wrong 
	 */

	if (connreq.mGoodProxies.size() > 1)
	{
		std::cerr << "bdConnectManager::requestConnection_direct() ERROR Multiple Peers for DIRECT connection";
		std::cerr << std::endl;
	}

#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdConnectManager::requestConnection_direct() CRINITSTATE Init Connection State";
	std::cerr << std::endl;
	std::cerr << connreq;
	std::cerr << std::endl;
#endif

	/* push connect onto queue, for later completion */

	mConnectionRequests[*target] = connreq;

	/* connection continued via iterator */
	return 1;
}

 
int bdConnectManager::requestConnection_proxy(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode)
{

#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdConnectManager::requestConnection_proxy()";
	std::cerr << std::endl;
#endif

	/* create a bdConnect, and put into the queue */

	bdConnectionRequest connreq;
	connreq.setupProxyConnection(laddr, target, mode);

	/* grab any peers from any existing query */
	std::list<bdId>::iterator pit;
	std::list<bdId> potentialProxies;
	mQueryMgr->proxies(target, connreq.mGoodProxies);
	mQueryMgr->potentialProxies(target, potentialProxies);

	/* check any potential proxies, must be same DHT Type */
	for(pit = potentialProxies.begin(); pit != potentialProxies.end(); )
	{
		/* check the type in bdSpace */
		if (checkPeerForFlag(&(*pit), BITDHT_PEER_STATUS_DHT_ENGINE_VERSION))
		{
			connreq.mGoodProxies.push_back(*pit);
			pit = potentialProxies.erase(pit);
		}
		else
		{
			pit++;
		}
	}


	/* if we don't have enough proxies ... ping the potentials */
	if (connreq.mGoodProxies.size() < MIN_START_PROXY_COUNT)
	{
		/* unknown, add to potential list, and ping! */
		for(pit = potentialProxies.begin(); pit != potentialProxies.end(); pit++)
		{

			connreq.mPotentialProxies.push_back(*pit);

			// If the pings come back will be handled by
			// updatePotentialConnectionProxy()

			mPub->send_ping(&(*pit));

			std::cerr << "bdConnectManager::requestConnection_proxy() Pinging Potential Proxy";
			mFns->bdPrintId(std::cerr, &(*pit));
			std::cerr << std::endl;
		}
	}

	// Final Desperate Measures!
	if (connreq.mGoodProxies.size() < MIN_START_PROXY_COUNT)
	{
		/* now find closest acceptable peers, 
	 	 * and trigger a search for target...
	 	 * this will hopefully find more suitable proxies.
	 	 */
	
		std::list<bdId> excluding;
		std::multimap<bdMetric, bdId> nearest;
	

		int number = CONNECT_NUM_PROXY_ATTEMPTS;
	
		mNodeSpace->find_nearest_nodes_with_flags(target, number, excluding, nearest, 
				BITDHT_PEER_STATUS_DHT_FOF       |
				BITDHT_PEER_STATUS_DHT_FRIEND);

		// just ask for the same number of closest (above doesn't return anything atm)	
		//int nFound = nearest.size();	
		//number = CONNECT_NUM_PROXY_ATTEMPTS - nFound;
	
		mNodeSpace->find_nearest_nodes_with_flags(target, number, excluding, nearest, 
								BITDHT_PEER_STATUS_DHT_ENGINE_VERSION );
	
		std::multimap<bdMetric, bdId>::iterator it;
		for(it = nearest.begin(); it != nearest.end(); it++)
		{
			std::cerr << "bdConnectManager::requestConnection_proxy() is Entry it connected to Friend? : ";
			mFns->bdPrintId(std::cerr, &(it->second));
			std::cerr << std::endl;

			//bdNodeId midId;
			//mFns->bdRandomMidId(target, &(it->second.id), &midId);
			/* trigger search */
			//mPub->send_query(&(it->second), &midId);
			mPub->send_query(&(it->second), target);
		}
	}


	if (connreq.mGoodProxies.size() < 1)
	{
		std::cerr << "bdConnectManager::requestConnection_proxy() ERROR initial proxyList.size() == 0";
		std::cerr << std::endl;
	}

	if (connreq.mGoodProxies.size() < MIN_START_PROXY_COUNT)
	{
		std::cerr << "bdConnectManager::requestConnection_proxy() WARNING initial proxyList.size() == SMALL PAUSING";
		std::cerr << std::endl;

		time_t now = time(NULL);
		/* PAUSE the connection Attempt, so we can wait for responses */
		connreq.mState = BITDHT_CONNREQUEST_PAUSED;
		connreq.mPauseTS = now + BITDHT_CR_PAUSE_BASE_PERIOD +
				(int) (bdRandom::random_f32() * BITDHT_CR_PAUSE_RND_PERIOD);
	}

#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdConnectManager::requestConnection_proxy() CRINITSTATE Init Connection State";
	std::cerr << std::endl;
	std::cerr << connreq;
	std::cerr << std::endl;
#endif


	/* push connect onto queue, for later completion */
	mConnectionRequests[*target] = connreq;
	
	return 1;
}

void bdConnectManager::addPotentialConnectionProxy(const bdId *srcId, const bdId *target)
{
#ifdef DEBUG_NODE_CONNECTION_EXTRA
	std::cerr << "bdConnectManager::addPotentialConnectionProxy() ";
	std::cerr << " srcId: ";
	bdStdPrintId(std::cerr, srcId);
	std::cerr << " target: ";
	bdStdPrintId(std::cerr, target);
	std::cerr << std::endl;
#endif

	if (!srcId)
	{
		/* not one of our targets... drop it */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::addPotentialConnectionProxy() srcID = NULL, useless to us";
		std::cerr << std::endl;
#endif
		return;
	}
	
	std::map<bdNodeId, bdConnectionRequest>::iterator it;
	it = mConnectionRequests.find(target->id);
	if (it == mConnectionRequests.end())
	{
		/* not one of our targets... drop it */
#ifdef DEBUG_NODE_CONNECTION_EXTRA
		std::cerr << "bdConnectManager::addPotentialConnectionProxy() Dropping Not one of Our Targets";
		std::cerr << std::endl;
#endif
		return;
	}

	if (it->second.mMode == BITDHT_CONNECT_MODE_DIRECT)
	{
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::addPotentialConnectionProxy() Dropping Target is DIRECT";
		std::cerr << std::endl;
#endif
		return;
	}

	/* This is one is strange elsewhere.... srcId = targetId.
	 * This means that peer is actually reachable! and we should be connecting directly.
	 * however there is not much we can do about it here. Really up to higher level logic.
	 */
	if (srcId->id == target->id)
	{
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::addPotentialConnectionProxy() ERROR srcId.id == target.id (more of a WARNING)";
		std::cerr << std::endl;
		std::cerr << "bdConnectManager::addPotentialConnectionProxy() NB: This means peer is actually reachable....";
		std::cerr << std::endl;
		std::cerr << "bdConnectManager::addPotentialConnectionProxy() and we should be connecting directly. Oh Well!";
		std::cerr << std::endl;
#endif
		return;
	}

	if (checkPeerForFlag(srcId, BITDHT_PEER_STATUS_DHT_ENGINE_VERSION))
	{
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::addPotentialConnectionProxy() Src passes FLAG test";
		std::cerr << std::endl;
#endif
		it->second.addGoodProxy(srcId);
	}
	else
	{
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::addPotentialConnectionProxy() Dropping SrcId failed FLAG test";
		std::cerr << std::endl;
#endif
	}
}


int bdConnectManager::checkPeerForFlag(const bdId *id, uint32_t with_flag)
{
	/* check the type in bdSpace */
	bdPeer peer;
	if (mNodeSpace->find_exactnode(id, peer))
	{
		if (peer.mPeerFlags & with_flag)
		{
			return 1;
		}
	}
	/* XXX eventually we'll need to check against extra peer lists.
	 * with our friends, etc
	 *
	 * ideally we'll track this info in the query!
	 */

	return 0;
}


void bdConnectManager::updatePotentialConnectionProxy(const bdId *id, uint32_t mode)
{
	if (mode & BITDHT_PEER_STATUS_DHT_ENGINE_VERSION)
	{
#ifdef DEBUG_NODE_CONNECTION_EXTRA
		std::cerr << "bdConnectManager::updatePotentialConnectionProxy() Peer is GOOD : ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;
		std::cerr << "bdConnectManager::updatePotentialConnectionProxy() checking Connection Requests";
		std::cerr << std::endl;
#endif
		/* good peer, see if any of our connectionrequests can use it */
		std::map<bdNodeId, bdConnectionRequest>::iterator it;
		for(it = mConnectionRequests.begin(); it != mConnectionRequests.end(); it++)
		{
			it->second.checkGoodProxyPeer(id);
		}
	}
}


int bdConnectManager::tickConnections()
{
	iterateConnectionRequests();
	iterateConnections();
	
	return 1;
}


void bdConnectManager::iterateConnectionRequests()
{
	time_t now = time(NULL);

	std::list<bdNodeId> eraseList;
	std::list<bdNodeId>::iterator eit;

	std::map<bdNodeId, bdConnectionRequest>::iterator it;
	for(it = mConnectionRequests.begin(); it != mConnectionRequests.end(); it++)
	{
		bool erase = false;

#ifdef DEBUG_NODE_CONNECTION_EXTRA
		std::cerr << "bdConnectManager::iterateConnectionAttempt() Request is:";
		std::cerr << std::endl;
		std::cerr << it->second;
		std::cerr << std::endl;
#endif

		/* check status of connection */
		if (it->second.mState == BITDHT_CONNREQUEST_READY)
		{
			std::cerr << "bdConnectManager::iterateConnectionAttempt() Request is READY, starting";
			std::cerr << std::endl;

			/* kick off the connection if possible */
			// goes to BITDHT_CONNREQUEST_INPROGRESS;
			if (!startConnectionAttempt(&(it->second)))
			{
				// FAILS if proxy is bad / nonexistent
				std::cerr << "bdConnectManager::iterateConnectionAttempt() Failed startup => PAUSED";
				std::cerr << std::endl;
				std::cerr << it->second;
				std::cerr << std::endl;

				/* timeout and restart */
				it->second.mState = BITDHT_CONNREQUEST_PAUSED;
				it->second.mStateTS = now;
				it->second.mPauseTS = now + BITDHT_CR_PAUSE_BASE_PERIOD + 
					(int) (bdRandom::random_f32() * BITDHT_CR_PAUSE_RND_PERIOD);

			}
		}
		else if (it->second.mState == BITDHT_CONNREQUEST_PAUSED)
		{
			/* forced pause, with period specified at PAUSE point */
			if (now > it->second.mPauseTS)
			{
				std::cerr << "bdConnectManager::iterateConnectionAttempt() PAUSED has reached timout -> READY";
				std::cerr << std::endl;

				/* if we have run out of proxies, or recycled too many times. kill it */	
				if (it->second.mGoodProxies.size() == 0)
				{
					std::cerr << "bdConnectManager::iterateConnectionAttempt() no more proxies => DONE";
					std::cerr << std::endl;

					it->second.mErrCode = BITDHT_CONNECT_ERROR_SOURCE_START |
								BITDHT_CONNECT_ERROR_OUTOFPROXY;
					it->second.mState = BITDHT_CONNREQUEST_DONE;
					it->second.mStateTS = now;
				}
				else if (it->second.mRecycled > it->second.mGoodProxies.size() * MAX_NUM_RETRIES)
				{
					std::cerr << "bdConnectManager::iterateConnectionAttempt() to many retries => DONE";
					std::cerr << std::endl;

					it->second.mErrCode = BITDHT_CONNECT_ERROR_SOURCE_START |
								BITDHT_CONNECT_ERROR_TOOMANYRETRY;
					it->second.mState = BITDHT_CONNREQUEST_DONE;
					it->second.mStateTS = now;
				}
				else
				{
					/* timeout and restart */
					it->second.mState = BITDHT_CONNREQUEST_READY;
					it->second.mStateTS = now;
				}
			}
		}
		else if (it->second.mState == BITDHT_CONNREQUEST_INPROGRESS)
		{
			/* single connection attempt */
			if (now - it->second.mStateTS > BITDHT_CONNREQUEST_TIMEOUT_INPROGRESS)
			{
				std::cerr << "bdConnectManager::iterateConnectionAttempt() INPROGRESS has reached timout -> READY";
				std::cerr << std::endl;
				std::cerr << it->second;
				std::cerr << std::endl;
				
				/* timeout and restart */
				it->second.mState = BITDHT_CONNREQUEST_PAUSED;
				it->second.mStateTS = now;
				it->second.mPauseTS = now + BITDHT_CR_PAUSE_BASE_PERIOD + 
					(int) (bdRandom::random_f32() * BITDHT_CR_PAUSE_RND_PERIOD);
			}
		}
		else if (it->second.mState == BITDHT_CONNREQUEST_EXTCONNECT)
		{
			/* connection completed, doing UDP connection */
			if (now - it->second.mStateTS > BITDHT_CONNREQUEST_TIMEOUT_CONNECT)
			{
				std::cerr << "bdConnectManager::iterateConnectionAttempt() EXTCONNECT has reached timout ->????";
				std::cerr << std::endl;
				std::cerr << it->second;
				std::cerr << std::endl;

				/* timeout and restart */
				it->second.mState = BITDHT_CONNREQUEST_PAUSED;
				it->second.mStateTS = now;
				it->second.mPauseTS = now + BITDHT_CR_PAUSE_BASE_PERIOD + 
					(int) (bdRandom::random_f32() * BITDHT_CR_PAUSE_RND_PERIOD);
			}
		}
		else if (it->second.mState == BITDHT_CONNREQUEST_DONE)
		{
			std::cerr << "bdConnectManager::iterateConnectionAttempt() DONE -> erase";
			std::cerr << std::endl;
			std::cerr << it->second;
			std::cerr << std::endl;

			erase = true;

		}

		// Cleanup
		if (now - it->second.mStateTS > BITDHT_CONNREQUEST_MAX_AGE)
		{
			std::cerr << "bdConnectManager::iterateConnectionAttempt() Cleaning Old ConnReq: ";
			std::cerr << std::endl;
			std::cerr << it->second;
			std::cerr << std::endl;
		}

		if (erase)
		{
			/* do callback */
			bdId srcId;
			bdId proxyId;
			bdId destId;
		
			destId.id = it->second.mTarget;	
			callbackConnect(&srcId, &proxyId, &destId, it->second.mMode, 
					BD_PROXY_CONNECTION_START_POINT, 
					BITDHT_CONNECT_CB_REQUEST, it->second.mErrCode);

			/* cleanup */
			eraseList.push_back(it->first);
		}
	}
	
	for(eit = eraseList.begin(); eit != eraseList.end(); eit++)
	{
		it = mConnectionRequests.find(*eit);
		if (it != mConnectionRequests.end())
		{
			mConnectionRequests.erase(it);
		}
	}
}



int bdConnectManager::startConnectionAttempt(bdConnectionRequest *req)
{
	std::cerr << "bdConnectManager::startConnectionAttempt() ConnReq: ";
	std::cerr << std::endl;
	std::cerr << *req;
	std::cerr << std::endl;

	if (req->mGoodProxies.size() < 1)
	{
		std::cerr << "bdConnectManager::startConnectionAttempt() No Potential Proxies... delaying attempt";
		std::cerr << std::endl;
		return 0;
	}

	bdId proxyId;
	bdId srcConnAddr;
	bdId destConnAddr;

	int mode = req->mMode;

	destConnAddr.id = req->mTarget;
	bdsockaddr_clear(&(destConnAddr.addr));

	srcConnAddr.id = mOwnId;
	srcConnAddr.addr = req->mLocalAddr;

	proxyId = req->mGoodProxies.front();
	req->mGoodProxies.pop_front();

	req->mCurrentAttempt = proxyId;
	//req->mPeersTried.push_back(proxyId);	

	req->mState = BITDHT_CONNREQUEST_INPROGRESS;
	req->mStateTS = time(NULL);

	bool failProxy = false;
	if (mode == BITDHT_CONNECT_MODE_DIRECT)	
	{
		// ONE BUG I HAVE SEEN.
		if (!(req->mTarget == proxyId.id))
		{
			std::cerr << "bdConnectManager::startConnectionAttempt() ERROR Trying to use a Proxy for DIRECT";
			std::cerr << std::endl;

			return 0;
		}
	}
	else
	{
		if (req->mTarget == proxyId.id)
		{
			std::cerr << "bdConnectManager::startConnectionAttempt() ERROR Trying connect direct for PROXY|RELAY";
			std::cerr << std::endl;

			return 0;
		}
	}


	return startConnectionAttempt(&proxyId, &srcConnAddr, &destConnAddr, mode);
}



/************************************************************************************************************
****************************************** Outgoing Triggers ************************************************
************************************************************************************************************/

/************************************************************************************************************
****************************************** Outgoing Triggers ************************************************
************************************************************************************************************/

/************************************************************************************************************
*************************************** Connection Requests Callback ****************************************
************************************************************************************************************/

/* Lots of Callbacks come through here... The Connection Request gets flagged, and the message
 * get passed on up if necessary.
 */

void bdConnectManager::callbackConnectRequest(bdId *srcId, bdId *proxyId, bdId *destId, 
					int mode, int point, int cbtype, int errcode)
{
	/* Check if we are the originator of the Connect Request. If so, then we do stuff to the CR.
	 */
        std::cerr << "bdConnectManager::callbackConnectRequest() ";
        std::cerr << "mode: " << mode;
        std::cerr << " point: " << point;
        std::cerr << " cbtype: " << cbtype;
        std::cerr << " errcode: " << errcode;
        std::cerr << std::endl;

        std::cerr << "\tsrcId: ";
        bdStdPrintId(std::cerr, srcId);
        std::cerr << std::endl;
        std::cerr << "\tproxyId: ";
        bdStdPrintId(std::cerr, proxyId);
        std::cerr << std::endl;
        std::cerr << "\tdestId: ";
        bdStdPrintId(std::cerr, destId);
        std::cerr << std::endl;


	if (point != BD_PROXY_CONNECTION_START_POINT)
	{
		/* ONLY ONE CASE THAT GOES HERE -> for sanity testing */
		if ((cbtype == BITDHT_CONNECT_CB_START) && (point == BD_PROXY_CONNECTION_END_POINT))
		{
        		std::cerr << "bdConnectManager::callbackConnectRequest() END & START checking ConnectRequest state";
        		std::cerr << std::endl;

			// Reverse lookup (srcId).
			std::map<bdNodeId, bdConnectionRequest>::iterator it =  mConnectionRequests.find(srcId->id);
			if (it != mConnectionRequests.end())
			{
				if (it->second.mState == BITDHT_CONNREQUEST_INPROGRESS)
				{
        				std::cerr << "bdConnectManager::callbackConnectRequest() ERROR alt CR also in progress!";
        				std::cerr << std::endl;
				}
			}
			callbackConnect(srcId, proxyId, destId, mode, point, cbtype, errcode);
			return;
		}

        	std::cerr << "bdConnectManager::callbackConnectRequest() ";
		std::cerr << "ERROR point != START, should not be receiving this callback, ignoring";
        	std::cerr << std::endl;
		return;
	}

	/* now find our peer in the map */
	std::map<bdNodeId, bdConnectionRequest>::iterator it =  mConnectionRequests.find(destId->id);
	if (it == mConnectionRequests.end())
	{
        	std::cerr << "bdConnectManager::callbackConnectRequest() ";
		std::cerr << "ERROR no associated Connection Request, ignoring";
        	std::cerr << std::endl;
		return;
	}
	bdConnectionRequest *cr = &(it->second);
	time_t now = time(NULL);

	/* what types of cbtype can we get?
	 *	BITDHT_CONNECT_CB_AUTH    not as START 
	 *	BITDHT_CONNECT_CB_PENDING not as START 
	 *	BITDHT_CONNECT_CB_START   YES important, change state to PAUSED and pass up 
	 *	BITDHT_CONNECT_CB_PROXY   not as START 
	 *	BITDHT_CONNECT_CB_FAILED  YES most important, trigger next one 
 	 */

	switch(cbtype)
	{
		default:  // all fallthrough.
	 	case BITDHT_CONNECT_CB_AUTH:
	 	case BITDHT_CONNECT_CB_PENDING:
	 	case BITDHT_CONNECT_CB_PROXY:
		{
        		std::cerr << "bdConnectManager::callbackConnectRequest() ";
			std::cerr << "ERROR unexpected CBTYPE: AUTH/PENDING/PROXY/other. ignoring";
        		std::cerr << std::endl;
			return;
		}
			
	 	case BITDHT_CONNECT_CB_FAILED:
		{
        		std::cerr << "bdConnectManager::callbackConnectRequest() ";
			std::cerr << "Connection FAILED.... determining if fatal/recycle/next";
        		std::cerr << std::endl;

			// one more big switch statement, to decide: fatal/delay/or next
			// default is move to next proxy/peer.
			bool fatal = false;
			bool recycle = false;

			int errtype = errcode & BITDHT_CONNECT_ERROR_MASK_TYPE;
			int errsrc = errcode & BITDHT_CONNECT_ERROR_MASK_SOURCE;

			switch(errtype)
			{
				default:
				// (These could be fatal or recycle cases... but really ERROR, try NEXT.
				case BITDHT_CONNECT_ERROR_GENERIC:
				case BITDHT_CONNECT_ERROR_PROTOCOL:
				case BITDHT_CONNECT_ERROR_TIMEOUT:   // SHould never receive. 
				{
        				std::cerr << "bdConnectManager::callbackConnectRequest() ";
					std::cerr << "ERROR unexpected errcode: " << errcode;
        				std::cerr << std::endl;
				}
					break;

				// FATAL ONES.
				case BITDHT_CONNECT_ERROR_UNREACHABLE: // END has Unstable ExtAddr. ONLY(PROXYMODE,END)
				{
					if ((errsrc == BITDHT_CONNECT_ERROR_SOURCE_END) && 
							(mode == BITDHT_CONNECT_MODE_PROXY))
					{
						// fatal.
						fatal = true;

	        				std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "END says UNREACHABLE. FATAL ;(";
       		 				std::cerr << std::endl;
					}
					else
					{
						// error.
	        				std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "ERROR, UNREACHABLE, but !END";
       		 				std::cerr << std::endl;
					}
				}
					break;
				case BITDHT_CONNECT_ERROR_AUTH_DENIED: // END won't accept conn   END|PROXY, RELAY|PROXY
				{
					if (errsrc == BITDHT_CONNECT_ERROR_SOURCE_END)
					{
						// fatal.
						fatal = true;

	        				std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "END says AUTH_DENIED, fatal";
       		 				std::cerr << std::endl;
					}
					else if (errsrc == BITDHT_CONNECT_ERROR_SOURCE_MID)
					{	
						// next. (unlikely).
	        				std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "WARNING MID says AUTH_DENIED";
       		 				std::cerr << std::endl;
					}
					else
					{
						// error.
	        				std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "ERROR strange AUTH_DENIED";
       		 				std::cerr << std::endl;
					}
				}
					break;
				case BITDHT_CONNECT_ERROR_UNSUPPORTED: // mode is unsupprted. fatal or next ANY/ANY
				{
					if (errsrc == BITDHT_CONNECT_ERROR_SOURCE_END)
					{
						// fatal.
						fatal = true;

	        				std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "END says UNSUPPORTED, fatal";
       		 				std::cerr << std::endl;
					}
					else if (errsrc == BITDHT_CONNECT_ERROR_SOURCE_MID)
					{	
						// next.
	        				std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "MID says UNSUPPORTED";
       		 				std::cerr << std::endl;

					}
					else
					{
						// error.
	        				std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "ERROR strange UNSUPPORTED";
       		 				std::cerr << std::endl;

					}
				}
					break;

				// RECYCLE PROXY
				case BITDHT_CONNECT_ERROR_TEMPUNAVAIL: // only END | PROXY, no extAddress
				{
					if (errsrc == BITDHT_CONNECT_ERROR_SOURCE_END)
					{
						recycle = true;

	        				std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "END says TEMPUNAVAIL, recycle";
       		 				std::cerr << std::endl;
					}
					else 
					{	
						// next.
	        				std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "ERROR strange TEMPUNAVAIL";
       		 				std::cerr << std::endl;

					}
				}
				case BITDHT_CONNECT_ERROR_DUPLICATE: // similar attempt. delay/recycle (ANY/ANY)
				{

	        			std::cerr << "bdConnectManager::callbackConnectRequest() ";
					std::cerr << " DUPLICATE, recycle";
       		 			std::cerr << std::endl;

					recycle = true;
				}	
					break;
				case BITDHT_CONNECT_ERROR_OVERLOADED: // not more space. PROXY in RELAY mode.
				{
					if ((errsrc == BITDHT_CONNECT_ERROR_SOURCE_MID) &&
							(mode == BITDHT_CONNECT_MODE_RELAY))
					{
						recycle = true;

		        			std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "MID says OVERLOADED, recycle";
       			 			std::cerr << std::endl;

					}
					else
					{
						//ERROR. 
		        			std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "ERROR strange OVERLOADED";
       			 			std::cerr << std::endl;
					}
				}
					break;

				// NEXT PROXY. 
				case BITDHT_CONNECT_ERROR_NOADDRESS: //Proxy doesn't know peer MID/(RELAY|PROXY)
				{
					if (errsrc == BITDHT_CONNECT_ERROR_SOURCE_MID) 
					{
						// could recycle? probably still won't work.
		        			std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "MID says NOADDRESS";
       			 			std::cerr << std::endl;
					}
					else
					{
						//ERROR. 
		        			std::cerr << "bdConnectManager::callbackConnectRequest() ";
						std::cerr << "ERROR strange NOADDRESS";
       			 			std::cerr << std::endl;
					}
				}
					break;
			} // end of error code switch.

			// Now act on the decision.
			int newerrcode = errcode;
			if (fatal)
			{
				/* kill connection request, do callback */
				/* setup for next one */
				cr->mState = BITDHT_CONNREQUEST_DONE;
			}
			else
			{
				 if (recycle)
				{
					/* rotate around */
					cr->mGoodProxies.push_back(cr->mCurrentAttempt);
					cr->mRecycled++;
				}
				else
				{
					cr->mPeersTried.push_back(cr->mCurrentAttempt);
				}

				/* setup for next one */
				cr->mState = BITDHT_CONNREQUEST_PAUSED;
				cr->mPauseTS = now + BITDHT_CR_PAUSE_BASE_PERIOD + 
					(int) (bdRandom::random_f32() * BITDHT_CR_PAUSE_RND_PERIOD);
			}

			cr->mStateTS = now;
			cr->mErrCode = errcode;

			/* just pass on the callbackConnect() */
			callbackConnect(srcId, proxyId, destId, mode, point, cbtype, errcode);

			return; // CALLBACK FINISHED for FAILURE CODES.

		}
			break;	
	 	case BITDHT_CONNECT_CB_START:
		{

			cr->mState = BITDHT_CONNREQUEST_EXTCONNECT;
			cr->mStateTS = now;

			callbackConnect(srcId, proxyId, destId, mode, point, cbtype, errcode);

		}
			break;
	}
}



/************************************************************************************************************
************************************** END of Connection Requests *******************************************
************************************************************************************************************/

/************************************************************************************************************
****************************************** Outgoing Triggers ************************************************
************************************************************************************************************/

/*** Called by iterator.
 * initiates the connection startup
 *
 * srcConnAddr must contain Own ID + Connection Port (DHT or TOU depending on Mode). 
 *
 * For a DIRECT Connection: proxyId == destination Id, and mode == DIRECT.
 * 
 * For RELAY | PROXY Connection: 
 *
 * In all cases, destConnAddr doesn't need to contain a valid address.
 */

int bdConnectManager::startConnectionAttempt(bdId *proxyId, bdId *srcConnAddr, bdId *destConnAddr, int mode)
{
#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdConnectManager::startConnectionAttempt()";
	std::cerr << std::endl;
#endif

	if (!(mConfigAllowedModes & mode))
	{
		/* MODE not supported */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::startConnectionAttempt() ERROR Mode Not Supported";
		std::cerr << std::endl;
#endif
		return 0;
	}

	/* Check for existing Connection */
	bdConnection *conn = findExistingConnectionBySender(proxyId, srcConnAddr, destConnAddr);
	if (conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::startConnectAttempt() ERROR EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return 0;
	}

	{
		// DO A CALLBACK to TELL higher levels, we are starting a connection attempt.
		int point = BD_PROXY_CONNECTION_START_POINT;
		int cbtype = BITDHT_CONNECT_CB_REQUEST;
		int errcode = 0;
		callbackConnect(srcConnAddr, proxyId, destConnAddr, mode, point, cbtype, errcode);
	}

	/* INSTALL a NEW CONNECTION */
	// not offically playing by the rules, but it should work.
	conn = newConnectionBySender(proxyId, srcConnAddr, destConnAddr);

	if (mode == BITDHT_CONNECT_MODE_DIRECT)
	{
		/* proxy is the real peer address, destConnAddr has an invalid address */
        	conn->ConnectionSetupDirect(proxyId, srcConnAddr);
	}
	else
	{
		conn->ConnectionSetup(proxyId, srcConnAddr, destConnAddr, mode);
	}


	int msgtype =  BITDHT_MSG_TYPE_CONNECT_REQUEST;
	int status = BITDHT_CONNECT_ANSWER_OKAY;
	mPub->send_connect_msg(&(conn->mProxyId), msgtype, 
		&(conn->mSrcConnAddr), &(conn->mDestConnAddr), conn->mMode, status);

	return 1;
}

/* This will be called in response to a callback.
 * the callback could be with regard to:
 * a Direct EndPoint.
 * a Proxy Proxy, or an Proxy EndPoint.
 * a Relay Proxy, or an Relay EndPoint.
 *
 * If we are going to store the minimal amount in the bdNode about connections, 
 * then the parameters must contain all the information:
 *  
 * case 1:
 *
 */
 
void bdConnectManager::AuthConnectionOk(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc)
{

#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdConnectManager::AuthConnectionOk()";
	std::cerr << std::endl;
#endif

	/* Check for existing Connection */
	bdConnection *conn = findExistingConnection(&(srcId->id), &(proxyId->id), &(destId->id));
	if (!conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::AuthConnectionOk() ERROR NO EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return;
	}

	/* we need to continue the connection */
	if (mode == BITDHT_CONNECT_MODE_DIRECT)
	{	
		if (conn->mState == BITDHT_CONNECTION_WAITING_AUTH)
		{	
#ifdef DEBUG_NODE_CONNECTION
			std::cerr << "bdConnectManager::AuthConnectionOk() Direct Connection, in WAITING_AUTH state... Authorising Direct Connect";
			std::cerr << std::endl;
#endif
			/* This pushes it into the START/ACK cycle, 
			 * which handles messages elsewhere
			 */
			conn->AuthoriseDirectConnection(srcId, proxyId, destId, mode, loc);
		}
		else
		{
			/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
			std::cerr << "bdConnectManager::AuthConnectionOk() ERROR Direct Connection, !WAITING_AUTH state... Ignoring";
			std::cerr << std::endl;
#endif

		}
		return;
	}

	if (loc == BD_PROXY_CONNECTION_END_POINT)
	{
		if (conn->mState == BITDHT_CONNECTION_WAITING_AUTH)
		{
#ifdef DEBUG_NODE_CONNECTION
			std::cerr << "bdConnectManager::AuthConnectionOk() Proxy End Connection, in WAITING_AUTH state... Authorising";
			std::cerr << std::endl;
#endif
			/*** XXX MUST RECEIVE THE ADDRESS FROM DEST for connection */
			conn->AuthoriseEndConnection(srcId, proxyId, destId, mode, loc);
			
			/* we respond to the proxy which will finalise connection */
			int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
			int status = BITDHT_CONNECT_ANSWER_OKAY;
			mPub->send_connect_msg(&(conn->mProxyId), msgtype, 
				&(conn->mSrcConnAddr), &(conn->mDestConnAddr), conn->mMode, status);
			
			return;
		}
		else
		{

			/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
			std::cerr << "bdConnectManager::AuthConnectionOk() ERROR Proxy End Connection, !WAITING_AUTH state... Ignoring";
			std::cerr << std::endl;
#endif
		}
	}

	if (conn->mState == BITDHT_CONNECTION_WAITING_AUTH)
	{
		/* otherwise we are the proxy (for either), pass on the request */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::AuthConnectionOk() Proxy Mid Connection, in WAITING_AUTH state... Authorising";
		std::cerr << std::endl;
#endif
	
		/* SEARCH for IP:Port of destination is done before AUTH  */
	
		conn->AuthoriseProxyConnection(srcId, proxyId, destId, mode, loc);
	
		int msgtype = BITDHT_MSG_TYPE_CONNECT_REQUEST;
		int status = BITDHT_CONNECT_ANSWER_OKAY;
		mPub->send_connect_msg(&(conn->mDestId), msgtype, 
			&(conn->mSrcConnAddr), &(conn->mDestConnAddr), conn->mMode, status);
	}
	else
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::AuthConnectionOk() ERROR Proxy Mid Connection, !WAITING_AUTH state... Ignoring";
		std::cerr << std::endl;
#endif
	}

	return;	
}



 
void bdConnectManager::AuthConnectionNo(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc, int errCode)
{
	
#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdConnectManager::AuthConnectionNo()";
	std::cerr << std::endl;
#endif
	
	/* Check for existing Connection */
	bdConnection *conn = findExistingConnection(&(srcId->id), &(proxyId->id), &(destId->id));
	if (!conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::AuthConnectionNo() ERROR NO EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return;
	}
	
	/* we need indicate failure of the connection */
	int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
	uint32_t status = createConnectionErrorCode(errCode, BITDHT_CONNECT_ERROR_AUTH_DENIED, conn->mPoint);

	if (mode == BITDHT_CONNECT_MODE_DIRECT)
	{
		/* we respond to the proxy which will finalise connection */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::AuthConnectionNo() Direct End Connection Cleaning up";
		std::cerr << std::endl;
#endif
		mPub->send_connect_msg(&(conn->mSrcId), msgtype, 
			  &(conn->mSrcConnAddr), &(conn->mDestConnAddr), mode, status);
		
		cleanConnection(&(srcId->id), &(proxyId->id), &(destId->id));
		return;
	}

	if (loc == BD_PROXY_CONNECTION_END_POINT)
	{
		/* we respond to the proxy which will finalise connection */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::AuthConnectionNo() Proxy End Connection Cleaning up";
		std::cerr << std::endl;
#endif
		mPub->send_connect_msg(&(conn->mProxyId), msgtype, 
			  &(conn->mSrcConnAddr), &(conn->mDestConnAddr), mode, status);
		
		cleanConnection(&(srcId->id), &(proxyId->id), &(destId->id));

		return;
	}

	/* otherwise we are the proxy (for either), reply FAIL */
#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdConnectManager::AuthConnectionNo() Proxy Mid Connection Cleaning up";
	std::cerr << std::endl;
#endif
	mPub->send_connect_msg(&(conn->mSrcId), msgtype, 
		&(conn->mSrcConnAddr), &(conn->mDestConnAddr), mode, status);

	cleanConnection(&(srcId->id), &(proxyId->id), &(destId->id));

	return;	
}


	


void bdConnectManager::iterateConnections()
{
	std::map<bdProxyTuple, bdConnection>::iterator it;
	std::list<bdProxyTuple> eraseList;
	time_t now = time(NULL);
	
	for(it = mConnections.begin(); it != mConnections.end(); it++)
	{
		if (now - it->second.mLastEvent > BD_CONNECTION_MAX_TIMEOUT)
		{
			/* cleanup event */
#ifdef DEBUG_NODE_CONNECTION
			std::cerr << "bdConnectManager::iterateConnections() Connection Timed Out: " << (it->first);
			std::cerr << std::endl;
#endif
			eraseList.push_back(it->first);
			continue;
		}

		if ((it->second.mState == BITDHT_CONNECTION_WAITING_ACK) &&
			(now - it->second.mLastStart > BD_CONNECTION_START_RETRY_PERIOD))
		{
			if (it->second.mRetryCount > BD_CONNECTION_START_MAX_RETRY)
			{
#ifdef DEBUG_NODE_CONNECTION
				std::cerr << "bdConnectManager::iterateConnections() Start/ACK cycle, Too many iterations: " << it->first;
				std::cerr << std::endl;
#endif
				/* connection failed! cleanup */
				if ((it->second.mMode != BITDHT_CONNECT_MODE_PROXY) || (!mConfigAutoProxy))
				{
					uint32_t errCode = createConnectionErrorCode(0, 
						BITDHT_CONNECT_ERROR_TIMEOUT,it->second.mPoint);
					callbackConnect(&(it->second.mSrcId),&(it->second.mProxyId),
						&(it->second.mDestId), it->second.mMode, it->second.mPoint, 
						BITDHT_CONNECT_CB_FAILED, errCode);
				}

				/* add to erase list */
				eraseList.push_back(it->first);
			}
			else
			{
#ifdef DEBUG_NODE_CONNECTION
				std::cerr << "bdConnectManager::iterateConnections() Start/ACK cycle, Retransmitting START: " << it->first;
				std::cerr << std::endl;
#endif
				it->second.mLastStart = now;
				it->second.mRetryCount++;
				if (!it->second.mSrcAck)
				{
					int msgtype = BITDHT_MSG_TYPE_CONNECT_START;
					mPub->send_connect_msg(&(it->second.mSrcId), msgtype, 
						&(it->second.mSrcConnAddr), &(it->second.mDestConnAddr), 
						it->second.mMode, it->second.mBandwidth);
				}
				if (!it->second.mDestAck)
				{
					int msgtype = BITDHT_MSG_TYPE_CONNECT_START;
					mPub->send_connect_msg(&(it->second.mDestId), msgtype, 
						  &(it->second.mSrcConnAddr), &(it->second.mDestConnAddr), 
						  it->second.mMode, it->second.mBandwidth);
				}
			}
		}
	}

	/* clean up */
	while(eraseList.size() > 0)
	{
		bdProxyTuple tuple = eraseList.front();
		eraseList.pop_front();

		std::map<bdProxyTuple, bdConnection>::iterator eit = mConnections.find(tuple);
		mConnections.erase(eit);
	}
}




/************************************************************************************************************
****************************************** Callback Functions    ********************************************
************************************************************************************************************/


void bdConnectManager::callbackConnect(bdId *srcId, bdId *proxyId, bdId *destId, 
					int mode, int point, int cbtype, int errcode)
{
	/* This is overloaded at a higher level */
	mPub->callbackConnect(srcId, proxyId, destId, mode, point, cbtype, errcode);
}


/************************************************************************************************************
************************************** ProxyTuple + Connection State ****************************************
************************************************************************************************************/

int operator<(const bdProxyTuple &a, const bdProxyTuple &b)
{
	if (a.srcId < b.srcId)
	{
		return 1;
	}
	
	if (a.srcId == b.srcId)
	{
 		if (a.proxyId < b.proxyId)
		{
			return 1;
		}
 		else if (a.proxyId == b.proxyId)
		{
			if (a.destId < b.destId)
			{
				return 1;
			}
		}
	}
	return 0;
}

int operator==(const bdProxyTuple &a, const bdProxyTuple &b)
{
	if ((a.srcId == b.srcId) && (a.proxyId == b.proxyId) && (a.destId == b.destId))
	{
		return 1;
	}
	return 0;
}

std::ostream &operator<<(std::ostream &out, const bdProxyTuple &t)
{
	out << "[---";
	bdStdPrintNodeId(out, &(t.srcId));
	out << "---";
	bdStdPrintNodeId(out, &(t.proxyId));
	out << "---";
	bdStdPrintNodeId(out, &(t.destId));
	out << "---]";

	return out;
}

bdConnection::bdConnection()
{
	/* DUMMY INITIALISATION FOR ALL DATA - DON"T THINK IT MATTERS
	 * But keeps Valgrind happy
	 */

	/* Connection State, and TimeStamp of Update */
	int mState = 0;
	time_t mLastEvent = 0;
	//
	///* Addresses of Start/Proxy/End Nodes */
	//bdId mSrcId;
	//bdId mDestId;
	//bdId mProxyId;
	//
	///* Where we are in the connection,
	//* and what connection mode.
	//*/
	int mPoint = 0;
	int mMode = 0;
	//
	///* must have ip:ports of connection ends (if proxied) */
	//bdId mSrcConnAddr;
	//bdId mDestConnAddr;
	//
	int mBandwidth = 0;
	//
	///* START/ACK Finishing ****/
	time_t mLastStart = 0;   /* timer for retries */
	int mRetryCount = 0;     /* retry counter */
	//
	bool mSrcAck = false;
	bool mDestAck = false;
	//
	//// Completion TS.
	time_t mCompletedTS = 0;
}

	/* heavy check, used to check for alternative connections, coming from other direction
	 * Caller must switch src/dest to use it properly (otherwise it'll find your connection!)
	 */
bdConnection *bdConnectManager::findSimilarConnection(bdNodeId *srcId, bdNodeId *destId)
{
	std::map<bdProxyTuple, bdConnection>::iterator it;
	for(it = mConnections.begin(); it != mConnections.end(); it++)
	{
		if ((it->first.srcId == *srcId) && (it->first.destId == *destId))
		{
			/* found similar connection */
			return &(it->second);
		}
	}
	return NULL;
}

bdConnection *bdConnectManager::findExistingConnection(bdNodeId *srcId, bdNodeId *proxyId, bdNodeId *destId)
{
	bdProxyTuple tuple(srcId, proxyId, destId);

	std::cerr << "bdConnectManager::findExistingConnection() Looking For: " << tuple << std::endl;

	std::map<bdProxyTuple, bdConnection>::iterator it = mConnections.find(tuple);
	if (it == mConnections.end())
	{
		std::cerr << "bdConnectManager::findExistingConnection() Failed to Find: " << tuple << std::endl;
		return NULL;
	}

	std::cerr << "bdConnectManager::findExistingConnection() Found: " << tuple << std::endl;
	return &(it->second);
}

bdConnection *bdConnectManager::newConnection(bdNodeId *srcId, bdNodeId *proxyId, bdNodeId *destId)
{
	bdProxyTuple tuple(srcId, proxyId, destId);
	bdConnection conn;

	std::cerr << "bdConnectManager::newConnection() Installing: " << tuple << std::endl;

	mConnections[tuple] = conn;
	std::map<bdProxyTuple, bdConnection>::iterator it = mConnections.find(tuple);
	if (it == mConnections.end())
	{
		std::cerr << "bdConnectManager::newConnection() ERROR Installing: " << tuple << std::endl;
		return NULL;
	}
	return &(it->second);
}

int bdConnectManager::cleanConnection(bdNodeId *srcId, bdNodeId *proxyId, bdNodeId *destId)
{
	bdProxyTuple tuple(srcId, proxyId, destId);
	bdConnection conn;

	std::cerr << "bdConnectManager::cleanConnection() Removing: " << tuple << std::endl;

	std::map<bdProxyTuple, bdConnection>::iterator it = mConnections.find(tuple);
	if (it == mConnections.end())
	{
		std::cerr << "bdConnectManager::cleanConnection() ERROR Removing: " << tuple << std::endl;
		return 0;
	}
	mConnections.erase(it);

	return 1;
}


int bdConnectManager::determinePosition(bdNodeId *sender, bdNodeId *src, bdNodeId *dest)
{
	int pos =  BD_PROXY_CONNECTION_UNKNOWN_POINT;
	if (mOwnId == *src)
	{
		pos = BD_PROXY_CONNECTION_START_POINT;
	}
	else if (mOwnId == *dest)
	{
		pos = BD_PROXY_CONNECTION_END_POINT;
	}
	else
	{
		pos = BD_PROXY_CONNECTION_MID_POINT;
	}
	return pos;	
}

int bdConnectManager::determineProxyId(bdNodeId *sender, bdNodeId *src, bdNodeId *dest, bdNodeId *proxyId)
{
	int pos = determinePosition(sender, src, dest);
	switch(pos)
	{
		case BD_PROXY_CONNECTION_START_POINT:
		case BD_PROXY_CONNECTION_END_POINT:
			*proxyId = *sender;
			return 1;
			break;
		default:
		case BD_PROXY_CONNECTION_MID_POINT:
			*proxyId = mOwnId;
			return 1;
			break;
	}
	return 0;
}



bdConnection *bdConnectManager::findExistingConnectionBySender(bdId *sender, bdId *src, bdId *dest)
{
	bdNodeId proxyId;
	bdNodeId *senderId = &(sender->id);
	bdNodeId *srcId = &(src->id);
	bdNodeId *destId = &(dest->id);
	determineProxyId(senderId, srcId, destId, &proxyId);

	return findExistingConnection(srcId, &proxyId, destId);
}

bdConnection *bdConnectManager::newConnectionBySender(bdId *sender, bdId *src, bdId *dest)
{
	bdNodeId proxyId;
	bdNodeId *senderId = &(sender->id);
	bdNodeId *srcId = &(src->id);
	bdNodeId *destId = &(dest->id);
	determineProxyId(senderId, srcId, destId, &proxyId);

	return newConnection(srcId, &proxyId, destId);
}


int bdConnectManager::cleanConnectionBySender(bdId *sender, bdId *src, bdId *dest)
{
	bdNodeId proxyId;
	bdNodeId *senderId = &(sender->id);
	bdNodeId *srcId = &(src->id);
	bdNodeId *destId = &(dest->id);
	determineProxyId(senderId, srcId, destId, &proxyId);

	return cleanConnection(srcId, &proxyId, destId);
}


/************************************************************************************************************
****************************************** Received Connect Msgs ********************************************
************************************************************************************************************/


/* This function is triggered by a CONNECT_REQUEST message.
 * it will occur on both the Proxy/Dest in the case of a Proxy (PROXY | RELAY) and on the Dest (DIRECT) nodes.
 *
 * In all cases, we store the request and ask for authentication.
 *
 */



int bdConnectManager::recvedConnectionRequest(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode)
{
#ifdef DEBUG_NODE_CONNECTION
	std::cerr << "bdConnectManager::recvedConnectionRequest()";
	std::cerr << std::endl;
#endif

	if (!(mConfigAllowedModes & mode))
	{
		/* MODE not supported */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::recvedConnectionRequest() WARNING Mode Not Supported";
		std::cerr << std::endl;
#endif
		/* reply existing connection */
		int pos = determinePosition(&(id->id), &(srcConnAddr->id), &(destConnAddr->id));
		uint32_t status = createConnectionErrorCode(0, BITDHT_CONNECT_ERROR_UNSUPPORTED, pos);

		int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
		mPub->send_connect_msg(id, msgtype, srcConnAddr, destConnAddr, mode, status);

		return 0;
	}

	/* Check for existing Connection */
	bdConnection *conn = findExistingConnectionBySender(id, srcConnAddr, destConnAddr);
	if (conn)
	{
		/* Likely ERROR: Warning */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::recvedConnectionRequest() WARNING Existing Connection: ";
		std::cerr << std::endl;
		std::cerr << std::endl;
#endif
		/* reply existing connection */
		uint32_t status = createConnectionErrorCode(0, BITDHT_CONNECT_ERROR_DUPLICATE, conn->mPoint);

		int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
		mPub->send_connect_msg(id, msgtype, srcConnAddr, destConnAddr, mode, status);

		return 0;
	}


	/* Switch the order of peers around to test for "opposite connections" */
	if (NULL != findSimilarConnection(&(destConnAddr->id), &(srcConnAddr->id)))
	{
		std::cerr << "bdConnectManager::recvedConnectionRequest() WARNING Found Similar Connection. Replying NO";
		std::cerr << std::endl;

		/* reply existing connection */
		int pos = determinePosition(&(id->id), &(srcConnAddr->id), &(destConnAddr->id));
		uint32_t status = createConnectionErrorCode(0, BITDHT_CONNECT_ERROR_DUPLICATE, pos);

		int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
		mPub->send_connect_msg(id, msgtype, srcConnAddr, destConnAddr, mode, status);
		return 0;
	}

	/* INSTALL a NEW CONNECTION */
	conn = bdConnectManager::newConnectionBySender(id, srcConnAddr, destConnAddr);

	int point = 0;
	if (mode == BITDHT_CONNECT_MODE_DIRECT)
	{
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::recvedConnectionRequest() Installing DIRECT CONNECTION";
		std::cerr << std::endl;
#endif

		/* we are actually the end node, store stuff, get auth and on with it! */
		point = BD_PROXY_CONNECTION_END_POINT;

		conn->ConnectionRequestDirect(id, srcConnAddr, destConnAddr);

#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::recvedConnectionRequest() Connection State:";
		std::cerr << std::endl;
		std::cerr << *conn;
		std::cerr << std::endl;
#endif
		callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
					conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_AUTH,
					BITDHT_CONNECT_ERROR_NONE);
	}
	else
	{
		/* check if we are proxy, or end point */
		bool areProxy = (srcConnAddr->id == id->id);
		if (areProxy)
		{
			std::cerr << "bdConnectManager::recvedConnectionRequest() We are MID Point for Proxy / Relay Connection.";
			std::cerr << std::endl;

			point = BD_PROXY_CONNECTION_MID_POINT;

			/* SEARCH for IP:Port of destination before AUTH  */
			int numNodes = 10;
			std::list<bdId> matchingIds;

			std::cerr << "bdConnectManager::recvedConnectionRequest() WARNING searching for \"VERSION\" flag... TO FIX LATER";
			std::cerr << std::endl;

			uint32_t with_flag = BITDHT_PEER_STATUS_DHT_ENGINE_VERSION;
			//BITDHT_PEER_STATUS_DHT_APPL | BITDHT_PEER_STATUS_DHT_APPL_VERSION);

			bool proxyOk = false;
			bdId destId;

			if (mNodeSpace->find_node(&(destConnAddr->id), numNodes, matchingIds, with_flag))
			{
				std::cerr << "bdConnectManager::recvedConnectionRequest() Found Suitable Destination Addr";
				std::cerr << std::endl;

				if (matchingIds.size() > 1)
				{
					/* WARNING multiple matches */
					std::cerr << "bdConnectManager::recvedConnectionRequest() WARNING Found Multiple Matching Destination Addr";
					std::cerr << std::endl;
				}

				proxyOk = true;
				destId = matchingIds.front();
			}

			if (proxyOk)
			{
				std::cerr << "bdConnectManager::recvedConnectionRequest() Proxy Addr Ok: ";
				bdStdPrintId(std::cerr, destConnAddr);
				std::cerr << "asking for AUTH to continue";
				std::cerr << std::endl;

				conn->ConnectionRequestProxy(id, srcConnAddr, &mOwnId, &destId, mode);

				/* ALLOW AUTO AUTH for MID Proxy Connections. */
				if (mConfigAutoProxy)
				{
				  	AuthConnectionOk(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
							conn->mMode, conn->mPoint);
				}
				else
				{

				  	callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
							conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_AUTH,
							BITDHT_CONNECT_ERROR_NONE);
				}
			}
			else
			{
				/* clean up connection... its not going to work */
				std::cerr << "bdConnectManager::recvedConnectionRequest() WARNING No Proxy Addr, Shutting Connect Attempt";
				std::cerr << std::endl;


				/* send FAIL message to SRC */
				int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
				uint32_t status = createConnectionErrorCode(0, BITDHT_CONNECT_ERROR_NOADDRESS, point);
				mPub->send_connect_msg(id, msgtype, srcConnAddr, destConnAddr, mode, status);

				/* remove connection */
				bdConnectManager::cleanConnectionBySender(id, srcConnAddr, destConnAddr);

			}
		}
		else
		{
			std::cerr << "bdConnectManager::recvedConnectionRequest() END Proxy/Relay Connection, asking for AUTH to continue";
			std::cerr << std::endl;

			point = BD_PROXY_CONNECTION_END_POINT;

			conn->ConnectionRequestEnd(id, srcConnAddr, destConnAddr, mode);

			callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
						conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_AUTH,
						BITDHT_CONNECT_ERROR_NONE);
		}
	}
	return 1;
}


/* This function is triggered by a CONNECT_REPLY message.
 * it will occur on either the Proxy or Source. And indicates YES / NO to the connection, 
 * as well as supplying address info to the proxy.
 *
 */

int bdConnectManager::recvedConnectionReply(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int status)
{
	/* retrieve existing connection data */
	bdConnection *conn = findExistingConnectionBySender(id, srcConnAddr, destConnAddr);
	if (!conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::recvedConnectionReply() ERROR NO EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return 0;
	}

	switch(conn->mPoint)
	{
		case BD_PROXY_CONNECTION_START_POINT:
		case BD_PROXY_CONNECTION_END_POINT:		/* NEVER EXPECT THIS */
		case BD_PROXY_CONNECTION_UNKNOWN_POINT:		/* NEVER EXPECT THIS */
		default:					/* NEVER EXPECT THIS */
		{


			/* Only situation we expect this, is if the connection is not allowed.
			 * DEST has sent back an ERROR Message
			 */
			uint32_t errCode = BITDHT_CONNECT_ERROR_GENERIC;
			if ((status != BITDHT_CONNECT_ANSWER_OKAY) && (conn->mPoint == BD_PROXY_CONNECTION_START_POINT))
			{
				/* connection is killed */
				std::cerr << "bdConnectManager::recvedConnectionReply() WARNING Connection Rejected. Error: ";
				std::cerr << decodeConnectionError(status);
				std::cerr << ", Killing It: "; 
				std::cerr << std::endl;
				std::cerr << *conn;
				std::cerr << std::endl;
				errCode = status; // Pass on the Error Message.

			}
			else
			{
				/* ERROR in protocol */
				std::cerr << "bdConnectManager::recvedConnectionReply() ERROR Unexpected Message, Killing It: ";
				std::cerr << std::endl;
				std::cerr << *conn;
				std::cerr << std::endl;
				errCode = createConnectionErrorCode(0, BITDHT_CONNECT_ERROR_PROTOCOL, conn->mPoint );
			}

			/* do Callback for Failed Connection */
			if (conn->mPoint == BD_PROXY_CONNECTION_START_POINT)
			{
				/* As we started the connection, callback internally first! */
				callbackConnectRequest(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
						conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_FAILED, errCode);
			}
			else
			{
				callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
						conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_FAILED, errCode);
			}

			/* Kill Connection always */
			cleanConnectionBySender(id, srcConnAddr, destConnAddr);

			return 0;
		}
			break;

		case BD_PROXY_CONNECTION_MID_POINT:
		{
			 /*    We are proxy. and OK / NOK for connection proceed.
			  */

			if ((status == BITDHT_CONNECT_ANSWER_OKAY) && (conn->mState == BITDHT_CONNECTION_WAITING_REPLY))
			{
				/* OK, continue connection! */
				std::cerr << "bdConnectManager::recvedConnectionReply() @MIDPOINT. Reply + State OK, continuing connection";
				std::cerr << std::endl;

				/* Upgrade Connection to Finishing Mode */
				conn->upgradeProxyConnectionToFinish(id, srcConnAddr, destConnAddr, mode, status);

				/* do Callback for Pending Connection */
				/* DONT CALLBACK in AutoProxy Mode: (PROXY & mConfigAutoProxy) */
				if ((conn->mMode != BITDHT_CONNECT_MODE_PROXY) || (!mConfigAutoProxy))
				{
					callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
							conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_PENDING,
							BITDHT_CONNECT_ERROR_NONE);
				}

				return 1;
			}
			else
			{


				std::cerr << "bdConnectManager::recvedConnectionReply() WARNING @MIDPOINT recved Error: ";
				std::cerr << decodeConnectionError(status);
				std::cerr << " Killing It: ";
				std::cerr << std::endl;
				std::cerr << *conn;
				std::cerr << std::endl;

				uint32_t errCode = status;
				if (errCode == BITDHT_CONNECT_ERROR_NONE)
				{
					errCode = createConnectionErrorCode(0, 
						BITDHT_CONNECT_ERROR_PROTOCOL, conn->mPoint );
				}

				/* do Callback for Failed Connection */
				/* DONT CALLBACK in AutoProxy Mode: (PROXY & mConfigAutoProxy) */
				if ((conn->mMode != BITDHT_CONNECT_MODE_PROXY) || (!mConfigAutoProxy))
				{
					callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
						conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_FAILED, errCode);
				}


				/* send on message to SRC */
				int msgtype = BITDHT_MSG_TYPE_CONNECT_REPLY;
				mPub->send_connect_msg(&(conn->mSrcId), msgtype, &(conn->mSrcConnAddr), &(conn->mDestConnAddr), mode, errCode);

				/* connection is killed */
				cleanConnectionBySender(id, srcConnAddr, destConnAddr);
			}
			return 0;
		}
			break;
	}
	return 0;
}


/* This function is triggered by a CONNECT_START message.
 * it will occur on both the Src/Dest in the case of a Proxy (PROXY | RELAY) and on the Src (DIRECT) nodes.
 *
 * parameters are checked against pending connections.
 *  Acks are set, and connections completed if possible (including callback!).
 */

int bdConnectManager::recvedConnectionStart(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int bandwidth)
{
	std::cerr << "bdConnectManager::recvedConnectionStart()";
	std::cerr << std::endl;

	/* retrieve existing connection data */
	bdConnection *conn = findExistingConnectionBySender(id, srcConnAddr, destConnAddr);
	if (!conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::recvedConnectionStart() ERROR NO EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return 0;
	}


	if (conn->mPoint == BD_PROXY_CONNECTION_MID_POINT)
	{
		std::cerr << "bdConnectManager::recvedConnectionStart() ERROR We Are Connection MID Point";
		std::cerr << std::endl;
		/* ERROR */
	}

	/* check state */
	if ((conn->mState != BITDHT_CONNECTION_WAITING_START) && (conn->mState != BITDHT_CONNECTION_COMPLETED))
	{
		/* ERROR */
		std::cerr << "bdConnectManager::recvedConnectionStart() ERROR State != WAITING_START && != COMPLETED";
		std::cerr << std::endl;

		return 0;
	}

	/* ALL Okay, Send ACK */
	std::cerr << "bdConnectManager::recvedConnectionStart() Passed basic tests, Okay to send ACK";
	std::cerr << std::endl;

	int msgtype = BITDHT_MSG_TYPE_CONNECT_ACK;
	int status = BITDHT_CONNECT_ANSWER_OKAY;
	mPub->send_connect_msg(id, msgtype, &(conn->mSrcId), &(conn->mDestId), mode, status);

	/* do complete Callback */

	/* flag as completed */
	if (conn->mState != BITDHT_CONNECTION_COMPLETED)
	{
		std::cerr << "bdConnectManager::recvedConnectionStart() Switching State to COMPLETED, doing callback";
		std::cerr << std::endl;

		conn->CompleteConnection(id, srcConnAddr, destConnAddr);

		std::cerr << "bdConnectManager::recvedConnectionStart() Connection State: ";
		std::cerr << *conn;
		std::cerr << std::endl;


		if (conn->mPoint == BD_PROXY_CONNECTION_START_POINT)
		{
			/* internal callback first */
			callbackConnectRequest(&(conn->mSrcConnAddr),&(conn->mProxyId),&(conn->mDestConnAddr),
							conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_START,
							BITDHT_CONNECT_ERROR_NONE);

		}
		else
		{
			/* internal callback first */
			callbackConnectRequest(&(conn->mSrcConnAddr),&(conn->mProxyId),&(conn->mDestConnAddr),
							conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_START,
							BITDHT_CONNECT_ERROR_NONE);
		}

	}
	else
	{
		std::cerr << "bdConnectManager::recvedConnectionStart() Just sent duplicate ACK";
		std::cerr << std::endl;
	}
	/* don't delete, if ACK is lost, we want to be able to re-respond */

	return 1;
}


/* This function is triggered by a CONNECT_ACK message.
 * it will occur on both the Proxy (PROXY | RELAY) and on the Dest (DIRECT) nodes.
 *
 * parameters are checked against pending connections.
 *  Acks are set, and connections completed if possible (including callback!).
 */

int bdConnectManager::recvedConnectionAck(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode)
{
	/* retrieve existing connection data */
	bdConnection *conn = findExistingConnectionBySender(id, srcConnAddr, destConnAddr);
	if (!conn)
	{
		/* ERROR */
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdConnectManager::recvedConnectionAck() ERROR NO EXISTING CONNECTION";
		std::cerr << std::endl;
#endif
		return 0;
	}

	if (conn->mPoint == BD_PROXY_CONNECTION_START_POINT)
	{
		/* ERROR */
		std::cerr << "bdConnectManager::recvedConnectionAck() ERROR ACK received at START POINT";
		std::cerr << std::endl;

		return 0;
	}

	/* check state */
	if (conn->mState != BITDHT_CONNECTION_WAITING_ACK)
	{
		/* ERROR */
		std::cerr << "bdConnectManager::recvedConnectionAck() conn->mState != WAITING_ACK, actual State: " << conn->mState;
		std::cerr << std::endl;

		return 0;
	}

	if (id->id == srcConnAddr->id)
	{
		std::cerr << "bdConnectManager::recvedConnectionAck() from Src, marking So";
		std::cerr << std::endl;

		/* recved Ack from source */
		conn->mSrcAck = true;
	}
	else if (id->id == destConnAddr->id)
	{
		std::cerr << "bdConnectManager::recvedConnectionAck() from Dest, marking So";
		std::cerr << std::endl;
		/* recved Ack from dest */
		conn->mDestAck = true;
	}

	if (conn->mSrcAck && conn->mDestAck)
	{
		std::cerr << "bdConnectManager::recvedConnectionAck() ACKs from Both Src & Dest, Connection Complete: callback & cleanup";
		std::cerr << std::endl;

		/* connection complete! cleanup */
		if (conn->mMode == BITDHT_CONNECT_MODE_DIRECT)
		{
			int mode = conn->mMode | BITDHT_CONNECT_ANSWER_OKAY;
			/* callback to connect to Src address! */
			// Slightly different callback, use ConnAddr for start message!
			// Also callback to ConnectionRequest first.
			// ACTUALLY we are END, so shouldn't (AT This Point do this).
			callbackConnect(&(conn->mSrcConnAddr),&(conn->mProxyId),&(conn->mDestId),
						conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_START,
						BITDHT_CONNECT_ERROR_NONE);

		}
		else
		{
			/* DONT CALLBACK in AutoProxy Mode: (PROXY & mConfigAutoProxy) */
			if ((conn->mMode != BITDHT_CONNECT_MODE_PROXY) || (!mConfigAutoProxy))
			{
				callbackConnect(&(conn->mSrcId),&(conn->mProxyId),&(conn->mDestId),
						conn->mMode, conn->mPoint, BITDHT_CONNECT_CB_PROXY, 
						BITDHT_CONNECT_ERROR_NONE);
			}


		}

		/* Finished Connection! */
		cleanConnectionBySender(id, srcConnAddr, destConnAddr);
	}
	return 1;
}



/************************************************************************************************************
********************************* Connection / ConnectionRequest Functions **********************************
************************************************************************************************************/

// Initialise a new Connection (request by User)

// Any Connection initialised at Source (START_POINT), prior to Auth.
int bdConnection::ConnectionSetup(bdId *proxyId, bdId *srcConnAddr, bdId *destId, int mode)
{
	mState = BITDHT_CONNECTION_WAITING_START; /* or REPLY, no AUTH required */
	mLastEvent = time(NULL);
	mSrcId = *srcConnAddr;    /* self, IP unknown */
	mDestId = *destId;  /* dest, IP unknown */
	mProxyId =  *proxyId;  /* full proxy/dest address */

	mPoint = BD_PROXY_CONNECTION_START_POINT;
	mMode = mode;

	mSrcConnAddr = *srcConnAddr; /* self, full ID/IP */
	mDestConnAddr = *destId; /* IP unknown */

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mSrcId.addr)); 
	bdsockaddr_clear(&(mDestId.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}

int bdConnection::ConnectionSetupDirect(bdId *destId, bdId *srcConnAddr)
{
	mState = BITDHT_CONNECTION_WAITING_START; /* or REPLY, no AUTH required */
	mLastEvent = time(NULL);
	mSrcId = *srcConnAddr;    /* self, IP unknown */
	mDestId = *destId;  /* full proxy/dest address */
	mProxyId =  *destId;  /* full proxy/dest address */

	mPoint = BD_PROXY_CONNECTION_START_POINT;
	mMode = BITDHT_CONNECT_MODE_DIRECT;

	mSrcConnAddr = *srcConnAddr; /* self, full ID/IP */
	mDestConnAddr = *destId; /* IP unknown */

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mSrcId.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}



// Initialise a new Connection. (receiving a Connection Request)
// Direct Connection initialised at Destination (END_POINT), prior to Auth.
int bdConnection::ConnectionRequestDirect(bdId *id, bdId *srcConnAddr, bdId *destId)
{
	mState = BITDHT_CONNECTION_WAITING_AUTH;
	mLastEvent = time(NULL);
	mSrcId = *id;       /* peer ID/IP known */
	mDestId = *destId;  /* self, IP unknown */
	mProxyId = *id;  /* src ID/IP known */

	mPoint = BD_PROXY_CONNECTION_END_POINT;
	mMode = BITDHT_CONNECT_MODE_DIRECT;

	mSrcConnAddr = *srcConnAddr; /* connect address ID/IP known */
	mDestConnAddr = *destId; /* self IP unknown */

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mDestId.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}


// Proxy Connection initialised at Proxy (MID_POINT), prior to Auth.
int bdConnection::ConnectionRequestProxy(bdId *id, bdId *srcConnAddr, bdNodeId *ownId, bdId *destId, int mode)
{
	mState = BITDHT_CONNECTION_WAITING_AUTH;
	mLastEvent = time(NULL);
	mSrcId = *id;		/* ID/IP Known */
	mDestId = *destId;  /* destination, ID/IP known  */
	mProxyId.id =  *ownId;  /* own id, must be set for callback, IP Unknown */

	mPoint = BD_PROXY_CONNECTION_MID_POINT;
	mMode = mode;

	mSrcConnAddr = *srcConnAddr;
	mDestConnAddr = *destId; /* other peer, IP unknown */

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mProxyId.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}


// Proxy Connection initialised at Destination (END_POINT), prior to Auth.
int bdConnection::ConnectionRequestEnd(bdId *id, bdId *srcId, bdId *destId, int mode)
{
	mState = BITDHT_CONNECTION_WAITING_AUTH;
	mLastEvent = time(NULL);
	mSrcId = *srcId;   /* src IP unknown */
	mDestId = *destId;  /* self, IP unknown */
	mProxyId = *id;  /* src of message, full ID/IP of proxy */

	mPoint = BD_PROXY_CONNECTION_END_POINT;
	mMode = mode;

	mSrcConnAddr = *srcId;   /* ID, not IP */
	mDestConnAddr = *destId; /* ID, not IP */

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mSrcId.addr)); 
	bdsockaddr_clear(&(mDestId.addr)); 
	bdsockaddr_clear(&(mSrcConnAddr.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}

// Received AUTH, step up to next stage.
// Search for dest ID/IP is done before AUTH. so actually nothing to do here, except set the state
int bdConnection::AuthoriseProxyConnection(bdId *srcId, bdId *proxyId, bdId *destId, int mode, int loc)
{
	mState = BITDHT_CONNECTION_WAITING_REPLY;
	mLastEvent = time(NULL);

	//mSrcId, (peer) (ID/IP known)
	//mDestId (other peer) (ID/IP known)
	//mProxyId (self) (IP unknown)

	// mPoint, mMode should be okay.

	// mSrcConnAddr (ID/IP known)
	// mDestConnAddr is still pending.

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mProxyId.addr)); 
	bdsockaddr_clear(&(mDestConnAddr.addr)); 

	/* don't bother with START/ACK parameters */
	
	return 1;
}


/* we are end of a Proxy Connection */
int bdConnection::AuthoriseEndConnection(bdId *srcId, bdId *proxyId, bdId *destConnAddr, int mode, int loc)
{
	mState = BITDHT_CONNECTION_WAITING_START;
	mLastEvent = time(NULL);

	//mSrcId, (peer) should be okay. (IP unknown)
	//mDestId (self) doesn't matter. (IP unknown)
	//mProxyId (peer) should be okay. (ID/IP known)

	// mPoint, mMode should be okay.

	// mSrcConnAddr should be okay. (IP unknown)
	// Install the correct destConnAddr. (just received)
	mDestConnAddr = *destConnAddr; 

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mSrcId.addr)); 
	bdsockaddr_clear(&(mDestId.addr)); 
	bdsockaddr_clear(&(mSrcConnAddr.addr)); 


	// Initialise the START/ACK Parameters.
	mRetryCount = 0;
	mLastStart = 0;
	mSrcAck = false;
	mDestAck = true; // Automatic ACK, as it is from us.
	mCompletedTS = 0;
	
	return 1;
}




// Auth of the Direct Connection, means we move straight to WAITING_ACK mode.
int bdConnection::AuthoriseDirectConnection(bdId *srcId, bdId *proxyId, bdId *destConnAddr, int mode, int loc)
{
	mState = BITDHT_CONNECTION_WAITING_ACK;
	mLastEvent = time(NULL);

	//mSrcId, (peer) should be okay. (ID/IP known)
	//mDestId (self) doesn't matter. (IP Unknown)
	//mProxyId (peer) should be okay. (ID/IP known)

	// mPoint, mMode should be okay.

	// mSrcConnAddr should be okay.  (ID/IP known)
	// Install the correct destConnAddr. (just received)
	mDestConnAddr = *destConnAddr; 

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mDestId.addr)); 

	// Should we check for these? This will not help for the Dest address, as we just blanked it above! */
	checkForDefaultConnectAddress();

	// Initialise the START/ACK Parameters.
	mRetryCount = 0;
	mLastStart = 0;
	mSrcAck = false;
	mDestAck = true; // Automatic ACK, as it is from us.
	mCompletedTS = 0;
	
	return 1;
}

// Proxy Connection => at Proxy, Ready to send out Start and get back ACKs!!
int bdConnection::upgradeProxyConnectionToFinish(bdId *id, bdId *srcConnAddr, bdId *destConnAddr, int mode, int status)
{
	mState = BITDHT_CONNECTION_WAITING_ACK;
	mLastEvent = time(NULL);

	//mSrcId,mDestId should be okay. (ID/IP okay)
	//mProxyId, not set, doesn't matter. (IP Unknown)

	// mPoint, mMode should be okay.

	// mSrcConnAddr should be okay. (ID/IP known)
	// Install the correct destConnAddr. (just received)
	mDestConnAddr = *destConnAddr; 

	/* clear IP Addresses to enforce this */
	bdsockaddr_clear(&(mProxyId.addr)); 

	checkForDefaultConnectAddress();

	// Initialise the START/ACK Parameters.
	mRetryCount = 0;
	mLastStart = 0;
	mSrcAck = false;
	mDestAck = false;
	mCompletedTS = 0;

	return 1;
}


// Final Sorting out of Addresses.
int bdConnection::CompleteConnection(bdId *id, bdId *srcConnAddr, bdId *destConnAddr)
{
	/* Store Final Addresses */
	time_t now = time(NULL);

	mState = BITDHT_CONNECTION_COMPLETED;
	mCompletedTS = now;
	mLastEvent = now;

	// Received Definitive Final Addresses from Proxy.
	// These have to be done by proxy, as its the only one who know both our addresses.

	mSrcConnAddr = *srcConnAddr;
	mDestConnAddr = *destConnAddr;

	checkForDefaultConnectAddress();

	return 1;
}



int bdConnection::checkForDefaultConnectAddress()
{
	// We can check if the DestConnAddr / SrcConnAddr are real.
	// If there is nothing there, we assume that that want to connect on the 
	// same IP:Port as the DHT Node.

	if (mSrcConnAddr.addr.sin_addr.s_addr == 0)
	{
		std::cerr << "bdConnectManager::checkForDefaultConnectAddress() SrcConnAddr.addr is BLANK, installing Dht Node Address";
		std::cerr << std::endl;

		mSrcConnAddr.addr = mSrcId.addr;
	}

	if (mDestConnAddr.addr.sin_addr.s_addr == 0)
	{
		std::cerr << "bdConnectManager::checkForDefaultConnectAddress() DestConnAddr.addr is BLANK, installing Dht Node Address";
		std::cerr << std::endl;

		mDestConnAddr.addr = mDestId.addr;
	}

	return 1;

}


int bdConnectionRequest::setupDirectConnection(struct sockaddr_in *laddr, bdNodeId *target)
{
	mState = BITDHT_CONNREQUEST_READY;
	mStateTS = time(NULL);
	mPauseTS = 0;
	mTarget = *target;
	mLocalAddr = *laddr;
	mMode = BITDHT_CONNECT_MODE_DIRECT;
	mRecycled = 0;
	mErrCode = 0;

	return 1;
}

int bdConnectionRequest::setupProxyConnection(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode)
{
	mState = BITDHT_CONNREQUEST_READY;
	mStateTS = time(NULL);
	mPauseTS = 0;
	mTarget = *target;
	mLocalAddr = *laddr;
	mMode = mode;
	mRecycled = 0;
	mErrCode = 0;

	return 1;
}

/* this is a good proxy peer (with flags already checked).
 * if it is in the potential proxy list, then we can add it into the good proxy list.
 */

int bdConnectionRequest::checkGoodProxyPeer(const bdId *id)
{
	std::cerr << "bdConnectionRequest::checkProxyPeer() ";
	bdStdPrintId(std::cerr, id);
	std::cerr << std::endl;

	std::list<bdId>::iterator it = std::find(mPotentialProxies.begin(), mPotentialProxies.end(), *id);
	if (it != mPotentialProxies.end())
	{
		std::cerr << "bdConnectionRequest::checkProxyPeer() Found in PotentialProxies List, adding in";
		std::cerr << std::endl;

		it = mPotentialProxies.erase(it);

		/* now add it in */
		addGoodProxy(id);
	}
	return 1;
}


int bdConnectionRequest::addGoodProxy(const bdId *srcId)
{
	std::cerr << "bdConnectionRequest::addGoodProxy() ";
	bdStdPrintId(std::cerr, srcId);
	std::cerr << std::endl;

	if (*srcId == mCurrentAttempt)
	{
		std::cerr << "bdConnectionRequest::addGoodProxy() Duplicate with CurrentAttempt";
		std::cerr << std::endl;
		return 0;
	}

	std::list<bdId>::iterator it = std::find(mPeersTried.begin(), mPeersTried.end(), *srcId);
	if (it == mPeersTried.end())
	{
		it = std::find(mGoodProxies.begin(), mGoodProxies.end(), *srcId);
		if (it == mGoodProxies.end())
		{
			std::cerr << "bdConnectionRequest::addGoodProxy() CRINITSTATE Found New Proxy: ";
			bdStdPrintId(std::cerr, srcId);
			std::cerr << std::endl;

			mGoodProxies.push_back(*srcId);

			/* if it is potentialProxies then remove */
			it = std::find(mPotentialProxies.begin(), mPotentialProxies.end(), *srcId);
			if (it != mPotentialProxies.end())
			{
				std::cerr << "bdConnectionRequest::addGoodProxy() Removing from PotentialProxy List";
				std::cerr << std::endl;

				it = mPotentialProxies.erase(it);
			}
			return 1;
		}
		else
		{
			std::cerr << "bdConnectionRequest::addGoodProxy() Duplicate in mPotentialProxies List";
			std::cerr << std::endl;
		}
	}
	else
	{
		std::cerr << "bdConnectionRequest::addGoodProxy() Already tried this peer";
		std::cerr << std::endl;
	}
	return 0;
}


std::ostream &operator<<(std::ostream &out, const bdConnectionRequest &req)
{
	time_t now = time(NULL);
	out << "bdConnectionRequest: ";
	out << "State: " << req.mState;
	out << " StateTS: " << now - req.mStateTS;
	out << " Recycled: " << req.mRecycled;
	out << std::endl;
	out << "\tTargetId: ";
	bdStdPrintNodeId(out, &(req.mTarget));
	out << std::endl;
	out << "\tMode: " << req.mMode;
	out << std::endl;

	out << "CurrentAttempt:";
	bdStdPrintId(out, &(req.mCurrentAttempt));
	out << std::endl;

	out << "GoodProxies:";
	out << std::endl;

        std::list<bdId>::const_iterator it;
	for(it = req.mGoodProxies.begin(); it != req.mGoodProxies.end(); it++)
	{
		out << "\t";
		bdStdPrintId(out, &(*it));
		out << std::endl;
	}

	out << "PotentialProxies:";
	out << std::endl;

	for(it = req.mPotentialProxies.begin(); it != req.mPotentialProxies.end(); it++)
	{
		out << "\t";
		bdStdPrintId(out, &(*it));
		out << std::endl;
	}

	out << "PeersTried:";
	out << std::endl;

	for(it = req.mPeersTried.begin(); it != req.mPeersTried.end(); it++)
	{
		out << "\t";
		bdStdPrintId(out, &(*it));
		out << std::endl;
	}
	return out;
}

std::ostream &operator<<(std::ostream &out, const bdConnection &conn)
{
	time_t now = time(NULL);
	out << "bdConnection: ";
	out << "State: " << conn.mState;
	out << " LastEvent: " << now -  conn.mLastEvent;
	out << " Point: " << conn.mPoint;
	out << " Mode: " << conn.mMode;
	out << std::endl;

	out << "\tsrcId: ";
	bdStdPrintId(out, &(conn.mSrcId));
	out << std::endl;
	out << "\tproxyId: ";
	bdStdPrintId(out, &(conn.mProxyId));
	out << std::endl;
	out << "\tdestId: ";
	bdStdPrintId(out, &(conn.mDestId));
	out << std::endl;

	out << "\tsrcConnAddr: ";
	bdStdPrintId(out, &(conn.mSrcConnAddr));
	out << std::endl;

	out << "\tdestConnAddr: ";
	bdStdPrintId(out, &(conn.mDestConnAddr));
	out << std::endl;

	out << "\tretryCount: " << conn.mRetryCount;
	out << " retryCount: " << conn.mLastStart;
	out << " srcAck: " << conn.mSrcAck;
	out << " destAck: " << conn.mDestAck;
	out << " completedTS: " << now - conn.mCompletedTS;
	out << std::endl;

	return out;

}


uint32_t createConnectionErrorCode(uint32_t userProvided, uint32_t fallback, uint32_t point)
{	
	int status = userProvided & BITDHT_CONNECT_ERROR_MASK_TYPE;
	if (status == BITDHT_CONNECT_ERROR_NONE)
	{
		status = fallback; 
	}
	/* backup, backup. */
	if (status == BITDHT_CONNECT_ERROR_NONE)
	{
		status = BITDHT_CONNECT_ERROR_GENERIC; /* FALLBACK ERROR CODE */
	}

	switch(point)
	{
		case BD_PROXY_CONNECTION_START_POINT:
			status |= BITDHT_CONNECT_ERROR_SOURCE_START;
			break;
		case BD_PROXY_CONNECTION_MID_POINT:
			status |= BITDHT_CONNECT_ERROR_SOURCE_MID;
			break;
		case BD_PROXY_CONNECTION_END_POINT:
			status |= BITDHT_CONNECT_ERROR_SOURCE_END;
			break;
	}

	return status;
}



std::string decodeConnectionErrorType(uint32_t errcode)
{
	uint32_t errtype = errcode & BITDHT_CONNECT_ERROR_MASK_TYPE;
	std::string namedtype = "UNKNOWN";
	switch(errtype)
	{
		default:
			break;
		case BITDHT_CONNECT_ERROR_GENERIC:
			namedtype = "GENERIC";
			break;
		case BITDHT_CONNECT_ERROR_PROTOCOL:
			namedtype = "PROTOCOL";
			break;
		case BITDHT_CONNECT_ERROR_TIMEOUT:
			namedtype = "TIMEOUT";
			break;
		case BITDHT_CONNECT_ERROR_TEMPUNAVAIL:
			namedtype = "TEMPUNAVAIL";
			break;
		case BITDHT_CONNECT_ERROR_NOADDRESS:
			namedtype = "NOADDRESS";
			break;
		case BITDHT_CONNECT_ERROR_UNREACHABLE:
			namedtype = "UNREACHABLE";
			break;
		case BITDHT_CONNECT_ERROR_UNSUPPORTED:
			namedtype = "UNSUPPORTED";
			break;
		case BITDHT_CONNECT_ERROR_OVERLOADED:
			namedtype = "OVERLOADED";
			break;
		case BITDHT_CONNECT_ERROR_AUTH_DENIED:
			namedtype = "AUTH_DENIED";
			break;
		case BITDHT_CONNECT_ERROR_DUPLICATE:
			namedtype = "DUPLICATE";
			break;
		case BITDHT_CONNECT_ERROR_TOOMANYRETRY:
			namedtype = "TOOMANYRETRY";
			break;
		case BITDHT_CONNECT_ERROR_OUTOFPROXY:
			namedtype = "OUTOFPROXY";
			break;
		case BITDHT_CONNECT_ERROR_USER:
			namedtype = "USER DEFINED";
			break;
	}
	return namedtype;
}


std::string decodeConnectionErrorSource(uint32_t errcode)
{
	uint32_t errsrc = errcode & BITDHT_CONNECT_ERROR_MASK_SOURCE;
	std::string namedtype = "UNKNOWN";
	switch(errsrc)
	{
		default:
			break;
		case BITDHT_CONNECT_ERROR_SOURCE_START:
			namedtype = "START";
			break;
		case BITDHT_CONNECT_ERROR_SOURCE_MID:
			namedtype = "MID";
			break;
		case BITDHT_CONNECT_ERROR_SOURCE_END:
			namedtype = "END";
			break;
		case BITDHT_CONNECT_ERROR_SOURCE_OTHER:
			namedtype = "OTHER";
			break;
	}
	return namedtype;
}


#if 0
std::string decodeConnectionErrorCRMove(uint32_t errcode)
{
	uint32_t errcr = errcode & BITDHT_CONNECT_ERROR_MASK_CRMOVE;
	std::string namedtype = "UNKNOWN";
	switch(errcr)
	{
		default:
			break;
		case 0:
			namedtype = "REMOTE";
			break;
		case BITDHT_CONNECT_ERROR_CRMOVE_FATAL:
			namedtype = "FATAL";
			break;
		case BITDHT_CONNECT_ERROR_SOURCE_NOMOREIDS:
			namedtype = "NOMOREIDS";
			break;
		case BITDHT_CONNECT_ERROR_SOURCE_NEXTID:
			namedtype = "NEXTID";
			break;
		case BITDHT_CONNECT_ERROR_CRMOVE_PAUSED:
			namedtype = "PAUSED";
			break;
	}
	return namedtype;
}
#endif



std::string decodeConnectionError(uint32_t errcode)
{
	std::string totalerror;
	if (!errcode)
	{
		totalerror = "NoError";
	}
	else
	{
		//totalerror = decodeConnectionErrorCRMove(errcode);
		//totalerror += ":";
		totalerror = decodeConnectionErrorSource(errcode);
		totalerror += ":";
		totalerror += decodeConnectionErrorType(errcode);
	}

	return totalerror;
}

