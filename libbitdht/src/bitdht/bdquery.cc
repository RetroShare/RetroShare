/*******************************************************************************
 * bitdht/bdquery.cc                                                           *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2010 by Robert Fernie <bitdht@lunamutt.com>                       *
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

#include "bitdht/bdquery.h"
#include "bitdht/bdstddht.h"
#include "util/bdnet.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

/**
 * #define DEBUG_QUERY 1
**/

#define EXPECTED_REPLY 10 // Speed up queries
#define QUERY_IDLE_RETRY_PEER_PERIOD 600 // 10min =  (mFns->bdNumQueryNodes() * 60)
#define MAX_QUERY_IDLE_PERIOD 	     1200 // 20min.


/************************************************************
 * bdQuery logic:
 *  1) as replies come in ... maintain list of M closest peers to ID.
 *  2) select non-queried peer from list, and query.
 *  3) halt when we have asked all M closest peers about the ID.
 * 
 * Flags can be set to disguise the target of the search.
 * This involves 
 */

bdQuery::bdQuery(const bdNodeId *id, std::list<bdId> &startList, uint32_t queryFlags, 
		bdDhtFunctions *fns)
{
	/* */
	mId = *id;
	mFns = fns;

	time_t now = time(NULL);
	std::list<bdId>::iterator it;
	for(it = startList.begin(); it != startList.end(); it++)
	{
		bdPeer peer;
		peer.mLastSendTime = 0;
		peer.mLastRecvTime = 0;
		peer.mPeerFlags = 0;
		peer.mFoundTime = now;
		peer.mPeerId = *it;

		bdMetric dist;

		mFns->bdDistance(&mId, &(peer.mPeerId.id), &dist);

		mClosest.insert(std::pair<bdMetric, bdPeer>(dist, peer));


	}

	mState = BITDHT_QUERY_QUERYING;
	mQueryFlags = queryFlags;
	mQueryTS = now;
	mSearchTime = 0;
	mClosestListSize = (int) (1.5 * mFns->bdNumQueryNodes());
	mPotPeerCleanTS = now;

	mQueryIdlePeerRetryPeriod = QUERY_IDLE_RETRY_PEER_PERIOD;
	mRequiredPeerFlags = BITDHT_PEER_STATUS_DHT_ENGINE_VERSION; // XXX to update later.

	/* setup the limit of the search
	 * by default it is setup to 000000 = exact match
	 */
	bdZeroNodeId(&mLimit);
}

bool bdQuery::result(std::list<bdId> &answer)
{
	/* get all the matches to our query */
        std::multimap<bdMetric, bdPeer>::iterator sit, eit;
	sit = mClosest.begin();
	eit = mClosest.upper_bound(mLimit);
	int i = 0;
	for(; sit != eit; sit++)
	{
		if ((sit->second).mLastRecvTime != 0)
		{
			answer.push_back(sit->second.mPeerId);
			i++;
		}
	}
	return (i > 0);
}


