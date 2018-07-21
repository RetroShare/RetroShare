/*******************************************************************************
 * bitdht/bdfriendlist.h                                                       *
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

#ifndef BITDHT_FRIEND_LIST_H
#define BITDHT_FRIEND_LIST_H

/* 
 * This class maintains a list of current friends and friends-of-friends.
 * It should also be updated when a peer's address has been identified.
 *
 * It is used for selecting preferential peers for DHT Connections.
 * and for detecting bad peers that are duplicating real RS peers.
 *
 */


#include "bitdht/bdiface.h"
#include <set>

#define BD_FRIEND_ENTRY_TIMEOUT		10


//#define BD_FRIEND_ENTRY_ONLINE		0x0001
//#define BD_FRIEND_ENTRY_ADDR_OK		0x0002

//#define BD_FRIEND_ENTRY_WHITELIST	BITDHT_PEER_STATUS_DHT_WHITELIST
//#define BD_FRIEND_ENTRY_FOF		BITDHT_PEER_STATUS_DHT_FOF
//#define BD_FRIEND_ENTRY_FRIEND 	BITDHT_PEER_STATUS_DHT_FRIEND
//#define BD_FRIEND_ENTRY_RELAY_SERVER	BITDHT_PEER_STATUS_DHT_RELAY_SERVER

//#define BD_FRIEND_ENTRY_SELF		BITDHT_PEER_STATUS_DHT_SELF

//#define BD_FRIEND_ENTRY_MASK_KNOWN	BITDHT_PEER_STATUS_MASK_KNOWN

class bdFriendEntry
{
	public:
	bdFriendEntry();

bool	addrKnown(struct sockaddr_in *addr);
uint32_t getPeerFlags();

	bdId mPeerId;
	uint32_t mFlags; 
	time_t mLastSeen;
};

class bdFriendList
{

	public:
	bdFriendList(const bdNodeId *ownid);

bool	updatePeer(const bdId *id, uint32_t flags);
bool	removePeer(const bdNodeId *id);

bool	findPeerEntry(const bdNodeId *id, bdFriendEntry &entry);
bool    findPeersWithFlags(uint32_t flags, std::list<bdNodeId> &peerList);

bool    print(std::ostream &out);
	private:

	std::map<bdNodeId, bdFriendEntry> mPeers;
};

class bdPeerQueue
{

	public:
	bdPeerQueue();

bool	queuePeer(const bdId *id, uint32_t flags);
bool	popPeer(bdId *id, uint32_t &flags);

	private:

	std::list<bdFriendEntry> mPeerQueue;
};

	
#endif

