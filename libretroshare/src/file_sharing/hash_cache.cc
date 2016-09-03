#include "util/rsdir.h"
#include "rsserver/p3face.h"
#include "pqi/authssl.h"
#include "hash_cache.h"
#include "filelist_io.h"
#include "file_sharing_defaults.h"

#define HASHSTORAGE_DEBUG 1

static const uint32_t DEFAULT_INACTIVITY_SLEEP_TIME = 50*1000;
static const uint32_t     MAX_INACTIVITY_SLEEP_TIME = 2*1000*1000;

HashStorage::HashStorage(const std::string& save_file_name)
    : mFilePath(save_file_name), mHashMtx("Hash Storage mutex")
{
    mInactivitySleepTime = DEFAULT_INACTIVITY_SLEEP_TIME;
    mRunning = false ;
    mLastSaveTime = 0 ;

    {
        RS_STACK_MUTEX(mHashMtx) ;
        locked_load() ;
    }
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
            std::cerr << "nothing to hash. Sleeping for " << st << " us" << std::endl;

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

        std::cerr << "Hashing file " << job.full_path << "..." ; std::cerr.flush();

        std::string tmpout;
        //rs_sprintf(tmpout, "%lu/%lu (%s - %d%%) : %s", cnt+1, n_files, friendlyUnit(size).c_str(), int(size/double(total_size)*100.0), fe.name.c_str()) ;

        RsServer::notify()->notifyHashingInfo(NOTIFY_HASHTYPE_HASH_FILE, job.full_path) ;

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
    std::cerr << "Not in cache. Scheduling for re-hash." << std::endl ;
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
    RS_STACK_MUTEX(mHashMtx) ;

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

    std::cerr << mFiles.size() << " Entries saved." << std::endl;

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

    free(section_data) ;

    return true;
}

std::ostream& operator<<(std::ostream& o,const HashStorage::HashStorageInfo& info)
{
    return o << info.hash << " " << info.size << " " << info.filename ;
}