int bdQuery::nextQuery(bdId &id, bdNodeId &targetNodeId)
{
	if ((mState != BITDHT_QUERY_QUERYING) && !(mQueryFlags & BITDHT_QFLAGS_DO_IDLE))
	{
#ifdef DEBUG_QUERY 
        	fprintf(stderr, "NextQuery() Query is done\n");
#endif
		return 0;
	}

	/* search through through list, find closest not queried */
	time_t now = time(NULL);

	/* update IdlePeerRetry */
	if ((now - mQueryTS) / 2 > mQueryIdlePeerRetryPeriod)
	{
		mQueryIdlePeerRetryPeriod = (now-mQueryTS) / 2;
		if (mQueryIdlePeerRetryPeriod > MAX_QUERY_IDLE_PERIOD)
		{
			mQueryIdlePeerRetryPeriod = MAX_QUERY_IDLE_PERIOD;
		}
	}

	bool notFinished = false;
	std::multimap<bdMetric, bdPeer>::iterator it;
	int i = 0;
	for(it = mClosest.begin(); it != mClosest.end(); it++, i++)
	{
		bool queryPeer = false;

		/* if never queried */
		if (it->second.mLastSendTime == 0)
		{
#ifdef DEBUG_QUERY 
        		fprintf(stderr, "NextQuery() Found non-sent peer. queryPeer = true : ");
			mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
			std::cerr << std::endl;
#endif
			queryPeer = true;
		}

		/* re-request every so often */
		if ((!queryPeer) && (mQueryFlags & BITDHT_QFLAGS_DO_IDLE) && 
			(now - it->second.mLastSendTime > mQueryIdlePeerRetryPeriod))
		{
#ifdef DEBUG_QUERY 
        		fprintf(stderr, "NextQuery() Found out-of-date. queryPeer = true : ");
			mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
			std::cerr << std::endl;
#endif
			queryPeer = true;
		}
		
		/* expecting every peer to be up-to-date is too hard...
		 * enough just to have received lists from each 
		 * - replacement policy will still work.
		 *
		 * Need to wait at least EXPECTED_REPLY, to make sure their answers are pinged
		 */	

		if (((it->second.mLastRecvTime == 0) || (now - it->second.mLastRecvTime < EXPECTED_REPLY)) && 
				(i < mFns->bdNumQueryNodes()))
		{
#ifdef DEBUG_QUERY 
        		fprintf(stderr, "NextQuery() Never Received @Idx(%d) notFinished = true: ", i);
			mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
			std::cerr << std::endl;
#endif
			notFinished = true;
		}

		if (queryPeer)
		{
			id = it->second.mPeerId;
			it->second.mLastSendTime = now;

			if (mQueryFlags & BITDHT_QFLAGS_DISGUISE)
			{
				/* calc Id mid point between Target and Peer */
				bdNodeId midRndId;
				mFns->bdRandomMidId(&mId, &(id.id), &midRndId);

				targetNodeId = midRndId;
			}
			else
			{
				targetNodeId = mId;
			}
#ifdef DEBUG_QUERY 
        		fprintf(stderr, "NextQuery() Querying Peer: ");
			mFns->bdPrintId(std::cerr, &id);
			std::cerr << std::endl;
#endif
			return 1;
		}
	}

	/* allow query to run for a minimal amount of time
	 * This is important as startup - when we might not have any peers.	
	 * Probably should be handled elsewhere.
	 */
	time_t age = now - mQueryTS;

	if (age < BITDHT_MIN_QUERY_AGE)
	{
#ifdef DEBUG_QUERY 
       		fprintf(stderr, "NextQuery() under Min Time: Query not finished / No Query\n");
#endif
		return 0;
	}

	if (age > BITDHT_MAX_QUERY_AGE)
	{
#ifdef DEBUG_QUERY 
       		fprintf(stderr, "NextQuery() over Max Time: Query force to Finish\n");
#endif
		/* fall through and stop */
	}
	else if ((mClosest.size() < mFns->bdNumQueryNodes()) || (notFinished))
	{
#ifdef DEBUG_QUERY 
       		fprintf(stderr, "NextQuery() notFinished | !size(): Query not finished / No Query\n");
#endif
		/* not full yet... */
		return 0;
	}

#ifdef DEBUG_QUERY 
	fprintf(stderr, "NextQuery() Finished\n");
#endif

	/* if we get here - query finished */
	if (mState == BITDHT_QUERY_QUERYING)
	{
		/* store query time */
		mSearchTime = now - mQueryTS;
	}

	/* cleanup PotentialPeers before doing the final State */;
	removeOldPotentialPeers();
	/* check if we found the node */
	if (mClosest.size() > 0)
	{
		if (((mClosest.begin()->second).mPeerId.id == mId) && 
			((mClosest.begin()->second).mLastRecvTime != 0))
		{
			mState = BITDHT_QUERY_SUCCESS;
		}
		else if ((mPotentialPeers.begin()->second).mPeerId.id == mId)
		{
			mState = BITDHT_QUERY_PEER_UNREACHABLE;
		}
		else
		{
			mState = BITDHT_QUERY_FOUND_CLOSEST;
		}
	}
	else
	{
		mState = BITDHT_QUERY_FAILURE;
	}
	return 0;
}

