/*
 * libretroshare/src/ft: ftfilesearch.cc
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

#ifndef FT_FILE_SEARCH_HEADERd
#define FT_FILE_SEARCH_HEADER

/* 
 * ftFileSearch
 *
 * This is actually implements the ftSearch Interface.
 *
 */

#include "ft/ftfilesearch.h"
#include "dbase/cachestrapper.h"
#include "dbase/fimonitor.h"
#include "dbase/fistore.h"

bool	ftFileSearch::search(std::string hash, uint64_t size, uint32_t hintflags, FileInfo &info)
{

#if 0
	/* actual search depends on the hints */
	if (hintflags | CACHE)
	{
		mCacheStrapper->..
	}

	if (hintflags | LOCAL)
	{

	}

	if (hintflags | EXTRA)
	{


	}

	if (hintflags | REMOTE)
	{

	}

	private:

	CacheStrapper *mCacheStrapper;
	ftExtraList *mExtraList;
	FileIndexMonitor *mFileMonitor;
	FileIndexStore   *mFileStore;
#endif

}




