/*
 * "$Id: tou.cc,v 1.7 2007-02-18 21:46:50 rmf24 Exp $"
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




#include "tou.h"

static  const int kInitStreamTable = 5;

#include <stdlib.h>
#include <string.h>

#include "udp/udpstack.h"
#include "tcpstream.h"
#include <vector>
#include <iostream>

#include <errno.h>

#define DEBUG_TOU_INTERFACE 1

struct TcpOnUdp_t
{
	int tou_fd;
	int lasterrno;
	TcpStream *tcp;
	UdpSubReceiver *udpsr;
	int udptype;
	bool idle;
};

typedef struct TcpOnUdp_t TcpOnUdp;

static  std::vector<TcpOnUdp *> tou_streams;

static  int tou_inited = 0;


#include "tcponudp/udppeer.h"
#include "tcponudp/udprelay.h"

static  UdpSubReceiver *udpSR[MAX_TOU_RECEIVERS] = {NULL};
static  uint32_t	udpType[MAX_TOU_RECEIVERS] = { 0 };
static  uint32_t        noUdpSR = 0;

static int	tou_tick_all();

/* 	tou_init 
 *
 * 	Modified to accept a number of UdpSubRecievers!
 * 	these can be linked to arbitary UdpStacks. 
 *      (removed all UdpStack references here!)
 *
 *      Unfortunately, the UdpSubReceivers have different initialisation for starting a connection.
 *      So the TOU interface has to accomodate this.
 *      
 */
/* 	tou_init - opens the udp port (universal bind) */
int 	tou_init(void **in_udpsubrecvs, int *type, int number)
{
	UdpSubReceiver **usrArray = (UdpSubReceiver **) in_udpsubrecvs;
	if (number > MAX_TOU_RECEIVERS)
	{
		std::cerr << "tou_init() Invalid number of receivers";
		std::cerr << std::endl;
		return 0;
	}

	if (tou_inited)
	{
		return 1;
	}

	noUdpSR = number;
	uint32_t i;
	for(i = 0; i < noUdpSR; i++)
	{
		udpSR[i] = usrArray[i];
		udpType[i] = type[i];
	}

	tou_streams.resize(kInitStreamTable);

	tou_inited = 1;
	return 1;
}


/* 	open - allocates a sockfd, and checks that the type is okay */
int     tou_socket(uint32_t recvIdx, uint32_t type, int /*protocol*/)
{
	if (!tou_inited)
	{
		return -1;
	}

	if (recvIdx >= noUdpSR)
	{
		std::cerr << "tou_socket() ERROR recvIdx greater than #receivers";
		std::cerr << std::endl;
		return -1;
	}

	/* check that the index matches the type */
	UdpSubReceiver *recver = udpSR[recvIdx];
	uint32_t recverType = udpType[recvIdx];

	if (recverType != type)
	{
		std::cerr << "tou_socket() ERROR type doesn't match expected type";
		std::cerr << std::endl;
		return -1;
	}

	for(unsigned int i = 1; i < tou_streams.size(); i++)
	{
		if (tou_streams[i] == NULL)
		{
			tou_streams[i] = new TcpOnUdp();
			tou_streams[i] -> tou_fd = i;
			tou_streams[i] -> tcp = NULL;
			tou_streams[i] -> udpsr = recver;
			tou_streams[i] -> udptype = recverType;
			return i;
		}
	}

	TcpOnUdp *tou = new TcpOnUdp();

	tou_streams.push_back(tou);

	if (tou == tou_streams[tou_streams.size() -1])
	{
		tou -> tou_fd = tou_streams.size() -1;
		tou -> tcp = NULL;
		tou -> udpsr = recver;
		tou -> udptype = recverType;
		return tou->tou_fd;
	}

	tou -> lasterrno = EUSERS;

	return -1;
}

