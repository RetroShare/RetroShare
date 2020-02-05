/*******************************************************************************
 * libretroshare/src/tcponudp: udpstunner.cc                                   *
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
#include "tcponudp/udpstunner.h"
#include <iostream>
#include "util/rstime.h"

#include "util/rsrandom.h"
#include "util/rsprint.h"
#include "util/rsmemory.h"
#include "util/rsstring.h"

static const int STUN_TTL = 64;

#define TOU_STUN_MIN_PEERS 20

/*
 * #define DEBUG_UDP_STUNNER 1
 * #define DEBUG_UDP_STUNNER_FILTER 1
 */

//#define DEBUG_UDP_STUNNER 1

const uint32_t TOU_STUN_MAX_FAIL_COUNT = 3; /* 3 tries (could be higher?) */
const int32_t TOU_STUN_MAX_SEND_RATE = 5;  /* every 5  seconds */
const uint32_t TOU_STUN_MAX_RECV_RATE = 25; /* every 25 seconds */
// TIMEOUT is now tied to  STUN RATE ... const int32_t TOU_STUN_ADDR_MAX_AGE  = 120; /* 2 minutes */

const int32_t TOU_STUN_DEFAULT_TARGET_RATE  = 15; /* 20 secs is minimum to keep a NAT UDP port open */
const double  TOU_SUCCESS_LPF_FACTOR = 0.90;

/*
 * based on RFC 3489
 */
const uint16_t STUN_BINDING_REQUEST  = 0x0001;
const uint16_t STUN_BINDING_RESPONSE = 0x0101;

#define EXCLUSIVE_MODE_TIMEOUT	300

UdpStunner::UdpStunner(UdpPublisher *pub)
	:UdpSubReceiver(pub), stunMtx("UdpSubReceiver"), eaddrKnown(false), eaddrStable(false),
        	mStunLastRecvResp(0), mStunLastRecvAny(0), 
		mStunLastSendStun(0), mStunLastSendAny(0)
{
#ifdef UDPSTUN_ALLOW_LOCALNET	
	mAcceptLocalNet = false;
	mSimExclusiveNat = false;
	mSimSymmetricNat = false;
	mSimUnstableExt = false;
#endif



	/* these parameters determine the rate we attempt stuns */
	mPassiveStunMode = false;
	mSuccessRate = 0.0; 
	mTargetStunPeriod = TOU_STUN_DEFAULT_TARGET_RATE;

	mExclusiveMode = false;
	mExclusiveModeTS = 0;
	mForceRestun = false;

	return;
}

#ifdef UDPSTUN_ALLOW_LOCALNET	

	// For Local Testing Only (Releases should have the #define disabled)
void	UdpStunner::SetAcceptLocalNet()
{
        RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	mAcceptLocalNet = true;
}

	// For Local Testing Only (Releases should have the #define disabled)
void	UdpStunner::SimExclusiveNat()
{
        RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	mSimExclusiveNat = true;
	mSimUnstableExt = true;
}

void	UdpStunner::SimSymmetricNat()
{
        RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	mSimSymmetricNat = true;
	mSimUnstableExt = true;
}



#endif



int	UdpStunner::grabExclusiveMode(std::string holder)  /* returns seconds since last send/recv */
{
        RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/
	rstime_t now = time(NULL);


#ifdef DEBUG_UDP_STUNNER_FILTER
	std::cerr << "UdpStunner::grabExclusiveMode()";
	std::cerr << std::endl;
#endif
	
	if (mExclusiveMode)
	{
#ifdef DEBUG_UDP_STUNNER_FILTER
		std::cerr << "UdpStunner::grabExclusiveMode() FAILED";
		std::cerr << std::endl;
#endif

		std::cerr << "UdpStunner::grabExclusiveMode() FAILED, already held by: " << mExclusiveHolder;
		std::cerr << std::endl;
		std::cerr << "UdpStunner::grabExclusiveMode() Was Grabbed: " << now - mExclusiveModeTS;
		std::cerr << " secs ago";
		std::cerr << std::endl;

		/* This can happen if AUTH, but START never received! (occasionally).
		 */
		if (now - mExclusiveModeTS > EXCLUSIVE_MODE_TIMEOUT)
		{
			mExclusiveMode = false;
			mForceRestun = true;

			std::cerr << "UdpStunner::grabExclusiveMode() Held for too Long... TIMEOUT & Stun Forced";
			std::cerr << std::endl;
		}
		
		return 0;
	}

	mExclusiveMode = true;
	mExclusiveModeTS = now;
        mExclusiveHolder = holder;

	int lastcomms = mStunLastRecvAny;
	if (mStunLastSendAny > lastcomms)
	{
		lastcomms = mStunLastSendAny;
	}

	int commsage = now - lastcomms;

	/* cannot return 0, as this indicates error */
	if (commsage == 0)
	{
		commsage = 1;
	}
#ifdef DEBUG_UDP_STUNNER_FILTER
	std::cerr << "UdpStunner::grabExclusiveMode() SUCCESS. last comms: " << commsage;
	std::cerr << " ago";
	std::cerr << std::endl;
	std::cerr << "UdpStunner::grabExclusiveMode() Exclusive held by: " << mExclusiveHolder;
	std::cerr << std::endl;
#endif

	return commsage;
}

