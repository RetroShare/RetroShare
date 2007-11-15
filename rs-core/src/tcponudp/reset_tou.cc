/*
 * "$Id: reset_tou.cc,v 1.4 2007-02-18 21:46:50 rmf24 Exp $"
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




/**********************************************************
 * This test is designed to ensure that tou networking
 * can be reset, and restarted.
 *
 */

#include <iostream>

// for printing sockaddr
#include "udplayer.h"

#include "tou.h"
#include "tou_net.h"

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

int  setup_socket(struct sockaddr_in addr);
int  connect_socket_pair(int fd1, int fd2, 
		struct sockaddr_in addr1, struct sockaddr_in addr2);
int  send_data_via_pair(int sockfd1, int sockfd2, char *data, int size);

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
	int i,j;

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

	tounet_init();


	/* setup the local/remote addresses.
	 */
	struct sockaddr_in laddr;
	struct sockaddr_in raddr;

	laddr.sin_family = AF_INET;
	raddr.sin_family = AF_INET;

	if ((!tounet_inet_aton(argv[optind], &(laddr.sin_addr))) ||
		(!tounet_inet_aton(argv[optind+2], &(raddr.sin_addr))))
	{
		std::cerr << "Invalid addresses!" << std::endl;
		usage(argv[0]);
	}

	unsigned short laddr_port = atoi(argv[optind+1]);
	unsigned short raddr_port = atoi(argv[optind+3]);

	for(i = 0; i < 10; i++)
	{
		laddr.sin_port = htons(laddr_port);
		raddr.sin_port = htons(raddr_port);
		//laddr.sin_port = htons(laddr_port + i);
		//raddr.sin_port = htons(raddr_port + i);

		std::cerr << "Interation: " << i << std::endl;


		/* setup the sockets */
		int sockfd1 = setup_socket(laddr);
		int sockfd2 = setup_socket(raddr);

		if ((sockfd1 < 0) || (sockfd2 < 0))
		{
			std::cerr << "Failed to setup sockets!";
			std::cerr << std::endl;
			return -1;
		}

		std::cerr << "Local Address: " << laddr;
		std::cerr << " fd: " << sockfd1 << std::endl;
		std::cerr << "Remote Address: " << raddr;
		std::cerr << " fd: " << sockfd2 << std::endl;
	
		/* connect */
		int  err = connect_socket_pair(sockfd1, sockfd2, laddr, raddr);
		if (err < 0)
		{
			std::cerr << "Failed to connect sockets!";
			std::cerr << std::endl;
			return -1;
		}
	
		/* send the data */
		int size = 102400;
		char rnddata[size];

		int data_loops = (i+1) * (i+1);
		for(int k = 0; k < data_loops; k++)
		{
		  std::cerr << "Send Iteration: " << k+1 << " of " << data_loops << std::endl;
		  for(j = 0; j < size; j++)
		  {
			rnddata[j] = (unsigned char) (255.0 * 
				rand() / (RAND_MAX + 1.0));
		  }
		  send_data_via_pair(sockfd1, sockfd2, rnddata, size);
		  std::cerr << "Send Iteration: " << k+1 << " of " << data_loops << std::endl;
		  sleep(2);
		}
	
		std::cerr << "Completed Successful transfer of " << size * data_loops << " bytes";
		std::cerr << std::endl;

		sleep(10);
		std::cerr << "closing sockfd1: " << sockfd1 << std::endl;
		tou_close(sockfd1);
	
		std::cerr << "closing sockfd2: " << sockfd2 << std::endl;
		tou_close(sockfd2);


	}
	return 1;
}

int  setup_socket(struct sockaddr_in addr)
{
	int sockfd = tou_socket(PF_INET, SOCK_STREAM, 0);

	if (sockfd <= 0)
	{
		std::cerr << "Failed to open socket!: ";
		std::cerr << "Socket Error:" << tou_errno(sockfd) << std::endl;
		return -1;
	}

	std::cerr << "Socket Created: " << sockfd << std::endl;

        int err = tou_bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));

	if (err < 0) 
	{
		std::cerr << "Error: Cannot bind socket: ";
		std::cerr << err << std::endl;
		return -1;
	}

	std::cerr << "Socket1 Bound to: " << addr << std::endl;
	return sockfd;
}

int  connect_socket_pair(int fd1, int fd2, 
		struct sockaddr_in addr1, struct sockaddr_in addr2)
{
	std::cerr << "Socket2 Listening " << std::endl;
	/* listen */
	int err = tou_listenfor(fd2, (struct sockaddr *) &addr1, sizeof(addr1));
	int err_num;

	if (err < 0)
	{
		err_num = tou_errno(fd2);
		if (err_num != EINPROGRESS)
		{
			std::cerr << "Cannot Listen!: " << err_num << std::endl;
			return -1;
		}
	}


