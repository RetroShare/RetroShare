/*
 * RetroShare FileCache Module: fimontest.cc
 *   
 * Copyright 2004-2007 by Robert Fernie.
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

#include "findex.h"
#include "fimonitor.h"

#include <iostream>

void usage(char *name)
{
	std::cerr << "Usage: " << name << " [-p <period> ] shareDir1 [shareDir2 [shareDir3 [...]]]";
	std::cerr << std::endl;
	exit(1);
}



int main(int argc, char **argv)
{
	/* handle commandline arguments */
        int c;
	int period = 60; /* recheck period in seconds */

	while((c = getopt(argc, argv,"p:")) != -1)
	{
		switch(c)
		{
			case 'p':
				period = atoi(optarg);
				break;
			default:
				std::cerr << "Bad Option.";
				std::cerr << std::endl;
				usage(argv[0]);
				break;
		}
	}

	std::list<std::string> rootdirs;

	/* add all the rest of the commandline arguments to rootdirs list */
	for(; optind < argc; optind++)
	{
		rootdirs.push_back(argv[optind]);
		std::cerr << "Adding shared directory: " << argv[optind] << std::endl;
	}

	if (rootdirs.size() < 1)
	{
		usage(argv[0]);
	}

	sleep(1);

	FileIndexMonitor mon(NULL,NULL, "", "OWN ID");

	/* setup monitor */
	mon.setPeriod(period);
	mon.setSharedDirectories(rootdirs);

	/* simulate running the thread */
	mon.run();

	return 1;
}


