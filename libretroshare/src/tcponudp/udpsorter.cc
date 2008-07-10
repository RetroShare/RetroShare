/*
 * libretroshare/src/tcponudp: udpsorter.cc
 *
 * TCP-on-UDP (tou) network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#include "udpsorter.h"
#include "util/rsnet.h"
#include "util/rsprint.h"

#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "util/rsdebug.h"
const int rsudpsorterzone = 28477;

static const int STUN_TTL = 64;

/*
 * #define DEBUG_UDP_SORTER 1
 */


UdpSorter::UdpSorter(struct sockaddr_in &local)
	:udpLayer(NULL), laddr(local), eaddrKnown(false), eaddrStable(false),
        mStunKeepAlive(false), mStunLastRecv(0), mStunLastSend(0)


{
	sockaddr_clear(&eaddr);

	openSocket();
	return;
}


/* higher level interface */
void UdpSorter::recvPkt(void *data, int size, struct sockaddr_in &from)
{
	/* print packet information */
#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::recvPkt(" << size << ") from: " << from;
	std::cerr << std::endl;
#endif

        sortMtx.lock();   /********** LOCK MUTEX *********/
	mStunLastRecv = time(NULL);

	/* look for a peer */
        std::map<struct sockaddr_in, UdpPeer *>::iterator it;
	it = streams.find(from);

	/* check for STUN packet */
	if (UdpStun_isStunPacket(data, size))
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::recvPkt() is Stun Packet";
		std::cerr << std::endl;
#endif

		/* respond */
		locked_handleStunPkt(data, size, from);
	}
	else if (it == streams.end())
	{
		/* peer unknown */
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::recvPkt() Peer Unknown!";
		std::cerr << std::endl;
#endif
		std::ostringstream out;
		out << "UdpSorter::recvPkt() ";
		out << "from unknown: " << from;
		rslog(RSL_WARNING,rsudpsorterzone,out.str());
	}
	else
	{
		/* forward to them */
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::recvPkt() Sending to UdpPeer: ";
		std::cerr << it->first;
		std::cerr << std::endl;
#endif
		(it->second)->recvPkt(data, size);
	}

        sortMtx.unlock();   /******** UNLOCK MUTEX *********/
	/* done */
}

	
int  UdpSorter::sendPkt(void *data, int size, struct sockaddr_in &to, int ttl)
{
	/* print packet information */
#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::sendPkt(" << size << ") ttl: " << ttl;
	std::cerr << " to: " << to;
	std::cerr << std::endl;
#endif

	/* send to udpLayer */
	return udpLayer->sendPkt(data, size, to, ttl);
}

int     UdpSorter::status(std::ostream &out)
{
        sortMtx.lock();   /********** LOCK MUTEX *********/

	out << "UdpSorter::status()" << std::endl;
	out << "localaddr: " << laddr << std::endl;
	out << "UdpSorter::peers:" << std::endl;
        std::map<struct sockaddr_in, UdpPeer *>::iterator it;
	for(it = streams.begin(); it != streams.end(); it++)
	{
		out << "\t" << it->first << std::endl;
	}
	out << std::endl;

        sortMtx.unlock();   /******** UNLOCK MUTEX *********/

	udpLayer->status(out);

	return 1;
}

/* setup connections */
int UdpSorter::openSocket()	
{
	udpLayer = new UdpLayer(this, laddr);
	udpLayer->start();

	return 1;
}

/* monitoring / updates */
int UdpSorter::okay()
{
	return udpLayer->okay();
}

int UdpSorter::tick()
{
#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::tick()" << std::endl;
#endif
	checkStunKeepAlive();

	return 1;
}


int UdpSorter::close()
{
	/* TODO */
	return 1;
}


        /* add a TCPonUDP stream */
int UdpSorter::addUdpPeer(UdpPeer *peer, const struct sockaddr_in &raddr)
{
        sortMtx.lock();   /********** LOCK MUTEX *********/


	/* check for duplicate */
        std::map<struct sockaddr_in, UdpPeer *>::iterator it;
	it = streams.find(raddr);
	bool ok = (it == streams.end());
	if (!ok)
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::addUdpPeer() Peer already exists!" << std::endl;
		std::cerr << "UdpSorter::addUdpPeer() ERROR" << std::endl;
#endif
	}
	else
	{
		streams[raddr] = peer;
	}

        sortMtx.unlock();   /******** UNLOCK MUTEX *********/
	return ok;
}

int UdpSorter::removeUdpPeer(UdpPeer *peer)
{
        RsStackMutex stack(sortMtx);   /********** LOCK MUTEX *********/

	/* check for duplicate */
        std::map<struct sockaddr_in, UdpPeer *>::iterator it;
	for(it = streams.begin(); it != streams.end(); it++)
	{
		if (it->second == peer)
		{
#ifdef DEBUG_UDP_SORTER
			std::cerr << "UdpSorter::removeUdpPeer() SUCCESS" << std::endl;
#endif
			streams.erase(it);
			return 1;
		}
	}

#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::removeUdpPeer() ERROR" << std::endl;
#endif
	return 0;
}


