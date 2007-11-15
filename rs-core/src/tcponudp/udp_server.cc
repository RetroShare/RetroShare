/*
 * "$Id: udp_server.cc,v 1.4 2007-02-18 21:46:50 rmf24 Exp $"
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
#include "tcpstream.h"

#include <iostream>

/* unix only
#include <netinet/in.h>
#include <arpa/inet.h>
*/

#include "tou_net.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

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

	while(-1 != (c = getopt(argc, argv, "f:pco")))
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
			case 'f':
	/* this can only work when the define below exists in tcpstream */
#ifdef DEBUG_TCP_STREAM_EXTRA 
				setupBinaryCheck(std::string(optarg));
#else
				std::cerr << "Binary Check no Enabled!" << std::endl;
#endif
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

	tounet_init();

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

	laddr.sin_port = htons(atoi(argv[optind+1]));
	raddr.sin_port = htons(atoi(argv[optind+3]));

	std::cerr << "Local Address: " << laddr << std::endl;
	std::cerr << "Remote Address: " << raddr << std::endl;

	//LossyUdpLayer udpl(laddr, 0.01);
	UdpLayer udpl(laddr);
	if (!udpl.openSocket())
	{
		std::cerr << "Cannot Open Local Address: " << laddr << std::endl;
		exit(1);
	}

	udpl.setRemoteAddr(raddr);

	TcpStream tcp(&udpl);

	if (toConnect)
	{
		tcp.connect();
	}

	while(!tcp.isConnected())
	{
		sleep(1);
		std::cerr << "Waiting for TCP to Connect!" << std::endl;
		udpl.status(std::cerr);
		tcp.status(std::cerr);
		tcp.tick();
	}
	std::cerr << "TCP Connected***************************" << std::endl;
	udpl.status(std::cerr);
	tcp.status(std::cerr);
	std::cerr << "TCP Connected***************************" << std::endl;

	int count = 1;

	if (toConnect)
	{
		/* send data */
		int bufsize = 51;
		char buffer[bufsize];
		int readsize = 0;

		tounet_fcntl(0, F_SETFL, O_NONBLOCK);

		bool done = false;
		bool blockread = false;
		while(!done)
		{
			//sleep(1);
			usleep(10000);
			//usleep(1000);
			if (blockread != true)
			{
				readsize = read(0, buffer, bufsize);
			}
			if (readsize == 0)
			{
				/* eof */
				done = true;
			}
			else if ((readsize == -1) && ( EAGAIN == errno ))
			{
				continue;
			}

			/* now we write */
			if (-1 == tcp.write(buffer, readsize))
				blockread = true;
			else
				blockread = false;

			
			tcp.tick();
			if (count++ % 10 == 0)
			{
				std::cerr << "******************************************" << std::endl;
				tcp.status(std::cerr);
			}
		}

		tcp.closeWrite();

		while(!tcp.widle())
		{
			sleep(1);
			//usleep(10000);
			//usleep(1000);
			tcp.tick();
			if (count++ % 10 == 0)
			{
				std::cerr << "Waiting for Idle()" << std::endl;
				std::cerr << "******************************************" << std::endl;
				tcp.status(std::cerr);
			}
		}

		std::cerr << "Transfer Complete: " << tcp.wbytes() << " bytes";
		std::cerr << std::endl;
		return 1;
	}

	/* recv data */
	int  bufsize = 1523;
	char data[bufsize];
	tounet_fcntl(1,F_SETFL,O_NONBLOCK);
	while(1)
	{
		//sleep(1);
		usleep(10000);
		//usleep(1000);
		//int writesize = bufsize;
		int ret;
		if (0 < (ret = tcp.read(data, bufsize)))
		{
			std::cerr << "TF(" << ret << ")" << std::endl;
			write(1, data, ret);
		}
		else if (ret == 0)
		{
			/* completed transfer */
			std::cerr << "Transfer complete :" << tcp.rbytes();
			std::cerr << " bytes" << std::endl;
			break;
		}

		tcp.tick();
		if (count++ % 10 == 0)
		{
			std::cerr << "******************************************" << std::endl;
			tcp.status(std::cerr);
		}
		if ((!stayOpen) && tcp.ridle())
		{
			std::cerr << "Transfer Idle after " << tcp.rbytes();
			std::cerr << " bytes" << std::endl;
			close(1);
			break;
		}
	}

	tcp.closeWrite();

	/* tick for a bit */
	while((stayOpen) || (!tcp.ridle()))
	{
		tcp.tick();
		sleep(1);
		if (count++ % 10 == 0)
		{
			std::cerr << "Waiting for Idle()" << std::endl;
			std::cerr << "******************************************" << std::endl;
			tcp.status(std::cerr);
		}
	}


	if ((!stayOpen) && tcp.ridle())
	{
		//std::cerr << "Transfer complete :" << tcp.rbytes();
		//std::cerr << " bytes" << std::endl;
		close(1);
	return 1;
	}

	return 1;
}



	
