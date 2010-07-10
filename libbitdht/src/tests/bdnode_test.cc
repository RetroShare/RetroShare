/*
 * bitdht/bdnode_test.cc
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
#include "bitdht/bdstddht.h"
#include "bitdht/bdquery.h"
#include "bitdht/bdnode.h"

#define N_PEERS_TO_ADD_INIT 10
#define N_PEERS_TO_ADD 11
#define N_PEERS_TO_START 10
#define N_PEERS_TO_PRINT 1
#define N_QUERIES 2

int main(int argc, char **argv)
{

	/* create some ids */
	bdDhtFunctions *fns = new bdStdDht();

	bdNodeId ownId;
	bdStdRandomNodeId(&ownId);

	bdNode node(&ownId, "bdTEST","./dht.log", fns);

	int i = 0;
	for (i = 0; i < N_PEERS_TO_ADD_INIT; i++)
	{
		bdId tmpId;
		bdStdRandomId(&tmpId);

		node.addPeer(&tmpId, 0);
	}

	node.printState();


	for(i = 0; i < N_QUERIES; i++)
	{
		/* create a query */
		bdNodeId queryId;
		bdStdRandomNodeId(&queryId);

		node.addQuery(&queryId, 0);
	}

	node.printState();

	for (i = 0; i < N_PEERS_TO_ADD; i++)
	{
		bdId tmpId;
		bdStdRandomId(&tmpId);

		node.addPeer(&tmpId, 0);

		if (i % N_PEERS_TO_PRINT == 0)
		{
			node.printState();
			node.iteration();
			sleep(5);
		}
	}

	return 1;
}


