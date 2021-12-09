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

#ifdef __ANDROID__
#	include "rs_android/retroshareserviceandroid.hpp"
#endif // def __ANDROID__


/*extern*/ RsBroadcastDiscovery* rsBroadcastDiscovery = nullptr;

struct BroadcastDiscoveryPack : RsSerializable
{
	BroadcastDiscoveryPack() : mLocalPort(0) {}

	RsPgpFingerprint mPgpFingerprint;
	RsPeerId mSslId;
	uint16_t mLocalPort;
	std::string mProfileName;

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx ) override
	{
		RS_SERIAL_PROCESS(mPgpFingerprint);
		RS_SERIAL_PROCESS(mSslId);
		RS_SERIAL_PROCESS(mLocalPort);
		RS_SERIAL_PROCESS(mProfileName);
	}

	static BroadcastDiscoveryPack fromPeerDetails(const RsPeerDetails& pd)
	{
		BroadcastDiscoveryPack bdp;
		bdp.mPgpFingerprint = pd.fpr;
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


BroadcastDiscoveryService::BroadcastDiscoveryService(
        RsPeers& pRsPeers ) :
    mDiscoveredDataMutex("BroadcastDiscoveryService discovered data mutex"),
    mRsPeers(pRsPeers)
{
	if(mRsPeers.isHiddenNode(mRsPeers.getOwnId())) return;

#ifdef __ANDROID__
	createAndroidMulticastLock();
#endif // def __ANDROID__

	enableMulticastListening();

	mUdcParameters.set_can_discover(true);
	mUdcParameters.set_can_be_discovered(true);
	mUdcParameters.set_port(port);
	mUdcParameters.set_application_id(appId);

	mUdcPeer.Start(mUdcParameters, "");
	updatePublishedData();
}

BroadcastDiscoveryService::~BroadcastDiscoveryService()
{
	mUdcPeer.Stop(true);
	disableMulticastListening();
}

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
	mUdcPeer.SetUserData(
	            BroadcastDiscoveryPack::fromPeerDetails(od).serializeToString());
}

void BroadcastDiscoveryService::threadTick()
{
	if( mUdcParameters.can_discover() &&
	        !mRsPeers.isHiddenNode(mRsPeers.getOwnId()) )
	{
		auto currentEndpoints = mUdcPeer.ListDiscovered();
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
					if(rsEvents)
                    {
						auto ev = std::make_shared<RsBroadcastDiscoveryEvent>();

                        ev->mDiscoveryEventType = RsBroadcastDiscoveryEventType::PEER_FOUND;
                        ev->mData = rbdr;

						rsEvents->postEvent(ev);
                    }
				}
			}
		}
	}

	/* Probably this would be better if done only on actual change */
	if( mUdcParameters.can_be_discovered() &&
	        !mRsPeers.isHiddenNode(mRsPeers.getOwnId()) ) updatePublishedData();

    // This avoids waiting 5 secs when the thread should actually terminate (when RS closes).
    for(uint32_t i=0;i<10;++i)
    {
        if(shouldStop())
            return;
        rstime::rs_usleep(500*1000); // sleep for 0.5 sec.
    }
}

RsBroadcastDiscoveryResult BroadcastDiscoveryService::createResult(
        const udpdiscovery::IpPort& ipp, const std::string& uData )
{
	BroadcastDiscoveryPack bdp =
	        BroadcastDiscoveryPack::fromSerializedString(uData);

	RsBroadcastDiscoveryResult rbdr;
	rbdr.mPgpFingerprint = bdp.mPgpFingerprint;
	rbdr.mSslId = bdp.mSslId;
	rbdr.mProfileName = bdp.mProfileName;
	rbdr.mLocator.
	        setScheme("ipv4").
	        setHost(UDC::IpToString(ipp.ip())).
	        setPort(bdp.mLocalPort);

	return rbdr;
}

