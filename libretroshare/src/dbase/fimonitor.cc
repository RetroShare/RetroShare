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
#include "util/rsstring.h"
#include "util/rswin.h"
#endif

#include "rsserver/p3face.h"
#include "dbase/fimonitor.h"
#include "util/rsdir.h"
#include "pqi/authssl.h"
#include "serialiser/rsserviceids.h"
#include "retroshare/rsiface.h"
#include "pqi/p3notify.h"
#include "retroshare/rspeers.h"
#include "retroshare/rstypes.h"
#include "util/folderiterator.h"
#include <errno.h>

#include <iostream>
#include <iomanip>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <time.h>

// ***********
//#define FIM_DEBUG 1
// ***********/

FileIndexMonitor::FileIndexMonitor(CacheStrapper *cs, std::string cachedir, const RsPeerId& pid,const std::string& config_dir)
    :CacheSource(RS_SERVICE_TYPE_FILE_INDEX, true, cs, cachedir), fiMutex("FileIndexMonitor"), fi(pid),
		pendingDirs(false), pendingForceCacheWrite(false),
		mForceCheck(false), mInCheck(false), hashCache(config_dir+"/" + "file_cache"),useHashCache(true)

{
	updatePeriod = 15 * 60; // 15 minutes
	reference_time = 0 ;
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

	// check for unencrypted
	
	std::istream *f = NULL ;
	uint64_t file_size ;

	if(RsDirUtil::checkFile( _path+".bin",file_size,false ) )
	{
		std::cerr << "Encrypted hash cache file present. Reading it." << std::endl;

		// read the binary stream into memory.
		//
		void *buffer = malloc(file_size) ;

		if(buffer == NULL)
		{
			std::cerr << "Cannot allocate memory for reading encrypted file cache, bytes=" << file_size << std::endl;
			return ;
		}
		FILE *F = fopen( (_path+".bin").c_str(),"rb") ;

		if(fread(buffer,1,file_size,F) != file_size)
		{
			std::cerr << "Cannot read from file " + _path+".bin" << ": something's wrong." << std::endl;
			free(buffer) ;
			fclose(F) ;
			return ;
		}
		fclose(F) ;

		void *decrypted_data =NULL;
		int decrypted_data_size =0;

		if(!AuthSSL::getAuthSSL()->decrypt(decrypted_data, decrypted_data_size, buffer, file_size))
		{
			std::cerr << "Cannot decrypt encrypted file cache. Something's wrong." << std::endl;
			free(buffer) ;
			return ;
		}
		free(buffer) ;

		std::string s((char *)decrypted_data,decrypted_data_size) ;
		f = new std::istringstream(s) ;

		free(decrypted_data) ;
	}
	else
	{
		std::cerr << "Encrypted file cache not present. Trying unencrypted..." << std::endl;

		f  = new std::ifstream( (_path+".lst").c_str()) ;

		if(!f->good())
		{
			std::cerr << "Unencrypted file cache not present either." << std::endl;
			return ;
		}
	}

	std::streamsize max_line_size = 2000 ; // should be enough. Anyway, if we
														// miss one entry, we just lose some
														// cache itemsn but this is not too
														// much of a problem.
	char *buff = new char[max_line_size] ;

#ifdef FIM_DEBUG
	std::cerr << "Loading HashCache from file " << path << std::endl ;
	int n=0 ;
#endif

	while(!f->eof())
	{
		HashCacheInfo info ;

		f->getline(buff,max_line_size,'\n') ;
		std::string name(buff) ;

		f->getline(buff,max_line_size,'\n') ; //if(sscanf(buff,"%llu",&info.size) != 1) break ;

		info.size = 0 ;
		sscanf(buff, RsDirUtil::scanf_string_for_uint(sizeof(info.size)), &info.size);

		f->getline(buff,max_line_size,'\n') ; if(sscanf(buff,RsDirUtil::scanf_string_for_uint(sizeof(info.time_stamp)),&info.time_stamp) != 1) { std::cerr << "Could not read one entry! Giving up." << std::endl; break ; }
		f->getline(buff,max_line_size,'\n') ; if(sscanf(buff,RsDirUtil::scanf_string_for_uint(sizeof(info.modf_stamp)),&info.modf_stamp) != 1) { std::cerr << "Could not read one entry! Giving up." << std::endl; break ; }
		f->getline(buff,max_line_size,'\n') ; info.hash = RsFileHash(std::string(buff)) ;

#ifdef FIM_DEBUG
		std::cerr << "  (" << name << ", " << info.size << ", " << info.time_stamp << ", " << info.modf_stamp << ", " << info.hash << std::endl ;
		++n ;
#endif
		_files[name] = info ;
	}

	delete[] buff ;
	delete f ;
#ifdef FIM_DEBUG
	std::cerr << n << " entries loaded." << std::endl ;
#endif
}

