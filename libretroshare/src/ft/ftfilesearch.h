/*******************************************************************************
 * libretroshare/src/ft: ftfilesearch.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008 by Robert Fernie <retroshare@lunamutt.com>                   *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

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

bool    addSearchMode(ftSearch *search, FileSearchFlags hintflags);
virtual bool    search(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const;

	private:

	// should have a mutex to protect vector....
	// but not really necessary as it is const most of the time.

	std::vector<ftSearch *> mSearchs;
};


#endif