int bdQuery::addClosestPeer(const bdId *id, uint32_t mode)
{
	bdMetric dist;
	time_t ts = time(NULL);

	mFns->bdDistance(&mId, &(id->id), &dist);

#ifdef DEBUG_QUERY 
        fprintf(stderr, "bdQuery::addPeer(");
	mFns->bdPrintId(std::cerr, id);
        fprintf(stderr, ", %u)\n", mode);
#endif

	std::multimap<bdMetric, bdPeer>::iterator it, sit, eit;
	sit = mClosest.lower_bound(dist);
	eit = mClosest.upper_bound(dist);
	int i = 0;
	int actualCloser = 0;
	int toDrop = 0;
	// switched end condition to upper_bound to provide stability for NATTED peers.
	// we will favour the older entries!
	for(it = mClosest.begin(); it != eit; it++, i++, actualCloser++)
	{
		time_t sendts = ts - it->second.mLastSendTime;
		bool hasSent = (it->second.mLastSendTime != 0);
		bool hasReply = (it->second.mLastRecvTime >= it->second.mLastSendTime);
		if ((hasSent) && (!hasReply) && (sendts > EXPECTED_REPLY))
		{
			i--; /* dont count this one */
			toDrop++;
		}
	}
		// Counts where we are.
#ifdef DEBUG_QUERY 
        fprintf(stderr, "Searching.... %di = %d - %d peers closer than this one\n", i, actualCloser, toDrop);
#endif


	if (i > mClosestListSize - 1)
	{
#ifdef DEBUG_QUERY 
        	fprintf(stderr, "Distance to far... dropping\n");
#endif
		/* drop it */
		return 0;
	}

	for(it = sit; it != eit; it++, i++)
	{
		/* full id check */
		if (mFns->bdSimilarId(id, &(it->second.mPeerId)))
		{
#ifdef DEBUG_QUERY 
        		fprintf(stderr, "Peer Already here!\n");
#endif
			if (mode)
			{
				/* also update port from incoming id, as we have definitely recved from it */
				if (mFns->bdUpdateSimilarId(&(it->second.mPeerId), id))
				{
					/* updated it... must be Unstable */
					it->second.mExtraFlags |= BITDHT_PEER_EXFLAG_UNSTABLE;
				}
			}
			if (mode & BITDHT_PEER_STATUS_RECV_NODES)
			{
				/* only update recvTime if sendTime > checkTime.... (then its our query) */
#ifdef DEBUG_QUERY 
				fprintf(stderr, "Updating LastRecvTime\n");
#endif
				it->second.mLastRecvTime = ts;
				it->second.mPeerFlags |= mode;
			}
			return 1;
		}
	}

#ifdef DEBUG_QUERY 
        fprintf(stderr, "Peer not in Query\n");
#endif
	/* firstly drop unresponded (bit ugly - but hard structure to extract from) */
	int j;
	for(j = 0; j < toDrop; j++)
	{
#ifdef DEBUG_QUERY 
        	fprintf(stderr, "Dropping Peer that dont reply\n");
#endif
		for(it = mClosest.begin(); it != mClosest.end(); ++it)
		{
			time_t sendts = ts - it->second.mLastSendTime;
			bool hasSent = (it->second.mLastSendTime != 0);
			bool hasReply = (it->second.mLastRecvTime >= it->second.mLastSendTime);

			if ((hasSent) && (!hasReply) && (sendts > EXPECTED_REPLY))
			{
#ifdef DEBUG_QUERY 
        			fprintf(stderr, "Dropped: ");
				mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
        			fprintf(stderr, "\n");
#endif
				mClosest.erase(it);
				break ;
			}
		}
	}

	/* trim it back */
	while(mClosest.size() > (uint32_t) (mClosestListSize - 1))
	{
		std::multimap<bdMetric, bdPeer>::iterator it;
		it = mClosest.end();
		if (!mClosest.empty())
		{
			it--;
#ifdef DEBUG_QUERY 
			fprintf(stderr, "Removing Furthest Peer: ");
			mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
			fprintf(stderr, "\n");
#endif

			mClosest.erase(it);
		}
	}

#ifdef DEBUG_QUERY 
        fprintf(stderr, "bdQuery::addPeer(): Closer Peer!: ");
	mFns->bdPrintId(std::cerr, id);
        fprintf(stderr, "\n");
#endif

	/* add it in */
	bdPeer peer;
	peer.mPeerId = *id;
	peer.mPeerFlags = mode;
	peer.mLastSendTime = 0;
	peer.mLastRecvTime = 0;
	peer.mFoundTime = ts;

	if (mode & BITDHT_PEER_STATUS_RECV_NODES)
	{
		peer.mLastRecvTime = ts;
	}

	mClosest.insert(std::pair<bdMetric, bdPeer>(dist, peer));
	return 1;
}


/*******************************************************************************************
 ********************************* Add Peer Interface  *************************************
 *******************************************************************************************/

/**** These functions are called by bdNode to add peers to the query 
 * They add/update the three sets of lists.
 *
 * int bdQuery::addPeer(const bdId *id, uint32_t mode)
 * Proper message from a peer.
 *
 * int bdQuery::addPotentialPeer(const bdId *id, const bdId *src, uint32_t srcmode)
 * This returns 1 if worthy of pinging, 0 if to ignore.
 */

#define PEER_MESSAGE		0
#define FIND_NODE_RESPONSE	1

int bdQuery::addPeer(const bdId *id, uint32_t mode)
{
	addClosestPeer(id, mode);
	updatePotentialPeer(id, mode, PEER_MESSAGE);
	updateProxy(id, mode);
	return 1;
}

