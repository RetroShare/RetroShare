/*
 * "$Id: udplayer.cc,v 1.8 2007-02-18 21:46:50 rmf24 Exp $"
 *
 * TCP-on-UDP (tou) network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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




#include "udplayer.h"

#include <iostream>
#include <sstream>
#include <iomanip>

/*
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
*/

/*
 * #define DEBUG_UDP_LAYER 1
 */

static const int UDP_DEF_TTL = 64;

class   udpPacket
{
	public:
	udpPacket(struct sockaddr_in *addr, void *dta, int dlen)
	:raddr(*addr), len(dlen)
	{
		data = malloc(len);
		memcpy(data, dta, len);
	}

	~udpPacket()
	{
		if (data)
		{
			free(data);
			data = NULL;
			len = 0;
		}
	}

	struct sockaddr_in raddr;
	void *data;
	int len;
};


std::ostream &operator<<(std::ostream &out,  struct sockaddr_in &addr)
{
	out << "[" << inet_ntoa(addr.sin_addr) << ":";
	out << htons(addr.sin_port) << "]";
	return out;
}


bool operator==(struct sockaddr_in &addr, struct sockaddr_in &addr2)
{
	if (addr.sin_family != addr2.sin_family)
		return false;
	if (addr.sin_addr.s_addr != addr2.sin_addr.s_addr)
		return false;
	if (addr.sin_port != addr2.sin_port)
		return false;
	return true;
}

std::string printPkt(void *d, int size)
{
	std::ostringstream out;
	out << "Packet:" << "**********************";
	for(int i = 0; i < size; i++)
	{
		if (i % 16 == 0)
			out << std::endl;
		out << std::hex << std::setw(2) << (unsigned int) ((unsigned char *) d)[i] << " ";
	}
	out << std::endl << "**********************";
	out << std::endl;
	return out.str();
}


std::string printPktOffset(unsigned int offset, void *d, unsigned int size)
{
	std::ostringstream out;
	out << "Packet:" << "**********************";
	out << std::endl;
	out << "Offset: " << std::hex << offset << " -> " << offset + size;
	out << std::endl;
	out << "Packet:" << "**********************";

	unsigned int j = offset % 16;
	if (j != 0)
	{
	  out << std::endl;
	  out << std::hex << std::setw(6) << (unsigned int) offset - j;
	  out << ": ";
	  for(unsigned int i = 0; i < j; i++)
	  {
		out << "xx ";
	  }
	}
	for(unsigned int i = offset; i < offset + size; i++)
	{
		if (i % 16 == 0)
		{
			out << std::endl;
			out << std::hex << std::setw(6) << (unsigned int) i;
			out << ": ";
		}
		out << std::hex << std::setw(2) << (unsigned int) ((unsigned char *) d)[i-offset] << " ";
	}
	out << std::endl << "**********************";
	out << std::endl;
	return out.str();
}



UdpLayer::UdpLayer(struct sockaddr_in &local)
	:laddr(local), raddrKnown(false), errorState(0), ttl(UDP_DEF_TTL)
{
	openSocket();
	return;
}

int     UdpLayer::status(std::ostream &out)
{
	out << "UdpLayer::status()" << std::endl;
	out << "localaddr: " << laddr << std::endl;
	if (raddrKnown)
	{
		out << "remoteaddr: " << raddr << std::endl;
	}
	else
	{
		out << "remoteaddr unKnown!" << std::endl;
	}
	out << "sockfd: " << sockfd << std::endl;
	out << std::endl;
	return 1;
}

int UdpLayer::close()
{
	/* close socket if open */
	if (sockfd > 0)
	{
       		tounet_close(sockfd);
	}
	return 1;
}


