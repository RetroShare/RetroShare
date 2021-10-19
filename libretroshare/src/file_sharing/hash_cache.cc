/*******************************************************************************
 * libretroshare/src/file_sharing: hash_cache.cc                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 by Mr.Alice <mralice@users.sourceforge.net>                  *
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
 ******************************************************************************/
#include "util/rsdir.h"
#include "util/rsprint.h"
#include "util/rstime.h"
#include "rsserver/p3face.h"
#include "pqi/authssl.h"
#include "hash_cache.h"
#include "filelist_io.h"
#include "file_sharing_defaults.h"
#include "retroshare/rsinit.h"

//#define HASHSTORAGE_DEBUG 1

static const uint32_t DEFAULT_INACTIVITY_SLEEP_TIME = 50*1000;
static const uint32_t     MAX_INACTIVITY_SLEEP_TIME = 2*1000*1000;

HashStorage::HashStorage(const std::string& save_file_name)
    : mFilePath(save_file_name), mHashMtx("Hash Storage mutex")
{
    mInactivitySleepTime = DEFAULT_INACTIVITY_SLEEP_TIME;
    mRunning = false ;
    mLastSaveTime = 0 ;
    mTotalSizeToHash = 0;
    mTotalFilesToHash = 0;
	mCurrentHashingSpeed = 0 ;
    mMaxStorageDurationDays = DEFAULT_HASH_STORAGE_DURATION_DAYS ;
	mHashingProcessPaused = false;
	mHashedBytes = 0 ;
	mHashingTime = 0 ;

    {
        RS_STACK_MUTEX(mHashMtx) ;

        if(!locked_load())
            try_load_import_old_hash_cache();
    }
}

void HashStorage::togglePauseHashingProcess()
{
	RS_STACK_MUTEX(mHashMtx) ;
	mHashingProcessPaused = !mHashingProcessPaused ;
}
bool HashStorage::hashingProcessPaused()
{
	RS_STACK_MUTEX(mHashMtx) ;
	return mHashingProcessPaused;
}

