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
#include "retroshare/rsfiles.h"

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


class DirContentToHash
{
	public:
		std::vector<FileEntry> fentries ;

		std::string realpath ;
		std::string dirpath ;
};

class HashCache
{
	public:
		HashCache(const std::string& save_file_name) ;

		void save() ;
		void insert(const std::string& full_path,uint64_t size,time_t time_stamp,const std::string& hash) ;
		bool find(const  std::string& full_path,uint64_t size,time_t time_stamp,std::string& hash) ;
		void clean() ;

		typedef struct 
		{
			uint64_t size ;
			uint64_t time_stamp ;
			uint64_t modf_stamp ;
			std::string hash ;
		} HashCacheInfo ;

		void setRememberHashFilesDuration(uint32_t days) { _max_cache_duration_days = days ; }
		uint32_t rememberHashFilesDuration() const { return _max_cache_duration_days ; }
		void clear() { _files.clear(); }
		bool empty() const { return _files.empty() ; }
	private:
		uint32_t _max_cache_duration_days ;	// maximum duration of un-requested cache entries
		std::map<std::string, HashCacheInfo> _files ;
		std::string _path ;
		bool _changed ;
};

/******************************************************************************************
 * FileIndexMonitor
 *****************************************************************************************/

class FileIndexMonitor: public CacheSource, public RsThread
{
	public:
		FileIndexMonitor(CacheStrapper *cs, std::string cachedir, std::string pid, const std::string& config_dir);
		virtual ~FileIndexMonitor();

		/* external interface for filetransfer */
		bool findLocalFile(std::string hash,FileSearchFlags flags,const std::string& peer_id, std::string &fullpath, uint64_t &size,FileStorageFlags& storage_flags,std::list<std::string>& parent_groups) const;

		int SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags,const std::string& peer_id) ;
		int SearchBoolExp(Expression *exp, std::list<DirDetails> &results,FileSearchFlags flags,const std::string& peer_id) const ;

		int filterResults(std::list<FileEntry*>& firesults,std::list<DirDetails>& results,FileSearchFlags flags,const std::string& peer_id) const ;


		/* external interface for local access to files */
		bool    convertSharedFilePath(std::string path, std::string &fullpath);


		/* Interacting with CacheSource */
		/* overloaded from CacheSource */
		virtual bool loadLocalCache(const RsCacheData &data);  /* called with stored data */
		bool 	updateCache(const RsCacheData &data,const std::set<std::string>& dest_peers);     /* we call when we have a new cache for others */


		/* the FileIndexMonitor inner workings */
		//virtual void 	run(std::string& currentJob); /* overloaded from RsThread */
		//void 	updateCycle(std::string& currentJob);
		virtual void 	run(); /* overloaded from RsThread */
		void 	updateCycle();

		// Interface for browsing dir hirarchy
		int RequestDirDetails(void*, DirDetails&, FileSearchFlags) const ;
		uint32_t getType(void*) const ;
		int RequestDirDetails(const std::string& path, DirDetails &details) const ;

		// set/update shared directories
		virtual void    setSharedDirectories(const std::list<SharedDirInfo>& dirs);
		void    getSharedDirectories(std::list<SharedDirInfo>& dirs);
		void	updateShareFlags(const SharedDirInfo& info) ;

		void    forceDirectoryCheck();				// Force re-sweep the directories and see what's changed
		void    forceDirListsRebuildAndSend() ; 	// Force re-build dir lists because groups have changed. Does not re-check files.
		bool	inDirectoryCheck();

		/* util fns */

		// from CacheSource
		virtual bool cachesAvailable(RsPeerId /* pid */, std::map<CacheId, RsCacheData> &ids) ;

	protected:
		// Sets/gets the duration period within which already hashed files are remembered.
		//
		void	setRememberHashFilesDuration(uint32_t days) ;
		uint32_t rememberHashFilesDuration() const ;
		void	setRememberHashFiles(bool) ;
		bool rememberHashFiles() ;
		// Remove any memory of formerly hashed files that are not shared anymore
		void   clearHashFiles() ;
		void   setPeriod(int insecs);
		int  getPeriod() const;

		bool autoCheckEnabled() const ;

	private:
		/* the mutex should be locked before calling these 3. */

		// Saves file indexs and update the cache.  Returns the name of the main
		// file index, which becomes the new reference file for mod times.
		//
		time_t locked_saveFileIndexes(bool update_cache) ;

		// Finds the share flags associated with this file entry.
		void locked_findShareFlagsAndParentGroups(FileEntry *fe,FileStorageFlags& shareflags,std::list<std::string>& parent_groups) const ;

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

		HashCache hashCache ;
		bool useHashCache ;

		std::map<RsPeerId,RsCacheData> _cache_items_per_peer ;	// stored the cache items to be sent to each peer.

		// This file is the location of the current index file. When checking for new files, we compare the modification time
		// of this file to the mod time of the files on the disk. This allows to now account for time-shift in the computer.
		//
		time_t reference_time ;	
};


#endif


