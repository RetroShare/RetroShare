#include "util/rsdir.h"
#include "hash_cache.h"

#define HASHSTORAGE_DEBUG 1

HashStorage::HashStorage(const std::string& save_file_name)
    : mFilePath(save_file_name), mHashMtx("Hash Storage mutex")
{
    mRunning = false ;
}

void HashStorage::data_tick()
{
    FileHashJob job;
    RsFileHash hash;
    uint64_t size ;

    {
        RS_STACK_MUTEX(mHashMtx) ;

        if(mFilesToHash.empty())
        {
            std::cerr << "Stopping hashing thread." << std::endl;
            shutdown();
            mRunning = false ;
            std::cerr << "done." << std::endl;

            usleep(2*1000*1000);	// when no files to hash, just wait for 2 secs. This avoids a dramatic loop.
            return ;
        }

        job = mFilesToHash.begin()->second ;

        std::cerr << "Hashing file " << job.full_path << "..." ; std::cerr.flush();


        if(!RsDirUtil::getFileHash(job.full_path, hash,size, this))
            std::cerr << "ERROR: cannot hash file " << job.full_path << std::endl;
        else
            std::cerr << "done."<< std::endl;

        mFilesToHash.erase(mFilesToHash.begin()) ;

        // store the result

        HashStorageInfo& info(mFiles[job.full_path]);

        info.filename = job.full_path ;
        info.size = size ;
        info.modf_stamp = job.ts ;
        info.time_stamp = time(NULL);
        info.hash = hash;
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

    if(it != mFiles.end() && (uint64_t)mod_time == it->second.modf_stamp && size == it->second.size)
    {
        it->second.time_stamp = now ;
        known_hash = it->second.hash;
#ifdef HASHSTORAGE_DEBUG
        std::cerr << "Found in cache." << std::endl ;
#endif
        return true ;
    }
#ifdef HASHSTORAGE_DEBUG
    std::cerr << "Not in cache. Sceduling for re-hash." << std::endl ;
#endif

    // we need to schedule a re-hashing

    if(mFilesToHash.find(full_path) != mFilesToHash.end())
        return false ;

    FileHashJob job ;

    job.client = c ;
    job.client_param = client_param ;
    job.full_path = full_path ;
    job.ts = mod_time ;

    mFilesToHash[full_path] = job;

    if(!mRunning)
    {
        mRunning = true ;
        std::cerr << "Starting hashing thread." << std::endl;
        start() ;
    }

    return false;
}

void HashStorage::clean()
{
#ifdef HASHSTORAGE_DEBUG
    std::cerr << "Cleaning HashStorage..." << std::endl ;
#endif
    time_t now = time(NULL) ;
    time_t duration = mMaxStorageDurationDays * 24 * 3600 ; // seconds

#ifdef HASHSTORAGE_DEBUG
    std::cerr << "cleaning hash cache." << std::endl ;
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

