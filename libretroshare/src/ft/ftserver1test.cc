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
 * Test for Whole Basic system.....
 *
 * Put it all together, and make it compile.
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

#include <sstream>



void	do_random_server_test(ftDataMultiplex *mplex, ftExtraList *eList, std::list<std::string> &files);


void usage(char *name)
{
	std::cerr << "Usage: " << name << " [-p <period>] [-d <dperiod>] <path> [<path2> ... ] ";
	std::cerr << std::endl;
}
	
int main(int argc, char **argv)
{
        int c;
        uint32_t period = 1;
        uint32_t debugLevel = 5;
	bool debugStderr = true;

        std::list<std::string> fileList;
	std::list<std::string> peerIds;
	std::map<std::string, ftServer *> mFtServers;
	std::map<std::string, p3ConnectMgr *> mConnMgrs;

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


        while(-1 != (c = getopt(argc, argv, "d:p:s")))
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
                default:
                        usage(argv[0]);
                        break;
                }
        }

	/* do logging */
  	setOutputLevel(debugLevel);

	if (optind >= argc)
	{
		std::cerr << "Missing Files" << std::endl;
		usage(argv[0]);
	}
	std::cerr << "Point 1" << std::endl;

	for(; optind < argc; optind++)
	{
		std::cerr << "Adding: " << argv[optind] << std::endl;
		fileList.push_back(std::string(argv[optind]));
	}
	std::cerr << "Point 2" << std::endl;

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

	std::cerr << "Point 3" << std::endl;
	P3Hub *testHub = new P3Hub();
	testHub->start();
	std::cerr << "Point 4" << std::endl;

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
	std::cerr << "Point 5" << std::endl;

	std::ostringstream pname;
	pname << "/tmp/rstst-" << time(NULL);

	std::string basepath = pname.str();
	RsDirUtil::checkCreateDirectory(basepath);

	std::cerr << "Point 6" << std::endl;


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

                sleep(60);

		NotifyBase *base = NULL;
		server->SetupFtServer(base);

		testHub->addP3Pipe(*it, pipe, connMgr);
		server->StartupThreads();

		/* setup any extra bits */
		server->setSharedDirectories(fileList);

	}

	/* stick your real test here */
	std::map<std::string, ftServer *>::iterator sit;
	std::map<std::string, p3ConnectMgr *>::iterator cit;

	while(1)
	{
		std::cerr << "ftserver1test::sleep()";
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


#if 0

uint32_t do_random_server_iteration(ftDataMultiplex *mplex, ftExtraList *eList, std::list<std::string> &files)
{
	std::cerr << "do_random_server_iteration()";
	std::cerr << std::endl;

	std::list<std::string>::iterator it;
	uint32_t i = 0;
	for(it = files.begin(); it != files.end(); it++)
	{
		FileInfo info;
		if (eList->hashExtraFileDone(*it, info))
		{
			std::cerr << "Hash Done for: " << *it;
			std::cerr << std::endl;
			std::cerr << info << std::endl;

			std::cerr << "Requesting Data Packet";
			std::cerr << std::endl;

        		/* Server Recv */
                	uint64_t offset = 10000;
			uint32_t chunk = 20000;
			mplex->recvDataRequest("Peer", info.hash, info.size, offset, chunk);
			
			i++;
		}
		else
		{
			std::cerr << "do_random_server_iteration() Hash Not Done for: " << *it;
			std::cerr << std::endl;
		}
	}

	return i;

}


void	do_random_server_test(ftDataMultiplex *mplex, ftExtraList *eList, std::list<std::string> &files)
{
	std::cerr << "do_random_server_test()";
	std::cerr << std::endl;

	uint32_t size = files.size();
	while(size > do_random_server_iteration(mplex, eList, files))
	{
		std::cerr << "do_random_server_test() sleep";
		std::cerr << std::endl;

		sleep(10);
	}
}


#endif

