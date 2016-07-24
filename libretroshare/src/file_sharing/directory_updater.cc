#include "util/folderiterator.h"

#include "directory_storage.h"
#include "directory_updater.h"

#define DEBUG_LOCAL_DIR_UPDATER 1

void RemoteDirectoryUpdater::tick()
{
	// use the stored iterator
}

LocalDirectoryUpdater::LocalDirectoryUpdater(HashCache *hc)
    : mHashCache(hc)
{
}

void LocalDirectoryUpdater::tick()
{
    std::cerr << "LocalDirectoryUpdater::tick()" << std::endl;

	// recursive update algorithm works that way:
	// 	- the external loop starts on the shared directory list and goes through sub-directories
    // 	- at the same time, it updates the local list of shared directories.  A single sweep is performed over the whole directory structure.
	// 	- the information that is costly to compute (the hash) is store externally into a separate structure.
	// 	- doing so, changing directory names or moving files between directories does not cause a re-hash of the content. 
	//
    std::list<SharedDirInfo> shared_directory_list ;
    std::list<std::string> sub_dir_list ;

    for(std::list<SharedDirInfo>::const_iterator real_dir_it(shared_directory_list.begin());real_dir_it!=shared_directory_list.end();++real_dir_it)
        sub_dir_list.push_back( (*real_dir_it).filename ) ;

    // make sure that entries in stored_dir_it are the same than paths in real_dir_it, and in the same order.

    mSharedDirectories->updateSubDirectoryList(*mSharedDirectories->root(),sub_dir_list) ;

    // now for each of them, go recursively and match both files and dirs

    DirectoryStorage::DirIterator stored_dir_it(mSharedDirectories->root()) ;

    for(std::list<SharedDirInfo>::const_iterator real_dir_it(shared_directory_list.begin());real_dir_it!=shared_directory_list.end();++real_dir_it, ++stored_dir_it)
        recursUpdateSharedDir(real_dir_it->filename, *stored_dir_it) ;
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

    std::list<std::string> subfiles ;
    std::list<std::string> subdirs ;

    while(dirIt.readdir())
    {
        switch(dirIt.file_type())
        {
                case librs::util::FolderIterator::TYPE_FILE:	subfiles.push_back(dirIt.file_name()) ;
                                                                std::cerr << "  adding sub-file \"" << dirIt.file_name() << "\"" << std::endl;
                                                            break;

                case librs::util::FolderIterator::TYPE_DIR:	subdirs.push_back(dirIt.file_name()) ;
                                                                std::cerr << "  adding sub-dir \"" << dirIt.file_name() << "\"" << std::endl;
                                                            break;

        default:
            std::cerr << "(EE) Dir entry of unknown type with path \"" << cumulated_path << "/" << dirIt.file_name() << "\"" << std::endl;
        }
    }
    // update file and dir lists for current directory.

    mSharedDirectories->updateSubDirectoryList(indx,subdirs) ;
    mSharedDirectories->updateSubFilesList(indx,subfiles) ;

    // now go through list of subfiles and request the hash to hashcache

    for(DirectoryStorage::FileIterator dit(indx);dit;++dit)
    {
        // ask about the hash. If not present, ask HashCache. If not present, or different, the callback will update it.

        mHashCache->requestHash(dit.fullpath(),dit.size(),dit.modtime(),dit.hash(),this) ;
    }

    // go through the list of sub-dirs and recursively update

    DirectoryStorage::DirIterator stored_dir_it(indx) ;

    for(std::list<std::string>::const_iterator real_dir_it(subdirs.begin());real_dir_it!=subdirs.end();++real_dir_it, ++stored_dir_it)
        recursUpdateSharedDir(*real_dir_it, *stored_dir_it) ;
}

