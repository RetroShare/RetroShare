/*
 * "$Id: udplayer.h,v 1.5 2007-02-18 21:46:50 rmf24 Exp $"
 *
 * TCP-on-UDP (tou) network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#ifndef TOU_UDP_LAYER_H
#define TOU_UDP_LAYER_H


/*
#include <netinet/in.h> 
#include <sys/types.h>
#include <sys/socket.h>
*/


#include "util/rsthreads.h"

/* universal networking functions */
#include "tou_net.h"

#include <iosfwd>
#include <list>
#include <deque>

bool operator==(const struct sockaddr_in &addr, const struct sockaddr_in &addr2);
bool operator<(const struct sockaddr_in &addr, const struct sockaddr_in &addr2);

std::string printPkt(void *d, int size);
std::string printPktOffset(unsigned int offset, void *d, unsigned int size);


/* UdpLayer ..... is the bottom layer which 
 * just sends and receives Udp packets.
 */

class UdpReceiver
{
	public:
virtual void recvPkt(void *data, int size, struct sockaddr_in &from) = 0;
};

class UdpLayer: public RsThread
{
	public:

	UdpLayer(UdpReceiver *recv, struct sockaddr_in &local);
virtual ~UdpLayer() { return; }

int 	reset(struct sockaddr_in &local); /* calls join, close, openSocket */

int     status(std::ostream &out);

	/* setup connections */
	int close();
	int openSocket();

	/* RsThread functions */
virtual void run(); /* called once the thread is started */

void	recv_loop(); /* uses callback to UdpReceiver */

	/* Higher Level Interface */
	//int  readPkt(void *data, int *size, struct sockaddr_in &from);
	int  sendPkt(void *data, int size, struct sockaddr_in &to, int ttl);

	/* monitoring / updates */
	int okay();
	int tick();


	/* data */
	/* internals */
	protected:

virtual	int receiveUdpPacket(void *data, int *size, struct sockaddr_in &from);
virtual	int sendUdpPacket(const void *data, int size, struct sockaddr_in &to);
 
	int setTTL(int t);
	int getTTL();

	/* low level */
	private:

	UdpReceiver *recv;

	struct sockaddr_in laddr; /* local addr */

	int  errorState;
	int sockfd;
	int ttl;
	bool stopThread;

	RsMutex sockMtx;
};

#include <iostream>
#include <stdlib.h>

class LossyUdpLayer: public UdpLayer
{
	public:

	LossyUdpLayer(UdpReceiver *udpr, struct sockaddr_in &local, double frac)
	:UdpLayer(udpr, local), lossFraction(frac)
	{
		return;
	}
	virtual ~LossyUdpLayer() { return; }

	protected:

	virtual int receiveUdpPacket(void *data, int *size, struct sockaddr_in &from)
	{
		double prob = (1.0 * (rand() / (RAND_MAX + 1.0)));

		if (prob < lossFraction)
		{
			/* but discard */
			if (0 < UdpLayer::receiveUdpPacket(data, size, from))
			{
				std::cerr << "LossyUdpLayer::receiveUdpPacket() Dropping packet!";
				std::cerr << std::endl;
				std::cerr << printPkt(data, *size);
				std::cerr << std::endl;
				std::cerr << "LossyUdpLayer::receiveUdpPacket() Packet Dropped!";
				std::cerr << std::endl;
			}

			size = 0;
			return -1;

		}

		// otherwise read normally;
		return UdpLayer::receiveUdpPacket(data, size, from);
	}


	virtual int sendUdpPacket(const void *data, int size, struct sockaddr_in &to)
	{
		double prob = (1.0 * (rand() / (RAND_MAX + 1.0)));

		if (prob < lossFraction)
		{
			/* discard */

			std::cerr << "LossyUdpLayer::sendUdpPacket() Dropping packet!";
			std::cerr << std::endl;
			std::cerr << printPkt((void *) data, size);
			std::cerr << std::endl;
			std::cerr << "LossyUdpLayer::sendUdpPacket() Packet Dropped!";
			std::cerr << std::endl;

			return size;
		}

		// otherwise read normally;
		return UdpLayer::sendUdpPacket(data, size, to);
	}

	double lossFraction;
};

#endif
