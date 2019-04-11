/*******************************************************************************
 * bitdht/bdfilter.h                                                           *
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

#ifndef BITDHT_FILTER_H
#define BITDHT_FILTER_H

#include "bitdht/bdiface.h"
#include <set>

/* Query result flags are in bdiface.h */

#define BITDHT_FILTER_REASON_OWNID	0x0001

class bdFilteredPeer
{
    public:
    struct sockaddr_in mAddr;
    uint32_t mFilterFlags; /* reasons why we are filtering */
    time_t mFilterTS;
    time_t mLastSeen;
};

class bdNodeManager;

class bdFilter
{
	public:
    bdFilter(const std::string& fname, const bdNodeId *ownid, uint32_t filterFlags, bdDhtFunctions *fns, bdNodeManager *manager);

	// get the answer.
//bool	filtered(std::list<bdFilteredPeer> &answer);
bool	filteredIPs(std::list<struct sockaddr_in> &answer);

//void    loadFilteredPeers(const std::list<bdFilteredPeer>& peers) ;
void 	getFilteredPeers(std::list<bdFilteredPeer> &peers);

int 	checkPeer(const bdId *id, uint32_t peerFlags);

int 	addrOkay(struct sockaddr_in *addr);
int 	addPeerToFilter(const struct sockaddr_in &addr, uint32_t flags);

bool 	cleanupFilter();

void loadBannedIpFile() ;
void writeBannedIpFile() ;

private:

bool	isOwnIdWithoutBitDhtFlags(const bdId *id, uint32_t peerFlags);

	// searching for
	bdNodeId mOwnId;
	uint32_t mFilterFlags;

	std::map<uint32_t,bdFilteredPeer> mFiltered;
	bdDhtFunctions *mFns;
	std::string mFilename ;

	// have access to the manager for isBanned callback
	bdNodeManager* mNodeManager;
};


#endif

