/*
 * "$Id: pair_tou.cc,v 1.3 2007-02-18 21:46:50 rmf24 Exp $"
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




#include <iostream>
#include <stdlib.h>

//#define USE_TCP_SOCKET

// for printing sockaddr
#include "udp/udpstack.h"

#ifndef USE_TCP_SOCKET
	#include "tcponudp/tou.h"
#endif

#include "util/bdnet.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


/* This is a simple test to ensure that the tou behaviour
 * is almost identical to a standard tcp socket.
 *
 * In this version we open 2 sockets, and attempt to 
 * communicate with ourselves....
 *
 */

int 	Check_Socket(int fd);

void usage(char *name)
{
	std::cerr << "Usage: " << name; 
	std::cerr << " [-pco] <laddr> <lport> ";
	std::cerr << " <raddr> <rport> ";
	std::cerr << std::endl;
	exit(1);
	return;	
}

int main(int argc, char **argv)
{
	int c;
	bool isProxy = false;
	bool toConnect = false;
	bool stayOpen = false;

	int totalwbytes = 0;
	int totalrbytes = 0;

	while(-1 != (c = getopt(argc, argv, "pco")))
	{
		switch (c)
		{
			case 'p':
				isProxy = true;
				break;
			case 'c':
				toConnect = true;
				break;
			case 'o':
				stayOpen = true;
				break;
			default:
				usage(argv[0]);
				break;
		}
	}
				
	if (argc-optind < 4)
	{
		usage(argv[0]);
		return 1;	
	}

	/* setup the local/remote addresses.
	 */
	struct sockaddr_in laddr;
	struct sockaddr_in raddr;

	laddr.sin_family = AF_INET;
	raddr.sin_family = AF_INET;

	if ((!bdnet_inet_aton(argv[optind], &(laddr.sin_addr))) ||
		(!bdnet_inet_aton(argv[optind+2], &(raddr.sin_addr))))
	{
		std::cerr << "Invalid addresses!" << std::endl;
		usage(argv[0]);
	}

	laddr.sin_port = htons(atoi(argv[optind+1]));
	raddr.sin_port = htons(atoi(argv[optind+3]));

	std::cerr << "Local Address: " << laddr << std::endl;
	std::cerr << "Remote Address: " << raddr << std::endl;

        UdpStack udps(laddr);
        tou_init((void *) &udps);


#ifdef USE_TCP_SOCKET
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	int sockfd2 = socket(PF_INET, SOCK_STREAM, 0);
#else
	int sockfd = tou_socket(PF_INET, SOCK_STREAM, 0);
	int sockfd2 = tou_socket(PF_INET, SOCK_STREAM, 0);
#endif
	if ((sockfd <= 0) || (sockfd2 <= 0))
	{
		std::cerr << "Failed to open socket!: ";
#ifdef USE_TCP_SOCKET
		std::cerr << "Socket Error:" << errno << std::endl;
#else
		std::cerr << "Socket Error:" << tou_errno(sockfd) << std::endl;
		std::cerr << "Socket Error:" << tou_errno(sockfd2) << std::endl;
#endif
		return -1;
	}
	std::cerr << "Sockets Created: " << sockfd << " & " << sockfd2 << std::endl;


	/* make nonblocking */
#ifdef USE_TCP_SOCKET
	int err = fcntl(sockfd,F_SETFD,O_NONBLOCK);
	int err2 = fcntl(sockfd2,F_SETFD,O_NONBLOCK);
#else
	int err = 0;
	int err2 = 0;
#endif
	if ((err < 0) || (err2 < 0))
	{
		std::cerr << "Error: Cannot make socket NON-Blocking: ";
		std::cerr << err << std::endl;
		return -1;
	}

	std::cerr << "Socket Non-Blocking" << std::endl;

#ifdef USE_TCP_SOCKET
        err = bind(sockfd, (struct sockaddr *) &laddr, sizeof(laddr));
        err2 = bind(sockfd2, (struct sockaddr *) &raddr, sizeof(raddr));
#else
        err = tou_bind(sockfd, (struct sockaddr *) &laddr, sizeof(laddr));
        err2 = tou_bind(sockfd2, (struct sockaddr *) &raddr, sizeof(raddr));
#endif
	if ((err < 0) || (err2 < 0))
	{
		std::cerr << "Error: Cannot bind socket: ";
		std::cerr << err << std::endl;
		return -1;
	}

	std::cerr << "Socket1 Bound to: " << laddr << std::endl;
	std::cerr << "Socket2 Bound to: " << raddr << std::endl;

	// listening.
	if (1) // socket2.	
	{
		std::cerr << "Socket2 Listening " << std::endl;
		/* listen */
#ifdef USE_TCP_SOCKET
		err = listen(sockfd2, 1);
#else
		err = tou_listenfor(sockfd2, 
				(struct sockaddr *) &laddr, sizeof(laddr));
#endif
	}


	if (1) // only one program.
	{
		std::cerr << "Socket1 Connecting to: " << raddr << std::endl;
#ifdef USE_TCP_SOCKET
		err = connect(sockfd, (struct sockaddr *) &raddr, sizeof(raddr));
#else
		err = tou_connect(sockfd, (struct sockaddr *) &raddr, sizeof(raddr), 30);
#endif
		if (err < 0)
		{
#ifndef USE_TCP_SOCKET
			errno = tou_errno(sockfd);
#endif
			if (errno != EINPROGRESS)
			{
				std::cerr << "Cannot Connect!: " << errno << std::endl;
				return 1;
			}
		}
	}

	bool sock1Connected = false;
	bool sock2Connected = false;

	while((!sock1Connected) || (!sock2Connected))
	{
		sleep(1);


		/* sock1 */
#ifdef USE_TCP_SOCKET
		if((!sock1Connected) && (0 == (err = Check_Socket(sockfd))))
#else
		if((!sock1Connected) && (0 == (err = tou_connected(sockfd))))
#endif
		{
			std::cerr << "Waiting for Connect (Sock1)!" << std::endl;
		}
		if ((!sock1Connected) && (err < 0))
		{
			std::cerr << "Connect Failed" << std::endl;
			return 1;
		}
		else if (!sock1Connected)
		{
			// else connected!
			sock1Connected = true;
		}

		/* accept - sock2 */
		struct sockaddr_in inaddr;
		socklen_t addrlen = sizeof(inaddr);
		int nsock = -1;

#ifdef USE_TCP_SOCKET
		if ((!sock2Connected) && (0 > (nsock = accept(sockfd2, 
				(struct sockaddr *) &inaddr, &addrlen))))
#else
		if ((!sock2Connected) && (0 > (nsock = tou_accept(sockfd2, 
				(struct sockaddr *) &inaddr, &addrlen))))
#endif
		{
#ifndef USE_TCP_SOCKET
			errno = tou_errno(sockfd2);
#endif
			if (errno != EAGAIN)
			{
				std::cerr << "Cannot Connect!: " << errno << std::endl;
				return 1;
			}
			else
			{
				std::cerr << "Waiting for Connect (Sock2)!" << std::endl;
			}
		}
		else if (nsock > 0)
		{
			/* connected */
			sock2Connected = true;
			sockfd2 = nsock;
			std::cerr << "Socket Accepted from: " << inaddr << std::endl;
		}
	}

	std::cerr << "Socket Connected" << std::endl;

	/* send data */
	int bufsize = 1511;
	char buffer[bufsize];
	char data[bufsize];
	int readsize = 0;

	bdnet_fcntl(0, F_SETFL, O_NONBLOCK);
	bdnet_fcntl(1,F_SETFL,O_NONBLOCK);

	bool doneWrite = false;
	bool doneRead  = false;
	bool blockread = false;
	while((!doneWrite) || (!doneRead))
	{
		/* read -> write_socket... */
		sleep(1);
		if (blockread != true)
		{
			readsize = read(0, buffer, bufsize);
		}
		if (readsize == 0)
		{
			/* eof */
			doneWrite = true;
		}
		/* now we write */
#ifdef USE_TCP_SOCKET
		if ((readsize > 0) && (-1 == write(sockfd, buffer, readsize)))
#else
		if ((readsize > 0) && (-1 == tou_write(sockfd, buffer, readsize)))
#endif
		{
			//std::cerr << "Blocked Write!" << std::endl;
#ifndef USE_TCP_SOCKET
			//std::cerr << "Error: " << tou_errno(sockfd) << std::endl;
#endif
			blockread = true;
		}
		else
		{
			blockread = false;
			totalwbytes += readsize;
		}

		int ret = 0;
#ifdef USE_TCP_SOCKET
		if (0 < (ret = read(sockfd2, data, bufsize)))
#else
		if (0 < (ret = tou_read(sockfd2, data, bufsize)))
#endif

		{
			std::cerr << "TF(" << ret << ")" << std::endl;
			write(1, data, ret);
			totalrbytes += ret;
		}
		else if (ret == 0)
		{
			doneRead = true;
		}
		else
		{
				//std::cerr << "Blocked Read!" << std::endl;
#ifndef USE_TCP_SOCKET
				//std::cerr << "Error: " << tou_errno(sockfd) << std::endl;
#endif
		}


	}

#ifdef USE_TCP_SOCKET
	close(sockfd);
	close(sockfd2);
#else
	/* this is blocking??? */
	tou_close(sockfd);
	tou_close(sockfd2);
#endif

	std::cerr << "Transfer Complete: " << totalwbytes << " bytes";
	std::cerr << std::endl;
	return 1;
}

