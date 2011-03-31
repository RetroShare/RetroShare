/*
 * RetroShare FileCache Module: fimonitor.cc
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

#ifdef WINDOWS_SYS
#include "util/rswin.h"
#endif

#include "dbase/fimonitor.h"
#include "util/rsdir.h"
#include "serialiser/rsserviceids.h"
#include "retroshare/rsiface.h"
#include "retroshare/rsnotify.h"
#include "util/folderiterator.h"
#include <errno.h>

#include <iostream>
#include <sstream>
#include <iomanip>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>

//***********
//#define FIM_DEBUG 1
// ***********/

FileIndexMonitor::FileIndexMonitor(CacheStrapper *cs, NotifyBase *cb_in,std::string cachedir, std::string pid)
	:CacheSource(RS_SERVICE_TYPE_FILE_INDEX, false, cs, cachedir), fi(pid),
		pendingDirs(false), pendingForceCacheWrite(false),
		mForceCheck(false), mInCheck(false),cb(cb_in), hashCache(cachedir+"/" + "fc-own-cache.rsfa"),useHashCache(true)

{
	updatePeriod = 60;
}

bool FileIndexMonitor::autoCheckEnabled() const
{
	RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */
	return updatePeriod > 0 ;
}

bool FileIndexMonitor::rememberHashFiles()
{
	RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */
	return useHashCache ;
}
void FileIndexMonitor::setRememberHashFiles(bool b)
{
	RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */
#ifdef FIM_DEBUG
	std::cerr << "Setting useHashCache to " << b << std::endl;
#endif
	useHashCache = b ;
}
void	FileIndexMonitor::setRememberHashFilesDuration(uint32_t days) 
{
	RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */

#ifdef FIM_DEBUG
	std::cerr << "Setting HashCache duration to " << days << std::endl;
#endif
	hashCache.setRememberHashFilesDuration(days) ;
	hashCache.clean() ;
}

uint32_t FileIndexMonitor::rememberHashFilesDuration() const 
{
	RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */
	
	return hashCache.rememberHashFilesDuration() ;
}

// Remove any memory of formerly hashed files that are not shared anymore
void   FileIndexMonitor::clearHashFiles() 
{
	RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */

	hashCache.clear() ;
	hashCache.save() ;
}

HashCache::HashCache(const std::string& path)
	: _path(path)
{
	_max_cache_duration_days = 10 ; // 10 days is the default value.
	_files.clear() ;
	_changed = false ;
	std::ifstream f(_path.c_str()) ;

	std::streamsize max_line_size = 2000 ; // should be enough. Anyway, if we
														// miss one entry, we just lose some
														// cache itemsn but this is not too
														// much of a problem.
	char *buff = new char[max_line_size] ;

#ifdef FIM_DEBUG
	std::cerr << "Loading HashCache from file " << path << std::endl ;
	int n=0 ;
#endif

	while(f.good())
	{
		HashCacheInfo info ;

		f.getline(buff,max_line_size,'\n') ;
		std::string name(buff) ;

		f.getline(buff,max_line_size,'\n') ; if(sscanf(buff,"%lld",&info.size) != 1) break ;
		f.getline(buff,max_line_size,'\n') ; if(sscanf(buff,"%ld",&info.time_stamp) != 1) break ;
		f.getline(buff,max_line_size,'\n') ; if(sscanf(buff,"%ld",&info.modf_stamp) != 1) break ;
		f.getline(buff,max_line_size,'\n') ; info.hash = std::string(buff) ;

#ifdef FIM_DEBUG
		std::cerr << "  (" << name << ", " << info.size << ", " << info.time_stamp << ", " << info.modf_stamp << ", " << info.hash << std::endl ;
		++n ;
#endif
		_files[name] = info ;
	}
	f.close() ;

	delete[] buff ;
#ifdef FIM_DEBUG
	std::cerr << n << " entries loaded." << std::endl ;
#endif
}

void HashCache::save()
{
	if(_changed)
	{
#ifdef FIM_DEBUG
		std::cerr << "Saving Hash Cache to file " << _path << "..." << std::endl ;
#endif
		std::ofstream f( (_path+".tmp").c_str() ) ;

		for(std::map<std::string,HashCacheInfo>::const_iterator it(_files.begin());it!=_files.end();++it)
		{
			f << it->first << std::endl ;
			f << it->second.size << std::endl;
			f << it->second.time_stamp << std::endl;
			f << it->second.modf_stamp << std::endl;
			f << it->second.hash << std::endl;
		}
		f.close() ;

		RsDirUtil::renameFile(_path+".tmp",_path) ;
#ifdef FIM_DEBUG
		std::cerr << "done." << std::endl ;
#endif

		_changed = false ;
	}
#ifdef FIM_DEBUG
	else
		std::cerr << "Hash cache not changed. Not saving." << std::endl ;
#endif
}