/* higher level interface */
int UdpLayer::readPkt(void *data, int *size)
{
	int nsize = *size;
	struct sockaddr_in from;
	if (0 >= receiveUdpPacket(data, &nsize, from))
	{
#ifdef DEBUG_UDP_LAYER
		//std::cerr << "UdpLayer::readPkt() not ready" << from;
		//std::cerr << std::endl;
#endif
		return -1;
	}

#ifdef DEBUG_UDP_LAYER
	//std::cerr << "UdpLayer::readPkt()  from : " << from << std::endl;
	//std::cerr << printPkt(data, nsize);
#endif

	if ((raddrKnown) && (from == raddr))
	{
#ifdef DEBUG_UDP_LAYER
		std::cerr << "UdpLayer::readPkt() from RemoteAddr: " << from;
		std::cerr << std::endl;
#endif
		*size = nsize;
		return nsize;
	}

#ifdef DEBUG_UDP_LAYER
	std::cerr << "UdpLayer::readPkt() from unknown remote addr: " << from;
	std::cerr << std::endl;
#endif
	std::cerr << "UdpLayer::readPkt() storing Random packet from: " << from;
	std::cerr << std::endl;
	randomPkts.push_back(new udpPacket(&from,data, nsize));
	return -1;
}

int UdpLayer::sendPkt(void *data, int size)
{
	if (raddrKnown)
	{
#ifdef DEBUG_UDP_LAYER
		std::cerr << "UdpLayer::sendPkt()  to: " << raddr << std::endl;
		//std::cerr << printPkt(data, size);
#endif
		sendUdpPacket(data, size, raddr);
		return size;
	}
	else
	{
#ifdef DEBUG_UDP_LAYER
		std::cerr << "UdpLayer::sendPacket() unknown remote addr!";
		std::cerr << std::endl;
#endif
		return -1;
	}
	return 1;
}


/* setup connections */
int UdpLayer::openSocket()	
{
	/* make a socket */
       	sockfd = tounet_socket(PF_INET, SOCK_DGRAM, 0);
#ifdef DEBUG_UDP_LAYER
	std::cerr << "UpdStreamer::openSocket()" << std::endl;
#endif
	/* bind to address */
	if (0 != tounet_bind(sockfd, (struct sockaddr *) (&laddr), sizeof(laddr)))
	{
#ifdef DEBUG_UDP_LAYER
		std::cerr << "Socket Failed to Bind to : " << laddr << std::endl;
		std::cerr << "Error: " << tounet_errno() << std::endl;
#endif
		errorState = EADDRINUSE;
		//exit(1);
		return -1;
	}

	if (-1 == tounet_fcntl(sockfd, F_SETFL, O_NONBLOCK))
	{
#ifdef DEBUG_UDP_LAYER
		std::cerr << "Failed to Make Non-Blocking" << std::endl;
#endif
	}

#ifdef DEBUG_UDP_LAYER
	std::cerr << "Socket Bound to : " << laddr << std::endl;
#endif
#ifdef DEBUG_UDP_LAYER
	std::cerr << "Setting TTL to " << UDP_DEF_TTL << std::endl;
#endif
	setTTL(UDP_DEF_TTL);

	errorState = 0;
	return 1;

}

int UdpLayer::setTTL(int t)
{
	int err = tounet_setsockopt(sockfd, IPPROTO_IP, IP_TTL, &t, sizeof(int));
	ttl = t;

#ifdef DEBUG_UDP_LAYER
	std::cerr << "UdpLayer::setTTL(" << t << ") returned: " << err;
	std::cerr << std::endl;
#endif

	return err;
}

int UdpLayer::getTTL()
{
	return ttl;
}



int UdpLayer::sendToProxy(struct sockaddr_in &proxy, const void *data, int size)
{
	sendUdpPacket(data, size, proxy);
	return 1; 
}

int  UdpLayer::setRemoteAddr(struct sockaddr_in &remote)
{
	raddr = remote;
	raddrKnown = true;
	return 1;
}
	

int  UdpLayer::getRemoteAddr(struct sockaddr_in &remote)
{
	if (raddrKnown)
	{
		remote = raddr;
		return 1;
	}
	return 0;
}
	
/* monitoring / updates */
int UdpLayer::okay()
{
	bool nonFatalError = ((errorState == 0) ||
				(errorState == EAGAIN) ||
				(errorState == EINPROGRESS));

#ifdef DEBUG_UDP_LAYER
	if (!nonFatalError)
	{
		std::cerr << "UdpLayer::NOT okay(): Error: " << errorState << std::endl;
	}

#endif

	return nonFatalError;
}

