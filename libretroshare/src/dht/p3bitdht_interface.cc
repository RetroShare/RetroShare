/*
 * libretroshare/src/dht: p3bitdht.h
 *
 * BitDht interface for RetroShare.
 *
 * Copyright 2009-2011 by Robert Fernie.
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

#include "util/rsnet.h"
#include "dht/p3bitdht.h"

#include "tcponudp/udprelay.h"
#include "bitdht/bdstddht.h"

#include <sstream>

void convertBdPeerToRsDhtPeer(RsDhtPeer &peer, const bdPeer &int_peer);
void convertDhtPeerDetailsToRsDhtNetPeer(RsDhtNetPeer &status, const DhtPeerDetails &details);
void convertUdpRelayEndtoRsDhtRelayEnd(RsDhtRelayEnd &end, const UdpRelayEnd &int_end);
void convertUdpRelayProxytoRsDhtRelayProxy(RsDhtRelayProxy &proxy, const UdpRelayProxy &int_proxy);


/***********************************************************************************************
 ********** External RsDHT Interface (defined in libretroshare/src/retroshare/rsdht.h) *********
************************************************************************************************/

uint32_t p3BitDht::getNetState(uint32_t type)
{

	return 1;
}

int      p3BitDht::getDhtPeers(int lvl, std::list<RsDhtPeer> &peers)
{
	/* this function we can actually implement! */
	bdBucket int_peers;
	std::list<bdPeer>::iterator it;
        mUdpBitDht->getDhtBucket(lvl, int_peers);

	for(it = int_peers.entries.begin(); it != int_peers.entries.end(); it++)
	{
		RsDhtPeer peer;

		convertBdPeerToRsDhtPeer(peer, *it);
		peer.mBucket = lvl;

		peers.push_back(peer);
	}
	return (int_peers.entries.size() > 0);
}

int      p3BitDht::getNetPeerList(std::list<std::string> &peerIds)
{
	RsStackMutex stack(dhtMtx); /*********** LOCKED **********/
	std::map<bdNodeId, DhtPeerDetails>::iterator it;
	for(it = mPeers.begin(); it != mPeers.end(); it++)
	{
		peerIds.push_back(it->second.mRsId);
	}

	return 1;
}

int      p3BitDht::getNetPeerStatus(std::string peerId, RsDhtNetPeer &status)
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

	for(it = int_relayEnds.begin(); it != int_relayEnds.end(); it++)
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

	for(it = int_relayProxies.begin(); it != int_relayProxies.end(); it++)
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

/***********************************************************************************************
 ********** External RsDHT Interface (defined in libretroshare/src/retroshare/rsdht.h) *********
************************************************************************************************/


void convertBdPeerToRsDhtPeer(RsDhtPeer &peer, const bdPeer &int_peer)
{
	std::ostringstream out;
	bdStdPrintNodeId(out, &(int_peer.mPeerId.id));

	peer.mDhtId = out.str();

	std::ostringstream addr;
	addr << rs_inet_ntoa(int_peer.mPeerId.addr.sin_addr) << ":" << ntohs(int_peer.mPeerId.addr.sin_port);
	peer.mAddr = addr.str();

	peer.mLastSendTime = int_peer.mLastSendTime;
	peer.mLastRecvTime = int_peer.mLastRecvTime;
	peer.mFoundTime = int_peer.mFoundTime;
	peer.mPeerFlags = int_peer.mPeerFlags;
	peer.mExtraFlags = int_peer.mExtraFlags;	
}



void	convertDhtPeerDetailsToRsDhtNetPeer(RsDhtNetPeer &status, const DhtPeerDetails &details)
{
	std::ostringstream out;
	bdStdPrintNodeId(out, &(details.mDhtId.id));

	status.mDhtId = out.str();
	status.mRsId = details.mRsId;

	status.mDhtState = details.mDhtState;

	status.mConnectState = details.mConnectLogic.connectState();

	status.mPeerReqState = details.mPeerReqState;

	status.mPeerConnectState = details.mPeerConnectState;

	switch(details.mPeerConnectMode)
	{
		default:
		case BITDHT_CONNECT_MODE_DIRECT:
			status.mPeerConnectMode = RSDHT_TOU_MODE_DIRECT;
			break;
		case BITDHT_CONNECT_MODE_PROXY:
			status.mPeerConnectMode = RSDHT_TOU_MODE_PROXY;
			break;
		case BITDHT_CONNECT_MODE_RELAY:
			status.mPeerConnectMode = RSDHT_TOU_MODE_RELAY;
			break;
	}

	//status.mPeerConnectProxyId = details.mPeerConnectProxyId;
	std::ostringstream out2;
	bdStdPrintId(out2, &(details.mPeerConnectProxyId));
	status.mPeerConnectProxyId = out2.str();

	status.mCbPeerMsg = details.mPeerCbMsg;

	return;
}

void	convertUdpRelayEndtoRsDhtRelayEnd(RsDhtRelayEnd &end, const UdpRelayEnd &int_end)
{
	std::ostringstream addr;
	addr << rs_inet_ntoa(int_end.mLocalAddr.sin_addr) << ":" << ntohs(int_end.mLocalAddr.sin_port);
	end.mLocalAddr = addr.str();

	addr.clear();
	addr << rs_inet_ntoa(int_end.mProxyAddr.sin_addr) << ":" << ntohs(int_end.mProxyAddr.sin_port);
	end.mProxyAddr = addr.str();


	addr.clear();
	addr << rs_inet_ntoa(int_end.mRemoteAddr.sin_addr) << ":" << ntohs(int_end.mRemoteAddr.sin_port);
	end.mRemoteAddr = addr.str();

	end.mCreateTS = 0;
	return;
}
	
void	convertUdpRelayProxytoRsDhtRelayProxy(RsDhtRelayProxy &proxy, const UdpRelayProxy &int_proxy)
{
	std::ostringstream addr;
	addr << rs_inet_ntoa(int_proxy.mAddrs.mSrcAddr.sin_addr) << ":" << ntohs(int_proxy.mAddrs.mSrcAddr.sin_port);
	proxy.mSrcAddr = addr.str();

	addr.clear();
	addr << rs_inet_ntoa(int_proxy.mAddrs.mDestAddr.sin_addr) << ":" << ntohs(int_proxy.mAddrs.mDestAddr.sin_port);
	proxy.mDestAddr = addr.str();

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
        //time_t mLastBandwidthTS;

}

	
