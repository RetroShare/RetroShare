/*
 * libretroshare/src/ft: ftdbase.cc
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

#include "ft/ftdbase.h"
#include "util/rsdir.h"


ftFiStore::ftFiStore(CacheStrapper *cs, CacheTransfer *cft, NotifyBase *cb_in,
                        RsPeerId ownid, std::string cachedir)
	:FileIndexStore(cs, cft, cb_in, ownid, cachedir)
{
	return;
}

bool ftFiStore::search(std::string hash, uint64_t size, uint32_t hintflags, FileInfo &info) const
{
	std::list<FileDetail> results;
	std::list<FileDetail>::iterator it;

	if (SearchHash(hash, results))
	{
		for(it = results.begin(); it != results.end(); it++)
		{
			if (it->size == size)
			{
				/* 
				 */

			}
		}
	}
	return false;
}

		
ftFiMonitor::ftFiMonitor(CacheStrapper *cs, std::string cachedir, std::string pid)
	:FileIndexMonitor(cs, cachedir, pid)
{
	return;
}

bool ftFiMonitor::search(std::string hash, uint64_t size, uint32_t hintflags, FileInfo &info) const
{
	uint64_t fsize;
	std::string path;

	if (findLocalFile(hash, path, fsize))
	{
		/* fill in details */

		info.size = fsize;
		info.fname = RsDirUtil::getTopDir(path);
		info.path = path;

		return true;
	}

	return false;
};

ftCacheStrapper::ftCacheStrapper(p3AuthMgr *am, p3ConnectMgr *cm)
	:CacheStrapper(am, cm)
{
	return;
}

	/* overloaded search function */
bool ftCacheStrapper::search(std::string hash, uint64_t size, uint32_t hintflags, FileInfo &info) const
{
	CacheData data;
	if (findCache(hash, data))
	{
		/* ... */
		info.size = data.size;
		info.fname = data.name;
		info.path = data.path + "/" + data.name;

		return true;
	}
	return false;
}

