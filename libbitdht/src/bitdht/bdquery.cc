
/*
 * bitdht/bdquery.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
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


#include "bitdht/bdquery.h"
#include "util/bdnet.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream>

/**
 * #define DEBUG_QUERY 1
**/


#define EXPECTED_REPLY 20
#define QUERY_IDLE_RETRY_PEER_PERIOD 300 // 5min =  (mFns->bdNodesPerBucket() * 30)


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

	mQueryIdlePeerRetryPeriod = QUERY_IDLE_RETRY_PEER_PERIOD;

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
	for(; sit != eit; sit++, i++)
	{
		answer.push_back(sit->second.mPeerId);
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
	}

	bool notFinished = false;
	std::multimap<bdMetric, bdPeer>::iterator it;
	for(it = mClosest.begin(); it != mClosest.end(); it++)
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
		 */	
		if (it->second.mLastRecvTime == 0)
		{
#ifdef DEBUG_QUERY 
        		fprintf(stderr, "NextQuery() Never Received: notFinished = true: ");
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
       		fprintf(stderr, "NextQuery() under Min Time: Query not finished / No Query\n");
#endif
		/* fall through and stop */
	}
	else if ((mClosest.size() < mFns->bdNodesPerBucket()) || (notFinished))
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

	/* check if we found the node */
	if (mClosest.size() > 0)
	{
		if ((mClosest.begin()->second).mPeerId.id == mId)
		{
			mState = BITDHT_QUERY_SUCCESS;
		}
		else if ((mPotentialClosest.begin()->second).mPeerId.id == mId)
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

int bdQuery::addPeer(const bdId *id, uint32_t mode)
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
	for(it = mClosest.begin(); it != sit; it++, i++, actualCloser++)
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

	if (i > mFns->bdNodesPerBucket() - 1)
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
		if (it->second.mPeerId == *id)
		{
#ifdef DEBUG_QUERY 
        		fprintf(stderr, "Peer Already here!\n");
#endif
			if (mode & BITDHT_PEER_STATUS_RECV_NODES)
			{
				/* only update recvTime if sendTime > checkTime.... (then its our query) */
#ifdef DEBUG_QUERY 
				fprintf(stderr, "Updating LastRecvTime\n");
#endif
				it->second.mLastRecvTime = ts;
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
		bool removed = false;
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
				removed = true;
				break ;
			}
		}
	}

	/* trim it back */
	while(mClosest.size() > (uint32_t) (mFns->bdNodesPerBucket() - 1))
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


/* we also want to track unreachable node ... this allows us
 * to detect if peer are online - but uncontactible by dht.
 * 
 * simple list of closest.
 */

