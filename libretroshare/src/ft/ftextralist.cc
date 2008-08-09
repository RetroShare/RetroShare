/*
 * libretroshare/src/ft: ftextralist.cc
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

#include "ft/ftextralist.h"
#include "util/rsdir.h"

#define DEBUG_ELIST	1

ftExtraList::ftExtraList()
	:p3Config(CONFIG_FT_EXTRA_LIST)
{
	return;
}


void ftExtraList::run()
{
	bool todo = false;
	time_t cleanup = 0;
	time_t now = 0;

	while (1)
	{
#ifdef  DEBUG_ELIST
		std::cerr << "ftExtraList::run() Iteration";
		std::cerr << std::endl;
#endif

		now = time(NULL);

		{
			RsStackMutex stack(extMutex);

			todo = (mToHash.size() > 0);
		}

		if (todo)
		{
			/* Hash a file */
			hashAFile();

			/* microsleep */
			usleep(10);
		}
		else
		{
			/* cleanup */
			if (cleanup < now)
			{
				cleanupOldFiles();
				cleanup = now + CLEANUP_PERIOD;
			}

			/* sleep */
			sleep(1);
		}
	}
}



void ftExtraList::hashAFile()
{
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::hashAFile()";
	std::cerr << std::endl;
#endif

	/* extract entry from the queue */
	FileDetails details;

	{
		RsStackMutex stack(extMutex);

		if (mToHash.size() == 0)
			return;

		details = mToHash.front();
		mToHash.pop_front();
	}

#ifdef  DEBUG_ELIST
	std::cerr << "Hashing: " << details.info.path;
	std::cerr << std::endl;
#endif

	/* hash it! */
	std::string name, hash;
	//uint64_t size;
	if (RsDirUtil::hashFile(details.info.path, details.info.fname, 
				details.info.hash, details.info.size))
	{
		RsStackMutex stack(extMutex);

		details.start = time(NULL);

		/* stick it in the available queue */
		mFiles[details.info.hash] = details;

		/* add to the path->hash map */
		mHashedList[details.info.path] = details.info.hash;
	}
}

		/***
		 * If the File is alreay Hashed, then just add it in.
		 **/

bool	ftExtraList::addExtraFile(std::string path, std::string hash, 
				uint64_t size, uint32_t period, uint32_t flags)
{
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::addExtraFile() path: " << path;
	std::cerr << " hash: " << hash;
	std::cerr << " size: " << size;
	std::cerr << " period: " << period;
	std::cerr << " flags: " << flags;

	std::cerr << std::endl;
#endif

	RsStackMutex stack(extMutex);

	FileDetails details;

	details.info.path = path;
	details.info.fname = RsDirUtil::getTopDir(path);
	details.info.hash = hash;
	details.info.size = size;

	details.start = time(NULL);
	details.flags = flags;
	details.period = period;

	/* stick it in the available queue */
	mFiles[details.info.hash] = details;

	return true;
}

bool ftExtraList::removeExtraFile(std::string hash, uint32_t flags)
{
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::removeExtraFile()";
	std::cerr << " hash: " << hash;
	std::cerr << " flags: " << flags;

	std::cerr << std::endl;
#endif

	RsStackMutex stack(extMutex);

	std::map<std::string, FileDetails>::iterator it;
	it = mFiles.find(hash);
	if (it == mFiles.end())
	{
		return false;
	}

	mFiles.erase(it);

	return true;
}


	
bool	ftExtraList::cleanupOldFiles()
{
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::cleanupOldFiles()";
	std::cerr << std::endl;
#endif

	RsStackMutex stack(extMutex);

	time_t now = time(NULL);

	std::list<std::string> toRemove;
	std::list<std::string>::iterator rit;

	std::map<std::string, FileDetails>::iterator it;
	for(it = mFiles.begin(); it != mFiles.end(); it++)
	{
		/* check timestamps */
		if (it->second.start + it->second.period < (unsigned) now)
		{
			toRemove.push_back(it->first);
		}
	}

	if (toRemove.size() > 0)
	{
		/* remove items */
		for(rit = toRemove.begin(); rit != toRemove.end(); rit++)
		{
			if (mFiles.end() != (it = mFiles.find(*rit)))
			{
				mFiles.erase(it);
			}
		}
	}
	return true;
}


		/***
		 * Hash file, and add to the files, 
		 * file is removed after period.
		 **/

bool 	ftExtraList::hashExtraFile(std::string path, uint32_t period, uint32_t flags)
{
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::hashExtraFile() path: " << path;
	std::cerr << " period: " << period;
	std::cerr << " flags: " << flags;

	std::cerr << std::endl;
#endif

	/* add into queue */
	RsStackMutex stack(extMutex);

	FileDetails details(path, period, flags);
	mToHash.push_back(details);

	return true;
}

bool	ftExtraList::hashExtraFileDone(std::string path, FileInfo &info)
{
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::hashExtraFileDone()";
	std::cerr << std::endl;
#endif

	std::string hash;
	{
		/* Find in the path->hash map */
		RsStackMutex stack(extMutex);

		std::map<std::string, std::string>::iterator it;
		if (mHashedList.end() == (it = mHashedList.find(path)))
		{
			return false;
		}
		hash = it->second;
	}
	return search(hash, 0, 0, info);
}

	/***
	 * Search Function - used by File Transfer 
	 *
	 **/
bool    ftExtraList::search(std::string hash, uint64_t size, uint32_t hintflags, FileInfo &info) const
{

#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::search()";
	std::cerr << std::endl;
#endif

	RsStackMutex stack(extMutex);

	/* find hash */
	std::map<std::string, FileDetails>::const_iterator fit;
	if (mFiles.end() == (fit = mFiles.find(hash)))
	{
		return false;
	}

	info = fit->second.info;
	return true;
}


	/***
	 * Configuration - store extra files.
	 *
	 **/

RsSerialiser *ftExtraList::setupSerialiser()
{
	return NULL;
}

std::list<RsItem *> ftExtraList::saveList(bool &cleanup)
{
	std::list<RsItem *> sList;
	return sList;
}

bool    ftExtraList::loadList(std::list<RsItem *> load)
{
	return true;
}

