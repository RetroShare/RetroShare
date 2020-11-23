/*******************************************************************************
 * bitdht/bdpeer.cc                                                            *
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

#include "bitdht/bdpeer.h"
#include "util/bdnet.h"
#include "util/bdrandom.h"
#include "util/bdstring.h"
#include "bitdht/bdiface.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <iomanip>

/**
 * #define BITDHT_DEBUG 1
**/

void bdSockAddrInit(struct sockaddr_in *addr)
{
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
}

bdId::bdId()
{
	/* blank everything */
	bdSockAddrInit(&addr);
	memset(&id.data, 0, BITDHT_KEY_LEN);
}


bdId::bdId(bdNodeId in_id, struct sockaddr_in in_addr)
{
	/* this bit is to ensure the address is valid for windows / osx */
	bdSockAddrInit(&addr);
	addr.sin_addr.s_addr = in_addr.sin_addr.s_addr;
	addr.sin_port = in_addr.sin_port;

	for(int i = 0; i < BITDHT_KEY_LEN; i++)
	{
		id.data[i] = in_id.data[i];
	}
};


void bdZeroNodeId(bdNodeId *id)
{
	uint32_t *a_data = (uint32_t *) id->data;
	for(int i = 0; i < BITDHT_KEY_INTLEN; i++)
	{
		a_data[i] = 0;
	}
	return;
}


int operator<(const bdNodeId &a, const bdNodeId &b)
{
#if 0
	std::cerr <<  "operator<(");
	bdPrintNodeId(std::cerr, &a);
	std::cerr <<  ",";
	bdPrintNodeId(std::cerr, &b);
	std::cerr <<  ")" << std::endl;
#endif
	
	uint8_t *a_data = (uint8_t *) a.data;
	uint8_t *b_data = (uint8_t *) b.data;
	for(int i = 0; i < BITDHT_KEY_LEN; i++)	
	{
		if (*a_data < *b_data)
		{
			//fprintf(stderr, "Return 1, at i = %d\n", i);
			return 1;
		}
		else if (*a_data > *b_data)
		{
			//fprintf(stderr, "Return 0, at i = %d\n", i);
			return 0;
		}
		a_data++;
		b_data++;
	}
	//fprintf(stderr, "Return 0, at i = KEYLEN\n");
	return 0;
}

#if 0
int operator<(const struct sockaddr_in &a, const struct sockaddr_in &b)
{
	/* else NodeIds the same - check id addresses */
	if (a.sin_addr.s_addr < b.sin_addr.s_addr)
		return 1;
	if (b.sin_addr.s_addr > a.sin_addr.s_addr)
		return 0;

	if (a.sin_port < b.sin_port)
		return 1;

	return 0;
}
#endif

int operator<(const bdId &a, const bdId &b)
{
	if (a.id < b.id)
		return 1;
	if (b.id < a.id)
		return 0;

	/* else NodeIds the same - check id addresses */
	if (a.addr.sin_addr.s_addr < b.addr.sin_addr.s_addr)
		return 1;
	if (b.addr.sin_addr.s_addr > a.addr.sin_addr.s_addr)
		return 0;

	if (a.addr.sin_port < b.addr.sin_port)
		return 1;

	return 0;
}


int operator==(const bdNodeId &a, const bdNodeId &b)
{
	uint8_t *a_data = (uint8_t *) a.data;
	uint8_t *b_data = (uint8_t *) b.data;
	for(int i = 0; i < BITDHT_KEY_LEN; i++)	
	{
		if (*a_data < *b_data)
		{
			return 0;
		}
		else if (*a_data > *b_data)
		{
			return 0;
		}
		a_data++;
		b_data++;
	}
	return 1;
}

int operator==(const bdId &a, const bdId &b)
{
	if (!(a.id == b.id))
		return 0;

	if ((a.addr.sin_addr.s_addr == b.addr.sin_addr.s_addr) &&
	    (a.addr.sin_port == b.addr.sin_port))
	{
		return 1;
	}
	return 0;
}



bdBucket::bdBucket()
{
	return;
}

bdSpace::bdSpace(bdNodeId *ownId, bdDhtFunctions *fns)
	:mOwnId(*ownId), mFns(fns)
{
	/* make some space for data */
	buckets.resize(mFns->bdNumBuckets());

	mAttachTS = 0;
	mAttachedFlags = 0;
	mAttachedCount = 0;

	return;
}

	/* empty the buckets */
int     bdSpace::clear()
{
	std::vector<bdBucket>::iterator it;
        /* iterate through the buckets, and sort by distance */
        for(it = buckets.begin(); it != buckets.end(); it++)
	{
		it->entries.clear();
	}
	return 1;
}

