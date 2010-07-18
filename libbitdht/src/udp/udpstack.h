#ifndef BITDHT_UDP_STACK_RECEIVER_H
#define BITDHT_UDP_STACK_RECEIVER_H

/*
 * udp/udpstack.h
 *
 * BitDHT: An Flexible DHT library.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */


#include "util/bdthreads.h"
#include "util/bdnet.h"

#include <iosfwd>
#include <list>
#include <deque>

#include <iosfwd>
#include <map>

#include "udp/udplayer.h"

/* UdpStackReceiver is a Generic Receiver of info from a UdpLayer class.
 * it provides a UdpReceiver class, and accepts a stack of UdpReceivers, 
 * which will be iterated through (in-order) until someone accepts the packet.
 *
 * It is important to order these Receivers correctly!
 * 
 * This infact becomes the holder of the UdpLayer, and all controls
 * go through the StackReceiver.
 */

class UdpSubReceiver: public UdpReceiver
{
	public:
	UdpSubReceiver(UdpPublisher *pub);

		/* calls mPublisher->sendPkt */
virtual int sendPkt(const void *data, int size, struct sockaddr_in &to, int ttl);
		/* callback for recved data (overloaded from UdpReceiver) */
//virtual int recvPkt(void *data, int size, struct sockaddr_in &from) = 0;

	UdpPublisher *mPublisher;
};


class UdpStack: public UdpReceiver, public UdpPublisher
{
	public:

	UdpStack(struct sockaddr_in &local);
virtual ~UdpStack() { return; }

bool	resetAddress(struct sockaddr_in &local);


	/* add in a receiver */
int	addReceiver(UdpReceiver *recv);
int 	removeReceiver(UdpReceiver *recv);

	/* Packet IO */
		/* pass-through send packets */
virtual int sendPkt(const void *data, int size, struct sockaddr_in &to, int ttl);
		/* callback for recved data (overloaded from UdpReceiver) */

virtual int recvPkt(void *data, int size, struct sockaddr_in &from);

int     status(std::ostream &out);

	/* setup connections */
	int openSocket();

	/* monitoring / updates */
	int okay();
//	int tick();

	int close();

	private:

	UdpLayer *udpLayer;

	bdMutex stackMtx; /* for all class data (below) */

	struct sockaddr_in laddr; /* local addr */

	std::list<UdpReceiver *> mReceivers;
};

#endif
