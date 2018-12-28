/*
 * RetroShare FileCache Module: ficachetest.cc
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

#include "dbase/cachestrapper.h"
#include "dbase/cachetest.h"
#include "pqi/p3connmgr.h"

#include <iostream>
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
#else
	#include <windows.h>
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


void handleQuery(CacheStrapper *csp, RsPeerId pid, 
		std::map<RsPeerId, CacheStrapper *> &strappers);

/* A simple test of the CacheStrapper Code.
 *
 * create 3 different CacheStrappers, each with a Source/Store Pair and Transfer Class.
 * pass queries and responses between the CacheStrappers, 
 * and ensure that the hashes in the Caches are updated.
 *
 */

int main(int argc, char **argv)
{

	rstime_t period = 11;
	RsPeerId  pid1("0x0101");
	RsPeerId  pid2("0x0102");
	RsPeerId  pid3("0x0103");

	p3ConnectMgr *connMgr1 = new p3ConnectMgr();
	p3ConnectMgr *connMgr2 = connMgr1;
	p3ConnectMgr *connMgr3 = connMgr1;

	CacheStrapper sc1(connMgr1);
	CacheStrapper sc2(connMgr2);
	CacheStrapper sc3(connMgr3);
	CacheTransfer ctt1(&sc1);
	CacheTransfer ctt2(&sc2);
	CacheTransfer ctt3(&sc3);

	std::map<RsPeerId, CacheStrapper *> strappers;
	strappers[pid1] = &sc1;
	strappers[pid2] = &sc2;
	strappers[pid3] = &sc3;


	std::string nulldir = "";

	CacheSource *csrc1 = new CacheTestSource(&sc1, nulldir);
	CacheStore  *cstore1 = new CacheTestStore(&ctt1, &sc1, nulldir);
	CacheId cid1(TESTID, 0);

	CacheSource *csrc2 = new CacheTestSource(&sc2, nulldir);
	CacheStore  *cstore2 = new CacheTestStore(&ctt2, &sc2, nulldir);
	CacheId cid2(TESTID, 0);

	CacheSource *csrc3 = new CacheTestSource(&sc3, nulldir);
	CacheStore  *cstore3 = new CacheTestStore(&ctt3, &sc3, nulldir);
	CacheId cid3(TESTID, 0);

	CachePair cp1(csrc1, cstore1, cid1);
	CachePair cp2(csrc2, cstore2, cid2);
	CachePair cp3(csrc3, cstore3, cid3);

	sc1.addCachePair(cp1);
	sc2.addCachePair(cp2);
	sc3.addCachePair(cp3);

	/* add in a cache to sc2 */
	RsCacheData cdata;

	cdata.pid = pid1;
	cdata.cid = cid1;
	cdata.name = "Perm Cache";
	cdata.path = "./";
	cdata.hash = "GHJKI";

	csrc1->refreshCache(cdata);

	cdata.pid = pid2;
	cdata.cid = cid2;
	cdata.name = "Funny Cache";
	cdata.path = "./";
	cdata.hash = "ABCDEF";

	csrc2->refreshCache(cdata);

	/* now exercise it */

	for(int i = 0; 1 ; i++)
	{
		RsPeerId src("");
		CacheStrapper *csp = NULL;

		if (i % 5 == 1)
		{
			src = pid1;
			csp = &sc1;
		}
		else if (i % 5 == 2)
		{
			src = pid2;
			csp = &sc2;
		}
		else if (i % 5 == 3)
		{
			src = pid3;
			csp = &sc3;
		}
		std::cerr << std::endl;
		std::cerr << "Cache Iteraton: " << time(NULL) << std::endl;
		std::cerr << std::endl;

		if (src != "")
		{
			handleQuery(csp, src, strappers);
		}


		if (i % 21 == 0)
		{
			/* print out the resources */
			sc1.listCaches(std::cerr);
			sc2.listCaches(std::cerr);
			sc3.listCaches(std::cerr);
		}

		/* every once in a while change the cache on 2 */
		if (i % 31 == 25)
		{
			cdata.hash += "X";
			csrc2->refreshCache(cdata);
		}

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
		sleep(1);
#else
		Sleep(1000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

		/* tick the systems */

	}

	/* Cleanup - TODO */

	return 1;
}

void handleQuery(CacheStrapper *csp, RsPeerId pid, 
		std::map<RsPeerId, CacheStrapper *> &strappers)
{
	/* query */
	std::list<RsPeerId> ids;
	std::list<RsPeerId>::iterator pit;

	std::cerr << "Cache Query from: " << pid << std::endl;

	//csp -> sendCacheQuery(ids, time(NULL));
	for(pit = ids.begin(); pit != ids.end(); pit++)
	{
		std::cerr << "Cache Query for: " << (*pit) << std::endl;
		std::map<RsPeerId, CacheStrapper *>::iterator sit;
		if (strappers.end() != (sit = strappers.find(*pit)))
		{
			std::map<CacheId, RsCacheData> hashs;
			std::map<CacheId, RsCacheData>::iterator hit;
			(sit -> second) -> handleCacheQuery(pid, hashs);
			for(hit = hashs.begin(); hit != hashs.end(); hit++)
			{
				csp -> recvCacheResponse(hit->second, time(NULL));
			}
		}
		else
		{
			std::cerr << "Unknown Query Destination!" << std::endl;
		}
	}
}