bool HashCache::find(const std::string& full_path,uint64_t size,time_t time_stamp,std::string& hash)
{
#ifdef FIM_DEBUG
	std::cerr << "HashCache: looking for " << full_path << std::endl ;
#endif
	time_t now = time(NULL) ;
	std::map<std::string,HashCacheInfo>::iterator it(_files.find(full_path)) ;

	if(it != _files.end() && time_stamp == it->second.modf_stamp && size == it->second.size)
	{
		hash = it->second.hash ;
		it->second.time_stamp = now ;
#ifdef FIM_DEBUG
		std::cerr << "Found in cache." << std::endl ;
#endif
		return true ;
	}
	else
	{
#ifdef FIM_DEBUG
		std::cerr << "not found in cache." << std::endl ;
#endif
		return false ;
	}
}
void HashCache::insert(const std::string& full_path,uint64_t size,time_t time_stamp,const std::string& hash)
{
	HashCacheInfo info ;
	info.size = size ;
	info.modf_stamp = time_stamp ;
	info.time_stamp = time(NULL) ;
	info.hash = hash ;

	_files[full_path] = info ;
	_changed = true ;

#ifdef FIM_DEBUG
	std::cerr << "Entry inserted in cache: " << full_path << ", " << size << ", " << time_stamp << std::endl ;
#endif
}
void HashCache::clean()
{
#ifdef FIM_DEBUG
	std::cerr << "Cleaning HashCache..." << std::endl ;
#endif
	time_t now = time(NULL) ;
	time_t duration = _max_cache_duration_days * 24 * 3600 ; // seconds

#ifdef FIM_DEBUG
	std::cerr << "cleaning hash cache." << std::endl ;
#endif

	for(std::map<std::string,HashCacheInfo>::iterator it(_files.begin());it!=_files.end();)
		if(it->second.time_stamp + duration < now)
		{
#ifdef FIM_DEBUG
			std::cerr << "  Entry too old: " << it->first << ", ts=" << it->second.time_stamp << std::endl ;
#endif
			std::map<std::string,HashCacheInfo>::iterator tmp(it) ;
			++tmp ;
			_files.erase(it) ;
			it=tmp ;
		}
		else
			++it ;

#ifdef FIM_DEBUG
	std::cerr << "Done." << std::endl;
#endif
}

FileIndexMonitor::~FileIndexMonitor()
{
	/* Data cleanup - TODO */
}

int FileIndexMonitor::SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,uint32_t flags)
{
	results.clear();
	std::list<FileEntry *> firesults;

	{ 
		RsStackMutex stackM(fiMutex) ;/* LOCKED DIRS */
		fi.searchTerms(keywords, firesults);
	}

	return filterResults(firesults,results,flags) ;
}

int FileIndexMonitor::SearchBoolExp(Expression *exp, std::list<DirDetails>& results,uint32_t flags) const
{
	results.clear();
	std::list<FileEntry *> firesults;

	{ 
		RsStackMutex stackM(fiMutex) ;/* LOCKED DIRS */
		fi.searchBoolExp(exp, firesults);
	}

	return filterResults(firesults,results,flags) ;
}

int FileIndexMonitor::filterResults(std::list<FileEntry*>& firesults,std::list<DirDetails>& results,uint32_t flags) const
{
	/* translate/filter results */

	for(std::list<FileEntry*>::const_iterator rit(firesults.begin()); rit != firesults.end(); ++rit)
	{
		DirDetails cdetails ;
		RequestDirDetails (*rit,cdetails,0);
#ifdef FIM_DEBUG
		std::cerr << "Filtering candidate " << (*rit)->name  << ", flags=" << cdetails.flags ;
#endif

		if (cdetails.type == DIR_TYPE_FILE && ( cdetails.flags & flags & (DIR_FLAGS_BROWSABLE | DIR_FLAGS_NETWORK_WIDE)) > 0) 
		{
			cdetails.id = "Local";
			results.push_back(cdetails);
#ifdef FIM_DEBUG
			std::cerr << ": kept" << std::endl ;
#endif
		}
#ifdef FIM_DEBUG
		else
			std::cerr << ": discarded" << std::endl ;
#endif
	}
	return !results.empty() ;
}

