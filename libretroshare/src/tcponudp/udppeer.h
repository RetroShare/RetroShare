/*******************************************************************************
 * libretroshare/src/tcponudp: udppeer.h                                       *
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
#ifndef RS_UDP_PEER_RECV_H
#define RS_UDP_PEER_RECV_H

#ifndef WINDOWS_SYS
#include <netinet/in.h>
#endif

#include "util/rsthreads.h"

#include <iosfwd>
#include <list>
#include <deque>

#include "tcponudp/rsudpstack.h"

class UdpPeer
{
	public:
virtual ~UdpPeer() { return; }
virtual void recvPkt(void *data, int size) = 0;
};


class UdpPeerReceiver: public UdpSubReceiver
{
	public:

	UdpPeerReceiver(UdpPublisher *pub);
virtual ~UdpPeerReceiver() { return; }

	/* add a TCPonUDP stream */
int	addUdpPeer(UdpPeer *peer, const struct sockaddr_in &raddr);
int 	removeUdpPeer(UdpPeer *peer);

	/* callback for recved data (overloaded from UdpReceiver) */
virtual int recvPkt(void *data, int size, struct sockaddr_in &from);

int     status(std::ostream &out);

	private:

	RsMutex peerMtx; /* for all class data (below) */

	std::map<struct sockaddr_in, UdpPeer *> streams;

};


#endif
