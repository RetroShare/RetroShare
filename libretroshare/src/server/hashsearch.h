/*
 * "$Id: hashsearch.h,v 1.5 2007-02-19 20:08:30 rmf24 Exp $"
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




#ifndef MRK_FILE_HASH_SEARCH_H
#define MRK_FILE_HASH_SEARCH_H

/**********
 * SearchInterface for the FileTransfer
 */

#include "rsiface/rstypes.h"
class FileIndexStore;
class FileIndexMonitor;
#include "dbase/fistore.h"
#include "dbase/fimonitor.h"

class FileHashSearch
{
	public:
	FileHashSearch(FileIndexStore *s, FileIndexMonitor *m)
	:store(s), monitor(m) { return; }

	~FileHashSearch() { return; }

	/* Search Interface - For FileTransfer Lookup */
	int searchLocalHash(std::string hash, std::string &path, uint32_t &size);

	int searchRemoteHash(std::string hash, std::list<FileDetail> &results);

	private:

	FileIndexStore   *store;
	FileIndexMonitor *monitor;
};

#endif