int bdQuery::addPotentialPeer(const bdId *id, const bdId *src, uint32_t srcmode)
{
	// is it a Potential Proxy? Always Check This.
	addProxy(id, src, srcmode);

	int worthy = worthyPotentialPeer(id);
	int shouldPing = 0;
	if (worthy)
	{
		shouldPing = updatePotentialPeer(id, 0, FIND_NODE_RESPONSE);
	}
	return shouldPing;
}


/*******************************************************************************************
 ********************************* Closest Peer ********************************************
 *******************************************************************************************/


/*******************************************************************************************
 ******************************** Potential Peer *******************************************
 *******************************************************************************************/




/*******
 * Potential Peers are a list of the closest answers to our queries.
 * Lots of these peers will not be reachable.... so will only exist in this list.
 * They will also never have there PeerFlags set ;(
 *
 */


/*** utility functions ***/

int bdQuery::worthyPotentialPeer(const bdId *id)
{
	bdMetric dist;
	mFns->bdDistance(&mId, &(id->id), &dist);

#ifdef DEBUG_QUERY 
	std::cerr << "bdQuery::worthyPotentialPeer(";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << std::endl;
#endif

	/* we check if this is a worthy potential peer....
	 * if it is already in mClosest -> false. old peer.
	 * if it is > mClosest.rbegin() -> false. too far way.
	 */

	std::multimap<bdMetric, bdPeer>::iterator it, sit, eit;
	sit = mClosest.lower_bound(dist);
	eit = mClosest.upper_bound(dist);

	/* check if outside range, & bucket is full  */
	if ((sit == mClosest.end()) && (mClosest.size() >= mFns->bdNumQueryNodes()))
	{
#ifdef DEBUG_QUERY 
		fprintf(stderr, "Peer to far away for Potential\n");
#endif
		return 0; /* too far way */
	}


	for(it = sit; it != eit; it++)
	{
		if (mFns->bdSimilarId(id, &(it->second.mPeerId)))
		{
			// Not updating Full Peer Id here... as inspection function.
#ifdef DEBUG_QUERY 
			fprintf(stderr, "Peer already in mClosest\n");
#endif
			return 0;
		}
	}

	return 1; /* either within mClosest Range (but not there!), or there aren't enough peers */
}


/***** 
 *
 * mLastSendTime ... is the last FIND_NODE_RESPONSE that we returned 1. (indicating to PING).
 * mLastRecvTime ... is the last time we received an updatei about/from them
 *
 * The update is dependent on the flags passed in the function call. (saves duplicate code).
 *
 * 
 * XXX IMPORTANT TO DECIDE WHAT IS RETURNED HERE.
 * original algorithm return 0 if exists in potential peers, 1 if unknown.
 * This is used to limit the number of pings to non-responding potentials.
 *
 * MUST think about this. Need to install HISTORY tracking again. to look at the statistics.
 *
 * It is important that the potential Peers list extends all the way back to == mClosest().end().
 * Otherwise we end up with [TARGET] .... [ POTENTIAL ] ..... [ CLOSEST ] ......
 * and the gap between POT and CLOSEST will get hammered with pings.
 * 
 */

#define MIN_PING_POTENTIAL_PERIOD 300

int bdQuery::updatePotentialPeer(const bdId *id, uint32_t mode, uint32_t addType)
{
	bdMetric dist;
	time_t now = time(NULL);

	mFns->bdDistance(&mId, &(id->id), &dist);

	std::multimap<bdMetric, bdPeer>::iterator it, sit, eit;
	sit = mPotentialPeers.lower_bound(dist);
	eit = mPotentialPeers.upper_bound(dist);

	for(it = sit; it != eit; it++)
	{
		if (mFns->bdSimilarId(id, &(it->second.mPeerId)))
		{

			it->second.mPeerFlags |= mode;
			it->second.mLastRecvTime = now;
			if (addType == FIND_NODE_RESPONSE)
			{
				// We could lose peers here by not updating port... but should be okay.
				if (now - it->second.mLastSendTime  > MIN_PING_POTENTIAL_PERIOD)
				{
					it->second.mLastSendTime = now;
					return 1;
				}
			}
			else if (mode) 
			{
				/* also update port from incoming id, as we have definitely recved from it */
				if (mFns->bdUpdateSimilarId(&(it->second.mPeerId), id))
				{
					/* updated it... must be Unstable */
					it->second.mExtraFlags |= BITDHT_PEER_EXFLAG_UNSTABLE;
				}
			}
			return 0;
		}
	}

// Removing this check - so that we can have varying length PotentialPeers.
// Peer will always be added, then probably removed straight away.

#if 0
	/* check if outside range, & bucket is full  */
	if ((sit == mPotentialPeers.end()) && (mPotentialPeers.size() >= mFns->bdNumQueryNodes()))
	{
#ifdef DEBUG_QUERY 
		fprintf(stderr, "Peer to far away for Potential\n");
#endif
		return 0;
	}
#endif

	/* add it in */
	bdPeer peer;
	peer.mPeerId = *id;
	peer.mPeerFlags = mode;
	peer.mFoundTime = now;
	peer.mLastRecvTime = now;
	peer.mLastSendTime = 0;
	if (addType == FIND_NODE_RESPONSE)
	{
		peer.mLastSendTime = now;
	}

	mPotentialPeers.insert(std::pair<bdMetric, bdPeer>(dist, peer));

#ifdef DEBUG_QUERY 
	fprintf(stderr, "Flagging as Potential Peer!\n");
#endif

	trimPotentialPeers_toClosest();

	return 1;
}


