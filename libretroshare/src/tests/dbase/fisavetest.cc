/*
 * RetroShare FileCache Module: fisavetest.cc
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

#include "dbase/findex.h"
#include <iostream>

FileIndex *createBasicFileIndex(rstime_t age);

int main()
{
	FileIndex *fi1 = createBasicFileIndex(100);
	FileIndex *fi2 = new FileIndex("A SILLY ID");

	std::string out ;
	fi1->printFileIndex(out);
	std::cout << out <<std::endl;
	std::string fhash;
	uint64_t size;
	std::set<std::string> forbiddenroots;
	fi1->saveIndex("test.index", fhash, size, forbiddenroots);

	std::cout << " Saved Index: Size: " << size << " Hash: " << fhash << std::endl;
	std::cout << " -- new file index -- " << std::endl;
	
	fi2->loadIndex("test.index", fhash, size);
	out.clear() ;
	fi2->printFileIndex(out);
	std::cout << out << std::endl;

	delete fi1;
	delete fi2;

	return 1;
}


FileIndex *createBasicFileIndex(rstime_t age)
{
	FileIndex *fi = new FileIndex("A SILLY ID");

	FileEntry fe;

	std::list<std::string> rootdirs;
	rootdirs.push_back("base1");
	rootdirs.push_back("base2");
	rootdirs.push_back("base3");

	fi -> setRootDirectories(rootdirs, age);

	/* add some entries */
	fe.name = "dir1";
	fi -> updateDirEntry("base1",fe, age);
	fe.name = "dir2";
	fi -> updateDirEntry("base1",fe, age);

	fe.name = "dir01";
	fi -> updateDirEntry("/base1/dir1/",fe, age);

	fe.name = "dir001";
	fi -> updateDirEntry("/base1/dir1/dir01/",fe, age);

	fe.name = "file1";
	fi -> updateFileEntry("/base1/dir1/",fe, age);
	fe.name = "file2";
	fi -> updateFileEntry("/base1/dir1/",fe, age);
	fe.name = "file3";
	fi -> updateFileEntry("/base1/dir1/",fe, age);
	fe.name = "file4";
	fi -> updateFileEntry("/base1/dir1/",fe, age);


	fe.name = "dir2";
	fi -> updateDirEntry("/base1",fe, age);
	fe.name = "file5";
	fi -> updateFileEntry("/base1/dir2/",fe, age);
	fe.name = "file6";
	fi -> updateFileEntry("/base1/dir2/",fe, age);
	fe.name = "file7";
	fi -> updateFileEntry("/base1/dir2/",fe, age);
	fe.name = "file8";
	fi -> updateFileEntry("/base1/",fe, age);


	fe.name = "dir3";
	fi -> updateDirEntry("/base1/dir2/",fe, age);
	fe.name = "file10";
	fi -> updateFileEntry("/base1/dir2/dir3",fe, age);
	fe.name = "file11";
	fi -> updateFileEntry("/base1/dir2/dir3",fe, age);
	fe.name = "file12";
	fi -> updateFileEntry("/base1/dir2/dir3",fe, age);


	fe.name = "dir4";
	fi -> updateDirEntry("/base3/",fe, age);
	fe.name = "file20";
	fi -> updateFileEntry("/base3/dir4/",fe, age);
	fe.name = "file21";
	fi -> updateFileEntry("/base3/dir4",fe, age);

	return fi;
}

