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

bool	ftManager::lookupLocalHash(std::string hash, std::string &path, uint32_t &size)
{
	std::list<FileDetail> details;

	std::cerr << "ftManager::lookupLocalHash() hash: " << hash << std::endl;

	if (FindCacheFile(hash, path, size))
	{
		/* got it from the CacheTransfer() */
		std::cerr << "ftManager::lookupLocalHash() Found in CacheStrapper:";
		std::cerr << path << " size: " << size << std::endl;

		return true;
	}

	bool ok = false;
	if (fhs)
	{
		ok = (0 != fhs -> searchLocalHash(hash, path, size));
	}
	else
	{
		std::cerr << "Warning FileHashSearch is Invalid" << std::endl;
	}

	if (ok)
	{
		std::cerr << "ftManager::lookupLocalHash() Found in FileHashSearch:";
		std::cerr << path << " size: " << size << std::endl;
		return true;
	}
	return ok;

}

		

bool	ftManager::lookupRemoteHash(std::string hash, std::list<std::string> &ids)
{
	std::list<FileDetail> details;
	std::list<FileDetail>::iterator it;

	std::cerr << "ftManager::lookupRemoteHash() hash: " << hash << std::endl;

	if (fhs)
	{
		fhs -> searchRemoteHash(hash, details);
	}
	else
	{
		std::cerr << "Warning FileHashSearch is Invalid" << std::endl;
	}

	if (details.size() == 0)
	{
		std::cerr << "ftManager::lookupRemoteHash() Not Found!" << std::endl;
		return false;
	}

	for(it = details.begin(); it != details.end(); it++)
	{
		std::cerr << "ftManager::lookupRemoteHash() Found in FileHashSearch:";
		std::cerr << " id: " << it->id << std::endl;
		ids.push_back(it->id);
	}
	return true;
}


