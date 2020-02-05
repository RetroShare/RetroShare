/*******************************************************************************
 * libretroshare/src/tcponudp: udpstunner.h                                    *
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
#ifndef RS_UDP_STUN_H
#define RS_UDP_STUN_H

#ifndef WINDOWS_SYS
#include <netinet/in.h>
#endif

#include "tcponudp/rsudpstack.h"
#include "util/rsthreads.h"
#include <string>

/**
 * @brief The TouStunPeer class
 * Stuns peers to determine external addresses.
 */
class TouStunPeer
{
	public:
	TouStunPeer()
	:response(false), lastsend(0), failCount(0) 
	{ 
		eaddr.sin_addr.s_addr = 0;
		eaddr.sin_port = 0;
		return; 
	}
	
	TouStunPeer(std::string id_in, const struct sockaddr_in &addr)
	:id(id_in), remote(addr), response(false), lastsend(0), failCount(0)
	{ 
		eaddr.sin_addr.s_addr = 0;
		eaddr.sin_port = 0;
		return; 
	}
	
	/// id for identification
	std::string id;
	/// remote address
	struct sockaddr_in remote, eaddr;
	/// true when a response was received in the past
	bool response;
	/// used to rate limit STUN requests
	rstime_t lastsend;
	/// fail counter for dead/bad peer detection (0 = good)
	uint32_t failCount;
};

/*
 * FOR TESTING ONLY.
 * #define UDPSTUN_ALLOW_LOCALNET	1	
 */

/**
 * @brief The UdpStunner class
 * The UDP stunner implements the STUN protocol to determin the NAT type (behind that RS is usually running).
 * It maintains a list of DHT peers that are regulary contacted.
 *
 * The actual NAT type determination logic is located in void pqiNetStateBox::determineNetworkState()
 */
class UdpStunner: public UdpSubReceiver
{
	public:

	UdpStunner(UdpPublisher *pub);
virtual ~UdpStunner() { return; }

#ifdef UDPSTUN_ALLOW_LOCALNET
	// For Local Testing Mode.
	void SetAcceptLocalNet();
	void SimExclusiveNat();
	void SimSymmetricNat();
#endif

int	grabExclusiveMode(std::string holder);		/* returns seconds since last send/recv */
int     releaseExclusiveMode(std::string holder, bool forceStun);


void 	setTargetStunPeriod(int32_t sec_per_stun);
bool    addStunPeer(const struct sockaddr_in &remote, const char *peerid);
bool    dropStunPeer(const struct sockaddr_in &remote);

bool    getStunPeer(int idx, std::string &id,
                struct sockaddr_in &remote, struct sockaddr_in &eaddr,
                uint32_t &failCount, rstime_t &lastSend);

bool	needStunPeers();

bool    externalAddr(struct sockaddr_in &remote, uint8_t &stable);

	/* Packet IO */
virtual int recvPkt(void *data, int size, struct sockaddr_in &from);
virtual int status(std::ostream &out);

	/* monitoring / updates */
	int tick();

	private:

bool  	checkStunDesired();
bool 	attemptStun();

int     doStun(struct sockaddr_in stun_addr);
bool    storeStunPeer(const struct sockaddr_in &remote, const char *peerid, bool sent);


	/* STUN handling */
bool 	locked_handleStunPkt(void *data, int size, struct sockaddr_in &from);

bool    locked_printStunList();
bool    locked_recvdStun(const struct sockaddr_in &remote, const struct sockaddr_in &extaddr);
bool    locked_checkExternalAddress();


	RsMutex stunMtx; /* for all class data (below) */

	struct sockaddr_in eaddr; /* external addr */

	bool eaddrKnown;
	bool eaddrStable; /* if true then usable. if false -> Symmettric NAT */
	rstime_t eaddrTime;

	rstime_t mStunLastRecvResp;
	rstime_t mStunLastRecvAny;
	rstime_t mStunLastSendStun;
	rstime_t mStunLastSendAny;

	std::list<TouStunPeer> mStunList; /* potentials */

#ifdef UDPSTUN_ALLOW_LOCALNET
	// For Local Testing Mode.
        bool mAcceptLocalNet;
	bool mSimUnstableExt;
        bool mSimExclusiveNat;
        bool mSimSymmetricNat;

#endif

	bool mPassiveStunMode;
	uint32_t mTargetStunPeriod;
	double mSuccessRate;

	bool mExclusiveMode; /* when this is switched on, the stunner stays silent (and extAddr is maintained) */
	rstime_t mExclusiveModeTS;
	std::string mExclusiveHolder;
	bool mForceRestun;

};

	/* generic stun functions */

bool	UdpStun_isStunPacket(void *data, int size);
bool    UdpStun_response(void *stun_pkt, int size, struct sockaddr_in &addr);
void   *UdpStun_generate_stun_reply(struct sockaddr_in *stun_addr, int *len, const void* data);
bool    UdpStun_generate_stun_pkt(void *stun_pkt, int *len);

#endif
