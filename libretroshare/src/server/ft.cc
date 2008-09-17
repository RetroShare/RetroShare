/*
 * "$Id: pqifiler.cc,v 1.13 2007-02-19 20:08:30 rmf24 Exp $"
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


#include "server/ft.h"

/****
 * #define FT_DEBUG 1
 ***/


bool	ftManager::lookupLocalHash(std::string hash, std::string &path, uint64_t &size)
{
	std::list<FileDetail> details;

#ifdef FT_DEBUG 
	std::cerr << "ftManager::lookupLocalHash() hash: " << hash << std::endl;
#endif

	if (FindCacheFile(hash, path, size))
	{
		/* got it from the CacheTransfer() */
#ifdef FT_DEBUG 
		std::cerr << "ftManager::lookupLocalHash() Found in CacheStrapper:";
		std::cerr << path << " size: " << size << std::endl;
#endif

		return true;
	}

	bool ok = false;
	if (fhs)
	{
		ok = (0 != fhs -> searchLocalHash(hash, path, size));
	}
	else
	{
#ifdef FT_DEBUG 
		std::cerr << "Warning FileHashSearch is Invalid" << std::endl;
#endif
	}

	if (ok)
	{
#ifdef FT_DEBUG 
		std::cerr << "ftManager::lookupLocalHash() Found in FileHashSearch:";
		std::cerr << path << " size: " << size << std::endl;
#endif
		return true;
	}
	return ok;

}

		

bool	ftManager::lookupRemoteHash(std::string hash, std::list<std::string> &ids)
{
	std::list<FileDetail> details;
	std::list<FileDetail>::iterator it;

#ifdef FT_DEBUG 
	std::cerr << "ftManager::lookupRemoteHash() hash: " << hash << std::endl;
#endif

	if (fhs)
	{
		fhs -> searchRemoteHash(hash, details);
	}
	else
	{
#ifdef FT_DEBUG 
		std::cerr << "Warning FileHashSearch is Invalid" << std::endl;
#endif
	}

	if (details.size() == 0)
	{
#ifdef FT_DEBUG 
		std::cerr << "ftManager::lookupRemoteHash() Not Found!" << std::endl;
#endif
		return false;
	}

	for(it = details.begin(); it != details.end(); it++)
	{
#ifdef FT_DEBUG 
		std::cerr << "ftManager::lookupRemoteHash() Found in FileHashSearch:";
		std::cerr << " id: " << it->id << std::endl;
#endif
		ids.push_back(it->id);
	}
	return true;
}


