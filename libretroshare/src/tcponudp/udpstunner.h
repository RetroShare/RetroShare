#ifndef RS_UDP_STUN_H
#define RS_UDP_STUN_H
/*
 * tcponudp/udpstunner.h
 *
 * libretroshare.
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
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifdef WINDOWS_SYS
#include "util/rswin.h"
#endif

#ifndef WINDOWS_SYS
#include <netinet/in.h>
#endif

#include "tcponudp/rsudpstack.h"
#include "util/rsthreads.h"
#include <string>

/* UdpStun.
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
	
	std::string id;
	struct sockaddr_in remote, eaddr;
	bool response;
	time_t lastsend;
	uint32_t failCount;
};

/*
 * FOR TESTING ONLY.
 * #define UDPSTUN_ALLOW_LOCALNET	1	
 */

#define UDPSTUN_ALLOW_LOCALNET	1	

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

int	grabExclusiveMode();		/* returns seconds since last send/recv */
int     releaseExclusiveMode(bool forceStun);


void 	setTargetStunPeriod(int32_t sec_per_stun);
bool    addStunPeer(const struct sockaddr_in &remote, const char *peerid);
bool    getStunPeer(int idx, std::string &id,
                struct sockaddr_in &remote, struct sockaddr_in &eaddr,
                uint32_t &failCount, time_t &lastSend);

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
	time_t eaddrTime;

	time_t mStunLastRecvResp;
	time_t mStunLastRecvAny;
	time_t mStunLastSendStun;
	time_t mStunLastSendAny;

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
	time_t mExclusiveModeTS;
	bool mForceRestun;

};

	/* generic stun functions */

bool	UdpStun_isStunPacket(void *data, int size);
bool    UdpStun_response(void *stun_pkt, int size, struct sockaddr_in &addr);
void   *UdpStun_generate_stun_reply(struct sockaddr_in *stun_addr, int *len);
bool    UdpStun_generate_stun_pkt(void *stun_pkt, int *len);

#endif