int	UdpStunner::releaseExclusiveMode(std::string holder, bool forceStun)
{
        RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	if (!mExclusiveMode)
	{
#ifdef DEBUG_UDP_STUNNER_FILTER
		std::cerr << "UdpStunner::cancelExclusiveMode() ERROR, not in exclusive Mode";
		std::cerr << std::endl;
#endif
		return 0;
	}

	mExclusiveMode = false;
	if (forceStun)
	{
		mForceRestun = true;
	}

#ifdef UDPSTUN_ALLOW_LOCALNET	
	/* if we are simulating an exclusive NAT, then immediately after we release - it'll become unstable. 
	 * In reality, it will only become unstable if we have tried a UDP connection.
	 * so we use the forceStun parameter (which is true when a UDP connection has been tried).
	 */

	if ((mSimExclusiveNat) && (forceStun))
	{
		mSimUnstableExt = true;
	}
#endif

	if (mExclusiveHolder != holder)
	{
		std::cerr << "UdpStunner::cancelExclusiveMode() ERROR release MisMatch: ";
		std::cerr << " Original Grabber: ";
		std::cerr << mExclusiveHolder;
		std::cerr << " Releaser: ";
		std::cerr << holder;
		std::cerr << std::endl;
	}

#ifdef DEBUG_UDP_STUNNER_FILTER
	rstime_t now = time(NULL);
	std::cerr << "UdpStunner::cancelExclusiveMode() Canceled. Was in ExclusiveMode for: " << now - mExclusiveModeTS;
	std::cerr << " secs";
	std::cerr << std::endl;
#endif

	return 1;
}


void	UdpStunner::setTargetStunPeriod(int32_t sec_per_stun)
{
        RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	if (sec_per_stun < 0)
	{
		mPassiveStunMode = false;
		mTargetStunPeriod = TOU_STUN_DEFAULT_TARGET_RATE;
	}
	else
	{
		if (sec_per_stun == 0)
		{
			mPassiveStunMode = true;
		}
		else
		{
			mPassiveStunMode = false;
		}
		mTargetStunPeriod = sec_per_stun;
	}

}

/* higher level interface */
int UdpStunner::recvPkt(void *data, int size, struct sockaddr_in &from)
{
	/* print packet information */
#ifdef DEBUG_UDP_STUNNER_FILTER
	std::cerr << "UdpStunner::recvPkt(" << size << ") from: " << from;
	std::cerr << std::endl;
#endif

        RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	/* check for STUN packet */
	if (UdpStun_isStunPacket(data, size))
	{
		mStunLastRecvAny = time(NULL);
#ifdef DEBUG_UDP_STUNNER_FILTER
		std::cerr << "UdpStunner::recvPkt() is Stun Packet";
		std::cerr << std::endl;
#endif

		/* respond */
		locked_handleStunPkt(data, size, from);

		return 1;
	}
	return 0;
}


int     UdpStunner::status(std::ostream &out)
{
        RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	out << "UdpStunner::status() TargetStunPeriod: " << mTargetStunPeriod;
	out << " SuccessRate: " << mSuccessRate;
	out << std::endl;

	out << "UdpStunner::status()" << std::endl;

	locked_printStunList();

	return 1;
}

