/*
 * libretroshare/src/ft: ftextralist.h
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

		ftExtraList();

		/***
		 * If the File is alreay Hashed, then just add it in.
		 **/

bool		addExtraFile(std::string path, std::string hash, 
				uint64_t size, uint32_t period, uint32_t flags);

		/***
		 * Hash file, and add to the files, 
		 * file is removed after period.
		 **/

bool 		hashExtraFile(std::string path, uint32_t period, uint32_t flags);
bool	 	hashExtraFileDone(std::string path, FileDetails &details);

		/***
		 * Search Function - used by File Transfer 
		 *
		 **/
bool		searchExtraFiles(std::string hash, FileDetails &details);

		/***
		 * Configuration - store extra files.
		 *
		 **/
        protected:
virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool    loadList(std::list<RsItem *> load);

	private:

		RsMutex extMutex;

		std::map<std::string, std::string> hashedList; /* path -> hash ( not saved ) */
		std::map<std::string, FileDetails> files;
};




