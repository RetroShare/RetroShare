/*
 * tcponudp/udprelay.cc
 *
 * libretroshare.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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

#include "udppeer.h"
#include <iostream>

/*
 * #define DEBUG_UDP_PEER 1
 */


UdpPeerReceiver::UdpPeerReceiver(UdpPublisher *pub)
	:UdpSubReceiver(pub)
{
	return;
}

/* higher level interface */
int UdpPeerReceiver::recvPkt(void *data, int size, struct sockaddr_in &from)
{
	/* print packet information */
#ifdef DEBUG_UDP_PEER
	std::cerr << "UdpPeerReceiver::recvPkt(" << size << ") from: " << from;
	std::cerr << std::endl;
#endif

        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/

	/* look for a peer */
        std::map<struct sockaddr_in, UdpPeer *>::iterator it;
	it = streams.find(from);

	if (it == streams.end())
	{
		/* peer unknown */
#ifdef DEBUG_UDP_PEER
		std::cerr << "UdpPeerReceiver::recvPkt() Peer Unknown!";
		std::cerr << std::endl;
#endif
		return 0;
	}
	else
	{
		/* forward to them */
#ifdef DEBUG_UDP_PEER
		std::cerr << "UdpPeerReceiver::recvPkt() Sending to UdpPeer: ";
		std::cerr << it->first;
		std::cerr << std::endl;
#endif
		(it->second)->recvPkt(data, size);
		return 1;
	}
	/* done */
}

	
int     UdpPeerReceiver::status(std::ostream &out)
{
        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/

	out << "UdpPeerReceiver::status()" << std::endl;
	out << "UdpPeerReceiver::peers:" << std::endl;
        std::map<struct sockaddr_in, UdpPeer *>::iterator it;
	for(it = streams.begin(); it != streams.end(); it++)
	{
		out << "\t" << it->first << std::endl;
	}
	out << std::endl;

	return 1;
}

        /* add a TCPonUDP stream */
int UdpPeerReceiver::addUdpPeer(UdpPeer *peer, const struct sockaddr_in &raddr)
{
        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/


	/* check for duplicate */
        std::map<struct sockaddr_in, UdpPeer *>::iterator it;
	it = streams.find(raddr);
	bool ok = (it == streams.end());
	if (!ok)
	{
#ifdef DEBUG_UDP_PEER
		std::cerr << "UdpPeerReceiver::addUdpPeer() Peer already exists!" << std::endl;
		std::cerr << "UdpPeerReceiver::addUdpPeer() ERROR" << std::endl;
#endif
	}
	else
	{
		streams[raddr] = peer;
	}

	return ok;
}

int UdpPeerReceiver::removeUdpPeer(UdpPeer *peer)
{
        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/

	/* check for duplicate */
        std::map<struct sockaddr_in, UdpPeer *>::iterator it;
	for(it = streams.begin(); it != streams.end(); it++)
	{
		if (it->second == peer)
		{
#ifdef DEBUG_UDP_PEER
			std::cerr << "UdpPeerReceiver::removeUdpPeer() SUCCESS" << std::endl;
#endif
			streams.erase(it);
			return 1;
		}
	}

#ifdef DEBUG_UDP_PEER
	std::cerr << "UdpPeerReceiver::removeUdpPeer() ERROR" << std::endl;
#endif
	return 0;
}





/****************** UDP RELAY STUFF **********/

int UdpRelayReciever::checkRelays()
{
        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/

	/* iterate through the Relays */

	out << "UdpRelayReceiver::checkRelays()" << std::endl;

        std::list<UdpRelayAddrSet> eraseList;
        std::map<UdpRelayAddrSet, UdpRelay>::iterator rit;
	for(rit = mRelays.begin(); rit != mRelays.end(); rit++)
	{
		/* calc bandwidth */
		rit->second.mBandwidth = rit->second.mDataSize / (float) (now - rit->second.mLastBandwidthTS);
		rit->second.mDataSize = 0;
		rit->second.mLastBandwidthTS = now;

		if (rit->second.mBandwidth > RELAY_MAX_BANDWIDTH)
		{
			/* if exceeding bandwidth -> drop */
			eraseList.push_back(rit->first);
		}
		else if (now - rit->second.mLastTS > RELAY_TIMEOUT)
		{
			/* if haven't transmitted for ages -> drop */
			out << "\t" << rit->first << " : " << rit->second;
			out << std::endl;
			eraseList.push_back(rit->first);
		}
	}

        std::list<UdpRelayAddrSet>::iterator it;
	for(it = eraseList.begin(); it != eraseList.end(); it++)
	{
		/* find in Relay list */

		/* rotate around and delete matching set */
	}
}


int UdpRelayReciever::addUdpRelay(UdpRelayAddrSet *addrSet)
{
        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/


}

int UdpRelayReciever::removeUdpRelay(UdpRelayAddrSet *addrSet)
{
        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/


}

int UdpRelayReciever::addRelayUdpPeer(UdpPeer *peer, UdpRelayAddrSet *addrSet)
{
        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/



}

