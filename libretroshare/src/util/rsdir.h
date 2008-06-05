
/*
 * "$Id: rsdir.h,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
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


#ifndef RSUTIL_DIRFNS_H
#define RSUTIL_DIRFNS_H

#include <string>
#include <list>

namespace RsDirUtil {

std::string 	getTopDir(std::string);
std::string 	getRootDir(std::string);
std::string 	removeRootDir(std::string path);
std::string     removeTopDir(std::string dir);

std::string 	removeRootDirs(std::string path, std::string root);

int     	breakupDirList(std::string path,
                        	std::list<std::string> &subdirs);

bool    	checkCreateDirectory(std::string dir);
bool    	cleanupDirectory(std::string dir, std::list<std::string> keepFiles);

bool 		getFileHash(std::string filepath,                
			std::string &hash, uint64_t &size);

}

	
#endif
