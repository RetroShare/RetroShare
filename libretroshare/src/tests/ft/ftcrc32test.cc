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

#include <stdio.h>
#include <iostream>
#include "retroshare/rstypes.h"
#include <util/rsdir.h>
#include <ft/ftchunkmap.h>

#ifdef WIN32
#include "util/rswin.h"
#endif

#include "util/rsdir.h"
#include "util/utest.h"
#include <common/testutils.h>
#include <util/argstream.h>

INITTEST() ;

int main(int argc, char **argv)
{
	int c;
	uint32_t period = 1;
	uint32_t dPeriod = 600; /* default 10 minutes */

	std::string inputfile ;

	argstream as(argc,argv) ;

	as >> parameter('i',"input",inputfile,"Input file to hash. If none, a random file will be created in /tmp",false)
		>> help() ;

	as.defaultErrorHandling() ;

	if(inputfile == "")
	{
		uint64_t S = 3983782 ;
		std::cerr << "Creating a dummy input file in /tmp, of size " << S << std::endl;
		inputfile = "crc_test_data.bin" ;

		if(!TestUtils::createRandomFile(inputfile,S))
			return 1 ;
	}

	std::list<std::string> hashList ;
	hashList.push_back(inputfile) ;

	uint32_t flags = 0;
	for(std::list<std::string>::const_iterator it(hashList.begin()); it  != hashList.end(); it++)
	{
		std::cerr << "Hashing file :" << *it << std::endl ;

		std::string hash  ;
		uint64_t size ;
		std::string name ;
		RsDirUtil::hashFile( *it,name,hash,size) ;

		std::cerr << "Hash = " << hash << std::endl;

		FILE *f = fopen( (*it).c_str(),"rb" ) ;

		if(f == NULL)
		{
			std::cerr << "Could not open this file! Sorry." << std::endl ;
			return 1 ;
		}
		CRC32Map crc_map ;

		if(fseek(f,0,SEEK_END))
		{
			std::cerr << "Could not fseek to end of this file! Sorry." << std::endl ;
			fclose(f) ;
			return 1 ;
		}

		size = ftell(f) ;

		if(fseek(f,0,SEEK_SET))
		{
			std::cerr << "Could not fseek to beginning of this file! Sorry." << std::endl ;
			fclose(f) ;
			return 1 ;
		}

		std::cerr << "File size:" << size << std::endl ;

		RsDirUtil::crc32File(f,size,ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE,crc_map) ;
		fclose(f) ;

		std::cerr << "Got this CRC map: "<< std::endl ;

		for(uint32_t i=0;i<crc_map.size();++i)
			std::cerr << (void*)crc_map[i] ;
		std::cerr << std::endl;
	}

	FINALREPORT("CRC32 test") ;
	return TESTRESULT() ;
}