int UdpRelayReceiver::removeRelayUdpPeer(UdpPeer *peer)
{
        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/

	/* check for duplicate */
        std::map<UdpRelayAddrSet, UdpPeer *>::iterator it;
	for(it = mStreams.begin(); it != mStreams.end(); it++)
	{
		if (it->second == peer)
		{
#ifdef DEBUG_UDP_RELAY
			std::cerr << "UdpRelayReceiver::removeUdpPeer() SUCCESS" << std::endl;
#endif
			mStreams.erase(it);
			return 1;
		}
	}

#ifdef DEBUG_UDP_RELAY
	std::cerr << "UdpRelayReceiver::removeUdpPeer() ERROR" << std::endl;
#endif
	return 0;
}

	
int     UdpRelayReceiver::status(std::ostream &out)
{
        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/

	out << "UdpRelayReceiver::status()" << std::endl;
	out << "UdpRelayReceiver::Relayed Connections:" << std::endl;

        std::map<UdpRelayAddrSet, UdpRelay>::iterator rit;

	for(rit = mRelays.begin(); rit != mRelays.end(); rit++)
	{
		out << "\t" << rit->first << " : " << rit->second;
		out << std::endl;
	}

	out << "UdpRelayReceiver::Connections:" << std::endl;

        std::map<UdpRelayAddrSet, UdpPeer *>::iterator pit;
	for(pit = mStreams.begin(); pit != mStreams.end(); pit++)
	{
		out << "\t" << pit->first << " : " << pit->second;
		out << std::endl;
	}

	return 1;
}

/* higher level interface */
int UdpRelayReceiver::recvPkt(void *data, int size, struct sockaddr_in &from)
{
	/* print packet information */
#ifdef DEBUG_UDP_RELAY
	std::cerr << "UdpRelayReceiver::recvPkt(" << size << ") from: " << from;
	std::cerr << std::endl;
#endif

        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/

	/* decide if we are the relay, or the endpoint */
	UdpRelayAddrSet addrSet;
	if (!extractUdpRelayAddrSet(data, size, addrSet))
	{
		/* fails most basic test, drop */
		return 0; 
	}
		
	/* lookup relay first (double entries) */
        std::map<UdpRelayAddrSet, UdpRelay>::iterator rit = mRelays.find(addrSet);
	if (rit != mRelays.end())
	{
		/* we are the relay */
#ifdef DEBUG_UDP_RELAY
		std::cerr << "UdpRelayReceiver::recvPkt() We are the Relay. Passing onto: ";
		std::cerr << it->second.endpoint;
		std::cerr << std::endl;
#endif
		/* do accounting */
		it->second.mLastTS = now;
		it->second.mDataSize += size;

		mPublisher->sendPkt(data, size, it->second.endpoint, STD_TTL);
		return 1;
	}

	/* otherwise we are likely to be the endpoint */
        std::map<UdpRelayAddrSet, UdpPeer *>::iterator pit = mStreams.find(addrSet);
	if (pit != mStreams.end())
	{
		/* we are the end-point */
#ifdef DEBUG_UDP_RELAY
		std::cerr << "UdpRelayReceiver::recvPkt() Sending to UdpPeer: ";
		std::cerr << it->first;
		std::cerr << std::endl;
#endif
		/* remove the header */
		void *pktdata = (void *) (((uint8_t *) data) + RELAY_HEADER_SIZE);
		int   pktsize = size - RELAY_HEADER_SIZE;
		if (pktsize > 0)
		{
			(it->second)->recvPkt(pktdata, pktsize);
		}
		else
		{
			/* packet undersized */
#ifdef DEBUG_UDP_RELAY
			std::cerr << "UdpRelayReceiver::recvPkt() ERROR Packet Undersized";
			std::cerr << std::endl;
#endif
		}
		return 1;
	}
	/* done */

	/* unknown */
#ifdef DEBUG_UDP_RELAY
	std::cerr << "UdpRelayReceiver::recvPkt() Peer Unknown!";
	std::cerr << std::endl;
#endif
	return 0;
}


int UdpRelayReceiver::sendPkt(const void *data, int size, sockaddr_in &to, int ttl)
{
        RsStackMutex stack(peerMtx);   /********** LOCK MUTEX *********/

	/* work out who the proxy is */
        std::map<struct sockaddr_in, UdpRelayProxy>::iterator it;
	it = mProxies.find(to);
	if (it == mProxies.end())
	{
		return 0;
	}

	/* add a header to packet */
	char tmpPkt[MAXSIZE];
	int tmpSize = MAXSIZE;

	createRelayUdpPacket(data, size, tmpPkt, tmpSize, it->second);

	/* send the packet on */
	return mPublisher->send(tmpPkt, tmpSize, it->second.proxyAddr, STD_TTL);
}

class	UdpRelayAddrSet
{
	public:

        struct sockaddr_in srcAddr;
        struct sockaddr_in proxyAddr;
        struct sockaddr_in destAddr;
};


int extractUdpRelayAddrSet(const void *data, const int size, UdpRelayAddrSet &addrSet)
{
	

	return 1;
}

int createRelayUdpPacket(const void *data, const int size, void *newpkt, int *newsize, UdpRelayProxy &urp)
{



	return 1;
}

