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

#ifdef WIN32
#include "util/rswin.h"
#endif

#include "ft/ftextralist.h"
#include "util/utest.h"

extern "C" void* runExtraList(void* p)
{
        ftExtraList *eList = (ftExtraList *) p;
        if (!eList)
        {
                pthread_exit(NULL);
        }

	while (1)
	{
		//eList->tick();
		sleep(1);

	}

        delete eList;

        pthread_exit(NULL);

        return NULL;
}



void	displayExtraListDetails(ftExtraList *eList, std::list<std::string> toHash, std::list<std::string> hashed);


void usage(char *name)
{
	std::cerr << "Usage: " << name << " -h <path> -p <period> -d <dperiod>";
	std::cerr << std::endl;
}
	
INITTEST() ;

int main(int argc, char **argv)
{
        int c;
        uint32_t period = 1;
        uint32_t dPeriod = 600; /* default 10 minutes */

        std::list<std::string> hashList;

        while(-1 != (c = getopt(argc, argv, "h:p:d:")))
        {
                switch (c)
                {
                case 'h':
                        hashList.push_back(std::string(optarg));
                        break;
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
	
	ftExtraList *eList = new ftExtraList();

	/* now startup background thread to keep it reunning */
	eList->start();



	/* now work the thread */
	std::list<std::string> toHash;
	std::list<std::string> hashed;
	std::list<std::string>::iterator it;

	TransferRequestFlags flags(0);
	for(it = hashList.begin(); it  != hashList.end(); it++)
	{
		sleep(period);

		/* load up file */
		//eList->addExtraFile(*it);
		eList->hashExtraFile(*it, dPeriod, flags);

		toHash.push_back(*it);

		displayExtraListDetails(eList,  toHash, hashed);
	}

	for(int i=0;i<20;++i)
	{
		sleep(period);

		displayExtraListDetails(eList,  toHash, hashed);
	}

	FINALREPORT("Extra list test.") ;
	return 0; 
}

void	displayExtraListDetails(ftExtraList *eList, std::list<std::string> toHash, std::list<std::string> hashed)
{
	std::cerr << "displayExtraListDetails()";
	std::cerr << std::endl;

	std::list<std::string>::iterator it;
	for(it = toHash.begin(); it != toHash.end(); it++)
	{
		FileInfo info;
		if (eList->hashExtraFileDone(*it, info))
		{
			std::cerr << "displayExtraListDetails() Hash Completed for: " << *it;
			std::cerr << std::endl;
			std::cerr << info << std::endl;
		}
		else
		{
			std::cerr << "displayExtraListDetails() Hash Not Done for: " << *it;
			std::cerr << std::endl;
		}
	}

	for(it = hashed.begin(); it != hashed.end(); it++)
	{
		FileInfo info;
		if (eList->search(*it, FileSearchFlags(0), info))
		{
			std::cerr << "displayExtraListDetails() Found Hash: " << *it;
			std::cerr << std::endl;
			std::cerr << info << std::endl;
		}
		else
		{
			std::cerr << "displayExtraListDetails() Hash Not Found: " << *it;
			std::cerr << std::endl;
		}
	}
}



