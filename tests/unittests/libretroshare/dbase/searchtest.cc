/*
 * RetroShare FileCache Module: searchtest.cc
 *   
 * Copyright 2004-2007 by Kefei Zhou.
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

#include <gtest/gtest.h>

#include "dbase/findex.h"
#include <iostream>
#include <fstream>

TEST(libretroshare_dbase, SearchTest)
{
	RsPeerId peerId;
	peerId.random();

	std::cout << std::string::npos << std::endl;
	std::string testfile = "searchtest.index";
	RsFileHash fhash("6851c28d99a6616a86942c3914476bf11997242a");
	FileIndex *fi = new FileIndex(peerId);

	// loading fileindex
	std::cout << std::endl << "Test load" << std::endl;
	fi->loadIndex(testfile, fhash, 1532);
	fi->printFileIndex(std::cout);
	std::cout << "FileIndex Loaded" << std::endl << std::endl;

	std::list<FileEntry *> hashresult;
	std::list<FileEntry *> termresult;

	// searchhash
	RsFileHash findhash("82bffa6e1cdf8419397311789391238174817481");

	std::cout << "Search hash : " << findhash << std::endl;
	fi->searchHash(findhash, hashresult);

	while(!hashresult.empty())
	{
	   std::string out ;
		hashresult.back()->print(out);
		std::cout << out << std::endl;
		hashresult.pop_back();
	}

	// searchterm
	std::list<std::string> terms;
	terms.push_back("paper");
	terms.push_back("doc");

	std::cout << "Search terms" << std::endl;
	fi->searchTerms(terms, termresult);

	while(!termresult.empty())
	{
	   std::string out ;
		termresult.back()->print(out);
		std::cout << out << std::endl;
		termresult.pop_back();
	}

	delete fi;
}
