
/*
 * "$Id: dirtest.cc,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
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



#include "util/rsdir.h"
#include "util/utest.h"

#include <iostream>
#include <list>
#include <string>

bool testRsDirUtils(std::string path);

INITTEST() ;

int main()
{

	std::list<std::string> dirs;
	std::list<std::string>::iterator it;
	dirs.push_back("/incoming/htuyr/file.txt");
	dirs.push_back("/incoming/htuyr/file.txt ");
	dirs.push_back("/incoming/htuyr/file.txt/");
	dirs.push_back("/incoming/htuyr/file.txt//");
	dirs.push_back("/incoming/htuyr//file.txt//");
	dirs.push_back("/incoming/htuyr//file .txt");
	dirs.push_back("/incoming/htuyr/Q");
	dirs.push_back("/incoming/htuyr/Q//");
	dirs.push_back("/incoming/htuyr/Q/");
	dirs.push_back("/incoming/htuyr/Q/text");
	dirs.push_back("/home/tst1//test2///test3/");
	dirs.push_back("home2/tst4//test5///test6");
	dirs.push_back("//home3");
	dirs.push_back("//");
	dirs.push_back("A");
	dirs.push_back("ABC");
	dirs.push_back("////ABC////");
	dirs.push_back("A/B/C");

	for(it = dirs.begin(); it != dirs.end(); it++)
	{
		testRsDirUtils(*it);
	}

	FINALREPORT("dirtest");

	return TESTRESULT() ;
}

bool testRsDirUtils(std::string path)
{

	std::cerr << "RsUtilTest input: [" << path << "]";
	std::cerr << std::endl;

	std::string top = RsDirUtil::getTopDir(path);
	std::string root = RsDirUtil::getRootDir(path);
	std::string topdirs = RsDirUtil::removeRootDir(path);
	std::string topdirs2 = RsDirUtil::removeRootDirs(path, root);
	std::string restdirs ;
	RsDirUtil::removeTopDir(path,restdirs);
	std::list<std::string> split;
	std::list<std::string>::iterator it;
	RsDirUtil::breakupDirList(path, split);

	std::cerr << "\tTop: [" << top << "]";
	std::cerr << std::endl;
	std::cerr << "\tRest: [" << restdirs << "]";
	std::cerr << std::endl;

	std::cerr << "\tRoot: [" << root << "]";
	std::cerr << std::endl;
	std::cerr << "\tRemoveRoot: [" << topdirs << "]"; 
	std::cerr << std::endl;
	std::cerr << "\tSplit Up "; 
	for(it = split.begin(); it != split.end(); it++)
	{
		std::cerr << ":" << (*it);
	}
	std::cerr << std::endl;
	std::cerr << std::endl;
	return true;
}
