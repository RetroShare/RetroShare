#ifndef BITDHT_PEER_H
#define BITDHT_PEER_H

/*
 * bitdht/bdpeer.h
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
#include <time.h>

/*******
 * These type of parameters are now DHT Function dependent
 *
#define BITDHT_BUCKET_SIZE 20
#define BITDHT_BUCKET_SIZE_BITS 5

#define BITDHT_N_BUCKETS  BITDHT_KEY_BITLEN
 *
 *
 ***/

/*** 
 * DEFINED in bdiface.h
 * #define BITDHT_KEY_LEN 20
 * #define BITDHT_KEY_INTLEN 5
 * #define BITDHT_KEY_BITLEN 160
 ***/

#define BITDHT_ULLONG_BITS 64

#define BITDHT_MAX_SEND_PERIOD	600   // retry every 10 secs.
#define BITDHT_MAX_RECV_PERIOD	1500   // out-of-date


#include <list>
#include <string>
#include <map>
#include <vector>

/****
 * DEFINED in bdiface.h
 *
 * class bdNodeId
 * {
 * 	public:
 *	unsigned char data[BITDHT_KEY_LEN];
 * };
 ****/


/****
 * DEFINED in bdiface.h
 *

class bdMetric: public bdNodeId {};

class bdId
{
	public:

	bdId();
	bdId(bdNodeId in_id, struct sockaddr_in in_addr);

	struct sockaddr_in addr;
	bdNodeId id;
};
 *
 *********/



//void bdRandomNodeId(bdNodeId *id);

// Only Functions that are common for all Dhts.
// zero, basic comparisons..
void bdZeroNodeId(bdNodeId *id);

//void bdRandomId(bdId *id);
//int bdDistance(const bdNodeId *a, const bdNodeId *b, bdMetric *r);
//int bdBucketDistance(const bdMetric *m);
//int bdBucketDistance(const bdNodeId *a, const bdNodeId *b);
//int operator<(const bdMetric &a, const bdMetric &b);

//int operator<(const struct sockaddr_in &a, const struct sockaddr_in &b);

int operator<(const bdNodeId &a, const bdNodeId &b);
int operator<(const bdId &a, const bdId &b);
int operator==(const bdNodeId &a, const bdNodeId &b);
int operator==(const bdId &a, const bdId &b);

//void bdRandomMidId(const bdNodeId *target, const bdNodeId *other, bdNodeId *mid);

//void bdPrintId(std::ostream &out, const bdId *a);
//void bdPrintNodeId(std::ostream &out, const bdNodeId *a);

//std::string bdConvertToPrintable(std::string input);

class bdPeer
{
	public:

	bdId   mPeerId;
	uint32_t mPeerFlags;
	time_t mLastSendTime;
	time_t mLastRecvTime;
	time_t mFoundTime;     /* time stamp that peer was found */
};




class bdBucket
{
	public:

	bdBucket();

	/* list so we can queue properly */
	std::list<bdPeer> entries;
};

class bdSpace
{
	public:

	bdSpace(bdNodeId *ownId, bdDhtFunctions *fns);

int	clear();

	/* accessors */
int 	find_nearest_nodes(const bdNodeId *id, int number, 
		std::list<bdId> excluding, std::multimap<bdMetric, bdId> &nearest);

int	out_of_date_peer(bdId &id); // side-effect updates, send flag on peer.
int     add_peer(const bdId *id, uint32_t mode);
int     printDHT();

uint32_t calcNetworkSize();
uint32_t calcNetworkSizeWithFlag(uint32_t withFlag);
uint32_t calcSpaceSize();

	/* to add later */
int	updateOwnId(bdNodeId *newOwnId);

	private:

	std::vector<bdBucket> buckets;
	bdNodeId mOwnId;
	bdDhtFunctions *mFns;
};


#endif

