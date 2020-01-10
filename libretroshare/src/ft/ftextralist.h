/*******************************************************************************
 * libretroshare/src/ft: ftextralist.h                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008 by Robert Fernie <retroshare@lunamutt.com>                   *
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

#include <list>
#include <map>
#include <string>

#include "ft/ftsearch.h"
#include "util/rsthreads.h"
#include "retroshare/rsfiles.h"
#include "pqi/p3cfgmgr.h"
#include "util/rstime.h"

class FileDetails
{
	public:
		FileDetails()
		{
			return;
		}

		FileDetails(std::string path, uint32_t /*p*/, TransferRequestFlags f)
		{
			info.path = path;
//			period = p;
			info.transfer_info_flags = f;
		}

		FileDetails(FileInfo &i, uint32_t /*p*/, TransferRequestFlags f)
		{
			info = i;
    //		period = p;
			info.transfer_info_flags = f;
		}

		FileInfo info;

#if 0   /*** WHAT IS NEEDED ***/
		std::list<std::string> sources;
		std::string path;
		std::string fname;
        RsFileHash hash;
		uint64_t size;
#endif

        //uint32_t start;
        //uint32_t period;
		//TransferRequestFlags flags;
};

const uint32_t FT_DETAILS_CLEANUP	= 0x0100; 	/* remove when it expires */
const uint32_t FT_DETAILS_LOCAL		= 0x0001;
const uint32_t FT_DETAILS_REMOTE	= 0x0002;

const uint32_t CLEANUP_PERIOD		= 600; /* 10 minutes */


class ftExtraList: public RsTickingThread, public p3Config, public ftSearch
{

public:

	ftExtraList();

	/***
		 * If the File is alreay Hashed, then just add it in.
		 **/

	bool		addExtraFile(std::string path, const RsFileHash &hash,
	                         uint64_t size, uint32_t period, TransferRequestFlags flags);

	bool		removeExtraFile(const RsFileHash& hash);
	bool 		moveExtraFile(std::string fname, const RsFileHash& hash, uint64_t size,
	                          std::string destpath);


    uint32_t    size() const { return mFiles.size() ; }

	/***
		 * Hash file, and add to the files,
		 * file is removed after period.
		 **/

	bool 		hashExtraFile(std::string path, uint32_t period, TransferRequestFlags flags);
	bool	 	hashExtraFileDone(std::string path, FileInfo &info);

	/***
		 * Search Function - used by File Transfer
		 * implementation of ftSearch.
		 *
		 **/
	virtual bool    search(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const;

    /*!
     * \brief getExtraFileList
     * 				Retrieves the list for display purposes
     */
    void getExtraFileList(std::vector<FileInfo>& files) const ;

	void threadTick() override; /// @see RsTickingThread

	/***
		 * Configuration - store extra files.
		 *
		 **/

protected:
	virtual RsSerialiser *setupSerialiser();
	virtual bool saveList(bool &cleanup, std::list<RsItem*>&);
	virtual bool    loadList(std::list<RsItem *>& load);

	static RsFileHash makeEncryptedHash(const RsFileHash& hash);

private:

	/* Worker Functions */
	void	hashAFile();
	bool	cleanupOldFiles();
	bool    cleanupEntry(std::string path, TransferRequestFlags flags);

	mutable RsMutex extMutex;

	std::list<FileDetails> mToHash;

	std::map<std::string, RsFileHash> mHashedList; /* path -> hash ( not saved ) */
	std::map<RsFileHash, FileDetails> mFiles;
	std::map<RsFileHash, RsFileHash>  mHashOfHash;	/* sha1(hash) map so as to answer requests to encrypted transfers */

	rstime_t cleanup ;
};




#endif
