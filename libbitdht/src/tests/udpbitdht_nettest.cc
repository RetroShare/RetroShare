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


#include "udp/udpbitdht.h"
#include "udp/udpstack.h"
#include "bitdht/bdstddht.h"

#include <string.h>
#include <stdlib.h>

/*******************************************************************
 * DHT test program.
 *
 *
 * Create a couple of each type.
 */

#define MAX_MESSAGE_LEN	10240

int args(char *name)
{
	std::cerr << "Usage: " << name;
	std::cerr << " -p <port> ";
	std::cerr << " -b </path/to/bootfile> ";
	std::cerr << " -u <uid> ";
	std::cerr << " -r (do random queries) ";
	std::cerr << std::endl;
	return 1;
}

#define DEF_PORT	7500

#define MIN_DEF_PORT	1001
#define MAX_DEF_PORT	16000

#define DEF_BOOTFILE	"bdboot.txt"

int main(int argc, char **argv)
{
	int c;
	int port = DEF_PORT;
	std::string bootfile = DEF_BOOTFILE;
	std::string uid;
	bool setUid = false;
	bool doRandomQueries = false;

	srand(time(NULL));

	while((c = getopt(argc, argv,"p:b:u:r")) != -1)
	{
		switch (c)
		{
			case 'p':
			{
				int tmp_port = atoi(optarg);
				if ((tmp_port > MIN_DEF_PORT) && (tmp_port < MAX_DEF_PORT))
				{
					port = tmp_port;
					std::cerr << "Port: " << port;
					std::cerr << std::endl;
				}
				else
				{
					std::cerr << "Invalid Port";
					std::cerr << std::endl;
					args(argv[0]);
					return 1;
				}
					
			}
			break;
			case 'b':
			{
				bootfile = optarg;
				std::cerr << "Bootfile: " << bootfile;
				std::cerr << std::endl;
			}
			break;
			case 'u':
			{
				setUid = true;
				uid = optarg;
				std::cerr << "UID: " << uid;
				std::cerr << std::endl;
			}
			break;
			case 'r':
			{
				doRandomQueries = true;
				std::cerr << "Doing Random Queries";
				std::cerr << std::endl;
			}
			break;

			default:
			{
				args(argv[0]);
				return 1;
			}
			break;
		}
	}

				
	bdDhtFunctions *fns = new bdStdDht();

	bdNodeId id;

	/* start off with a random id! */
	bdStdRandomNodeId(&id);

	if (setUid)
	{
		int len = uid.size();
		if (len > 20)
		{
			len = 20;
		}

		for(int i = 0; i < len; i++)
		{
			id.data[i] = uid[i];
		}
	}

	std::cerr << "Using NodeId: ";
	fns->bdPrintNodeId(std::cerr, &id);
	std::cerr << std::endl;

	/* setup the udp port */
        struct sockaddr_in local;
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = 0;
	local.sin_port = htons(port);
        UdpStack *udpstack = new UdpStack(local);

	/* create bitdht component */
	std::string dhtVersion = "dbTEST";
        UdpBitDht *bitdht = new UdpBitDht(udpstack, &id, dhtVersion,  bootfile, fns);

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
	mode = 0;


	int count = 0;
	while(1)
	{
		sleep(60);
		if (++count == 2)
		{
			/* switch to one-shot searchs */
			mode = 0;
		}

		if (doRandomQueries)
		{
			bdNodeId rndId;
			bdStdRandomNodeId(&rndId);
			bitdht->addFindNode(&rndId, mode);
		}
	}

	return 1;
}


