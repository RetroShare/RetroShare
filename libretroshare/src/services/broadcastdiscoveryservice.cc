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

#include <functional>
#include <set>
#include <thread>
#include <chrono>
#include <vector>
#include <iostream>

#include "services/broadcastdiscoveryservice.h"
#include "retroshare/rspeers.h"
#include "serialiser/rsserializable.h"
#include "serialiser/rsserializer.h"
#include "retroshare/rsevents.h"

/*extern*/ std::shared_ptr<RsBroadcastDiscovery> rsBroadcastDiscovery(nullptr);
RsBroadcastDiscovery::~RsBroadcastDiscovery() { /* Beware of Rs prefix! */ }
RsBroadcastDiscoveryResult::~RsBroadcastDiscoveryResult() {}
RsBroadcastDiscoveryPeerFoundEvent::~RsBroadcastDiscoveryPeerFoundEvent() {}

struct BroadcastDiscoveryPack : RsSerializable
{
	BroadcastDiscoveryPack() : mLocalPort(0) {}

	RsPeerId mSslId;
	uint16_t mLocalPort;
	std::string mProfileName;

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx ) override
	{
		RS_SERIAL_PROCESS(mSslId);
		RS_SERIAL_PROCESS(mLocalPort);
		RS_SERIAL_PROCESS(mProfileName);
	}

	static BroadcastDiscoveryPack fromPeerDetails(const RsPeerDetails& pd)
	{
		BroadcastDiscoveryPack bdp;
		bdp.mSslId = pd.id;
		bdp.mLocalPort = pd.localPort;
		bdp.mProfileName = pd.name;
		return bdp;
	}

	static BroadcastDiscoveryPack fromSerializedString(const std::string& st)
	{
		RsGenericSerializer::SerializeContext ctx(
		            reinterpret_cast<uint8_t*>(const_cast<char*>(st.data())),
		            static_cast<uint32_t>(st.size()) );
		BroadcastDiscoveryPack bdp;
		bdp.serial_process(RsGenericSerializer::DESERIALIZE, ctx);
		return bdp;
	}

	std::string serializeToString()
	{
		/* After some experiments it seems very unlikely that UDP broadcast
		 * packets bigger then this could get trought a network */
		std::vector<uint8_t> buffer(512, 0);
		RsGenericSerializer::SerializeContext ctx(
		            buffer.data(), static_cast<uint32_t>(buffer.size()) );
		serial_process(RsGenericSerializer::SERIALIZE, ctx);
		return std::string(reinterpret_cast<char*>(buffer.data()), ctx.mOffset);
	}

	BroadcastDiscoveryPack(const BroadcastDiscoveryPack&) = default;
	~BroadcastDiscoveryPack() override;
};

BroadcastDiscoveryPack::~BroadcastDiscoveryPack() {};

BroadcastDiscoveryService::BroadcastDiscoveryService(
        RsPeers& pRsPeers ) :
    mDiscoveredDataMutex("BroadcastDiscoveryService discovered data mutex"),
    mRsPeers(pRsPeers)
{
	if(mRsPeers.isHiddenNode(mRsPeers.getOwnId())) return;

	mUdcParameters.set_can_discover(true);
	mUdcParameters.set_can_be_discovered(true);
	mUdcParameters.set_port(port);
	mUdcParameters.set_application_id(appId);

	mUdcEndpoint.Start(mUdcParameters, "");
	updatePublishedData();
}

BroadcastDiscoveryService::~BroadcastDiscoveryService()
{ mUdcEndpoint.Stop(true); }

std::vector<RsBroadcastDiscoveryResult>
BroadcastDiscoveryService::getDiscoveredPeers()
{
	std::vector<RsBroadcastDiscoveryResult> ret;

	RS_STACK_MUTEX(mDiscoveredDataMutex);
	for(auto&& pp: mDiscoveredData)
		ret.push_back(createResult(pp.first, pp.second));

	return ret;
}

void BroadcastDiscoveryService::updatePublishedData()
{
	RsPeerDetails od;
	mRsPeers.getPeerDetails(mRsPeers.getOwnId(), od);
	mUdcEndpoint.SetUserData(
	            BroadcastDiscoveryPack::fromPeerDetails(od).serializeToString());
}

void BroadcastDiscoveryService::data_tick()
{
	auto nextRunAt = std::chrono::system_clock::now() + std::chrono::seconds(5);

	if( mUdcParameters.can_discover() &&
	        !mRsPeers.isHiddenNode(mRsPeers.getOwnId()) )
	{
		auto currentEndpoints = mUdcEndpoint.ListDiscovered();
		std::map<UDC::IpPort, std::string> currentMap;
		std::map<UDC::IpPort, std::string> updateMap;

		mDiscoveredDataMutex.lock();
		for(auto&& dEndpoint: currentEndpoints)
		{
			currentMap[dEndpoint.ip_port()] = dEndpoint.user_data();

			auto findIt = mDiscoveredData.find(dEndpoint.ip_port());
			if( !dEndpoint.user_data().empty() && (
			            findIt == mDiscoveredData.end() ||
			            findIt->second != dEndpoint.user_data() ) )
				updateMap[dEndpoint.ip_port()] = dEndpoint.user_data();
		}
		mDiscoveredData = currentMap;
		mDiscoveredDataMutex.unlock();

		if(!updateMap.empty())
		{
			for (auto&& pp : updateMap)
			{
				RsBroadcastDiscoveryResult rbdr =
				        createResult(pp.first, pp.second);

				const bool isFriend = mRsPeers.isFriend(rbdr.mSslId);
				if( isFriend && rbdr.mLocator.hasPort() &&
				        !mRsPeers.isOnline(rbdr.mSslId) )
				{
					mRsPeers.setLocalAddress(
					            rbdr.mSslId, rbdr.mLocator.host(),
					            rbdr.mLocator.port() );
					mRsPeers.connectAttempt(rbdr.mSslId);
				}
				else if(!isFriend)
				{
					typedef RsBroadcastDiscoveryPeerFoundEvent Evt_t;

					// Ensure rsEvents is not deleted while we use it
					std::shared_ptr<RsEvents> lockedRsEvents = rsEvents;
					if(lockedRsEvents)
						lockedRsEvents->postEvent(
						            std::unique_ptr<Evt_t>(new Evt_t(rbdr)) );
				}
			}
		}
	}

	/* Probably this would be better if done only on actual change */
	if( mUdcParameters.can_be_discovered() &&
	        !mRsPeers.isHiddenNode(mRsPeers.getOwnId()) ) updatePublishedData();

	std::this_thread::sleep_until(nextRunAt);
}

RsBroadcastDiscoveryResult BroadcastDiscoveryService::createResult(
        const udpdiscovery::IpPort& ipp, const std::string& uData )
{
	BroadcastDiscoveryPack bdp =
	        BroadcastDiscoveryPack::fromSerializedString(uData);

	RsBroadcastDiscoveryResult rbdr;
	rbdr.mSslId = bdp.mSslId;
	rbdr.mProfileName = bdp.mProfileName;
	rbdr.mLocator.
	        setScheme("ipv4").
	        setHost(UDC::IpToString(ipp.ip())).
	        setPort(bdp.mLocalPort);

	return rbdr;
}