#ifdef USE_TCP_SOCKET
	

int 	Check_Socket(int fd)
{
	std::cerr << "Check_Socket()" << std::endl;

	std::cerr << "1) Checking with Select()" << std::endl;

	fd_set ReadFDs, WriteFDs, ExceptFDs;
	FD_ZERO(&ReadFDs);
	FD_ZERO(&WriteFDs);
	FD_ZERO(&ExceptFDs);

	FD_SET(fd, &ReadFDs);
	FD_SET(fd, &WriteFDs);
	FD_SET(fd, &ExceptFDs);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int sr = 0;
	if (0 > (sr = select(fd + 1, 
			&ReadFDs, &WriteFDs, &ExceptFDs, &timeout))) 
	{
		std::cerr << "Check_Socket() Select ERROR: " << sr << std::endl;
		return -1;
	}

	if (FD_ISSET(fd, &ExceptFDs))
	{
		std::cerr << "Check_Socket() Exception on socket!" << std::endl;
		return -1;
	}

	if (FD_ISSET(fd, &WriteFDs))
	{
		std::cerr << "Check_Socket() Can Write!" << std::endl;
	}
	else
	{
		// not ready return 0;
		std::cerr << "Check_Socket() Cannot Write!" << std::endl;
		std::cerr << "Check_Socket() Socket Not Ready!" << std::endl;
		return 0;
	}

	if (FD_ISSET(fd, &ReadFDs))
	{
		std::cerr << "Check_Socket() Can Read!" << std::endl;
	}
	else
	{
		std::cerr << "Check_Socket() Cannot Read!" << std::endl;
		std::cerr << "Check_Socket() Socket Not Ready!" << std::endl;
		return 0;
	}

	std::cerr << "Select() Tests indicate Socket Good!" << std::endl;
	std::cerr << "2) Checking with getsockopt()" << std::endl;

	int err = 1;
	socklen_t optlen = 4;
	if (0==getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &optlen))
	{
		std::cerr << "Check_Socket() getsockopt returned :" << err;
		std::cerr << ", optlen:" << optlen;
		std::cerr << std::endl;

		if (err == 0)
		{
		  std::cerr << "Check_Socket() getsockopt";
		  std::cerr  << " Indicates TCP Connection Complete:";
		  std::cerr << std::endl;
		  return 1;
		}
		else if (err == EINPROGRESS)
		{
		  std::cerr << "Check_Socket() getsockopt";
		  std::cerr  << " Indicates TCP Connection INPROGRESS";
		  std::cerr << std::endl;
		  return 0;
		}
		else if ((err == ENETUNREACH) || (err == ETIMEDOUT))
		{
		  std::cerr << "Check_Socket() getsockopt";
		  std::cerr  << " Indicates TCP Connection ENETUNREACH/ETIMEDOUT";
		  std::cerr << std::endl;
		  return -1;
		}
		else if ((err == EHOSTUNREACH) || (err == EHOSTDOWN))
		{
		  std::cerr << "Check_Socket() getsockopt";
		  std::cerr  << " Indicates TCP Connection ENETUNREACH/ETIMEDOUT";
		  std::cerr << std::endl;
		  return -1;
		}
		else
		{
		  std::cerr << "Check_Socket() getsockopt";
		  std::cerr  << " Indicates Other Error: " << err;
		  std::cerr << std::endl;
		  return -1;
		}
			

	}
	else
	{
	  std::cerr << "Check_Socket() getsockopt";
	  std::cerr  << " FAILED ";
	  std::cerr << std::endl;
	  return -1;
	}
}


#else

int 	Check_Socket(int fd)
{
	return 0;
}

#endif
