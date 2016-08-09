#pragma once

#include <map>
#include "util/rsthreads.h"
#include "retroshare/rsfiles.h"

class HashStorageClient
{
public:
    HashStorageClient() {}
    virtual ~HashStorageClient() {}

    virtual void hash_callback(uint32_t client_param, const std::string& name, const RsFileHash& hash, uint64_t size)=0;
};

class HashStorage: public RsTickingThread
{
public:
    HashStorage(const std::string& save_file_name) ;

    /*!
     * \brief requestHash  Requests the hash for the given file, assuming size and mod_time are the same.
     *
     * \param full_path    Full path to reach the file
     * \param size         Actual file size
     * \param mod_time     Actual file modification time
     * \param known_hash   Returned hash for the file.
     * \param c            Hash cache client to which the hash should be sent once calculated
     * \param client_param Param to be passed to the client callback
     *
     * \return true if the supplied hash info is up to date.
     */
    bool requestHash(const  std::string& full_path, uint64_t size, time_t mod_time, RsFileHash& known_hash, HashStorageClient *c, uint32_t client_param) ;

    struct HashStorageInfo
    {
        std::string filename ;		// full path of the file
        uint64_t size ;
        uint32_t time_stamp ;		// last time the hash was tested/requested
        uint32_t modf_stamp ;
        RsFileHash hash ;
    } ;

    // interaction with GUI, called from p3FileLists
    void setRememberHashFilesDuration(uint32_t days) { mMaxStorageDurationDays = days ; }
    uint32_t rememberHashFilesDuration() const { return mMaxStorageDurationDays ; }
    void clear() { mFiles.clear(); }
    bool empty() const { return mFiles.empty() ; }

    // Functions called by the thread

    virtual void data_tick() ;

    friend std::ostream& operator<<(std::ostream& o,const HashStorageInfo& info) ;
private:
    void clean() ;

    void save() ;
    void load() ;

    bool readHashStorageInfo(const unsigned char *data,uint32_t total_size,uint32_t& offset,HashStorageInfo& info) const;
    bool writeHashStorageInfo(unsigned char *& data,uint32_t&  total_size,uint32_t& offset,const HashStorageInfo& info) const;

    // Local configuration and storage

    uint32_t mMaxStorageDurationDays ; 				// maximum duration of un-requested cache entries
    std::map<std::string, HashStorageInfo> mFiles ;		// stored as (full_path, hash_info)
    std::string mFilePath ;
    bool mChanged ;

    struct FileHashJob
    {
        std::string full_path;
        HashStorageClient *client;
        uint32_t client_param ;
        time_t ts;
    };

    // current work

    std::map<std::string,FileHashJob> mFilesToHash ;

    // thread/mutex stuff

    RsMutex mHashMtx ;
    bool mRunning;
    uint32_t mInactivitySleepTime = 50*1000 ;
};