int     bdSpace::setAttachedFlag(uint32_t withflags, int count)
{
	mAttachedFlags = withflags;
	mAttachedCount = count;
	mAttachTS = 0;
	return 1;
}

int bdSpace::find_nearest_nodes_with_flags(const bdNodeId *id, int number, 
		std::list<bdId> /* excluding */, 
		std::multimap<bdMetric, bdId> &nearest, uint32_t with_flags)
{
	std::multimap<bdMetric, bdId> closest;
	std::multimap<bdMetric, bdId>::iterator mit;

	bdMetric dist;
	mFns->bdDistance(id, &(mOwnId), &dist);

#ifdef DEBUG_BD_SPACE
	int bucket = mFns->bdBucketDistance(&dist);

	std::cerr << "bdSpace::find_nearest_nodes(NodeId:";
	mFns->bdPrintNodeId(std::cerr, id);

	std::cerr << " Number: " << number;
	std::cerr << " Query Bucket #: " << bucket;
	std::cerr << std::endl;
#endif

	std::vector<bdBucket>::iterator it;
	std::list<bdPeer>::iterator eit;
	/* iterate through the buckets, and sort by distance */
	for(it = buckets.begin(); it != buckets.end(); it++)
	{
		for(eit = it->entries.begin(); eit != it->entries.end(); eit++) 
		{
			if ((!with_flags) || ((with_flags & eit->mPeerFlags) == with_flags))
			{
			  mFns->bdDistance(id, &(eit->mPeerId.id), &dist);
			  closest.insert(std::pair<bdMetric, bdId>(dist, eit->mPeerId));

#if 0
			  std::cerr << "Added NodeId: ";
			  bdPrintNodeId(std::cerr, &(eit->mPeerId.id));
			  std::cerr << " Metric: ";
			  bdPrintNodeId(std::cerr, &(dist));
			  std::cerr << std::endl;
#endif
			}
		}
	}

	/* take the first number of nodes */
	int i = 0;
	for(mit = closest.begin(); (mit != closest.end()) && (i < number); mit++, i++)
	{
		mFns->bdDistance(&(mOwnId), &(mit->second.id), &dist);

#ifdef DEBUG_BD_SPACE
		int iBucket = mFns->bdBucketDistance(&(mit->first));

		std::cerr << "Closest " << i << ": ";
		mFns->bdPrintNodeId(std::cerr, &(mit->second.id));
		std::cerr << " Bucket:        " << iBucket;
		std::cerr << std::endl;
#endif


#if 0
		std::cerr << "\tNodeId: ";
		mFns->bdPrintNodeId(std::cerr, &(mit->second.id));
		std::cerr << std::endl;

		std::cerr << "\tOwn Id: ";
		mFns->bdPrintNodeId(std::cerr, &(mOwnId));
		std::cerr << std::endl;

		std::cerr << "     Us Metric: ";
		mFns->bdPrintNodeId(std::cerr, &dist);
		std::cerr << " Bucket:        " << oBucket;
		std::cerr << std::endl;

		std::cerr << "\tFindId: ";
		mFns->bdPrintNodeId(std::cerr, id);
		std::cerr << std::endl;

		std::cerr << "     Id Metric: ";
		mFns->bdPrintNodeId(std::cerr, &(mit->first));
		std::cerr << " Bucket:        " << iBucket;
		std::cerr << std::endl;
#endif

		nearest.insert(*mit);
	}

#ifdef DEBUG_BD_SPACE
	std::cerr << "#Nearest: " << (int) nearest.size();
	std::cerr << " #Closest: " << (int) closest.size();
	std::cerr << " #Requested: " << number;
	std::cerr << std::endl << std::endl;
#endif

	return 1;
}

int bdSpace::find_nearest_nodes(const bdNodeId *id, int number,
                		std::multimap<bdMetric, bdId> &nearest)
{
	std::list<bdId> excluding;
	uint32_t with_flag = 0;

	return find_nearest_nodes_with_flags(id, number, excluding, nearest, with_flag);
}


/* This is much cheaper than find nearest... we only look in the one bucket
 */

