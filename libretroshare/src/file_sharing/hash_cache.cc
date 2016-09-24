/*
 * RetroShare Hash cache
 *
 *     file_sharing/hash_cache.cc
 *
 * Copyright 2016 Mr.Alice
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
 * Please report all bugs and problems to "retroshare.project@gmail.com".
 *
 */
#include "util/rsdir.h"
#include "util/rsprint.h"
#include "rsserver/p3face.h"
#include "pqi/authssl.h"
#include "hash_cache.h"
#include "filelist_io.h"
#include "file_sharing_defaults.h"

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
    mMaxStorageDurationDays = DEFAULT_HASH_STORAGE_DURATION_DAYS ;

    {
        RS_STACK_MUTEX(mHashMtx) ;
        locked_load() ;
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

void HashStorage::data_tick()
{
    FileHashJob job;
    RsFileHash hash;
    uint64_t size ;

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

            usleep(st);	// when no files to hash, just wait for 2 secs. This avoids a dramatic loop.

            if(st > MAX_INACTIVITY_SLEEP_TIME)
            {
                RS_STACK_MUTEX(mHashMtx) ;

                mInactivitySleepTime = MAX_INACTIVITY_SLEEP_TIME;

                if(!mChanged)	// otherwise it might prevent from saving the hash cache
                {
                    std::cerr << "Stopping hashing thread." << std::endl;
                    shutdown();
                    mRunning = false ;
                    mTotalSizeToHash = 0;
                    mTotalFilesToHash = 0;
                    std::cerr << "done." << std::endl;
                }

                RsServer::notify()->notifyHashingInfo(NOTIFY_HASHTYPE_FINISH, "") ;
            }
            else
            {
                RS_STACK_MUTEX(mHashMtx) ;
                mInactivitySleepTime = 2*st ;
            }

            return ;
        }
        mInactivitySleepTime = DEFAULT_INACTIVITY_SLEEP_TIME;

        {
            RS_STACK_MUTEX(mHashMtx) ;

            job = mFilesToHash.begin()->second ;
            mFilesToHash.erase(mFilesToHash.begin()) ;
        }

        if(job.client->hash_confirm(job.client_param))
        {
            std::cerr << "Hashing file " << job.full_path << "..." ; std::cerr.flush();

            std::string tmpout;
            rs_sprintf(tmpout, "%lu/%lu (%s - %d%%) : %s", (unsigned long int)mHashCounter+1, (unsigned long int)mTotalFilesToHash, friendlyUnit(mTotalHashedSize).c_str(), int(mTotalHashedSize/double(mTotalSizeToHash)*100.0), job.full_path.c_str()) ;

            RsServer::notify()->notifyHashingInfo(NOTIFY_HASHTYPE_HASH_FILE, tmpout) ;

            if(!RsDirUtil::getFileHash(job.full_path, hash,size, this))
                std::cerr << "ERROR: cannot hash file " << job.full_path << std::endl;
            else
                std::cerr << "done."<< std::endl;

            // store the result

            {
                RS_STACK_MUTEX(mHashMtx) ;
                HashStorageInfo& info(mFiles[job.full_path]);

                info.filename = job.full_path ;
                info.size = size ;
                info.modf_stamp = job.ts ;
                info.time_stamp = time(NULL);
                info.hash = hash;

                mChanged = true ;
                ++mHashCounter ;
                mTotalHashedSize += size ;
            }
        }
    }
    // call the client

    if(!hash.isNull())
        job.client->hash_callback(job.client_param, job.full_path, hash, size);
}

bool HashStorage::requestHash(const std::string& full_path,uint64_t size,time_t mod_time,RsFileHash& known_hash,HashStorageClient *c,uint32_t client_param)
{
    // check if the hash is up to date w.r.t. cache.

#ifdef HASHSTORAGE_DEBUG
    std::cerr << "HASH Requested for file " << full_path << ": ";
#endif
    RS_STACK_MUTEX(mHashMtx) ;

    time_t now = time(NULL) ;
    std::map<std::string,HashStorageInfo>::iterator it = mFiles.find(full_path) ;

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
        if(it->second.time_stamp != (uint64_t)mod_time)
        {
            std::cerr << "(WW) detected a 1 hour shift in file modification time. This normally happens to many files at once, when daylight saving time shifts (file=\"" << full_path << "\")." << std::endl;
            it->second.time_stamp = (uint64_t)mod_time;
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

    if(mFilesToHash.find(full_path) != mFilesToHash.end())
        return false ;

    FileHashJob job ;

    job.client = c ;
    job.size = size ;
    job.client_param = client_param ;
    job.full_path = full_path ;
    job.ts = mod_time ;

    mFilesToHash[full_path] = job;

    mTotalSizeToHash += size ;
    ++mTotalFilesToHash;

    if(!mRunning)
    {
        mRunning = true ;
        std::cerr << "Starting hashing thread." << std::endl;
        mHashCounter = 0;
        mTotalHashedSize = 0;

        start() ;
    }

    return false;
}

void HashStorage::clean()
{
    RS_STACK_MUTEX(mHashMtx) ;

    time_t now = time(NULL) ;
    time_t duration = mMaxStorageDurationDays * 24 * 3600 ; // seconds

#ifdef HASHSTORAGE_DEBUG
    std::cerr << "Cleaning hash cache." << std::endl ;
#endif

    for(std::map<std::string,HashStorageInfo>::iterator it(mFiles.begin());it!=mFiles.end();)
        if(it->second.time_stamp + duration < (uint64_t)now)
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

void HashStorage::locked_load()
{
    unsigned char *data = NULL ;
    uint32_t data_size=0;

    if(!FileListIO::loadEncryptedDataFromFile(mFilePath,data,data_size))
    {
        std::cerr << "(EE) Cannot read hash cache." << std::endl;
        return ;
    }
    uint32_t offset = 0 ;
    HashStorageInfo info ;
    uint32_t n=0;

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
    unsigned char *section_data = NULL ;
    uint32_t section_size = 0;
    uint32_t section_offset = 0;

    // This way, the entire section is either read or skipped. That avoids the risk of being stuck somewhere in the middle
    // of a section because of some unknown field, etc.

    if(!FileListIO::readField(data,total_size,offset,FILE_LIST_IO_TAG_HASH_STORAGE_ENTRY,section_data,section_size))
       return false;

    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,info.filename  )) return false ;
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,info.size      )) return false ;
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_UPDATE_TS     ,info.time_stamp)) return false ;
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,info.modf_stamp)) return false ;
    if(!FileListIO::readField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,info.hash      )) return false ;

    free(section_data);
    return true;
}

bool HashStorage::writeHashStorageInfo(unsigned char *& data,uint32_t&  total_size,uint32_t& offset,const HashStorageInfo& info) const
{
    unsigned char *section_data = NULL ;
    uint32_t section_offset = 0 ;
    uint32_t section_size = 0;

    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_NAME     ,info.filename  )) return false ;
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_SIZE     ,info.size      )) return false ;
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_UPDATE_TS     ,info.time_stamp)) return false ;
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_MODIF_TS      ,info.modf_stamp)) return false ;
    if(!FileListIO::writeField(section_data,section_size,section_offset,FILE_LIST_IO_TAG_FILE_SHA1_HASH,info.hash      )) return false ;

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
    return o << info.hash << " " << info.size << " " << info.filename ;
}
