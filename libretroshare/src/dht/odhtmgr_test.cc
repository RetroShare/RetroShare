/*
 * libretroshare/src/dht: odhtmgr_test.cc
 *
 * Interface with OpenDHT for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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



/***** Test for the new DHT system *****/

#include "pqi/p3dhtmgr.h"
#include "pqi/pqimonitor.h"
#include "dht/opendhtmgr.h"

#include "util/rsnet.h"
#include "util/rsthreads.h"
#include "util/rsprint.h"


#include <iostream>
#include <sstream>
#include <unistd.h>

void usage(char *name)
{
	std::cerr << "USAGE: " << name << " -o OwnId [ -p PeerId1 [ -p PeerId2 [ ... ] ] ] ";
	std::cerr << std::endl;
	exit(1);
}

int main(int argc, char **argv)
{
	int c;
	bool setOwnId = false;
	std::string ownId;
	std::list<std::string> peerIds;
	
	while(-1 != (c = getopt(argc, argv, "o:p:")))
	{
		switch (c)
		{
		case 'o':
			ownId = optarg;
			setOwnId = true;
			break;
		case 'p':
			peerIds.push_back(std::string(optarg));
			break;
		default:
			usage(argv[0]);
			break;
		}
	}

	if (!setOwnId)
	{
		std::cerr << "Missing OwnId!";
		usage(argv[0]);
	}
	
	bool haveOwnAddress = false;
	time_t startTime = time(NULL);

	pqiConnectCbDummy cbTester;
	OpenDHTMgr  dhtTester(ownId, &cbTester);

	/* startup dht */
	std::cerr << "Starting up DhtTester()" << std::endl;
	dhtTester.start();

	/* wait for a little before switching on */
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
        sleep(1);
#else
        Sleep(1000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


	std::cerr << "Switching on DhtTester()" << std::endl;
	dhtTester.setDhtOn(true);

	std::cerr << "Adding a List of Peers" << std::endl;
	std::list<std::string>::iterator it;
	for(it = peerIds.begin(); it != peerIds.end(); it++)
	{
		dhtTester.findPeer(*it);
	}
		

	/* wait loop */
	while(1)
	{
		std::cerr << "Main waiting..." << std::endl;
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	        sleep(3);
#else
		Sleep(3000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

		if (!haveOwnAddress)
		{
			if (time(NULL) - startTime > 20)
			{
				std::cerr << "Setting Own Address!" << std::endl;
				haveOwnAddress = true;

				uint32_t type = DHT_ADDR_UDP;

				struct sockaddr_in laddr;
				inet_aton("10.0.0.111", &(laddr.sin_addr));
				laddr.sin_port = htons(7812);
				laddr.sin_family = AF_INET;

				struct sockaddr_in raddr;
				inet_aton("10.0.0.11", &(raddr.sin_addr));
				raddr.sin_port = htons(7812);
				raddr.sin_family = AF_INET;

				dhtTester.setExternalInterface(laddr, raddr, type);
			}
		}

	}
};