int bdSpace::find_node(const bdNodeId *id, int number, std::list<bdId> &matchIds, uint32_t with_flags)
{
	bdMetric dist;
	mFns->bdDistance(id, &(mOwnId), &dist);
	int buckno = mFns->bdBucketDistance(&dist);

#ifdef DEBUG_BD_SPACE
    std::cerr << "bdSpace::find_node(NodeId:";
	mFns->bdPrintNodeId(std::cerr, id);
	std::cerr << ")";

	std::cerr << " Number: " << number;
	std::cerr << " Bucket #: " << buckno;
	std::cerr << std::endl;
#else
	(void)number;
#endif

	bdBucket &buck = buckets[buckno];

	std::list<bdPeer>::iterator eit;
	int matchCount = 0;
	for(eit = buck.entries.begin(); eit != buck.entries.end(); eit++) 
	{
#ifdef DEBUG_BD_SPACE
        std::cerr << "bdSpace::find_node() Checking Against Peer: ";
		mFns->bdPrintId(std::cerr, &(eit->mPeerId));
		std::cerr << " withFlags: " << eit->mPeerFlags;
        std::cerr << std::endl;
#endif

		if ((!with_flags) || ((with_flags & eit->mPeerFlags) == with_flags))
		{
			if (*id == eit->mPeerId.id)
			{
		  		matchIds.push_back(eit->mPeerId);
				matchCount++;

#ifdef DEBUG_BD_SPACE
                std::cerr << "bdSpace::find_node() Found Matching Peer: ";
			  	mFns->bdPrintId(std::cerr, &(eit->mPeerId));
				std::cerr << " withFlags: " << eit->mPeerFlags;
                std::cerr << std::endl;
#endif
			}
		}
		else
		{
			if (*id == eit->mPeerId.id)
			{
		  		//matchIds.push_back(eit->mPeerId);
				//matchCount++;

#ifdef DEBUG_BD_SPACE
                std::cerr << "bdSpace::find_node() Found (WITHOUT FLAGS) Matching Peer: ";
			  	mFns->bdPrintId(std::cerr, &(eit->mPeerId));
				std::cerr << " withFlags: " << eit->mPeerFlags;
                std::cerr << std::endl;
#endif
			}
		}
	}

#ifdef DEBUG_BD_SPACE
    std::cerr << "bdSpace::find_node() Found " << matchCount << " Matching Peers";
	std::cerr << std::endl << std::endl;
#endif

	return matchCount;
}

/* even cheaper again... no big lists */
int bdSpace::find_exactnode(const bdId *id, bdPeer &peer)
{
	bdMetric dist;
	mFns->bdDistance(&(id->id), &(mOwnId), &dist);
	int buckno = mFns->bdBucketDistance(&dist);

#ifdef DEBUG_BD_SPACE
	std::cerr << "bdSpace::find_exactnode(Id:";
	mFns->bdPrintId(std::cerr, id);
	std::cerr << ")";

	std::cerr << " Bucket #: " << buckno;
	std::cerr << std::endl;
#endif

	bdBucket &buck = buckets[buckno];

	std::list<bdPeer>::iterator eit;
	for(eit = buck.entries.begin(); eit != buck.entries.end(); eit++) 
	{
		if (*id == eit->mPeerId)
		{
#ifdef DEBUG_BD_SPACE
			std::cerr << "bdSpace::find_exactnode() Found Matching Peer: ";
		  	mFns->bdPrintId(std::cerr, &(eit->mPeerId));
			std::cerr << " withFlags: " << eit->mPeerFlags;
		  	std::cerr << std::endl;
#endif

			peer = (*eit);
			return 1;
		}
	}
#ifdef DEBUG_BD_SPACE
	std::cerr << "bdSpace::find_exactnode() WARNING Failed to find Matching Peer: ";
	std::cerr << std::endl;
#endif

	return 0;
}



int bdSpace::clean_node_flags(uint32_t flags)
{
	std::cerr << "bdSpace::clean_node_flags(" << flags << ")";
	std::cerr << std::endl;

	int count = 0;
	std::vector<bdBucket>::iterator bit;
	for(bit = buckets.begin(); bit != buckets.end(); bit++)
	{
		std::list<bdPeer>::iterator eit;
		for(eit = bit->entries.begin(); eit != bit->entries.end(); eit++) 
		{
			if (flags & eit->mPeerFlags)
			{	
#ifdef DEBUG_BD_SPACE
                std::cerr << "bdSpace::clean_node_flags() Found Match: ";
		  		mFns->bdPrintId(std::cerr, &(eit->mPeerId));
				std::cerr << " withFlags: " << eit->mPeerFlags;
                std::cerr << std::endl;
#endif

				count++;
				eit->mPeerFlags &= ~flags;
			}
		}
	}
#ifdef DEBUG_BD_SPACE
    std::cerr << "bdSpace::clean_node_flags() Cleaned " << count << " Matching Peers";
    std::cerr << std::endl;
#endif

	return count;
}



#define BITDHT_ATTACHED_SEND_PERIOD 	17

