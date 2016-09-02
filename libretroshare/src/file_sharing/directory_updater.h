// This class crawls the given directry hierarchy and updates it. It does so by calling the
// shared file list source. This source may be of two types:
// 	- local: directories are crawled n disk and files are hashed / requested from a cache
// 	- remote: directories are requested remotely to a providing client
//
#include "file_sharing/hash_cache.h"
#include "file_sharing/directory_storage.h"

class LocalDirectoryUpdater: public HashStorageClient, public RsTickingThread
{
public:
    LocalDirectoryUpdater(HashStorage *hash_cache,LocalDirectoryStorage *lds) ;
    virtual ~LocalDirectoryUpdater() {}

    virtual void forceUpdate();

    void setFileWatchPeriod(uint32_t seconds) { mDelayBetweenDirectoryUpdates = seconds ; }
    uint32_t fileWatchPeriod() const { return mDelayBetweenDirectoryUpdates ; }

protected:
    virtual void data_tick() ;

    virtual void hash_callback(uint32_t client_param, const std::string& name, const RsFileHash& hash, uint64_t size);
    void recursUpdateSharedDir(const std::string& cumulated_path,DirectoryStorage::EntryIndex indx);
    void sweepSharedDirectories();

private:
    HashStorage *mHashCache ;
    LocalDirectoryStorage *mSharedDirectories ;

    time_t mLastSweepTime;
    time_t mLastTSUpdateTime;

    uint32_t mDelayBetweenDirectoryUpdates;
};

