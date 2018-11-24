/*******************************************************************************
 * libretroshare/src/tcponudp: udprelay.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2010-2010 by Robert Fernie <retroshare@lunamutt.com>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef RS_UDP_RELAY_H
#define RS_UDP_RELAY_H

#include "tcponudp/udppeer.h"
#include <retroshare/rsdht.h>
#include <vector>

class UdpRelayAddrSet;

class UdpRelayAddrSet
{
	public:
	UdpRelayAddrSet();
	UdpRelayAddrSet(const sockaddr_in *ownAddr, const sockaddr_in *destAddr);

	UdpRelayAddrSet flippedSet();

	struct sockaddr_in mSrcAddr; /* msg source */
	struct sockaddr_in mDestAddr; /* final destination */
};

int operator<(const UdpRelayAddrSet &a, const UdpRelayAddrSet &b);
	
class UdpRelayProxy
{
	public:
	UdpRelayProxy();
	UdpRelayProxy(UdpRelayAddrSet *addrSet, int relayClass, uint32_t bandwidth);

	UdpRelayAddrSet mAddrs;
	double mBandwidth;
	uint32_t mDataSize;
	rstime_t mLastBandwidthTS;
	rstime_t mLastTS;

	rstime_t mStartTS;
	double mBandwidthLimit;

	int mRelayClass;
};


class UdpRelayEnd
{
	public:

	UdpRelayEnd();
	UdpRelayEnd(UdpRelayAddrSet *endPoints, const struct sockaddr_in *proxyaddr);

	struct sockaddr_in mLocalAddr; 
	struct sockaddr_in mProxyAddr; 
	struct sockaddr_in mRemoteAddr; 
};

std::ostream &operator<<(std::ostream &out, const UdpRelayAddrSet &uras);
std::ostream &operator<<(std::ostream &out, const UdpRelayProxy &urp);
std::ostream &operator<<(std::ostream &out, const UdpRelayEnd &ure);

/* we define a couple of classes (determining which class is done elsewhere)
 * There will be various maximums for each type.
 * Ideally you want to allow your friends to use your proxy in preference
 * to randoms.
 *
 * At N x 2 x maxBandwidth.
 *
 * 10 x 2 x 1Kb/s => 20Kb/s In and Out. (quite a bit!)
 * 20 x 2 x 1Kb/s => 40Kb/s Huge.
 */

#define UDP_RELAY_DEFAULT_COUNT_ALL	2
#define UDP_RELAY_DEFAULT_FRIEND	0
#define UDP_RELAY_DEFAULT_FOF		1
#define UDP_RELAY_DEFAULT_GENERAL	1
#define UDP_RELAY_DEFAULT_BANDWIDTH	1024

#define UDP_RELAY_FRAC_GENERAL		(0.5)
#define UDP_RELAY_FRAC_FOF		(0.5)
#define UDP_RELAY_FRAC_FRIENDS		(0.0)



/**** DEFINED IN EXTERNAL HEADER FILE ***/
#define UDP_RELAY_NUM_CLASS		RSDHT_RELAY_NUM_CLASS

#define UDP_RELAY_CLASS_ALL		RSDHT_RELAY_CLASS_ALL		
#define UDP_RELAY_CLASS_GENERAL		RSDHT_RELAY_CLASS_GENERAL		
#define UDP_RELAY_CLASS_FOF		RSDHT_RELAY_CLASS_FOF		
#define UDP_RELAY_CLASS_FRIENDS		RSDHT_RELAY_CLASS_FRIENDS		

// Just for some testing fun!
//#define UDP_RELAY_LIFETIME_GENERAL	180	// 3 minutes
//#define UDP_RELAY_LIFETIME_FOF	360	// 6 minutes.
//#define UDP_RELAY_LIFETIME_FRIENDS	720 	// 12 minutes.

#define UDP_RELAY_LIFETIME_GENERAL	3600	// 1 hour (chosen so we at least transfer 1 or 2 meg at lowest speed)
#define UDP_RELAY_LIFETIME_FOF		7200	// 2 Hours.
#define UDP_RELAY_LIFETIME_FRIENDS	14400 	// 4 Hours.