int	bdSpace::scanOutOfDatePeers(std::list<bdId> &peerIds)
{
	/* 
	 * 
	 */
	bool doAttached = (mAttachedCount > 0);
	uint32_t attachedCount = 0;

	std::map<bdMetric, bdId> closest;
	std::map<bdMetric, bdId>::iterator mit;

	std::vector<bdBucket>::iterator it;
	std::list<bdPeer>::iterator eit;
	time_t ts = time(NULL);

	/* iterate through the buckets, and sort by distance */
	for(it = buckets.begin(); it != buckets.end(); it++)
	{
		for(eit = it->entries.begin(); eit != it->entries.end(); ) 
		{
			bool added = false;
			if (doAttached)
			{
				if (eit->mExtraFlags & BITDHT_PEER_EXFLAG_ATTACHED)
				{
					/* add to send list, if we haven't pinged recently */
					if ((ts - eit->mLastSendTime > BITDHT_ATTACHED_SEND_PERIOD ) &&
						(ts - eit->mLastRecvTime > BITDHT_ATTACHED_SEND_PERIOD ))
					{
						peerIds.push_back(eit->mPeerId);
						eit->mLastSendTime = ts;
						added = true;
					}
					attachedCount++;
				}
			}
				

			/* timeout on last send time! */
			if ((!added) && (ts - eit->mLastSendTime > BITDHT_MAX_SEND_PERIOD ))
			{
				/* We want to ping a peer iff:
		 	 	 * 1) They are out-of-date: mLastRecvTime is too old.
			 	 * 2) They don't have 0x0001 flag (we haven't received a PONG) and never sent.
			 	 */
				if ((ts - eit->mLastRecvTime > BITDHT_MAX_SEND_PERIOD ) || 
					!(eit->mPeerFlags & BITDHT_PEER_STATUS_RECV_PONG))
				{
					peerIds.push_back(eit->mPeerId);
					eit->mLastSendTime = ts;
				}
			}


			/* we also want to remove very old entries (should it happen here?) 
			 * which are not pushed out by newer entries (will happen in for closer buckets)
			 */

			bool discard = false;
			/* discard very old entries */
			if (ts - eit->mLastRecvTime > BITDHT_DISCARD_PERIOD)
			{
				discard = true;
			}
		
			/* discard peers which have not responded to anything (ie have no flags set) */
			/* changed into have not id'ed themselves, as we've added ping to list of flags. */
			if ((ts - eit->mFoundTime > BITDHT_MAX_RESPONSE_PERIOD ) &&
				!(eit->mPeerFlags & BITDHT_PEER_STATUS_RECV_PONG))
			{
				discard = true;
			}
			

			/* INCREMENT */
			if (discard)
			{	
				eit = it->entries.erase(eit);
			}
			else
			{
				eit++;
			}
		}
	}

#define ATTACH_UPDATE_PERIOD	600

	if ((ts - mAttachTS > ATTACH_UPDATE_PERIOD) || (attachedCount != mAttachedCount))
	{
		//std::cerr << "Updating ATTACH Stuff";
		//std::cerr << std::endl;
		updateAttachedPeers(); /* XXX TEMP HACK to look at stability */
		mAttachTS = ts;
	}

	return (peerIds.size());
}


/* Attached should be changed to preferentially choose the ones we want 
 * If we have RELAYS enabled - then we want them to attach to the RELAYS
 * Not sure about this yet!
 *
 * Have to think about it a bit more. (and redesign code to handle multiple passes)
 */

int	bdSpace::updateAttachedPeers()
{
	/* 
	 * 
	 */
	bool doAttached = (mAttachedCount > 0);
	uint32_t attachedCount = 0;

	// Must scan through - otherwise we can never remove Attached state.
	// It is only once every 10 minutes or so!
#if 0
	if (!doAttached)
	{
		return 0;
	}
#endif

	std::map<bdMetric, bdId> closest;
	std::map<bdMetric, bdId>::iterator mit;

	std::vector<bdBucket>::iterator it;
	std::list<bdPeer>::reverse_iterator eit;


	/* skip the first bucket, as we don't want to ping ourselves! */	
	it = buckets.begin();
	if (it != buckets.end())
	{
		it++;
	}

	/* iterate through the buckets (sorted by distance) */
	for(; it != buckets.end(); it++)
	{
		/* start from the back, as these are the most recently seen (and more likely to be the old ATTACHED) */
		for(eit = it->entries.rbegin(); eit != it->entries.rend(); eit++) 
		{
			if (doAttached)
			{
				if ((eit->mPeerFlags & mAttachedFlags) == mAttachedFlags)
				{
					/* flag as attached */
					eit->mExtraFlags |= BITDHT_PEER_EXFLAG_ATTACHED;

					/* inc count, and cancel search if we've found them */
					attachedCount++;
					if (attachedCount >= mAttachedCount)
					{
						doAttached = false;
					}
				}
				else
				{
					eit->mExtraFlags &= ~BITDHT_PEER_EXFLAG_ATTACHED;
				}
			}
			else
			{
				eit->mExtraFlags &= ~BITDHT_PEER_EXFLAG_ATTACHED;
			}
		}
	}
	return 1;
}