bool FileIndexMonitor::findLocalFile(std::string hash,uint32_t hint_flags, std::string &fullpath, uint64_t &size) const
{
	std::list<FileEntry *> results;
	bool ok = false;

	{ 
		RsStackMutex stackM(fiMutex) ;/* LOCKED DIRS */

#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::findLocalFile() Hash: " << hash << std::endl;
#endif
		/* search through the fileIndex */
		fi.searchHash(hash, results);

		if (results.size() > 0)
		{
			/* find the full path for the first entry */
			FileEntry *fe = results.front();
			DirEntry  *de = fe->parent; /* all files must have a valid parent! */

			uint32_t share_flags = locked_findShareFlags(fe) ;
#ifdef FIM_DEBUG
			std::cerr << "FileIndexMonitor::findLocalFile: Filtering candidate " << fe->name  << ", flags=" << share_flags << ", hint_flags=" << hint_flags << std::endl ;
#endif

			if((share_flags & hint_flags & (RS_FILE_HINTS_BROWSABLE | RS_FILE_HINTS_NETWORK_WIDE)) > 0) 
			{
#ifdef FIM_DEBUG
				std::cerr << "FileIndexMonitor::findLocalFile() Found Name: " << fe->name << std::endl;
#endif
				std::string shpath =  RsDirUtil::removeRootDir(de->path);
				std::string basedir = RsDirUtil::getRootDir(de->path);
				std::string realroot = locked_findRealRoot(basedir);

				/* construct full name */
				if (realroot.length() > 0)
				{
					fullpath = realroot + "/";
					if (shpath != "")
					{
						fullpath += shpath + "/";
					}
					fullpath += fe->name;

					size = fe->size;
					ok = true;
				}
#ifdef FIM_DEBUG
				std::cerr << "FileIndexMonitor::findLocalFile() Found Path: " << fullpath << std::endl;
				std::cerr << "FileIndexMonitor::findLocalFile() Found Size: " << size << std::endl;
#endif
			}
#ifdef FIM_DEBUG
			else
				std::cerr << "FileIndexMonitor::findLocalFile() discarded" << std::endl ;
#endif
		}
	}  /* UNLOCKED DIRS */

	return ok;
}

bool    FileIndexMonitor::convertSharedFilePath(std::string path, std::string &fullpath)
{
	bool ok = false;

	{ 
		RsStackMutex stackM(fiMutex) ;/* LOCKED DIRS */

#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::convertSharedFilePath() path: " << path << std::endl;
#endif

		std::string shpath =  RsDirUtil::removeRootDir(path);
		std::string basedir = RsDirUtil::getRootDir(path);
		std::string realroot = locked_findRealRoot(basedir);

		/* construct full name */
		if (realroot.length() > 0)
		{
			fullpath = realroot + "/";
			fullpath += shpath;
#ifdef FIM_DEBUG
			std::cerr << "FileIndexMonitor::convertSharedFilePath() Found Path: " << fullpath << std::endl;
#endif
			ok = true;
		}

	} /* UNLOCKED DIRS */

	return ok;
}


bool FileIndexMonitor::loadLocalCache(const CacheData &data)  /* called with stored data */
{
	bool ok = false;

	{ 
		RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */

		/* More error checking needed here! */

		std::string name = data.name ;	// this trick allows to load the complete file. Not the one being shared.
		name[name.length()-1] = 'c' ;

		if ((ok = fi.loadIndex(data.path + '/' + name, "", data.size)))
		{
#ifdef FIM_DEBUG
			std::cerr << "FileIndexMonitor::loadCache() Success!";
			std::cerr << std::endl;
#endif
			fi.root->row = 0;
			fi.root->name = data.pname ;
		}
		else
		{
#ifdef FIM_DEBUG
			std::cerr << "FileIndexMonitor::loadCache() Failed!";
			std::cerr << std::endl;
#endif
		}

		fi.updateMaxModTime() ;
	}

	if (ok)
	{
		return updateCache(data);
	}
	return false;
}

bool FileIndexMonitor::updateCache(const CacheData &data)  /* we call this one */
{
	return refreshCache(data);
}


int	FileIndexMonitor::getPeriod() const
{
//#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::setPeriod() getting watch period" << std::endl;
//#endif
	RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */
	return updatePeriod ;
}

void 	FileIndexMonitor::setPeriod(int period)
{
	RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */
	updatePeriod = period;
//#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::setPeriod() Setting watch period to " << updatePeriod << std::endl;
//#endif
}

void 	FileIndexMonitor::run()
{
	if(autoCheckEnabled())
		updateCycle();

	while(isRunning())
	{
		int i=0 ;
		for(;;i++)
		{
			if(!isRunning()) 
				return;

			/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
			sleep(1);
#else
			Sleep(1000);
#endif
			/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

			/* check dirs if they've changed */
			if (internal_setSharedDirectories())
				break;

			{
				RsStackMutex mtx(fiMutex) ;

				if(i >= abs(updatePeriod))
					break  ;
			}
		}

		if(i < abs(updatePeriod) || autoCheckEnabled())
			updateCycle();

#ifdef FIM_DEBUG
		{
			RsStackMutex mtx(fiMutex) ;
			std::cerr <<"*********** FileIndex **************" << std::endl ;
			fi.printFileIndex(std::cerr) ;
			std::cerr <<"************** END *****************" << std::endl ;
			std::cerr << std::endl ;
		}
#endif
	}
}

