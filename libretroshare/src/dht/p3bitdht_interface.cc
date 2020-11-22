/*******************************************************************************
 * libretroshare/src/dht: p3bitdht_interface.cc                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2009-2011 by Robert Fernie <drbob@lunamutt.com>                   *
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

#include "util/rsnet.h"
#include "dht/p3bitdht.h"

#include "tcponudp/udprelay.h"
#include "tcponudp/udpstunner.h"
#include "bitdht/bdstddht.h"


void convertBdPeerToRsDhtPeer(RsDhtPeer &peer, const bdPeer &int_peer);
void convertDhtPeerDetailsToRsDhtNetPeer(RsDhtNetPeer &status, const DhtPeerDetails &details);
void convertUdpRelayEndtoRsDhtRelayEnd(RsDhtRelayEnd &end, const UdpRelayEnd &int_end);
void convertUdpRelayProxytoRsDhtRelayProxy(RsDhtRelayProxy &proxy, const UdpRelayProxy &int_proxy);


/***********************************************************************************************
 ********** External RsDHT Interface (defined in libretroshare/src/retroshare/rsdht.h) *********
************************************************************************************************/

uint32_t p3BitDht::getNetState(uint32_t /*type*/)
{

	return 1;
}

int      p3BitDht::getDhtPeers(int lvl, std::list<RsDhtPeer> &peers)
{
	/* this function we can actually implement! */
	bdBucket int_peers;
	std::list<bdPeer>::iterator it;
        mUdpBitDht->getDhtBucket(lvl, int_peers);

	for(it = int_peers.entries.begin(); it != int_peers.entries.end(); ++it)
	{
		RsDhtPeer peer;

		convertBdPeerToRsDhtPeer(peer, *it);
		peer.mBucket = lvl;

		peers.push_back(peer);
	}
	return (int_peers.entries.size() > 0);
}

int      p3BitDht::getNetPeerList(std::list<RsPeerId> &peerIds)
{
	RsStackMutex stack(dhtMtx); /*********** LOCKED **********/
	std::map<bdNodeId, DhtPeerDetails>::iterator it;
	for(it = mPeers.begin(); it != mPeers.end(); ++it)
	{
		peerIds.push_back(it->second.mRsId);
	}

	return 1;
}

int      p3BitDht::getNetPeerStatus(const RsPeerId& peerId, RsDhtNetPeer &status)
{

	RsStackMutex stack(dhtMtx); /*********** LOCKED **********/

	DhtPeerDetails *dpd = findInternalRsPeer_locked(peerId);
	if (!dpd)
	{
		return 0;
	}

	convertDhtPeerDetailsToRsDhtNetPeer(status, *dpd);
	return 1;
}


int     p3BitDht::getRelayEnds(std::list<RsDhtRelayEnd> &relayEnds)
{
	/* no need for mutex as mRelayReceiver is protected */
	if (!mRelay)
	{
		return 0;
	}

	std::list<UdpRelayEnd> int_relayEnds;
	std::list<UdpRelayEnd>::iterator it;
        mRelay->getRelayEnds(int_relayEnds);

	for(it = int_relayEnds.begin(); it != int_relayEnds.end(); ++it)
	{
		RsDhtRelayEnd end;
		convertUdpRelayEndtoRsDhtRelayEnd(end, *it);

		relayEnds.push_back(end);
	}
	return 1;
}


int     p3BitDht::getRelayProxies(std::list<RsDhtRelayProxy> &relayProxies)
{
	/* no need for mutex as mRelayReceiver is protected */
	if (!mRelay)
	{
		return 0;
	}

	std::list<UdpRelayProxy> int_relayProxies;
	std::list<UdpRelayProxy>::iterator it;
        mRelay->getRelayProxies(int_relayProxies);

	for(it = int_relayProxies.begin(); it != int_relayProxies.end(); ++it)
	{
		RsDhtRelayProxy proxy;
		convertUdpRelayProxytoRsDhtRelayProxy(proxy, *it);
		relayProxies.push_back(proxy);
	}
	return 1;
}


#if 0
int      p3BitDht::getNetFailedPeer(std::string peerId, PeerStatus &status)
{
	return 1;
}
#endif

