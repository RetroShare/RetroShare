/*
 * RetroShare FileCache Module: fitest2.cc
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

#include "dbase/findex.h"

#include <iostream>

FileIndex *createBasicFileIndex(rstime_t age);

int test1(FileIndex *fi);
int test2(FileIndex *fi);

int main()
{
	FileIndex *fi = createBasicFileIndex(100);

	test1(fi);

	delete fi;

	return 1;
}

int test1(FileIndex *fi)
{
	/* in this test we are going to get the old directories - and update them */
	rstime_t stamp = 200;

	DirEntry *olddir = NULL;
	FileEntry fe;
	while((olddir = fi -> findOldDirectory(stamp)))
	{
		/* update the directories and files here */
		std::map<std::string, DirEntry *>::iterator  dit;
		std::map<std::string, FileEntry *>::iterator fit;
		
		/* update this dir */
		fe.name = olddir->name;
		fi -> updateDirEntry(olddir->parent->path, fe, stamp);

		/* update subdirs */
		for(dit = olddir->subdirs.begin(); dit != olddir->subdirs.end(); dit++)
		{
			fe.name = (dit->second)->name;
			/* set the age as out-of-date so that it gets checked */
			fi -> updateDirEntry(olddir->path, fe, 0);
		}

		/* update files */
		for(fit = olddir->files.begin(); fit != olddir->files.end(); fit++)
		{
			fe.name = (fit->second)->name;
			fi -> updateFileEntry(olddir->path, fe, stamp);
		}

	  	/* clean up the dir (should have no effect) */
	  	fi -> removeOldDirectory(olddir->parent->path, olddir->name, stamp);

		std::string out ;
		fi -> printFileIndex(out);
		std::cout << out << std::endl;
	}

		std::string out ;
	fi -> printFileIndex(out);
		std::cout << out << std::endl;

	return 1;
}


int test2(FileIndex *fi)
{
	/* in this test we are going to simulate that 2 directories have disappeared */
	rstime_t stamp = 200;

	DirEntry *olddir = NULL;
	FileEntry fe;
	bool missingdir = false;
	int i = 0;

	while((olddir = fi -> findOldDirectory(stamp)))
	{
	  missingdir = false;
	  if (i % 2 == 0)
	  {
		std::cerr << " Simulating that dir doesnt exist :" << olddir->path;
		std::cerr << std::endl;
		missingdir = true;
	  }
	  i++;

          if (!missingdir)
          {
		/* update the directories and files here */
		std::map<std::string, DirEntry *>::iterator  dit;
		std::map<std::string, FileEntry *>::iterator fit;
		
		/* update this dir */
		fe.name = olddir->name;
		fi -> updateDirEntry(olddir->parent->path, fe, stamp);

		/* update subdirs */
		for(dit = olddir->subdirs.begin(); dit != olddir->subdirs.end(); dit++)
		{
			fe.name = (dit->second)->name;
			/* set the age as out-of-date so that it gets checked */
			fi -> updateDirEntry(olddir->path, fe, 0);
		}

		/* update files */
		for(fit = olddir->files.begin(); fit != olddir->files.end(); fit++)
		{
			fe.name = (fit->second)->name;
			fi -> updateFileEntry(olddir->path, fe, stamp);
		}
	  }
	  /* clean up the dir */
	  fi -> removeOldDirectory(olddir->parent->path, olddir->name, stamp);

		std::string out ;
	  fi -> printFileIndex(out);
	  std::cout << out << std::endl;
	}

		std::string out ;
	fi -> printFileIndex(out);
	std::cout << out << std::endl;

	return 1;
}





FileIndex *createBasicFileIndex(rstime_t age)
{
	FileIndex *fi = new FileIndex("A SILLY ID");

	FileEntry fe;

	/* print empty FileIndex */
	std::string out ;
	fi -> printFileIndex(out);
	std::cout << out << std::endl;

	std::list<std::string> rootdirs;
	rootdirs.push_back("base1");
	rootdirs.push_back("base2");
	rootdirs.push_back("base3");

	fi -> setRootDirectories(rootdirs, age);

	/* add some entries */
	fe.name = "dir1";
	fi -> updateDirEntry("base1",fe, age);
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

	// one that will fail.
	fe.name = "file20";
	fi -> updateFileEntry("/base3/",fe, age);

	out.clear() ;
	fi -> printFileIndex(out);
	std::cout << out << std::endl;

	return fi;
}