//void 	FileIndexMonitor::updateCycle(std::string& current_job)
void 	FileIndexMonitor::updateCycle()
{
	time_t startstamp = time(NULL);

	/* iterate through all out-of-date directories */
	bool moretodo = true;
	bool fiMods = false;

	{
		RsStackMutex stack(fiMutex); /**** LOCKED DIRS ****/
		mInCheck = true;
	}

	cb->notifyHashingInfo(NOTIFY_HASHTYPE_EXAMINING_FILES, "") ;

	std::vector<DirContentToHash> to_hash ;

	bool cache_is_new ;
	{
		RsStackMutex mtx(fiMutex) ;
		cache_is_new = useHashCache && hashCache.empty() ;
	}
			
	while(isRunning() && moretodo)
	{
		/* sleep a bit for each loop */
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
		usleep(100000); /* 1/10 sec */
#else
		Sleep(100);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

		/* check if directories have been updated */
		if (internal_setSharedDirectories()) /* reset start time */
			startstamp = time(NULL);

		/* Handle a Single out-of-date directory */

		time_t stamp = time(NULL);

		/* lock dirs from now on */
		RsStackMutex mtx(fiMutex) ;

		DirEntry *olddir = fi.findOldDirectory(startstamp);

		if (!olddir)
		{
			/* finished */
			moretodo = false;
			continue;
		}

#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::updateCycle()";
		std::cerr << " Checking: " << olddir->path << std::endl;
#endif

		FileEntry fe;
		/* entries that need to be checked properly */
		std::list<FileEntry>::iterator hit;

		/* determine the full root path */
		std::string dirpath  = olddir->path;
		std::string rootdir  = RsDirUtil::getRootDir(olddir->path);
		std::string remdir   = RsDirUtil::removeRootDir(olddir->path);
		std::string realroot = locked_findRealRoot(rootdir);
		std::string realpath = realroot;

		if (remdir != "")
			realpath += "/" + remdir;

#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::updateCycle()";
		std::cerr << " RealPath: " << realpath << std::endl;
#endif

		/* check for the dir existance */
		librs::util::FolderIterator dirIt(realpath);
		if (!dirIt.isValid())
		{
#ifdef FIM_DEBUG
			std::cerr << "FileIndexMonitor::updateCycle()";
			std::cerr << " Missing Dir: " << realpath << std::endl;
#endif
			/* bad directory - delete */
			if (!fi.removeOldDirectory(olddir->parent->path, olddir->name, stamp))
			{
				/* bad... drop out of updateCycle() - hopefully the initial cleanup
				 * will deal with it next time! - otherwise we're in a continual loop
				 */
				std::cerr << "FileIndexMonitor::updateCycle()";
				std::cerr << "ERROR Failed to Remove: " << olddir->path << std::endl;
			}
			continue;
		}

		/* update this dir - as its valid */
		fe.name = olddir->name;
		fi.updateDirEntry(olddir->parent->path, fe, stamp);

		/* update the directories and files here */
		std::map<std::string, DirEntry *>::iterator  dit;
		std::map<std::string, FileEntry *>::iterator fit;

		/* flag existing subdirs as old */
		for(dit = olddir->subdirs.begin(); dit != olddir->subdirs.end(); dit++)
		{
			fe.name = (dit->second)->name;
			/* set the age as out-of-date so that it gets checked */
			fi.updateDirEntry(olddir->path, fe, 0);
		}

		/* now iterate through the directory...
		 * directories - flags as old,
		 * files checked to see if they have changed. (rehashed)
		 */

		struct stat64 buf;

		to_hash.push_back(DirContentToHash()) ;
		to_hash.back().realpath = realpath ;
		to_hash.back().dirpath = dirpath ;

		while(isRunning() && dirIt.readdir())
		{
			/* check entry type */
			std::string fname;
			dirIt.d_name(fname);
			std::string fullname = realpath + "/" + fname;
#ifdef FIM_DEBUG
			std::cerr << "calling stats on " << fullname <<std::endl;
#endif

#ifdef WINDOWS_SYS
			std::wstring wfullname;
			librs::util::ConvertUtf8ToUtf16(fullname, wfullname);
			if (-1 != _wstati64(wfullname.c_str(), &buf))
#else
			if (-1 != stat64(fullname.c_str(), &buf))
#endif
			{
#ifdef FIM_DEBUG
				std::cerr << "buf.st_mode: " << buf.st_mode <<std::endl;
#endif
				if (S_ISDIR(buf.st_mode))
				{
					if ((fname == ".") || (fname == ".."))
					{
#ifdef FIM_DEBUG
						std::cerr << "Skipping:" << fname << std::endl;
#endif
						continue; /* skipping links */
					}

#ifdef FIM_DEBUG
					std::cerr << "Is Directory: " << fullname << std::endl;
#endif

					/* add in directory */
					fe.name = fname;
					/* set the age as out-of-date so that it gets checked */
					fi.updateDirEntry(olddir->path, fe, 0);
				}
				else if (S_ISREG(buf.st_mode))
				{
					/* is file */
					bool toadd = false;
#ifdef FIM_DEBUG
					std::cerr << "Is File: " << fullname << std::endl;
#endif

					fe.name = fname;
					fe.size = buf.st_size;
					fe.modtime = buf.st_mtime;

					/* check if it exists already */
					fit = olddir->files.find(fname);
					if (fit == olddir->files.end())
					{
						/* needs to be added */
#ifdef FIM_DEBUG
						std::cerr << "File Missing from List:" << fname << std::endl;
#endif
						toadd = true;
					}
					else
					{
						/* check size / modtime are the same */
						if ((fe.size != (fit->second)->size) || (fe.modtime != (fit->second)->modtime))
						{
#ifdef FIM_DEBUG
							std::cerr << "File ModTime/Size changed:" << fname << std::endl;
							std::cerr << "fe.modtime = " << fe.modtime << std::endl;
							std::cerr << "fit.mdtime = " << fit->second->modtime << std::endl;
#endif
							toadd = true;
						}
						else
							fe.hash = (fit->second)->hash; /* keep old info */
					}
					if (toadd)
					{
						/* push onto Hash List */
#ifdef FIM_DEBUG
						std::cerr << "Adding to Update List: ";
						std::cerr << olddir->path;
						std::cerr << fname << std::endl;
#endif
						to_hash.back().fentries.push_back(fe);
						fiMods = true ;
					}
					else /* update with new time */
					{
#ifdef FIM_DEBUG
						std::cerr << "File Hasn't Changed:" << fname << std::endl;
#endif
						fi.updateFileEntry(olddir->path, fe, stamp);

						if(cache_is_new)
							hashCache.insert(realpath+"/"+fe.name,fe.size,fe.modtime,fe.hash) ;
					}
				}
			}
#ifdef FIM_DEBUG
			else
				std::cout << "stat error " << errno << std::endl ;
#endif
		}

		if(to_hash.back().fentries.empty())
			to_hash.pop_back() ;

		/* now we unlock the lock, and iterate through the
		 * next files - hashing them, before adding into the system.
		 */
		/* for safety - blank out data we cannot use (TODO) */
		olddir = NULL;

		/* close directory */
		dirIt.closedir();
	}

	// Now, hash all files at once.
	//
	if(isRunning() && !to_hash.empty())
		hashFiles(to_hash) ;

	cb->notifyHashingInfo(NOTIFY_HASHTYPE_FINISH, "") ;

	{ /* LOCKED DIRS */
		RsStackMutex stack(fiMutex); /**** LOCKED DIRS ****/

		/* finished update cycle - cleanup extra dirs/files that
		 * have not had their timestamps updated.
		 */

		fi.cleanOldEntries(startstamp) ;

#ifdef FIM_DEBUG
		/* print out the new directory structure */
		fi.printFileIndex(std::cerr);
#endif
		/* now if we have changed things -> restore file/hash it/and
		 * tell the CacheSource
		 */

		if (pendingForceCacheWrite)
		{
			pendingForceCacheWrite = false;
			fiMods = true;
		}

		if (fiMods)
			locked_saveFileIndexes() ;

		fi.updateMaxModTime() ;	// Update modification times for proper display.

		mInCheck = false;

		if(useHashCache)
			hashCache.save() ;
	}
}

