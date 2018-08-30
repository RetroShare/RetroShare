/*******************************************************************************
 * libretroshare/src/tcponudp: udppeer.cc                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2010-2010 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include "udppeer.h"
#include <iostream>

/*
 * #define DEBUG_UDP_PEER 1
 */


UdpPeerReceiver::UdpPeerReceiver(UdpPublisher *pub)
	:UdpSubReceiver(pub), peerMtx("UdpSubReceiver")
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
	for(it = streams.begin(); it != streams.end(); ++it)
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
	for(it = streams.begin(); it != streams.end(); ++it)
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


