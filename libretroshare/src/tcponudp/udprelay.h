#ifndef RS_UDP_RELAY_H
#define RS_UDP_RELAY_H

/*
 * tcponudp/udprelay.h
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

#include "tcponudp/udppeer.h"

class UdpRelayReceiver: public UdpSubReceiver, public UdpPublisher
{
	public:

	UdpRelayReceiver(UdpPublisher *pub);
virtual ~UdpRelayReceiver() { return; }

	/* add a TCPonUDP stream */
int	addUdpPeer(UdpPeer *peer, const struct sockaddr_in &raddr);
int 	removeUdpPeer(UdpPeer *peer);

	/* callback for recved data (overloaded from UdpReceiver) */
virtual int recvPkt(void *data, int size, struct sockaddr_in &from);

	/* wrapper function for relay (overloaded from UdpPublisher) */
virtual int sendPkt(const void *data, int size, struct sockaddr_in &to, int ttl);

int     status(std::ostream &out);

	private:

	RsMutex peerMtx; /* for all class data (below) */

	std::map<struct sockaddr_in, UdpPeer *> streams;

};

#endif