	std::cerr << "Socket1 Connecting to: " << addr2 << std::endl;
	err = tou_connect(fd1, (struct sockaddr *) &addr2, sizeof(addr2));
	if (err < 0)
	{
		err_num = tou_errno(fd1);
		if (err_num != EINPROGRESS)
		{
			std::cerr << "Cannot Connect!: " << err_num << std::endl;
			return -1;
		}
	}

	bool sock1Connected = false;
	bool sock2Connected = false;

	while((!sock1Connected) || (!sock2Connected))
	{
		sleep(1);

		/* sock1 */
		if((!sock1Connected) && (0 == (err = tou_connected(fd1))))
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

		if ((!sock2Connected) && (0 > (nsock = tou_accept(fd2, 
				(struct sockaddr *) &inaddr, &addrlen))))
		{
			errno = tou_errno(fd2);
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
			fd2 = nsock;
			std::cerr << "Socket Accepted from: " << inaddr << std::endl;
		}
	}

	std::cerr << "Socket Connected" << std::endl;

	return 1;
}


/* This transmits into sockfd1, and check to see that we recv
 * it back from sockfd2
 */

int  send_data_via_pair(int sockfd1, int sockfd2, char *data, int size)
{
	/* what we recvd */
	char *recvd = (char *) malloc(size * 2);
	int   recvdsize = 0;
	int   sent = 0;
	int   sendsize = 0;

	int   ts_start = time(NULL);

	int   minsends = 100; /* min of 100 sends to complete all data */
	/* ensure we don't end up sending nothing */
	if (minsends * 10 > size)
	{
		minsends = size / 10; 
	}

	bool doneWrite = false;
	bool doneRead  = false;
	while((!doneWrite) || (!doneRead))
	{
		/* have a little break */
		//usleep(10000); /* 0.01 sec */
		//usleep(250000); /* 0.25 sec */
		usleep(500000); /* 0.50 sec */
		/* decide how much to send */
		sendsize = (int) (((float) (size / minsends)) * 
				(rand() / (RAND_MAX + 1.0)));

		/* limit send */
		if (sent + sendsize > size)
		{
			sendsize = size - sent;
		}
		/* if we've finished */
		if (sent == size)
		{
			/* eof */
			std::cerr << "Write Done!" << std::endl;
			doneWrite = true;
			sendsize = 0;
		}

		/* now we write */
		if ((sendsize > 0)&&(-1==tou_write(sockfd1,&(data[sent]),sendsize)))
		{
			std::cerr << "Write Error: " << tou_errno(sockfd1) << std::endl;
			if (tou_errno(sockfd1) != EAGAIN)
			{
				std::cerr << "FATAL ERROR ending transfer" << std::endl;
				doneRead = true;
				doneWrite = true;
			}
				
		}
		else
		{
			sent += sendsize;
		}

		int ret = 0;
		int readsize = (int) (((float) (size / minsends)) * 
				(rand() / (RAND_MAX + 1.0)));

		if (readsize > size - recvdsize)
			readsize = size - recvdsize;

		if (0 < (ret = tou_read(sockfd2, &(recvd[recvdsize]), readsize)))
		{
			std::cerr << "TF(" << ret << ")" << std::endl;
			recvdsize += ret;
		}
		else if (ret == 0)
		{
			doneRead = true;
			std::cerr << "Read Done! (ret:0)" << std::endl;
		}
		else
		{
			std::cerr << "Read Error: " << tou_errno(sockfd2) << std::endl;
			std::cerr << "Read " << recvdsize << "/" << size;
			std::cerr << " attempted: " << readsize << std::endl;
			if (tou_errno(sockfd2) != EAGAIN)
			{
				std::cerr << "FATAL ERROR ending transfer" << std::endl;
				doneRead = true;
				doneWrite = true;
			}

		}

		if (recvdsize == size)
		{
			doneRead = true;
			std::cerr << "Read Done!" << std::endl;
		}

	}

	/* we have transmitted it all, so 
	 * check the data 
	 */

	int i;
	int diffCount = 0;
	for(i = 0; i < size; i++)
	{
		if (recvd[i] != data[i])
		{
			diffCount++;
			if (diffCount < 10)
			{
				std::cerr << "Error Byte:" << i << " is different";
				std::cerr << std::endl;
			}
		}
	}
	if (diffCount)
	{
		std::cerr << "Errors (" << diffCount << "/" << size << ") in tranmission ... Exiting!";
		std::cerr << std::endl;
		exit(1);
	}

	int   ts_end = time(NULL);
	double rough_rate = size / (double) (ts_end - ts_start);

	std::cerr << "Successful Data Tranmission: " << size << " in " << ts_end-ts_start << " secs";
	std::cerr << std::endl;
	std::cerr << "Approximate Rate: " << rough_rate / 1000.0 << " kbytes/sec";
	std::cerr << std::endl;
		
	return 1;
}








