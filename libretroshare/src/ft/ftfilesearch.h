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
 * This is actually implements the ftSearch Interface.
 *
 */

#include "ft/ftsearch.h"

class CacheStrapper;
class ftExtraList;
class FileIndexMonitor;
class FileIndexStore;

class ftFileSearch: public ftSearch
{

	public:

	ftFileSearch(CacheStrapper *c, ftExtraList *x, FileIndexMonitor *m, FileIndexStore *s)
	:mCacheStrapper(c), mExtraList(x), mFileMonitor(m), mFileStore(s) { return; }

virtual bool	search(std::string hash, uint64_t size, uint32_t hintflags, FileInfo &info);

	private:

	CacheStrapper *mCacheStrapper;
	ftExtraList *mExtraList;
	FileIndexMonitor *mFileMonitor;
	FileIndexStore   *mFileStore;

};




