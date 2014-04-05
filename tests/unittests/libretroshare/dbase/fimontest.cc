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

//#include "pqi/p3connmgr.h"
#include "retroshare/rsiface.h"
#include "dbase/cachestrapper.h"
#include "dbase/findex.h"
#include "dbase/fimonitor.h"

#include <iostream>

void usage(char *name)
{
	std::cerr << "Usage: " << name << " [-p <period> ] shareDir1 [shareDir2 [shareDir3 [...]]]";
	std::cerr << std::endl;
	exit(1);
}


/*!
 * What is tested:
 * 1.
 * 2.
 * 3.
 */
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

	std::list<SharedDirInfo> rootdirs;

	/* add all the rest of the commandline arguments to rootdirs list */
	for(; optind < argc; optind++)
	{
		SharedDirInfo dir;
		dir.filename = argv[optind];
		dir.shareflags = DIR_FLAGS_PERMISSIONS_MASK ;
		rootdirs.push_back(dir);
		std::cerr << "Adding shared directory: " << argv[optind] << std::endl;
	}

	if (rootdirs.size() < 1)
	{
		usage(argv[0]);
	}

	//p3ConnectMgr connMgr;
	NotifyBase nb;
	CacheStrapper* cs = NULL;

	FileIndexMonitor mon(cs, &nb, "", "OWN ID", std::string("."));

	/* setup monitor */
	mon.setSharedDirectories(rootdirs);

	/* simulate running the thread */
	mon.run();

	return 1;
}