/* 	bind - opens the udp port */
int 	tou_bind(int sockfd, const struct sockaddr * /* my_addr */, socklen_t /* addrlen */ )
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];

	/* this now always returns an error! */
	tous -> lasterrno = EADDRINUSE;
	return -1;
}

/* 	records peers address, and sends syn pkt 
 *  	the timeout is very slow initially - to give 
 *  	the peer a chance to startup 
 *
 *  	- like a tcp/ip connection, the connect
 *  	will return -1 EAGAIN, until connection complete.
 *  	- always non blocking.
 */
int 	tou_connect(int sockfd, const struct sockaddr *serv_addr, 
					socklen_t addrlen, uint32_t conn_period)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	

	if (addrlen != sizeof(struct sockaddr_in))
	{
		tous -> lasterrno = EINVAL;
		return -1;
	}

	/* enforce that the udptype is correct */
	if (tous -> udptype != TOU_RECEIVER_TYPE_UDPPEER)
	{
		std::cerr << "tou_connect() ERROR connect method invalid for udptype";
		std::cerr << std::endl;
		tous -> lasterrno = EINVAL;
		return -1;
	}

	
#ifdef TOU_DYNAMIC_CAST_CHECK 
	/* extra checking -> for testing purposes (dynamic cast) */
	UdpPeerReceiver *upr = dynamic_cast<UdpPeerReceiver *>(tous->udpsr);
	if (!upr)
	{
		std::cerr << "tou_connect() ERROR cannot convert type to UdpPeerReceiver";
		std::cerr << std::endl;
		tous -> lasterrno = EINVAL;
		return -1;
	}
#else
	UdpPeerReceiver *upr = (UdpPeerReceiver *) (tous->udpsr);
#endif

	/* create a TCP stream to connect with. */
	if (!tous->tcp)
	{
		tous->tcp = new TcpStream(tous->udpsr);
		upr->addUdpPeer(tous->tcp, 
			*((const struct sockaddr_in *) serv_addr));
	}

	tous->tcp->connect(*(const struct sockaddr_in *) serv_addr, conn_period);
	tous->tcp->tick();
	tou_tick_all();
	if (tous->tcp->isConnected())
	{
		return 0;	
	}

	tous -> lasterrno = EINPROGRESS;
	return -1;
}

/* is this ever used? should it be depreciated? */
int 	tou_listenfor(int sockfd, const struct sockaddr *serv_addr, 
					socklen_t addrlen)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	
	if (addrlen != sizeof(struct sockaddr_in))
	{
		tous -> lasterrno = EINVAL;
		return -1;
	}

	/* enforce that the udptype is correct */
	if (tous -> udptype != TOU_RECEIVER_TYPE_UDPPEER)
	{
		std::cerr << "tou_connect() ERROR connect method invalid for udptype";
		std::cerr << std::endl;
		tous -> lasterrno = EINVAL;
		return -1;
	}

#ifdef TOU_DYNAMIC_CAST_CHECK 
	/* extra checking -> for testing purposes (dynamic cast) */
	UdpPeerReceiver *upr = dynamic_cast<UdpPeerReceiver *>(tous->udpsr);
	if (!upr)
	{
		std::cerr << "tou_connect() ERROR cannot convert type to UdpPeerReceiver";
		std::cerr << std::endl;
		tous -> lasterrno = EINVAL;
		return -1;
	}
#else
	UdpPeerReceiver *upr = (UdpPeerReceiver *) (tous->udpsr);
#endif

	/* create a TCP stream to connect with. */
	if (!tous->tcp)
	{
		tous->tcp = new TcpStream(tous->udpsr);
		upr->addUdpPeer(tous->tcp, 
			*((const struct sockaddr_in *) serv_addr));
	}

	tous->tcp->listenfor(*((struct sockaddr_in *) serv_addr));
	tous->tcp->tick();
	tou_tick_all();

	return 0;
}

int     tou_listen(int /* sockfd */ , int /* backlog */ )
{
	tou_tick_all();
	return 1;
}

