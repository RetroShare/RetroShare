/*******************************************************************************
 * bitdht/bdstore.cc                                                           *
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

#include "bitdht/bdstore.h"
#include "util/bdnet.h"
#include "util/bdfile.h"

#include <stdio.h>
#include <iostream>

//#define DEBUG_STORE 1

bdStore::bdStore(std::string file, std::string backupfile, bdDhtFunctions *fns)
	:mFns(fns)
{
#ifdef DEBUG_STORE
	std::cerr << "bdStore::bdStore(" << file << ")";
	std::cerr << std::endl;
#endif

	/* read data from file */
	mStoreFile = file;
	mStoreFileBak = backupfile;

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
	int result = reloadFromStore(mStoreFile);
	if( result != 0 && store.size() > 0){
		return result;
	} else if(mStoreFileBak != "") { //Nothing loaded, try the backup file
		return reloadFromStore(mStoreFileBak);
	} else {
		return 0;
	}
}

int bdStore::reloadFromStore(std::string file)
{
	clear();

	FILE *fd = fopen(file.c_str(), "r");
	if (!fd)
	{
		fprintf(stderr, "Failed to Open File: %s ... No Peers\n", file.c_str());
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
				bdZeroNodeId(&(peer.mPeerId.id));
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

// This is a very ugly function!
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

int     bdStore::filterIpList(const std::list<struct sockaddr_in> &filteredIPs)
{
	// Nasty O(n^2) iteration over 500 entries!!!. 
	// hope its not used to often.

	std::list<struct sockaddr_in>::const_iterator it;
	for(it = filteredIPs.begin(); it != filteredIPs.end(); it++)
	{
		std::list<bdPeer>::iterator sit;
		for(sit = store.begin(); sit != store.end();)
		{
			if (it->sin_addr.s_addr == sit->mPeerId.addr.sin_addr.s_addr)
			{
				std::cerr << "bdStore::filterIpList() Found Bad entry in Store. Erasing!";
				std::cerr << std::endl;

				sit = store.erase(sit);
			}
			else
			{
				sit++;
			}
		}
	}
	return 1;
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

	std::list<bdPeer>::iterator it;
	for(it = store.begin(); it != store.end(); )
	{
		if ((it->mPeerId.addr.sin_addr.s_addr == peer->mPeerId.addr.sin_addr.s_addr) &&
		    (it->mPeerId.addr.sin_port == peer->mPeerId.addr.sin_port))
		{
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

	std::string filetmp = file + ".tmp" ;

	FILE *fd = fopen(filetmp.c_str(), "w");

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
		fprintf(fd, "%s %d\n", bdnet_inet_ntoa(it->mPeerId.addr.sin_addr).c_str(), ntohs(it->mPeerId.addr.sin_port));
#ifdef DEBUG_STORE
		fprintf(stderr, "Storing Peer Address: %s %d\n", inet_ntoa(it->mPeerId.addr.sin_addr), ntohs(it->mPeerId.addr.sin_port));
#endif

	}
	fclose(fd);

	if(!bdFile::renameFile(filetmp,file))
		std::cerr << "Could not rename file !!" << std::endl;
#ifdef DEBUG_STORE
	else
		std::cerr << "Successfully renamed file " << filetmp << " to " << file << std::endl;
#endif
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



