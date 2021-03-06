/*******************************************************************************
 * libretroshare/src/retroshare: rsdht.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RETROSHARE_DHT_GUI_INTERFACE_H
#define RETROSHARE_DHT_GUI_INTERFACE_H

#include <inttypes.h>
#include <string>
#include <list>
#include <retroshare/rstypes.h>
#include "util/rsnet.h"
#include "retroshare/rsflags.h"

/* The Main Interface Class - for information about your Peers */
class RsDht;
extern RsDht *rsDht;

//std::ostream &operator<<(std::ostream &out, const RsPhotoShowDetails &detail);
//std::ostream &operator<<(std::ostream &out, const RsPhotoDetails &detail);

enum class RsDhtPeerType : uint8_t
{
	ANY		= 0,
	OTHER	= 1,
	FOF		= 2,
	FRIEND	= 3
};

enum class RsDhtPeerDht : uint8_t
{
	NOT_ACTIVE	= 0,
	SEARCHING	= 1,
	FAILURE		= 2,
	OFFLINE		= 3,
	UNREACHABLE	= 4,
	ONLINE		= 5
};

enum class RsDhtPeerConnectState : uint8_t
{
	DISCONNECTED	= 1,
	UDP_STARTED		= 2,
	CONNECTED		= 3
};

enum class RsDhtPeerRequest : uint8_t
{
	STOPPED	= 1,
	RUNNING = 2
};

enum class RsDhtTouMode : uint8_t
{
	NONE	= 0,
	DIRECT	= 1,
	PROXY	= 2,
	RELAY	= 3
};

enum class RsDhtRelayClass : uint8_t
{
	ALL			= 0,
	GENERAL		= 1,
	FOF			= 2,
	FRIENDS		= 3,

	NUM_CLASS	= 4
};

enum class RsDhtRelayMode : uint16_t
{
	DISABLED= 0x0000,
	ENABLED	= 0x0001,

	MASK	= 0x00f0,
	OFF		= 0x0010,
	ON		= 0x0020,
	SERVER	= 0x0040
};
RS_REGISTER_ENUM_FLAGS_TYPE(RsDhtRelayMode)

class RsDhtPeer
{
	public:
	RsDhtPeer();

	int mBucket;
        std::string mDhtId;
        std::string mAddr;
        rstime_t mLastSendTime;
        rstime_t mLastRecvTime;
        rstime_t mFoundTime;
        uint32_t mPeerFlags;
        uint32_t mExtraFlags;   
};

class RsDhtNetPeer
{
	public:
	RsDhtNetPeer();

        std::string mDhtId;
        RsPeerId mRsId;

	RsDhtPeerType mPeerType;
	RsDhtPeerDht mDhtState;

	std::string mConnectState; 	// connectLogic.

	RsDhtPeerConnectState mPeerConnectState;
	RsDhtTouMode mPeerConnectMode;
	bool  mExclusiveProxyLock;

	std::string mPeerConnectProxyId;

	RsDhtPeerRequest mPeerReqState;
	std::string mCbPeerMsg; 	// Peer Cb Mgs.

};

class RsDhtRelayEnd
{
	public:

        RsDhtRelayEnd();
        std::string mLocalAddr;
        std::string mProxyAddr;
        std::string mRemoteAddr;
        rstime_t mCreateTS;
};

class RsDhtRelayProxy
{
        public:
        RsDhtRelayProxy();

        std::string mSrcAddr;
        std::string mDestAddr;

        double mBandwidth;
        int mRelayClass;
        rstime_t mLastTS;
        rstime_t mCreateTS;

        //uint32_t mDataSize;
        //rstime_t mLastBandwidthTS;

};
class RsDhtFilteredPeer
{
public:
    struct sockaddr_in mAddr;
    uint32_t mFilterFlags; /* reasons why we are filtering */
    rstime_t mFilterTS;
    rstime_t mLastSeen;
};

class RsDht
{
	public:

	RsDht()  { return; }
virtual ~RsDht() { return; }

virtual uint32_t getNetState(uint32_t type) = 0;
virtual int 	 getDhtPeers(int lvl, std::list<RsDhtPeer> &peers) = 0;
virtual int 	 getNetPeerList(std::list<RsPeerId> &peerIds) = 0;
virtual int 	 getNetPeerStatus(const RsPeerId& peerId, RsDhtNetPeer &status) = 0;

virtual int 	getRelayEnds(std::list<RsDhtRelayEnd> &relayEnds) = 0;
virtual int 	getRelayProxies(std::list<RsDhtRelayProxy> &relayProxies) = 0;

//virtual int 	 getNetFailedPeer(std::string peerId, PeerStatus &status);

virtual std::string getUdpAddressString() = 0;

virtual void    getDhtRates(float &read, float &write) = 0;
virtual void    getRelayRates(float &read, float &write, float &relay) = 0;

	// Interface for controlling Relays & DHT Relay Mode 
virtual int 	getRelayServerList(std::list<std::string> &ids) = 0;
virtual int 	addRelayServer(std::string ids) = 0;
virtual int 	removeRelayServer(std::string ids) = 0;

virtual	RsDhtRelayMode getRelayMode() = 0;
virtual	int	 setRelayMode(RsDhtRelayMode mode) = 0;

virtual int	getRelayAllowance(RsDhtRelayClass classIdx, uint32_t &count, uint32_t &bandwidth) = 0;
virtual int	setRelayAllowance(RsDhtRelayClass classIdx, uint32_t  count, uint32_t  bandwidth) = 0;

	// So we can provide to clients.
virtual bool    getOwnDhtId(std::string &ownDhtId) = 0;

virtual bool	isAddressBanned(const struct sockaddr_storage& raddr) =0;
virtual void	getListOfBannedIps(std::list<RsDhtFilteredPeer>& lst) =0;

#if 0
virtual std::string getPeerStatusString();
virtual std::string getDhtStatusString();

virtual int get_dht_queries(std::map<bdNodeId, bdQueryStatus> &queries);
virtual int get_query_status(std::string id, bdQuerySummary &query);

virtual int get_peer_status(std::string peerId, PeerStatus &status);

virtual int get_net_failedpeers(std::list<std::string> &peerIds);
virtual int get_failedpeer_status(std::string peerId, PeerStatus &status);
#endif

};

#endif
