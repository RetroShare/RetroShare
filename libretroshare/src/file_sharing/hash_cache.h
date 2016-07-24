#pragma once

#include <map>
#include "util/rsthreads.h"
#include "retroshare/rsfiles.h"

class HashCacheClient
{
public:
    virtual void hash_callback(const std::string& full_path,const RsFileHash& hash) ;
};

class FileHashingThread: public RsTickingThread
{
public:
    FileHashingThread() {}

    virtual void data_tick() ;
};

class HashCache
{
public:
    HashCache(const std::string& save_file_name) ;

    bool requestHash(const  std::string& full_path,uint64_t size,time_t mod_time,const RsFileHash& known_hash,HashCacheClient *c) ;

    struct HashCacheInfo
    {
        std::string filename ;
        uint64_t size ;
        uint32_t time_stamp ;
        uint32_t modf_stamp ;
        RsFileHash hash ;
    } ;

    // interaction with GUI, called from p3FileLists
    void setRememberHashFilesDuration(uint32_t days) { mMaxCacheDurationDays = days ; }
    uint32_t rememberHashFilesDuration() const { return mMaxCacheDurationDays ; }
    void clear() { mFiles.clear(); }
    bool empty() const { return mFiles.empty() ; }

private:
    void clean() ;

    void save() ;
    void load() ;

    // threaded stuff
    FileHashingThread mHashingThread ;
    RsMutex mHashMtx ;

    // Local configuration and storage

    uint32_t mMaxCacheDurationDays ; // maximum duration of un-requested cache entries
    std::map<std::string, HashCacheInfo> mFiles ;
    std::string mFilePath ;
    bool mChanged ;

    // current work

    std::map<std::string,HashCacheClient *> mFilesToHash ;
};