int UdpStunner::tick()
{

#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::tick()" << std::endl;
#endif

	if (checkStunDesired())
	{
		attemptStun();
#ifdef DEBUG_UDP_STUNNER
		status(std::cerr);
#endif
	}

	return 1;
}

/******************************* STUN Handling ********************************/

		/* respond */
bool UdpStunner::locked_handleStunPkt(void *data, int size, struct sockaddr_in &from)
{
	if (size == 20) /* request */
	{
#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::handleStunPkt() got Request from: ";
		std::cerr << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port);
		std::cerr << std::endl;
#endif
		/* generate a response */
		int len;
		void *pkt = UdpStun_generate_stun_reply(&from, &len, data);
		if (!pkt)
			return false;

		rstime_t now = time(NULL);
		mStunLastSendAny = now;
		int sentlen = sendPkt(pkt, len, from, STUN_TTL);
		free(pkt);

#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::handleStunPkt() sent Response size:" << sentlen;
		std::cerr << std::endl;
#endif

		return (len == sentlen);
	}
	else if (size == 28)
	{
#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::handleStunPkt() got Response";
		std::cerr << std::endl;
#endif
		/* got response */
		struct sockaddr_in eAddr;
		bool good = UdpStun_response(data, size, eAddr);
		if (good)
		{
#ifdef DEBUG_UDP_STUNNER
			std::cerr << "UdpStunner::handleStunPkt() got Ext Addr: ";
			std::cerr << inet_ntoa(eAddr.sin_addr) << ":" << ntohs(eAddr.sin_port);
			std::cerr << " from: " << from;
			std::cerr << std::endl;
#endif
			locked_recvdStun(from, eAddr);

			return true;
		}
	}

#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::handleStunPkt() Bad Packet";
	std::cerr << std::endl;
#endif
	return false;
}


bool    UdpStunner::externalAddr(struct sockaddr_in &external, uint8_t &stable)
{
        RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	if (eaddrKnown)
	{
		/* address timeout
		 * no timeout if in exclusive mode
		 */
		if ((time(NULL) - eaddrTime > (long) (mTargetStunPeriod * 2)) && (!mExclusiveMode))
		{
			std::cerr << "UdpStunner::externalAddr() eaddr expired";
			std::cerr << std::endl;

			eaddrKnown = false;
			return false;
		}
		
		/* Force Restun is triggered after an Exclusive Mode... as Ext Address is likely to have changed
		 * Until the Restun has got an address - we act as if we don't have an external address
		 */
		if (mForceRestun)
		{
			return false;
		}

		external = eaddr;

		if (eaddrStable)
			stable = 1;
		else
			stable = 0;

#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::externalAddr() eaddr:" << inet_ntoa(external.sin_addr);
		std::cerr << ":" << ntohs(external.sin_port) << " stable: " << (int) stable;
		std::cerr << std::endl;
#endif


		return true;
	}
#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::externalAddr() eaddr unknown";
	std::cerr << std::endl;
#endif

	return false;
}


int     UdpStunner::doStun(struct sockaddr_in stun_addr)
{
#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::doStun()";
	std::cerr << std::endl;
#endif

	/* send out a stun packet -> save in the local variable */

#define MAX_STUN_SIZE 64
	char stundata[MAX_STUN_SIZE];
	int tmplen = MAX_STUN_SIZE;
	bool done = UdpStun_generate_stun_pkt(stundata, &tmplen);
	if (!done)
	{
#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::doStun() Failed";
		std::cerr << std::endl;
#endif
		//pqioutput(PQL_ALERT, pqistunzone, "pqistunner::stun() Failed!");
		return 0;
	}

	/* send it off */
#ifdef DEBUG_UDP_STUNNER
	int sentlen =
#endif
	sendPkt(stundata, tmplen, stun_addr, STUN_TTL);

	{
		RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

		rstime_t now = time(NULL);
		mStunLastSendStun = now;
		mStunLastSendAny = now;
	}

#ifdef DEBUG_UDP_STUNNER
	std::string out;
	rs_sprintf(out, "UdpStunner::doStun() Sent Stun Packet(%d) to:%s:%u", sentlen, rs_inet_ntoa(stun_addr.sin_addr).c_str(), ntohs(stun_addr.sin_port));

	std::cerr << out << std::endl;

	//pqioutput(PQL_ALERT, pqistunzone, out);
#endif

	return 1;
}