bool BroadcastDiscoveryService::isMulticastListeningEnabled()
{
#ifdef __ANDROID__
	if(!mAndroidWifiMulticastLock)
	{
		RS_ERR("Android multicast lock not initialized!");
		return false;
	}

	auto uenv = jni::GetAttachedEnv(RsJni::getVM());
	JNIEnv& env = *uenv;
	auto& multicastLockClass = jni::Class<AndroidMulticastLock>::Singleton(env);

	auto isHeld =
	        multicastLockClass.GetMethod<jni::jboolean()>(
	            env, "isHeld" );

	return mAndroidWifiMulticastLock.Call(env, isHeld);
#else // def __ANDROID__
	return true;
#endif // def __ANDROID__
}

bool BroadcastDiscoveryService::enableMulticastListening()
{
#ifdef __ANDROID__
	if(!mAndroidWifiMulticastLock)
	{
		RS_ERR("Android multicast lock not initialized!");
		return false;
	}

	if(!isMulticastListeningEnabled())
	{
		auto uenv = jni::GetAttachedEnv(RsJni::getVM());
		JNIEnv& env = *uenv;
		auto& multicastLockClass = jni::Class<AndroidMulticastLock>::Singleton(env);

		auto acquire =
		        multicastLockClass.GetMethod<void()>(
		            env, "acquire" );

		mAndroidWifiMulticastLock.Call(env, acquire);

		return true;
	}
#endif // def __ANDROID__

	return false;
}

bool BroadcastDiscoveryService::disableMulticastListening()
{
#ifdef __ANDROID__
	if(!mAndroidWifiMulticastLock)
	{
		RS_ERR("Android multicast lock not initialized!");
		return false;
	}

	if(isMulticastListeningEnabled())
	{
		auto uenv = jni::GetAttachedEnv(RsJni::getVM());
		JNIEnv& env = *uenv;
		auto& multicastLockClass = jni::Class<AndroidMulticastLock>::Singleton(env);

		auto release =
		        multicastLockClass.GetMethod<void()>(
		            env, "release" );

		mAndroidWifiMulticastLock.Call(env, release);

		return true;
	}
#endif // def __ANDROID__

	return false;
}

#ifdef __ANDROID__

bool BroadcastDiscoveryService::createAndroidMulticastLock()
{
	if(mAndroidWifiMulticastLock)
	{
		RS_ERR("Android multicast lock is already initialized");
		print_stacktrace();
		return false;
	}

	auto uenv = jni::GetAttachedEnv(RsJni::getVM());
	JNIEnv& env = *uenv;

	using AContextTag = RetroShareServiceAndroid::Context;
	using AContext = jni::Class<AContextTag>;
	static auto& contextClass = AContext::Singleton(env);

	auto wifiServiceField = jni::StaticField<AContextTag, jni::String>(
	            env, contextClass, "WIFI_SERVICE");

	jni::Local<jni::String> WIFI_SERVICE = contextClass.Get(
	            env, wifiServiceField );

	auto androidContext = RetroShareServiceAndroid::getAndroidContext(env);

	auto getSystemService =
	        contextClass.GetMethod<jni::Object<jni::ObjectTag> (jni::String)>(
	            env, "getSystemService" );

	struct WifiManager
	{ static constexpr auto Name() { return "android/net/wifi/WifiManager"; } };

	auto& wifiManagerClass = jni::Class<WifiManager>::Singleton(env);

	auto wifiManager = jni::Cast<WifiManager>(
	            env, wifiManagerClass,
	            androidContext.Call(env, getSystemService, WIFI_SERVICE) );

	auto createMulticastLock =
	        wifiManagerClass.GetMethod<jni::Object<AndroidMulticastLock>(jni::String)>(
	            env, "createMulticastLock" );

	mAndroidWifiMulticastLock = jni::NewGlobal(
	            env, wifiManager.Call(
	                env, createMulticastLock,
	                jni::Make<jni::String>(
	                    env, "RetroShare BroadcastDiscoveryService" ) ) );

	return true;
}

#endif // def __ANDROID__

RsBroadcastDiscovery::~RsBroadcastDiscovery() = default;
RsBroadcastDiscoveryResult::~RsBroadcastDiscoveryResult() = default;
BroadcastDiscoveryPack::~BroadcastDiscoveryPack() = default;
