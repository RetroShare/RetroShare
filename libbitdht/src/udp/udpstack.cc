/*******************************************************************************
 * udp/udpstack.cc                                                             *
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

#include "udp/udpstack.h"

#include <iostream>
#include <iomanip>
#include <string.h>
#include <stdlib.h>

#include <algorithm>

/***
 * #define DEBUG_UDP_RECV	1
 ***/

//#define DEBUG_UDP_RECV	1


UdpStack::UdpStack(struct sockaddr_in &local)
	:udpLayer(NULL), laddr(local)
{
	openSocket();
	return;
}

UdpStack::UdpStack(int testmode, struct sockaddr_in &local)
	:udpLayer(NULL), laddr(local)
{
	std::cerr << "UdpStack::UdpStack() Evoked in TestMode" << std::endl;
	if (testmode == UDP_TEST_LOSSY_LAYER)
	{
		std::cerr << "UdpStack::UdpStack() Installing LossyUdpLayer" << std::endl;
		udpLayer = new LossyUdpLayer(this, laddr, UDP_TEST_LOSSY_FRAC);
	}
	else if (testmode == UDP_TEST_RESTRICTED_LAYER)
	{
		std::cerr << "UdpStack::UdpStack() Installing RestrictedUdpLayer" << std::endl;
		udpLayer = new RestrictedUdpLayer(this, laddr);
	}
	else if (testmode == UDP_TEST_TIMED_LAYER)
	{
		std::cerr << "UdpStack::UdpStack() Installing TimedUdpLayer" << std::endl;
		udpLayer = new TimedUdpLayer(this, laddr);
	}
	else
	{
		std::cerr << "UdpStack::UdpStack() Installing Standard UdpLayer" << std::endl;
		// standard layer 
		openSocket();
	}
	return;
}

UdpLayer *UdpStack::getUdpLayer() /* for testing only */
{
	return udpLayer;
}

bool    UdpStack::getLocalAddress(struct sockaddr_in &local)
{
	local = laddr;
	return true;
}

bool    UdpStack::resetAddress(struct sockaddr_in &local)
{
#ifdef DEBUG_UDP_RECV
    std::cerr << "UdpStack::resetAddress(" << local << ")";
	std::cerr << std::endl;
#endif
	laddr = local;

	return udpLayer->reset(local);
}



/* higher level interface */
int UdpStack::recvPkt(void *data, int size, struct sockaddr_in &from)
{
	/* print packet information */
#ifdef DEBUG_UDP_RECV
	std::cerr << "UdpStack::recvPkt(" << size << ") from: " << from;
	std::cerr << std::endl;
#endif

        bdStackMutex stack(stackMtx);   /********** LOCK MUTEX *********/

        std::list<UdpReceiver *>::iterator it;
	for(it = mReceivers.begin(); it != mReceivers.end(); it++)
	{
		// See if they want the packet.
		if ((*it)->recvPkt(data, size, from))
		{
#ifdef DEBUG_UDP_RECV
			std::cerr << "UdpStack::recvPkt(" << size << ") from: " << from;
			std::cerr << std::endl;
#endif
			break;
		}
	}
	return 1;
}

int  UdpStack::sendPkt(const void *data, int size, const struct sockaddr_in &to, int ttl)
{
	/* print packet information */
#ifdef DEBUG_UDP_RECV
	std::cerr << "UdpStack::sendPkt(" << size << ") ttl: " << ttl;
	std::cerr << " to: " << to;
	std::cerr << std::endl;
#endif

	/* send to udpLayer */
	return udpLayer->sendPkt(data, size, to, ttl);
}

int     UdpStack::status(std::ostream &out)
{
	{
	        bdStackMutex stack(stackMtx);   /********** LOCK MUTEX *********/
	
		out << "UdpStack::status()" << std::endl;
		out << "localaddr: " << laddr << std::endl;
		out << "UdpStack::SubReceivers:" << std::endl;
        	std::list<UdpReceiver *>::iterator it;
		int i = 0;
		for(it = mReceivers.begin(); it != mReceivers.end(); it++, i++)
		{
			out << "\tReceiver " << i << " --------------------" << std::endl;
			(*it)->status(out);
		}
		out << "--------------------" << std::endl;
		out << std::endl;
	}

	udpLayer->status(out);

	return 1;
}

/* setup connections */
int UdpStack::openSocket()	
{
	udpLayer = new UdpLayer(this, laddr);
	return 1;
}

/* monitoring / updates */
int UdpStack::okay()
{
	return udpLayer->okay();
}

int UdpStack::close()
{
	/* TODO */
	return 1;
}


        /* add a TCPonUDP stream */
int UdpStack::addReceiver(UdpReceiver *recv)
{
        bdStackMutex stack(stackMtx);   /********** LOCK MUTEX *********/

	/* check for duplicate */
        std::list<UdpReceiver *>::iterator it;
	it = std::find(mReceivers.begin(), mReceivers.end(), recv);
	if (it == mReceivers.end())
	{
		mReceivers.push_back(recv);
		return 1;
	}

	/* otherwise its already there! */

#ifdef DEBUG_UDP_RECV
	std::cerr << "UdpStack::addReceiver() Recv already exists!" << std::endl;
	std::cerr << "UdpStack::addReceiver() ERROR" << std::endl;
#endif

	return 0;
}

int UdpStack::removeReceiver(UdpReceiver *recv)
{
        bdStackMutex stack(stackMtx);   /********** LOCK MUTEX *********/

	/* check for duplicate */
        std::list<UdpReceiver *>::iterator it;
	it = std::find(mReceivers.begin(), mReceivers.end(), recv);
	if (it != mReceivers.end())
	{
		mReceivers.erase(it);
		return 1;
	}

	/* otherwise its not there! */

#ifdef DEBUG_UDP_RECV
	std::cerr << "UdpStack::removeReceiver() Recv dont exist!" << std::endl;
	std::cerr << "UdpStack::removeReceiver() ERROR" << std::endl;
#endif

	return 0;
}



/*****************************************************************************************/

UdpSubReceiver::UdpSubReceiver(UdpPublisher *pub)
	:mPublisher(pub) 
{ 
	return; 
}

int  UdpSubReceiver::sendPkt(const void *data, int size, const struct sockaddr_in &to, int ttl)
{
	/* print packet information */
#ifdef DEBUG_UDP_RECV
	std::cerr << "UdpSubReceiver::sendPkt(" << size << ") ttl: " << ttl;
	std::cerr << " to: " << to;
	std::cerr << std::endl;
#endif

	/* send to udpLayer */
	return mPublisher->sendPkt(data, size, to, ttl);
}