void HashCache::save()
{
	if(!_changed)
	{
#ifdef FIM_DEBUG
		std::cerr << "Hash cache not changed. Not saving." << std::endl ;
#endif
		return;
	}

#ifdef FIM_DEBUG
	std::cerr << "Saving Hash Cache to file " << _path << "..." << std::endl ;
#endif

	std::ostringstream f ;
	for(std::map<std::string,HashCacheInfo>::const_iterator it(_files.begin());
		it != _files.end();
		++it)
	{
		f << it->first << std::endl ;
		f << it->second.size << std::endl;
		f << it->second.time_stamp << std::endl;
		f << it->second.modf_stamp << std::endl;
		f << it->second.hash << std::endl;
	}

	void *encryptedData = NULL ;
	int encDataLen = 0 ;

	if(!AuthSSL::getAuthSSL()->encrypt(
				encryptedData,
				encDataLen,
				f.str().c_str(),
				f.str().length(),
				AuthSSL::getAuthSSL()->OwnId()))
	{
		std::cerr << "Cannot encrypt hash cache. Something's wrong." << std::endl;
		return;
	}

	FILE *F = fopen( (_path+".bin.tmp").c_str(),"wb" ) ;

	if(!F)
	{
		std::cerr << "Cannot open encrypted file cache for writing: " << _path+".bin.tmp" << std::endl;
		goto save_free;
	}
    if(fwrite(encryptedData,1,encDataLen,F) != (uint32_t)encDataLen)
	{
		std::cerr << "Could not write entire encrypted hash cache file. Out of disc space??" << std::endl;
		fclose(F) ;
		goto save_free;
	}

	fclose(F) ;

	RsDirUtil::renameFile(_path+".bin.tmp",_path+".bin") ;
#ifdef FIM_DEBUG
	std::cerr << "done." << std::endl ;
#endif

	_changed = false;

save_free:
	free(encryptedData);
}