/******************************* STUN Handling ********************************/
/***** These next functions are generic and not dependent on class variables **/
/******************************* STUN Handling ********************************/

bool    UdpStun_response(void *stun_pkt, int size, struct sockaddr_in &addr)
{
	/* check what type it is */
	if (size < 28)
	{
		return false;
	}

	if (htons(((uint16_t *) stun_pkt)[0]) != STUN_BINDING_RESPONSE)
	{
		/* not a response */
		return false;
	}

	/* iterate through the packet */
	/* for now assume the address follows the header directly */
	/* all stay in netbyteorder! */
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = ((uint32_t *) stun_pkt)[6];
	addr.sin_port = ((uint16_t *) stun_pkt)[11];


#ifdef DEBUG_UDP_STUNNER_FILTER
	std::string out;
	rs_sprintf(out, "UdpStunner::response() Recvd a Stun Response, ext_addr: %s:%u", rs_inet_ntoa(addr.sin_addr).c_str(), ntohs(addr.sin_port));
	std::cerr << out << std::endl;
#endif

	return true;

}

bool UdpStun_generate_stun_pkt(void *stun_pkt, int *len)
{
	if (*len < 20)
	{
		return false;
	}

	/* just the header */
	((uint16_t *) stun_pkt)[0] = (uint16_t) htons(STUN_BINDING_REQUEST);
	((uint16_t *) stun_pkt)[1] = (uint16_t) htons(20); /* only header */
	/* RFC 3489
	 *	The transaction ID is used to correlate requests and responses.
	 *
	 * RFC 5389 introduces a mmgic cokie at the location where preciously the transaction ID was located:
	 *	In RFC 3489 [RFC3489], this field was part of
	 *	the transaction ID; placing the magic cookie in this location allows
	 *	a server to detect if the client will understand certain attributes
	 *	that were added in this revised specification.
	 */
	((uint32_t *) stun_pkt)[1] = htonl(RsRandom::random_u32());
	((uint32_t *) stun_pkt)[2] = htonl(RsRandom::random_u32());
	((uint32_t *) stun_pkt)[3] = htonl(RsRandom::random_u32());
	((uint32_t *) stun_pkt)[4] = htonl(RsRandom::random_u32());
	*len = 20;
	return true;
}


void *UdpStun_generate_stun_reply(struct sockaddr_in *stun_addr, int *len, const void *data)
{
	/* just the header */
	void *stun_pkt = rs_malloc(28);
    
	if(!stun_pkt)
		return NULL ;
        
	((uint16_t *) stun_pkt)[0] = (uint16_t) htons(STUN_BINDING_RESPONSE);
	((uint16_t *) stun_pkt)[1] = (uint16_t) htons(28); /* only header + 8 byte addr */
	/* RFC 3489
	 *	The Binding Response MUST contain the same transaction ID contained in the Binding Request.
	 */
	memcpy(&((uint32_t *) stun_pkt)[1], &((uint32_t *) data)[1], 4 * sizeof (uint32_t));

	/* now add address
	 *  0  1    2  3
	 * <INET>  <port>
	 * <inet address>
	 */

	/* THESE SHOULD BE NET ORDER ALREADY */
	((uint16_t *) stun_pkt)[10] = AF_INET;
	((uint16_t *) stun_pkt)[11] = stun_addr->sin_port;
	((uint32_t *) stun_pkt)[6] =  stun_addr->sin_addr.s_addr;

	*len = 28;
	return stun_pkt;
}

