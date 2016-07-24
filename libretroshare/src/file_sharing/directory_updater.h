// This class crawls the given directry hierarchy and updates it. It does so by calling the
// shared file list source. This source may be of two types:
// 	- local: directories are crawled n disk and files are hashed / requested from a cache
// 	- remote: directories are requested remotely to a providing client
//
#include "file_sharing/hash_cache.h"

class LocalDirectoryStorage ;

class DirectoryUpdater
{
	public:
		DirectoryUpdater() ;

		// Does some updating job. Crawls the existing directories and checks wether it has been updated
		// recently enough. If not, calls the directry source.
		//
        virtual void tick() =0;

		// 
};

class LocalDirectoryUpdater: public DirectoryUpdater, public HashCacheClient
{
public:
    LocalDirectoryUpdater(HashCache *hash_cache) ;
    virtual void tick() ;

protected:
    virtual void hash_callback(const std::string& full_name,const RsFileHash& hash) ;
    void recursUpdateSharedDir(const std::string& cumulated_path,DirectoryStorage::EntryIndex indx);

private:
    HashCache *mHashCache ;
    LocalDirectoryStorage *mSharedDirectories ;
};

class RemoteDirectoryUpdater: public DirectoryUpdater
{
    public:
        virtual void tick() ;
};
