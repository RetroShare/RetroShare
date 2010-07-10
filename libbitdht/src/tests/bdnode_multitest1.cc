/*
 * bitdht/bdnode_multitest1.cc
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


#include "bitdht/bdnode.h"
#include "bitdht/bdstddht.h"

#include <stdlib.h>

/**********************************************************************************
 * tests of multi bdnodes all connected together.
 * in these cases, the networking step is shortcut and the ip addresses ignored.
 * instead the port number is used as an index to peers.
 *
 * test1() 
 *     Small cross seeding, and static list of peers.
 *     Set it going - and see what happens.
 */
 
std::map<bdId, bdNode *> nodes;
std::map<uint16_t, bdId> portIdx;

int main(int argc, char **argv)
{
	time_t sim_time = 60;
	time_t starttime = time(NULL);
	int    n_nodes = 10;
	std::map<bdId, bdNode *>::iterator it;
	std::map<bdId, bdNode *>::iterator nit;
	std::map<uint16_t, bdId>::iterator pit;

	int i, j;

	bdDhtFunctions *fns = new bdStdDht();

	std::cerr << "bdnode_multitest1() Setting up Nodes" << std::endl;
	/* setup nodes */
	for(i = 0; i < n_nodes; i++)
	{
		bdId id;

		bdStdRandomId(&id);

		id.addr.sin_port = htons(i);
		((uint32_t *) (id.id.data))[0] = i * 16 * 16; /* force this so the sort order is maintained! */
		std::cerr << "bdnode_multitest1() Id: ";
		fns->bdPrintId(std::cerr, &id);
		std::cerr << std::endl;

		bdNode *node = new bdNode(&(id.id), "bdTEST", "", fns);

		/* Store in nodes */
		nodes[id] = node;
		/* Store in indices */
		portIdx[i] = id;

	}

	std::cerr << "bdnode_multitest1() Cross Seeding" << std::endl;
	/* do a little cross seeding */
	for(i = 0; i < n_nodes; i++)
	{
		bdId nid = portIdx[i];
		bdNode *node = nodes[nid];

		for(j = 0; j < 5; j++)
		{		
			int peeridx = rand() % n_nodes;

			bdId pid = portIdx[peeridx];
			node->addPotentialPeer(&pid);
		}
	}

	/* ready to run */
	
	std::cerr << "bdnode_multitest1() Simulation Time....." << std::endl;
	i = 0;
	while(time(NULL) < starttime + sim_time)
	{
		i++;
		std::cerr << "bdnode_multitest1() Iteration: " << i << std::endl;

		for(it = nodes.begin(), j = 0; it != nodes.end(); it++, j++)
		{
			/* extract messages to go -> and deliver */
#define MAX_MSG_SIZE	10240
			struct sockaddr_in addr;
			char data[MAX_MSG_SIZE];
			int len = MAX_MSG_SIZE;

			while(it->second->outgoingMsg(&addr, data, &len))
			{
				std::cerr << "bdnode_multitest1() Msg from Peer: " << j;

				/* find the peer */
				int peeridx = htons(addr.sin_port);
				pit = portIdx.find(peeridx);
				nit = nodes.end();
				if (pit != portIdx.end())
				{
					nit = nodes.find(pit->second);
					std::cerr << " For: ";
					fns->bdPrintId(std::cerr, &(nit->first));
					std::cerr << std::endl;
				}
				else
				{
					std::cerr << " For Unknown Destination";
					std::cerr << std::endl;

				}

				if (nit != nodes.end())
				{
					/* set from address */
					nit->second->incomingMsg( (sockaddr_in *) &(it->first.addr), data, len);
				}
				/* reset message size */
				len = MAX_MSG_SIZE;
			}
		}

		for(it = nodes.begin(), j = 0; it != nodes.end(); it++, j++)
		{
			/* tick */
			std::cerr << "bdnode_multitest1() Ticking peer: " << j << std::endl;
			it->second->iteration();
		}

		if (i % 5 == 0)
		{
			std::cerr << "bdnode_multitest1() Displying States"<< std::endl;
			for(it = nodes.begin(), j = 0; it != nodes.end(); it++, j++)
			{
				/* tick */
				std::cerr << "bdnode_multitest1() Peer State: " << j << std::endl;
				it->second->printState();
			}
		}


		/* have a rest */
		sleep(1);
	}
}
















