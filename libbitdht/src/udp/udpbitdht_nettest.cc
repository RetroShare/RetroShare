/*
 * bitdht/udpbitdht_nettest.cc
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


#include "udpbitdht.h"
#include "udpstack.h"
#include "bdstddht.h"

#include <string.h>
#include <stdlib.h>

/*******************************************************************
 * Test of bencode message creation functions in bdmsgs.cc
 *
 * Create a couple of each type.
 */

#define MAX_MESSAGE_LEN	10240

int main(int argc, char **argv)
{
	/* setup id, and bootstrap file */
	if (argc < 2)
	{
		std::cerr << " Missing port number " << std::endl;
		exit(1);
	}

	int delta = 0;
	if (argc < 3)
	{
		std::cerr << "No Id Delta" << std::endl;
	}
	else
	{
		delta = atoi(argv[2]);
	}

	int port = atoi(argv[1]);
	std::cerr << "Using Port: " << port << std::endl;
	std::cerr << "Using Delta: " << delta << std::endl;

	bdDhtFunctions *fns = new bdStdDht();

	bdNodeId id;
	memcpy(((char *) id.data), "1234567890abcdefghi", 20);
	uint32_t *deltaptr = (uint32_t *) (id.data);
	(*deltaptr) += htonl(delta);

	std::cerr << "Using NodeId: ";
	fns->bdPrintNodeId(std::cerr, &id);
	std::cerr << std::endl;

	std::string bootstrapfile = "bdboot.txt";

	/* setup the udp port */
        struct sockaddr_in local;
	local.sin_port = htons(port);
        UdpStack *udpstack = new UdpStack(local);

	/* create bitdht component */
	std::string dhtVersion = "dbTEST";
        UdpBitDht *bitdht = new UdpBitDht(udpstack, &id, dhtVersion,  bootstrapfile, fns);

	/* add in the stack */
	udpstack->addReceiver(bitdht);

	/* register callback display */
	bdDebugCallback *cb = new bdDebugCallback();
	bitdht->addCallback(cb);

	/* startup threads */
	//udpstack->start();
	bitdht->start();




	/* do a couple of random continuous searchs. */

	uint32_t mode = BITDHT_QFLAGS_DO_IDLE;


	int count = 0;
	while(1)
	{
		sleep(120);
		if (++count == 2)
		{
			/* switch to one-shot searchs */
			mode = 0;
		}

		bdNodeId rndId;
		bdStdRandomNodeId(&rndId);
		bitdht->addFindNode(&rndId, mode);
	}

	return 1;
}