std::string p3BitDht::getUdpAddressString()
{
	std::string out;
#ifdef RS_USE_DHT_STUNNER
	struct sockaddr_in extAddr;
	uint8_t extStable;

	if (mDhtStunner->externalAddr(extAddr, extStable))
	{
		rs_sprintf_append(out, " DhtExtAddr: %s:%u", rs_inet_ntoa(extAddr.sin_addr).c_str(), ntohs(extAddr.sin_port));
		
		if (extStable)
		{
			out += " (Stable) ";
		}
		else
		{
			out += " (Unstable) ";
		}
	}
	else
	{
		out += " DhtExtAddr: Unknown ";
	}
	if (mProxyStunner->externalAddr(extAddr, extStable))
	{
		rs_sprintf_append(out, " ProxyExtAddr: %s:%u", rs_inet_ntoa(extAddr.sin_addr).c_str(), ntohs(extAddr.sin_port));
		
		if (extStable)
		{
			out += " (Stable) ";
		}
		else
		{
			out += " (Unstable) ";
		}
	}
	else
	{
		out += " ProxyExtAddr: Unknown ";
	}
#else // RS_USE_DHT_STUNNER
	// whitespaces taken from above
	out = " DhtExtAddr: Unknown  ProxyExtAddr: Unknown ";
#endif // RS_USE_DHT_STUNNER
	return out;
}

void    p3BitDht::updateDataRates()
{
	uint32_t relayRead = 0;
	uint32_t relayWrite = 0;
	uint32_t relayRelay = 0;
	uint32_t dhtRead = 0;
	uint32_t dhtWrite = 0;

	mRelay->getDataTransferred(relayRead, relayWrite, relayRelay);
	mUdpBitDht->getDataTransferred(dhtRead, dhtWrite);

	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	rstime_t now = time(NULL);
	float period = now - mLastDataRateUpdate;

#define RATE_FACTOR (0.75)
	
	mRelayReadRate *= RATE_FACTOR;
	mRelayReadRate += (1.0 - RATE_FACTOR) * (relayRead / period);

	mRelayWriteRate *= RATE_FACTOR;
	mRelayWriteRate += (1.0 - RATE_FACTOR) * (relayWrite / period);

	mRelayRelayRate *= RATE_FACTOR;
	mRelayRelayRate += (1.0 - RATE_FACTOR) * (relayRelay / period);

	mDhtReadRate *= RATE_FACTOR;
	mDhtReadRate += (1.0 - RATE_FACTOR) * (dhtRead / period);

	mDhtWriteRate *= RATE_FACTOR;
	mDhtWriteRate += (1.0 - RATE_FACTOR) * (dhtWrite / period);

	mLastDataRateUpdate = now;

}

void    p3BitDht::clearDataRates()
{
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	mRelayReadRate = 0;
	mRelayWriteRate = 0;
	mRelayRelayRate = 0;
	mDhtReadRate  = 0;
	mDhtWriteRate = 0;

	mLastDataRateUpdate = time(NULL);
}


/* in kB/s */
void    p3BitDht::getDhtRates(float &read, float &write)
{
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	read = mDhtReadRate / 1024.0;
	write = mDhtWriteRate / 1024.0;
}

void    p3BitDht::getRelayRates(float &read, float &write, float &relay)
{
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	read = mRelayReadRate / 1024.0;
	write = mRelayWriteRate / 1024.0;
	relay = mRelayRelayRate / 1024.0;
}



/***********************************************************************************************
 ********** External RsDHT Interface (defined in libretroshare/src/retroshare/rsdht.h) *********
************************************************************************************************/

void convertBdPeerToRsDhtPeer(RsDhtPeer &peer, const bdPeer &int_peer)
{
	bdStdPrintNodeId(peer.mDhtId, &(int_peer.mPeerId.id), false);

	rs_sprintf(peer.mAddr, "%s:%u", rs_inet_ntoa(int_peer.mPeerId.addr.sin_addr).c_str(), ntohs(int_peer.mPeerId.addr.sin_port));

	peer.mLastSendTime = int_peer.mLastSendTime;
	peer.mLastRecvTime = int_peer.mLastRecvTime;
	peer.mFoundTime = int_peer.mFoundTime;
	peer.mPeerFlags = int_peer.mPeerFlags;
	peer.mExtraFlags = int_peer.mExtraFlags;	
}



