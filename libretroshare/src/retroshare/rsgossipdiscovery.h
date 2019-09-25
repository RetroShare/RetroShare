/*******************************************************************************
 * RetroShare remote peers gossip discovery                                    *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2008  Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#pragma once

#include <cstdint>
#include <string>
#include <list>
#include <map>

#include "retroshare/rstypes.h"
#include "retroshare/rsevents.h"
#include "util/rsmemory.h"

class RsGossipDiscovery;

/**
 * Pointer to global instance of RsGossipDiscovery service implementation
 * @jsonapi{development}
 *
 * TODO: this should become std::weak_ptr once we have a reasonable services
 * management.
 */
extern std::shared_ptr<RsGossipDiscovery> rsGossipDiscovery;

/**
 * @brief Emitted when a pending PGP certificate is received
 */
struct RsGossipDiscoveryFriendInviteReceivedEvent : RsEvent
{
	RsGossipDiscoveryFriendInviteReceivedEvent(
	        const std::string& invite );

	std::string mInvite;

	/// @see RsSerializable
	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx )
	{
		RsEvent::serial_process(j,ctx);
		RS_SERIAL_PROCESS(mInvite);
	}
};

class RsGossipDiscovery
{
public:
    virtual ~RsGossipDiscovery() = default;

	/**
	 * @brief getDiscFriends get a list of all friends of a given friend
	 * @jsonapi{development}
	 * @param[in] id peer to get the friends of
	 * @param[out] friends list of friends (ssl id)
	 * @return true on success false otherwise
	 */
	virtual bool getDiscFriends( const RsPeerId& id,
	                             std::list<RsPeerId>& friends ) = 0;

	/**
	 * @brief getDiscPgpFriends get a list of all friends of a given friend
	 * @jsonapi{development}
	 * @param[in] pgpid peer to get the friends of
	 * @param[out] gpg_friends list of friends (gpg id)
	 * @return true on success false otherwise
	 */
	virtual bool getDiscPgpFriends(
	        const RsPgpId& pgpid, std::list<RsPgpId>& gpg_friends ) = 0;

	/**
	 * @brief getPeerVersion get the version string of a peer.
	 * @jsonapi{development}
	 * @param[in] id peer to get the version string of
	 * @param[out] version version string sent by the peer
	 * @return true on success false otherwise
	 */
	virtual bool getPeerVersion(const RsPeerId& id, std::string& version) = 0;

	/**
	 * @brief getWaitingDiscCount get the number of queued discovery packets.
	 * @jsonapi{development}
	 * @param[out] sendCount number of queued outgoing packets
	 * @param[out] recvCount number of queued incoming packets
	 * @return true on success false otherwise
	 */
	virtual bool getWaitingDiscCount(size_t& sendCount, size_t& recvCount) = 0;
};
