/*
 * "$Id: filelook.cc,v 1.3 2007-03-05 21:26:03 rmf24 Exp $"
 *
 * Other Bits for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#include "dbase/filelook.h"
#include "util/rsdir.h"

#include <sstream>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>


/* 
 * The Basic Low-Level Local File/Directory Interface.
 *
 * Requests are queued, and serviced (slowly)
 *
 *
 * So this is a simple
 * Local Lookup service,
 * with a locked threaded interface.
 *
 * Input:
 *    "lookup directory".
 *
 * Output:
 *    "FileItems"
 *    "DirItems"
 *
 *
 * a file index.
 *
 * initiated with a list of directories.
 *
 */


fileLook::fileLook()
{
	return;
}

fileLook::~fileLook()
{

	return;
}

	/* interface */
void	fileLook::setSharedDirectories(std::list<std::string> dirs)
{
	int i;
	lockDirs(); { /* LOCKED DIRS */

	/* clear old directories */
	sharedDirs.clear();

	/* iterate through the directories */
	std::list<std::string>::iterator it;
	for(it = dirs.begin(); it != dirs.end(); it++)
	{
		/* get the head directory */
		std::string root_dir = *it;
		std::string top_dir  = RsDirUtil::getTopDir(root_dir);

		/* if unique -> add, else add modifier  */
		bool unique = false;
		for(i = 0; !unique; i++)
		{
			std::string tst_dir = top_dir;
			if (i > 0)
			{
				std::ostringstream out;
				out << "-" << i;
				tst_dir += out.str();
			}
			std::map<std::string, std::string>::const_iterator cit;
			if (sharedDirs.end()== (cit=sharedDirs.find(tst_dir)))
			{
				unique = true;
				/* add it! */
				sharedDirs[tst_dir.c_str()] = root_dir;
				std::cerr << "Added [" << tst_dir << "] => " << root_dir << std::endl;
			}
		}
	}

	} unlockDirs(); /* UNLOCKED DIRS */
}



int	fileLook::lookUpDirectory(PQItem *i)
{
	std::cerr << "lookUpDirectory() About to Lock" << std::endl;
	lockQueues();

	std::cerr << "lookUpDirectory():" << std::endl;
	i -> print(std::cerr);

	mIncoming.push_back(i);

	unlockQueues();
	std::cerr << "lookUpDirectory() Unlocked" << std::endl;

	return 1;
}

PQItem  *fileLook::getResult()
{
	PQItem *i = NULL;

	lockQueues();

	if (mOutgoing.size() > 0)
	{
		i = mOutgoing.front();
		mOutgoing.pop_front();

		std::cerr << "getResult()" << std::endl;
		//i -> print(std::cerr);
	}

	unlockQueues();

	return i;
}

	/* locking 
void	fileLook::lockQueues() 
{}

void	fileLook::unlockQueues() 
{}

void	fileLook::lockDirs() 
{}

void	fileLook::unlockDirs() 
{}

************/

void	fileLook::run()
{
	std::cerr << "fileLook::run() started." << std::endl;
	processQueue();
}

void	fileLook::processQueue()
{
	while(1)
	{
		
		PQItem *i = NULL;

		//std::cerr << "fileLook::run() -> lockQueues()" << std::endl;
		lockQueues();

		if (mIncoming.size() > 0)
		{
			i = mIncoming.front();
			mIncoming.pop_front();
		}

		unlockQueues();
		//std::cerr << "fileLook::run() -> unlockQueues()" << std::endl;

		if (i)
		{
			/* process item */

			/* look up the directory */
			std::cerr << "fileLook::run() -> loadDirectory()" << std::endl;
			loadDirectory(i);
		}
		else
		{
			/* sleep */
			//std::cerr << "fileLook::sleep() ;)" << std::endl;
#ifndef WINDOWS_SYS /* UNIX */
			usleep(50000); /* 50 milli secs (1/20)th of sec */
#else
			Sleep(50); /* 50 milli secs (1/20)th of sec */
#endif

		}
	}

}


