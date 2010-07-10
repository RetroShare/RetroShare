/*
 * bitdht/bdudp_test.cc
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */


#include "bitdht/bdpeer.h"
#include "bitdht/bdquery.h"
#include "udp/udpbitdht.h"

#define N_PEERS_TO_ADD 10000
#define N_PEERS_TO_PRINT 1000
#define N_PEERS_TO_START 10

int main(int argc, char **argv)
{

	/* create some ids */
	bdId ownId;
	bdRandomId(&ownId);


	struct sockaddr_in local;
	local.sin_addr.s_addr = 0;
	local.sin_port = htons(7812);
	std::string bootstrapfile = "dht.log";

	bdId bid;

	bid.addr = local;
	bid.id = ownId.id;

        UdpBitDht ubd(local, 0, &bid, bootstrapfile);



	while(1)
	{
		ubd.tick();
		sleep(1);
	}

	return 1;
}


