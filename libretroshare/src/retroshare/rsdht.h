#ifndef RETROSHARE_DHT_GUI_INTERFACE_H
#define RETROSHARE_DHT_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsdht.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2011-2011 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <inttypes.h>
#include <string>
#include <list>

/* The Main Interface Class - for information about your Peers */
class RsDht;
extern RsDht *rsDht;


//std::ostream &operator<<(std::ostream &out, const RsPhotoShowDetails &detail);
//std::ostream &operator<<(std::ostream &out, const RsPhotoDetails &detail);


#define RSDHT_NETSTART_NETWORKMODE	0x0001
#define RSDHT_NETSTART_NATTYPE		0x0002
#define RSDHT_NETSTART_NATHOLE		0x0003
#define RSDHT_NETSTART_CONNECTMODES	0x0004
#define RSDHT_NETSTART_NETSTATE		0x0005



#define RSDHT_PEERTYPE_ANY		0x0000
#define RSDHT_PEERTYPE_OTHER		0x0001
#define RSDHT_PEERTYPE_FOF		0x0002
#define RSDHT_PEERTYPE_FRIEND		0x0003

#define RSDHT_PEERDHT_NOT_ACTIVE	0x0000
#define RSDHT_PEERDHT_SEARCHING		0x0001
#define RSDHT_PEERDHT_FAILURE		0x0002
#define RSDHT_PEERDHT_OFFLINE		0x0003
#define RSDHT_PEERDHT_UNREACHABLE	0x0004
#define RSDHT_PEERDHT_ONLINE		0x0005

#define RSDHT_PEERCONN_DISCONNECTED               1
#define RSDHT_PEERCONN_UDP_STARTED                2
#define RSDHT_PEERCONN_CONNECTED                  3

#define RSDHT_PEERREQ_STOPPED                     1
#define RSDHT_PEERREQ_RUNNING                     2

#define RSDHT_TOU_MODE_DIRECT		1
#define RSDHT_TOU_MODE_PROXY		2
#define RSDHT_TOU_MODE_RELAY		3


#define RSDHT_RELAY_NUM_CLASS             4

#define RSDHT_RELAY_CLASS_ALL             0
#define RSDHT_RELAY_CLASS_GENERAL         1
#define RSDHT_RELAY_CLASS_FOF             2
#define RSDHT_RELAY_CLASS_FRIENDS         3


#define RSDHT_RELAY_MODE_MASK		0x00f0

#define RSDHT_RELAY_ENABLED		0x0001

#define RSDHT_RELAY_MODE_OFF		0x0010
#define RSDHT_RELAY_MODE_ON		0x0020
#define RSDHT_RELAY_MODE_SERVER		0x0040


class RsDhtPeer
{
	public:
	RsDhtPeer();

	int mBucket;
        std::string mDhtId;
        std::string mAddr;
        time_t mLastSendTime;
        time_t mLastRecvTime;
        time_t mFoundTime;
        uint32_t mPeerFlags;
        uint32_t mExtraFlags;   
};

class RsDhtNetPeer
{
	public:
	RsDhtNetPeer();

        std::string mDhtId;
        std::string mRsId;

        uint32_t mPeerType;
	uint32_t mDhtState;

	std::string mConnectState; 	// connectLogic.

	uint32_t mPeerConnectState; 	// connect Status
	uint32_t mPeerConnectMode; 	// connect mode
	bool  mExclusiveProxyLock;

	std::string mPeerConnectProxyId;

	uint32_t mPeerReqState; 	// Req Status.
	std::string mCbPeerMsg; 	// Peer Cb Mgs.

};

class RsDhtRelayEnd
{
	public:

        RsDhtRelayEnd();
        std::string mLocalAddr;
        std::string mProxyAddr;
        std::string mRemoteAddr;
        time_t mCreateTS;
};

class RsDhtRelayProxy
{
        public:
        RsDhtRelayProxy();

        std::string mSrcAddr;
        std::string mDestAddr;

        double mBandwidth;
        int mRelayClass;
        time_t mLastTS;
        time_t mCreateTS;

        //uint32_t mDataSize;
        //time_t mLastBandwidthTS;

};


class RsDht
{
	public:

	RsDht()  { return; }
virtual ~RsDht() { return; }

virtual uint32_t getNetState(uint32_t type) = 0;
virtual int 	 getDhtPeers(int lvl, std::list<RsDhtPeer> &peers) = 0;
virtual int 	 getNetPeerList(std::list<std::string> &peerIds) = 0;
virtual int 	 getNetPeerStatus(std::string peerId, RsDhtNetPeer &status) = 0;

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

virtual	uint32_t getRelayMode() = 0;
virtual	int	 setRelayMode(uint32_t mode) = 0;

virtual int	getRelayAllowance(int  classIdx, uint32_t &count, uint32_t &bandwidth) = 0;
virtual int	setRelayAllowance(int classIdx, uint32_t  count, uint32_t  bandwidth) = 0;


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