/* THIS IS THE ONLY BIT WHICH DEPENDS ON PQITEM STRUCTURE */
void	fileLook::loadDirectory(PQItem *dir)
{
	/* extract the path */
	std::cerr << "loadDirectory()" << std::endl;

	/* check its a PQFileItem */
	PQFileItem *fi = dynamic_cast<PQFileItem *>(dir);
	if (!fi)
	{
		return;
	}

	/* we need the root directory, and the rest */
	std::string req_dir = fi -> path;
	std::string root_dir  = RsDirUtil::getRootDir(req_dir);
	std::string rest_path  = RsDirUtil::removeRootDirs(req_dir, root_dir);

	/* get directory listing */
	std::string full_root = "/dev/null";

	if (root_dir == "")
	{
		std::cerr << "loadDirectory() Root Request" << std::endl;
		loadRootDirs(fi);
	}

	int err = 0;

	lockDirs();
	{
		/* check that dir is one of the shared */
		std::map<std::string, std::string>::iterator it;
		it = sharedDirs.find(root_dir.c_str());
		if (it == sharedDirs.end())
		{
			err = 1;
		}
		else
		{
			full_root = it -> second;
		}
	}
	unlockDirs();
	std::cerr << "root_dir = [" << root_dir << "]" << std::endl;
	std::cerr << "full_root = " << full_root << std::endl;

	if (err)
	{
		std::cerr << "Rejected: root_dir = " << root_dir << std::endl;
		for(unsigned int i = 0; i < root_dir.length(); i++)
		{
			std::cerr << "[" << (int) root_dir[i] << "]:[" << (int) root_dir[i] << "]" << std::endl;
		}
			
		std::map<std::string, std::string>::iterator it;
		for(it = sharedDirs.begin(); it != sharedDirs.end(); it++)
		{
			std::cerr << "Possible Root: " << it -> first << std::endl;
		}

		return;
	}

	/* otherwise load directory */
	std::string full_path = full_root;

	if (rest_path.length() > 0)
	{
		full_path += "/" + rest_path;
	}

	std::cerr << "Looking up path:" << full_path << std::endl;

	/* lookup base dir */
        DIR *rootdir = opendir(full_path.c_str());
	struct dirent *rootent, *subent;
	struct stat buf;

        if (rootdir == NULL) {
		/* error */
		std::cerr << "Error in Path" << std::endl;
		return;
	}

	std::list<PQItem *> subitems;
	int rootcount = 0;
        while(NULL != (rootent = readdir(rootdir)))
	{
		/* iterate through the entries */
		bool issubdir = false;

		/* check entry type */
		std::string fname = rootent -> d_name;
		std::string fullname = full_path + "/" + fname;
		std::cerr << "Statting dirent: " << fullname <<std::endl;

		if (-1 != stat(fullname.c_str(), &buf))
		{
			std::cerr << "buf.st_mode: " << buf.st_mode <<std::endl;
			if (S_ISDIR(buf.st_mode))
			{
				std::cerr << "Is Directory: " << fullname << std::endl;
				if ((fname == ".") || (fname == ".."))
				{
					std::cerr << "Skipping:" << fname << std::endl;
					continue; /* skipping links */
				}
				issubdir = true;
			}
			else if (S_ISREG(buf.st_mode))
			{
				/* is file */
				std::cerr << "Is File: " << fullname << std::endl;
				issubdir = false;
			}
			else
			{
				/* unknown , ignore */
				continue;
			}
		}


		/* If subdir - Count the subentries */
		DIR *subdir = NULL;
		if (issubdir)
		{
			subdir = opendir(fullname.c_str());
		}
		int subdirno = 0;
		if (subdir)
		{
        		while(NULL != (subent = readdir(subdir)))
			{

                        	std::string fname = subent -> d_name;
				if (
#ifndef WINDOWS_SYS /* UNIX */
				   (DT_DIR == subent -> d_type) &&  /* Only works under Unix */
#else /* WIndows */
#endif
					((fname == ".") || (fname == "..")))
				{
					/* do not add! */
					std::cerr << "Removed entry:" << fname << " from subdir count" << std::endl;
				}
				else
				{
					subdirno++;
				}
			}
			/* clean up dir */
        		closedir(subdir);
		}
		rootcount++;

		std::string subname = rootent -> d_name;

		/* create an item */
		PQFileItem *finew = fi -> clone();

		// path should be copied auto.
		finew -> name = subname;

		if (issubdir)
		{
			finew -> reqtype = PQIFILE_DIRLIST_DIR;
			finew -> size = subdirno; /* number of entries in dir */
		}
		else
		{
			finew -> reqtype = PQIFILE_DIRLIST_FILE;
			finew -> size = buf.st_size;
		}
		subitems.push_back(finew);
	}

	/* clean up dir */
        closedir(rootdir);


	/* if necessary create Root Entry */


	/* if constructing Tree - do it here */

	/* put on queue */
	lockQueues();

	std::list<PQItem *>::iterator it;
	for(it = subitems.begin(); it != subitems.end(); it++)
	{
		/* push onto outgoing queue */
		mOutgoing.push_back(*it);
	}
	subitems.clear();

	unlockQueues();
};

