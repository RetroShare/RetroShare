/*
 * libretroshare/src/ft: ftserver1test.cc
 *
 * File Transfer for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
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

/*
 * ftServer2Test - Demonstrates how to check for test stuff.
 */

#ifdef WIN32
#include "util/rswin.h"
#endif


#include "ft/ftserver.h"

#include "ft/ftextralist.h"
#include "ft/ftdatamultiplex.h"
#include "ft/ftfilesearch.h"

#include "pqi/p3authmgr.h"
#include "pqi/p3connmgr.h"

#include "util/rsdebug.h"

#include "ft/pqitestor.h"
#include "util/rsdir.h"
#include "util/utest.h"


#include <sstream>


class TestData
{
	public:

	ftServer *loadServer;
	std::list<ftServer *> otherServers;
	std::list<std::string> extraList;
};


extern "C" void *do_server_test_thread(void *p);


void usage(char *name)
{
	std::cerr << "Usage: " << name << " [-sa] [-p <peerId>] [-d <debugLvl>] [-e <extrafile>] [<path> [<path2> ... ]] ";
	std::cerr << std::endl;
}
	
int main(int argc, char **argv)
{
        int c;
        uint32_t period = 1;
        uint32_t debugLevel = 5;
	bool debugStderr = true;
	bool loadAll = false;

        std::list<std::string> fileList;
        std::list<std::string> extraList;
	std::list<std::string> peerIds;
	std::map<std::string, ftServer *> mFtServers;
	std::map<std::string, p3ConnectMgr *> mConnMgrs;

	ftServer *mLoadServer = NULL;
	std::list<ftServer *> mOtherServers;
        std::list<std::string>::iterator eit;

#ifdef PTW32_STATIC_LIB
         pthread_win32_process_attach_np();
#endif   

#ifdef WIN32
        // Windows Networking Init.
        WORD wVerReq = MAKEWORD(2,2);
        WSADATA wsaData;

        if (0 != WSAStartup(wVerReq, &wsaData))
        {
                std::cerr << "Failed to Startup Windows Networking";
                std::cerr << std::endl;
        }
        else
        {
                std::cerr << "Started Windows Networking";
                std::cerr << std::endl;
        }

#endif


        while(-1 != (c = getopt(argc, argv, "asd:p:e:")))
        {
                switch (c)
                {
                case 'p':
                        peerIds.push_back(optarg);
                        break;
                case 'd':
                        debugLevel = atoi(optarg);
                        break;
                case 's':
                        debugStderr = true;
                        break;
                case 'e':
                        extraList.push_back(optarg);
                        break;
                case 'a':
                        loadAll = true;
                        break;
                default:
                        usage(argv[0]);
                        break;
                }
        }

	/* do logging */
  	setOutputLevel(debugLevel);

	if (optind >= argc)
	{
		std::cerr << "Missing Shared Directories" << std::endl;
		usage(argv[0]);
	}

	for(; optind < argc; optind++)
	{
		std::cerr << "Adding: " << argv[optind] << std::endl;
		fileList.push_back(std::string(argv[optind]));
	}

	/* We need to setup a series 2 - 4 different ftServers....
	 *
	 * Each one needs:
	 *
	 *
	 * A List of peerIds...
	 */

	std::list<std::string>::const_iterator it, jit;

	std::list<pqiAuthDetails> baseFriendList, friendList;
	std::list<pqiAuthDetails>::iterator fit;

	P3Hub *testHub = new P3Hub();
	testHub->start();

	/* Setup Base Friend Info */
	for(it = peerIds.begin(); it != peerIds.end(); it++)
	{
		pqiAuthDetails pad;
		pad.id = *it;
		pad.name = *it;
		pad.trustLvl = 5;
		pad.ownsign = true;
		pad.trusted = false;

		baseFriendList.push_back(pad);

		std::cerr << "ftserver1test::setup peer: " << *it;
		std::cerr << std::endl;
	}

	std::ostringstream pname;
	pname << "/tmp/rstst-" << time(NULL);

	std::string basepath = pname.str();
	RsDirUtil::checkCreateDirectory(basepath);



	for(it = peerIds.begin(); it != peerIds.end(); it++)
	{
		friendList = baseFriendList;
		/* remove current one */
		for(fit = friendList.begin(); fit != friendList.end(); fit++)
		{
			if (fit->id == *it)
			{
				friendList.erase(fit);
				break;
			}
		}

		p3AuthMgr *authMgr = new p3DummyAuthMgr(*it, friendList);
		p3ConnectMgr *connMgr = new p3ConnectMgr(authMgr);
		mConnMgrs[*it] = connMgr;


		for(fit = friendList.begin(); fit != friendList.end(); fit++)
		{
			/* add as peer to authMgr */
			connMgr->addFriend(fit->id);
		}

		P3Pipe *pipe = new P3Pipe(); //(*it);

		/* add server */
		ftServer *server;
		server = new ftServer(authMgr, connMgr);
		mFtServers[*it] = server;
		if (!mLoadServer)
		{
			mLoadServer = server;
		}
		else
		{
			mOtherServers.push_back(server);
		}


		server->setP3Interface(pipe);

		std::string configpath = basepath + "/" + *it;
		RsDirUtil::checkCreateDirectory(configpath);

		std::string cachepath = configpath + "/cache";
		RsDirUtil::checkCreateDirectory(cachepath);

		std::string localpath = cachepath + "/local";
		RsDirUtil::checkCreateDirectory(localpath);
		
		std::string remotepath = cachepath + "/remote";
		RsDirUtil::checkCreateDirectory(remotepath);

		server->setConfigDirectory(configpath);

		NotifyBase *base = NULL;
		server->SetupFtServer(base);

		testHub->addP3Pipe(*it, pipe, connMgr);
		server->StartupThreads();

		/* setup any extra bits */
		if (loadAll)
		{
			server->setSharedDirectories(fileList);
			for(eit = extraList.begin(); eit != extraList.end(); eit++)
			{
				server->ExtraFileHash(*eit, 3600, 0);
			}
		}

	}

	if (mLoadServer)
	{
		mLoadServer->setSharedDirectories(fileList);
		for(eit = extraList.begin(); eit != extraList.end(); eit++)
		{
			mLoadServer->ExtraFileHash(*eit, 3600, 0);
		}
	}
		

	/* stick your real test here */
	std::map<std::string, ftServer *>::iterator sit;
	std::map<std::string, p3ConnectMgr *>::iterator cit;

	/* Start up test thread */
	pthread_t tid;
	TestData *mFt = new TestData;

	/* set data */
	mFt->loadServer = mLoadServer;
	mFt->otherServers = mOtherServers;
	mFt->extraList = extraList;

        void *data = (void *) mFt;
	pthread_create(&tid, 0, &do_server_test_thread, data);
	pthread_detach(tid); /* so memory is reclaimed in linux */

	while(1)
	{
		std::cerr << "ftserver2test::sleep()";
		std::cerr << std::endl;
		sleep(1);

		/* tick the connmgrs */
		for(sit = mFtServers.begin(); sit != mFtServers.end(); sit++)
		{
			/* update */
			(sit->second)->tick();
		}

		for(cit = mConnMgrs.begin(); cit != mConnMgrs.end(); cit++)
		{
			/* update */
			(cit->second)->tick();
		}
	}
}

