/*
 * bitdht/udpbitdht.cc
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


#include "udp/udpbitdht.h"
#include "bitdht/bdpeer.h"
#include "bitdht/bdstore.h"
#include "bitdht/bdmsgs.h"
#include "bitdht/bencode.h"

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <string.h>

#include "util/bdnet.h"

/*
 * #define DEBUG_UDP_BITDHT 1
 */
//#define DEBUG_UDP_BITDHT 1

/*************************************/

UdpBitDht::UdpBitDht(UdpPublisher *pub, bdNodeId *id, std::string dhtVersion, std::string bootstrapfile, bdDhtFunctions *fns)
	:UdpSubReceiver(pub), mFns(fns)
{
	/* setup nodeManager */
	mBitDhtManager = new bdNodeManager(id, dhtVersion, bootstrapfile, fns);

}


UdpBitDht::~UdpBitDht() 
{ 
	return; 
}


        /*********** External Interface to the World ************/

        /***** Functions to Call down to bdNodeManager ****/
        /* Request DHT Peer Lookup */
        /* Request Keyword Lookup */
void UdpBitDht::addFindNode(bdNodeId *id, uint32_t mode)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mBitDhtManager->addFindNode(id, mode);
}

void UdpBitDht::removeFindNode(bdNodeId *id)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mBitDhtManager->removeFindNode(id);
}

void UdpBitDht::findDhtValue(bdNodeId *id, std::string key, uint32_t mode)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mBitDhtManager->findDhtValue(id, key, mode);
}

        /***** Add / Remove Callback Clients *****/
void UdpBitDht::addCallback(BitDhtCallback *cb)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mBitDhtManager->addCallback(cb);
}

void UdpBitDht::removeCallback(BitDhtCallback *cb)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mBitDhtManager->removeCallback(cb);
}

int UdpBitDht::getDhtPeerAddress(bdNodeId *id, struct sockaddr_in &from)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->getDhtPeerAddress(id, from);
}

int 	UdpBitDht::getDhtValue(bdNodeId *id, std::string key, std::string &value)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->getDhtValue(id, key, value);
}

        /******************* Internals *************************/

        /***** Iteration / Loop Management *****/

        /*** Overloaded from UdpSubReceiver ***/
int UdpBitDht::recvPkt(void *data, int size, struct sockaddr_in &from)
{
	/* pass onto bitdht */
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	/* check packet suitability */
	if (mBitDhtManager->isBitDhtPacket((char *) data, size, from))
	{

		mBitDhtManager->incomingMsg(&from, (char *) data, size);
		return 1;
	}
	return 0;
}
			
int UdpBitDht::status(std::ostream &out)
{
	out << "UdpBitDht::status()" << std::endl;

	return 1;
}

        /*** Overloaded from iThread ***/
void UdpBitDht::run()
{
	while(1)
	{
		tick();
		sleep(1);
	}
}

int UdpBitDht::tick()
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	/* pass on messages from the node */
	int i = 0;
	char data[BITDHT_MAX_PKTSIZE];
	struct sockaddr_in toAddr;
	int size = BITDHT_MAX_PKTSIZE;

	/* accept up to 10 msgs / tick() */
	while((i < 10) && (mBitDhtManager->outgoingMsg(&toAddr, data, &size)))
	{
#ifdef DEBUG_UDP_BITDHT 
		std::cerr << "UdpBitDht::tick() outgoing msg(" << size << ") to " << toAddr;
		std::cerr << std::endl;
#endif

		sendPkt(data, size, toAddr, BITDHT_TTL);

		// iterate
		i++;
		size = BITDHT_MAX_PKTSIZE; // reset msg size!
	}

	mBitDhtManager->iteration();
	return 1;
}



