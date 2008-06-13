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

#include "dbase/fimonitor.h"
#include "util/rsdir.h"
#include "serialiser/rsserviceids.h"

#include <iostream>
#include <sstream>
#include <iomanip>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>

/***********
 * #define FIM_DEBUG 1
 ***********/

FileIndexMonitor::FileIndexMonitor(CacheStrapper *cs, std::string cachedir, std::string pid)
	:CacheSource(RS_SERVICE_TYPE_FILE_INDEX, false, cs, cachedir), fi(pid), 
		pendingDirs(false), pendingForceCacheWrite(false), 
		mForceCheck(false), mInCheck(false)

{
	updatePeriod = 60;
}


FileIndexMonitor::~FileIndexMonitor()
{
	/* Data cleanup - TODO */
	return;
}

bool    FileIndexMonitor::findLocalFile(std::string hash, 
				std::string &fullpath, uint64_t &size)
{
	std::list<FileEntry *> results;
	bool ok = false;

	fiMutex.lock(); { /* LOCKED DIRS */

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

#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::findLocalFile() Found Name: " << fe->name << std::endl;
#endif
		std::string shpath =  RsDirUtil::removeRootDir(de->path);
		std::string basedir = RsDirUtil::getRootDir(de->path);
		std::string realroot = findRealRoot(basedir);

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


	} fiMutex.unlock(); /* UNLOCKED DIRS */

	return ok;
}

bool    FileIndexMonitor::convertSharedFilePath(std::string path, std::string &fullpath)
{
	bool ok = false;

	fiMutex.lock(); { /* LOCKED DIRS */

#ifdef FIM_DEBUG
	std::cerr << "FileIndexMonitor::convertSharedFilePath() path: " << path << std::endl;
#endif

	std::string shpath =  RsDirUtil::removeRootDir(path);
	std::string basedir = RsDirUtil::getRootDir(path);
	std::string realroot = findRealRoot(basedir);

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

	} fiMutex.unlock(); /* UNLOCKED DIRS */

	return ok;
}


bool FileIndexMonitor::loadLocalCache(const CacheData &data)  /* called with stored data */
{
	bool ok = false;

	fiMutex.lock(); { /* LOCKED DIRS */

	//fi.root->name = data.pid;

	/* More error checking needed here! */
	if ((ok = fi.loadIndex(data.path + '/' + data.name, 
				data.hash, data.size)))
	{
#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::loadCache() Success!";
		std::cerr << std::endl;
#endif
	}
	else
	{
#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::loadCache() Failed!";
		std::cerr << std::endl;
#endif
	}
		
	} fiMutex.unlock(); /* UNLOCKED DIRS */

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


void 	FileIndexMonitor::setPeriod(int period)
{
	updatePeriod = period;
}

void 	FileIndexMonitor::run()
{

	updateCycle();

	while(1)
	{

		for(int i = 0; i < updatePeriod; i++)
		{

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
			sleep(1);
#else

                	Sleep(1000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

			/* check dirs if they've changed */
			if (internal_setSharedDirectories())
			{
				break;
			}
		}

		updateCycle();
	}
}


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

	while(moretodo)
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
		if (internal_setSharedDirectories())
		{
			/* reset start time */
			startstamp = time(NULL);
		}
		
		/* Handle a Single out-of-date directory */

		time_t stamp = time(NULL);

		/* lock dirs */
		fiMutex.lock();

        	DirEntry *olddir = fi.findOldDirectory(startstamp);

		if (!olddir)
		{
			/* finished */
			fiMutex.unlock();
			moretodo = false;
			continue;
		}

#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::updateCycle()";
		std::cerr << " Checking: " << olddir->path << std::endl;
#endif


        	FileEntry fe;
		/* entries that need to be checked properly */
		std::list<FileEntry> filesToHash; 
		std::list<FileEntry>::iterator hit;

		/* determine the full root path */
		std::string dirpath = olddir->path;
		std::string rootdir = RsDirUtil::getRootDir(olddir->path);
		std::string remdir  = RsDirUtil::removeRootDir(olddir->path);

		std::string realroot = findRealRoot(rootdir);

		std::string realpath = realroot;
		if (remdir != "")
		{
			realpath += "/" + remdir;
		}
			


#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::updateCycle()";
		std::cerr << " RealPath: " << realpath << std::endl;
#endif

		/* check for the dir existance */
		DIR *dir = opendir(realpath.c_str());
		if (!dir)
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

			fiMutex.unlock();
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

		struct dirent *dent;
		struct stat buf;

		while(NULL != (dent = readdir(dir)))
		{
			/* check entry type */
			std::string fname = dent -> d_name;
			std::string fullname = realpath + "/" + fname;

	 		if (-1 != stat(fullname.c_str(), &buf))
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
						if ((fe.size != (fit->second)->size) ||
						    (fe.modtime != (fit->second)->modtime))
						{
#ifdef FIM_DEBUG
						std::cerr << "File ModTime/Size changed:" << fname << std::endl;
#endif
							toadd = true;
						}
						else
						{
							/* keep old info */
							fe.hash = (fit->second)->hash;
						}
					}
					if (toadd)
					{
						/* push onto Hash List */
#ifdef FIM_DEBUG
						std::cerr << "Adding to Update List: ";
						std::cerr << olddir->path;
						std::cerr << fname << std::endl;
#endif
						filesToHash.push_back(fe);
					}
					else
					{
						/* update with new time */
#ifdef FIM_DEBUG
						std::cerr << "File Hasn't Changed:" << fname << std::endl;
#endif
                        			fi.updateFileEntry(olddir->path, fe, stamp);
					}
	 			}
				else
				{
					/* unknown , ignore */
					continue;
				}
			}
		}


		/* now we unlock the lock, and iterate through the
		 * next files - hashing them, before adding into the system.
		 */
		/* for safety - blank out data we cannot use (TODO) */
		olddir = NULL;	

		/* close directory */
		closedir(dir);
			
		/* unlock dirs */
		fiMutex.unlock();

		if (filesToHash.size() > 0)
		{
#ifdef FIM_DEBUG
			std::cerr << "List of Files to rehash in: " << dirpath << std::endl;
#endif
			fiMods = true;
		}