static std::string friendlyUnit(uint64_t val) 
{
	const std::string units[5] = {"B","KB","MB","GB","TB"};
	char buf[50] ;

	double fact = 1.0 ;
	
	for(unsigned int i=0; i<5; ++i) 
		if(double(val)/fact < 1024.0)
		{
			sprintf(buf,"%2.2f",double(val)/fact) ;
			return std::string(buf) + " " + units[i];
		}
		else
			fact *= 1024.0f ;

	sprintf(buf,"%2.2f",double(val)/fact*1024.0f) ;
	return  std::string(buf) + " TB";
}


void FileIndexMonitor::hashFiles(const std::vector<DirContentToHash>& to_hash)
{
	// Size interval at which we save the file lists
	static const uint64_t MAX_SIZE_WITHOUT_SAVING = 10737418240ull ; // 10 GB

	cb->notifyListPreChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);

	time_t stamp = time(NULL);

	// compute total size of files to hash
	uint64_t total_size = 0 ;
	uint32_t n_files = 0 ;

	for(uint32_t i=0;i<to_hash.size();++i)
		for(uint32_t j=0;j<to_hash[i].fentries.size();++j,++n_files)
			total_size += to_hash[i].fentries[j].size ;

#ifdef FIM_DEBUG
	std::cerr << "Hashing content of " << to_hash.size() << " different directories." << std::endl ;
	std::cerr << "Total number of files: " << n_files << std::endl;
	std::cerr << "Total size: " << total_size << " bytes"<< std::endl;
