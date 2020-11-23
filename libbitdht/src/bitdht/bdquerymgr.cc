/*******************************************************************************
 * bitdht/bdquerymgr.cc                                                        *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <bitdht@lunamutt.com>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "bitdht/bdquerymgr.h"
#include "bitdht/bdnode.h"

#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>
#include <iomanip>


#define BITDHT_QUERY_START_PEERS    10
#define BITDHT_QUERY_NEIGHBOUR_PEERS    8
#define BITDHT_MAX_REMOTE_QUERY_AGE	10

/****
 * #define DEBUG_NODE_MULTIPEER 1
 * #define DEBUG_NODE_MSGS 1
 * #define DEBUG_NODE_ACTIONS 1

 * #define DEBUG_NODE_MSGIN 1
 * #define DEBUG_NODE_MSGOUT 1
 ***/

//#define DEBUG_NODE_MSGS 1


bdQueryManager::bdQueryManager(bdSpace *space, bdDhtFunctions *fns, bdNodePublisher *pub)
	:mNodeSpace(space), mFns(fns), mPub(pub)
{
}

/***** Startup / Shutdown ******/
void bdQueryManager::shutdownQueries()
{
	/* clear the queries */
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end();it++)
	{
		delete (*it);
	}

	mLocalQueries.clear();
}


void bdQueryManager::printQueries()
{
	std::cerr << "bdQueryManager::printQueries()";
	std::cerr << std::endl;

	int i = 0;
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++, i++)
	{
		fprintf(stderr, "Query #%d:\n", i);
		(*it)->printQuery();
		fprintf(stderr, "\n");
	}
}


int bdQueryManager::iterateQueries(int maxQueries)
{
#ifdef DEBUG_NODE_MULTIPEER 
	std::cerr << "bdQueryManager::iterateQueries() of Peer: ";
	mFns->bdPrintNodeId(std::cerr, &mOwnId);
	std::cerr << std::endl;
#endif

	/* allow each query to send up to one query... until maxMsgs has been reached */
	int numQueries = mLocalQueries.size();
	int sentQueries = 0;
	int i = 0;
	
	bdId id;
	bdNodeId targetNodeId;
	
	while((i < numQueries) && (sentQueries < maxQueries))
	{
		bdQuery *query = mLocalQueries.front();
		mLocalQueries.pop_front();
		mLocalQueries.push_back(query);

		/* go through the possible queries */
		if (query->nextQuery(id, targetNodeId))
		{
#ifdef DEBUG_NODE_MSGS 
			std::cerr << "bdQueryManager::iteration() send_query(";
			mFns->bdPrintId(std::cerr, &id);
			std::cerr << ",";
			mFns->bdPrintNodeId(std::cerr, &targetNodeId);
			std::cerr << ")";
			std::cerr << std::endl;
#endif
			mPub->send_query(&id, &targetNodeId, false);
			sentQueries++;
		}
		i++;
	}
	
#ifdef DEBUG_NODE_ACTIONS 
	std::cerr << "bdQueryManager::iteration() maxMsgs: " << maxMsgs << " sentPings: " << sentPings;
	std::cerr << " / " << allowedPings;
	std::cerr << " sentQueries: " << sentQueries;
	std::cerr << " / " << numQueries;
	std::cerr << std::endl;
#endif

	//printQueries();

	return sentQueries;
}


bool bdQueryManager::checkPotentialPeer(bdId *id, bdId *src)
{
	bool isWorthyPeer = false;
	/* also push to queries */
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		if ((*it)->addPotentialPeer(id, src, 0))
		{
			isWorthyPeer = true;
		}
	}

	if (!isWorthyPeer)
	{
		isWorthyPeer = checkWorthyPeerSources(src);
	}

	return isWorthyPeer;
}


void bdQueryManager::addPeer(const bdId *id, uint32_t peerflags)
{

#ifdef DEBUG_NODE_ACTIONS 
	fprintf(stderr, "bdQueryManager::addPeer(");
	mFns->bdPrintId(std::cerr, id);
	fprintf(stderr, ")\n");
#endif

	/* iterate through queries */
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		(*it)->addPeer(id, peerflags);
	}
}


/************************************ Query Details        *************************/
void bdQueryManager::addQuery(const bdNodeId *id, uint32_t qflags)
{

	std::list<bdId> startList;
	std::multimap<bdMetric, bdId> nearest;
	std::multimap<bdMetric, bdId>::iterator it;

	mNodeSpace->find_nearest_nodes(id, BITDHT_QUERY_START_PEERS, nearest);

#ifdef DEBUG_NODE_ACTIONS 
	fprintf(stderr, "bdQueryManager::addQuery(");
	mFns->bdPrintNodeId(std::cerr, id);
	fprintf(stderr, ")\n");
#endif

	for(it = nearest.begin(); it != nearest.end(); it++)
	{
		startList.push_back(it->second);
	}

	bdQuery *query = new bdQuery(id, startList, qflags, mFns);
	mLocalQueries.push_back(query);
}