int bdQuery::trimPotentialPeers_FixedLength()
{
	/* trim it back */
	while(mPotentialPeers.size() > (uint32_t) (mFns->bdNumQueryNodes()))
	{
		std::multimap<bdMetric, bdPeer>::iterator it;
		it = mPotentialPeers.end();
		it--; // must be more than 1 peer here?
#ifdef DEBUG_QUERY 
		fprintf(stderr, "Removing Furthest Peer: ");
		mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
		fprintf(stderr, "\n");
#endif
		mPotentialPeers.erase(it);
	}
	return 1;
}


int bdQuery::trimPotentialPeers_toClosest()
{
	if (mPotentialPeers.size() <= (uint32_t) (mFns->bdNumQueryNodes()))
		return 1;

	std::multimap<bdMetric, bdPeer>::reverse_iterator it;
	it = mClosest.rbegin();
	bdMetric lastClosest = it->first;
	
	/* trim it back */
	while(mPotentialPeers.size() > (uint32_t) (mFns->bdNumQueryNodes()))
	{
		std::multimap<bdMetric, bdPeer>::iterator it;
		it = mPotentialPeers.end();
		it--; // must be more than 1 peer here?
		if (lastClosest < it->first)
		{
#ifdef DEBUG_QUERY 
			fprintf(stderr, "Removing Furthest Peer: ");
			mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
			fprintf(stderr, "\n");
#endif
			mPotentialPeers.erase(it);
		}
		else
		{
			return 1;
		}
	}
	return 1;
}


/* as Potential Peeers are to determine if a peer is CLOSEST or UNREACHABLE
 * we need to drop ones that we haven't heard about in ages.
 *
 * only do this in IDLE mode...
 * The timeout period is dependent on our RetryPeriod.
 */

#define POT_PEER_CLEAN_PERIOD	60
#define POT_PEER_RECV_TIMEOUT_PERIOD (mQueryIdlePeerRetryPeriod + EXPECTED_REPLY)

int bdQuery::removeOldPotentialPeers()
{
	if (!(mQueryFlags & BITDHT_QFLAGS_DO_IDLE))
	{
		return 0;
	}

	time_t now = time(NULL);
	if (now - mPotPeerCleanTS < POT_PEER_CLEAN_PERIOD)
	{
		return 0;
	}

	mPotPeerCleanTS = now;

	/* painful loop */
	std::multimap<bdMetric, bdPeer>::iterator it;
	for(it = mPotentialPeers.begin(); it != mPotentialPeers.end();)
	{
		/* which timestamp do we care about? */
		if (now - it->second.mLastRecvTime > POT_PEER_RECV_TIMEOUT_PERIOD)
		{ 
#ifdef DEBUG_QUERY 
			std::cerr << "bdQuery::removeOldPotentialPeers() removing: ";
			mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
			fprintf(stderr, "\n");
#endif
			std::multimap<bdMetric, bdPeer>::iterator it2 = it;
			++it2 ;
			mPotentialPeers.erase(it);
			it = it2 ;

			// Unfortunately have to start again... as pointers invalidated.
			//it = mPotentialPeers.begin();
		}
		else
		{
			++it;
		}
	}
	return 1 ;
}



/*******************************************************************************************
 ******************************** Potential Proxies ****************************************
 *******************************************************************************************/