#define STD_RELAY_TTL	64

class UdpRelayReceiver: public UdpSubReceiver
{
	public:

	UdpRelayReceiver(UdpPublisher *pub);
virtual ~UdpRelayReceiver();

	/* add a TCPonUDP stream (ENDs) */
int	addUdpPeer(UdpPeer *peer, UdpRelayAddrSet *endPoints, const struct sockaddr_in *proxyaddr);
int 	removeUdpPeer(UdpPeer *peer);

	/* add a Relay Point (for the Relay).
	 * These don't have to be explicitly removed.
	 * They will be timed out when 
	 * the end-points drop the connections 
	 */

	int addUdpRelay(UdpRelayAddrSet *addrSet, int &relayClass, uint32_t &bandwidth);
	int removeUdpRelay(UdpRelayAddrSet *addrs);

	/* Need some stats, to work out how many relays we are supporting */
	int checkRelays();

	int setRelayTotal(int count); /* sets all the Relay Counts (frac based on total) */
	int setRelayClassMax(int classIdx, int count, int bandwidth); /* set a specific class maximum */
	int getRelayClassMax(int classIdx);
	int getRelayClassBandwidth(int classIdx);
	int getRelayCount(int classIdx); /* how many relays (of this type) do we have */
	int RelayStatus(std::ostream &out);

	/* Extract Relay Data */
	int getRelayEnds(std::list<UdpRelayEnd> &relayEnds); 		
	int getRelayProxies(std::list<UdpRelayProxy> &relayProxies); 
	
	/* callback for recved data (overloaded from UdpReceiver) */
virtual int recvPkt(void *data, int size, struct sockaddr_in &from);

	/* wrapper function for relay (overloaded from UdpSubReceiver) */
virtual int sendPkt(const void *data, int size, const struct sockaddr_in &to, int ttl);

int     status(std::ostream &out);
int 	UdpPeersStatus(std::ostream &out);

void 	getDataTransferred(uint32_t &read, uint32_t &write, uint32_t &relay);

	private:

	void clearDataTransferred();

	int removeUdpRelay_relayLocked(UdpRelayAddrSet *addrs);
	int installRelayClass_relayLocked(int &classIdx, uint32_t &bandwidth);
	int removeRelayClass_relayLocked(int classIdx);

	/* Unfortunately, Due the reentrant nature of this classes activities...
	 * the SendPkt() must be callable from inside RecvPkt().
	 * This means we need two seperate mutexes.
	 *  - one for UdpPeer's, and one for Relay Data.
	 *
	 * care must be taken to lock these mutex's in a consistent manner to avoid deadlock.
	 *  - You are not allowed to hold both at the same time!
	 */

	RsMutex udppeerMtx; /* for all class data (below) */
	
	std::map<struct sockaddr_in, UdpPeer *> mPeers; /* indexed by <dest> */
	uint32_t mReadBytes;

	RsMutex relayMtx; /* for all class data (below) */

	std::vector<int> mClassLimit, mClassCount, mClassBandwidth;
	std::map<struct sockaddr_in, UdpRelayEnd> mStreams; /* indexed by <dest> */
	std::map<UdpRelayAddrSet, UdpRelayProxy> mRelays; /* indexed by <src,dest> */

	void *mTmpSendPkt;
	uint32_t mTmpSendSize;

	uint32_t mWriteBytes;
	uint32_t mRelayBytes;

};

/* utility functions for creating / extracting UdpRelayPackets */
int isUdpRelayPacket(const void *data, const int size);
int getPacketFromUdpRelayPacket(const void *data, const int size, void **realdata, int *realsize);

int createRelayUdpPacket(const void *data, const int size, void *newpkt, int newsize, UdpRelayEnd *ure);
int extractUdpRelayAddrSet(const void *data, const int size, UdpRelayAddrSet &addrSet);




#endif
