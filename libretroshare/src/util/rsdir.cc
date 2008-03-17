
/*
 * "$Id: rsdir.cc,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
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

// Includes for directory creation.
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "util/rsdir.h"
#include <string>
#include <iostream>

std::string 	RsDirUtil::getTopDir(std::string dir)
{
	std::string top;

	/* find the subdir: [/][dir1.../]<top>[/]
	 */
	int i,j;
	int len = dir.length();
	for(j = len - 1; (j > 0) && (dir[j] == '/'); j--);
	for(i = j; (i > 0) && (dir[i] != '/'); i--);

	if (dir[i] == '/')
		i++;

	for(; i <= j; i++)
	{
		top += dir[i];
	}

	return top;
}

std::string 	RsDirUtil::removeTopDir(std::string dir)
{
	std::string rest;

	/* remove the subdir: [/][dir1.../]<top>[/]
	 */
	int i,j;
	int len = dir.length();
	for(j = len - 1; (j > 0) && (dir[j] == '/'); j--);
	for(i = j; (i >= 0) && (dir[i] != '/'); i--);

	/* remove any more slashes */
	for(; (i >= 0) && (dir[i] == '/'); i--);

	for(j = 0; j <= i; j++)
	{
		rest += dir[j];
	}

	return rest;
}

std::string 	RsDirUtil::getRootDir(std::string dir)
{
	std::string root;

	/* find the subdir: [/]root[/...]
	 */
	int i,j;
	int len = dir.length();
	for(i = 0; (i < len) && (dir[i] == '/'); i++);
	for(j = i; (j < len) && (dir[j] != '/'); j++);
	if (i == j)
		return root; /* empty */
	for(; i < j; i++)
	{
		root += dir[i];
	}
	return root;
}

std::string RsDirUtil::removeRootDir(std::string path)
{
	unsigned int i, j;
	unsigned int len = path.length();
	std::string output;

	/* chew leading '/'s */
	for(i = 0; (i < len) && (path[i] == '/'); i++);
	if (i == len)
			return output; /*  empty string */

	for(j = i; (j < len) && (path[j] != '/'); j++); /* run to next '/' */
	for(; (j < len) && (path[j] == '/'); j++); 	/* chew leading '/'s */

	for(; j < len; j++)
	{
		output += path[j];
	}

	return output;
}

std::string RsDirUtil::removeRootDirs(std::string path, std::string root)
{
	/* too tired */
	std::string notroot;
	//std::cerr << "remoteRootDir( TODO! )";

	unsigned int i = 0, j = 0;

	/* catch empty data */
	if ((root.length() < 1) || (path.length() < 1))
		return notroot;
		
	if ((path[0] == '/') && (root[0] != '/'))
	{
		i++;
	}

	for(; (i < path.length()) && (j < root.length()) && (path[i] == root[j]); i++, j++);

	/* should have consumed root. */
	if (j == root.length())
	{
		//std::cerr << "matched root!" << std::endl;
	}
	else
	{
		//std::cerr << "failed i: " << i << ", j: " << j << std::endl;
		//std::cerr << "root: " << root << " path: " << path << std::endl;
		return notroot;
	}

	if (path[i] == '/')
	{
		i++;
	}

	for(; i < path.length(); i++)
	{
		notroot += path[i];
	}

	//std::cerr << "Found NotRoot: " << notroot << std::endl;

	return notroot;
}



int	RsDirUtil::breakupDirList(std::string path, 
			std::list<std::string> &subdirs)
{
	int start = 0;
	unsigned int i;
	for(i = 0; i < path.length(); i++)
	{
		if (path[i] == '/')
		{
			if (i - start > 0)
			{
				subdirs.push_back(path.substr(start, i-start));
			}
			start = i+1;
		}
	}
	// get the final one.
	if (i - start > 0)
	{
		subdirs.push_back(path.substr(start, i-start));
	}
	return 1;
}



bool	RsDirUtil::checkCreateDirectory(std::string dir)
{
	struct stat buf;
	int val = stat(dir.c_str(), &buf);
	if (val == -1)
	{
		// directory don't exist. create.
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS // UNIX
		if (-1 == mkdir(dir.c_str(), 0777))
#else // WIN
		if (-1 == mkdir(dir.c_str()))
#endif
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/

		{
		  std::cerr << "check_create_directory() Fatal Error --";
		  std::cerr <<std::endl<< "\tcannot create:" <<dir<<std::endl;
		  return 0;
		}

		std::cerr << "check_create_directory()";
		std::cerr <<std::endl<< "\tcreated:" <<dir<<std::endl;
	} 
	else if (!S_ISDIR(buf.st_mode))
	{
		// Some other type - error.
		std::cerr<<"check_create_directory() Fatal Error --";
		std::cerr<<std::endl<<"\t"<<dir<<" is nor Directory"<<std::endl;
		return 0;
	}
	std::cerr << "check_create_directory()";
	std::cerr <<std::endl<< "\tDir Exists:" <<dir<<std::endl;
	return 1;
}



#include <dirent.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>
//#include <unistd.h>

bool 	RsDirUtil::cleanupDirectory(std::string cleandir, std::list<std::string> keepFiles)
{

	/* check for the dir existance */
	DIR *dir = opendir(cleandir.c_str());
	std::list<std::string>::const_iterator it;

	if (!dir)
	{
		return false;
	}

	struct dirent *dent;
	struct stat buf;

	while(NULL != (dent = readdir(dir)))
	{
		/* check entry type */
		std::string fname = dent -> d_name;
		std::string fullname = cleandir + "/" + fname;

	 	if (-1 != stat(fullname.c_str(), &buf))
		{
			/* only worry about files */
			if (S_ISREG(buf.st_mode))
			{
				/* check if we should keep it */
				if (keepFiles.end() == (it = std::find(keepFiles.begin(), keepFiles.end(), fname)))
				{
					/* can remove */
					remove(fullname.c_str());
				}
			}
		}
	}
	/* close directory */
	closedir(dir);

	return true;
}

#include <openssl/sha.h>
#include <sstream>
#include <iomanip>


/* Function to hash, and get details of a file */
bool RsDirUtil::getFileHash(std::string filepath, 
				std::string &hash, uint64_t &size)
{
	FILE *fd;
	int  len;
	SHA_CTX *sha_ctx = new SHA_CTX;
	unsigned char sha_buf[SHA_DIGEST_LENGTH];
	unsigned char gblBuf[512];

	if (NULL == (fd = fopen(filepath.c_str(), "rb")))
		return false;

	/* determine size */
 	fseek(fd, 0, SEEK_END);
	size = ftell(fd);
	fseek(fd, 0, SEEK_SET);

	SHA1_Init(sha_ctx);
	while((len = fread(gblBuf,1, 512, fd)) > 0)
	{
		SHA1_Update(sha_ctx, gblBuf, len);
	}

	/* reading failed for some reason */
	if (ferror(fd)) 
	{
		delete sha_ctx;
		fclose(fd);
		return false;
	}

	SHA1_Final(&sha_buf[0], sha_ctx);

        std::ostringstream tmpout;
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		tmpout << std::setw(2) << std::setfill('0') << std::hex << (unsigned int) (sha_buf[i]);
	}
	hash = tmpout.str();

	delete sha_ctx;
	fclose(fd);
	return true;
}