/********
 * Potential Proxies. a list of peers that have returned our target in response to a query.
 *
 * We are particularly interested in peers with specific flags...
 * But all the peers have been pinged already by the time they reach this list.
 * So there are two options:
 *   1) Track everythings mode history - which is a waste of resources.
 *   2) Store the list, and ping later.
 *   
 * We will store these in two lists: Flags & Unknown.
 * we keep the most recent of each, and move around as required.
 *
 * we could also check the Closest/PotentialPeer lists to grab the flags, 
 * for an unknown peer?
 *
 * All Functions manipulating PotentialProxies are here.
 * We need several functions:
 *
 * For Extracting Proxies.
bool bdQuery::proxies(std::list<bdId> &answer)
bool bdQuery::potentialProxies(std::list<bdId> &answer)
 *
 * For Adding/Updating Proxies.
int bdQuery::addProxy(const bdId *id, const bdId *src, uint32_t srcmode)
int bdQuery::updateProxy(const bdId *id, uint32_t mode)
 *
 */

/*** Two Functions to extract Proxies... ***/
bool bdQuery::proxies(std::list<bdId> &answer)
{
	/* get all the matches to our query */
        std::list<bdPeer>::iterator it;
	int i = 0;
	for(it = mProxiesFlagged.begin(); it != mProxiesFlagged.end(); it++, i++)
	{
		answer.push_back(it->mPeerId);
	}
	return (i > 0);
}

bool bdQuery::potentialProxies(std::list<bdId> &answer)
{
	/* get all the matches to our query */
        std::list<bdPeer>::iterator it;
	int i = 0;
	for(it = mProxiesUnknown.begin(); it != mProxiesUnknown.end(); it++, i++)
	{
		answer.push_back(it->mPeerId);
	}
	return (i > 0);
}



int bdQuery::addProxy(const bdId *id, const bdId *src, uint32_t srcmode)
{
	bdMetric dist;
	time_t now = time(NULL);
	
	mFns->bdDistance(&mId, &(id->id), &dist);
	
	/* finally if it is an exact match, add as potential proxy */
	int bucket = mFns->bdBucketDistance(&dist);
	if ((bucket != 0) || (src == NULL))
	{
		/* not a potential proxy */
		return 0;
	}

#ifdef DEBUG_QUERY 
	fprintf(stderr, "Bucket = 0, Have Potential Proxy!\n");
#endif

	bool found = false;
	if (updateProxyList(src, srcmode, mProxiesUnknown))
	{
		found = true;
	}

	if (!found)
	{
		if (updateProxyList(src, srcmode, mProxiesFlagged))
		{
			found = true;
		}
	}

	if (!found)
	{
		/* if we get here. its not in the list */
#ifdef DEBUG_QUERY 
		fprintf(stderr, "Adding Source to Proxy List:\n");
#endif
		bdPeer peer;
		peer.mPeerId = *src;
		peer.mPeerFlags = srcmode;
		peer.mLastSendTime = 0;
		peer.mLastRecvTime = now;
		peer.mFoundTime = now;
	
		/* add it in */
		if ((srcmode & mRequiredPeerFlags) == mRequiredPeerFlags)
		{
			mProxiesFlagged.push_front(peer);
		}
		else
		{
			mProxiesUnknown.push_front(peer);
		}
	}

	
	trimProxies();
	
	return 1;
}


int bdQuery::updateProxy(const bdId *id, uint32_t mode)
{
	if (!updateProxyList(id, mode, mProxiesUnknown))
	{
		updateProxyList(id, mode, mProxiesFlagged);
	}

	trimProxies();
	return 1;
}


/**** Utility functions that do all the work! ****/

int bdQuery::updateProxyList(const bdId *id, uint32_t mode, std::list<bdPeer> &searchProxyList)
{
	std::list<bdPeer>::iterator it;
	for(it = searchProxyList.begin(); it != searchProxyList.end(); it++)
	{
		if (mFns->bdSimilarId(id, &(it->mPeerId)))
		{
			/* found it ;( */
#ifdef DEBUG_QUERY 
			std::cerr << "bdQuery::updateProxyList() Found peer, updating";
			std::cerr << std::endl;
#endif

			time_t now = time(NULL);
			if (mode)
			{
				/* also update port from incoming id, as we have definitely recved from it */
				if (mFns->bdUpdateSimilarId(&(it->mPeerId), id))
				{
					/* updated it... must be Unstable */
					it->mExtraFlags |= BITDHT_PEER_EXFLAG_UNSTABLE;
				}
			}
			it->mPeerFlags |= mode;
			it->mLastRecvTime = now;

			/* now move it to the front of required list...
			 * note this could be exactly the same list as &searchProxyList, or a different one!
		 	 */

			bdPeer peer = *it;
			it = searchProxyList.erase(it);

			if ((peer.mPeerFlags & mRequiredPeerFlags) == mRequiredPeerFlags)
			{
				mProxiesFlagged.push_front(peer);
			}
			else
			{
				mProxiesUnknown.push_front(peer);
			}

			return 1;
			break;
		}
	}

	return 0;
}

