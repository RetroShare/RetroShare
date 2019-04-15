/*******************************************************************************
 * RetroShare Broadcast Domain Discovery                                       *
 *                                                                             *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@altermundi.net>                *
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

#include <functional>
#include <memory>
#include <vector>
#include <string>

#include "retroshare/rsids.h"
#include "retroshare/rstypes.h"
#include "serialiser/rsserializable.h"
#include "util/rstime.h"
#include "util/rsurl.h"
#include "util/rsmemory.h"
#include "retroshare/rsevents.h"

class RsBroadcastDiscovery;

/**
 * Pointer to global instance of RsBroadcastDiscovery service implementation
 * @jsonapi{development}
 *
 * TODO: this should become std::weak_ptr once we have a reasonable services
 * management.
 */
extern std::shared_ptr<RsBroadcastDiscovery> rsBroadcastDiscovery;

struct RsBroadcastDiscoveryResult : RsSerializable
{
	PGPFingerprintType mPgpFingerprint;
	RsPeerId mSslId;
	std::string mProfileName;
	RsUrl locator;

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RS_SERIAL_PROCESS(mPgpFingerprint);
		RS_SERIAL_PROCESS(mSslId);
		RS_SERIAL_PROCESS(mProfileName);
		RS_SERIAL_PROCESS(locator);
	}
};

struct RsBroadcastDiscoveryPeerFoundEvent : RsEvent
{
	RsBroadcastDiscoveryPeerFoundEvent(
	        const RsBroadcastDiscoveryResult& eventData ) :
	    RsEvent(RsEventType::BROADCAST_DISCOVERY_PEER_FOUND), mData(eventData) {}

	RsBroadcastDiscoveryResult mData;

	/// @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RsEvent::serial_process(j, ctx);
		RS_SERIAL_PROCESS(mData);
	}

	~RsBroadcastDiscoveryPeerFoundEvent();
};

/**
 * Announce own RetroShare instace and look friends and peers in own broadcast
 * domain (aka LAN).
 * Emit event @see RsBroadcastDiscoveryPeerFoundEvent when a new peer (not
 * friend yet) is found.
 */
class RsBroadcastDiscovery
{
public:
	/**
	 * @brief Get potential peers that have been discovered up until now
	 * @jsonapi{development}
	 * @return vector containing discovered peers, may be empty.
	 */
	virtual std::vector<RsBroadcastDiscoveryResult> getDiscoveredPeers() = 0;

	virtual ~RsBroadcastDiscovery();
};