bool HashCache::find(const std::string& full_path,uint64_t size,time_t time_stamp,RsFileHash& hash)
{
#ifdef FIM_DEBUG
	std::cerr << "HashCache: looking for " << full_path << std::endl ;
#endif
	time_t now = time(NULL) ;
	std::map<std::string,HashCacheInfo>::iterator it(_files.find(full_path)) ;

	if(it != _files.end() && (uint64_t)time_stamp == it->second.modf_stamp && size == it->second.size)
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
void HashCache::insert(const std::string& full_path,uint64_t size,time_t time_stamp,const RsFileHash& hash)
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
		if(it->second.time_stamp + duration < (uint64_t)now)
		{
#ifdef FIM_DEBUG
			std::cerr << "  Entry too old: " << it->first << ", ts=" << it->second.time_stamp << std::endl ;
#endif
			std::map<std::string,HashCacheInfo>::iterator tmp(it) ;
			++tmp ;
			_files.erase(it) ;
			it=tmp ;
			_changed = true ;
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

int FileIndexMonitor::SearchKeywords(std::list<std::string> keywords, std::list<DirDetails> &results,FileSearchFlags flags,const RsPeerId& peer_id)
{
	results.clear();
	std::list<FileEntry *> firesults;

	{ 
		RsStackMutex stackM(fiMutex) ;/* LOCKED DIRS */
		fi.searchTerms(keywords, firesults);
	}

	return filterResults(firesults,results,flags,peer_id) ;
}

int FileIndexMonitor::SearchBoolExp(Expression *exp, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& peer_id) const
{
	results.clear();
	std::list<FileEntry *> firesults;

	{ 
		RsStackMutex stackM(fiMutex) ;/* LOCKED DIRS */
		fi.searchBoolExp(exp, firesults);
	}

	return filterResults(firesults,results,flags,peer_id) ;
}

int FileIndexMonitor::filterResults(std::list<FileEntry*>& firesults,std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& peer_id) const
{
#ifdef DEBUG
	if((flags & ~RS_FILE_HINTS_PERMISSION_MASK) > 0)
		std::cerr << "(EE) ***** FileIndexMonitor:: Flags ERROR in filterResults!!" << std::endl;
#endif
	/* translate/filter results */

	for(std::list<FileEntry*>::const_iterator rit(firesults.begin()); rit != firesults.end(); ++rit)
	{
		DirDetails cdetails ;
		RequestDirDetails (*rit,cdetails,FileSearchFlags(0u));
#ifdef FIM_DEBUG
		std::cerr << "Filtering candidate " << (*rit)->name  << ", flags=" << cdetails.flags << ", peer=" << peer_id ;
#endif

		if(!peer_id.isNull())
		{
			FileSearchFlags permission_flags = rsPeers->computePeerPermissionFlags(peer_id,cdetails.flags,cdetails.parent_groups) ;

			if (cdetails.type == DIR_TYPE_FILE && ( permission_flags & flags ))
			{
				cdetails.id.clear() ;
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
		else
			results.push_back(cdetails);
	}

	return !results.empty() ;
}

bool FileIndexMonitor::findLocalFile(const RsFileHash& hash,FileSearchFlags hint_flags, const RsPeerId& peer_id,std::string &fullpath, uint64_t &size,FileStorageFlags& storage_flags,std::list<std::string>& parent_groups) const
{
	std::list<FileEntry *> results;
	bool ok = false;

	{ 
		RsStackMutex stackM(fiMutex) ;/* LOCKED DIRS */

#ifdef FIM_DEBUG
//		std::cerr << "FileIndexMonitor::findLocalFile() Hash: " << hash << std::endl;
#endif
		/* search through the fileIndex */
		fi.searchHash(hash, results);

		if ( !results.empty() )
		{
			/* find the full path for the first entry */
			FileEntry *fe = results.front();
			DirEntry  *de = fe->parent; /* all files must have a valid parent! */

			locked_findShareFlagsAndParentGroups(fe,storage_flags,parent_groups) ;

			// turn share flags into hint flags

			FileSearchFlags shflh = peer_id.isNull()?(RS_FILE_HINTS_BROWSABLE|RS_FILE_HINTS_NETWORK_WIDE):rsPeers->computePeerPermissionFlags(peer_id,storage_flags,parent_groups) ;
#ifdef FIM_DEBUG
			std::cerr << "FileIndexMonitor::findLocalFile: Filtering candidate " << fe->name  << ", flags=" << storage_flags << ", hint_flags=" << hint_flags << ", peer_id = " << peer_id << std::endl ;
#endif

			if(peer_id.isNull() || (shflh & hint_flags))
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


bool FileIndexMonitor::loadLocalCache(const RsCacheData &data)  /* called with stored data */
{

#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::loadLocalCache(): subid = " << data.cid.subid << ", filename=" << data.name << ", peer id = " << data.pid << std::endl;
#endif

	if(!strcmp(data.name.c_str()+data.name.size()-5,".rsfc"))// this trick allows to load the complete file. Not the one being shared.
	{ 									// other files are discarded and re-created in case permissions have changed.
		RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */

		/* More error checking needed here! */

		std::string name = data.name ;	

		if ( fi.loadIndex(data.path + '/' + name, RsFileHash(), data.size) )
		{
#ifdef FIM_DEBUG
			std::cerr << "FileIndexMonitor::loadCache() Success!";
			std::cerr << std::endl;
#endif
			fi.root->row = 0;
			fi.root->name = data.pid.toStdString(); // XXX Hack here - TODO

			std::string fname_browsable = data.path + '/' + name ;
			struct stat64 buf;

#ifdef WINDOWS_SYS
			std::wstring wfullname;
			librs::util::ConvertUtf8ToUtf16(fname_browsable, wfullname);
			if ( 0 == _wstati64(wfullname.c_str(), &buf))
#else
			if ( 0 == stat64(fname_browsable.c_str(), &buf))
#endif
			{
				reference_time = buf.st_mtime ;
#ifdef FIM_DEBUG
				std::cerr << "Read new reference time of created file " << fname_browsable << ", to " << reference_time << std::endl;
#endif
			}
			else
			{
				std::cerr << "(EE) Error. Cannot get the proper modification time for file " << fname_browsable << " errno=" << errno << std::endl;
				reference_time = 0 ;
			}
#ifdef FIM_DEBUG
			std::cerr << "Current reference time is now : " << reference_time << std::endl;
#endif
		}
		else
		{
#ifdef FIM_DEBUG
			std::cerr << "FileIndexMonitor::loadCache() Failed!";
			std::cerr << std::endl;
#endif
			reference_time = 0 ;
		}

		fi.updateMaxModTime() ;

		// The index is re-saved. 
		// - we might have new friends
		// - the cache system removes old cache items so we need to re-create it.
		//
		locked_saveFileIndexes(false) ;
	}
#ifdef FIM_DEBUG
	else
		std::cerr << "FileIndexMonitor:: not loading cache item " << data.name << std::endl;
#endif

	return false;
}

bool FileIndexMonitor::updateCache(const RsCacheData &data,const std::set<RsPeerId>& destination_peers)  /* we call this one */
{
	return refreshCache(data,destination_peers);
}


int	FileIndexMonitor::getPeriod() const
{
#ifdef FIM_DEBUG
 std::cerr << "FileIndexMonitor::setPeriod() getting watch period" << std::endl;
#endif
	RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */
	return updatePeriod ;
}

void 	FileIndexMonitor::setPeriod(int period)
{
	RsStackMutex mtx(fiMutex) ; /* LOCKED DIRS */
	updatePeriod = period;
#ifdef FIM_DEBUG
 std::cerr << "FileIndexMonitor::setPeriod() Setting watch period to " << updatePeriod << std::endl;
#endif
}

void 	FileIndexMonitor::run()
{
	if(autoCheckEnabled())
		updateCycle();

	while(isRunning())
	{
		int i=0 ;
		for(;;++i)
		{
			if(!isRunning()) 
				return;

			usleep(1*1000*1000); // 1 sec

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
	}
}

void 	FileIndexMonitor::updateCycle()
{
	time_t startstamp = time(NULL);

#ifdef FIM_DEBUG
	std::cerr << "Checking directory for new/modified files. Reference time is " << reference_time << std::endl;
#endif
	/* iterate through all out-of-date directories */
	bool moretodo = true;
	bool fiMods = false;

	{
		RsStackMutex stack(fiMutex); /**** LOCKED DIRS ****/
		mInCheck = true;
	}

	RsServer::notify()->notifyHashingInfo(NOTIFY_HASHTYPE_EXAMINING_FILES, "") ;

	std::vector<DirContentToHash> to_hash ;

	bool cache_is_new ;
	{
		RsStackMutex mtx(fiMutex) ;
		cache_is_new = useHashCache && hashCache.empty() ;
	}
	struct stat64 buf;
		
	while(isRunning() && moretodo)
	{
		/* sleep a bit for each loop */
// csoler: I'm disabling this since it causes a very long update cycle when the number
// 		 of directories to go through is very large.
//
// 		usleep(100*1000); /* 100 msec */

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
			std::cerr << " Root Dir: " << rootdir << std::endl;
			std::cerr << " remdir: " << remdir << std::endl;
#endif
			if(directoryMap.end() != directoryMap.find(rootdir) && remdir=="")
			{
#ifdef FIM_DEBUG
				std::cerr << " This is a root directory. Keeping it empty." << std::endl;
#endif
			}
			else 
			{
				if (!fi.removeOldDirectory(olddir->parent->path, olddir->name, stamp))/* bad directory - delete */
				{
					/* bad... drop out of updateCycle() - hopefully the initial cleanup
					 * will deal with it next time! - otherwise we're in a continual loop
					 */
					std::cerr << "FileIndexMonitor::updateCycle()";
					std::cerr << "ERROR Failed to Remove: " << olddir->path << std::endl;
				}
				continue;
			}
		}

		/* update this dir - as its valid */
		fe.name = olddir->name;
		fi.updateDirEntry(olddir->parent->path, fe, stamp);

		/* update the directories and files here */
		std::map<std::string, DirEntry *>::iterator  dit;
		std::map<std::string, FileEntry *>::iterator fit;

		/* flag existing subdirs as old */
		for(dit = olddir->subdirs.begin(); dit != olddir->subdirs.end(); ++dit)
		{
			fe.name = (dit->second)->name;
			/* set the age as out-of-date so that it gets checked */
			fi.updateDirEntry(olddir->path, fe, 0);
		}

		/* now iterate through the directory...
		 * directories - flags as old,
		 * files checked to see if they have changed. (rehashed)
		 */

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
						// if reference_time was not inited, we revert to the old method: we test the saved mod time for the file
						// versus the measured mod time. If reference is inited (this is what should always happen) we compare the measured 
						// mod time with the reference time.
						//
						if ((fe.size != (fit->second)->size) || (reference_time==0 && fe.modtime != (fit->second)->modtime) || fe.modtime > reference_time) //(fit->second)->modtime))
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

	RsServer::notify()->notifyHashingInfo(NOTIFY_HASHTYPE_FINISH, "") ;

	int cleanedCount = 0;

	{ /* LOCKED DIRS */
		RsStackMutex stack(fiMutex); /**** LOCKED DIRS ****/

		/* finished update cycle - cleanup extra dirs/files that
		 * have not had their timestamps updated.
		 */

		cleanedCount = fi.cleanOldEntries(startstamp) ;

#ifdef FIM_DEBUG
		/* print out the new directory structure */
//		fi.printFileIndex(std::cerr);
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
		{
			reference_time = locked_saveFileIndexes(true) ;
#ifdef FIM_DEBUG
			std::cerr << "Index saved. New reference time is " << reference_time << std::endl;
#endif
		}

		fi.updateHashIndex() ;	// update hash map that is used to accelerate search.
		fi.updateMaxModTime() ;	// Update modification times for proper display.

		mInCheck = false;

		if(useHashCache)
		{
			hashCache.clean() ;
			hashCache.save() ;
		}
	}

	if (cleanedCount > 0) {
		RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
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

	RsServer::notify()->notifyListPreChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);

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
#ifdef FIM_DEBUG
			std::cerr << "Hashing file " << to_hash[i].fentries[j].name << " in dirpath=" << to_hash[i].dirpath << std::endl;
#endif

			// Before hashign each file, we test if the directory list has changed. If yes, we trigger 
			// to update directoryMap, so that we can properly test if the current file 
			// still is in a shared directory. If not, we skip it.
			//
			if(pendingDirs)
			{
#ifdef FIM_DEBUG
				std::cerr << "Pending dir list changed. Updating!" << std::endl;
#endif
				internal_setSharedDirectories() ;
			}

			{
				RsStackMutex stack(fiMutex) ;	/* LOCKED DIRS */

				if(directoryMap.end() == directoryMap.find(RsDirUtil::getRootDir(to_hash[i].dirpath)))
				{
					std::cerr << "Early suppression of root directory. Hash cancelled" << std::endl;
					total_size -= to_hash[i].fentries[j].size ;
					continue ;
				}
			}

			// This is a very basic progress notification. To be more complete and user friendly, one would
			// rather send a completion ratio based on the size of files vs/ total size.
			//
			FileEntry fe(to_hash[i].fentries[j]) ;	// copied, because hashFile updates the hash member
			std::string tmpout;
			rs_sprintf(tmpout, "%lu/%lu (%s - %d%%) : %s", cnt+1, n_files, friendlyUnit(size).c_str(), int(size/double(total_size)*100.0), fe.name.c_str()) ;

			RsServer::notify()->notifyHashingInfo(NOTIFY_HASHTYPE_HASH_FILE, tmpout) ;

			std::string real_path = RsDirUtil::makePath(to_hash[i].realpath, fe.name);

			// 1st look into the hash cache if this file already exists.
			//
			if(useHashCache && hashCache.find(real_path,fe.size,fe.modtime,fe.hash)) 
				fi.updateFileEntry(to_hash[i].dirpath,fe,stamp);
			else if(RsDirUtil::getFileHash(real_path, fe.hash,fe.size, this))		// not found, then hash it.
			{
				RsStackMutex stack(fiMutex); /**** LOCKED DIRS ****/

				/* update fileIndex with new time */
				/* update with new time */

				// Check again that the hashed file hasn't been modified since the beginning of the hashing process.
				// If so, drop it.
				//
				struct stat64 buf;

#ifdef WINDOWS_SYS
				std::wstring wfullname;
				librs::util::ConvertUtf8ToUtf16(real_path, wfullname);
				if ( 0 == _wstati64(wfullname.c_str(), &buf))
#else
				if ( 0 == stat64(real_path.c_str(), &buf))
#endif
				{
					if(buf.st_mtime != fe.modtime) 
						std::cerr << "File " << real_path << " has been modified while being hashed. It will be dropped to avoid data inconsistency" << std::endl;
					else
					{
						fi.updateFileEntry(to_hash[i].dirpath,fe,stamp);

						hashed_size += to_hash[i].fentries[j].size ;

						// Update the hash cache
						//
						if(useHashCache)
							hashCache.insert(real_path,fe.size,fe.modtime,fe.hash) ;
					}
				}
			}
			else
				std::cerr << "Failed to Hash File " << fe.name << std::endl;

			size += to_hash[i].fentries[j].size ;

			// don't hit the disk too hard!
			usleep(10*1000); // 10 msec

			// Save the hashing result every 60 seconds, so has to save what is already hashed.
#ifdef FIM_DEBUG
			std::cerr << "size - last_save_size = " << hashed_size - last_save_size << ", max=" << MAX_SIZE_WITHOUT_SAVING << std::endl ;
#endif

			if(hashed_size > last_save_size + MAX_SIZE_WITHOUT_SAVING)
			{
				RsServer::notify()->notifyHashingInfo(NOTIFY_HASHTYPE_SAVE_FILE_INDEX, "") ;

				//Waiting 1 sec before pass to other hash ??is it really needed??
				usleep(1*1000*1000); // 1 sec

				RsStackMutex stack(fiMutex); /**** LOCKED DIRS ****/
				fi.updateHashIndex() ;
				FileIndexMonitor::locked_saveFileIndexes(true) ;
				last_save_size = hashed_size ;

				if(useHashCache)
					hashCache.save() ;
			}

			// check if thread is running
			running = isRunning();
		}

	fi.updateHashIndex() ;

	RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
}


time_t FileIndexMonitor::locked_saveFileIndexes(bool update_cache)
{
	/* store to the cacheDirectory */

	std::string path = getCacheDir();

	// Multiple files are saved: for every kind of peers, the set of browsable files will be different. A specific file is
	// prepared for all situations, and shared by all peers having the same situation.
	//
	// A complete file collection is also saved, and serves as memory for the FileIndexMonitor system.
	//
#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::updateCycle() FileIndex modified ... updating" << std::endl;
#endif
	// Make for each peer the list of forbidden shared directories. Make a separate cache file for each different set.
	// To figure out which sets are different, we index them by the set of forbidden indexes from the directory list.
	// This is probably a bit costly, but we can't suppose that the number of shared directories is bounded.
	//
    std::list<RsPeerId> all_friend_ids ;
	rsPeers->getFriendList(all_friend_ids);

#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::updateCycle(): got list of all friends." << std::endl ;
	for(std::list<RsPeerId>::const_iterator it(all_friend_ids.begin());it!=all_friend_ids.end();++it)
		std::cerr << "  " << *it << std::endl;
#endif

    std::map<std::set<std::string>, std::set<RsPeerId> > peers_per_directory_combination ;

    for(std::list<RsPeerId>::const_iterator it(all_friend_ids.begin());it!=all_friend_ids.end();++it)
	{
#ifdef FIM_DEBUG
		std::cerr << "About to save, with the following restrictions:" << std::endl ;
		std::cerr << "Peer : " << *it << std::endl;
#endif

		std::set<std::string> forbidden_dirs ;
		for(std::map<std::string,SharedDirInfo>::const_iterator dit(directoryMap.begin());dit!=directoryMap.end();++dit)
		{
#ifdef FIM_DEBUG
			std::cerr << "   dir=" << dit->first << ", " ;
			std::cerr << "parent groups: " ;
			for(std::list<std::string>::const_iterator mit(dit->second.parent_groups.begin());mit!=dit->second.parent_groups.end();++mit)
				std::cerr << (*mit) << ", " ;
			std::cerr << std::endl;;
#endif

			FileSearchFlags permission_flags = rsPeers->computePeerPermissionFlags(*it,dit->second.shareflags,dit->second.parent_groups) ;

			if(!(permission_flags & RS_FILE_HINTS_BROWSABLE))
			{
#ifdef FIM_DEBUG
				std::cerr << "forbidden" << std::endl;
#endif
				forbidden_dirs.insert(dit->first) ;
			}
#ifdef FIM_DEBUG
			else
				std::cerr << "autorized" << std::endl;
#endif
		}

		peers_per_directory_combination[forbidden_dirs].insert(*it) ;
	}
    RsPeerId ownId = rsPeers->getOwnId() ;
	peers_per_directory_combination[std::set<std::string>()].insert(ownId) ;	// add full configuration to self, i.e. no forbidden directories.

	int n=0 ;
	time_t now = time(NULL) ;
	time_t mod_time = 0 ;

    for(std::map<std::set<std::string>, std::set<RsPeerId> >::const_iterator it(peers_per_directory_combination.begin());
			it!=peers_per_directory_combination.end();++it,++n)
	{
		std::string tmpname_browsable;

		if(it->first.empty())
			rs_sprintf(tmpname_browsable, "fc-own-%ld.rsfc",now,n);
		else
			rs_sprintf(tmpname_browsable, "fc-own-%ld.%04d",now,n);

		std::string fname_browsable = path + "/" + tmpname_browsable;

#ifdef FIM_DEBUG
		std::cerr << "Sending file list: " << std::endl;
		std::cerr << "   filename	: " << tmpname_browsable << std::endl;
		std::cerr << "   to peers  : " << std::endl;
		for(std::set<RsPeerId>::const_iterator itt(it->second.begin());itt!= it->second.end();++itt)
			std::cerr << "       " << *itt << std::endl;
		std::cerr << "   forbidden : " << std::endl;
		for(std::set<std::string>::const_iterator itt(it->first.begin());itt!= it->first.end();++itt)
			std::cerr << "       " << *itt << std::endl;
#endif

        RsFileHash hash ;
		uint64_t size ;

#ifdef FIM_DEBUG
		std::cerr << "writing file " << fname_browsable << std::endl;
#endif
		fi.saveIndex(fname_browsable, hash, size,it->first);		// save only browsable files

		if(size > 0)
		{
#ifdef FIM_DEBUG
			std::cerr << "FileIndexMonitor::updateCycle() saved with hash:" << hash <<  std::endl;
#endif

			/* should clean up the previous cache.... */

			/* flag as new info */
			RsCacheData data;
			data.pid = fi.root->id;
			data.cid.type  = getCacheType();
			data.cid.subid = n;
			data.path = path;
			data.name = tmpname_browsable;
			data.hash = hash;
			data.size = size;
			data.recvd = time(NULL);

            for(std::set<RsPeerId>::const_iterator ff(it->second.begin());ff!=it->second.end();++ff)
				_cache_items_per_peer[*ff] = data ;

            data.cid.subid = n;

			if(update_cache)
				updateCache(data,it->second);
		}

		if(it->first.empty())
		{
			// Computes the reference time.
			//
			struct stat64 buf;

#ifdef WINDOWS_SYS
			std::wstring wfullname;
			librs::util::ConvertUtf8ToUtf16(fname_browsable, wfullname);
			if ( 0 == _wstati64(wfullname.c_str(), &buf))
#else
			if ( 0 == stat64(fname_browsable.c_str(), &buf))
#endif
			{
				mod_time = buf.st_mtime ;
			}
			else
			{
				std::cerr << "(EE) Error. Cannot get the proper modification time for file " << fname_browsable << " errno=" << errno << std::endl;
				mod_time = 0 ;
			}
		}
	}

#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::updateCycle() called updateCache()";
	std::cerr <<  std::endl;
#endif
	return mod_time ;
}

bool FileIndexMonitor::cachesAvailable(const RsPeerId &pid,std::map<CacheId, RsCacheData> &ids)
{
	lockData() ;
#ifdef FIM_DEBUG
	std::cerr << "In cachesAvailable..." << std::endl;
#endif

	// Go through the list of saved cache items for that particular peer.
	//
	ids.clear() ;
	std::map<RsPeerId,RsCacheData>::const_iterator it(_cache_items_per_peer.find(pid)) ;
    RsPeerId ownId = rsPeers->getOwnId();

	if(it != _cache_items_per_peer.end())
    {
		ids[it->second.cid] = it->second ;

		if(pid != ownId)
		ids[it->second.cid].cid.subid = 0 ;	// Force subid to be 0, so that it's
														// not going to be mixed up at the client with other files received if the
														// subid changes for that peer.
														//
#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor: caches available for peer " << pid << ": " << it->second.name << std::endl ;
#endif
	}
#ifdef FIM_DEBUG
	else
		std::cerr << "No cache item for peer " << pid << std::endl;
#endif

	unlockData() ;

	return true ;
}

void    FileIndexMonitor::updateShareFlags(const SharedDirInfo& dir)
{
	RsServer::notify()->notifyListPreChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);

	bool fimods = false ;
#ifdef FIM_DEBUG
	std::cerr << "*** FileIndexMonitor: Updating flags for " << dir.filename << " to " << dir.shareflags << std::endl ;
#endif
	{
		RsStackMutex stack(fiMutex) ;	/* LOCKED DIRS */

		for(std::list<SharedDirInfo>::iterator it(pendingDirList.begin());it!=pendingDirList.end();++it)
		{
#ifdef FIM_DEBUG
			std::cerr  << "** testing pending dir " << (*it).filename << std::endl ;
#endif
			if((*it).filename == dir.filename)
			{
#ifdef FIM_DEBUG
				std::cerr  << "** Updating to " << (*it).shareflags << "!!" << std::endl ;
#endif
				(*it).shareflags = dir.shareflags ;
				(*it).parent_groups = dir.parent_groups ;
				break ;
			}
		}

		for(std::map<std::string,SharedDirInfo>::iterator it(directoryMap.begin());it!=directoryMap.end();++it)
		{
#ifdef FIM_DEBUG
			std::cerr  << "** testing " << (*it).second.filename << std::endl ;
#endif
			if((*it).second.filename == dir.filename)
			{
#ifdef FIM_DEBUG
				std::cerr  << "** Updating from " << it->second.shareflags << "!!" << std::endl ;
#endif
				(*it).second.shareflags = dir.shareflags ;
				(*it).second.parent_groups = dir.parent_groups ;
				fimods = true ;
				break ;
			}
		}
	}
	if(fimods)
	{
		RsStackMutex stack(fiMutex) ;	/* LOCKED DIRS */
		locked_saveFileIndexes(true) ;
	}
	RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
}
	/* interface */
void    FileIndexMonitor::setSharedDirectories(const std::list<SharedDirInfo>& dirs)
{
	RsServer::notify()->notifyListPreChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);

	std::list<SharedDirInfo> checkeddirs;

	std::list<SharedDirInfo>::const_iterator it;
#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::setSharedDirectories() :\n";
#endif

	for(it = dirs.begin(); it != dirs.end(); ++it)
	{

#ifdef FIM_DEBUG
		std::cerr << "\t" << (*it).filename;
		std::cerr <<  std::endl;
#endif

		checkeddirs.push_back(*it);

#ifdef REMOVED_CODE
		// this code has been removed because it prevents unmounted shared directories to stay in the list of shared files. It's better
		// to keep them showing empty than removing them completely.
		//
		/* check if dir exists before adding in */
//		std::string path = (*it).filename;
//		if (!RsDirUtil::checkDirectory(path))
//		{
//#ifdef FIM_DEBUG
//			std::cerr << "FileIndexMonitor::setSharedDirectories()";
//			std::cerr << " Ignoring NonExistant SharedDir: " << path << std::endl;
//#endif
//		}
//		else
//		{
//			checkeddirs.push_back(*it);
//		}
#endif
	}

	{
		RsStackMutex stack(fiMutex) ;/* LOCKED DIRS */

		pendingDirs = true;
		pendingDirList = checkeddirs;
	}

	RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
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

		for(it = directoryMap.begin(); it != directoryMap.end(); ++it)
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


void    FileIndexMonitor::forceDirListsRebuildAndSend()
{
	RsStackMutex stack(fiMutex) ; /* LOCKED DIRS */
	locked_saveFileIndexes(true) ;
}

	/* interface */
bool    FileIndexMonitor::inDirectoryCheck()
{
	RsStackMutex stack(fiMutex); /**** LOCKED DIRS ****/

	return mInCheck;
}


bool    FileIndexMonitor::internal_setSharedDirectories()
{
	bool changed = false;

	{
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
		for(it = pendingDirList.begin(); it != pendingDirList.end(); ++it)
		{
			/* get the head directory */
			std::string root_dir = (*it).filename;
			std::string top_dir  = it->virtualname;
			if (top_dir.empty()) {
				top_dir = RsDirUtil::getTopDir(root_dir);
			}

			/* if unique -> add, else add modifier  */
			bool unique = false;
			for(int i = 0; !unique; ++i)
			{
				std::string tst_dir = top_dir;
				if (i > 0)
				{
					rs_sprintf_append(tst_dir, "-%d", i);
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
		for(cit = directoryMap.begin(); cit != directoryMap.end(); ++cit)
		{
			topdirs.push_back(cit->first);
		}

		if (fi.setRootDirectories(topdirs, 0) > 0)
		{
			changed = true;
		}

		locked_saveFileIndexes(true) ;
	}

	if (changed)
	{
		RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
	}

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

int FileIndexMonitor::RequestDirDetails(const std::string& path, DirDetails& details) const
{
	/* lock it up */
	RsStackMutex mutex(fiMutex) ;
	return fi.extractData(path,details) ;
}

uint32_t FileIndexMonitor::getType(void *ref) const
{
	RsStackMutex mutex(fiMutex) ;

	return fi.getType(ref) ;
}
int FileIndexMonitor::RequestDirDetails(void *ref, DirDetails &details, FileSearchFlags flags) const
{
	/* remove unused parameter warnings */
	(void) flags;

	RsStackMutex mutex(fiMutex) ;

#ifdef FIM_DEBUG2
	std::cerr << "FileIndexMonitor::RequestDirDetails() ref=" << ref << " flags: " << flags << std::endl;
#endif

	/* root case */

#ifdef FIM_DEBUG2
	fi.root->checkParentPointers();
#endif

	// If ref is NULL, we build a root node

	if (ref == NULL)
	{
#ifdef FI_DEBUG2
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
		details.hash.clear() ;
		details.path = "root";
		details.age = 0;
		details.flags.clear() ;
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
		locked_findShareFlagsAndParentGroups(file,details.flags,details.parent_groups) ;
	}
	return true ;
}

void FileIndexMonitor::locked_findShareFlagsAndParentGroups(FileEntry *file,FileStorageFlags& flags,std::list<std::string>& parent_groups) const
{
	flags.clear() ;
	static const FileStorageFlags PERMISSION_MASK = DIR_FLAGS_BROWSABLE_OTHERS | DIR_FLAGS_NETWORK_WIDE_OTHERS | DIR_FLAGS_BROWSABLE_GROUPS | DIR_FLAGS_NETWORK_WIDE_GROUPS ;

	DirEntry *dir = dynamic_cast<DirEntry*>(file) ;
	if(dir == NULL)
		dir = dynamic_cast<DirEntry*>(file->parent) ;

	if(dir != NULL && dir->parent != NULL)
		while(dir->parent->parent != NULL)
			dir = dir->parent ;

	if(dir != NULL && dir->parent != NULL)
	{
#ifdef FIM_DEBUG2
		std::cerr << "FileIndexMonitor::RequestDirDetails: top parent name=" << dir->name << std::endl ;
#endif
		std::map<std::string,SharedDirInfo>::const_iterator it = directoryMap.find(dir->name) ;

		if(it == directoryMap.end())
			std::cerr << "*********** ERROR *********** In " << __PRETTY_FUNCTION__ << std::endl ;
		else
		{
			flags = it->second.shareflags ;
			flags &= PERMISSION_MASK ;
			flags &= ~DIR_FLAGS_NETWORK_WIDE_GROUPS ; // Disabling this flag for now, because it has inconsistent effects.
			parent_groups = it->second.parent_groups ;
		}
#ifdef FIM_DEBUG2
		std::cerr << "flags = " << flags << std::endl ;
#endif
	}
}


