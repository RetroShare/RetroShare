/*
 * bitdht/bdmidids_test.cc
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
#include <iostream>

int main(int argc, char **argv)
{

	/* Do this multiple times */
	int i, j;

	std::cerr << "Test Mid Peer Intersection....." << std::endl;
	for(i = 0; i < 10; i++)
	{
		bdNodeId targetId;
		bdNodeId peerId;

		bdStdRandomNodeId(&targetId);
		bdStdRandomNodeId(&peerId);
	
		std::cerr << "-------------------------------------------------" << std::endl;

		for(j = 0; j < 10; j++)
		{

			bdNodeId midId;

			bdStdRandomMidId(&targetId, &peerId, &midId);

			bdMetric TPmetric;
			bdMetric TMmetric;
			bdMetric PMmetric;

			bdStdDistance(&targetId, &peerId, &TPmetric);
			bdStdDistance(&targetId, &midId,  &TMmetric);
			bdStdDistance(&peerId, &midId,    &PMmetric);

			int TPdist = bdStdBucketDistance(&TPmetric);
			int TMdist = bdStdBucketDistance(&TMmetric);
			int PMdist = bdStdBucketDistance(&PMmetric);

			std::cerr << "Target: ";
			bdStdPrintNodeId(std::cerr,&targetId);
			std::cerr << " Peer: ";
			bdStdPrintNodeId(std::cerr,&peerId);
			std::cerr << std::endl;

			std::cerr << "\tTarget ^ Peer: ";
			bdStdPrintNodeId(std::cerr,&TPmetric);
			std::cerr << " Bucket: " << TPdist;
			std::cerr << std::endl;

			std::cerr << "\tTarget ^ Mid: ";
			bdStdPrintNodeId(std::cerr,&TMmetric);
			std::cerr << " Bucket: " << TMdist;
			std::cerr << std::endl;

			std::cerr << "\tPeer ^ Mid: ";
			bdStdPrintNodeId(std::cerr,&PMmetric);
			std::cerr << " Bucket: " << PMdist;
			std::cerr << std::endl;

			/* now save mid to peer... and repeat */
			peerId = midId;
		}
	}

	return 1;
}


