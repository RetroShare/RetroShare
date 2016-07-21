#pragma once

#include <map>
#include "util/rsthreads.h"

class HashCache
{
public:
    HashCache(const std::string& save_file_name) ;

    void insert(const std::string& full_path,uint64_t size,time_t time_stamp,const RsFileHash& hash) ;
    bool find(const  std::string& full_path,uint64_t size,time_t time_stamp,RsFileHash& hash) ;

    struct HashCacheInfo
    {
        std::string filename ;
        uint64_t size ;
        uint32_t time_stamp ;
        uint32_t modf_stamp ;
        RsFileHash hash ;
    } ;

    // interaction with GUI, called from p3FileLists
    void setRememberHashFilesDuration(uint32_t days) { _max_cache_duration_days = days ; }
    uint32_t rememberHashFilesDuration() const { return _max_cache_duration_days ; }
    void clear() { _files.clear(); }
    bool empty() const { return _files.empty() ; }

private:
    void clean() ;

    void save() ;
    void load() ;

    // threaded stuff
    RsTickingThread mHashingThread ;
    RsMutex mHashMtx ;

    // Local configuration and storage

    uint32_t mMaxCacheDurationDays ; // maximum duration of un-requested cache entries
    std::map<std::string, HashCacheInfo> mFiles ;
    std::string mFilePath ;
    bool mChanged ;

    // current work

    std::map<std::string> mFilesToHash ;
};