bool UdpStun_isStunPacket(void *data, int size)
{
#ifdef DEBUG_UDP_STUNNER_FILTER
	std::cerr << "UdpStunner::isStunPacket() ?";
	std::cerr << std::endl;
#endif

	if (size < 20)
	{
#ifdef DEBUG_UDP_STUNNER_FILTER
		std::cerr << "UdpStunner::isStunPacket() (size < 20) -> false";
		std::cerr << std::endl;
#endif
		return false;
	}

	/* match size field */
	uint16_t pktsize = ntohs(((uint16_t *) data)[1]);
	if (size != pktsize)
	{
#ifdef DEBUG_UDP_STUNNER_FILTER
		std::cerr << "UdpStunner::isStunPacket() (size != pktsize) -> false";
		std::cerr << std::endl;
#endif
		return false;
	}

	if ((size == 20) && (STUN_BINDING_REQUEST == ntohs(((uint16_t *) data)[0])))
	{
#ifdef DEBUG_UDP_STUNNER_FILTER
		std::cerr << "UdpStunner::isStunPacket() (size=20 & data[0]=STUN_BINDING_REQUEST) -> true";
		std::cerr << std::endl;
#endif
		/* request */
		return true;
	}

	if ((size == 28) && (STUN_BINDING_RESPONSE == ntohs(((uint16_t *) data)[0])))
	{
#ifdef DEBUG_UDP_STUNNER_FILTER
		std::cerr << "UdpStunner::isStunPacket() (size=28 & data[0]=STUN_BINDING_RESPONSE) -> true";
		std::cerr << std::endl;
#endif
		/* response */
		return true;
	}
	return false;
}


/******************************* STUN Handling ********************************
 * KeepAlive has been replaced by a targetStunRate. Set this to zero to disable.
 */

/******************************* STUN Handling ********************************/


bool    UdpStunner::addStunPeer(const struct sockaddr_in &remote, const char *peerid)
{
	/* add to the list */
#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::addStunPeer()";
	std::cerr << std::endl;
#endif

	bool toStore = true;
	{
        	RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

		/* only store if we're active */
		toStore = !mPassiveStunMode;
	}

	if (toStore)
	{
		storeStunPeer(remote, peerid, 0);
	}
	return true;
}

bool    UdpStunner::storeStunPeer(const struct sockaddr_in &remote, const char *peerid, bool sent)
{

#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::storeStunPeer()";
	std::cerr << std::endl;
#endif

        RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	std::list<TouStunPeer>::iterator it;
	for(it = mStunList.begin(); it != mStunList.end(); ++it)
	{
		if ((remote.sin_addr.s_addr == it->remote.sin_addr.s_addr) &&
		    (remote.sin_port == it->remote.sin_port))
		{
#ifdef DEBUG_UDP_STUNNER
			std::cerr << "UdpStunner::storeStunPeer() Peer Already There!";
			std::cerr << std::endl;
#endif
			/* already there */
			if (sent)
			{
				it->failCount += 1;
				it->lastsend = time(NULL);
			}
			return false;
		}
	}

	std::string peerstring;
	if (peerid)
	{
		peerstring = std::string(peerid);
	}

	TouStunPeer peer(peerstring, remote);
	if (sent)
	{
		peer.failCount += 1;
		peer.lastsend = time(NULL);
	}

	mStunList.push_back(peer);

#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::storeStunPeer() Added Peer";
	std::cerr << std::endl;
#endif

	return true;
}


bool    UdpStunner::dropStunPeer(const struct sockaddr_in &remote)
{

#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::dropStunPeer() : ";
	std::cerr << std::endl;
#endif

        RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	std::list<TouStunPeer>::iterator it;
	int count = 0;
	for(it = mStunList.begin(); it != mStunList.end();)
	{
		if ((remote.sin_addr.s_addr == it->remote.sin_addr.s_addr) &&
		    (remote.sin_port == it->remote.sin_port))
		{
#ifdef DEBUG_UDP_STUNNER
			std::cerr << "UdpStunner::dropStunPeer() Found Entry";
			std::cerr << std::endl;
#endif

			it = mStunList.erase(it);
			count++;
		}
		else
		{
			++it;
		}
	}

	if (count)
	{
#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::dropStunPeer() Dropped " << count << " Instances";
		std::cerr << std::endl;
#endif
		return true;
	}

#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::dropStunPeer() Peer Not Here";
	std::cerr << std::endl;
#endif

	return false;
}


