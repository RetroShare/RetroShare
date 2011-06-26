/*
 * libretroshare/src/tcponudp: udptestfn.h
 *
 * TCP-on-UDP (tou) network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "udp/udplayer.h"
#include "udp/udpstack.h"
#include "tcponudp/udppeer.h"

//#include "udpsorter.h"

#ifndef TOU_UDP_TEST_FN_H
#define TOU_UDP_TEST_FN_H

class UdpRecvTest: public UdpReceiver
{
	public:
virtual int recvPkt(void *data, int size, struct sockaddr_in &from);
virtual int status(std::ostream&) { return 0; }
};


class UdpPeerTest: public UdpPeer
{
	public:
	UdpPeerTest(struct sockaddr_in &addr);
virtual void recvPkt(void *data, int size);

        struct sockaddr_in raddr;
};


#endif