/******************************* STUN Handling ********************************/

		/* respond */
bool UdpSorter::locked_handleStunPkt(void *data, int size, struct sockaddr_in &from)
{
	if (size == 20) /* request */
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::handleStunPkt() got Request from: ";
		std::cerr << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port);
		std::cerr << std::endl;
#endif
		{
			std::ostringstream out;
			out << "UdpSorter::handleStunPkt() got Request from: " << from;
			rslog(RSL_WARNING,rsudpsorterzone,out.str());
		}

		/* generate a response */
		int len;
		void *pkt = UdpStun_generate_stun_reply(&from, &len);
		if (!pkt)
			return false;

		int sentlen = sendPkt(pkt, len, from, STUN_TTL);
		free(pkt);

#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::handleStunPkt() sent Response size:" << sentlen;
		std::cerr << std::endl;
#endif

		return (len == sentlen);
	}
	else if (size == 28)
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::handleStunPkt() got Response";
		std::cerr << std::endl;
#endif
		/* got response */
		struct sockaddr_in eAddr;
		bool good = UdpStun_response(data, size, eAddr);
		if (good)
		{
#ifdef DEBUG_UDP_SORTER
			std::cerr << "UdpSorter::handleStunPkt() got Ext Addr: ";
			std::cerr << inet_ntoa(eAddr.sin_addr) << ":" << ntohs(eAddr.sin_port);
			std::cerr << std::endl;
#endif
			{
				std::ostringstream out;
				out << "UdpSorter::handleStunPkt() got Response from: " << from;
				out << " Ext Addr: " << eAddr;
				rslog(RSL_WARNING,rsudpsorterzone,out.str());
			}

			locked_recvdStun(from, eAddr);

			return true;
		}
	}

#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::handleStunPkt() Bad Packet";
	std::cerr << std::endl;
#endif
	return false;
}


bool    UdpSorter::externalAddr(struct sockaddr_in &external, uint8_t &stable)
{
	if (eaddrKnown)
	{
		external = eaddr;

		if (eaddrStable)
			stable = 1;
		else
			stable = 0;

		return true;
	}
	return false;
}


int     UdpSorter::doStun(struct sockaddr_in stun_addr)
{
#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::doStun()";
	std::cerr << std::endl;
#endif

	/* send out a stun packet -> save in the local variable */
	if (!okay())
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::doStun() Not Active";
		std::cerr << std::endl;
#endif
	}

#define MAX_STUN_SIZE 64
	char stundata[MAX_STUN_SIZE];
	int tmplen = MAX_STUN_SIZE;
	bool done = UdpStun_generate_stun_pkt(stundata, &tmplen);
	if (!done)
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::doStun() Failed";
		std::cerr << std::endl;
#endif
		//pqioutput(PQL_ALERT, pqistunzone, "pqistunner::stun() Failed!");
		return 0;
	}

	/* send it off */
	int sentlen = sendPkt(stundata, tmplen, stun_addr, STUN_TTL);

        sortMtx.lock();   /********** LOCK MUTEX *********/
	mStunLastSend = time(NULL);
        sortMtx.unlock();   /******** UNLOCK MUTEX *********/

#ifdef DEBUG_UDP_SORTER
	std::ostringstream out;
	out << "UdpSorter::doStun() Sent Stun Packet(" << sentlen << ") from:";
	out << inet_ntoa(laddr.sin_addr) << ":" << ntohs(laddr.sin_port);
	out << " to:";
	out << inet_ntoa(stun_addr.sin_addr) << ":" << ntohs(stun_addr.sin_port);

	std::cerr << out.str() << std::endl;

	//pqioutput(PQL_ALERT, pqistunzone, out.str());
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

	if (htons(((uint16_t *) stun_pkt)[0]) != 0x0101)
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


#ifdef DEBUG_UDP_SORTER
	std::ostringstream out;
	out << "UdpSorter::response() Recvd a Stun Response, ext_addr: ";
	out << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
	std::cerr << out.str() << std::endl;
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
	((uint16_t *) stun_pkt)[0] = (uint16_t) htons(0x0001);
	((uint16_t *) stun_pkt)[1] = (uint16_t) htons(20); /* only header */
	/* transaction id - should be random */
	((uint32_t *) stun_pkt)[1] = (uint32_t) htonl(0x0020); 
	((uint32_t *) stun_pkt)[2] = (uint32_t) htonl(0x0121); 
	((uint32_t *) stun_pkt)[3] = (uint32_t) htonl(0x0111); 
	((uint32_t *) stun_pkt)[4] = (uint32_t) htonl(0x1010); 
	*len = 20;
	return true;
}


