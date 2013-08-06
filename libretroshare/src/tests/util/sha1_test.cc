
/*
 * "$Id: dirtest.cc,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2012-2012 by Cyril Soler
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



#include "util/rsdir.h"
#include "util/utest.h"
#include <util/argstream.h>

#include <iostream>
#include <list>
#include <string>
#include <stdio.h>

void printHelp(int argc,char *argv[])
{
	std::cerr << argv[0] << ": test RS sha1sum class implementation." << std::endl;
	std::cerr << "Usage: " << argv[0] << " [test file]" << std::endl ;
}

INITTEST() ;

int main(int argc,char *argv[])
{
	std::string inputfile ;
	argstream as(argc,argv) ;

	as >> parameter('i',"input",inputfile,"input file name to hash",false)
		>> help() ;

	as.defaultErrorHandling() ;

	if(inputfile.empty())
		inputfile = argv[0] ;

	FILE *f = RsDirUtil::rs_fopen(inputfile.c_str(),"r") ;

	CHECK(f != NULL) ;

		std::cerr << "Testing sha1" << std::endl;
	uint32_t SIZE = 1024*1024 ;
	unsigned char *buf = new unsigned char[SIZE] ;
	int len = fread(buf,1,SIZE,f) ;

		std::cerr << "Read " << len << " bytes" << std::endl;

	Sha1CheckSum sum = RsDirUtil::sha1sum(buf,len) ;
	{
		std::cerr << std::hex << sum.fourbytes[0] << std::endl;
		std::cerr << "New method        : " << sum.toStdString() << std::endl;
	}

	std::string hash ;
	uint64_t size ;
	RsDirUtil::getFileHash(inputfile.c_str(),hash,size) ;

		std::cerr << "Old method        : " << hash << std::endl;
		CHECK(hash == sum.toStdString()) ;

	Sha1CheckSum H(hash) ;
		std::cerr << "Hashed transformed: " << H.toStdString() << std::endl;
		CHECK(hash == H.toStdString()) ;

		std::cerr << "Computing all chunk hashes:" << std::endl;

	fseek(f,0,SEEK_SET) ;
	int n=0 ;

	while(len = fread(buf,1,SIZE,f))
	{
		Sha1CheckSum sum = RsDirUtil::sha1sum(buf,len) ;
			std::cerr << "Chunk " << n << ": " << sum.toStdString() << std::endl;
		n++;
	}

	fclose(f) ;

	FINALREPORT("Sha1Test") ;
	return TESTRESULT() ;
}

