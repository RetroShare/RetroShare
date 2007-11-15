/*
 * "$Id: pqistunner.cc,v 1.2 2007-02-18 21:46:50 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
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





#include "pqi/pqistunner.h"
#include "tcponudp/tou.h"

#include "pqi/pqidebug.h"
#include <sstream>

const int pqistunzone = 13754;


pqistunner::pqistunner(struct sockaddr_in addr)
	:sockfd(-1), active(false)  
{
	pqioutput(PQL_ALERT, pqistunzone, 
		"pqistunner::pqistunner()");

	setListenAddr(addr);
	setuplisten();

	stunpkt = malloc(1024); /* 20 = size of a stun request */
	stunpktlen = 1024;
}


int     pqistunner::setListenAddr(struct sockaddr_in addr)
{
	laddr = addr;
	// increment the port by 1. (can't open on same)
	laddr.sin_port = htons(ntohs(laddr.sin_port) + 1);

	return 1;
}

int     pqistunner::resetlisten()
{
	if (sockfd > -1)
	{
		/* close it down */
		std::ostringstream out;
		out << "pqistunner::resetlisten() Closed stun port";
		pqioutput(PQL_ALERT, pqistunzone, out.str());

		tou_close(sockfd);
		sockfd = -1;
	}
	active = false;
	return 1;
}

int     pqistunner::setuplisten()
{
	sockfd = tou_socket(0,0,0);
	if (0 == tou_bind(sockfd, (struct sockaddr *) &laddr, sizeof(laddr)))
	{
		active = true;
		std::ostringstream out;
		out << "pqistunner::setuplisten() Attached to Listen Address: ";
		out << inet_ntoa(laddr.sin_addr) << ":" << ntohs(laddr.sin_port);
		pqioutput(PQL_ALERT, pqistunzone, out.str());
	}
	else
	{
		pqioutput(PQL_ALERT, pqistunzone, "pqistunner::setuplisten Failed!");
	}
	return 1;
}

int     pqistunner::stun(struct sockaddr_in stun_addr)
{
	pqioutput(PQL_DEBUG_BASIC, pqistunzone, "pqistunner::stun()");

	/* send out a stun packet -> save in the local variable */
	if (!active)
	{
		pqioutput(PQL_ALERT, pqistunzone, "pqistunner::stun() Not Active!");
		return 0;
	}

	int tmplen = stunpktlen;
	bool done = generate_stun_pkt(stunpkt, &tmplen);
	if (!done)
	{
		pqioutput(PQL_ALERT, pqistunzone, "pqistunner::stun() Failed!");
		return 0;
	}

	/* increment the port +1 */
	stun_addr.sin_port = htons(ntohs(stun_addr.sin_port) + 1);
	/* and send it off */
 	int sentlen = tou_sendto(sockfd, stunpkt, stunpktlen, 0, 
		(const struct sockaddr *) &stun_addr, sizeof(stun_addr));

	std::ostringstream out;
	out << "pqistunner::stun() Sent Stun Packet(" << sentlen << ") from:";
	out << inet_ntoa(laddr.sin_addr) << ":" << ntohs(laddr.sin_port);
	out << " to:";
	out << inet_ntoa(stun_addr.sin_addr) << ":" << ntohs(stun_addr.sin_port);
	pqioutput(PQL_ALERT, pqistunzone, out.str());

	return 1;
}



bool    pqistunner::response(void *stun_pkt, int size, struct sockaddr_in &addr)
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


	std::ostringstream out;
	out << "pqistunner::response() Recvd a Stun Response, ext_addr: ";
	out << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
	pqioutput(PQL_ALERT, pqistunzone, out.str());

	return true;

}



/************************** Basic Functionality ******************/
int     pqistunner::recvfrom(void *data, int *size, struct sockaddr_in &addr)
{
	if (!active)
	{
		pqioutput(PQL_ALERT, pqistunzone, "pqistunner::recvfrom() Not Active!");
		return 0;
	}

	/* check the socket */
	socklen_t addrlen = sizeof(addr);

	int rsize = tou_recvfrom(sockfd, data, *size, 0, (struct sockaddr *) &addr, &addrlen);
	if (rsize > 0)
	{
		std::ostringstream out;
		out << "pqistunner::recvfrom() Recvd a Pkt on: ";
		out << inet_ntoa(laddr.sin_addr) << ":" << ntohs(laddr.sin_port);
		out << " from: ";
		out << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
		pqioutput(PQL_ALERT, pqistunzone, out.str());

		*size = rsize;
		return 1;
	}
	return 0;
}

int     pqistunner::reply(void *data, int size, struct sockaddr_in &addr)
{
	/* so we design a new packet with the external address in it */
	int pktlen = 0;
	void *pkt = generate_stun_reply(&addr, &pktlen);

	/* and send it off */
 	int sentlen = tou_sendto(sockfd, pkt, pktlen, 0, 
			(const struct sockaddr *) &addr, sizeof(addr));

	{
		std::ostringstream out;
		out << "pqistunner::reply() Replying from: ";
		out << inet_ntoa(laddr.sin_addr) << ":" << ntohs(laddr.sin_port);
		out << " to: ";
		out << inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
		pqioutput(PQL_ALERT, pqistunzone, out.str());
	}

	free(pkt);

	/* display status */

	return sentlen;
}

bool pqistunner::generate_stun_pkt(void *stun_pkt, int *len)
{
	if (*len < 20)
	{
		return false;
	}

	/* just the header */
	((uint16_t *) stun_pkt)[0] = 0x0001;
	((uint16_t *) stun_pkt)[1] = 0x0020; /* only header */
	/* transaction id - should be random */
	((uint32_t *) stun_pkt)[1] = 0x0020; 
	((uint32_t *) stun_pkt)[2] = 0x0121; 
	((uint32_t *) stun_pkt)[3] = 0x0111; 
	((uint32_t *) stun_pkt)[4] = 0x1010; 
	*len = 20;
	return true;
}


void *pqistunner::generate_stun_reply(struct sockaddr_in *stun_addr, int *len)
{
	/* just the header */
	void *stun_pkt = malloc(28);
	((uint16_t *) stun_pkt)[0] = 0x0101;
	((uint16_t *) stun_pkt)[1] = 0x0028; /* only header + 8 byte addr */
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


