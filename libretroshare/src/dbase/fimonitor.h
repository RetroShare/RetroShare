/*
 * RetroShare FileCache Module: fimonitor.h
 *
 * Copyright 2004-2007 by Robert Fernie.
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

#ifndef FILE_INDEX_MONITOR_H
#define FILE_INDEX_MONITOR_H

#include "dbase/cachestrapper.h"
#include "dbase/findex.h"
#include "util/rsthreads.h"
#include "rsiface/rsfiles.h"

/******************************************************************************************
 * The Local Monitoring Class: FileIndexMonitor.
 *
 * This periodically scans the directory tree, and updates any modified directories/files.
 *
 *****************************************************************************************/

/******************************************************************************************
  STILL TODO:

  (1) Implement Hash function.

bool 	FileIndexMonitor::hashFile(std::string path, FileEntry &fi);

  (2) Add Shared directory controls to Monitor.

int   FileIndexMonitor::addSharedDirectory(std::path);
int   FileIndexMonitor::removeSharedDirectory(std::path);
std::string FileIndexMonitor::findRealRoot(std::string base);

	These must be split into <base>/<top> and the mapping saved.
	eg:  addSharedDirectory("c:/home/stuff/dir1") --> "c:/home/stuff" <-> "dir1"
 	This code has been written already, and can just be moved over.

  FOR LATER:
  (2) Port File/Directory lookup code to windoze. (or compile dirent.c under windoze)
  (3) Add Load/Store interface to FileIndexMonitor. (later)
  (4) Integrate with real Thread/Mutex code (last thing to do)

******************************************************************************************/

class NotifyBase ;

class DirContentToHash
{
	public:
		std::vector<FileEntry> fentries ;

		std::string realpath ;
		std::string dirpath ;
};

/******************************************************************************************
 * FileIndexMonitor
 *****************************************************************************************/

class FileIndexMonitor: public CacheSource, public RsThread
{
	public:
		FileIndexMonitor(CacheStrapper *cs, NotifyBase *cb_in, std::string cachedir, std::string pid);
		virtual ~FileIndexMonitor();

		/* external interface for filetransfer */
		bool findLocalFile(std::string hash,uint32_t f, std::string &fullpath, uint64_t &size) const;
		int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,uint32_t flags) ;
		int SearchBoolExp(Expression *exp, std::list<DirDetails> &results,uint32_t flags) const ;
		int filterResults(std::list<FileEntry*>& firesults,std::list<DirDetails>& results,uint32_t flags) const ;


		/* external interface for local access to files */
		bool    convertSharedFilePath(std::string path, std::string &fullpath);


		/* Interacting with CacheSource */
		/* overloaded from CacheSource */
		virtual bool loadLocalCache(const CacheData &data);  /* called with stored data */
		bool 	updateCache(const CacheData &data);     /* we call when we have a new cache for others */


		/* the FileIndexMonitor inner workings */
		//virtual void 	run(std::string& currentJob); /* overloaded from RsThread */
		//void 	updateCycle(std::string& currentJob);
		virtual void 	run(); /* overloaded from RsThread */
		void 	updateCycle();

		// Interface for browsing dir hirarchy
		int RequestDirDetails(void*, DirDetails&, uint32_t) const ;
		int RequestDirDetails(std::string uid, std::string path, DirDetails &details) const ;

		// set/update shared directories
		virtual void    setSharedDirectories(std::list<SharedDirInfo> dirs);
		void    getSharedDirectories(std::list<SharedDirInfo>& dirs);
		void	updateShareFlags(const SharedDirInfo& info) ;

		void    setPeriod(int insecs);
		void    forceDirectoryCheck();
		bool	inDirectoryCheck();

		/* util fns */

	private:
		// saves file indexs and update the cache.
		void locked_saveFileIndexes() ;

		/* the mutex should be locked before calling... these. */
		std::string locked_findRealRoot(std::string base) const;
		void hashFiles(const std::vector<DirContentToHash>& to_hash) ;
		bool 	hashFile(std::string path, FileEntry &fi); /* To Implement */

		/* data */

		mutable RsMutex fiMutex;

		FileIndex fi;

		int updatePeriod;
		std::map<std::string, SharedDirInfo> directoryMap; /* used by findRealRoot */

		/* flags to kick - if we were busy or sleeping */
		bool pendingDirs;
		bool pendingForceCacheWrite;

		/* flags to force Check, to tell if we're in check */
		bool mForceCheck;
		bool mInCheck;

		std::list<SharedDirInfo> pendingDirList;
		bool    internal_setSharedDirectories();

		NotifyBase *cb ;
};


#endif


