#include "util/rsdir.h"
#include "rsserver/p3face.h"
#include "pqi/authssl.h"
#include "hash_cache.h"
#include "filelist_io.h"

#define HASHSTORAGE_DEBUG 1

static const uint32_t DEFAULT_INACTIVITY_SLEEP_TIME = 50*1000;
static const uint32_t     MAX_INACTIVITY_SLEEP_TIME = 2*1000*1000;

HashStorage::HashStorage(const std::string& save_file_name)
    : mFilePath(save_file_name), mHashMtx("Hash Storage mutex")
{
    mRunning = false ;
    load() ;
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

            RsServer::notify()->notifyHashingInfo(NOTIFY_HASHTYPE_FINISH, "") ;

            usleep(mInactivitySleepTime);	// when no files to hash, just wait for 2 secs. This avoids a dramatic loop.
            mInactivitySleepTime *= 2;

            if(mInactivitySleepTime > MAX_INACTIVITY_SLEEP_TIME)
               mInactivitySleepTime = MAX_INACTIVITY_SLEEP_TIME;

            return ;
        }
        mInactivitySleepTime = DEFAULT_INACTIVITY_SLEEP_TIME;

        job = mFilesToHash.begin()->second ;

        std::cerr << "Hashing file " << job.full_path << "..." ; std::cerr.flush();

        std::string tmpout;
        //rs_sprintf(tmpout, "%lu/%lu (%s - %d%%) : %s", cnt+1, n_files, friendlyUnit(size).c_str(), int(size/double(total_size)*100.0), fe.name.c_str()) ;

        RsServer::notify()->notifyHashingInfo(NOTIFY_HASHTYPE_HASH_FILE, job.full_path) ;

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

void HashStorage::load()
{
   uint64_t file_size ;

    if(!RsDirUtil::checkFile( mFilePath,file_size,false ) )
    {
        std::cerr << "Encrypted hash cache file not present." << std::endl;
        return ;
    }

    // read the binary stream into memory.
    //
    void *buffer = rs_malloc(file_size) ;

    if(buffer == NULL)
       return ;

    FILE *F = fopen( mFilePath.c_str(),"rb") ;
    if (!F)
    {
       std::cerr << "Cannot open file for reading encrypted file cache, filename " << mFilePath << std::endl;
       free(buffer);
       return;
    }
    if(fread(buffer,1,file_size,F) != file_size)
    {
       std::cerr << "Cannot read from file " + mFilePath << ": something's wrong." << std::endl;
       free(buffer) ;
       fclose(F) ;
       return ;
    }
    fclose(F) ;

    // now decrypt
    void *decrypted_data =NULL;
    int decrypted_data_size =0;

    if(!AuthSSL::getAuthSSL()->decrypt(decrypted_data, decrypted_data_size, buffer, file_size))
    {
       std::cerr << "Cannot decrypt encrypted file cache. Something's wrong." << std::endl;
       free(buffer) ;
       return ;
    }
    free(buffer) ;

#ifdef HASHSTORAGE_DEBUG
    std::cerr << "Loading HashCache from file " << mFilePath << std::endl ;
    int n=0 ;
#endif
    unsigned char *data = (unsigned char*)decrypted_data ;
    uint32_t offset = 0 ;
    HashStorageInfo info ;

    while(offset < decrypted_data_size)
       if(readHashStorageInfo(data,decrypted_data_size,offset,info))
       {
#ifdef HASHSTORAGE_DEBUG
          std::cerr << info << std::endl;
          ++n ;
#endif
          mFiles[info.filename] = info ;
       }

    free(decrypted_data) ;
#ifdef HASHSTORAGE_DEBUG
    std::cerr << n << " entries loaded." << std::endl ;
#endif
}

void HashStorage::save()
{
#ifdef HASHSTORAGE_DEBUG
    std::cerr << "Saving Hash Cache to file " << mFilePath << "..." << std::endl ;
#endif

    unsigned char *data = NULL ;
    uint32_t offset = 0 ;
    uint32_t total_size = 0;

    for(std::map<std::string,HashStorageInfo>::const_iterator it(mFiles.begin());it!=mFiles.end();++it)
        writeHashStorageInfo(it->second,data,total_size,offset) ;

    void *encryptedData = NULL ;
    int encDataLen = 0 ;

    if(!AuthSSL::getAuthSSL()->encrypt( encryptedData, encDataLen, data,offset, AuthSSL::getAuthSSL()->OwnId()))
    {
        std::cerr << "Cannot encrypt hash cache. Something's wrong." << std::endl;
        return;
    }

    FILE *F = fopen( (mFilePath+".tmp").c_str(),"wb" ) ;

    if(!F)
    {
        std::cerr << "Cannot open encrypted file cache for writing: " << mFilePath+".tmp" << std::endl;
        goto save_free;
    }
    if(fwrite(encryptedData,1,encDataLen,F) != (uint32_t)encDataLen)
    {
        std::cerr << "Could not write entire encrypted hash cache file. Out of disc space??" << std::endl;
        fclose(F) ;
        goto save_free;
    }

    fclose(F) ;

    RsDirUtil::renameFile(mFilePath+".tmp",mFilePath) ;
#ifdef FIM_DEBUG
    std::cerr << "done." << std::endl ;
#endif

save_free:
    free(encryptedData);
}

bool HashStorage::readHashStorageInfo(const unsigned char *data,uint32_t total_size,uint32_t& offset,HashStorageInfo& info) const
{
    unsigned char *section_data = NULL ;
    uint32_t section_size = 0;

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


