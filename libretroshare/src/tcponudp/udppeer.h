#ifndef RS_UDP_PEER_RECV_H
#define RS_UDP_PEER_RECV_H

/*
 * tcponudp/udppeer.h
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