/* Called to add or update peer.
 * sorts bucket lists by lastRecvTime.
 * updates requested node.
 */

/* peer flags
 * order is important!
 * higher bits = more priority.
 * BITDHT_PEER_STATUS_RECVPING
 * BITDHT_PEER_STATUS_RECVPONG
 * BITDHT_PEER_STATUS_RECVNODES
 * BITDHT_PEER_STATUS_RECVHASHES
 * BITDHT_PEER_STATUS_DHT_ENGINE  (dbXXxx)
 * BITDHT_PEER_STATUS_DHT_APPL    (XXRSxx)
 * BITDHT_PEER_STATUS_DHT_VERSION (XXxx50)
 * 
 */

int     bdSpace::add_peer(const bdId *id, uint32_t peerflags)
{
	/* find the peer */
	bool add = false;
	time_t ts = time(NULL);
	
#ifdef DEBUG_BD_SPACE
	fprintf(stderr, "bdSpace::add_peer()\n");
#endif

	/* calculate metric */
	bdMetric met;
	mFns->bdDistance(&(mOwnId), &(id->id), &met);
	int bucket = mFns->bdBucketDistance(&met);

#ifdef DEBUG_BD_SPACE
	fprintf(stderr, "peer:");
	mFns->bdPrintId(std::cerr, id);	
	fprintf(stderr, " bucket: %d", bucket);
	fprintf(stderr, "\n");
#endif

	/* select correct bucket */
	bdBucket &buck =  buckets[bucket];


	std::list<bdPeer>::iterator it;

	/* calculate the score for this new peer */
	uint32_t minScore = peerflags;

	/* loop through ids, to find it */
	for(it = buck.entries.begin(); it != buck.entries.end(); it++)
	{
                /* similar id check */
                if (mFns->bdSimilarId(id, &(it->mPeerId)))
                {
			bdPeer peer = *it;
			it = buck.entries.erase(it);

			peer.mLastRecvTime = ts;
			peer.mPeerFlags |= peerflags; /* must be cumulative ... so can do online, replynodes, etc */

			/* also update port from incoming id, as we have definitely recved from it */
			if (mFns->bdUpdateSimilarId(&(peer.mPeerId), id))
			{
				/* updated it... must be Unstable */
				peer.mExtraFlags |= BITDHT_PEER_EXFLAG_UNSTABLE;
			}

			buck.entries.push_back(peer);

#ifdef DEBUG_BD_SPACE
			std::cerr << "Peer already in bucket: moving to back of the list" << std::endl;
#endif

			return 1;
		}
		
		/* find lowest score */
		if (it->mPeerFlags < minScore)
		{
			minScore = it->mPeerFlags;
		}
	}

	/* not in the list! */

	if (buck.entries.size() < mFns->bdNodesPerBucket())
	{
#ifdef DEBUG_BD_SPACE
		std::cerr << "Bucket not full: allowing add" << std::endl;
#endif
		add = true;
	}
	else 
	{
		/* check head of list */
		bdPeer &peer = buck.entries.front();
		if (ts - peer.mLastRecvTime >  BITDHT_MAX_RECV_PERIOD)
		{
#ifdef DEBUG_BD_SPACE
			std::cerr << "Dropping Out-of-Date peer in bucket" << std::endl;
#endif
			buck.entries.pop_front();
			add = true;
		}
		else if (peerflags > minScore)
		{
			/* find one to drop */
			for(it = buck.entries.begin(); it != buck.entries.end(); it++)
			{
				if (it->mPeerFlags == minScore)
				{
					/* delete low priority peer */
					it = buck.entries.erase(it);
					add = true;
					break;
				}
			}

#ifdef DEBUG_BD_SPACE
			std::cerr << "Inserting due to Priority: minScore: " << minScore 
				<< " new Peer Score: " << peerscore <<  << std::endl;
#endif
		}
		else
		{
#ifdef DEBUG_BD_SPACE
			std::cerr << "No Out-Of-Date peers in bucket... dropping new entry" << std::endl;
#endif
		}
	}

	if (add)
	{
		bdPeer newPeer;

		newPeer.mPeerId = *id;
		newPeer.mLastRecvTime = ts;
		newPeer.mLastSendTime = 0; // ts; //????
		newPeer.mFoundTime = ts;
		newPeer.mPeerFlags = peerflags;
		newPeer.mExtraFlags = 0;

		buck.entries.push_back(newPeer);

#ifdef DEBUG_BD_SPACE
		/* useful debug */
		std::cerr << "bdSpace::add_peer() Added Bucket[";
		std::cerr << bucket << "] Entry: ";
		mFns->bdPrintId(std::cerr, id);
		std::cerr << std::endl;
#endif
	}
	return add;
}

        /* flag peer */