void	convertDhtPeerDetailsToRsDhtNetPeer(RsDhtNetPeer &status, const DhtPeerDetails &details)
{
	bdStdPrintId(status.mDhtId, &(details.mDhtId), false);

	status.mRsId = details.mRsId;

	status.mPeerType = details.mPeerType;

	status.mDhtState = details.mDhtState;

	status.mConnectState = details.mConnectLogic.connectState();

	status.mPeerReqState = details.mPeerReqState;

	status.mExclusiveProxyLock = details.mExclusiveProxyLock;

	status.mPeerConnectState = details.mPeerConnectState;

	switch(details.mPeerConnectMode)
	{
		default:
		    status.mPeerConnectMode = RsDhtTouMode::NONE;
			break;
		case BITDHT_CONNECT_MODE_DIRECT:
		    status.mPeerConnectMode = RsDhtTouMode::DIRECT;
			break;
		case BITDHT_CONNECT_MODE_PROXY:
		    status.mPeerConnectMode = RsDhtTouMode::PROXY;
			break;
		case BITDHT_CONNECT_MODE_RELAY:
		    status.mPeerConnectMode = RsDhtTouMode::RELAY;
			break;
	}

	//status.mPeerConnectProxyId = details.mPeerConnectProxyId;
	bdStdPrintId(status.mPeerConnectProxyId, &(details.mPeerConnectProxyId), false);

	status.mCbPeerMsg = details.mPeerCbMsg;

	return;
}

void	convertUdpRelayEndtoRsDhtRelayEnd(RsDhtRelayEnd &end, const UdpRelayEnd &int_end)
{
	rs_sprintf(end.mLocalAddr, "%s:%u", rs_inet_ntoa(int_end.mLocalAddr.sin_addr).c_str(), ntohs(int_end.mLocalAddr.sin_port));
	rs_sprintf(end.mProxyAddr, "%s:%u", rs_inet_ntoa(int_end.mProxyAddr.sin_addr).c_str(), ntohs(int_end.mProxyAddr.sin_port));
	rs_sprintf(end.mRemoteAddr, "%s:%u", rs_inet_ntoa(int_end.mRemoteAddr.sin_addr).c_str(), ntohs(int_end.mRemoteAddr.sin_port));

	end.mCreateTS = 0;
	return;
}
	
void	convertUdpRelayProxytoRsDhtRelayProxy(RsDhtRelayProxy &proxy, const UdpRelayProxy &int_proxy)
{
	rs_sprintf(proxy.mSrcAddr, "%s:%u", rs_inet_ntoa(int_proxy.mAddrs.mSrcAddr.sin_addr).c_str(), ntohs(int_proxy.mAddrs.mSrcAddr.sin_port));
	rs_sprintf(proxy.mDestAddr, "%s:%u", rs_inet_ntoa(int_proxy.mAddrs.mDestAddr.sin_addr).c_str(), ntohs(int_proxy.mAddrs.mDestAddr.sin_port));

    proxy.mBandwidth = int_proxy.mBandwidth;
    proxy.mRelayClass = int_proxy.mRelayClass;
    proxy.mLastTS = int_proxy.mLastTS;
	proxy.mCreateTS = 0;

    //proxy.mDataSize = int_proxy.mDataSize;
    //proxy.mLastBandwidthTS = int_proxy.mLastBandwidthTS;
}

RsDhtPeer::RsDhtPeer()
{
		mBucket = 0;

		//std::string mDhtId;
        mLastSendTime = 0;
        mLastRecvTime = 0;
        mFoundTime = 0;
        mPeerFlags = 0;
        mExtraFlags = 0;
}


RsDhtNetPeer::RsDhtNetPeer() 
{
	return;
}


RsDhtRelayEnd::RsDhtRelayEnd()
{
        mCreateTS = 0;
}


RsDhtRelayProxy::RsDhtRelayProxy()
{
        mBandwidth = 0;
        mRelayClass = 0;
        mLastTS = 0;
        mCreateTS = 0;

        //uint32_t mDataSize;
        //rstime_t mLastBandwidthTS;

}

	
