#ifndef BITDHT_QUERY_H
#define BITDHT_QUERY_H

/*
 * bitdht/bdquery.h
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


#include "bitdht/bdiface.h"
#include "bitdht/bdpeer.h"
#include "bitdht/bdobj.h"

/* Query result flags are in bdiface.h */

#define BITDHT_MIN_QUERY_AGE		10
#define BITDHT_MAX_QUERY_AGE		1800 /* 30 minutes */

class bdQuery
{
	public:
	bdQuery(const bdNodeId *id, std::list<bdId> &startList, uint32_t queryFlags, 
		bdDhtFunctions *fns);

	// get the answer.
bool	result(std::list<bdId> &answer);

	// returning results get passed to all queries.
//void 	addNode(const bdId *id, int mode);		
int 	nextQuery(bdId &id, bdNodeId &targetId);
int 	addPeer(const bdId *id, uint32_t mode);
int 	addPotentialPeer(const bdId *id, uint32_t mode);
int 	printQuery();

	// searching for
	bdNodeId mId;
	bdMetric mLimit;
	uint32_t mState;
	time_t mQueryTS;
	uint32_t mQueryFlags;
	int32_t mSearchTime;

	int32_t mQueryIdlePeerRetryPeriod; // seconds between retries.

	private:

	// closest peers
	std::multimap<bdMetric, bdPeer>  mClosest;
	std::multimap<bdMetric, bdPeer>  mPotentialClosest;

	bdDhtFunctions *mFns;
};

class bdQueryStatus
{
	public:
	uint32_t mStatus;
	uint32_t mQFlags;
	std::list<bdId> mResults;
};



/* this is just a container class.
 * we locally seach for this, once then discard.
 */
class bdRemoteQuery
{
	public:
	bdRemoteQuery(bdId *id, bdNodeId *query, bdToken *transId, uint32_t query_type);

	bdId mId;
	bdNodeId mQuery;
	bdToken mTransId;
	uint32_t mQueryType;

	time_t mQueryTS;
};

#endif

