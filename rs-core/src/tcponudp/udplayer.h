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

/* universal networking functions */
#include "tou_net.h"

#include <iosfwd>
#include <list>
#include <deque>

std::ostream &operator<<(std::ostream &out,  struct sockaddr_in &addr);
bool operator==(struct sockaddr_in &addr, struct sockaddr_in &addr2);
std::string printPkt(void *d, int size);
std::string printPktOffset(unsigned int offset, void *d, unsigned int size);


/* So the UdpLayer ..... has a couple of roles 
 *
 * Firstly Send Proxy Packet() (for address determination).
 * all the rest of this functionality is handled elsewhere.
 *
 * Secondly support TcpStreamer....
 */

class udpPacket;

class UdpLayer
{
	public:

	UdpLayer(struct sockaddr_in &local);
virtual ~UdpLayer() { return; }

int     status(std::ostream &out);

	/* setup connections */
	int openSocket();
	int setTTL(int t);
	int getTTL();

	int  sendToProxy(struct sockaddr_in &proxy, const void *data, int size);
	int  setRemoteAddr(struct sockaddr_in &remote);
	int  getRemoteAddr(struct sockaddr_in &remote);

	/* Higher Level Interface */
	int  readPkt(void *data, int *size);
	int  sendPkt(void *data, int size);

	/* monitoring / updates */
	int okay();
	int tick();

	int close();

	/* unix like interface for recving packets not part 
	 * of the tcp stream
	 */
ssize_t recvRndPktfrom(void *buf, size_t len, int flags, 
		struct sockaddr *from, socklen_t *fromlen);

	/* data */
	/* internals */
	protected:

virtual	int receiveUdpPacket(void *data, int *size, struct sockaddr_in &from);
virtual	int sendUdpPacket(const void *data, int size, struct sockaddr_in &to);
 
	/* low level */
	/*
	 * int rwSocket();
	 */
	private:


	struct sockaddr_in paddr; /* proxy addr */
	struct sockaddr_in raddr; /* remote addr */
	struct sockaddr_in laddr; /* local addr */

	bool raddrKnown;
	int  errorState;
	int sockfd;

	int ttl;

 	std::deque<udpPacket * > randomPkts;

};

#include <iostream>

class LossyUdpLayer: public UdpLayer
{
	public:

	LossyUdpLayer(struct sockaddr_in &local, double frac)
	:UdpLayer(local), lossFraction(frac)
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