int UdpLayer::tick()
{
#ifdef DEBUG_UDP_LAYER
	std::cerr << "UdpLayer::tick()" << std::endl;
#endif
	return 1;
}
/******************* Internals *************************************/


ssize_t UdpLayer::recvRndPktfrom(void *buf, size_t len, int flags, 
		                struct sockaddr *from, socklen_t *fromlen)
{
#ifdef DEBUG_UDP_LAYER
	std::cerr << "UdpLayer::recvRndPktfrom()" << std::endl;
#endif

	if (*fromlen != sizeof(struct sockaddr_in))
	{

#ifdef DEBUG_UDP_LAYER
	std::cerr << "UdpLayer::recvRndPktfrom() bad address length" << std::endl;
#endif
		return -1;
	}

	/* if raddr not known -> then we're not connected
	 * at a higher level and therefore our queue
	 * will not be filled (no ticking)....
	 * so feel free the get data.
	 */

	if (randomPkts.size() == 0)
	{
		if (!raddrKnown)
		{
#ifdef DEBUG_UDP_LAYER
	std::cerr << "UdpLayer::recvRndPktfrom() Checking Directly" << std::endl;
#endif
			int size = len;
			int ret = receiveUdpPacket(buf, &size, *((struct sockaddr_in *) from));
			if (ret > 0)
			{
#ifdef DEBUG_UDP_LAYER
	std::cerr << "UdpLayer::recvRndPktfrom() Got Pkt directly" << std::endl;
	std::cerr << "Pkt from:" << inet_ntoa(((struct sockaddr_in *) from)->sin_addr);
	std::cerr << ":" << ntohs(((struct sockaddr_in *) from)->sin_port) << std::endl;
#endif
				return ret;
			}
		}

#ifdef DEBUG_UDP_LAYER
	std::cerr << "UdpLayer::recvRndPktfrom() Nothing in the Queue" << std::endl;
#endif
		return -1;
	}

	udpPacket *pkt = randomPkts.front();
	randomPkts.pop_front();

	*((struct sockaddr_in *) from) = pkt->raddr;
	unsigned int size = pkt->len;
	if (len < size)
	{
		size = len;
	}

	memcpy(buf, pkt->data, size);
	*((struct sockaddr_in *) from) = pkt->raddr;

#ifdef DEBUG_UDP_LAYER
	std::cerr << "UdpLayer::recvRndPktfrom() returning stored Pkt" << std::endl;
	std::cerr << "Pkt from:" << inet_ntoa(pkt->raddr.sin_addr);
	std::cerr << ":" << ntohs(pkt->raddr.sin_port) << std::endl;
	std::cerr << "Length: " << pkt->len << std::endl;
#endif

	delete pkt;
	return size;
}

/******************* Internals *************************************/


int UdpLayer::receiveUdpPacket(void *data, int *size, struct sockaddr_in &from)
{
	struct sockaddr_in fromaddr;
	socklen_t fromsize = sizeof(fromaddr);
	int insize = *size;
	if (0<(insize=tounet_recvfrom(sockfd,data,insize,0,
			(struct sockaddr*)&fromaddr,&fromsize)))
	{
#ifdef DEBUG_UDP_LAYER
		std::cerr << "receiveUdpPacket() from: " << fromaddr;
		std::cerr << " Size: " << insize;
		std::cerr << std::endl;
#endif
		*size = insize;
		from = fromaddr;
		return insize;
	}
	return -1;
}

int UdpLayer::sendUdpPacket(const void *data, int size, struct sockaddr_in &to)
{
	/* send out */
#ifdef DEBUG_UDP_LAYER
	std::cerr << "UdpLayer::sendUdpPacket(): size: " << size;
	std::cerr << " To: " << to << std::endl;
#endif
	struct sockaddr_in toaddr = to;

	tounet_sendto(sockfd, data, size, 0, 
			   (struct sockaddr *) &(toaddr), 
				sizeof(toaddr));
	return 1;
}