bool    UdpStunner::checkStunDesired()
{

#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::checkStunDesired()";
	std::cerr << std::endl;
#endif

	rstime_t now;
	{
          RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	  if (mPassiveStunMode)
	  {
#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::checkStunDesired() In Passive Mode";
		std::cerr << std::endl;
#endif
		return false; /* all good */
	  }

	  if (mExclusiveMode)
	  {
		return false; /* no pings in exclusive mode */
	  }
		
	  if (mForceRestun)
	  {
			return true;
	  }
		

	  if (!eaddrKnown)
	  {
		/* check properly! (this will limit it to two successful stuns) */
		if (!locked_checkExternalAddress())
		{
#ifdef DEBUG_UDP_STUNNER
			std::cerr << "UdpStunner::checkStunDesired() YES, we don't have extAddr Yet";
			std::cerr << std::endl;
#endif
			return true; /* want our external address */
		}
	}

	  /* check if we need to send one now */
	  now = time(NULL);

	  /* based on SuccessRate & TargetStunRate, we work out if we should send one 
	   *
	   * if we have 100% success rate, then we can delay until exactly TARGET RATE.
	   * if we have 0% success rate, then try at double TARGET RATE.
	   *
	   * generalised to a rate_scale parameter below...
	   */

#define RATE_SCALE (3.0)
	  double stunPeriod = (mTargetStunPeriod / (RATE_SCALE)) * (1.0 + mSuccessRate * (RATE_SCALE - 1.0));
	  rstime_t nextStun = mStunLastRecvResp + (int) stunPeriod;

#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::checkStunDesired() TargetStunPeriod: " << mTargetStunPeriod;
	std::cerr << " SuccessRate: " << mSuccessRate;
	std::cerr << " DesiredStunPeriod: " << stunPeriod;
	std::cerr << " NextStun: " << nextStun - now << " secs";
	std::cerr << std::endl;
#endif

	  if (now >= nextStun)
	  {
#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::checkStunDesired() Stun is Desired";
		std::cerr << std::endl;
#endif
		return true;
	  }
	  else
	  {
#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::checkStunDesired() Stun is Not Needed";
		std::cerr << std::endl;
#endif
		return false;
	  }
	}
}


bool    UdpStunner::attemptStun()
{
	bool found = false;
	TouStunPeer peer;
	rstime_t now = time(NULL);

#ifdef DEBUG_UDP_STUNNER
	std::cerr << "UdpStunner::attemptStun()";
	std::cerr << std::endl;
#endif

	{
          RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	  size_t i;
	  for(i = 0; ((i < mStunList.size()) && (mStunList.size() > 0) && (!found)); i++)
	  {
	  	/* extract entry */
		peer = mStunList.front();
		mStunList.pop_front();

		/* check if expired */
		if (peer.failCount > TOU_STUN_MAX_FAIL_COUNT)
		{
#ifdef DEBUG_UDP_STUNNER
			std::cerr << "UdpStunner::attemptStun() Peer has expired, dropping";
			std::cerr << std::endl;
#endif
		}
		else
		{
			// Peer Okay, check last send time.
			if (now - peer.lastsend < TOU_STUN_MAX_SEND_RATE) 
			{
#ifdef DEBUG_UDP_STUNNER
				std::cerr << "UdpStunner::attemptStun() Peer was sent to Too Recently, pushing back";
				std::cerr << std::endl;
#endif
				mStunList.push_back(peer);
			}
			else
			{
				/* we have found a peer! */
#ifdef DEBUG_UDP_STUNNER
				std::cerr << "UdpStunner::attemptStun() Found Peer to Stun.";
				std::cerr << std::endl;
#endif
				peer.failCount++;
				peer.lastsend = now;
				mStunList.push_back(peer);
				mSuccessRate *= TOU_SUCCESS_LPF_FACTOR;

				found = true;
			}
		}
	  } // END OF WHILE LOOP.

	  if (mStunList.size() < 1)
	  {
#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::attemptStun() No Peers in List. FAILED";
		std::cerr << std::endl;
#endif
		return false;
	  }

#ifdef DEBUG_UDP_STUNNER
	  locked_printStunList();
#endif

	} // END OF MUTEX LOCKING.

	if (found)
	{
		doStun(peer.remote);
		return true;
	}
	return false;
}


