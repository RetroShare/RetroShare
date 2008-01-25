/*
 * libretroshare/src/tcponudp: udpsort_test.cc
 *
 * TCP-on-UDP (tou) network interface for RetroShare.
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

#include "udptestfn.h"
#include "udpsorter.h"

#define MAX_PEERS 16

int main(int argc, char **argv)
{
	/* get local and remote addresses */
	struct sockaddr_in local;
	struct sockaddr_in peers[MAX_PEERS];
	int numpeers = 0;
	int i,j;

	local.sin_family = AF_INET;
	inet_aton("127.0.0.1", &(local.sin_addr));
	local.sin_port = htons(8767);

	for(i = 0; i < MAX_PEERS; i++)
	{
		peers[i].sin_family = AF_INET;
		inet_aton("127.0.0.1", &(peers[i].sin_addr));
		peers[i].sin_port = htons(8768);
	}

	if (argc < 3)
	{
		std::cerr << "Usage: " << argv[0] << " <l-port> [<p-port1 ..]";
		std::cerr << std::endl;
		exit(1);
	}

	local.sin_port = htons(atoi(argv[1]));
	std::cerr << argv[0] << " Local Port: " << ntohs(local.sin_port);
	std::cerr << std::endl;

	UdpSorter udps(local);

	for(i = 2; i < argc; i++)
	{
		numpeers++;
		peers[i-2].sin_port = htons(atoi(argv[i]));
		std::cerr << "\tPeer Port: " << ntohs(peers[i-2].sin_port);
		std::cerr << std::endl;

		UdpPeerTest *pt = new UdpPeerTest(peers[i-2]);
		udps.addUdpPeer(pt, peers[i-2]);
	}

	int size = 12;
	void *data = malloc(size);
	int ttl = 64;

	/* push packets to the peer */
	for(i = 0; i < 60; i++)
	{
		sleep(1);
		for(j = 0; j < numpeers; j++)
		{
			udps.sendPkt(data, size, peers[j], ttl);
		}
	}
	return 1;
}



