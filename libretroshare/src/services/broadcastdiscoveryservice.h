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

#include <cstdint>
#include <map>
#include <iostream>
#include <endpoint.hpp>
#include <memory>
#include <forward_list>

#include "retroshare/rsbroadcastdiscovery.h"
#include "util/rsthreads.h"

namespace UDC = udpdiscovery;
class RsPeers;

class BroadcastDiscoveryService :
        public RsBroadcastDiscovery, public RsTickingThread
{
public:
	// TODO: std::shared_ptr<RsPeers> mRsPeers;
	BroadcastDiscoveryService(RsPeers& pRsPeers);
	virtual ~BroadcastDiscoveryService() override;

	/// @see RsBroadcastDiscovery
	std::vector<RsBroadcastDiscoveryResult> getDiscoveredPeers() override;

	/// @see RsTickingThread
	void data_tick() override;

protected:
	constexpr static uint16_t port = 36405;
	constexpr static uint32_t appId = 904571;

	void updatePublishedData();

	UDC::EndpointParameters mUdcParameters;
	UDC::Endpoint mUdcEndpoint;

	std::map<UDC::IpPort, std::string> mDiscoveredData;
	RsMutex mDiscoveredDataMutex;

	RsPeers& mRsPeers; // TODO: std::shared_ptr<RsPeers> mRsPeers;

	RsBroadcastDiscoveryResult createResult(
	        const UDC::IpPort& ipp, const std::string& uData );
};
