/*
 * bitdht/bdspace_test2.cc
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

#define N_PEERS_TO_ADD 10000
#define N_PEERS_TO_TEST 100
#define N_PEERS_TO_FIND 10

int main(int argc, char **argv)
{

	/* create some ids */
	bdNodeId ownId;
	bdStdRandomNodeId(&ownId);
	bdDhtFunctions *fns = new bdStdDht();

	bdSpace space(&ownId, fns);
	int i = 0;
	for (i = 0; i < N_PEERS_TO_ADD; i++)
	{
		bdId tmpId;
		bdStdRandomId(&tmpId);
		space.add_peer(&tmpId, 0);
	}

	space.printDHT();


	/* now generate random id's and test closeness */
	for(i = 0; i < N_PEERS_TO_TEST; i++)
	{
		bdId tmpId;
		bdStdRandomId(&tmpId);
		std::list<bdId> list1;
		std::multimap<bdMetric, bdId> list2;

		space.find_nearest_nodes(&(tmpId.id), N_PEERS_TO_FIND, list1, list2);
	}

	return 1;
}