#endif

	uint32_t cnt=0 ;
	uint64_t size=0 ;
	uint64_t hashed_size=0 ;
	uint64_t last_save_size=0 ;

	// check if thread is running
	bool running = isRunning();

	/* update files */
	for(uint32_t i=0;running && i<to_hash.size();++i)
		for(uint32_t j=0;running && j<to_hash[i].fentries.size();++j,++cnt)
		{
			// This is a very basic progress notification. To be more complete and user friendly, one would
			// rather send a completion ratio based on the size of files vs/ total size.
			//
			FileEntry fe(to_hash[i].fentries[j]) ;	// copied, because hashFile updates the hash member
			std::ostringstream tmpout;
			tmpout << cnt+1 << "/" << n_files << " (" << friendlyUnit(size) << " - " << int(size/double(total_size)*100.0) << "%) : " << fe.name ;

			cb->notifyHashingInfo(NOTIFY_HASHTYPE_HASH_FILE, tmpout.str()) ;

			std::string real_path(to_hash[i].realpath + "/" + fe.name) ;

			// 1st look into the hash cache if this file already exists.
			//
			if(useHashCache && hashCache.find(real_path,fe.size,fe.modtime,fe.hash)) 
				fi.updateFileEntry(to_hash[i].dirpath,fe,stamp);
			else if(RsDirUtil::getFileHash(real_path, fe.hash,fe.size, this))		// not found, then hash it.
			{
				RsStackMutex stack(fiMutex); /**** LOCKED DIRS ****/

				/* update fileIndex with new time */
				/* update with new time */

				fi.updateFileEntry(to_hash[i].dirpath,fe,stamp);

				hashed_size += to_hash[i].fentries[j].size ;

				// Update the hash cache
				if(useHashCache)
					hashCache.insert(real_path,fe.size,fe.modtime,fe.hash) ;
			}
			else
				std::cerr << "Failed to Hash File " << fe.name << std::endl;

			size += to_hash[i].fentries[j].size ;

			/* don't hit the disk too hard! */
#ifndef WINDOWS_SYS
			/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
			usleep(40000); /* 40 msec */
#else

			Sleep(40);
#endif

			// Save the hashing result every 60 seconds, so has to save what is already hashed.
			std::cerr << "size - last_save_size = " << hashed_size - last_save_size << ", max=" << MAX_SIZE_WITHOUT_SAVING << std::endl ;

			if(hashed_size > last_save_size + MAX_SIZE_WITHOUT_SAVING)
			{
				cb->notifyHashingInfo(NOTIFY_HASHTYPE_SAVE_FILE_INDEX, "") ;
#ifdef WINDOWS_SYS
				Sleep(1000) ;
#else
				sleep(1) ;
#endif
				RsStackMutex stack(fiMutex); /**** LOCKED DIRS ****/
				FileIndexMonitor::locked_saveFileIndexes() ;
				last_save_size = hashed_size ;

				if(useHashCache)
					hashCache.save() ;
			}

			// check if thread is running
			running = isRunning();
		}

	cb->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
}


void FileIndexMonitor::locked_saveFileIndexes()
{
	/* store to the cacheDirectory */

	std::string path = getCacheDir();

	// Two files are saved: one with only browsable dirs, which will be shared by the cache system,
	// and one with the complete file collection.
	//
	std::ostringstream out;
	out << "fc-own-" << time(NULL) << ".rsfb";
	std::string tmpname_browsable = out.str();
	std::string fname_browsable = path + "/" + tmpname_browsable;

	std::ostringstream out2;
	out2 << "fc-own-" << time(NULL) << ".rsfc";
	std::string tmpname_total = out2.str();
	std::string fname_total = path + "/" + tmpname_total;

#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::updateCycle() FileIndex modified ... updating";
	std::cerr <<  std::endl;
	std::cerr << "FileIndexMonitor::updateCycle() saving browsable file list to: " << fname_browsable << std::endl ;
	std::cerr << "FileIndexMonitor::updateCycle() saving total file list to  to: " << fname_total << std::endl ;
#endif

	std::string calchash;
	uint64_t size;

	std::cerr << "About to save, with the following restrictions:" << std::endl ;
	std::set<std::string> forbidden_dirs ;
	for(std::map<std::string,SharedDirInfo>::const_iterator it(directoryMap.begin());it!=directoryMap.end();++it)
	{
		std::cerr << "   dir=" << it->first << " : " ;
		if((it->second.shareflags & RS_FILE_HINTS_BROWSABLE) == 0)
		{
			std::cerr << "forbidden" << std::endl;
			forbidden_dirs.insert(it->first) ;
		}
		else
			std::cerr << "autorized" << std::endl;
	}

	uint64_t sizetmp ;

	fi.saveIndex(fname_total, calchash, sizetmp,std::set<std::string>());	// save all files
	fi.saveIndex(fname_browsable, calchash, size,forbidden_dirs);		// save only browsable files

	if(size > 0)
	{
#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::updateCycle() saved with hash:" << calchash;
		std::cerr <<  std::endl;
#endif

		/* should clean up the previous cache.... */

		/* flag as new info */
		CacheData data;
		data.pid = fi.root->id;
		data.cid.type  = getCacheType();
		data.cid.subid = 0;
		data.path = path;
		data.name = tmpname_browsable;
		data.hash = calchash;
		data.size = size;
		data.recvd = time(NULL);

		updateCache(data);
	}

#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::updateCycle() called updateCache()";
	std::cerr <<  std::endl;
#endif
}

