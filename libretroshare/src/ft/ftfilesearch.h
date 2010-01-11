/*
 * libretroshare/src/ft: ftfilesearch.h
 *
 * File Transfer for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
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

#ifndef FT_FILE_SEARCH_HEADER
#define FT_FILE_SEARCH_HEADER

/* 
 * ftFileSearch
 *
 * This is implements an array of ftSearch Interfaces.
 *
 */

#include <vector>

#include "ft/ftsearch.h"

class ftFileSearch: public ftSearch
{

	public:

	ftFileSearch();

bool    addSearchMode(ftSearch *search, uint32_t hintflags);
virtual bool    search(std::string hash, uint32_t hintflags, FileInfo &info) const;

	private:

	// should have a mutex to protect vector....
	// but not really necessary as it is const most of the time.

	std::vector<ftSearch *> mSearchs;
};


#endif
