/*
 * libretroshare/src/ft: ftcrc32test.cc
 *
 * File Transfer for RetroShare.
 *
 * Copyright 2008 by Cyril Soler
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

#include <iostream>
#include "retroshare/rstypes.h"
#include <util/rsdir.h>
#include <ft/ftchunkmap.h>

#ifdef WIN32
#include "util/rswin.h"
#endif

#include "util/rsdir.h"

void usage(char *name)
{
	std::cerr << "Computes a CRC32 map of a file." << std::endl;
	std::cerr << "Usage: " << name << " -h <path>" << std::endl;
}
	
int main(int argc, char **argv)
{
        int c;
        uint32_t period = 1;
        uint32_t dPeriod = 600; /* default 10 minutes */

        std::list<std::string> hashList;

        while(-1 != (c = getopt(argc, argv, "f:h:")))
		  {
			  switch (c)
			  {
				  case 'f':
					  hashList.push_back(std::string(optarg));
					  break;
				  default:
					  usage(argv[0]);
					  break;
			  }
		  }
	
	uint32_t flags = 0;
	for(std::list<std::string>::const_iterator it(hashList.begin()); it  != hashList.end(); it++)
	{
		std::cerr << "Hashing file :" << *it << std::endl ;

		std::string hash  ;
		RsDirUtil::hashFile( *it,hash) ;

		std::cerr << "Hash = " << hash << std::endl;

		FILE *f = fopen( (*it).c_str(),"rb" ) ;

		if(f == NULL)
		{
			std::cerr << "Could not open this file! Sorry." << std::endl ;
			return 0 ;
		}
		CRC32Map crc_map ;

		if(fseek(f,0,SEEK_END))
		{
			std::cerr << "Could not fseek to end of this file! Sorry." << std::endl ;
			fclose(f) ;
			return 0 ;
		}

		uint64_t size = ftell(f) ;

		if(fseek(f,0,SEEK_SET))
		{
			std::cerr << "Could not fseek to beginning of this file! Sorry." << std::endl ;
			fclose(f) ;
			return 0 ;
		}

		std::cerr << "File size:" << size << std::endl ;

		RsDirUtil::crc32File(f,size,ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE,crc_map) ;
		fclose(f) ;

		std::cerr << "Got this CRC map: "<< std::endl ;

		for(uint32_t i=0;i<crc_map.size();++i)
			std::cerr << (void*)crc_map[i] ;
		std::cerr << std::endl;

		return 1 ;
	}
}