/*
 *	This is the alternative RELAY connection.
 *
 *	User needs to provide 3 ip addresses.
 *      These addresses should have been provided by the RELAY negogiation
 *	a) own ip:port
 *	b) proxy ip:port
 *	c) dest ip:port
 *
 *	The reset of the startup is similar to other TOU connections.
 *	As this is likely to be run over an established UDP connection, 
 *	there is little need for a big connection period.
 *
 *  	- like a tcp/ip connection, the connect
 *  	will return -1 EAGAIN, until connection complete.
 *  	- always non blocking.
 */
#define DEFAULT_RELAY_CONN_PERIOD		1

int 	tou_connect_via_relay(int sockfd, 
			const struct sockaddr_in *own_addr, 
			const struct sockaddr_in *proxy_addr, 
			const struct sockaddr_in *dest_addr)

{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	
	/* enforce that the udptype is correct */
	if (tous -> udptype != TOU_RECEIVER_TYPE_UDPRELAY)
	{
		std::cerr << "tou_connect() ERROR connect method invalid for udptype";
		std::cerr << std::endl;
		tous -> lasterrno = EINVAL;
		return -1;
	}

#ifdef TOU_DYNAMIC_CAST_CHECK 
	/* extra checking -> for testing purposes (dynamic cast) */
	UdpRelayReceiver *urr = dynamic_cast<UdpRelayReceiver *>(tous->udpsr);
	if (!urr)
	{
		std::cerr << "tou_connect() ERROR cannot convert type to UdpRelayReceiver";
		std::cerr << std::endl;
		tous -> lasterrno = EINVAL;
		return -1;
	}
#else
	UdpRelayReceiver *urr = (UdpRelayReceiver *) (tous->udpsr);
#endif

	/* create a TCP stream to connect with. */
	if (!tous->tcp)
	{
		tous->tcp = new TcpStream(tous->udpsr);

		UdpRelayAddrSet addrSet(own_addr, dest_addr);
		urr->addUdpPeer(tous->tcp, &addrSet, proxy_addr);
	}

	/* We Point it at the Destination Address.
	 * The UdpRelayReceiver wraps and re-directs the packets to the proxy
	 */
	tous->tcp->connect(*dest_addr, DEFAULT_RELAY_CONN_PERIOD);
	tous->tcp->tick();
	tou_tick_all();
	if (tous->tcp->isConnected())
	{
		return 0;	
	}

	tous -> lasterrno = EINPROGRESS;
	return -1;
}


        /* slightly different - returns sockfd on connection */
int     tou_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	
	if (*addrlen != sizeof(struct sockaddr_in))
	{
		tous -> lasterrno = EINVAL;
		return -1;
	}

	//tous->tcp->connect();
	tous->tcp->tick();
	tou_tick_all();
	if (tous->tcp->isConnected())
	{
		// should get remote address
		tous->tcp->getRemoteAddress(*((struct sockaddr_in *) addr));
		return sockfd;	
	}

	tous -> lasterrno = EAGAIN;
	return -1;
}


int     tou_connected(int sockfd)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];

	tous->tcp->tick();
	tou_tick_all();

	return (tous->tcp->TcpState() == 4);
}


/* 	standard  stream read/write  non-blocking of course
 */

ssize_t tou_read(int sockfd, void *buf, size_t count)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];

	tous->tcp->tick();
	tou_tick_all();

	int err = tous->tcp->read((char *) buf, count);
	if (err < 0)
	{
		tous->lasterrno = tous->tcp->TcpErrorState();
		return -1;
	}
	return err;
}

ssize_t tou_write(int sockfd, const void *buf, size_t count)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];


	int err = tous->tcp->write((char *) buf, count);
	if (err < 0)
	{
		tous->lasterrno = tous->tcp->TcpErrorState();
		tous->tcp->tick();
		tou_tick_all();
		return -1;
	}
	tous->tcp->tick();
	tou_tick_all();
	return err;
}

        /* check stream */
