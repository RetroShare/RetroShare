
/*
 * bitdht/bdfilter.cc
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


#include "bitdht/bdfilter.h"

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <time.h>

/**
 * #define DEBUG_FILTER 1
**/

#define BDFILTER_ENTRY_DROP_PERIOD	(6 * 3600)


bdFilter::bdFilter(const bdNodeId *ownId, std::list<bdFilteredPeer> &startList, 
		uint32_t filterFlags, bdDhtFunctions *fns)
{
	/* */
	mOwnId = *ownId;
	mFns = fns;

	std::list<bdFilteredPeer>::iterator it;

	for(it = startList.begin(); it != startList.end(); it++)
	{
		mFiltered.push_back(*it);
	}

	mFilterFlags = filterFlags;
}

bool bdFilter::filtered(std::list<bdFilteredPeer> &answer)
{
	answer = mFiltered;
	return (answer.size() > 0);
}

bool bdFilter::filteredIPs(std::list<struct sockaddr_in> &answer)
{
	std::list<bdFilteredPeer>::iterator it;
	for(it = mFiltered.begin(); it != mFiltered.end(); it++)
	{
		answer.push_back(it->mAddr);
	}
	return (answer.size() > 0);
}

int bdFilter::checkPeer(const bdId *id, uint32_t mode)
{
	bool add = false;
	uint32_t flags = 0;
	if ((mFilterFlags & BITDHT_FILTER_REASON_OWNID) && 
			isOwnIdWithoutBitDhtFlags(id, mode))
	{
		add = true;
		flags |= BITDHT_FILTER_REASON_OWNID;
	}

	if (add)
	{
		bool isNew = addPeerToFilter(id, flags);
		if (isNew)
		{
			return 1;
		}
	}

	return 0;
}

int bdFilter::addPeerToFilter(const bdId *id, uint32_t flags)
{
	std::list<bdFilteredPeer>::iterator it;
	bool found = false;
	for(it = mFiltered.begin(); it != mFiltered.end(); it++)
	{
		if (id->addr.sin_addr.s_addr == it->mAddr.sin_addr.s_addr)
		{
			found = true;
			it->mLastSeen = time(NULL);
			it->mFilterFlags |= flags;
			break;
		}
	}

	if (!found)
	{
		time_t now = time(NULL);
		bdFilteredPeer fp;

		fp.mAddr = id->addr;
		fp.mAddr.sin_port = 0;
		fp.mFilterFlags = flags;
		fp.mFilterTS = now;	
		fp.mLastSeen = now;

		mFiltered.push_back(fp);

		uint32_t saddr = id->addr.sin_addr.s_addr;
		mIpsBanned.insert(saddr);

		std::cerr << "Adding New Banned Ip Address: " << bdnet_inet_ntoa(id->addr.sin_addr);
		std::cerr << std::endl;

		return true;
	}

	return false;
}

/* fast check if the addr is in the structure */
int bdFilter::addrOkay(struct sockaddr_in *addr)
{
	std::set<uint32_t>::const_iterator it = mIpsBanned.find(addr->sin_addr.s_addr);
	if (it == mIpsBanned.end())
	{
		return 1; // Address is Okay!
	}
#ifdef DEBUG_FILTER
	std::cerr << "Detected Packet From Banned Ip Address: " << inet_ntoa(addr->sin_addr);
	std::cerr << std::endl;
#endif
	return 0;
}


bool bdFilter::isOwnIdWithoutBitDhtFlags(const bdId *id, uint32_t peerFlags)
{
	if (peerFlags & BITDHT_PEER_STATUS_RECV_PONG)
	{
		if (peerFlags & BITDHT_PEER_STATUS_DHT_ENGINE)
		{
			/* okay! */
			return false;
		}

		/* now check distance */
		bdMetric dist;
		mFns->bdDistance(&mOwnId, &(id->id), &dist);
		int bucket = mFns->bdBucketDistance(&dist);

		/* if they match us... kill it */
		if (bucket == 0)
		{
			return true;
		}
	}
	return false;
}


/* periodically we want to cleanup the filter....
 * if we haven't had an IP address reported as filtered for several hours.
 * remove it from the list.
 */

bool bdFilter::cleanupFilter()
{
#ifdef DEBUG_FILTER
	std::cerr << "bdFilter::cleanupFilter() Current BanList" << std::endl;
	struct in_addr inaddr;

	std::set<uint32_t>::iterator sit;
	for(sit = mIpsBanned.begin(); sit != mIpsBanned.end(); sit++)
	{
		inaddr.s_addr = *sit;
		std::cerr << "\tBanned: " << inet_ntoa(inaddr) << std::endl;
	}
#endif

	mIpsBanned.clear();
	
#ifdef DEBUG_FILTER
	std::cerr << "Filter List:" << std::endl;
#endif

	time_t now = time(NULL);
	time_t dropTime = now - BDFILTER_ENTRY_DROP_PERIOD;

	std::list<bdFilteredPeer>::iterator it;
	for(it = mFiltered.begin(); it != mFiltered.end();)
	{
#ifdef DEBUG_FILTER
		std::cerr << "\t" << inet_ntoa(it->mAddr.sin_addr);
		std::cerr << " Flags: " << it->mFilterFlags;
		std::cerr << " FilterTS: " << now - it->mFilterTS;
		std::cerr << " LastSeen: " << now - it->mLastSeen;
#endif

		if (it->mLastSeen < dropTime)
		{
			/* remove from filter */
#ifdef DEBUG_FILTER
			std::cerr << " OLD DROPPING" << std::endl;
#endif
			it = mFiltered.erase(it);
		}
		else
		{
#ifdef DEBUG_FILTER
			std::cerr << " OK" << std::endl;
#endif
			uint32_t saddr = it->mAddr.sin_addr.s_addr;
			mIpsBanned.insert(saddr);

			it++;
		}
	}

	return true;
}