bool    bdSpace::flagpeer(const bdId *id, uint32_t flags, uint32_t ex_flags)
{
#ifdef DEBUG_BD_SPACE
	fprintf(stderr, "bdSpace::flagpeer()\n");
#endif

	/* calculate metric */
	bdMetric met;
	mFns->bdDistance(&(mOwnId), &(id->id), &met);
	int bucket = mFns->bdBucketDistance(&met);


	/* select correct bucket */
	bdBucket &buck =  buckets[bucket];

	/* loop through ids, to find it */
	std::list<bdPeer>::iterator it;
	for(it = buck.entries.begin(); it != buck.entries.end(); it++)
	{
                /* similar id check */
		if (mFns->bdSimilarId(id, &(it->mPeerId)))
		{
#ifdef DEBUG_BD_SPACE
			fprintf(stderr, "peer:");
			mFns->bdPrintId(std::cerr, id);	
			fprintf(stderr, " bucket: %d", bucket);
			fprintf(stderr, "\n");
			fprintf(stderr, "Original Flags: %x Extra: %x\n", 
				it->mPeerFlags, it->mExtraFlags);
#endif
			it->mPeerFlags |= flags;
			it->mExtraFlags |= ex_flags;

#ifdef DEBUG_BD_SPACE
			fprintf(stderr, "Updated Flags: %x Extra: %x\n",
				it->mPeerFlags, it->mExtraFlags);
#endif
		}
	}
	return true;
}

/* print tables.
 */

int     bdSpace::printDHT()
{
	std::map<bdMetric, bdId> closest;
	std::map<bdMetric, bdId>::iterator mit;

	std::vector<bdBucket>::iterator it;
	std::list<bdPeer>::iterator eit;

	/* iterate through the buckets, and sort by distance */
	int i = 0;

#ifdef BITDHT_DEBUG
	fprintf(stderr, "bdSpace::printDHT()\n");
	for(it = buckets.begin(); it != buckets.end(); it++, i++)
	{
		if (it->entries.size() > 0)
		{
			fprintf(stderr, "Bucket %d ----------------------------\n", i);
		}

		for(eit = it->entries.begin(); eit != it->entries.end(); eit++) 
		{
			bdMetric dist;
			mFns->bdDistance(&(mOwnId), &(eit->mPeerId.id), &dist);

			fprintf(stderr, " Metric: ");
			mFns->bdPrintNodeId(std::cerr, &(dist));
			fprintf(stderr, " Id: ");
			mFns->bdPrintId(std::cerr, &(eit->mPeerId));
			fprintf(stderr, " PeerFlags: %08x", eit->mPeerFlags);
			fprintf(stderr, "\n");
		}
	}
#endif

	fprintf(stderr, "--------------------------------------\n");
	fprintf(stderr, "DHT Table Summary --------------------\n");
	fprintf(stderr, "--------------------------------------\n");

	/* little summary */
	unsigned long long sum = 0;
	unsigned long long no_peers = 0;
	uint32_t count = 0;
	bool doPrint = false;
	bool doAvg = false;

	i = 0;
	for(it = buckets.begin(); it != buckets.end(); it++, i++)
	{
		int size = it->entries.size();
		int shift = BITDHT_KEY_BITLEN - i;
		bool toBig = false;

		if (shift > BITDHT_ULLONG_BITS - mFns->bdBucketBitSize() - 1)
		{
			toBig = true;
			shift = BITDHT_ULLONG_BITS - mFns->bdBucketBitSize() - 1;
		}
		unsigned long long no_nets = ((unsigned long long) 1 << shift);

		/* use doPrint so it acts as a single switch */
		if (size && !doAvg && !doPrint)
		{	
			doAvg = true;
		}

		if (size && !doPrint)
		{	
			doPrint = true;
		}

		if (size == 0)
		{
			/* reset counters - if empty slot - to discount outliers in average */
			sum = 0;
			no_peers = 0;
			count = 0;
		}

		if (doPrint)
		{
			if (size)
				fprintf(stderr, "Bucket %d: %d peers: ", i, size);
#ifdef BITDHT_DEBUG
			else
				fprintf(stderr, "Bucket %d: %d peers: ", i, size);
#endif
		}
		if (toBig)
		{
			if (size)
			{
				if (doPrint)
					bd_fprintf(stderr, "Estimated NetSize >> %llu\n", no_nets);
			}
			else
			{
#ifdef BITDHT_DEBUG
				if (doPrint)
					bd_fprintf(stderr, " Bucket = Net / >> %llu\n", no_nets);
#endif
			}
		}
		else
		{
			no_peers = no_nets * size;
			if (size)
			{	
				if (doPrint)
					bd_fprintf(stderr, "Estimated NetSize = %llu\n", no_peers);
			}
			else
			{

#ifdef BITDHT_DEBUG
				if (doPrint)
					bd_fprintf(stderr, " Bucket = Net / %llu\n", no_nets);
#endif

			}
		}
		if (doPrint && doAvg && !toBig)
		{
			if (size == mFns->bdNodesPerBucket())
			{
				/* last average */
				doAvg = false;
			}
			if (no_peers != 0)
			{
				sum += no_peers;
				count++;	
#ifdef BITDHT_DEBUG
				fprintf(stderr, "Est: %d: %llu => %llu / %d\n", 
					i, no_peers, sum, count);
#endif
			}
		}
			
	}
	if (count == 0)
	{
		fprintf(stderr, "Zero Network Size (count = 0)\n");
	}
	else
	{
		bd_fprintf(stderr, "Estimated Network Size = (%llu / %d) = %llu\n", sum, count, sum / count);
	}

	return 1;
}