#ifdef FIM_DEBUG
                for(hit = filesToHash.begin(); hit != filesToHash.end(); hit++)
		{
			std::cerr << "\t" << hit->name << std::endl;
		}

		if (filesToHash.size() > 0)
		{
			std::cerr << std::endl;
		}
#endif
			
                /* update files */
                for(hit = filesToHash.begin(); hit != filesToHash.end(); hit++)
                {
			if (hashFile(realpath, (*hit)))
			{
				/* lock dirs */
				fiMutex.lock();

				/* update fileIndex with new time */
				/* update with new time */
                        	fi.updateFileEntry(dirpath, *hit, stamp);
				
				/* unlock dirs */
				fiMutex.unlock();
			}
			else
			{
				std::cerr << "Failed to Hash File!" << std::endl;
			}

			/* don't hit the disk too hard! */
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
			usleep(10000); /* 1/100 sec */
#else

       		        Sleep(10);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

                }
        }

	fiMutex.lock(); { /* LOCKED DIRS */

	/* finished update cycle - cleanup extra dirs/files that 
	 * have not had their timestamps updated.
	 */

	if (fi.cleanOldEntries(startstamp))
	{
		//fiMods = true;
	}

	/* print out the new directory structure */

	fi.printFileIndex(std::cerr);

	/* now if we have changed things -> restore file/hash it/and 
	 * tell the CacheSource
	 */

	if (pendingForceCacheWrite)
	{
		pendingForceCacheWrite = false;
		fiMods = true;
	}

	} fiMutex.unlock(); /* UNLOCKED DIRS */


	if (fiMods)
	{
		/* store to the cacheDirectory */
		fiMutex.lock(); { /* LOCKED DIRS */

		std::string path = getCacheDir();
		std::ostringstream out; 
		out << "fc-own-" << time(NULL) << ".rsfc";

		std::string tmpname = out.str();
		std::string fname = path + "/" + tmpname;

#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::updateCycle() FileIndex modified ... updating";
		std::cerr <<  std::endl;
		std::cerr << "FileIndexMonitor::updateCycle() saving to: " << fname;
		std::cerr <<  std::endl;
#endif

		std::string calchash;
		uint64_t size;

		fi.saveIndex(fname, calchash, size);

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
		data.name = tmpname;
		data.hash = calchash;
		data.size = size;
		data.recvd = time(NULL);

		updateCache(data);

#ifdef FIM_DEBUG
		std::cerr << "FileIndexMonitor::updateCycle() called updateCache()";
		std::cerr <<  std::endl;
#endif

		} fiMutex.unlock(); /* UNLOCKED DIRS */
	}

	{
		RsStackMutex stack(fiMutex); /**** LOCKED DIRS ****/
		mInCheck = false;
	}
}

	/* interface */