void    FileIndexMonitor::updateShareFlags(const SharedDirInfo& dir)
{
	cb->notifyListPreChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);

	bool fimods = false ;
#ifdef FIM_DEBUG
	std::cerr << "*** FileIndexMonitor: Updating flags for " << dir.filename << " to " << dir.shareflags << std::endl ;
#endif
	{
		RsStackMutex stack(fiMutex) ;	/* LOCKED DIRS */

		for(std::list<SharedDirInfo>::iterator it(pendingDirList.begin());it!=pendingDirList.end();++it)
		{
			std::cerr  << "** testing pending dir " << (*it).filename << std::endl ;
			if((*it).filename == dir.filename)
			{
				std::cerr  << "** Updating to " << (*it).shareflags << "!!" << std::endl ;
				(*it).shareflags = dir.shareflags ;
				break ;
			}
		}

		for(std::map<std::string,SharedDirInfo>::iterator it(directoryMap.begin());it!=directoryMap.end();++it)
		{
			std::cerr  << "** testing " << (*it).second.filename << std::endl ;
			if((*it).second.filename == dir.filename)
			{
				std::cerr  << "** Updating from " << it->second.shareflags << "!!" << std::endl ;
				(*it).second.shareflags = dir.shareflags ;
				fimods = true ;
				break ;
			}
		}
	}
	if(fimods)
	{
		RsStackMutex stack(fiMutex) ;	/* LOCKED DIRS */
		locked_saveFileIndexes() ;
	}
	cb->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
}
	/* interface */
void    FileIndexMonitor::setSharedDirectories(const std::list<SharedDirInfo>& dirs)
{
	cb->notifyListPreChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);

	std::list<SharedDirInfo> checkeddirs;

	std::list<SharedDirInfo>::const_iterator it;
#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::setSharedDirectories() :\n";
#endif

	for(it = dirs.begin(); it != dirs.end(); it++)
	{

#ifdef FIM_DEBUG
		std::cerr << "\t" << (*it).filename;
		std::cerr <<  std::endl;
#endif

		/* check if dir exists before adding in */
		std::string path = (*it).filename;
		if (!RsDirUtil::checkDirectory(path))
		{
#ifdef FIM_DEBUG
			std::cerr << "FileIndexMonitor::setSharedDirectories()";
			std::cerr << " Ignoring NonExistant SharedDir: " << path << std::endl;
#endif
		}
		else
		{
			checkeddirs.push_back(*it);
		}
	}

	{
		RsStackMutex stack(fiMutex) ;/* LOCKED DIRS */

		pendingDirs = true;
		pendingDirList = checkeddirs;
	}

	cb->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
}

	/* interface */
void    FileIndexMonitor::getSharedDirectories(std::list<SharedDirInfo> &dirs)
{
	RsStackMutex stack(fiMutex) ; /* LOCKED DIRS */

	/* must provide pendingDirs, as other parts depend on instanteous response */
	if (pendingDirs)
		dirs = pendingDirList;
	else
	{
		/* get actual list (not pending stuff) */
		std::map<std::string, SharedDirInfo>::const_iterator it;

		for(it = directoryMap.begin(); it != directoryMap.end(); it++)
			dirs.push_back(it->second) ;
	}
}


	/* interface */
void    FileIndexMonitor::forceDirectoryCheck()
{
	RsStackMutex stack(fiMutex) ; /* LOCKED DIRS */

	if (!mInCheck)
		mForceCheck = true;
}


	/* interface */
bool    FileIndexMonitor::inDirectoryCheck()
{
	RsStackMutex stack(fiMutex); /**** LOCKED DIRS ****/

	return mInCheck;
}


