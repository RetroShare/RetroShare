/*******************************************************************************
 * libretroshare/src/ft: ftextralist.cc                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2008  Robert Fernie <retroshare@lunamutt.com>                 *
 * Copyright (C) 2018-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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

#ifdef WINDOWS_SYS
#include "util/rswin.h"
#endif

#include <retroshare/rstypes.h>
#include <retroshare/rsfiles.h>
#include "ft/ftextralist.h"
#include "rsitems/rsconfigitems.h"
#include "util/rsdir.h"
#include "util/rstime.h"
#include <stdio.h>
#include <unistd.h>		/* for (u)sleep() */
#include "util/rstime.h"

/******
 * #define DEBUG_ELIST	1
 *****/

ftExtraList::ftExtraList()
	:p3Config(), extMutex("p3Config")
{
    cleanup = 0;
    return;
}


void ftExtraList::threadTick()
{
    bool todo = false;
    rstime_t now = time(NULL);

    {
        RsStackMutex stack(extMutex);

        todo = (mToHash.size() > 0);
    }

    if (todo)
    {
        /* Hash a file */
        hashAFile();

        /* microsleep */
        rstime::rs_usleep(10);
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
#ifdef WIN32
        Sleep(1000);
#else
        sleep(1);
#endif
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
		RS_STACK_MUTEX(extMutex);

		if (mToHash.empty())
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
	if (RsDirUtil::hashFile(details.info.path, details.info.fname,  details.info.hash, details.info.size))
	{
		RS_STACK_MUTEX(extMutex);

		/* stick it in the available queue */
		mFiles[details.info.hash] = details;
        mHashOfHash[makeEncryptedHash(details.info.hash)] = details.info.hash ;

		/* add to the path->hash map */
		mHashedList[details.info.path] = details.info.hash;
	
		IndicateConfigChanged();
	}
}

		/***
		 * If the File is alreay Hashed, then just add it in.
		 **/

bool	ftExtraList::addExtraFile(std::string path, const RsFileHash& hash,
				uint64_t size, uint32_t period, TransferRequestFlags flags)
{
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::addExtraFile() path: " << path;
	std::cerr << " hash: " << hash;
	std::cerr << " size: " << size;
	std::cerr << " period: " << period;
	std::cerr << " flags: " << flags;

	std::cerr << std::endl;
#endif

	RS_STACK_MUTEX(extMutex);

	FileDetails details;

	details.info.path = path;
	details.info.fname = RsDirUtil::getTopDir(path);
	details.info.hash = hash;
	details.info.size = size;
	details.info.age = time(NULL) + period; /* if time > this... cleanup */
	details.info.transfer_info_flags = flags ;

	/* stick it in the available queue */
	mFiles[details.info.hash] = details;
	mHashOfHash[makeEncryptedHash(details.info.hash)] = details.info.hash ;

	IndicateConfigChanged();

	return true;
}

bool ftExtraList::removeExtraFile(const RsFileHash& hash)
{
	/* remove unused parameter warnings */
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::removeExtraFile()";
	std::cerr << " hash: " << hash;
	std::cerr << " flags: " << flags;

	std::cerr << std::endl;
#endif

	RS_STACK_MUTEX(extMutex);

    mHashOfHash.erase(makeEncryptedHash(hash)) ;

    std::map<RsFileHash, FileDetails>::iterator it;
	it = mFiles.find(hash);
	if (it == mFiles.end())
	{
		return false;
	}

	mFiles.erase(it);

	IndicateConfigChanged();

	return true;
}

bool ftExtraList::moveExtraFile(std::string fname, const RsFileHash &hash, uint64_t /*size*/,
                                std::string destpath)
{
	RsStackMutex stack(extMutex);

    std::map<RsFileHash, FileDetails>::iterator it;
	it = mFiles.find(hash);
	if (it == mFiles.end())
	{
		return false;
	}

	std::string path = destpath + '/' + fname;
	if (RsDirUtil::renameFile(it->second.info.path, path))
	{
		/* rename */
		it->second.info.path = path;
		it->second.info.fname = fname;
		IndicateConfigChanged();
	}

	return true;
}


	
bool	ftExtraList::cleanupOldFiles()
{
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::cleanupOldFiles()";
	std::cerr << std::endl;
#endif

	RS_STACK_MUTEX(extMutex);

	rstime_t now = time(NULL);

    std::list<RsFileHash> toRemove;

	for( std::map<RsFileHash, FileDetails>::iterator it = mFiles.begin(); it != mFiles.end(); ++it) /* check timestamps */
		if ((rstime_t)it->second.info.age < now)
			toRemove.push_back(it->first);

	if (toRemove.size() > 0)
	{
        std::map<RsFileHash, FileDetails>::iterator it;

		/* remove items */
		for(std::list<RsFileHash>::iterator rit = toRemove.begin(); rit != toRemove.end(); ++rit)
        {
			if (mFiles.end() != (it = mFiles.find(*rit)))
			{
				cleanupEntry(it->second.info.path, it->second.info.transfer_info_flags);
				mFiles.erase(it);
			}
			mHashOfHash.erase(makeEncryptedHash(*rit)) ;
        }

		IndicateConfigChanged();
	}
	return true;
}


bool	ftExtraList::cleanupEntry(std::string /*path*/, TransferRequestFlags /*flags*/)
{
//	if (flags & RS_FILE_CONFIG_CLEANUP_DELETE)
//	{
//		/* Delete the file? - not yet! */
//	}
	return true;
}

		/***
		 * Hash file, and add to the files, 
		 * file is removed after period.
		 **/

bool ftExtraList::hashExtraFile(
        std::string path, uint32_t period, TransferRequestFlags flags )
{
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::hashExtraFile() path: " << path;
	std::cerr << " period: " << period;
	std::cerr << " flags: " << flags;

	std::cerr << std::endl;
#endif

	auto failure = [](std::string errMsg)
	{
		RsErr() << __PRETTY_FUNCTION__ << " " << errMsg << std::endl;
		return false;
	};

	if(!RsDirUtil::fileExists(path))
		return failure("file: " + path + "not found");

	if(RsDirUtil::checkDirectory(path))
		return failure("Cannot add a directory: " + path + "as extra file");

	FileDetails details(path, period, flags);
	details.info.age = static_cast<int>(time(nullptr) + period);

	{
		RS_STACK_MUTEX(extMutex);
		mToHash.push_back(details); /* add into queue */
	}

	return true;
}

bool	ftExtraList::hashExtraFileDone(std::string path, FileInfo &info)
{
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::hashExtraFileDone()";
	std::cerr << std::endl;
#endif

    RsFileHash hash;
	{
		/* Find in the path->hash map */
		RS_STACK_MUTEX(extMutex);

        std::map<std::string, RsFileHash>::iterator it;
		if (mHashedList.end() == (it = mHashedList.find(path)))
		{
			return false;
		}
		hash = it->second;
	}
	return search(hash, FileSearchFlags(0), info);
}

	/***
	 * Search Function - used by File Transfer 
	 *
	 **/
bool    ftExtraList::search(const RsFileHash &hash, FileSearchFlags /*hintflags*/, FileInfo &info) const
{
#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::search() hash=" << hash ;
#endif

	/* find hash */
    std::map<RsFileHash, FileDetails>::const_iterator fit;
	if (mFiles.end() == (fit = mFiles.find(hash)))
	{
#ifdef  DEBUG_ELIST
        std::cerr << "  not found in mFiles. Trying encrypted... " ;
#endif
        // File not found. We try to look for encrypted hash.

        std::map<RsFileHash,RsFileHash>::const_iterator hit = mHashOfHash.find(hash) ;

        if(hit == mHashOfHash.end())
        {
#ifdef  DEBUG_ELIST
			std::cerr << "  not found." << std::endl;
#endif
			return false;
        }
#ifdef  DEBUG_ELIST
		std::cerr << "  found! Reaching data..." ;
#endif

        fit = mFiles.find(hit->second) ;

        if(fit == mFiles.end())		// not found. This is an error.
        {
#ifdef  DEBUG_ELIST
			std::cerr << "  no data. Returning false." << std::endl;
#endif
            return false ;
        }

#ifdef  DEBUG_ELIST
		std::cerr << "  ok! Accepting encrypted transfer." << std::endl;
#endif
		info = fit->second.info;
		info.storage_permission_flags = FileStorageFlags(DIR_FLAGS_ANONYMOUS_DOWNLOAD) ;
		info.transfer_info_flags |= RS_FILE_REQ_ENCRYPTED ;
	}
    else
    {
#ifdef  DEBUG_ELIST
        std::cerr << "  found! Accepting direct transfer" << std::endl;
#endif
		info = fit->second.info;

        // Unencrypted file transfer: We only allow direct transfers. This is not exactly secure since another friend can
        // swarm the file. But the hash being kept secret, there's no risk here.
		//
		info.storage_permission_flags = FileStorageFlags(DIR_FLAGS_BROWSABLE) ;
    }

	if(info.transfer_info_flags & RS_FILE_REQ_ANONYMOUS_ROUTING) info.storage_permission_flags |= DIR_FLAGS_ANONYMOUS_DOWNLOAD ;

	return true;
}

RsFileHash ftExtraList::makeEncryptedHash(const RsFileHash& hash)
{
	return RsDirUtil::sha1sum(hash.toByteArray(),hash.SIZE_IN_BYTES);
}

	/***
	 * Configuration - store extra files.
	 *
	 **/

RsSerialiser *ftExtraList::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();

	/* add in the types we need! */
	rss->addSerialType(new RsFileConfigSerialiser());
	return rss;
}

