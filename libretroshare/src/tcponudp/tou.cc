/*******************************************************************************
 * libretroshare/src/tcponudp: tou.cc                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include "tou.h"

static  const int kInitStreamTable = 5;

#include <stdlib.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <errno.h>

#include "util/rstime.h"
#include "udp/udpstack.h"
#include "pqi/pqinetwork.h"
#include "tcpstream.h"
#include "util/stacktrace.h"

#define DEBUG_TOU_INTERFACE 1
#define EUSERS          87

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

static RsMutex touMutex("touMutex"); 
// Mutex is used to control addition / removals from tou_streams.
// Lookup should be okay - as long as you stick to your allocated ID!

static std::vector<TcpOnUdp *> tou_streams;

static  int tou_inited = 0;


#include "tcponudp/udppeer.h"
#include "tcponudp/udprelay.h"

static  UdpSubReceiver *udpSR[MAX_TOU_RECEIVERS] = {NULL};
static  uint32_t	udpType[MAX_TOU_RECEIVERS] = { 0 };
static  uint32_t        noUdpSR = 0;

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
	RsStackMutex stack(touMutex); /***** LOCKED ******/

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
	RsStackMutex stack(touMutex); /***** LOCKED ******/

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


bool tou_stream_check(int sockfd)
{
	if(sockfd < 0)
	{
		std::cerr << __PRETTY_FUNCTION__ << " ERROR sockfd: " << sockfd
		          << " < 0" << std::endl;
		print_stacktrace();
		return false;
	}

	if(sockfd >= tou_streams.size())
	{
		std::cerr << __PRETTY_FUNCTION__ << " ERROR sockfd: " << sockfd
		          << " out of bound!" << std::endl;
		print_stacktrace();
		return false;
	}

	if(!tou_streams[sockfd])
	{
		std::cerr << __PRETTY_FUNCTION__ << " ERROR tou_streams[sockfd] == NULL"
		          << std::endl;
		print_stacktrace();
		return false;
	}

	return true;
}

