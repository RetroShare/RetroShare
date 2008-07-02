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

#ifndef FT_FILE_EXTRA_LIST_HEADER
#define FT_FILE_EXTRA_LIST_HEADER

/* 
 * ftFileExtraList
 *
 * This maintains a list of 'Extra Files' to share with peers.
 *
 * Files are added via:
 * 1) For Files which have been hashed already:
 * 	addExtraFile(std::string path, std::string hash, uint64_t size, uint32_t period, uint32_t flags);
 *
 * 2) For Files to be hashed:
 * 	hashExtraFile(std::string path, uint32_t period, uint32_t flags);
 *
 * Results of Hashing can be retrieved via:
 * 	hashExtraFileDone(std::string path, std::string &hash, uint64_t &size);
 *
 * Files can be searched for via:
 * 	searchExtraFiles(std::string hash, ftFileDetail file);
 *
 * This Class is Mutexed protected, and has a thread in it which checks the files periodically. 
 * If a file is found to have changed... It is discarded from the list - and not updated.
 *
 * this thread is also used to hash added files.
 *
 * The list of extra files is stored using the configuration system.
 *
 */

class FileDetails
{
	public:

	std::list<std::string> sources;
	std::string path;
	std::string fname;
	std::string hash;
	uint64_t size;

	uint32_t start;
	uint32_t period;
	uint32_t flags;
};

const uint32_t FT_DETAILS_CLEANUP	= 0x0100; 	/* remove when it expires */
const uint32_t FT_DETAILS_LOCAL		= 0x0001;
const uint32_t FT_DETAILS_REMOTE	= 0x0002;

class ftExtraList: public p3Config
{

	public:

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
	/* extract entry from the queue */
	std::string path;

	{
		RsStackMutex stack(extMutex);
		path = mToHash.front();
		mToHash.pop_front();
	}

	/* hash it! */
	if (hashFile(path, details))
	{
		/* stick it in the available queue */
		addExtraFile(path, hash, size, period, flags);

		/* add to the path->hash map */
		addNewlyHashed(path, details);
	}
}


		/***
		 * If the File is alreay Hashed, then just add it in.
		 **/

bool	ftExtraList::addExtraFile(std::string path, std::string hash, 
				uint64_t size, uint32_t period, uint32_t flags)
{
	RsStackMutex stack(extMutex);


}

	
bool	ftExtraList::cleanupOldFiles()
{
	RsStackMutex stack(extMutex);

	std::list<std::string> toRemove;
	std::list<std::string>::iterator rit;

	std::map<std::string, FileDetails>::iterator it;
	for(it = mFiles.begin(); it != mFiles.end(); it++)
	{
		/* check timestamps */
		if (it->
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
}


		/***
		 * Hash file, and add to the files, 
		 * file is removed after period.
		 **/

bool 	ftExtraList::hashExtraFile(std::string path, uint32_t period, uint32_t flags)
{
	/* add into queue */
	RsStackMutex stack(extMutex);

	FileDetails details(path, period, flags);
	mToHash.push_back(details);

	return true;
}

bool	ftExtraList::hashExtraFileDone(std::string path, FileDetails &details)
{
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
	return searchExtraFiles(hash, details);
}

	/***
	 * Search Function - used by File Transfer 
	 *
	 **/
bool	ftExtraList::searchExtraFiles(std::string hash, FileDetails &details)
{
	RsStackMutex stack(extMutex);

	/* find hash */
	std::map<std::string, FileDetails>::iterator fit;
	if (mFiles.end() == (fit = mFiles.find(hash)))
	{
		return false;
	}

	details = fit->second;
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