int     bdSpace::getDhtBucket(const int idx, bdBucket &bucket)
{
	if ((idx < 0) || (idx > (int) buckets.size() - 1 ))
	{
		return 0;
	}
	bucket = buckets[idx];
	return 1;
}

uint32_t  bdSpace::calcNetworkSize()
{
	return calcNetworkSizeWithFlag(~0u) ;
}

uint32_t  bdSpace::calcNetworkSizeWithFlag(uint32_t withFlag)
{
	std::vector<bdBucket>::iterator it;

	/* little summary */
	unsigned long long sum = 0;
	unsigned long long no_peers = 0;
	uint32_t count = 0;

#ifdef BITDHT_DEBUG
	std::cerr << "Estimating DHT network size. Flags=" << std::hex << withFlag << std::dec << std::endl;
#endif

	int i = 0;
	for(it = buckets.begin(); it != buckets.end(); it++, i++)
	{
		int size = 0;
		std::list<bdPeer>::iterator lit;
		for(lit = it->entries.begin(); lit != it->entries.end(); lit++)
			if (withFlag & lit->mPeerFlags)
				size++;

		int shift = BITDHT_KEY_BITLEN - i;

		if (shift > BITDHT_ULLONG_BITS - mFns->bdBucketBitSize() - 1)
			continue ;

		unsigned long long no_nets = ((unsigned long long) 1 << shift);

		no_peers = no_nets * size;

		if(size < mFns->bdNodesPerBucket() && size > 0)
		{
			sum += no_peers;
			count++;
		}

#ifdef BITDHT_DEBUG
		if(size > 0)
			std::cerr << "  Bucket " << shift << ": " << size << " / " << mFns->bdNodesPerBucket() << " peers. no_nets=" << no_nets << ". no_peers=" << no_peers << "." << std::endl;
#endif
	}


	uint32_t NetSize = 0;
	if (count != 0)
		NetSize = sum / count;

#ifdef BITDHT_DEBUG
	std::cerr << "Estimated net size: " << NetSize << ". Old style estimate: " << calcNetworkSizeWithFlag_old(withFlag) << std::endl;
#endif

	return NetSize;
}

/* (csoler) This is the old method for computing the DHT size estimate. The method is based on averaging the
 *          estimate given by each bucket: n 2^b where n is the bucket size and b is the number of similar bits
 *          in this bucket. The idea is that buckets that are not empty nor full give a estimate of the network
 *          size.
 *
 *          The existing implementation of this method was not using all non empty/full buckets, in order to avoid
 *          outliers, but this method is also biased, and tends to give a lower value (because it skips the largest buckets)
 *          especially when the network is very sparse.
 *
 *          The new implementation respects the original estimate without bias, but it still notoriously wrong. Only averaging
 *          the estimate over a large period of time would produce a reliable value.
 */
