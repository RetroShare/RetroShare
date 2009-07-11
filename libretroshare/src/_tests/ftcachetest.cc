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
#include "server/ftfiler.h"
#include "util/rsdir.h"

#include "pqi/pqidebug.h"

#include <iostream>
#include <fstream>

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
	/* setup test */
	std::string tmppath1 = "/tmp/ct1";
	std::string tmppath2 = "/tmp/ct2";
	std::string tmppath3 = "/tmp/ct3";
	std::string tmppathpart1 = tmppath1 + "/partials";
	std::string tmppathpart2 = tmppath2 + "/partials";
	std::string tmppathpart3 = tmppath3 + "/partials";
	std::string tmppathcompleted1 = tmppath1 + "/completed";
	std::string tmppathcompleted2 = tmppath2 + "/completed";
	std::string tmppathcompleted3 = tmppath3 + "/completed";

	std::string tmppathcache2 = tmppath2 + "/cache";
	std::string cachefile = "cachefile.txt";
	std::string tmppathcachefile2 = tmppathcache2 + "/" + cachefile;

	RsDirUtil::checkCreateDirectory(tmppath1.c_str());
	RsDirUtil::checkCreateDirectory(tmppath2.c_str());
	RsDirUtil::checkCreateDirectory(tmppath3.c_str());
	RsDirUtil::checkCreateDirectory(tmppathpart1.c_str());
	RsDirUtil::checkCreateDirectory(tmppathpart2.c_str());
	RsDirUtil::checkCreateDirectory(tmppathpart3.c_str());
	RsDirUtil::checkCreateDirectory(tmppathcompleted1.c_str());
	RsDirUtil::checkCreateDirectory(tmppathcompleted2.c_str());
	RsDirUtil::checkCreateDirectory(tmppathcompleted3.c_str());

	RsDirUtil::checkCreateDirectory(tmppathcache2.c_str());


	/* now create a file */
	std::ofstream out(tmppathcachefile2.c_str());
	out << "Hello this is a cache file!" << std::endl;
	out.close();


	setOutputLevel(10);
	time_t period = 11;
	RsPeerId  pid1("0x0101");
	RsPeerId  pid2("0x0102");
	RsPeerId  pid3("0x0103");

	CacheStrapper sc1(pid1, period);
	CacheStrapper sc2(pid2, period);
	CacheStrapper sc3(pid3, period);

	//CacheTransfer ctt1(&sc1);
	//CacheTransfer ctt2(&sc2);
	//CacheTransfer ctt3(&sc3);

	/* setup of the FileTransfer should wait until 
	 * the CacheSource + CacheStrapper are created
	 */

	FileHashSearch *fhs1 = NULL;
	FileHashSearch *fhs2 = NULL;
	FileHashSearch *fhs3 = NULL;
	ftfiler ff1(&sc1);
	ftfiler ff2(&sc2);
	ftfiler ff3(&sc3);

	ff1.setSaveBasePath(tmppath1);
	ff2.setSaveBasePath(tmppath2);
	ff3.setSaveBasePath(tmppath3);

	ff1.setFileHashSearch(fhs1);
	ff2.setFileHashSearch(fhs2);
	ff3.setFileHashSearch(fhs3);

	std::map<RsPeerId, CacheStrapper *> strappers;
	strappers[pid1] = &sc1;
	strappers[pid2] = &sc2;
	strappers[pid3] = &sc3;


	std::string nulldir = "";

	CacheSource *csrc1 = new CacheTestSource(nulldir);
	//CacheStore  *cstore1 = new CacheTestStore(&ctt1, nulldir);
	CacheStore  *cstore1 = new CacheTestStore(&ff1, nulldir);
	CacheId cid1(TESTID, 0);

	CacheSource *csrc2 = new CacheTestSource(nulldir);
	//CacheStore  *cstore2 = new CacheTestStore(&ctt2, nulldir);
	CacheStore  *cstore2 = new CacheTestStore(&ff2, nulldir);
	CacheId cid2(TESTID, 0);

	CacheSource *csrc3 = new CacheTestSource(nulldir);
	//CacheStore  *cstore3 = new CacheTestStore(&ctt3, nulldir);
	CacheStore  *cstore3 = new CacheTestStore(&ff3, nulldir);
	CacheId cid3(TESTID, 0);

	CachePair cp1(csrc1, cstore1, cid1);
	CachePair cp2(csrc2, cstore2, cid2);
	CachePair cp3(csrc3, cstore3, cid3);

	sc1.addCachePair(cp1);
	sc2.addCachePair(cp2);
	sc3.addCachePair(cp3);


	sc1.addPeerId(pid2);
	sc2.addPeerId(pid1);
	sc2.addPeerId(pid3);
	sc3.addPeerId(pid2);

	/* add in a cache to sc2 */
	CacheData cdata;

	cdata.pid = pid1;
	cdata.cid = cid1;
	cdata.name = cachefile;     //"Perm Cache";
	cdata.path = tmppathcache2; //"./";
	cdata.hash = "GHJKI";
	cdata.size = 28;

	csrc1->refreshCache(cdata);

	/* The file we created */
	cdata.pid = pid2;
	cdata.cid = cid2;
	cdata.name = "Funny Cache";
	cdata.path = "./";
	cdata.size = 1023;
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
		ff1.tick();
		ff2.tick();
		ff3.tick();

		/* exchange packets! */
		ftFileRequest *ftPkt = NULL;
		while(NULL != (ftPkt = ff1.sendFileInfo()))
		{
			std::cerr << "Outgoing ftPkt from ff1";
			std::cerr << std::endl;

			if (ftPkt->id == pid2)
			{
				std::cerr << "ftPkt for ff2" << std::endl;
				ftPkt->id = pid1; /* set source correctly */
				ff2.recvFileInfo(ftPkt);
			}
			else if (ftPkt->id == pid3)
			{
				std::cerr << "ftPkt for ff3" << std::endl;
				ftPkt->id = pid1; /* set source correctly */
				ff3.recvFileInfo(ftPkt);
			}
			else
			{
				std::cerr << "ERROR unknown ftPkt destination!: " << ftPkt->id;
				std::cerr << std::endl;
				delete ftPkt;
			}
		}

		while(NULL != (ftPkt = ff2.sendFileInfo()))
		{
			std::cerr << "Outgoing ftPkt from ff2";
			std::cerr << std::endl;

			if (ftPkt->id == pid1)
			{
				std::cerr << "ftPkt for ff1" << std::endl;
				ftPkt->id = pid2; /* set source correctly */
				ff1.recvFileInfo(ftPkt);
			}
			else if (ftPkt->id == pid3)
			{
				std::cerr << "ftPkt for ff3" << std::endl;
				ftPkt->id = pid2; /* set source correctly */
				ff3.recvFileInfo(ftPkt);
			}
			else
			{
				std::cerr << "ERROR unknown ftPkt destination!: " << ftPkt->id;
				std::cerr << std::endl;
				delete ftPkt;
			}
		}


		while(NULL != (ftPkt = ff3.sendFileInfo()))
		{
			std::cerr << "Outgoing ftPkt from ff3";
			std::cerr << std::endl;

			if (ftPkt->id == pid1)
			{
				std::cerr << "ftPkt for ff1" << std::endl;
				ftPkt->id = pid3; /* set source correctly */
				ff1.recvFileInfo(ftPkt);
			}
			else if (ftPkt->id == pid2)
			{
				std::cerr << "ftPkt for ff2" << std::endl;
				ftPkt->id = pid3; /* set source correctly */
				ff2.recvFileInfo(ftPkt);
			}
			else
			{
				std::cerr << "ERROR unknown ftPkt destination!: " << ftPkt->id;
				std::cerr << std::endl;
				delete ftPkt;
			}
		}
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

	csp -> sendCacheQuery(ids, time(NULL));
	for(pit = ids.begin(); pit != ids.end(); pit++)
	{
		std::cerr << "Cache Query for: " << (*pit) << std::endl;
		std::map<RsPeerId, CacheStrapper *>::iterator sit;
		if (strappers.end() != (sit = strappers.find(*pit)))
		{
			std::map<CacheId, CacheData> hashs;
			std::map<CacheId, CacheData>::iterator hit;
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