bool    UdpStunner::locked_recvdStun(const struct sockaddr_in &remote, const struct sockaddr_in &extaddr)
{
#ifdef DEBUG_UDP_STUNNER
	std::string out;
	rs_sprintf(out, "UdpStunner::locked_recvdStun() from:%s:%u", rs_inet_ntoa(remote.sin_addr).c_str(), ntohs(remote.sin_port));
	rs_sprintf_append(out, " claiming ExtAddr is:%s:%u", rs_inet_ntoa(extaddr.sin_addr).c_str(), ntohs(extaddr.sin_port));

	std::cerr << out << std::endl;
#endif

#ifdef UDPSTUN_ALLOW_LOCALNET	
	struct sockaddr_in fakeExtaddr = extaddr;
	if (mSimUnstableExt)
	{
		std::cerr << "UdpStunner::locked_recvdStun() TEST SIM UNSTABLE EXT: Forcing Port to be wrong to sim an ExclusiveNat";
		std::cerr << std::endl;

#define UNSTABLE_PORT_RANGE 100

		fakeExtaddr.sin_port = htons(ntohs(fakeExtaddr.sin_port) - (UNSTABLE_PORT_RANGE / 2) + RSRandom::random_u32() % UNSTABLE_PORT_RANGE);
		if (!mSimSymmetricNat)
		{
			mSimUnstableExt = false;
		}
	}
#endif

	/* sanoty checks on the address 
	 * have nasty peer that is returning its own address....
	 */

#ifndef UDPSTUN_ALLOW_LOCALNET // CANNOT HAVE THIS CHECK IN TESTING MODE!

	if (remote.sin_addr.s_addr == extaddr.sin_addr.s_addr)
	{
#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::locked_recvdStun() WARNING, BAD PEER: ";
		std::cerr << "Stun Peer Returned its own address: " << rs_inet_ntoa(remote.sin_addr);
		std::cerr << std::endl;
#endif
		return false;
	}
#endif

	bool found = false;
	std::list<TouStunPeer>::iterator it;
	for(it = mStunList.begin(); it != mStunList.end(); ++it)
	{
		if ((remote.sin_addr.s_addr == it->remote.sin_addr.s_addr) &&
		    (remote.sin_port == it->remote.sin_port))
		{
			it->failCount = 0;
#ifdef UDPSTUN_ALLOW_LOCALNET
			it->eaddr = fakeExtaddr;
#else			
			it->eaddr = extaddr;
#endif
			it->response = true;

			found = true;
			break;
		}
	}

	/* We've received a Stun, so the ForceStun can be cancelled */
	if (found)
	{
		mForceRestun = false;
	}
	
	/* if not found.. should we add it back in? */
	
	/* How do we calculate the success rate?
	 * Don't want to count all the stuns?
	 * Low Pass filter won't work either...
	 * at send... 
	 *		mSuccessRate = 0.95 * mSuccessRate.
	 * at recv...
	 *      mSuccessRate = 0.95 * mSuccessRate + 0.05;
	 *
	 * But if we split into a two stage eqn. it'll work!
	 * a
	 *		mSuccessRate = 0.95 * mSuccessRate.
	 * at recv...
	 *      mSuccessRate +=  0.05;
	 */
	 
	mSuccessRate += (1.0-TOU_SUCCESS_LPF_FACTOR); 

	rstime_t now = time(NULL);	
        mStunLastRecvResp = now;
        mStunLastRecvAny = now;

#ifdef DEBUG_UDP_STUNNER
	locked_printStunList();
#endif

	if (!mExclusiveMode)
	{
		locked_checkExternalAddress();
	}

	return found;
}