void bdQueryManager::clearQuery(const bdNodeId *rmId)
{
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end();)
	{
		if ((*it)->mId == *rmId)
		{
			bdQuery *query = (*it);
			it = mLocalQueries.erase(it);
			delete query;
		}
		else
		{
			it++;
		}
	}
}

void bdQueryManager::QueryStatus(std::map<bdNodeId, bdQueryStatus> &statusMap)
{
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		bdQueryStatus status;
		status.mStatus = (*it)->mState;
		status.mQFlags = (*it)->mQueryFlags;
		(*it)->result(status.mResults);
		statusMap[(*it)->mId] = status;
	}
}

int bdQueryManager::QuerySummary(const bdNodeId *id, bdQuerySummary &query)
{
	std::list<bdQuery *>::iterator it;
	for(it = mLocalQueries.begin(); it != mLocalQueries.end(); it++)
	{
		if ((*it)->mId == *id)
		{
			query.mId = (*it)->mId;
			query.mLimit = (*it)->mLimit;
			query.mState = (*it)->mState;
			query.mQueryTS = (*it)->mQueryTS;
			query.mQueryFlags = (*it)->mQueryFlags;
			query.mSearchTime = (*it)->mSearchTime;
			query.mClosest = (*it)->mClosest;
			query.mPotentialPeers = (*it)->mPotentialPeers;
			query.mProxiesUnknown = (*it)->mProxiesUnknown;
			query.mProxiesFlagged = (*it)->mProxiesFlagged;
 			query.mQueryIdlePeerRetryPeriod = (*it)->mQueryIdlePeerRetryPeriod;

			return 1;
		}
	}
	return 0;
}

/* Extract Results from Peer Queries */

#define BDQRYMGR_RESULTS 	1
#define BDQRYMGR_PROXIES	2
#define BDQRYMGR_POTPROXIES	3

int  bdQueryManager::getResults(bdNodeId *target, std::list<bdId> &answer, int querytype)
{
		
	/* grab any peers from any existing query */
	int results = 0;
	std::list<bdQuery *>::iterator qit;
	for(qit = mLocalQueries.begin(); qit != mLocalQueries.end(); qit++)
	{
		if (!((*qit)->mId == (*target)))
		{
			continue;
		}
		
#ifdef DEBUG_NODE_CONNECTION
		std::cerr << "bdQueryManager::getResults() Found Matching Query";
		std::cerr << std::endl;
#endif
		switch(querytype)
		{
			default:
			case BDQRYMGR_RESULTS:
				results = (*qit)->result(answer);
				break;
			case BDQRYMGR_PROXIES:
				results = (*qit)->proxies(answer);
				break;
			case BDQRYMGR_POTPROXIES:
				results = (*qit)->potentialProxies(answer);
				break;
		}
		/* will only be one matching query.. so end loop */
		return results;
	}
	return 0;
}

		
int  bdQueryManager::result(bdNodeId *target, std::list<bdId> &answer)
{
	return getResults(target, answer,  BDQRYMGR_RESULTS);
}
		
int  bdQueryManager::proxies(bdNodeId *target, std::list<bdId> &answer)
{
	return getResults(target, answer,  BDQRYMGR_PROXIES);
}
		
int  bdQueryManager::potentialProxies(bdNodeId *target, std::list<bdId> &answer)
{
	return getResults(target, answer,  BDQRYMGR_POTPROXIES);
}


/************ WORTHY PEERS **********/

#define MAX_WORTHY_PEER_AGE 15

void bdQueryManager::addWorthyPeerSource(bdId *src)
{
	time_t now = time(NULL);

	bdPeer peer;
	peer.mPeerId = *src;
	peer.mFoundTime = now;

#ifdef DEBUG_NODE_ACTIONS 
	std::cerr << "bdQueryManager::addWorthyPeerSource(";
	mFns->bdPrintId(std::cerr, src);
	std::cerr << ")" << std::endl;
#endif

	mWorthyPeerSources.push_back(peer);
}

bool bdQueryManager::checkWorthyPeerSources(bdId *src)
{
	if (!src)
		return false;

	time_t now = time(NULL);
	std::list<bdPeer>::iterator it;
	for(it = mWorthyPeerSources.begin(); it != mWorthyPeerSources.end(); )
	{
		if (now - it->mFoundTime > MAX_WORTHY_PEER_AGE)
		{
#ifdef DEBUG_NODE_ACTIONS 
			std::cerr << "bdQueryManager::checkWorthyPeerSource() Discard old Source: ";
			mFns->bdPrintId(std::cerr, &(it->mPeerId));
			std::cerr << std::endl;
#endif

			it = mWorthyPeerSources.erase(it);
		}
		else
		{
			if (it->mPeerId == *src)
			{
#ifdef DEBUG_NODE_ACTIONS 
				std::cerr << "bdQueryManager::checkWorthyPeerSource(";
				mFns->bdPrintId(std::cerr, src);
				std::cerr << ") = true" << std::endl;
#endif

				return true;
			}
			it++;
		}
	}
	return false;
}