bool ftExtraList::saveList(bool &cleanup, std::list<RsItem *>& sList)
{


	cleanup = true;

	/* called after each item is added */

	/* create a list of fileitems with
	 * age used to specify its timeout.
	 */

#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::saveList()";
	std::cerr << std::endl;
#endif

	RS_STACK_MUTEX(extMutex);


    std::map<RsFileHash, FileDetails>::const_iterator it;
	for(it = mFiles.begin(); it != mFiles.end(); ++it)
	{
		RsFileConfigItem *fi = new RsFileConfigItem();

		fi->file.path        = (it->second).info.path;
		fi->file.name        = (it->second).info.fname;
		fi->file.hash        = (it->second).info.hash;
		fi->file.filesize    = (it->second).info.size;
		fi->file.age         = (it->second).info.age;
		fi->flags            = (it->second).info.transfer_info_flags.toUInt32();

		sList.push_back(fi);
	}

	return true;
}


bool    ftExtraList::loadList(std::list<RsItem *>& load)
{
	/* for each item, check it exists .... 
	 * - remove any that are dead (or flag?) 
	 */

#ifdef  DEBUG_ELIST
	std::cerr << "ftExtraList::loadList()";
	std::cerr << std::endl;
#endif

	rstime_t ts = time(NULL);


	std::list<RsItem *>::iterator it;
	for(it = load.begin(); it != load.end(); ++it)
	{

		RsFileConfigItem *fi = dynamic_cast<RsFileConfigItem *>(*it);
		if (!fi)
		{
			delete (*it);
			continue;
		}

		/* open file */
		FILE *fd = RsDirUtil::rs_fopen(fi->file.path.c_str(), "rb");
		if (fd == NULL)
		{
			delete (*it);
			continue;
		}

		fclose(fd);
		fd = NULL ;

		if (ts > (rstime_t)fi->file.age)
		{
			/* to old */
			cleanupEntry(fi->file.path, TransferRequestFlags(fi->flags));
			delete (*it);
			continue ;
		}

		/* add into system */
		FileDetails file;

		RS_STACK_MUTEX(extMutex);

		FileDetails details;
	
		details.info.path = fi->file.path;
		details.info.fname = fi->file.name;
		details.info.hash = fi->file.hash;
		details.info.size = fi->file.filesize;
		details.info.age = fi->file.age; /* time that we remove it. */
		details.info.transfer_info_flags = TransferRequestFlags(fi->flags);
	
		/* stick it in the available queue */
		mFiles[details.info.hash] = details;
        mHashOfHash[makeEncryptedHash(details.info.hash)] = details.info.hash ;

		delete (*it);

		/* short sleep */
		rstime::rs_usleep(1000) ;
	}
    load.clear() ;
	return true;
}

void ftExtraList::getExtraFileList(std::vector<FileInfo>& files) const
{
	RS_STACK_MUTEX(extMutex);

    files.clear();

    for(auto it(mFiles.begin());it!=mFiles.end();++it)
        files.push_back(it->second.info);
}
