/*******************************************************************************
 * RetroShare Broadcast Domain Discovery                                       *
 *                                                                             *
 * Copyright (C) 2019-2021  Gioacchino Mazzurco <gio@altermundi.net>           *
 * Copyright (C) 2019-2021  Asociación Civil Altermundi <info@altermundi.net>  *
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
#include <memory>
#include <forward_list>

#include <udp_discovery_peer.hpp>

#include "retroshare/rsbroadcastdiscovery.h"
#include "util/rsthreads.h"
#include "util/rsdebug.h"

#ifdef __ANDROID__
#	include <jni/jni.hpp>
#	include "rs_android/rsjni.hpp"
#endif // def __ANDROID__


namespace UDC = udpdiscovery;
class RsPeers;

class BroadcastDiscoveryService :
        public RsBroadcastDiscovery, public RsTickingThread
{
public:
	explicit BroadcastDiscoveryService(RsPeers& pRsPeers);
	~BroadcastDiscoveryService() override;

	/// @see RsBroadcastDiscovery
	std::vector<RsBroadcastDiscoveryResult> getDiscoveredPeers() override;

	/// @see RsBroadcastDiscovery
	bool isMulticastListeningEnabled() override;

	/// @see RsBroadcastDiscovery
	bool enableMulticastListening() override;

	/// @see RsBroadcastDiscovery
	bool disableMulticastListening() override;

	void threadTick() override; /// @see RsTickingThread

protected:
	constexpr static uint16_t port = 36405;
	constexpr static uint32_t appId = 904571;

	void updatePublishedData();

	UDC::PeerParameters mUdcParameters;
	UDC::Peer mUdcPeer;

	std::map<UDC::IpPort, std::string> mDiscoveredData;
	RsMutex mDiscoveredDataMutex;

	RsPeers& mRsPeers;

	RsBroadcastDiscoveryResult createResult(
	        const UDC::IpPort& ipp, const std::string& uData );

#ifdef __ANDROID__
	struct AndroidMulticastLock
	{
		static constexpr auto Name()
		{ return "android/net/wifi/WifiManager$MulticastLock"; }
	};

	jni::Global<jni::Object<AndroidMulticastLock>> mAndroidWifiMulticastLock;

	/** Initialize the wifi multicast lock without acquiring it
	 * Needed to enable multicast listening in Android, for RetroShare broadcast
	 * discovery inspired by:
	 * https://github.com/flutter/flutter/issues/16335#issuecomment-420547860
	 */
	bool createAndroidMulticastLock();
#endif

	RS_SET_CONTEXT_DEBUG_LEVEL(3)
};