static std::string friendlyUnit(uint64_t val)
{
    const std::string units[6] = {"B","KB","MB","GB","TB","PB"};
    char buf[50] ;

    double fact = 1.0 ;

    for(unsigned int i=0; i<6; ++i)
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

void HashStorage::threadTick()
{
    FileHashJob job;
    RsFileHash hash;
    uint64_t size = 0;


    {
        bool empty ;
        uint32_t st ;

        {
            RS_STACK_MUTEX(mHashMtx) ;

            if(mChanged && mLastSaveTime + MIN_INTERVAL_BETWEEN_HASH_CACHE_SAVE < time(NULL))
            {
                locked_save();
                mLastSaveTime = time(NULL) ;
                mChanged = false ;
            }
        }

        {
            RS_STACK_MUTEX(mHashMtx) ;

            empty = mFilesToHash.empty();
            st = mInactivitySleepTime ;
        }

        // sleep off mutex!
        if(empty)
        {
#ifdef HASHSTORAGE_DEBUG
            std::cerr << "nothing to hash. Sleeping for " << st << " us" << std::endl;
#endif

            rstime::rs_usleep(st);	// when no files to hash, just wait for 2 secs. This avoids a dramatic loop.

            if(st > MAX_INACTIVITY_SLEEP_TIME)
            {
                RS_STACK_MUTEX(mHashMtx) ;

                mInactivitySleepTime = MAX_INACTIVITY_SLEEP_TIME;

                if(!mChanged)	// otherwise it might prevent from saving the hash cache
                {
                    stopHashThread();
                }

                if(rsEvents)
                {
                    auto ev = std::make_shared<RsSharedDirectoriesEvent>();
                    ev->mEventCode = RsSharedDirectoriesEventCode::DIRECTORY_SWEEP_ENDED;
                    rsEvents->postEvent(ev);
                }
                //RsServer::notify()->notifyHashingInfo(NOTIFY_HASHTYPE_FINISH, "") ;
            }
            else
            {
                RS_STACK_MUTEX(mHashMtx) ;
                mInactivitySleepTime = 2*st ;
            }

            return ;
        }
        mInactivitySleepTime = DEFAULT_INACTIVITY_SLEEP_TIME;

		bool paused = false ;
        {
            RS_STACK_MUTEX(mHashMtx) ;
			paused = mHashingProcessPaused ;
		}

		if(paused)	// we need to wait off mutex!!
		{
			rstime::rs_usleep(MAX_INACTIVITY_SLEEP_TIME) ;
			std::cerr << "Hashing process currently paused." << std::endl;
			return;
		}
		else
        {
            RS_STACK_MUTEX(mHashMtx) ;

            job = mFilesToHash.begin()->second ;
            mFilesToHash.erase(mFilesToHash.begin()) ;
        }

        if(job.client->hash_confirm(job.client_param))
        {
#ifdef HASHSTORAGE_DEBUG
            std::cerr << "Hashing file " << job.full_path << "..." ; std::cerr.flush();
#endif

            std::string tmpout;

			if(mCurrentHashingSpeed > 0)
				rs_sprintf(tmpout, "%lu/%lu (%s - %d%%, %d MB/s) : %s", (unsigned long int)mHashCounter+1, (unsigned long int)mTotalFilesToHash, friendlyUnit(mTotalHashedSize).c_str(), int(mTotalHashedSize/double(mTotalSizeToHash)*100.0), mCurrentHashingSpeed,job.full_path.c_str()) ;
			else
				rs_sprintf(tmpout, "%lu/%lu (%s - %d%%) : %s", (unsigned long int)mHashCounter+1, (unsigned long int)mTotalFilesToHash, friendlyUnit(mTotalHashedSize).c_str(), int(mTotalHashedSize/double(mTotalSizeToHash)*100.0), job.full_path.c_str()) ;

			{
				/* Emit deprecated event only for retrocompatibility
				 * TODO: create a proper event with structured data instead of a
				 * formatted string */
				auto ev = std::make_shared<RsSharedDirectoriesEvent>();
				ev->mEventCode = RsSharedDirectoriesEventCode::HASHING_FILE;
				ev->mMessage = tmpout;
				rsEvents->postEvent(ev);
			}

			double seconds_origin = rstime::RsScopeTimer::currentTime() ;

			if(RsDirUtil::getFileHash(job.full_path, hash, size, this))
			{
				// store the result

#ifdef HASHSTORAGE_DEBUG
				std::cerr << "done."<< std::endl;
#endif

				RS_STACK_MUTEX(mHashMtx) ;
				HashStorageInfo& info(mFiles[job.real_path]);

				info.filename = job.real_path ;
				info.size = size ;
				info.modf_stamp = job.ts ;
				info.time_stamp = time(NULL);
				info.hash = hash;

				mChanged = true ;
				mTotalHashedSize += size ;
			}
			else RS_ERR("Failure hashing file: ", job.full_path);

			mHashingTime += rstime::RsScopeTimer::currentTime() - seconds_origin ;
			mHashedBytes += size ;

			if(mHashingTime > 3)
			{
				mCurrentHashingSpeed = (int)(mHashedBytes / mHashingTime ) / (1024*1024) ;
				mHashingTime = 0 ;
				mHashedBytes = 0 ;
			}

            ++mHashCounter ;
        }
	}

	// call the client
	if(!hash.isNull())
		job.client->hash_callback(job.client_param, job.full_path, hash, size);

	/* Notify we completed hashing a file */
	auto ev = std::make_shared<RsFileHashingCompletedEvent>();
	ev->mFilePath = job.full_path;
	ev->mHashingSpeed = mCurrentHashingSpeed;
	ev->mFileHash = hash;
	rsEvents->postEvent(ev);
}

bool HashStorage::requestHash(const std::string& full_path,uint64_t size,rstime_t mod_time,RsFileHash& known_hash,HashStorageClient *c,uint32_t client_param)
{
    // check if the hash is up to date w.r.t. cache.

#ifdef HASHSTORAGE_DEBUG
    std::cerr << "HASH Requested for file " << full_path << ": mod_time: " << mod_time << ", size: " << size << " :" ;
#endif
    RS_STACK_MUTEX(mHashMtx) ;

	std::string real_path = RsDirUtil::removeSymLinks(full_path) ;

    rstime_t now = time(NULL) ;
    std::map<std::string,HashStorageInfo>::iterator it = mFiles.find(real_path) ;

    // On windows we compare the time up to +/- 3600 seconds. This avoids re-hashing files in case of daylight saving change.
    //
    // See:
    //		 https://support.microsoft.com/en-us/kb/190315
    //
    if(it != mFiles.end()
#ifdef WINDOWS_SYS
            && ( (uint64_t)mod_time == it->second.modf_stamp || (uint64_t)mod_time+3600 == it->second.modf_stamp ||(uint64_t)mod_time == it->second.modf_stamp+3600)
#else
            && (uint64_t)mod_time == it->second.modf_stamp
#endif
            && size == it->second.size)
    {
        it->second.time_stamp = now ;

#ifdef WINDOWS_SYS
        if(it->second.modf_stamp != (uint64_t)mod_time)
        {
            std::cerr << "(WW) detected a 1 hour shift in file modification time. This normally happens to many files at once, when daylight saving time shifts (file=\"" << full_path << "\")." << std::endl;
            it->second.modf_stamp = (uint64_t)mod_time;
            mChanged = true;
            startHashThread();
        }
#endif

        known_hash = it->second.hash;
#ifdef HASHSTORAGE_DEBUG
        std::cerr << "Found in cache." << std::endl ;
#endif
        return true ;
    }
#ifdef HASHSTORAGE_DEBUG
    std::cerr << "Not in cache. Scheduling for re-hash." << std::endl ;
#endif

    // we need to schedule a re-hashing

    if(mFilesToHash.find(real_path) != mFilesToHash.end())
        return false ;

    FileHashJob job ;

    job.client = c ;
    job.size = size ;
    job.client_param = client_param ;
    job.full_path = full_path ;
    job.real_path = real_path ;
    job.ts = mod_time ;

	// We store the files indexed by their real path, so that we allow to not re-hash files that are pointed multiple times through the directory links
	// The client will be notified with the full path instead of the real path.

    mFilesToHash[real_path] = job;

    mTotalSizeToHash += size ;
    ++mTotalFilesToHash;

    startHashThread();

    return false;
}

void HashStorage::startHashThread()
{
    if(!mRunning)
    {
        mRunning = true ;
        std::cerr << "Starting hashing thread." << std::endl;
        mHashCounter = 0;
        mTotalHashedSize = 0;

        start("fs hash cache") ;
    }
}

void HashStorage::stopHashThread()
{
	if(mRunning)
	{
		RsInfo() << __PRETTY_FUNCTION__ << "Stopping hashing thread."
		         << std::endl;

		RsThread::askForStop();
        mRunning = false ;
        mTotalSizeToHash = 0;
        mTotalFilesToHash = 0;
    }
}

void HashStorage::clean()
{
    RS_STACK_MUTEX(mHashMtx) ;

    rstime_t now = time(NULL) ;
    rstime_t duration = mMaxStorageDurationDays * 24 * 3600 ; // seconds

#ifdef HASHSTORAGE_DEBUG
    std::cerr << "Cleaning hash cache." << std::endl ;
#endif

    for(std::map<std::string,HashStorageInfo>::iterator it(mFiles.begin());it!=mFiles.end();)
		if((uint64_t)(it->second.time_stamp + duration) < (uint64_t)now)
        {
#ifdef HASHSTORAGE_DEBUG
            std::cerr << "  Entry too old: " << it->first << ", ts=" << it->second.time_stamp << std::endl ;
#endif
            std::map<std::string,HashStorageInfo>::iterator tmp(it) ;
            ++tmp ;
            mFiles.erase(it) ;
            it=tmp ;
            mChanged = true ;
        }
        else
            ++it ;

#ifdef HASHSTORAGE_DEBUG
    std::cerr << "Done." << std::endl;
#endif
}

bool HashStorage::locked_load()
{
    unsigned char *data = NULL ;
    uint32_t data_size=0;

    if(!FileListIO::loadEncryptedDataFromFile(mFilePath,data,data_size))
    {
        std::cerr << "(EE) Cannot read hash cache." << std::endl;
        return false;
    }
    uint32_t offset = 0 ;
    HashStorageInfo info ;
#ifdef HASHSTORAGE_DEBUG
    uint32_t n=0;
#endif

    while(offset < data_size)
       if(readHashStorageInfo(data,data_size,offset,info))
       {
#ifdef HASHSTORAGE_DEBUG
          std::cerr << info << std::endl;
          ++n ;
#endif
          mFiles[info.filename] = info ;
       }

    free(data) ;
#ifdef HASHSTORAGE_DEBUG
    std::cerr << n << " entries loaded." << std::endl ;
#endif
    return true ;
}

void HashStorage::locked_save()
{
#ifdef HASHSTORAGE_DEBUG
    std::cerr << "Saving Hash Cache to file " << mFilePath << "..." << std::endl ;
#endif

    unsigned char *data = NULL ;
    uint32_t offset = 0 ;
    uint32_t total_size = 0;

    for(std::map<std::string,HashStorageInfo>::const_iterator it(mFiles.begin());it!=mFiles.end();++it)
        writeHashStorageInfo(data,total_size,offset,it->second) ;

    if(!FileListIO::saveEncryptedDataToFile(mFilePath,data,offset))
    {
        std::cerr << "(EE) Cannot save hash cache data." << std::endl;
        free(data) ;
        return ;
    }

    std::cerr << mFiles.size() << " entries saved in hash cache." << std::endl;

    free(data) ;
}

bool HashStorage::readHashStorageInfo(const unsigned char *data,uint32_t total_size,uint32_t& offset,HashStorageInfo& info) const
{
    unsigned char *section_data = (unsigned char *)rs_malloc(FL_BASE_TMP_SECTION_SIZE) ;

    if(!section_data)
        return false ;

    uint32_t section_size = FL_BASE_TMP_SECTION_SIZE;
    uint32_t section_offset = 0;

    // This way, the entire section is either read or skipped. That avoids the risk of being stuck somewhere in the middle
    // of a section because of some unknown field, etc.

    if(!FileListIO::readField(data,total_size,offset,FILE_LIST_IO_TAG_HASH_STORAGE_ENTRY,section_data,section_size))
	{
		free(section_data);
		return false;
	}

    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,info.filename  )) { free(section_data); return false ; }
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,info.size      )) { free(section_data); return false ; }
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_UPDATE_TS     ,info.time_stamp)) { free(section_data); return false ; }
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,info.modf_stamp)) { free(section_data); return false ; }
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,info.hash      )) { free(section_data); return false ; }

    free(section_data);
    return true;
}

