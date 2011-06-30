/*
 * libretroshare/src/ft: fttransfermoduletest.cc
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
 * Test for Multiplexor.....
 * As this is a key middle component, it is hard to test without other bits.
 * It relies on ftFileProvider/ftFileCreator/ftTransferModule... 
 *
 * And has dummy ftDataSend and ftSearch.
 *
 */

#ifdef WIN32
#include "util/rswin.h"
#endif

#include "ft/ftextralist.h"
#include "ft/ftdatamultiplex.h"
#include "ft/ftfilesearch.h"

#include "ft/ftfileprovider.h"
#include "ft/ftfilecreator.h"
#include "ft/ftcontroller.h"
#include "ft/fttransfermodule.h"

#include "util/utest.h"

INITTEST()


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
        uint32_t dPeriod = 600; /* default 10 minutes */

        std::list<std::string> fileList;

        while(-1 != (c = getopt(argc, argv, "d:p:")))
        {
                switch (c)
                {
                case 'p':
                        period = atoi(optarg);
                        break;
                case 'd':
                        dPeriod = atoi(optarg);
                        break;
                default:
                        usage(argv[0]);
                        break;
                }
        }

	if (optind >= argc)
	{
		std::cerr << "Missing Files" << std::endl;
		usage(argv[0]);
	}

	for(; optind < argc; optind++)
	{
		std::cerr << "Adding: " << argv[optind] << std::endl;
		fileList.push_back(std::string(argv[optind]));
	}

	ftExtraList *eList = new ftExtraList();
	eList->start();

	ftSearch *ftsd = new ftSearchDummy(); 
	ftFileSearch *ftfs = new ftFileSearch();

	ftfs-> addSearchMode(ftsd, RS_FILE_HINTS_LOCAL);
	ftfs-> addSearchMode(eList, RS_FILE_HINTS_EXTRA);

	ftDataSendPair *ftds1 = new ftDataSendPair(NULL);
	ftDataSendPair *ftds2 = new ftDataSendPair(NULL);

	/* setup Actual Test bit */
	ftDataMultiplex *ftmplex1 = new ftDataMultiplex("ownId", ftds2, ftfs);
	ftDataMultiplex *ftmplex2 = new ftDataMultiplex("ownId", ftds1, ftfs);

	ftds1->mDataRecv = ftmplex1;
	ftds2->mDataRecv = ftmplex2;

	ftmplex1->start();
	ftmplex2->start();

	/* Setup Search with some valid results */
	/* Request Data */	

	/* now work the thread */
	std::list<std::string>::iterator it;
	uint32_t flags = 0;
	for(it = fileList.begin(); it  != fileList.end(); it++)
	{
		eList->hashExtraFile(*it, dPeriod, flags);
	}

	if (fileList.size() < 1)
	{
		std::cerr << "come on, give us some files...";
		std::cerr << std::endl;
		return 0;
	}

	/* now request files from ftDataMultiplex 
	 * by adding in a ftTransferModule!
	 */
	std::string filename = *(fileList.begin());

	/* wait for file to hash */
	FileInfo info;
	while(!eList->hashExtraFileDone(filename, info))
	{
		std::cerr << "Waiting for file to hash";
		std::cerr << std::endl;	
		sleep(1);
	}

	std::string savename = "/tmp/" + info.fname;	
	ftFileCreator *creator = new ftFileCreator(savename, info.size, info.hash);
	ftController *controller = NULL;

	ftTransferModule *transfer = new ftTransferModule(creator, ftmplex1, controller);

	ftmplex1->addTransferModule(transfer, creator);

	std::list<std::string> peerIds;
	peerIds.push_back("ownId2");

  	transfer->setFileSources(peerIds);
	transfer->setPeerState("ownId2", PQIPEER_IDLE, 1000);
	//transfer->resumeTransfer();

	/* check file progress */
	while(1)
	{
		std::cerr << "File Transfer Status";
		std::cerr << std::endl;

		std::cerr << "Transfered: " << creator->getRecvd();
		std::cerr << std::endl;

		transfer->tick();
		sleep(1);
	}
}

