/*
 * bitdht/bdstore.cc
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


#include "bitdht/bdstore.h"
#include "util/bdnet.h"

#include <stdio.h>
#include <iostream>

//#define DEBUG_STORE 1

bdStore::bdStore(std::string file, bdDhtFunctions *fns)
	:mFns(fns)
{
#ifdef DEBUG_STORE
	std::cerr << "bdStore::bdStore(" << file << ")";
	std::cerr << std::endl;
#endif

	/* read data from file */
	mStoreFile = file;

	reloadFromStore();
}

int bdStore::clear()
{
	mIndex = 0;
	store.clear();
	return 1;
}

int bdStore::reloadFromStore()
{
	clear();

	FILE *fd = fopen(mStoreFile.c_str(), "r");
	if (!fd)
	{
		fprintf(stderr, "Failed to Open File: %s ... No Peers\n", mStoreFile.c_str());
		return 0;
	}
		

	char line[10240];
	char addr_str[10240];
	struct sockaddr_in addr;
	addr.sin_family = PF_INET;
	unsigned short port;

	while(line == fgets(line, 10240, fd))
	{
		if (2 == sscanf(line, "%s %hd", addr_str, &port))
		{
			if (bdnet_inet_aton(addr_str, &(addr.sin_addr)))
			{
				addr.sin_port = htons(port);
				bdPeer peer;
				//bdZeroNodeId(&(peer.mPeerId.id));
				peer.mPeerId.addr = addr;
				peer.mLastSendTime = 0;
				peer.mLastRecvTime = 0;
				store.push_back(peer);
#ifdef DEBUG_STORE
				fprintf(stderr, "Read: %s %d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
#endif
			}
		}
	}

	fclose(fd);

#ifdef DEBUG_STORE
	fprintf(stderr, "Read %ld Peers\n", (long) store.size());
#endif

	return 1;

}

int 	bdStore::getPeer(bdPeer *peer)
{
#ifdef DEBUG_STORE
	fprintf(stderr, "bdStore::getPeer() %ld Peers left\n", (long) store.size());
#endif

	std::list<bdPeer>::iterator it;
	int i = 0;
	for(it = store.begin(); (it != store.end()) && (i < mIndex); it++, i++) ; /* empty loop */
	if (it != store.end())
	{
		*peer = *it;
		mIndex++;
		return 1;
	}
	return 0;
}

#define MAX_ENTRIES 500

	/* maintain a sorted list */
void	bdStore::addStore(bdPeer *peer)
{
#ifdef DEBUG_STORE
	std::cerr << "bdStore::addStore() ";
	mFns->bdPrintId(std::cerr, &(peer->mPeerId));
	std::cerr << std::endl;
#endif

	/* remove old entry */
	bool removed = false;

	std::list<bdPeer>::iterator it;
	for(it = store.begin(); it != store.end(); )
	{
		if ((it->mPeerId.addr.sin_addr.s_addr == peer->mPeerId.addr.sin_addr.s_addr) &&
		    (it->mPeerId.addr.sin_port == peer->mPeerId.addr.sin_port))
		{
			removed = true;
#ifdef DEBUG_STORE
			std::cerr << "bdStore::addStore() Removed Existing Entry: ";
			mFns->bdPrintId(std::cerr, &(it->mPeerId));
			std::cerr << std::endl;
#endif
			it = store.erase(it);
		}
		else
		{
			it++;
		}
	}

#ifdef DEBUG_STORE
	std::cerr << "bdStore::addStore() Push_back";
	std::cerr << std::endl;
#endif
	store.push_back(*peer);

	while(store.size() > MAX_ENTRIES)
	{
#ifdef DEBUG_STORE
		std::cerr << "bdStore::addStore() pop_front()";
		std::cerr << std::endl;
#endif
		store.pop_front();
	}
}

void	bdStore::writeStore(std::string file)
{
	/* write out store */
#ifdef DEBUG_STORE
	fprintf(stderr, "bdStore::writeStore(%s) =  %d entries\n", file.c_str(), store.size());
#endif

	if (store.size() < 0.9 * MAX_ENTRIES)
	{
		/* don't save yet! */
#ifdef DEBUG_STORE
		fprintf(stderr, "bdStore::writeStore() Delaying until more entries\n");
#endif
		return;
	}


	FILE *fd = fopen(file.c_str(), "w");

	if (!fd)
	{
#ifdef DEBUG_STORE
#endif
		fprintf(stderr, "bdStore::writeStore() FAILED to Open File\n");
		return;
	}
	
	std::list<bdPeer>::iterator it;
	for(it = store.begin(); it != store.end(); it++)
	{
		fprintf(fd, "%s %d\n", inet_ntoa(it->mPeerId.addr.sin_addr), ntohs(it->mPeerId.addr.sin_port));
#ifdef DEBUG_STORE
		fprintf(stderr, "Storing Peer Address: %s %d\n", inet_ntoa(it->mPeerId.addr.sin_addr), ntohs(it->mPeerId.addr.sin_port));
#endif

	}
	fclose(fd);
}

void	bdStore::writeStore()
{
#if 0
	if (mStoreFile == "")
	{
		return;
	}
#endif
	return writeStore(mStoreFile);
}

