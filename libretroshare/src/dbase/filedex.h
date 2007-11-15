/*
 * "$Id: filedex.h,v 1.4 2007-02-18 21:46:49 rmf24 Exp $"
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



#ifndef MRK_PQI_FILEINDEXER
#define MRK_PQI_FILEINDEXER

/* a file index.
 *
 * initiated with a list of directories.
 *
 */

#include <list>
#include <string>

#define FD_VIS_EXACT_ONLY    0
#define FD_VIS_SEARCH        1
#define FD_VIS_LISTING       2

/* if vis = FD_VIS_EXACT_ONLY 
 * Then only an exact filename match works
 * if vis = FD_VIS_SEARCH
 * Then only a search will return it.
 * if vis = FD_VIS_LISTING
 * Then any method will find it.
 */

class fdex
{
	public:
	fdex();
	fdex(const char *path, const char *srchpath, 
		const char *file, const char *ext, int len, int vis);

std::string path; // full path. (not searched)
std::string subdir; // searched if flag set.
std::string file;
std::string ext;
int	len;
int     vis; 

};


class DirItem
{
	public:
	DirItem(std::string bp, std::string sd, int v)
	: basepath(bp), subdir(sd), vis(v) {}
	std::string	getFullPath()
	{
		return basepath + subdir;
	}
	std::string basepath;
	std::string subdir;
	int vis;
};

class filedex
{
	public:

int	load(std::list<DirItem> dirs);
int	clear();

// -1 for no limit.
// If remote ... then restrictions apply.
std::list<fdex *> search(std::list<std::string>, int limit, bool local); 
std::list<fdex *> dirlisting(std::string); 
std::list<fdex *> findfilename(std::string); 

	private:

int	processdirs();

	std::list<DirItem> dirtodo;
	std::list<std::string> dirdone;

	/* dbase */
	std::list<fdex *> files;
	std::list<fdex *> dirlist;
};




#endif