void    FileIndexMonitor::setSharedDirectories(std::list<std::string> dirs)
{
	fiMutex.lock(); { /* LOCKED DIRS */

	pendingDirs = true;
	pendingDirList = dirs;

	} fiMutex.unlock(); /* UNLOCKED DIRS */
}

	/* interface */
void    FileIndexMonitor::forceDirectoryCheck()
{
	fiMutex.lock(); { /* LOCKED DIRS */

	if (!mInCheck)
		mForceCheck = true;

	} fiMutex.unlock(); /* UNLOCKED DIRS */
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
	fiMutex.lock(); /* LOCKED DIRS */

	if (!pendingDirs)
	{
		if (mForceCheck)
		{
			mForceCheck = false;
			fiMutex.unlock(); /* UNLOCKED DIRS */
			return true;
		}

		fiMutex.unlock(); /* UNLOCKED DIRS */
		return false;
	}
	
	mForceCheck = false;
	pendingDirs = false;
	pendingForceCacheWrite = true;
	
	/* clear old directories */
	directoryMap.clear();
	
	/* iterate through the directories */
	std::list<std::string>::iterator it;
	std::map<std::string, std::string>::const_iterator cit;
	for(it = pendingDirList.begin(); it != pendingDirList.end(); it++)
	{
		/* get the head directory */
		std::string root_dir = *it;
		std::string top_dir  = RsDirUtil::getTopDir(root_dir);
	
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
				/* add it! */
				directoryMap[tst_dir.c_str()] = root_dir;
				std::cerr << "Added [" << tst_dir << "] => " << root_dir << std::endl;
			}
		}
	}

	/* now we've decided on the 'root' dirs set them to the
	 * fileIndex
	 */
	std::list<std::string> topdirs;
	for(cit = directoryMap.begin(); cit != directoryMap.end(); cit++)
	{
		topdirs.push_back(cit->first);
	}

	fi.setRootDirectories(topdirs, 0);
	
	fiMutex.unlock(); /* UNLOCKED DIRS */

	return true;
}
	
	
	

/* lookup directory function */
std::string FileIndexMonitor::findRealRoot(std::string rootdir)
{
	/**** MUST ALREADY BE LOCKED ****/ 
	std::string realroot = "";

	std::map<std::string, std::string>::const_iterator cit;
	if (directoryMap.end()== (cit=directoryMap.find(rootdir)))
	{
		std::cerr << "FileIndexMonitor::findRealRoot() Invalid RootDir: ";
		std::cerr << rootdir << std::endl;
	}
	else
	{
		realroot = cit->second;
	}

	return realroot;
}



bool FileIndexMonitor::hashFile(std::string fullpath, FileEntry &fent)
{
	std::string f_hash = fullpath + "/" + fent.name;
	FILE *fd;
	int  len;
	SHA_CTX *sha_ctx = new SHA_CTX;
	unsigned char sha_buf[SHA_DIGEST_LENGTH];
	unsigned char gblBuf[512];

#ifdef FIM_DEBUG
	std::cerr << "File to hash = " << f_hash << std::endl;
#endif
	if (NULL == (fd = fopen(f_hash.c_str(), "rb")))	return false;

	SHA1_Init(sha_ctx);
	while((len = fread(gblBuf,1, 512, fd)) > 0)
	{
		SHA1_Update(sha_ctx, gblBuf, len);
	}

	/* reading failed for some reason */
	if (ferror(fd)) 
	{
		delete sha_ctx;
		fclose(fd);
		return false;
	}

	SHA1_Final(&sha_buf[0], sha_ctx);

	/* TODO: Actually we should store the hash data as binary ... 
	 * but then it shouldn't be put in a string.
	 */

        std::ostringstream tmpout;
	for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
	{
		tmpout << std::setw(2) << std::setfill('0') << std::hex << (unsigned int) (sha_buf[i]);
	}
	fent.hash = tmpout.str();

	delete sha_ctx;
	fclose(fd);
	return true;
}

