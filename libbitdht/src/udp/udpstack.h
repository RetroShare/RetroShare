/*******************************************************************************
 * udp/udpstack.h                                                              *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <bitdht@lunamutt.com>                       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef BITDHT_UDP_STACK_RECEIVER_H
#define BITDHT_UDP_STACK_RECEIVER_H

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
virtual int sendPkt(const void *data, int size, const struct sockaddr_in &to, int ttl);
		/* callback for recved data (overloaded from UdpReceiver) */
//virtual int recvPkt(void *data, int size, struct sockaddr_in &from) = 0;

	UdpPublisher *mPublisher;
};


#define UDP_TEST_LOSSY_LAYER		1
#define UDP_TEST_RESTRICTED_LAYER	2
#define UDP_TEST_TIMED_LAYER		3

#define UDP_TEST_LOSSY_FRAC		(0.10)

class UdpStack: public UdpReceiver, public UdpPublisher
{
	public:

	UdpStack(struct sockaddr_in &local);
	UdpStack(int testmode, struct sockaddr_in &local);
virtual ~UdpStack() { return; }

UdpLayer *getUdpLayer(); /* for testing only */

bool    getLocalAddress(struct sockaddr_in &local);
bool	resetAddress(struct sockaddr_in &local);


	/* add in a receiver */
int	addReceiver(UdpReceiver *recv);
int 	removeReceiver(UdpReceiver *recv);

	/* Packet IO */
		/* pass-through send packets */
virtual int sendPkt(const void *data, int size, const struct sockaddr_in &to, int ttl);
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