/* 	bind - opens the udp port */
int 	tou_bind(int sockfd, const struct sockaddr * /* my_addr */, socklen_t /* addrlen */ )
{
	if (!tou_stream_check(sockfd))
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
	if (!tou_stream_check(sockfd))
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	

	if (addrlen < sizeof(struct sockaddr_in))
	{
		std::cerr << "tou_connect() ERROR invalid size of sockaddr";
		std::cerr << std::endl;
		tous -> lasterrno = EINVAL;
		return -1;
	}

	// only IPv4 for the moment.
	const struct sockaddr_storage *ss_addr = (struct sockaddr_storage *) serv_addr;
	if (ss_addr->ss_family != AF_INET)
	{
		std::cerr << "tou_connect() ERROR not ipv4";
		std::cerr << std::endl;
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
	if (!tou_stream_check(sockfd))
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	
	if (addrlen < sizeof(struct sockaddr_in))
	{
		std::cerr << "tou_listenfor() ERROR invalid size of sockaddr";
		std::cerr << std::endl;
		tous -> lasterrno = EINVAL;
		return -1;
	}

	// only IPv4 for the moment.
	const struct sockaddr_storage *ss_addr = (struct sockaddr_storage *) serv_addr;
	if (ss_addr->ss_family != AF_INET)
	{
		std::cerr << "tou_listenfor() ERROR not ipv4";
		std::cerr << std::endl;
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

	return 0;
}

int     tou_listen(int /* sockfd */ , int /* backlog */ )
{
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

int tou_connect_via_relay( int sockfd, const sockaddr_in& own_addr,
                           const sockaddr_in& proxy_addr,
                           const sockaddr_in& dest_addr )

{
	std::cerr << __PRETTY_FUNCTION__ << std::endl;

	if (!tou_stream_check(sockfd)) return -EINVAL;

	TcpOnUdp& tous = *tou_streams[sockfd];
	
	/* enforce that the udptype is correct */
	if (tous.udptype != TOU_RECEIVER_TYPE_UDPRELAY)
	{
		std::cerr << __PRETTY_FUNCTION__ << " ERROR connect method invalid for "
		          << "udptype" << std::endl;
		tous.lasterrno = EINVAL;
		return -EINVAL;
	}

	UdpRelayReceiver* urr_ptr = dynamic_cast<UdpRelayReceiver*>(tous.udpsr);
	if(!urr_ptr)
	{
		std::cerr << __PRETTY_FUNCTION__ << " ERROR cannot convert to "
		          << "UdpRelayReceiver" << std::endl;
		tous.lasterrno = EINVAL;
		return -EINVAL;
	}

	UdpRelayReceiver& urr = *urr_ptr; urr_ptr = nullptr;

	/* create a TCP stream to connect with. */
	if (!tous.tcp)
	{
		tous.tcp = new TcpStream(tous.udpsr);

		UdpRelayAddrSet addrSet(&own_addr, &dest_addr);
		urr.addUdpPeer(tous.tcp, &addrSet, &proxy_addr);
	}

	/* We Point it at the Destination Address.
	 * The UdpRelayReceiver wraps and re-directs the packets to the proxy
	 */
	tous.tcp->connect(dest_addr, DEFAULT_RELAY_CONN_PERIOD);
	tous.tcp->tick();
	if (tous.tcp->isConnected()) return 0;

	tous.lasterrno = EINPROGRESS;
	return -EINPROGRESS;
}


        /* slightly different - returns sockfd on connection */
int     tou_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	if (!tou_stream_check(sockfd))
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	
	if (*addrlen < sizeof(struct sockaddr_in))
	{
		std::cerr << "tou_accept() ERROR invalid size of sockaddr";
		std::cerr << std::endl;
		tous -> lasterrno = EINVAL;
		return -1;
	}

	// only IPv4 for the moment.
	const struct sockaddr_storage *ss_addr = (struct sockaddr_storage *) addr;
	if (ss_addr->ss_family != AF_INET)
	{
		std::cerr << "tou_accept() ERROR not ipv4";
		std::cerr << std::endl;
		tous -> lasterrno = EINVAL;
		return -1;
	}

	//tous->tcp->connect();
	tous->tcp->tick();
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
	if (!tou_stream_check(sockfd))
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];

	tous->tcp->tick();

	return (tous->tcp->TcpState() == 4);
}


/* 	standard  stream read/write  non-blocking of course
 */

ssize_t tou_read(int sockfd, void *buf, size_t count)
{
	if (!tou_stream_check(sockfd))
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];

	tous->tcp->tick();

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
	if (!tou_stream_check(sockfd))
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];


	int err = tous->tcp->write((char *) buf, count);
	if (err < 0)
	{
		tous->lasterrno = tous->tcp->TcpErrorState();
		tous->tcp->tick();
		return -1;
	}
	tous->tcp->tick();
	return err;
}

        /* check stream */
int     tou_maxread(int sockfd)
{
	if (!tou_stream_check(sockfd))
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	tous->tcp->tick();

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
	if (!tou_stream_check(sockfd))
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	tous->tcp->tick();

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
	TcpOnUdp *tous = NULL;
	{
		RsStackMutex stack(touMutex); /***** LOCKED ******/
		if (!tou_stream_check(sockfd))
		{
			return -1;
		}
		tous = tou_streams[sockfd];
		tou_streams[sockfd] = NULL;
	}

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
        tous->tcp = NULL ;	// prevents calling

	}

	delete tous;
	return 1;
}

/*	get an error number */
int	tou_errno(int sockfd)
{
	if (!tou_stream_check(sockfd))
	{
		return ENOTSOCK;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	return tous->lasterrno;
}

int     tou_clear_error(int sockfd)
{
	if (!tou_stream_check(sockfd))
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	tous->lasterrno = 0;
	return 0;
}

