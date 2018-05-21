/*******************************************************************************
 * bitdht/udpbitdht.cc                                                         *
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

#include "udp/udpbitdht.h"
#include "bitdht/bdpeer.h"
#include "bitdht/bdstore.h"
#include "bitdht/bdmsgs.h"
#include "bitdht/bencode.h"

#include <stdlib.h>
#include <unistd.h>		/* for usleep() */
#include <iostream>
#include <iomanip>

#include <string.h>

#include "util/bdnet.h"

/*
 * #define DEBUG_UDP_BITDHT 1
 *
 * #define BITDHT_VERSION_ANONYMOUS	1
 */

//#define DEBUG_UDP_BITDHT 1

#define BITDHT_VERSION_IDENTIFER	1

// Original RS 0.5.0/0.5.1 version, is un-numbered.
//#define BITDHT_VERSION			"00" // First Release of BitDHT with Connections (Proxy Support + Dht Stun)
//#define BITDHT_VERSION			"01" // Testing Connections (Proxy Only)
#define BITDHT_VERSION				"02" // Completed Relay Connections from svn 4766
//#define BITDHT_VERSION			"04" // Full DHT implementation.?

/*************************************/

UdpBitDht::UdpBitDht(UdpPublisher *pub, bdNodeId *id, std::string appVersion, std::string bootstrapfile, const std::string& filteredipfile, bdDhtFunctions *fns)
	:UdpSubReceiver(pub), dhtMtx(true)//, mFns(fns)
{
	std::string usedVersion;

#ifdef BITDHT_VERSION_IDENTIFER
	usedVersion = "BD";
	usedVersion += BITDHT_VERSION;
#endif
	usedVersion += appVersion;

#ifdef BITDHT_VERSION_ANONYMOUS
	usedVersion = ""; /* blank it */
#endif

	clearDataTransferred();

	/* setup nodeManager */
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/
    mBitDhtManager = new bdNodeManager(id, usedVersion, bootstrapfile, filteredipfile, fns);
}


UdpBitDht::~UdpBitDht() 
{ 
	return; 
}


        /*********** External Interface to the World ************/

        /***** Functions to Call down to bdNodeManager ****/
        /* Friend Tracking */
void UdpBitDht::addBadPeer(const struct sockaddr_in &addr, uint32_t source, uint32_t reason, uint32_t age)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mBitDhtManager->addBadPeer(addr, source, reason, age);
}


void UdpBitDht::updateKnownPeer(const bdId *id, uint32_t type, uint32_t flags)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mBitDhtManager->updateKnownPeer(id, type, flags);
}

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

bool UdpBitDht::ConnectionRequest(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode, uint32_t delay, uint32_t start)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->ConnectionRequest(laddr, target, mode, delay, start);
}



void UdpBitDht::ConnectionAuth(bdId *srcId, bdId *proxyId, bdId *destId, uint32_t mode, uint32_t loc, 
								uint32_t bandwidth, uint32_t delay, uint32_t answer)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mBitDhtManager->ConnectionAuth(srcId, proxyId, destId, mode, loc, bandwidth, delay, answer);
}

void UdpBitDht::ConnectionOptions(uint32_t allowedModes, uint32_t flags)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mBitDhtManager->ConnectionOptions(allowedModes, flags);
}

bool UdpBitDht::setAttachMode(bool on)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->setAttachMode(on);
}


int UdpBitDht::getDhtPeerAddress(const bdNodeId *id, struct sockaddr_in &from)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->getDhtPeerAddress(id, from);
}

int 	UdpBitDht::getDhtValue(const bdNodeId *id, std::string key, std::string &value)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->getDhtValue(id, key, value);
}

int 	UdpBitDht::getDhtBucket(const int idx, bdBucket &bucket)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->getDhtBucket(idx, bucket);
}



int 	UdpBitDht::getDhtQueries(std::map<bdNodeId, bdQueryStatus> &queries)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->getDhtQueries(queries);
}

int 	UdpBitDht::getDhtQueryStatus(const bdNodeId *id, bdQuerySummary &query)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

    return mBitDhtManager->getDhtQueryStatus(id, query);
}

bool UdpBitDht::isAddressBanned(const sockaddr_in &raddr)
{
    return mBitDhtManager->addressBanned(raddr) ;
}

bool UdpBitDht::getListOfBannedIps(std::list<bdFilteredPeer>& ipl)
{
    return mBitDhtManager->getFilteredPeers(ipl) ;
}



        /* stats and Dht state */
int UdpBitDht:: startDht()
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->startDht();
}

int UdpBitDht:: stopDht()
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->stopDht();
}

int UdpBitDht::stateDht() 
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->stateDht();
}

uint32_t UdpBitDht::statsNetworkSize()
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->statsNetworkSize();
}

uint32_t UdpBitDht::statsBDVersionSize()
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->statsBDVersionSize();
}

uint32_t UdpBitDht::setDhtMode(uint32_t dhtFlags)
{
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	return mBitDhtManager->setDhtMode(dhtFlags);
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

		mReadBytes += size;
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

void UdpBitDht::clearDataTransferred()
{
	/* pass onto bitdht */
	bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

	mReadBytes = 0;
	mWriteBytes = 0;
}


void UdpBitDht::getDataTransferred(uint32_t &read, uint32_t &write)
{
	{
		/* pass onto bitdht */
		bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/

		read = mReadBytes;
		write = mWriteBytes;
	}
	clearDataTransferred();
}

			
        /*** Overloaded from iThread ***/
#define MAX_MSG_PER_TICK	100
#define TICK_PAUSE_USEC		20000  /* 20ms secs .. max messages = 50 x 100 = 5000 */

void UdpBitDht::run()
{
	while(1)
	{
		while(tick())
		{
			usleep(TICK_PAUSE_USEC);
		}

		{
			bdStackMutex stack(dhtMtx); /********** MUTEX LOCKED *************/
			mBitDhtManager->iteration();
		}
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

	while((i < MAX_MSG_PER_TICK) && (mBitDhtManager->outgoingMsg(&toAddr, data, &size)))
	{
#ifdef DEBUG_UDP_BITDHT 
		std::cerr << "UdpBitDht::tick() outgoing msg(" << size << ") to " << toAddr;
		std::cerr << std::endl;
#endif

		mWriteBytes += size;
		sendPkt(data, size, toAddr, BITDHT_TTL);

		// iterate
		i++;
		size = BITDHT_MAX_PKTSIZE; // reset msg size!
	}

	if (i == MAX_MSG_PER_TICK)
	{
		return 1; /* keep on ticking */
	}
	return 0;
}