bool HashStorage::writeHashStorageInfo(unsigned char *& data,uint32_t&  total_size,uint32_t& offset,const HashStorageInfo& info) const
{
    unsigned char *section_data = (unsigned char *)rs_malloc(FL_BASE_TMP_SECTION_SIZE) ;

    if(!section_data)
        return false ;

    uint32_t section_offset = 0 ;
    uint32_t section_size = FL_BASE_TMP_SECTION_SIZE;

    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,info.filename  )) { free(section_data); return false ; }
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,info.size      )) { free(section_data); return false ; }
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_UPDATE_TS     ,info.time_stamp)) { free(section_data); return false ; }
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,info.modf_stamp)) { free(section_data); return false ; }
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,info.hash      )) { free(section_data); return false ; }

    // now write the whole string into a single section in the file

    if(!FileListIO::writeField(data,total_size,offset,FILE_LIST_IO_TAG_HASH_STORAGE_ENTRY,section_data,section_offset)) return false ;
#ifdef HASHSTORAGE_DEBUG
    std::cerr << "Writing hash storage section " << RsUtil::BinToHex(section_data,section_offset) << std::endl;
    std::cerr << "Info.filename = " << info.filename << std::endl;
#endif
    free(section_data) ;

    return true;
}

std::ostream& operator<<(std::ostream& o,const HashStorage::HashStorageInfo& info)
{
    return o << info.hash << " " << info.modf_stamp << " " << info.size << " " << info.filename ;
}

