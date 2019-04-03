/*******************************************************************************
 * bitdht/bdpeer.h                                                             *
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
#ifndef BITDHT_PEER_H
#define BITDHT_PEER_H

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

#define BITDHT_MAX_RESPONSE_PERIOD      (15)
#define BITDHT_MAX_SEND_PERIOD	300   // 5 minutes.
#define BITDHT_MAX_RECV_PERIOD	(BITDHT_MAX_SEND_PERIOD + BITDHT_MAX_RESPONSE_PERIOD) // didn't respond to a ping.

// Properly out of date.
#define BITDHT_DISCARD_PERIOD		(2 * BITDHT_MAX_SEND_PERIOD + BITDHT_MAX_RESPONSE_PERIOD) // didn't respond to two pings.

// Must have a FLAG by this time. (Make it really quick - so we through away the rubbish).


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

/****
 * DEFINED in bdiface.h
 *

class bdPeer
{
	public:

	bdId   mPeerId;
	uint32_t mPeerFlags;
	time_t mLastSendTime;
	time_t mLastRecvTime;
	time_t mFoundTime;     // time stamp that peer was found 
};


class bdBucket
{
	public:

	bdBucket();

	// list so we can queue properly 
	std::list<bdPeer> entries;
};
 *
 *
 *****/

class bdSpace
{
	public:

	bdSpace(bdNodeId *ownId, bdDhtFunctions *fns);

int	clear();

int	setAttachedFlag(uint32_t withflags, int count);

	/* accessors */
int 	find_nearest_nodes(const bdNodeId *id, int number, 
		std::multimap<bdMetric, bdId> &nearest);

int 	find_nearest_nodes_with_flags(const bdNodeId *id, int number, 
		std::list<bdId> excluding, 
		std::multimap<bdMetric, bdId> &nearest, uint32_t with_flag);

int 	find_node(const bdNodeId *id, int number, 
		std::list<bdId> &matchIds, uint32_t with_flag);
int 	find_exactnode(const bdId *id, bdPeer &peer);

// switched to more efficient single sweep.
//int	out_of_date_peer(bdId &id); // side-effect updates, send flag on peer.
int     scanOutOfDatePeers(std::list<bdId> &peerIds);
int     updateAttachedPeers();

int     add_peer(const bdId *id, uint32_t mode);
int     printDHT();
int     getDhtBucket(const int idx, bdBucket &bucket);

uint32_t calcNetworkSize();
uint32_t calcNetworkSizeWithFlag(uint32_t withFlag);
uint32_t calcNetworkSizeWithFlag_old(uint32_t withFlag);
uint32_t calcSpaceSize();
uint32_t calcSpaceSizeWithFlag(uint32_t withFlag);

        /* special function to enable DHT localisation (i.e find peers from own network) */
bool 	findRandomPeerWithFlag(bdId &id, uint32_t withFlag);

	/* strip out flags - to switch in/out of relay mode */
int 	clean_node_flags(uint32_t flags);

	/* to add later */
int	updateOwnId(bdNodeId *newOwnId);

	/* flag peer */
bool	flagpeer(const bdId *id, uint32_t flags, uint32_t ex_flags);

	private:

	std::vector<bdBucket> buckets;
	bdNodeId mOwnId;
	bdDhtFunctions *mFns;

	uint32_t mAttachedFlags;
	uint32_t mAttachedCount;
	time_t   mAttachTS;
};


#endif

