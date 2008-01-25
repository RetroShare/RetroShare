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

#include <iostream>
#include <sstream>
#include <iomanip>

static const int STUN_TTL = 64;

/*
 * #define DEBUG_UDP_SORTER 1
 */

#define DEBUG_UDP_SORTER 1

UdpSorter::UdpSorter(struct sockaddr_in &local)
	: udpLayer(NULL), laddr(local)
{
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

	/* look for a peer */
        std::map<struct sockaddr_in, UdpPeer *>::iterator it;
	it = streams.find(from);

	/* check for STUN packet */
	if (isStunPacket(data, size))
	{
		std::cerr << "UdpSorter::recvPkt() is Stun Packet";
		std::cerr << std::endl;

		/* respond */
		handleStunPkt(data, size, from);
	}
	else if (it == streams.end())
	{
		/* peer unknown */
		std::cerr << "UdpSorter::recvPkt() Peer Unknown!";
		std::cerr << std::endl;
	}
	else
	{
		/* forward to them */
		std::cerr << "UdpSorter::recvPkt() Sending to UdpPeer: ";
		std::cerr << it->first;
		std::cerr << std::endl;
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


/******************************* STUN Handling ********************************/

		/* respond */
bool UdpSorter::handleStunPkt(void *data, int size, struct sockaddr_in &from)
{
	if (size == 20) /* request */
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::handleStunPkt() got Request";
		std::cerr << std::endl;
#endif

		/* generate a response */
		int len;
		void *pkt = generate_stun_reply(&from, &len);
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
		bool good = response(data, size, eAddr);
		if (good)
		{
#ifdef DEBUG_UDP_SORTER
			std::cerr << "UdpSorter::handleStunPkt() got Ext Addr: ";
			std::cerr << inet_ntoa(eAddr.sin_addr) << ":" << ntohs(eAddr.sin_port);
			std::cerr << std::endl;
#endif
			eaddrKnown = true;
			eaddr = eAddr;
			return true;
		}
	}

#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::handleStunPkt() Bad Packet";
	std::cerr << std::endl;
#endif
	return false;
}


bool    UdpSorter::addStunPeer(const struct sockaddr_in &remote, const char *peerid)
{
	/* add to the list */
#ifdef DEBUG_UDP_SORTER
	std::cerr << "UdpSorter::addStunPeer()";
	std::cerr << std::endl;

	std::cerr << "UdpSorter::addStunPeer() - just stun it!";
	std::cerr << std::endl;
#endif

	doStun(remote);
	return false;
}

bool    UdpSorter::externalAddr(struct sockaddr_in &external)
{
	if (eaddrKnown)
	{
		external = eaddr;
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
	bool done = generate_stun_pkt(stundata, &tmplen);
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

bool    UdpSorter::response(void *stun_pkt, int size, struct sockaddr_in &addr)
{
	/* check what type it is */
	if (size < 28)
	{
		return false;
	}

	if (((uint16_t *) stun_pkt)[0] != 0x0101)
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

bool UdpSorter::generate_stun_pkt(void *stun_pkt, int *len)
{
	if (*len < 20)
	{
		return false;
	}

	/* just the header */
	((uint16_t *) stun_pkt)[0] = 0x0001;
	((uint16_t *) stun_pkt)[1] = 20; /* only header */
	/* transaction id - should be random */
	((uint32_t *) stun_pkt)[1] = 0x0020; 
	((uint32_t *) stun_pkt)[2] = 0x0121; 
	((uint32_t *) stun_pkt)[3] = 0x0111; 
	((uint32_t *) stun_pkt)[4] = 0x1010; 
	*len = 20;
	return true;
}


void *UdpSorter::generate_stun_reply(struct sockaddr_in *stun_addr, int *len)
{
	/* just the header */
	void *stun_pkt = malloc(28);
	((uint16_t *) stun_pkt)[0] = 0x0101;
	((uint16_t *) stun_pkt)[1] = 28; /* only header + 8 byte addr */
	/* transaction id - should be random */
	((uint32_t *) stun_pkt)[1] = 0x0020; 
	((uint32_t *) stun_pkt)[2] = 0x0121; 
	((uint32_t *) stun_pkt)[3] = 0x0111; 
	((uint32_t *) stun_pkt)[4] = 0x1010; 
	/* now add address
	 *  0  1    2  3
	 * <INET>  <port>
	 * <inet address>
	 */

	((uint32_t *) stun_pkt)[6] =  stun_addr->sin_addr.s_addr;
	((uint16_t *) stun_pkt)[11] = stun_addr->sin_port;

	*len = 28;
	return stun_pkt;
}

bool UdpSorter::isStunPacket(void *data, int size)
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
	uint16_t pktsize = ((uint16_t *) data)[1];
	if (size != pktsize)
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::isStunPacket() (size != pktsize) -> false";
		std::cerr << std::endl;
#endif
		return false;
	}

	if ((size == 20) && (0x0001 == ((uint16_t *) data)[0]))
	{
#ifdef DEBUG_UDP_SORTER
		std::cerr << "UdpSorter::isStunPacket() (size=20 & data[0]=0x0001) -> true";
		std::cerr << std::endl;
#endif
		/* request */
		return true;
	}

	if ((size == 28) && (0x0101 == ((uint16_t *) data)[0]))
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