bool    FileIndexMonitor::internal_setSharedDirectories()
{
	int i;
	RsStackMutex stack(fiMutex) ; /* LOCKED DIRS */

	if (!pendingDirs)
	{
		if (mForceCheck)
		{
			mForceCheck = false;
			return true;
		}

		return false;
	}

	mForceCheck = false;
	pendingDirs = false;
	pendingForceCacheWrite = true;

	/* clear old directories */
	directoryMap.clear();

	/* iterate through the directories */
	std::list<SharedDirInfo>::iterator it;
	std::map<std::string, SharedDirInfo>::const_iterator cit;
	for(it = pendingDirList.begin(); it != pendingDirList.end(); it++)
	{
		/* get the head directory */
		std::string root_dir = (*it).filename;
		std::string top_dir  = it->virtualname;
		if (top_dir.empty()) {
			top_dir = RsDirUtil::getTopDir(root_dir);
		}

		/* if unique -> add, else add modifier  */
		bool unique = false;
		for(i = 0; !unique; i++)
		{
			std::string tst_dir = top_dir;
			if (i > 0)
			{
				std::ostringstream out;
				out << "-" << i;
				tst_dir += out.str();
			}
			if (directoryMap.end()== (cit=directoryMap.find(tst_dir)))
			{
				unique = true;
				/* store calculated name */
				it->virtualname = tst_dir;
				/* add it! */
				directoryMap[tst_dir.c_str()] = *it;
#ifdef FIM_DEBUG
				std::cerr << "Added [" << tst_dir << "] => " << root_dir << std::endl;
#endif
			}
		}
	}

	pendingDirList.clear();

	/* now we've decided on the 'root' dirs set them to the
	 * fileIndex
	 */
	std::list<std::string> topdirs;
	for(cit = directoryMap.begin(); cit != directoryMap.end(); cit++)
	{
		topdirs.push_back(cit->first);
	}

	fi.setRootDirectories(topdirs, 0);

	locked_saveFileIndexes() ;

	return true;
}

/* lookup directory function */
std::string FileIndexMonitor::locked_findRealRoot(std::string rootdir) const
{
	/**** MUST ALREADY BE LOCKED ****/
	std::string realroot = "";

	std::map<std::string, SharedDirInfo>::const_iterator cit;
	if (directoryMap.end()== (cit=directoryMap.find(rootdir)))
	{
		std::cerr << "FileIndexMonitor::locked_findRealRoot() Invalid RootDir: ";
		std::cerr << rootdir << std::endl;
	}
	else
	{
		realroot = cit->second.filename;
	}

	return realroot;
}

int FileIndexMonitor::RequestDirDetails(std::string uid, std::string path, DirDetails &details) const
{
	/* lock it up */
	RsStackMutex mutex(fiMutex) ;

	return (uid == fi.root->id) ;
}

int FileIndexMonitor::RequestDirDetails(void *ref, DirDetails &details, uint32_t flags) const
{
	RsStackMutex mutex(fiMutex) ;

#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::RequestDirDetails() ref=" << ref << " flags: " << flags << std::endl;
#endif

	/* root case */

#ifdef FIM_DEBUG
	fi.root->checkParentPointers();
#endif

	// If ref is NULL, we build a root node

	if (ref == NULL)
	{
#ifdef FI_DEBUG
		std::cerr << "FileIndex::RequestDirDetails() ref=NULL (root)" << std::endl;
#endif
		/* local only */
		DirStub stub;
		stub.type = DIR_TYPE_PERSON;
		stub.name = fi.root->name;
		stub.ref  = fi.root;
		details.children.push_back(stub);
		details.count = 1;

		details.parent = NULL;
		details.prow = -1;
		details.ref = NULL;
		details.type = DIR_TYPE_ROOT;
		details.name = "root";
		details.hash = "";
		details.path = "root";
		details.age = 0;
		details.flags = 0;
		details.min_age = 0 ;

		return true ;
	}	
	
	bool b = FileIndex::extractData(ref,details) ;

	if(!b)
		return false ;

	// look for the top level and setup flags accordingly
	// The top level directory is the first dir in parents for which
	// 	dir->parent->parent == NULL

	if(ref != NULL)
	{
		FileEntry *file = (FileEntry *) ref;

		uint32_t share_flags = locked_findShareFlags(file) ;

		details.flags |= (( (share_flags & RS_FILE_HINTS_BROWSABLE   )>0)?DIR_FLAGS_BROWSABLE   :0) ;
		details.flags |= (( (share_flags & RS_FILE_HINTS_NETWORK_WIDE)>0)?DIR_FLAGS_NETWORK_WIDE:0) ;
	}
	return true ;
}

uint32_t FileIndexMonitor::locked_findShareFlags(FileEntry *file) const
{
	uint32_t flags = 0 ;

	DirEntry *dir = dynamic_cast<DirEntry*>(file) ;
	if(dir == NULL)
		dir = dynamic_cast<DirEntry*>(file->parent) ;

	if(dir != NULL && dir->parent != NULL)
		while(dir->parent->parent != NULL)
			dir = dir->parent ;

	if(dir != NULL && dir->parent != NULL)
	{
#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::RequestDirDetails: top parent name=" << dir->name << std::endl ;
#endif
		std::map<std::string,SharedDirInfo>::const_iterator it = directoryMap.find(dir->name) ;

		if(it == directoryMap.end())
			std::cerr << "*********** ERROR *********** In " << __PRETTY_FUNCTION__ << std::endl ;
		else
			flags = it->second.shareflags & (RS_FILE_HINTS_BROWSABLE | RS_FILE_HINTS_NETWORK_WIDE) ;
#ifdef FIM_DEBUG
		std::cerr << "flags = " << flags << std::endl ;
#endif
	}

	return flags ;
}