int bdQuery::addPotentialPeer(const bdId *id, uint32_t mode)
{
	bdMetric dist;
	time_t ts = time(NULL);

	mFns->bdDistance(&mId, &(id->id), &dist);

#ifdef DEBUG_QUERY 
        fprintf(stderr, "bdQuery::addPotentialPeer(");
	mFns->bdPrintId(std::cerr, id);
        fprintf(stderr, ", %u)\n", mode);
#endif

	/* first we check if this is a worthy potential peer....
	 * if it is already in mClosest -> false. old peer.
	 * if it is > mClosest.rbegin() -> false. too far way.
	 */
	int retval = 1;

	std::multimap<bdMetric, bdPeer>::iterator it, sit, eit;
	sit = mClosest.lower_bound(dist);
	eit = mClosest.upper_bound(dist);

	for(it = sit; it != eit; it++)
	{
		if (it->second.mPeerId == *id)
		{
			/* already there */
			retval = 0;
#ifdef DEBUG_QUERY 
			fprintf(stderr, "Peer already in mClosest\n");
#endif
		}
		//empty loop.
	}

	/* check if outside range, & bucket is full  */
	if ((sit == mClosest.end()) && (mClosest.size() >= mFns->bdNodesPerBucket()))
	{
#ifdef DEBUG_QUERY 
		fprintf(stderr, "Peer to far away for Potential\n");
#endif
		retval = 0; /* too far way */
	}

	/* return if false; */
	if (!retval)
	{
#ifdef DEBUG_QUERY 
		fprintf(stderr, "Flagging as Not a Potential Peer!\n");
#endif
		return retval;
	}

	/* finally if a worthy & new peer -> add into potential closest
	 * and repeat existance tests with PotentialPeers
	 */

	sit = mPotentialClosest.lower_bound(dist);
	eit = mPotentialClosest.upper_bound(dist);
	int i = 0;
	for(it = mPotentialClosest.begin(); it != sit; it++, i++)
	{
		//empty loop.
	}

	if (i > mFns->bdNodesPerBucket() - 1)
	{
#ifdef DEBUG_QUERY 
        	fprintf(stderr, "Distance to far... dropping\n");
        	fprintf(stderr, "Flagging as Potential Peer!\n");
#endif
		/* outside the list - so we won't add to mPotentialClosest 
		 * but inside mClosest still - so should still try it 
		 */
		retval = 1;
		return retval;
	}

	for(it = sit; it != eit; it++, i++)
	{
		if (it->second.mPeerId == *id)
		{
			/* this means its already been pinged */
#ifdef DEBUG_QUERY 
        		fprintf(stderr, "Peer Already here in mPotentialClosest!\n");
#endif
			if (mode & BITDHT_PEER_STATUS_RECV_NODES)
			{
#ifdef DEBUG_QUERY 
        			fprintf(stderr, "Updating LastRecvTime\n");
#endif
				it->second.mLastRecvTime = ts;
			}
#ifdef DEBUG_QUERY 
        		fprintf(stderr, "Flagging as Not a Potential Peer!\n");
#endif
			retval = 0;
			return retval;
		}
	}

#ifdef DEBUG_QUERY 
        fprintf(stderr, "Peer not in Query\n");
#endif


	/* trim it back */
	while(mPotentialClosest.size() > (uint32_t) (mFns->bdNodesPerBucket() - 1))
	{
		std::multimap<bdMetric, bdPeer>::iterator it;
		it = mPotentialClosest.end();

		if(!mPotentialClosest.empty())
		{
			--it;
#ifdef DEBUG_QUERY 
			fprintf(stderr, "Removing Furthest Peer: ");
			mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
			fprintf(stderr, "\n");
#endif
			mPotentialClosest.erase(it);
		}
	}

#ifdef DEBUG_QUERY 
        fprintf(stderr, "bdQuery::addPotentialPeer(): Closer Peer!: ");
	mFns->bdPrintId(std::cerr, id);
        fprintf(stderr, "\n");
#endif

	/* add it in */
	bdPeer peer;
	peer.mPeerId = *id;
	peer.mLastSendTime = 0;
	peer.mLastRecvTime = ts;
	peer.mFoundTime = ts;
	mPotentialClosest.insert(std::pair<bdMetric, bdPeer>(dist, peer));

#ifdef DEBUG_QUERY 
	fprintf(stderr, "Flagging as Potential Peer!\n");
#endif
	retval = 1;
	return retval;
}

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
		fprintf(stderr," Found: %ld ago", ts-it->second.mFoundTime);
		fprintf(stderr," LastSent: %ld ago", ts-it->second.mLastSendTime);
		fprintf(stderr," LastRecv: %ld ago", ts-it->second.mLastRecvTime);
		fprintf(stderr, "\n");
	}

	fprintf(stderr, "\nClosest Potential Peers:\n");
	for(it = mPotentialClosest.begin(); it != mPotentialClosest.end(); it++)
	{
		fprintf(stderr, "Id:  ");
		mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
		fprintf(stderr, "  Bucket: %d ", mFns->bdBucketDistance(&(it->first)));
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
		fprintf(stderr," Found: %ld ago", ts-it->second.mFoundTime);
		fprintf(stderr," LastSent: %ld ago", ts-it->second.mLastSendTime);
		fprintf(stderr," LastRecv: %ld ago", ts-it->second.mLastRecvTime);
	}
	fprintf(stderr, "\n");

	fprintf(stderr, "Closest Potential Peer: ");
	it = mPotentialClosest.begin(); 
	if (it != mPotentialClosest.end())
	{
		mFns->bdPrintId(std::cerr, &(it->second.mPeerId));
		fprintf(stderr, "  Bucket: %d ", mFns->bdBucketDistance(&(it->first)));
		fprintf(stderr," Found: %ld ago", ts-it->second.mFoundTime);
		fprintf(stderr," LastSent: %ld ago", ts-it->second.mLastSendTime);
		fprintf(stderr," LastRecv: %ld ago", ts-it->second.mLastRecvTime);
	}
	fprintf(stderr, "\n");
#endif

	return 1;
}




/********************************* Remote Query **************************************/
bdRemoteQuery::bdRemoteQuery(bdId *id, bdNodeId *query, bdToken *transId, uint32_t query_type)
	:mId(*id), mQuery(*query), mTransId(*transId), mQueryType(query_type)
{
	mQueryTS = time(NULL);
}