void *UdpStun_generate_stun_reply(struct sockaddr_in *stun_addr, int *len)
{
	/* just the header */
	void *stun_pkt = malloc(28);
	((uint16_t *) stun_pkt)[0] = (uint16_t) htons(0x0101);
	((uint16_t *) stun_pkt)[1] = (uint16_t) htons(28); /* only header + 8 byte addr */
	/* transaction id - should be random */
	((uint32_t *) stun_pkt)[1] = (uint32_t) htonl(0x0f20); 
	((uint32_t *) stun_pkt)[2] = (uint32_t) htonl(0x0f21); 
	((uint32_t *) stun_pkt)[3] = (uint32_t) htonl(0x0f11); 
	((uint32_t *) stun_pkt)[4] = (uint32_t) htonl(0x1010); 
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
#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::isStunPacket() ?";
	std::cerr << std::endl;
#endif

	if (size < 20)
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::isStunPacket() (size < 20) -> false";
		std::cerr << std::endl;
#endif
		return false;
	}

	/* match size field */
	uint16_t pktsize = ntohs(((uint16_t *) data)[1]);
	if (size != pktsize)
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::isStunPacket() (size != pktsize) -> false";
		std::cerr << std::endl;
#endif
		return false;
	}

	if ((size == 20) && (0x0001 == ntohs(((uint16_t *) data)[0])))
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::isStunPacket() (size=20 & data[0]=0x0001) -> true";
		std::cerr << std::endl;
#endif
		/* request */
		return true;
	}

	if ((size == 28) && (0x0101 == ntohs(((uint16_t *) data)[0])))
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::isStunPacket() (size=28 & data[0]=0x0101) -> true";
		std::cerr << std::endl;
#endif
		/* response */
		return true;
	}
	return false;
}


/******************************* STUN Handling ********************************
 * The KeepAlive part - slightly more complicated
 *
 *
 */

const int32_t TOU_STUN_MAX_FAIL_COUNT = 10; /* 10 tries (could be higher?) */
const int32_t TOU_STUN_MAX_SEND_RATE = 5;  /* every 5  seconds */
const int32_t TOU_STUN_MAX_RECV_RATE = 25; /* every 25 seconds */

/******************************* STUN Handling ********************************/

bool UdpSorter::setStunKeepAlive(uint32_t required)
{
        sortMtx.lock();   /********** LOCK MUTEX *********/

	mStunKeepAlive = (required != 0);

#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::setStunKeepAlive() to: " << mStunKeepAlive;
	std::cerr << std::endl;
#endif
        sortMtx.unlock();   /******** UNLOCK MUTEX *********/

	return 1;
}

bool    UdpSorter::addStunPeer(const struct sockaddr_in &remote, const char *peerid)
{
	/* add to the list */
#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::addStunPeer()";
	std::cerr << std::endl;
#endif

	storeStunPeer(remote, peerid);

        sortMtx.lock();   /********** LOCK MUTEX *********/
	bool needStun = (!eaddrKnown);
        sortMtx.unlock();   /******** UNLOCK MUTEX *********/

	if (needStun)
	{
		doStun(remote);
	}

	return true;
}

bool    UdpSorter::storeStunPeer(const struct sockaddr_in &remote, const char *peerid)
{

#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::storeStunPeer()";
	std::cerr << std::endl;
#endif

        RsStackMutex stack(sortMtx);   /********** LOCK MUTEX *********/

	std::list<TouStunPeer>::iterator it;
	for(it = mStunList.begin(); it != mStunList.end(); it++)
	{
		if ((remote.sin_addr.s_addr == it->remote.sin_addr.s_addr) &&
		    (remote.sin_port == it->remote.sin_port))
		{
#ifdef DEBUG_UDP_SORTER
			std::cerr << "UdpSorter::storeStunPeer() Peer Already There!";
			std::cerr << std::endl;
#endif
			/* already there */
			return false;
		}
	}

	TouStunPeer peer(std::string(peerid), remote);
	mStunList.push_back(peer);

#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::storeStunPeer() Added Peer";
	std::cerr << std::endl;
#endif

	return true;
}