#define MAX_POTENTIAL_PROXIES 10

int bdQuery::trimProxies()
{

	/* drop excess Potential Proxies */
	while(mProxiesUnknown.size() > MAX_POTENTIAL_PROXIES)
	{
		mProxiesUnknown.pop_back();
	}

	while(mProxiesFlagged.size() > MAX_POTENTIAL_PROXIES)
	{
		mProxiesFlagged.pop_back();
	}
	return 1;
}


/*******************************************************************************************
 ******************************** Potential Proxies ****************************************
 *******************************************************************************************/



/* print query.
 */

int     bdQuery::printQuery()
{
#ifdef DEBUG_QUERY 
	fprintf(stderr, "bdQuery::printQuery()\n");
#endif
	
	time_t ts = time(NULL);
	fprintf(stderr, "Query for: ");
	mFns->bdPrintNodeId(std::cerr, &mId);
	fprintf(stderr, " Query State: %d", mState);
	fprintf(stderr, " Query Age %ld secs", ts-mQueryTS);
	if (mState >= BITDHT_QUERY_FAILURE)
	{
		fprintf(stderr, " Search Time: %d secs", mSearchTime);
	}
	fprintf(stderr, "\n");

#ifdef DEBUG_QUERY 
	fprintf(stderr, "Closest Available Peers:\n");
	std::multimap<bdMetric, bdPeer>::iterator it;
	for(it = mClosest.begin(); it != mClosest.end(); it++)
	{
		fprintf(stderr, "Id:  ");
		mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
		fprintf(stderr, "  Bucket: %d ", mFns->bdBucketDistance(&(it->first)));
		fprintf(stderr," Flags: %x", it->second.mPeerFlags);
		fprintf(stderr," Found: %ld ago", ts-it->second.mFoundTime);
		fprintf(stderr," LastSent: %ld ago", ts-it->second.mLastSendTime);
		fprintf(stderr," LastRecv: %ld ago", ts-it->second.mLastRecvTime);
		fprintf(stderr, "\n");
	}

	fprintf(stderr, "\nClosest Potential Peers:\n");
	for(it = mPotentialPeers.begin(); it != mPotentialPeers.end(); it++)
	{
		fprintf(stderr, "Id:  ");
		mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
		fprintf(stderr, "  Bucket: %d ", mFns->bdBucketDistance(&(it->first)));
		fprintf(stderr," Flags: %x", it->second.mPeerFlags);
		fprintf(stderr," Found: %ld ago", ts-it->second.mFoundTime);
		fprintf(stderr," LastSent: %ld ago", ts-it->second.mLastSendTime);
		fprintf(stderr," LastRecv: %ld ago", ts-it->second.mLastRecvTime);
		fprintf(stderr, "\n");
	}
#else
	// shortened version.
	fprintf(stderr, "Closest Available Peer: ");
	std::multimap<bdMetric, bdPeer>::iterator it = mClosest.begin(); 
	if (it != mClosest.end())
	{
		mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
		fprintf(stderr, "  Bucket: %d ", mFns->bdBucketDistance(&(it->first)));
		fprintf(stderr," Flags: %x", it->second.mPeerFlags);
		fprintf(stderr," Found: %ld ago", ts-it->second.mFoundTime);
		fprintf(stderr," LastSent: %ld ago", ts-it->second.mLastSendTime);
		fprintf(stderr," LastRecv: %ld ago", ts-it->second.mLastRecvTime);
	}
	fprintf(stderr, "\n");

	fprintf(stderr, "Closest Potential Peer: ");
	it = mPotentialPeers.begin(); 
	if (it != mPotentialPeers.end())
	{
		mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
		fprintf(stderr, "  Bucket: %d ", mFns->bdBucketDistance(&(it->first)));
		fprintf(stderr," Flags: %x", it->second.mPeerFlags);
		fprintf(stderr," Found: %ld ago", ts-it->second.mFoundTime);
		fprintf(stderr," LastSent: %ld ago", ts-it->second.mLastSendTime);
		fprintf(stderr," LastRecv: %ld ago", ts-it->second.mLastRecvTime);
	}
	fprintf(stderr, "\n");

#endif
	std::list<bdPeer>::iterator lit;
	fprintf(stderr, "Flagged Proxies:\n");
	for(lit = mProxiesFlagged.begin(); lit != mProxiesFlagged.end(); lit++)
	{
		fprintf(stderr, "ProxyId:  ");
		mFns->bdPrintId(std::cerr, &(lit->mPeerId));
		fprintf(stderr," Flags: %x", it->second.mPeerFlags);
		fprintf(stderr," Found: %ld ago", ts-lit->mFoundTime);
		fprintf(stderr," LastSent: %ld ago", ts-lit->mLastSendTime);
		fprintf(stderr," LastRecv: %ld ago", ts-lit->mLastRecvTime);
		fprintf(stderr, "\n");
	}
	
	fprintf(stderr, "Potential Proxies:\n");
	for(lit = mProxiesUnknown.begin(); lit != mProxiesUnknown.end(); lit++)
	{
		fprintf(stderr, "ProxyId:  ");
		mFns->bdPrintId(std::cerr, &(lit->mPeerId));
		fprintf(stderr," Flags: %x", it->second.mPeerFlags);
		fprintf(stderr," Found: %ld ago", ts-lit->mFoundTime);
		fprintf(stderr," LastSent: %ld ago", ts-lit->mLastSendTime);
		fprintf(stderr," LastRecv: %ld ago", ts-lit->mLastRecvTime);
		fprintf(stderr, "\n");
	}

	return 1;
}




