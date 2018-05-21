/*******************************************************************************
 * udp/udpbitdht.h                                                             *
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
#ifndef UDP_BIT_DHT_CLASS_H
#define UDP_BIT_DHT_CLASS_H

#include <iosfwd>
#include <map>
#include <string>

#include "udp/udpstack.h"
#include "bitdht/bdiface.h"
#include "bitdht/bdmanager.h"

/* 
 * This implements a UdpSubReceiver class to allow the DHT to talk to the network.
 * The parser is very strict - and will try to not pick up anyone else's messages.
 *
 * Mutexes are implemented at this level protecting the whole of the DHT code.
 * This class is also a thread - enabling it to do callback etc.
 */

// class BitDhtCallback defined in bdiface.h 

	
class UdpBitDht: public UdpSubReceiver, public bdThread, public BitDhtInterface
{
	public:

    UdpBitDht(UdpPublisher *pub, bdNodeId *id, std::string dhtVersion, std::string bootstrapfile, const std::string& filteredipfile,bdDhtFunctions *fns);
virtual ~UdpBitDht();


	/*********** External Interface to the World (BitDhtInterface) ************/

	/***** Functions to Call down to bdNodeManager ****/

        /* Friend Tracking */
virtual void addBadPeer(const struct sockaddr_in &addr, uint32_t source, uint32_t reason, uint32_t age);
virtual void updateKnownPeer(const bdId *id, uint32_t type, uint32_t flags);

	/* Request DHT Peer Lookup */
	/* Request Keyword Lookup */
virtual	void addFindNode(bdNodeId *id, uint32_t mode);
virtual	void removeFindNode(bdNodeId *id);
virtual	void findDhtValue(bdNodeId *id, std::string key, uint32_t mode);

	/***** Add / Remove Callback Clients *****/
virtual	void addCallback(BitDhtCallback *cb);
virtual	void removeCallback(BitDhtCallback *cb);

        /***** Connections Requests *****/
virtual bool ConnectionRequest(struct sockaddr_in *laddr, bdNodeId *target, uint32_t mode, uint32_t delay, uint32_t start);
virtual void ConnectionAuth(bdId *srcId, bdId *proxyId, bdId *destId, uint32_t mode, uint32_t loc, 
											uint32_t bandwidth, uint32_t delay, uint32_t answer);
virtual void ConnectionOptions(uint32_t allowedModes, uint32_t flags);
virtual bool setAttachMode(bool on);

        /***** Get Results Details *****/
virtual int getDhtPeerAddress(const bdNodeId *id, struct sockaddr_in &from);
virtual int getDhtValue(const bdNodeId *id, std::string key, std::string &value);
virtual int getDhtBucket(const int idx, bdBucket &bucket);

virtual int getDhtQueries(std::map<bdNodeId, bdQueryStatus> &queries);
virtual int getDhtQueryStatus(const bdNodeId *id, bdQuerySummary &query);

    virtual bool isAddressBanned(const sockaddr_in &raddr) ;
    virtual bool getListOfBannedIps(std::list<bdFilteredPeer> &ipl);

        /* stats and Dht state */
virtual int startDht();
virtual int stopDht();
virtual int stateDht(); 
virtual uint32_t statsNetworkSize();
virtual uint32_t statsBDVersionSize(); 
virtual uint32_t setDhtMode(uint32_t dhtFlags);

void getDataTransferred(uint32_t &read, uint32_t &write);

	/******************* Internals *************************/
	/***** Iteration / Loop Management *****/

	/*** Overloaded from UdpSubReceiver ***/
virtual int recvPkt(void *data, int size, struct sockaddr_in &from);
virtual int status(std::ostream &out);


	/*** Overloaded from iThread ***/
virtual void run();

	/**** do whats to be done ***/
int tick();
private:

void clearDataTransferred();

	bdMutex dhtMtx; /* for all class data (below) */
	bdNodeManager *mBitDhtManager;
	//bdDhtFunctions *mFns;


	uint32_t mReadBytes;
	uint32_t mWriteBytes;


};


#endif
