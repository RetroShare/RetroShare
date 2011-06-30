/*
 * "$Id: internal_tou.cc,v 1.2 2007-02-18 21:46:50 rmf24 Exp $"
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
 * There appears (was) to be an elusive bug in the tou internals.
 * most likely to be in the packing and unpacking of the
 * data queues.
 *
 * This test is designed to load the queues up and then
 * transfer the data, repeatly with different size packets.
 *
 * to do this effectively we need to access the TcpStream 
 * objects, instead of the tou.h interface.
 *
 */

#include <iostream>

// for printing sockaddr
#include "udplayer.h"
#include "tcpstream.h"

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

	//	tounet_init();


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

	laddr.sin_port = htons(laddr_port);
	raddr.sin_port = htons(raddr_port);

	/* so create the Udp/Tcp components */
	//UdpLayer udp1(laddr);
	//UdpLayer udp2(raddr);
	LossyUdpLayer udp1(laddr, 0.10);
	LossyUdpLayer udp2(raddr, 0.10);

	/* check that they are okay */
	if ((!udp1.okay()) || (!udp2.okay()))
	{
		std::cerr << "Trouble opening udp ports!";
		std::cerr << std::endl;
		return 1;
	}

	TcpStream tcp1(&udp1);
	TcpStream tcp2(&udp2);

	udp1.setRemoteAddr(raddr);
	udp2.setRemoteAddr(laddr);
	tcp1.connect(); // start the connection.

	/* now connect them */
	while ((!tcp1.isConnected()) || (!tcp2.isConnected()))
	{
		usleep(10000); /* 10 ms */
		tcp1.tick();
		tcp2.tick();
	}

	std::cerr << "Connection Established!" << std::endl;

	for(i = 0; i < 10; i++)
	{

		int size = 1024000;
		char rnddata1[size];
		char rnddata2[size];

		for(j = 0; j < size; j++)
		{
			rnddata1[j] = (unsigned char) (255.0 * 
				rand() / (RAND_MAX + 1.0));

			rnddata2[j] = (unsigned char) (255.0 * 
				rand() / (RAND_MAX + 1.0));
		}

		/* for each iteration, we want to 
		 * (1) fill up the outgoing buffers with stuff 
		 */

		int sent1 = 0;
		int sent2 = 0;
		int fill1, fill2;
		int MaxSend = 1000000;

		while((fill1 = tcp1.write_allowed()) && (sent1 < MaxSend))
		{
			/* fill with a random little bit more */
			int psize = (int) ((i + 1.0) * 255.0 * 
				rand() / (RAND_MAX + 1.0));

			/* don't overload */
			if (psize > fill1)
			{
				std::cerr << "LAST FILL1" << std::endl;
				psize = fill1;
			}

			int ret = tcp1.write(&(rnddata1[sent1]), psize);
			
			if (ret)
			{
				sent1 += ret;
				std::cerr << "Filled tcp1 with " << ret << " more bytes, total:";
				std::cerr << sent1 << " was allowed: " << fill1;
				std::cerr << std::endl;
			}

			//tcp1.status(std::cerr);

		}
		std::cerr << "Tcp1 full with " << sent1 << " bytes ";
		std::cerr << std::endl;

		while((fill2 = tcp2.write_allowed()) && (sent2 < MaxSend))
		{
			/* fill with a random little bit more */
			/* slightly larger sizes */
			int psize =  (int)  ((i + 1.0) * 1255.0 * 
				rand() / (RAND_MAX + 1.0));

			/* don't overload */
			if (psize > fill2)
			{
				std::cerr << "LAST FILL2" << std::endl;
				psize = fill2;
			}

			int ret = tcp2.write(&(rnddata2[sent2]), psize);
			
			if (ret)
			{
				sent2 += ret;
				std::cerr << "Filled tcp2 with " << ret << " more bytes, total:";
				std::cerr << sent2 << " was allowed: " << fill2;
				std::cerr << std::endl;
			}

			//tcp2.status(std::cerr);

		}
		std::cerr << "Tcp2 full with " << sent2 << " bytes ";
		std::cerr << std::endl;

		/* for every second iteration, fill up the read buffer before starting */
		if (i % 2 == 0)
		{
			for(j = 0; j < 100; j++)
			{
				tcp1.tick();
				tcp2.tick();
			}
		}

		/* now we read/tick and empty */
		int read1 = 0;
		int read2 = 0;

		while(read1 < sent2)
		{
			tcp1.tick();
			tcp2.tick();

			/* fill with a random little bit more */
			/* This one has a small read, while tcp2 has a large read */
			int psize = (int) ((i + 1.0) * 100.0 * 
				rand() / (RAND_MAX + 1.0));

			/* limit to what we have! */
			if (psize > sent2 - read1)
			{
				std::cerr << "LAST READ1" << std::endl;
				psize = sent2 - read1;
			}

			char rbuf[psize];
			int  rsize = psize;

			int ret = tcp1.read(rbuf, rsize);
			if (0 < ret)
			{
				/* check the data */
				for(j = 0; j < ret; j++)
				{
					if (rnddata2[read1 + j] != rbuf[j])
					{
						std::cerr << "Error Data Mismatch @ read1:" << read1;
						std::cerr << " + j:" << j << " rsize: " << rsize;
						std::cerr << " Index: " << read1 + j;
						std::cerr << std::endl;

						int badoffset = read1 + j;
						for(int k = -10; k < 10; k++)
						{
							printf("Orig: %02x, Trans: %02x\n", 
								(unsigned char) rnddata2[badoffset+k],
								(unsigned char) rbuf[j + k]);
						}


						exit(1);
					}
				}
				read1 += ret;
			}
			else
			{
				std::cerr << "Read Error: " << ret << std::endl;
			}
			std::cerr << "Requested " << psize << ", got " << ret << " bytes" << std::endl;
			std::cerr << "Read " << read1 << " of " << sent2 << " bytes" << std::endl;
		}

		sleep(2);


		while(read2 < sent1)
		{
			tcp1.tick();
			tcp2.tick();

			/* fill with a random little bit more */
			int psize = (int) ((i + 1.0) * 10000.0 * 
				rand() / (RAND_MAX + 1.0));

			/* limit to what we have! */
			if (psize > sent1 - read2)
			{
				std::cerr << "LAST READ2" << std::endl;
				psize = sent1 - read2;
			}

			char rbuf[psize];
			int  rsize = psize;

			int ret = tcp2.read(rbuf, rsize);
			if (0 < ret)
			{
				/* check the data */
				for(j = 0; j < ret; j++)
				{
					if (rnddata1[read2 + j] != rbuf[j])
					{
						std::cerr << "Error Data Mismatch @ read2:" << read2;
						std::cerr << " + j:" << j << " rsize: " << rsize;
						std::cerr << " Index: " << read2 + j;
						std::cerr << std::endl;
						exit(1);
					}
				}
				read2 += ret;
			}
			else
			{
				std::cerr << "Read Error: " << ret << std::endl;
			}
			std::cerr << "Requested " << psize << ", got " << ret << " bytes" << std::endl;
			std::cerr << "Read " << read2 << " of " << sent1 << " bytes" << std::endl;
		}

		std::cerr << "Iteration " << i + 1 << " finished correctly!" << std::endl;
		sleep(5);

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