/********************************* Remote Query **************************************/

#define QUERY_HISTORY_LIMIT	10 // Typically get max of 4-6 per 10minutes.
#define QUERY_HISTORY_PERIOD	600

bdRemoteQuery::bdRemoteQuery(bdId *id, bdNodeId *query, bdToken *transId, uint32_t query_type)
	:mId(*id), mQuery(*query), mTransId(*transId), mQueryType(query_type)
{
	mQueryTS = time(NULL);
}




bdQueryHistoryList::bdQueryHistoryList()
	:mBadPeer(false)
{

}


bool bdQueryHistoryList::addIncomingQuery(time_t recvd, const bdNodeId *aboutId)
{
	mList.insert(std::make_pair(recvd, *aboutId));
	mBadPeer = (mList.size() > QUERY_HISTORY_LIMIT);
	return mBadPeer;
}


// returns true if empty.
bool bdQueryHistoryList::cleanupMsgs(time_t before)
{
	if (before == 0)
	{
		mList.clear();
		return true;
	}

        // Delete the old stuff in the list.
        while((mList.begin() != mList.end()) && (mList.begin()->first < before))
        {
                mList.erase(mList.begin());
        }

        // return true if empty.
        if (mList.begin() == mList.end())
        {
                return true;
        }
        return false;
}

bdQueryHistory::bdQueryHistory()
 :mStorePeriod(QUERY_HISTORY_PERIOD)
{
	return;
}

bool bdQueryHistory::addIncomingQuery(time_t recvd, const bdId *id, const bdNodeId *aboutId)
{
	std::map<bdId, bdQueryHistoryList>::iterator it;

	it = mHistory.find(*id);
	if (it == mHistory.end())
	{
		mHistory[*id] = bdQueryHistoryList();
		it = mHistory.find(*id);
	}

	return (it->second).addIncomingQuery(recvd, aboutId);
}

bool bdQueryHistory::isBadPeer(const bdId *id)
{
	std::map<bdId, bdQueryHistoryList>::iterator it;

	it = mHistory.find(*id);
	if (it == mHistory.end())
	{
		return false;
	}

	return it->second.mBadPeer;
}


void bdQueryHistory::cleanupOldMsgs()
{
	if (mStorePeriod == 0)
	{
		return; // no cleanup.
	}

	time_t before = time(NULL) - mStorePeriod;
	std::map<bdId, bdQueryHistoryList>::iterator it;
	for(it = mHistory.begin(); it != mHistory.end(); )
	{
		if (it->second.cleanupMsgs(before))
		{
			std::map<bdId, bdQueryHistoryList>::iterator tit(it);
			++tit;
			mHistory.erase(it);
			it = tit;
		}
		else
		{
			++it;
		}
	}
}

	
void bdQueryHistory::printMsgs()
{
	std::ostream &out = std::cerr;

	out << "bdQueryHistory::printMsgs() IncomingQueries in last " << mStorePeriod;
	out << " secs" << std::endl;

	std::map<bdId, bdQueryHistoryList>::iterator it;
	for(it = mHistory.begin(); it != mHistory.end(); it++)
	{
		out << "\t";
		bdStdPrintId(out, &(it->first));
		out << " " << it->second.mList.size();
		if (it->second.mBadPeer)
		{
			out << " BadPeer";
		}
		out << std::endl;
	}
}









