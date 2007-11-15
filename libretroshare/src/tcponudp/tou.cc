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

#include "udplayer.h"
#include "tcpstream.h"
#include <vector>
#include <iostream>

#include <errno.h>

#define DEBUG_TOU_INTERFACE 1

struct TcpOnUdp_t
{
	int tou_fd;
	int lasterrno;
	UdpLayer  *udp;
	TcpStream *tcp;
	bool know_eaddr;
	struct sockaddr_in extaddr;
	bool idle;
};

typedef struct TcpOnUdp_t TcpOnUdp;

static  std::vector<TcpOnUdp *> tou_streams;

static  int tou_inited = 0;

static int	tou_tick_all();
static int	tou_init()
{
	if (tou_inited)
		return 1;

	tou_streams.resize(kInitStreamTable);

	tou_inited = 1;
	return 1;
}


/* 	open - which does nothing */
int     tou_socket(int /*domain*/, int /*type*/, int /*protocol*/)
{
	tou_init();
	for(unsigned int i = 1; i < tou_streams.size(); i++)
	{
		if (tou_streams[i] == NULL)
		{
			tou_streams[i] = new TcpOnUdp();
			tou_streams[i] -> tou_fd = i;
			tou_streams[i] -> know_eaddr = false;
			tou_streams[i] -> udp = NULL;
			tou_streams[i] -> tcp = NULL;
			return i;
		}
	}

	TcpOnUdp *tou = new TcpOnUdp();

	tou_streams.push_back(tou);

	if (tou == tou_streams[tou_streams.size() -1])
	{
		tou -> tou_fd = tou_streams.size() -1;
		tou -> know_eaddr = false;
		tou -> udp = NULL;
		tou -> tcp = NULL;
		return tou->tou_fd;
	}

	tou -> lasterrno = EUSERS;

	return -1;

#ifdef DEBUG_TOU_INTERFACE
	std::cerr << "tou_socket() FAILED" << std::endl;
	exit(1);
#endif 
}





/* 	bind - opens the udp port */
int 	tou_bind(int sockfd, const struct sockaddr *my_addr, 
					socklen_t addrlen)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];

	if (tous->udp)
	{
		tous -> lasterrno = EADDRINUSE;
		return -1;
	}

	if (tous->tcp)
	{
		tous -> lasterrno = EADDRINUSE;
		return -1;
	}

	if (addrlen != sizeof(struct sockaddr_in))
	{
		tous -> lasterrno = EADDRNOTAVAIL;
		return -1;
	}

	/* 
	 * tous->udp = new UdpLayer( *((struct sockaddr_in *) my_addr));
	 */
	// for testing - drop 5% of packets... */
	//tous->udp = new LossyUdpLayer( *((struct sockaddr_in *) my_addr), 0.05);
	tous->udp = new UdpLayer( *((struct sockaddr_in *) my_addr));
	/* check the bind succeeded */
	if (!(tous->udp->okay()))
	{
		delete (tous->udp);
		tous->udp = NULL;
		tous -> lasterrno = EADDRINUSE;
		return -1;
	}

	tous->tcp = new TcpStream(tous->udp);

	tous->tcp->tick();
	tou_tick_all();

	return 0;
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

	if (tous->tcp->state == 0)
	{
		tous->udp->setRemoteAddr(*((struct sockaddr_in *) serv_addr));
	}

	tous->tcp->connect();
	tous->tcp->tick();
	tou_tick_all();
	if (tous->tcp->isConnected())
	{
		return 0;	
	}

	tous -> lasterrno = EINPROGRESS;
	return -1;
}

int     tou_listen(int sockfd, int backlog)
{
	tou_tick_all();
	return 1;
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
		tous->udp->getRemoteAddr(*((struct sockaddr_in *) addr));
		return sockfd;	
	}

	tous -> lasterrno = EAGAIN;
	return -1;
}

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

	if (tous->tcp->state == 0)
	{
		tous->udp->setRemoteAddr(*((struct sockaddr_in *) serv_addr));
	}

	tous->tcp->tick();
	tou_tick_all();

	return 0;
}

/* 	This is a udp socket - after all
 *  	independent operation from all of the 
 *  	stream stuff.
 */
int     tou_extudp(const struct sockaddr *ext, socklen_t tolen)
{
	/* request a udp to listen on, in a leachy kinda way */
	std::vector<TcpOnUdp *>::iterator it;
	for(it = tou_streams.begin(); it != tou_streams.end(); it++)
	{
		if ((*it) && ((*it)->udp))
		{
			if ((*it)->know_eaddr)
			{
				*((struct sockaddr_in *) ext) = (*it)->extaddr;
				return (*it)->tou_fd;
			}
		}
	}
	return -1;
}

int     tou_extaddr(int sockfd, const struct sockaddr *ext, socklen_t tolen)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];
	tous -> know_eaddr = true;
	tous -> extaddr = *((struct sockaddr_in *) ext);
	return 0;
}


ssize_t tou_recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
	if (tou_streams[sockfd] == NULL)
	{
		std::cerr << "tou_recvfrom() Invalid sockfd:" << sockfd;
		std::cerr << std::endl;
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];

	if (*fromlen != sizeof(struct sockaddr_in))
	{
		std::cerr << "tou_recvfrom() Invalid addr size";
		std::cerr << std::endl;
		tous -> lasterrno = EINVAL;
		return -1;
	}

	/* extra checks */
	if ((!tous->tcp) || (!tous->udp))
	{
		std::cerr << "tou_recvfrom() Bad sockfd (!udp) || (!tcp) :" << sockfd;
		std::cerr << std::endl;
		return -1;
	}

	if (tous->tcp->state == 0)
	{
		//std::cerr << "tou_recvfrom() State = 0 .... Allow??";
		//std::cerr << std::endl;
	}


	if (tous->udp->okay())
	{
		int ret = tous->udp->recvRndPktfrom(buf,len,flags, from, fromlen);
		if (ret < 0)
		{
		//std::cerr << "tou_recvfrom() Sock Ok, Try Again later";
		//std::cerr << std::endl;
			tous -> lasterrno = EAGAIN;
			return -1;
		}
		std::cerr << "tou_recvfrom() Got a Packet on: " << sockfd;
		std::cerr << std::endl;
		tous -> lasterrno = 0;
		return ret;
	}
		std::cerr << "tou_recvfrom() Socket Not Okay:" << sockfd;
		std::cerr << std::endl;
	
	tous -> lasterrno = EAGAIN;
	return -1;
}


ssize_t tou_sendto(int sockfd, const void *buf, size_t len, int flags, const struct
	              sockaddr *to, socklen_t tolen)
{
	if (tou_streams[sockfd] == NULL)
	{
		return -1;
	}
	TcpOnUdp *tous = tou_streams[sockfd];

	if (tolen != sizeof(struct sockaddr_in))
	{
		tous -> lasterrno = EINVAL;
		return -1;
	}

	if (tous->tcp->state == 0)
	{
	}


	if (tous->udp->okay())
	{
        	tous->udp->sendToProxy(*((struct sockaddr_in *) to), 
					buf, len);
		return len;
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

	return (tous->tcp->state == 4);
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
		tous->lasterrno = tous->tcp->errorState;
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
		tous->lasterrno = tous->tcp->errorState;
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
		tous->lasterrno = tous->tcp->errorState;
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
		tous->lasterrno = tous->tcp->errorState;
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

	tous->tcp->tick();
	tou_tick_all();

	/* shut it down */
	tous->tcp->close();
	tous->udp->close();
	delete tous->tcp;
	delete tous->udp;
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