uint32_t  bdSpace::calcNetworkSizeWithFlag_old(uint32_t withFlag)
{
	std::vector<bdBucket>::iterator it;

	/* little summary */
	unsigned long long sum = 0;
	unsigned long long no_peers = 0;
	uint32_t count = 0;
	uint32_t totalcount = 0;
	bool doPrint = false;
	bool doAvg = false;

	int i = 0;
	for(it = buckets.begin(); it != buckets.end(); it++, i++)
	{
		int size = 0;
		std::list<bdPeer>::iterator lit;
		for(lit = it->entries.begin(); lit != it->entries.end(); lit++)
		{
			if (withFlag & lit->mPeerFlags)
			{
				size++;
			}
		}

		totalcount += size;

		int shift = BITDHT_KEY_BITLEN - i;
		bool toBig = false;

		if (shift > BITDHT_ULLONG_BITS - mFns->bdBucketBitSize() - 1)
		{
			toBig = true;
			shift = BITDHT_ULLONG_BITS - mFns->bdBucketBitSize() - 1;
		}
		unsigned long long no_nets = ((unsigned long long) 1 << shift);


		/* use doPrint so it acts as a single switch */
		if (size && !doAvg && !doPrint)
		{
			doAvg = true;
		}

		if (size && !doPrint)
		{
			doPrint = true;
		}

		if (size == 0)
		{
			/* reset counters - if empty slot - to discount outliers in average */
			sum = 0;
			no_peers = 0;
			count = 0;
		}

		if (!toBig)
		{
			no_peers = no_nets * size;
		}

		if (doPrint && doAvg && !toBig)
		{
			if (size == mFns->bdNodesPerBucket())
			{
				/* last average */
				doAvg = false;
			}
			if (no_peers != 0)
			{
				sum += no_peers;
				count++;
			}
		}
	}


	uint32_t NetSize = 0;
	if (count != 0)
	{
		NetSize = sum / count;
	}

	//std::cerr << "bdSpace::calcNetworkSize() : " << NetSize;
	//std::cerr << std::endl;

	return NetSize;
}

uint32_t  bdSpace::calcSpaceSize()
{
	std::vector<bdBucket>::iterator it;

	/* little summary */
	uint32_t totalcount = 0;
	for(it = buckets.begin(); it != buckets.end(); it++)
	{
		totalcount += it->entries.size();
	}
	return totalcount;
}

uint32_t  bdSpace::calcSpaceSizeWithFlag(uint32_t withFlag)
{
	std::vector<bdBucket>::iterator it;

	/* little summary */
	uint32_t totalcount = 0;
	
	it = buckets.begin();
	if (it != buckets.end())
	{
		it++; /* skip own bucket! */
	}
	for(; it != buckets.end(); it++)
	{
		int size = 0;
		std::list<bdPeer>::iterator lit;
		for(lit = it->entries.begin(); lit != it->entries.end(); lit++)
		{
			if (withFlag & lit->mPeerFlags)
			{
				size++;
			}
		}
		totalcount += size;
	}
	return totalcount;
}

        /* special function to enable DHT localisation (i.e find peers from own network) */
bool bdSpace::findRandomPeerWithFlag(bdId &id, uint32_t withFlag)
{
	std::vector<bdBucket>::iterator it;
	uint32_t totalcount = calcSpaceSizeWithFlag(withFlag);

	if(totalcount == 0)
		return false ;

	uint32_t rnd = bdRandom::random_u32() % totalcount;
	uint32_t i = 0;
	uint32_t buck = 0;

	//std::cerr << "bdSpace::findRandomPeerWithFlag()";
	//std::cerr << std::endl;

	it = buckets.begin();
	if (it != buckets.end())
	{
		it++; /* skip own bucket! */
		buck++;
	}
	for(; it != buckets.end(); it++, buck++)
	{
		std::list<bdPeer>::iterator lit;
		for(lit = it->entries.begin(); lit != it->entries.end(); lit++)
		{
			if (withFlag & lit->mPeerFlags)
			{
				if (i == rnd)
				{
#ifdef BITDHT_DEBUG
					std::cerr << "bdSpace::findRandomPeerWithFlag() found #" << i;
					std::cerr << " in bucket #" << buck;
					std::cerr << std::endl;
#endif
					/* found */
					id = lit->mPeerId;
					return true;
				}
				i++;
			}
		}
	}
	std::cerr << "bdSpace::findRandomPeerWithFlag() failed to find " << rnd << " / " << totalcount;
	std::cerr << std::endl;
#ifdef BITDHT_DEBUG
#endif

	return false;
}