int     tou_maxread(int sockfd)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	tous->tcp->tick();
	tou_tick_all();

	int ret = tous->tcp->read_pending();
	if (ret < 0)
	{
		tous->lasterrno = tous->tcp->TcpErrorState();
		return 0; // error detected next time.
	}
	return ret;
}

int     tou_maxwrite(int sockfd)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	tous->tcp->tick();
	tou_tick_all();

	int ret = tous->tcp->write_allowed();
	if (ret < 0)
	{
		tous->lasterrno = tous->tcp->TcpErrorState();
		return 0; // error detected next time?
	}
	return ret;
}


/*	close down the tcp over udp connection */
int 	tou_close(int sockfd)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];

	tou_tick_all();

	if (tous->tcp)
	{
		tous->tcp->tick();

		/* shut it down */
		tous->tcp->close();

		/* now we need to work out which type of receiver we have */
#ifdef TOU_DYNAMIC_CAST_CHECK 
		/* extra checking -> for testing purposes (dynamic cast) */
		UdpRelayReceiver *urr = dynamic_cast<UdpRelayReceiver *>(tous->udpsr);
		UdpPeerReceiver *upr = dynamic_cast<UdpPeerReceiver *>(tous->udpsr);
		if (urr)
		{
			urr->removeUdpPeer(tous->tcp);
		}
		else if (upr)
		{
			upr->removeUdpPeer(tous->tcp);
		}
		else
		{
			/* error */
			std::cerr << "tou_close() ERROR unknown udptype";
			std::cerr << std::endl;
			tous -> lasterrno = EINVAL;
		}
#else
		if (tous -> udptype == TOU_RECEIVER_TYPE_UDPRELAY)
		{
			UdpRelayReceiver *urr = (UdpRelayReceiver *) (tous->udpsr);
			urr->removeUdpPeer(tous->tcp);
		}
		else if (tous -> udptype == TOU_RECEIVER_TYPE_UDPPEER)
		{
			UdpPeerReceiver *upr = (UdpPeerReceiver *) (tous->udpsr);
			upr->removeUdpPeer(tous->tcp);
		}
		else
		{
			/* error */
			std::cerr << "tou_close() ERROR unknown udptype";
			std::cerr << std::endl;
			tous -> lasterrno = EINVAL;
		}
		
#endif

		delete tous->tcp;

	}

	delete tous;
	tou_streams[sockfd] = NULL;
	return 1;
}

/*	get an error number */
int	tou_errno(int sockfd)
{
	if (tou_streams[sockfd] == NULL)
	{
		return ENOTSOCK;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	return tous->lasterrno;
}

int     tou_clear_error(int sockfd)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	tous->lasterrno = 0;
	return 0;
}

/*	unfortuately the library needs to be ticked. (not running a thread)
 *	you can put it in a thread!
 */

/*
 * Some helper functions for stuff.
 *
 */

static int	tou_passall();
static int	tou_active_rw();

static int nextActiveCycle;
static int nextIdleCheck;
static const int kActiveCycleStep = 1;
static const int kIdleCheckStep = 5;

static int	tou_tick_all()
{
	tou_passall();
	return 1;

	/* check timer */
	int ts = time(NULL);
	if (ts > nextActiveCycle)
	{
		tou_active_rw();
		nextActiveCycle += kActiveCycleStep;
	}
	if (ts > nextIdleCheck)
	{
		tou_passall();
		nextIdleCheck += kIdleCheckStep;
	}
	return 0;
}


static int	tou_passall()
{
	/* iterate through all and clean up old sockets.
	 * check if idle are still idle.
	 */
	std::vector<TcpOnUdp *>::iterator it;
	for(it = tou_streams.begin(); it != tou_streams.end(); it++)
	{
		if ((*it) && ((*it)->tcp))
		{
			(*it)->tcp->tick();
		}
	}
	return 1;
}

static int	tou_active_rw()
{
	/* iterate through actives and tick
	 */
	return 1;
}


