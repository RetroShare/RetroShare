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



/******************************************************************************************
 * FileIndexMonitor
 *****************************************************************************************/

class FileIndexMonitor: public CacheSource, public RsThread
{
	public:
	FileIndexMonitor(CacheStrapper *cs, std::string cachedir, std::string pid);
virtual ~FileIndexMonitor();

	/* external interface for filetransfer */
bool    findLocalFile(std::string hash, std::string &fullpath, uint64_t &size);

	/* external interface for local access to files */
bool    convertSharedFilePath(std::string path, std::string &fullpath);


	/* Interacting with CacheSource */
	/* overloaded from CacheSource */
virtual bool loadLocalCache(const CacheData &data);  /* called with stored data */
bool 	updateCache(const CacheData &data);     /* we call when we have a new cache for others */


	/* the FileIndexMonitor inner workings */
virtual void 	run(); /* overloaded from RsThread */
void 	updateCycle();

void    setSharedDirectories(std::list<std::string> dirs);
void    setPeriod(int insecs);
void    forceDirectoryCheck();
bool	inDirectoryCheck();
	/* util fns */

	private:

	/* the mutex should be locked before calling... these. */
std::string findRealRoot(std::string base);        /* To Implement */
bool 	hashFile(std::string path, FileEntry &fi); /* To Implement */

	/* data */

	RsMutex fiMutex;

	FileIndex fi;

	int updatePeriod;
	std::map<std::string, std::string> directoryMap; /* used by findRealRoot */

	/* flags to kick - if we were busy or sleeping */
	bool pendingDirs;
	bool pendingForceCacheWrite;

	/* flags to force Check, to tell if we're in check */
	bool mForceCheck;
	bool mInCheck;

	std::list<std::string> pendingDirList;
bool    internal_setSharedDirectories();

};


#endif