/* So our actual test can run here.....
 *
 */

INITTEST();

void *do_server_test_thread(void *data)
{
	TestData *mFt = (TestData *) data;

	std::cerr << "do_server_test_thread() running";
	std::cerr << std::endl;

	/************************* TEST 1 **********************
	 * Check that the extra List has been processed.
	 */
	time_t start = time(NULL);

	FileInfo info, info2;
	time_t now = time(NULL);
        std::list<std::string>::iterator eit;
	for(eit = mFt->extraList.begin(); eit != mFt->extraList.end(); eit++)
	{
		while(!mFt->loadServer->ExtraFileStatus(*eit, info))
		{

			/* max of 30 seconds */
			now = time(NULL);
			if (now - start > 30)
			{
				/* FAIL */
				REPORT2( false, "Extra File Hashing");
			}

			sleep(1);
		}

		/* Got ExtraFileStatus */
		REPORT("Successfully Found ExtraFile");

		/* now we can try a search (should succeed) */
		uint32_t hintflags = 0;
		if (mFt->loadServer->FileDetails(info.hash, hintflags, info2))
		{
			CHECK(info2.hash == info.hash);
			CHECK(info2.size == info.size);
			CHECK(info2.fname == info.fname);
		}
		else
		{
			REPORT2( false, "Search for Extra File (Basic)");
		}

		/* search with flags (should succeed) */
		hintflags = RS_FILE_HINTS_EXTRA;
		if (mFt->loadServer->FileDetails(info.hash, hintflags, info2))
		{
			CHECK(info2.hash == info.hash);
			CHECK(info2.size == info.size);
			CHECK(info2.fname == info.fname);
		}
		else
		{
			REPORT2( false, "Search for Extra File (Extra Flag)");
		}

		/* search with other flags (should fail) */
		hintflags = RS_FILE_HINTS_REMOTE | RS_FILE_HINTS_SPEC_ONLY;
		if (mFt->loadServer->FileDetails(info.hash, hintflags, info2))
		{
			REPORT2( false, "Search for Extra File (Fail Flags)");
		}
		else
		{
			REPORT("Search for Extra File (Fail Flags)");
		}

		/* if we try to download it ... should just find existing one*/

		REPORT("Testing with Extra File");
	}

	FINALREPORT("ExtraList Hashing, Searching and Downloading");
	exit(1);
}




