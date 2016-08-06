#include "util/folderiterator.h"
#include "rsserver/p3face.h"

#include "directory_storage.h"
#include "directory_updater.h"

#define DEBUG_LOCAL_DIR_UPDATER 1

static const uint32_t DELAY_BETWEEN_DIRECTORY_UPDATES = 100 ; // 10 seconds for testing. Should be much more!!

void RemoteDirectoryUpdater::tick()
{
	// use the stored iterator
}

LocalDirectoryUpdater::LocalDirectoryUpdater(HashStorage *hc,LocalDirectoryStorage *lds)
    : mHashCache(hc),mSharedDirectories(lds)
{
    mLastSweepTime = 0;
}

void LocalDirectoryUpdater::data_tick()
{
    time_t now = time(NULL) ;

    if(now > DELAY_BETWEEN_DIRECTORY_UPDATES + mLastSweepTime)
    {
        sweepSharedDirectories() ;
        mLastSweepTime = now;
    }
    else
        usleep(10*1000*1000);
}

void LocalDirectoryUpdater::forceUpdate()
{
    mLastSweepTime = 0;
}

void LocalDirectoryUpdater::sweepSharedDirectories()
{
    RsServer::notify()->notifyListPreChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);

    std::cerr << "LocalDirectoryUpdater::sweep()" << std::endl;

	// recursive update algorithm works that way:
	// 	- the external loop starts on the shared directory list and goes through sub-directories
    // 	- at the same time, it updates the local list of shared directories.  A single sweep is performed over the whole directory structure.
	// 	- the information that is costly to compute (the hash) is store externally into a separate structure.
	// 	- doing so, changing directory names or moving files between directories does not cause a re-hash of the content. 
	//
    std::list<SharedDirInfo> shared_directory_list ;
    mSharedDirectories->getSharedDirectoryList(shared_directory_list);

    std::set<std::string> sub_dir_list ;

    for(std::list<SharedDirInfo>::const_iterator real_dir_it(shared_directory_list.begin());real_dir_it!=shared_directory_list.end();++real_dir_it)
        sub_dir_list.insert( (*real_dir_it).filename ) ;

    // make sure that entries in stored_dir_it are the same than paths in real_dir_it, and in the same order.

    mSharedDirectories->updateSubDirectoryList(mSharedDirectories->root(),sub_dir_list) ;

    // now for each of them, go recursively and match both files and dirs

    DirectoryStorage::DirIterator stored_dir_it(mSharedDirectories,mSharedDirectories->root()) ;

    for(std::list<SharedDirInfo>::const_iterator real_dir_it(shared_directory_list.begin());real_dir_it!=shared_directory_list.end();++real_dir_it, ++stored_dir_it)
    {
        std::cerr << "  recursing into " << real_dir_it->filename << std::endl;
        recursUpdateSharedDir(real_dir_it->filename, *stored_dir_it) ;
    }
    RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
}

void LocalDirectoryUpdater::recursUpdateSharedDir(const std::string& cumulated_path, DirectoryStorage::EntryIndex indx)
{
    std::cerr << "  parsing directory " << cumulated_path << ", index=" << indx << std::endl;

    // make sure list of subdirs is the same
    // make sure list of subfiles is the same
    // request all hashes to the hashcache

    librs::util::FolderIterator dirIt(cumulated_path);

    if(!dirIt.isValid())
    {
        mSharedDirectories->removeDirectory(indx) ;	// this is a complex operation since it needs to *update* it so that it is kept consistent.
        return ;
    }

    // collect subdirs and subfiles

    std::map<std::string,DirectoryStorage::FileTS> subfiles ;
    std::set<std::string> subdirs ;

    for(;dirIt.isValid();dirIt.next())
    {
        switch(dirIt.file_type())
        {
                case librs::util::FolderIterator::TYPE_FILE:	subfiles[dirIt.file_name()].modtime = dirIt.file_modtime() ;
                                                            subfiles[dirIt.file_name()].size = dirIt.file_size();
                                                            std::cerr << "  adding sub-file \"" << dirIt.file_name() << "\"" << std::endl;
                                                                break;

                case librs::util::FolderIterator::TYPE_DIR:  subdirs.insert(dirIt.file_name()) ;
                                                             std::cerr << "  adding sub-dir \"" << dirIt.file_name() << "\"" << std::endl;
                                                            break;
        default:
            std::cerr << "(EE) Dir entry of unknown type with path \"" << cumulated_path << "/" << dirIt.file_name() << "\"" << std::endl;
        }
    }
    // update file and dir lists for current directory.

    mSharedDirectories->updateSubDirectoryList(indx,subdirs) ;

    std::map<std::string,DirectoryStorage::FileTS> new_files ;
    mSharedDirectories->updateSubFilesList(indx,subfiles,new_files) ;

    // now go through list of subfiles and request the hash to hashcache

    for(DirectoryStorage::FileIterator dit(mSharedDirectories,indx);dit;++dit)
    {
        // ask about the hash. If not present, ask HashCache. If not present, or different, the callback will update it.

        RsFileHash hash ;

        if(mHashCache->requestHash(cumulated_path + "/" + dit.name(),dit.size(),dit.modtime(),hash,this,*dit) && dit.hash() != hash)
                mSharedDirectories->updateHash(*dit,hash);
    }

    // go through the list of sub-dirs and recursively update

    DirectoryStorage::DirIterator stored_dir_it(mSharedDirectories,indx) ;

    for(std::set<std::string>::const_iterator real_dir_it(subdirs.begin());real_dir_it!=subdirs.end();++real_dir_it, ++stored_dir_it)
        recursUpdateSharedDir(cumulated_path + "/" + *real_dir_it, *stored_dir_it) ;
}

void LocalDirectoryUpdater::hash_callback(uint32_t client_param, const std::string& name, const RsFileHash& hash, uint64_t size)
{
    if(!mSharedDirectories->updateHash(DirectoryStorage::EntryIndex(client_param),hash))
        std::cerr << "(EE) Cannot update file. Something's wrong." << std::endl;
}

