/*
 * libretroshare/src/ft: ftextralisttest.cc
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

#include <sstream>
#include <util/utest.h>
#include <common/fileutils.h>
#include "ft/ftextralist.h"
#include "ft/ftdatamultiplex.h"
#include "ft/ftfilesearch.h"

#include "ftdata_dummy.h"
#include "ftsearch_dummy.h"

void	do_random_server_test(ftDataMultiplex *mplex, ftExtraList *eList, std::list<std::string> &files);

INITTEST() ;

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
		uint32_t N = 4 ;
		std::cerr << "Missing Files. Generating " << N << " random files." << std::endl;

		for(uint32_t i=0;i<N;++i)
		{
			std::ostringstream ss ;
			ss << "file_" << i << ".bin" ;
			uint64_t size = lrand48()%1000 + 200000 ;
			std::string filename = ss.str() ;
			if(!FileUtils::createRandomFile(filename,size))
				return 1 ;
			std::cerr << "  file: " << filename << ", size=" << size << std::endl;
		}
	}
	else
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

	ftDataSend *ftds = new ftDataSendDummy();

	/* setup Actual Test bit */
	ftDataMultiplex *ftmplex = new ftDataMultiplex("ownId", ftds, ftfs);
	ftmplex->start();

	/* Setup Search with some valid results */


	/* Request Data */	

	/* now work the thread */
	std::list<std::string>::iterator it;
	TransferRequestFlags flags(0);
	for(it = fileList.begin(); it  != fileList.end(); it++)
	{
		eList->hashExtraFile(*it, dPeriod, flags);
	}


	/* now request files from ftDataMultiplex */

	/* just request random data packets first */
	do_random_server_test(ftmplex, eList, fileList);

	FINALREPORT("FtDataPlex test") ;

	return TESTRESULT() ;
}



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


