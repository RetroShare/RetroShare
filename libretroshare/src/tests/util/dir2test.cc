/*
 * dirtest.cc
 *
 * RetroShare Test Program.
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

#include <iostream>

int processpath(std::string path);

int main()
{

	std::string path1 = "/home/tst1//test2///test3/";
	std::string path2 = "home2/tst4//test5///test6";
	std::string path3 = "//home3";
	std::string path4 = "//";
	std::string path5 = "/a/b/c/d/";
	std::string path6 = "a//b/c//d";

	processpath(path1);
	processpath(path2);
	processpath(path3);
	processpath(path4);
	processpath(path5);
	processpath(path6);

	return 1;	
}


int processpath(std::string path)
{
	std::string pathtogo = path;
	while(pathtogo != "")
	{
		std::string basedir = RsDirUtil::getRootDir(pathtogo);
		std::string rempath = RsDirUtil::removeRootDir(pathtogo);
		std::string topdir =  RsDirUtil::getTopDir(pathtogo);
		std::string remtoppath = RsDirUtil::removeTopDir(pathtogo);

		std::cerr << "Processing: \"" << pathtogo << "\"" << std::endl;
		std::cerr << "\tRootDir  : \"" << basedir << "\"" << std::endl;
		std::cerr << "\tRemaining: \"" << rempath << "\"" << std::endl;
		std::cerr << "\tTopDir  : \"" <<  topdir << "\"" << std::endl;
		std::cerr << "\tRemaining(Top): \"" << remtoppath << "\"" << std::endl;
		std::cerr << std::endl;

		pathtogo = rempath;
	}
	return 1;
}


	