bool    UdpStunner::locked_checkExternalAddress()
{
#ifdef DEBUG_UDP_STUNNER
	std::string out = "UdpStunner::locked_checkExternalAddress()";
	std::cerr << out << std::endl;
#endif

	bool found1 = false;
	bool found2 = false;
	rstime_t now = time(NULL);
	/* iterator backwards - as these are the most recent */

	/********
	 *  DUE TO PEERS SENDING BACK FAKE STUN PACKETS... we are increasing.
	 *  requirements to three peers...they all need matching IP addresses to have a known ExtAddr
	 *
	 * Wanted to compare 3 peer addresses... but this will mean that the UDP connections
	 * will take much longer... have to think of a better solution.
	 *
	 */
	std::list<TouStunPeer>::reverse_iterator it;
	std::list<TouStunPeer>::reverse_iterator p1;
	std::list<TouStunPeer>::reverse_iterator p2;
	for(it = mStunList.rbegin(); it != mStunList.rend(); ++it)
	{
		/* check:
		   1) have response.
		   2) have eaddr.
		   3) no fails.
		   4) recent age.
		 */

		rstime_t age = (now - it->lastsend);
		if (it->response && 
#ifdef UDPSTUN_ALLOW_LOCALNET	
			( mAcceptLocalNet || isExternalNet(&(it->eaddr.sin_addr))) &&
#else
			(isExternalNet(&(it->eaddr.sin_addr))) &&
#endif
			(it->failCount == 0) && (age < (long) (mTargetStunPeriod * 2)))
		{
			if (!found1)
			{
				p1 = it;
				found1 = true;
			}
			else
			{
				p2 = it;
				found2 = true;
				break;
			}
		}
	}

	if (found1 && found2)
	{
		/* If any of the addresses are different - two possibilities...
		 * 1) We have changed IP address.
		 * 2) Someone has sent us a fake STUN Packet. (Wrong Address).
		 *
		 */
		if (p1->eaddr.sin_addr.s_addr == p2->eaddr.sin_addr.s_addr)
		{
			eaddrKnown = true;
		}
		else
		{
#ifdef DEBUG_UDP_STUNNER
			std::cerr << "UdpStunner::locked_checkExternalAddress() Found Address mismatch:";
			std::cerr << std::endl;
			std::cerr << "  " << inet_ntoa(p1->eaddr.sin_addr);
			std::cerr << "  " << inet_ntoa(p2->eaddr.sin_addr);
			std::cerr << std::endl;
			std::cerr << "UdpStunner::locked_checkExternalAddress() Flagging Ext Addr as Unknown";
			std::cerr << std::endl;
#endif
			eaddrKnown = false;
		}

		if ((eaddrKnown) &&	
		    (p1->eaddr.sin_port == p2->eaddr.sin_port))
		{
			eaddrStable = true;
		}
		else
		{
			eaddrStable = false;
		}

		eaddr = p1->eaddr;
		eaddrTime = now;

#ifdef DEBUG_UDP_STUNNER
		std::cerr << "UdpStunner::locked_checkExternalAddress() Found State:";
		if (eaddrStable)
			std::cerr << " Stable NAT translation (GOOD!) ";
		else
			std::cerr << " unStable (symmetric NAT translation (BAD!) or Address Unknown";

		std::cerr << std::endl;
#endif

		return true;
	}

	return false;
}



bool    UdpStunner::locked_printStunList()
{
#ifdef DEBUG_UDP_STUNNER
	std::string out = "locked_printStunList()\n";

	rstime_t now = time(NULL);
	rs_sprintf_append(out, "\tLastSendStun: %ld\n", now - mStunLastSendStun);
	rs_sprintf_append(out, "\tLastSendAny: %ld\n", now - mStunLastSendAny);
	rs_sprintf_append(out, "\tLastRecvResp: %ld\n", now - mStunLastRecvResp);
	rs_sprintf_append(out, "\tLastRecvAny: %ld\n", now - mStunLastRecvAny);

	std::list<TouStunPeer>::iterator it;
	for(it = mStunList.begin(); it != mStunList.end(); ++it)
	{
		out += "id:" + RsUtil::BinToHex(it->id);
		rs_sprintf_append(out, " addr: %s:%u", rs_inet_ntoa(it->remote.sin_addr).c_str(), htons(it->remote.sin_port));
		rs_sprintf_append(out, " eaddr: %s:%u", rs_inet_ntoa(it->eaddr.sin_addr).c_str(), htons(it->eaddr.sin_port));
		rs_sprintf_append(out, " failCount: %lu", it->failCount);
		rs_sprintf_append(out, " lastSend: %ld\n", now - it->lastsend);
	}

	std::cerr << out;
#endif

	return true;
}

	
bool    UdpStunner::getStunPeer(int idx, std::string &id,
		struct sockaddr_in &remote, struct sockaddr_in &eaddr, 
		uint32_t &failCount, rstime_t &lastSend)
{
	RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	std::list<TouStunPeer>::iterator it;
	int i;
	for(i=0, it=mStunList.begin(); (i<idx) && (it!=mStunList.end()); ++it, i++) ;

	if (it != mStunList.end())
	{
		id = RsUtil::BinToHex(it->id);
		remote = it->remote;
		eaddr = it->eaddr;
		failCount = it->failCount;
		lastSend = it->lastsend;
		return true;
	}

	return false;
}


bool    UdpStunner::needStunPeers()
{
	RsStackMutex stack(stunMtx);   /********** LOCK MUTEX *********/

	return (mStunList.size() < TOU_STUN_MIN_PEERS);
}


