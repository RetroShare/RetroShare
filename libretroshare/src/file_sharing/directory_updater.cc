#include "directory_storage.h"
#include "directory_updater.h"

void RemoteDirectoryUpdater::tick()
{
	// use the stored iterator
}

LocalDirectoryUpdater::LocalDirectoryUpdater()
{
    // tell the hash cache that we're the client
    mHashCache->setClient(this) ;
}

void LocalDirectoryUpdater::tick()
{
	// recursive update algorithm works that way:
	// 	- the external loop starts on the shared directory list and goes through sub-directories
    // 	- at the same time, it updates the local list of shared directories.  A single sweep is performed over the whole directory structure.
	// 	- the information that is costly to compute (the hash) is store externally into a separate structure.
	// 	- doing so, changing directory names or moving files between directories does not cause a re-hash of the content. 
	//
    std::list<SharedDirInfo> shared_directory_list ;

    for(std::list<SharedDirInfo>::const_iterator real_dir_it(shared_directory_list.begin());it!=shared_directory_list.end();++it)
        sub_dir_list.push_back( (*it).filename ) ;

    // make sure that entries in stored_dir_it are the same than paths in real_dir_it, and in the same order.

    mSharedDirectories->updateSubDirectoryList(mSharedDirectories->root(),sub_dir_list) ;

    // now for each of them, go recursively and match both files and dirs

    DirectoryStorage::DirIterator stored_dir_it(mLocalSharedDirs.root()) ;

    for(std::list<SharedDirInfo>::const_iterator real_dir_it(shared_directory_list.begin());it!=shared_directory_list.end();++it, ++stored_dir_it)
        recursUpdateSharedDir(*real_dir_it, *stored_dir_it) ;
}

LocalDirectoryUpdater::recursUpdateSharedDir(const std::string& cumulated_path,const DirectoryStorage::EntryIndex indx)
{
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

    std::list<std::string> subfiles ;
    std::list<std::string> subdirs ;

    while(dirIt.readdir())
    {
        std::string name ;
        uint64_t size ;
        uint8_t type ;
        time_t mod_time ;

        dirIt.readEntryInformation(type,name,size,mod_time) ;

        switch(type)
        {
                case librs::util::FolderIterator::TYPE_FILE: subfiles.push_back(name) ;
                                                        break;
                case librs::util::FolderIterator::TYPE_DIR:  subdirs.push_back(name) ;
                                                            break;

        default:
            std::cerr << "(EE) Dir entry of unknown type with path \"" << cumulated_path << "/" << name << "\"" << std::endl;
        }
    }
    // update file and dir lists for current directory.

    mSharedDirectories->updateSubDirectoryList(indx,subdirs) ;
    mSharedDirectories->updateSubFilesList(indx,subfiles) ;

    // now go through list of subfiles and request the hash to hashcache

    for(DirectoryStorage::FileIterator dit(indx);dit;++dit)
    {
        // ask about the hash. If not present, ask HashCache. If not present, the callback will update it.

        if()
            if(mHashCache.requestHash(realpath,name,size,modtime,hash),this)
                mSharedDirectories->updateFileHash(*dit,hash) ;
    }

    // go through the list of sub-dirs and recursively update

    DirectoryStorage::DirIterator stored_dir_it(indx) ;

    for(std::list<std::string>::const_iterator real_dir_it(subdirs.begin());it!=subdirs.end();++real_dir_it, ++stored_dir_it)
        recursUpdateSharedDir(*real_dir_it, *stored_dir_it) ;
}

