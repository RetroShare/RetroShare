/*
 * udp/udpstack.cc
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

#include "udp/udpstack.h"

#include <iostream>
#include <sstream>
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

bool    UdpStack::resetAddress(struct sockaddr_in &local)
{
	std::cerr << "UdpStack::resetAddress(" << local << ")";
	std::cerr << std::endl;

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

int  UdpStack::sendPkt(const void *data, int size, struct sockaddr_in &to, int ttl)
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

int  UdpSubReceiver::sendPkt(const void *data, int size, struct sockaddr_in &to, int ttl)
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


