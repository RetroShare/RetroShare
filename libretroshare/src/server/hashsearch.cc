/*
 * "$Id: hashsearch.cc,v 1.5 2007-02-19 20:08:30 rmf24 Exp $"
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

/**********
 * SearchInterface for the FileTransfer
 */

#include "server/hashsearch.h"
#include "dbase/fistore.h"
#include "dbase/fimonitor.h"

	/* Search Interface - For FileTransfer Lookup */
int FileHashSearch::searchLocalHash(std::string hash, std::string &path, uint64_t &size)
{
	if (monitor)
	{
		return monitor->findLocalFile(hash, path, size);
	}
	return 0;
}

int FileHashSearch::searchRemoteHash(std::string hash, std::list<FileDetail> &results)
{
	if (store)
		store->SearchHash(hash, results);
	return results.size();
}


