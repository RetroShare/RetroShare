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

// for printing sockaddr
#include "udp/udpstack.h"

#include "tcponudp/tou.h"
#include "tcponudp/udppeer.h"
#include "tcponudp/udprelay.h"

#include "util/bdnet.h"

#include "util/rsrandom.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>


/* 
 * This test creates the same stacks as are used in RS.
 * This additionally tests the "checkFns" for the other UdpReceivers, which could potentially be modifying the data.
 * obviously they shouldn't!
 *
 */

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
	int totalmbytes = 0;
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

	std::cerr << "Creating with Lossy Udp Layers for further testing";
	std::cerr << std::endl;

        UdpStack *udpStack1 = new UdpStack(UDP_TEST_LOSSY_LAYER,laddr);
        UdpStack *udpStack2 = new UdpStack(UDP_TEST_LOSSY_LAYER,raddr);

        UdpSubReceiver *udpReceivers[4];
        int udpTypes[4];

	// NOTE DHT is too hard to add at the moment.

        UdpRelayReceiver *mRelay1 = new UdpRelayReceiver(udpStack1);
        udpReceivers[1] = mRelay1; 
        udpTypes[1] = TOU_RECEIVER_TYPE_UDPRELAY;
        udpStack1->addReceiver(udpReceivers[1]);

        udpReceivers[0] = new UdpPeerReceiver(udpStack1);
        udpTypes[0] = TOU_RECEIVER_TYPE_UDPPEER;
        udpStack1->addReceiver(udpReceivers[0]);

	// STACK 2.
        UdpRelayReceiver *mRelay2 = new UdpRelayReceiver(udpStack2);
        udpReceivers[3] = mRelay2; 
        udpTypes[3] = TOU_RECEIVER_TYPE_UDPRELAY;
        udpStack2->addReceiver(udpReceivers[3]);

        udpReceivers[2] = new UdpPeerReceiver(udpStack2);
        udpTypes[2] = TOU_RECEIVER_TYPE_UDPPEER;
        udpStack2->addReceiver(udpReceivers[2]);

        tou_init((void **) udpReceivers, udpTypes, 4);


	int sockfd = tou_socket(0, TOU_RECEIVER_TYPE_UDPPEER, 0);
	int sockfd2 = tou_socket(2, TOU_RECEIVER_TYPE_UDPPEER, 0);
	if ((sockfd <= 0) || (sockfd2 <= 0))
	{
		std::cerr << "Failed to open socket!: ";
		std::cerr << "Socket Error:" << tou_errno(sockfd) << std::endl;
		std::cerr << "Socket Error:" << tou_errno(sockfd2) << std::endl;
		return -1;
	}
	std::cerr << "Sockets Created: " << sockfd << " & " << sockfd2 << std::endl;

	std::cerr << "Socket Non-Blocking" << std::endl;

	std::cerr << "Socket1 Bound to: " << laddr << std::endl;
	std::cerr << "Socket2 Bound to: " << raddr << std::endl;

	int err = 0;

	// listening.
	if (1) // socket2.	
	{
		std::cerr << "Socket2 Listening " << std::endl;
		/* listen */
		err = tou_listenfor(sockfd2, 
				(struct sockaddr *) &laddr, sizeof(laddr));
	}


	if (1) // only one program.
	{
		std::cerr << "Socket1 Connecting to: " << raddr << std::endl;
		err = tou_connect(sockfd, (struct sockaddr *) &raddr, sizeof(raddr), 30);
		if (err < 0)
		{
			errno = tou_errno(sockfd);
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
		usleep(1000);


		/* sock1 */
		if((!sock1Connected) && (0 == (err = tou_connected(sockfd))))
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

		if ((!sock2Connected) && (0 > (nsock = tou_accept(sockfd2, 
				(struct sockaddr *) &inaddr, &addrlen))))
		{
			errno = tou_errno(sockfd2);
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

#define MAX_READ 20000

	/* send data */
	int bufsize = MAX_READ;
	char buffer[MAX_READ];
	char middata[MAX_READ];
	char data[MAX_READ];
	int readsize = 0;
	int midsize = 0;

	bdnet_fcntl(0, F_SETFL, O_NONBLOCK);
	bdnet_fcntl(1,F_SETFL,O_NONBLOCK);

	rstime_t lastRead = time(NULL);
	bool doneWrite = false;
	bool doneRead  = false;
	bool blockread = false;
	bool blockmid = false;

	while((!doneWrite) || (!doneRead))
	{
		/* read -> write_socket... */
		usleep(10);

		if (blockread != true)
		{
			int readbuf = 1 + RSRandom::random_u32() % (MAX_READ - 1);
			readsize = read(0, buffer, readbuf);
			if (readsize)
			{
				//std::cerr << "read " << readsize << " / " << readbuf;
				//std::cerr << std::endl;
			}
		}
		if ((readsize == 0) && (!doneWrite))
		{
			/* eof */
			doneWrite = true;
			std::cerr << "Write Done Total: " << totalwbytes;
			std::cerr << std::endl;
		}
		/* now we write */
		if ((readsize > 0) && (-1 == tou_write(sockfd, buffer, readsize)))
		{
			//std::cerr << "Blocked Write ";
			//std::cerr << "Error: " << tou_errno(sockfd);
			//std::cerr << std::endl;
			blockread = true;
		}
		else
		{
			blockread = false;
			totalwbytes += readsize;

			if (readsize)
			{
				std::cerr << "Written: " << readsize << ", Total: " << totalwbytes;
				std::cerr << " Total In Transit: " << totalwbytes - totalrbytes;
				std::cerr << std::endl;
			}
		}

		/*************** Here we read it in, and send it back out... 
		 * so the data is flowing both ways through the UDP connection 
	  	 *
		 *****/


		if (!blockmid)
		{
			/* read from socket 2, and write back to socket 2 */

			int readmidbuf = 1 + RSRandom::random_u32() % (MAX_READ - 1);
			if (0 < (midsize = tou_read(sockfd2, middata, readmidbuf)))
			{
				//std::cerr << "MidRead: " << mid << " Total: " << totalrbytes;
				//std::cerr << " In Transit: " << totalwbytes - totalrbytes;
				//std::cerr << std::endl;
			}
		}

		/* now we write */
		if ((midsize > 0) && (-1 == tou_write(sockfd2, middata, midsize)))
		{
			//std::cerr << "Blocked Write ";
			//std::cerr << "Error: " << tou_errno(sockfd);
			//std::cerr << std::endl;
			blockmid = true;
		}
		else
		{
			blockmid = false;

			if (midsize > 0)
			{
				totalmbytes += midsize;

				std::cerr << "MidRead: " << midsize << ", Total: " << totalmbytes;
				std::cerr << " => Transit => : " << totalwbytes - totalmbytes;
				std::cerr << " <= Transit <= : " << totalmbytes - totalrbytes;
				std::cerr << std::endl;
			}
		}

		/***** READ FROM FIRST SOCKET *****/

		int ret = 0;
		int readbuf2 = 1 + RSRandom::random_u32() % (MAX_READ - 1);
		if (0 < (ret = tou_read(sockfd, data, readbuf2)))
		{
			//std::cerr << "TF(" << ret << ")" << std::endl;
			write(1, data, ret);
			totalrbytes += ret;

			std::cerr << "Read: " << ret << " Total: " << totalrbytes;
			std::cerr << " Total In Transit: " << totalwbytes - totalrbytes;
			std::cerr << std::endl;

			lastRead = time(NULL);
		}
		else if ((ret == 0) && (!doneRead))
		{
			doneRead = true;
			std::cerr << "Read Done Total: " << totalrbytes;
			std::cerr << std::endl;
		}
		else
		{
				//std::cerr << "Blocked Read: " <<  "Error: " << tou_errno(sockfd);
				//std::cerr << std::endl;
#define TIMEOUT 15
			if ((!doneRead) && (time(NULL) - lastRead > TIMEOUT))
			{
				doneRead = true;
				std::cerr << "TIMEOUT: Read Done Total: " << totalrbytes;
				std::cerr << std::endl;
			}
		}


	}

	/* this is blocking??? */
	tou_close(sockfd);
	tou_close(sockfd2);

	std::cerr << "Transfer Complete: " << totalwbytes << " bytes";
	std::cerr << std::endl;
	return 1;
}