/********************************************************************************************************************************/
/* 09/26/2016                                                                                                                   */
/* This method should be removed in a few months. It only helps importing the old hash cache in order to                        */
/* spare the re-hashing of everything. If successful, it also renames the old hash cache into a new name so that                */
/* the file is not found at the next attempt.                                                                                   */
/********************************************************************************************************************************/
//
#include "rsserver/rsaccounts.h"
#include <sstream>

bool HashStorage::try_load_import_old_hash_cache()
{
    // compute file name

    std::string base_dir = RsAccounts::AccountDirectory();
    std::string old_cache_filename = base_dir + "/" + "file_cache.bin" ;

    // check for unencrypted

    std::istream *f = NULL ;
    uint64_t file_size ;

    if(RsDirUtil::checkFile( old_cache_filename,file_size,false ) )
    {
        std::cerr << "Encrypted hash cache file present. Reading it." << std::endl;

        // read the binary stream into memory.
        //
        RsTemporaryMemory buffer(file_size) ;

        if(buffer == NULL)
            return false;

        FILE *F = fopen( old_cache_filename.c_str(),"rb") ;
        if (!F)
        {
            std::cerr << "Cannot open file for reading encrypted file cache, filename " << old_cache_filename << std::endl;
            return false;
        }
        if(fread(buffer,1,file_size,F) != file_size)
        {
            std::cerr << "Cannot read from file " << old_cache_filename << ": something's wrong." << std::endl;
            fclose(F) ;
            return false;
        }
        fclose(F) ;

        void *decrypted_data =NULL;
        int decrypted_data_size =0;

        if(!AuthSSL::getAuthSSL()->decrypt(decrypted_data, decrypted_data_size, buffer, file_size))
        {
            std::cerr << "Cannot decrypt encrypted file cache. Something's wrong." << std::endl;
            return false;
        }
        std::string s((char *)decrypted_data,decrypted_data_size) ;
        f = new std::istringstream(s) ;

        free(decrypted_data) ;
    }
    else
        return false;

    std::streamsize max_line_size = 2000 ; // should be enough. Anyway, if we
                                                        // miss one entry, we just lose some
                                                        // cache itemsn but this is not too
                                                        // much of a problem.
    char *buff = new char[max_line_size] ;

    std::cerr << "Importing hashCache from file " << old_cache_filename << std::endl ;
    int n=0 ;

    std::map<std::string, HashStorageInfo> tmp_files ;	// stored as (full_path, hash_info)

    while(!f->eof())
    {
        HashStorageInfo info ;

        f->getline(buff,max_line_size,'\n') ;
        info.filename = std::string(buff) ;

        f->getline(buff,max_line_size,'\n') ; //if(sscanf(buff,"%llu",&info.size) != 1) break ;

        info.size = 0 ;
        sscanf(buff, RsDirUtil::scanf_string_for_uint(sizeof(info.size)), &info.size);

        f->getline(buff,max_line_size,'\n') ; if(sscanf(buff,RsDirUtil::scanf_string_for_uint(sizeof(info.time_stamp)),&info.time_stamp) != 1) { std::cerr << "Could not read one entry! Giving up." << std::endl; break ; }
        f->getline(buff,max_line_size,'\n') ; if(sscanf(buff,RsDirUtil::scanf_string_for_uint(sizeof(info.modf_stamp)),&info.modf_stamp) != 1) { std::cerr << "Could not read one entry! Giving up." << std::endl; break ; }
        f->getline(buff,max_line_size,'\n') ; info.hash = RsFileHash(std::string(buff)) ;

        std::cerr << "  (" << info.filename << ", " << info.size << ", " << info.time_stamp << ", " << info.modf_stamp << ", " << info.hash << std::endl ;
        ++n ;

        tmp_files[info.filename] = info ;
    }

    delete[] buff ;
    delete f ;

    std::cerr << "(II) Successfully imported hash cache from file " << old_cache_filename << " for a total of " << n << " entries." << std::endl;
    std::cerr << "(II) This file is now obsolete, and will be renamed " << old_cache_filename + ".bak" << " in case you needed it. But you can safely remove it." << std::endl;

    RsDirUtil::renameFile(old_cache_filename,old_cache_filename+".bak") ;

    mFiles = tmp_files ;
    locked_save() ;		// this is called explicitly here because the ticking thread is not active.

    return true;
}
/********************************************************************************************************************************/