bool    UdpSorter::checkStunKeepAlive()
{

#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::checkStunKeepAlive()";
	std::cerr << std::endl;
#endif

	TouStunPeer peer;
	time_t now;
	{
          RsStackMutex stack(sortMtx);   /********** LOCK MUTEX *********/

	  if (!mStunKeepAlive)
	  {
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::checkStunKeepAlive() FALSE";
		std::cerr << std::endl;
#endif
		return false; /* all good */
	  }

	  /* check if we need to send one now */
	  now = time(NULL);

	  if ((now - mStunLastSend < TOU_STUN_MAX_SEND_RATE) || 
	      (now - mStunLastRecv < TOU_STUN_MAX_RECV_RATE))
	  {
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::checkStunKeepAlive() To Fast ... delaying";
		std::cerr << std::endl;
#endif
	  	/* too fast */
		return false;
	  }

	  if (mStunList.size() < 1)
	  {
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::checkStunKeepAlive() No Peers in List!";
		std::cerr << std::endl;
#endif
		return false;
	  }

	  /* extract entry */
	  peer = mStunList.front();
	  mStunList.pop_front();
	}

	doStun(peer.remote);

	{
          RsStackMutex stack(sortMtx);   /********** LOCK MUTEX *********/
	  if (peer.failCount < TOU_STUN_MAX_FAIL_COUNT)
	  {
	  	peer.failCount++;
		peer.lastsend = now;
		mStunList.push_back(peer);

#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::checkStunKeepAlive() pushing Stun peer to back of list";
		std::cerr << std::endl;
#endif

	  }
	  else
	  {
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::checkStunKeepAlive() Discarding bad stun peer";
		std::cerr << std::endl;
#endif
	  }

#ifdef DEBUG_UDP_SORTER
	  locked_printStunList();
#endif

	}


	return true;
}


bool    UdpSorter::locked_recvdStun(const struct sockaddr_in &remote, const struct sockaddr_in &extaddr)
{
#ifdef DEBUG_UDP_SORTER
	std::ostringstream out;
	out << "UdpSorter::locked_recvdStun() from:";
	out << inet_ntoa(remote.sin_addr) << ":" << ntohs(remote.sin_port);
	out << " claiming ExtAddr is:";
	out << inet_ntoa(extaddr.sin_addr) << ":" << ntohs(extaddr.sin_port);

	std::cerr << out.str() << std::endl;
#endif

	bool found = true;
	std::list<TouStunPeer>::iterator it;
	for(it = mStunList.begin(); it != mStunList.end(); it++)
	{
		if ((remote.sin_addr.s_addr == it->remote.sin_addr.s_addr) &&
		    (remote.sin_port == it->remote.sin_port))
		{
			it->failCount = 0;
			it->eaddr = extaddr;
			it->response = true;

			found = true;
			break;
		}
	}

#ifdef DEBUG_UDP_SORTER
	locked_printStunList();
#endif

	if (!eaddrKnown)
	{
		locked_checkExternalAddress();
	}

	return found;
}

bool    UdpSorter::locked_checkExternalAddress()
{
#ifdef DEBUG_UDP_SORTER
	std::ostringstream out;
	out << "UdpSorter::locked_checkExternalAddress()";
	std::cerr << out.str() << std::endl;
#endif

	bool found1 = false;
	bool found2 = false;

	std::list<TouStunPeer>::iterator it;
	std::list<TouStunPeer>::iterator p1;
	std::list<TouStunPeer>::iterator p2;
	for(it = mStunList.begin(); it != mStunList.end(); it++)
	{
		if (it->response && isExternalNet(&(it->eaddr.sin_addr)))
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
		if ((p1->eaddr.sin_addr.s_addr == p2->eaddr.sin_addr.s_addr) &&
		    (p1->eaddr.sin_port == p2->eaddr.sin_port))
		{
			eaddrStable = true;
		}
		else
		{
			eaddrStable = false;
		}
		eaddrKnown = true;
		eaddr = p1->eaddr;

#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::locked_checkExternalAddress() Found State:";
		if (eaddrStable)
			std::cerr << " Stable NAT translation (GOOD!) ";
		else
			std::cerr << " unStable (symmetric NAT translation (BAD!) ";
		std::cerr << std::endl;
#endif

		return true;
	}

	return false;
}



bool    UdpSorter::locked_printStunList()
{
#ifdef DEBUG_UDP_SORTER
	std::ostringstream out;

	time_t now = time(NULL);
	out << "locked_printStunList()" << std::endl;
	out << "\tLastSend: " << now - mStunLastSend << std::endl;
	out << "\tLastRecv: " << now - mStunLastRecv << std::endl;

	std::list<TouStunPeer>::iterator it;
	for(it = mStunList.begin(); it != mStunList.end(); it++)
	{
		out << "id:" << RsUtil::BinToHex(it->id) << " addr: " << inet_ntoa(it->remote.sin_addr);
		out << ":" << htons(it->remote.sin_port);
		out << " eaddr: " << inet_ntoa(it->eaddr.sin_addr);
		out << ":" << htons(it->eaddr.sin_port);
		out << " failCount: " << it->failCount;
		out << " lastSend: " << now - it->lastsend;
		out << std::endl;
	}

	std::cerr << out.str();
#endif

	return true;
}


	