void	fileLook::loadRootDirs(PQFileItem *dir)
{
	/* extract the path */
	std::cerr << "loadRootDirs()" << std::endl;

	dir -> path = "";
	int rootcount = 0;

	lockDirs();

	std::list<PQItem *> subitems;
	std::map<std::string, std::string>::iterator it;
	for(it = sharedDirs.begin(); it != sharedDirs.end(); it++)
	{

		/* lookup base dir */
        	DIR *entdir = opendir(it -> second.c_str());
		struct dirent *subent;

		if (!entdir)
		{
			std::cerr << "Bad Shared Dir: " << it->second << std::endl;
			continue;
		}

		int subdirno = 0;
        	while(NULL != (subent = readdir(entdir)))
		{

                       	std::string fname = subent -> d_name;
			if (
#ifndef WINDOWS_SYS /* UNIX */
				   (DT_DIR == subent -> d_type) &&  /* Only works under Unix */
#else /* WIndows */
#endif
				((fname == ".") || (fname == "..")))
			{
				/* do not add! */
				std::cerr << "Removed entry:" << fname << " from subdir count" << std::endl;
			}
			else
			{
				subdirno++;
			}
		}

		closedir(entdir);

		rootcount++;

		/* create an item */
		PQFileItem *finew = dir -> clone();

		// path should be copied auto.
		finew -> name = it -> first;
		finew -> reqtype = PQIFILE_DIRLIST_DIR;
		finew -> size = subdirno; /* number of entries in dir */

		subitems.push_back(finew);
	}
	unlockDirs();


	/* put on queue */
	lockQueues();

	std::list<PQItem *>::iterator it2;
	for(it2 = subitems.begin(); it2 != subitems.end(); it2++)
	{
		/* push onto outgoing queue */
		mOutgoing.push_back(*it2);
	}
	subitems.clear();

	unlockQueues();
};


PQFileItem *fileLook::findFileEntry(PQFileItem *fi)
{
	/* extract the path */
	std::cerr << "findFileEntry()" << std::endl;

	/* we need the root directory, and the rest */
	std::string req_dir = fi -> path;
	std::string root_dir  = RsDirUtil::getRootDir(req_dir);
	std::string rest_path  = RsDirUtil::removeRootDirs(req_dir, root_dir);

	/* get directory listing */
	std::string full_root = "/dev/null";

	int err = 0;

	lockDirs();
	{
		/* check that dir is one of the shared */
		std::map<std::string, std::string>::iterator it;
		it = sharedDirs.find(root_dir.c_str());
		if (it == sharedDirs.end())
		{
			err = 1;
		}
		else
		{
			full_root = it -> second;
		}
	}
	unlockDirs();

	PQFileItem *rtn = NULL;
	if (err)
		return rtn;
	
	rtn = fi -> clone();
	rtn -> path = full_root + "/" + rest_path;

	return rtn;
}

