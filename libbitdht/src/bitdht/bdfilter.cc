
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
#include "util/bdfile.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <time.h>

/**
 * #define DEBUG_FILTER 1
**/
#define BDFILTER_ENTRY_DROP_PERIOD	(7 * 24 * 3600)

bdFilter::bdFilter(const std::string &fname, const bdNodeId *ownid,  uint32_t filterFlags, bdDhtFunctions *fns)
{
	/* */
    mOwnId = *ownid;
    mFns = fns;
    mFilename = fname ;

    loadBannedIpFile() ;

    mFilterFlags = filterFlags;
}

void bdFilter::writeBannedIpFile()
{
    std::string filetmp = mFilename + ".tmp" ;

    FILE *fd = fopen(filetmp.c_str(), "w");

    if (!fd)
    {
        std::cerr << "(EE) bdFilter::writeBannedIpFile() FAILED to Open File " << mFilename << std::endl;
        return;
    }

    for( std::map<uint32_t,bdFilteredPeer>::iterator it=mFiltered.begin();it!=mFiltered.end();++it)
    {
        fprintf(fd, "%s %u %lu %lu\n", bdnet_inet_ntoa(it->second.mAddr.sin_addr).c_str(), it->second.mFilterFlags, it->second.mFilterTS, it->second.mLastSeen) ;
#ifdef DEBUG_FILTER
        fprintf(stderr, "Storing Peer Address: %s \n", bdnet_inet_ntoa(it->second.mAddr.sin_addr).c_str()) ;
#endif

    }
    fclose(fd);

    if(!bdFile::renameFile(filetmp,mFilename))
        std::cerr << "Could not rename file !!" << std::endl;
#ifdef DEBUG_FILTER
    else
        std::cerr << "Successfully renamed file " << filetmp << " to " << mFilename << std::endl;
#endif
}

void bdFilter::loadBannedIpFile()
{
        char line[10240];
        char addr_str[10240];

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = PF_INET;

    FILE *fd = fopen(mFilename.c_str(),"r") ;

    if(fd == NULL)
    {
        std::cerr << "(EE) Cannot load filter file " << mFilename << std::endl;
        return ;
    }

        while(line == fgets(line, 10240, fd))
        {
        uint32_t filter_flags ;
        unsigned long long int filter_ts ;
        unsigned long long int last_seen ;

            if (4 == sscanf(line, "%s %u %llu %llu", addr_str, &filter_flags,&filter_ts,&last_seen))
            {
                if (bdnet_inet_aton(addr_str, &(addr.sin_addr)))
                {
                    addr.sin_port = 0;

                    bdFilteredPeer peer;

                    peer.mAddr = addr;
                    peer.mFilterTS = filter_ts;
                    peer.mLastSeen = last_seen;
                    peer.mFilterFlags = filter_flags;

            mFiltered[addr.sin_addr.s_addr] = peer ;
    #ifdef DEBUG_FILTER
                    std::cerr << "Loaded filtered IP: " << std::string(addr_str) << " last seen: " << last_seen << ", TS=" << filter_ts << std::endl;
    #endif
                }
            }
        }

        fclose(fd);
}

//bool bdFilter::filtered(std::list<bdFilteredPeer> &answer)
//{
//	answer = mFiltered;
//	return (answer.size() > 0);
//}

bool bdFilter::filteredIPs(std::list<struct sockaddr_in> &answer)
{
    std::map<uint32_t,bdFilteredPeer>::iterator it;
	for(it = mFiltered.begin(); it != mFiltered.end(); it++)
	{
        answer.push_back(it->second.mAddr);
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
        bool isNew = addPeerToFilter(id->addr, flags);

		if (isNew)
		{
			return 1;
		}
	}

	return 0;
}

int bdFilter::addPeerToFilter(const struct sockaddr_in& addr, uint32_t flags)
{
    std::map<uint32_t,bdFilteredPeer>::iterator it = mFiltered.find(addr.sin_addr.s_addr) ;

    if(it != mFiltered.end())
	{
            it->second.mLastSeen = time(NULL);
            it->second.mFilterFlags |= flags;
    }
    else
    {
        time_t now = time(NULL);
        bdFilteredPeer fp;

        fp.mAddr = addr;
        fp.mAddr.sin_port = 0;
        fp.mFilterFlags = flags;
        fp.mFilterTS = now;
        fp.mLastSeen = now;

        uint32_t saddr = addr.sin_addr.s_addr;

        mFiltered[saddr] = fp;

        std::cerr << "Adding New Banned Ip Address: " << bdnet_inet_ntoa(addr.sin_addr);
        std::cerr << std::endl;
    }
    writeBannedIpFile() ;

    return true;
}

// void bdFilter::loadFilteredPeers(const std::list<bdFilteredPeer>& peers)
// {
//     for(std::list<bdFilteredPeer>::iterator it = peers.begin(); it != peers.end();++it)
//     {
// #ifdef DEBUG_FILTER
//         std::cerr << "Loading filtered peer " << inet_ntoa(it->mAddr.sin_addr) << " Flags: " << it->mFilterFlags << " FilterTS: "
//                   << now - it->mFilterTS << " LastSeen: " << now - it->mLastSeen << std::endl;
// #endif
//         uint32_t saddr = it->mAddr.sin_addr.s_addr;
//         mFiltered[saddr] = *it ;
//     }
// }
void bdFilter::getFilteredPeers(std::list<bdFilteredPeer>& peers)
{
    for(std::map<uint32_t,bdFilteredPeer>::iterator it = mFiltered.begin(); it != mFiltered.end();++it)
        peers.push_back(it->second) ;
}
/* fast check if the addr is in the structure */
int bdFilter::addrOkay(struct sockaddr_in *addr)
{
    std::map<uint32_t,bdFilteredPeer>::const_iterator it = mFiltered.find(addr->sin_addr.s_addr);

    if (it == mFiltered.end())
        return 1; // Address is Okay!

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
    std::cerr << "bdFilter: Checking current filter List:" << std::endl;
#endif

	time_t now = time(NULL);
	time_t dropTime = now - BDFILTER_ENTRY_DROP_PERIOD;

    for(std::map<uint32_t,bdFilteredPeer>::iterator it = mFiltered.begin(); it != mFiltered.end();)
    {
#ifdef DEBUG_FILTER
        std::cerr << "\t" << bdnet_inet_ntoa(it->second.mAddr.sin_addr);
        std::cerr << " Flags: " << it->second.mFilterFlags;
        std::cerr << " FilterTS: " << now - it->second.mFilterTS;
        std::cerr << " LastSeen: " << now - it->second.mLastSeen;
#endif

        if (it->second.mLastSeen < dropTime)
        {
            /* remove from filter */
#ifdef DEBUG_FILTER
            std::cerr << " OLD DROPPING" << std::endl;
#endif
            std::map<uint32_t,bdFilteredPeer>::iterator tmp(it) ;
            ++tmp ;

            mFiltered.erase(it);
            it = tmp ;
        }
        else
        {
#ifdef DEBUG_FILTER
            std::cerr << " OK" << std::endl;
#endif
            it++;
        }
    }

	return true;
}



