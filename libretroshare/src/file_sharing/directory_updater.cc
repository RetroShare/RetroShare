/*
 * RetroShare Directory watching system.
 *
 *      file_sharing/directory_updater.cc
 *
 * Copyright 2016 Mr.Alice
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare.project@gmail.com".
 *
 */
#include "util/folderiterator.h"
#include "rsserver/p3face.h"

#include "directory_storage.h"
#include "directory_updater.h"
#include "file_sharing_defaults.h"

//#define DEBUG_LOCAL_DIR_UPDATER 1

//=============================================================================================================//
//                                           Local Directory Updater                                           //
//=============================================================================================================//

LocalDirectoryUpdater::LocalDirectoryUpdater(HashStorage *hc,LocalDirectoryStorage *lds)
    : mHashCache(hc),mSharedDirectories(lds)
{
    mLastSweepTime = 0;
    mLastTSUpdateTime = 0;

    mDelayBetweenDirectoryUpdates = DELAY_BETWEEN_DIRECTORY_UPDATES;
    mIsEnabled = false ;
    mFollowSymLinks = FOLLOW_SYMLINKS_DEFAULT ;

    // Can be left to false, but setting it to true will force to re-hash any file that has been left unhashed in the last session.

    mNeedsFullRecheck = true ;
    mIsChecking = false ;
    mForceUpdate = false ;
}

bool LocalDirectoryUpdater::isEnabled() const
{
    return mIsEnabled ;
}
void LocalDirectoryUpdater::setEnabled(bool b)
{
    if(mIsEnabled == b)
        return ;

    if(!b)
        shutdown();
    else if(!isRunning())
        start("fs dir updater") ;

    mIsEnabled = b ;
}

void LocalDirectoryUpdater::data_tick()
{
    time_t now = time(NULL) ;

    if (mIsEnabled || mForceUpdate)
    {
        if(now > mDelayBetweenDirectoryUpdates + mLastSweepTime)
        {
            if(sweepSharedDirectories())
            {
                mNeedsFullRecheck = false;
                mLastSweepTime = now ;
                mSharedDirectories->notifyTSChanged();
                mForceUpdate = false ;
            }
            else
                std::cerr << "(WW) sweepSharedDirectories() failed. Will do it again in a short time." << std::endl;
        }

        if(now > DELAY_BETWEEN_LOCAL_DIRECTORIES_TS_UPDATE + mLastTSUpdateTime)
        {
            mSharedDirectories->updateTimeStamps() ;
            mLastTSUpdateTime = now ;
        }
    }

    usleep(10*1000*1000);
}

void LocalDirectoryUpdater::forceUpdate()
{
    mForceUpdate = true ;
	mLastSweepTime = 0 ;
}

bool LocalDirectoryUpdater::sweepSharedDirectories()
{
    if(mHashSalt.isNull())
    {
        std::cerr << "(EE) no salt value in LocalDirectoryUpdater. Is that a bug?" << std::endl;
        return false;
    }

    mIsChecking = true ;

    RsServer::notify()->notifyListPreChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
#ifdef DEBUG_LOCAL_DIR_UPDATER
    std::cerr << "[directory storage] LocalDirectoryUpdater::sweep()" << std::endl;
#endif

    // recursive update algorithm works that way:
    // 	- the external loop starts on the shared directory list and goes through sub-directories
    // 	- at the same time, it updates the local list of shared directories.  A single sweep is performed over the whole directory structure.
    // 	- the information that is costly to compute (the hash) is store externally into a separate structure.
    // 	- doing so, changing directory names or moving files between directories does not cause a re-hash of the content.
    //
    std::list<SharedDirInfo> shared_directory_list ;
    mSharedDirectories->getSharedDirectoryList(shared_directory_list);

    std::set<std::string> sub_dir_list ;

    // We re-check that each dir actually exists. It might have been removed from the disk.

    for(std::list<SharedDirInfo>::const_iterator real_dir_it(shared_directory_list.begin());real_dir_it!=shared_directory_list.end();++real_dir_it)
        if(RsDirUtil::checkDirectory( (*real_dir_it).filename ) )
		        sub_dir_list.insert( (*real_dir_it).filename ) ;

    // make sure that entries in stored_dir_it are the same than paths in real_dir_it, and in the same order.

    mSharedDirectories->updateSubDirectoryList(mSharedDirectories->root(),sub_dir_list,mHashSalt) ;

    // now for each of them, go recursively and match both files and dirs

    std::set<std::string> existing_dirs ;

    for(DirectoryStorage::DirIterator stored_dir_it(mSharedDirectories,mSharedDirectories->root()) ; stored_dir_it;++stored_dir_it)
    {
#ifdef DEBUG_LOCAL_DIR_UPDATER
        std::cerr << "[directory storage]   recursing into " << stored_dir_it.name() << std::endl;
#endif

        recursUpdateSharedDir(stored_dir_it.name(), *stored_dir_it,existing_dirs) ;		// here we need to use the list that was stored, instead of the shared dir list, because the two
                                                                            // are not necessarily in the same order.
    }

    RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
    mIsChecking = false ;

    return true ;
}

void LocalDirectoryUpdater::recursUpdateSharedDir(const std::string& cumulated_path, DirectoryStorage::EntryIndex indx,std::set<std::string>& existing_directories)
{
#ifdef DEBUG_LOCAL_DIR_UPDATER
    std::cerr << "[directory storage]   parsing directory " << cumulated_path << ", index=" << indx << std::endl;
#endif

    if(mFollowSymLinks)
	{
		std::string real_path = RsDirUtil::removeSymLinks(cumulated_path) ;
		if(existing_directories.end() != existing_directories.find(real_path))
		{
            std::cerr << "(WW) Directory " << cumulated_path << " has real path " << real_path << " which already belongs to another shared directory. Ignoring" << std::endl;
			return ;
		}
		existing_directories.insert(real_path) ;
	}

    // make sure list of subdirs is the same
    // make sure list of subfiles is the same
    // request all hashes to the hashcache

    librs::util::FolderIterator dirIt(cumulated_path,mFollowSymLinks,false);	// disallow symbolic links and files from the future.

    time_t dir_local_mod_time ;
    if(!mSharedDirectories->getDirectoryLocalModTime(indx,dir_local_mod_time))
    {
        std::cerr << "(EE) Cannot get local mod time for dir index " << indx << std::endl;
        return;
    }

    if(mNeedsFullRecheck || dirIt.dir_modtime() > dir_local_mod_time)	// the > is because we may have changed the virtual name, and therefore the TS wont match.
																		// we only want to detect when the directory has changed on the disk
    {
       // collect subdirs and subfiles

       std::map<std::string,DirectoryStorage::FileTS> subfiles ;
       std::set<std::string> subdirs ;

       for(;dirIt.isValid();dirIt.next())
       {
          switch(dirIt.file_type())
          {
          case librs::util::FolderIterator::TYPE_FILE:	subfiles[dirIt.file_name()].modtime = dirIt.file_modtime() ;
             subfiles[dirIt.file_name()].size = dirIt.file_size();
#ifdef DEBUG_LOCAL_DIR_UPDATER
             std::cerr << "  adding sub-file \"" << dirIt.file_name() << "\"" << std::endl;
#endif
             break;

          case librs::util::FolderIterator::TYPE_DIR:  	subdirs.insert(dirIt.file_name());
#ifdef DEBUG_LOCAL_DIR_UPDATER
             std::cerr << "  adding sub-dir \"" << dirIt.file_name() << "\"" << std::endl;
#endif
             break;
          default:
             std::cerr << "(EE) Dir entry of unknown type with path \"" << cumulated_path << "/" << dirIt.file_name() << "\"" << std::endl;
          }
       }
       // update folder modificatoin time, which is the only way to detect e.g. removed or renamed files.

       mSharedDirectories->setDirectoryLocalModTime(indx,dirIt.dir_modtime()) ;

       // update file and dir lists for current directory.

       mSharedDirectories->updateSubDirectoryList(indx,subdirs,mHashSalt) ;

       std::map<std::string,DirectoryStorage::FileTS> new_files ;
       mSharedDirectories->updateSubFilesList(indx,subfiles,new_files) ;

       // now go through list of subfiles and request the hash to hashcache

       for(DirectoryStorage::FileIterator dit(mSharedDirectories,indx);dit;++dit)
		   if(filterFile(dit.name()))
		   {
			   // ask about the hash. If not present, ask HashCache. If not present, or different, the callback will update it.

			   RsFileHash hash ;

			   // mSharedDirectories does two things: store H(F), and compute H(H(F)), which is used in FT. The later is always needed.

			   if(mHashCache->requestHash(cumulated_path + "/" + dit.name(),dit.size(),dit.modtime(),hash,this,*dit))
				   mSharedDirectories->updateHash(*dit,hash,hash != dit.hash());
		   }
    }
#ifdef DEBUG_LOCAL_DIR_UPDATER
    else
        std::cerr << "  directory is unchanged. Keeping existing files and subdirs list." << std::endl;
#endif

    // go through the list of sub-dirs and recursively update

    for(DirectoryStorage::DirIterator stored_dir_it(mSharedDirectories,indx) ; stored_dir_it; ++stored_dir_it)
    {
#ifdef DEBUG_LOCAL_DIR_UPDATER
        std::cerr << "  recursing into " << stored_dir_it.name() << std::endl;
#endif
        recursUpdateSharedDir(cumulated_path + "/" + stored_dir_it.name(), *stored_dir_it,existing_directories) ;
    }
}

bool LocalDirectoryUpdater::filterFile(const std::string& fname) const
{
	if(mIgnoreFlags & RS_FILE_SHARE_FLAGS_IGNORE_SUFFIXES)
		for(auto it(mIgnoredSuffixes.begin());it!=mIgnoredSuffixes.end();++it)
			if(fname.size() >= (*it).size() && fname.substr( fname.size() - (*it).size()) == *it)
				return false ;

	if(mIgnoreFlags & RS_FILE_SHARE_FLAGS_IGNORE_PREFIXES)
		for(auto it(mIgnoredPrefixes.begin());it!=mIgnoredPrefixes.end();++it)
			if(fname.size() >= (*it).size() && fname.substr( 0,(*it).size()) == *it)
				return false ;

	return true ;
}

bool LocalDirectoryUpdater::inDirectoryCheck() const
{
    return mHashCache->isRunning();
}

void LocalDirectoryUpdater::hash_callback(uint32_t client_param, const std::string &/*name*/, const RsFileHash &hash, uint64_t /*size*/)
{
    if(!mSharedDirectories->updateHash(DirectoryStorage::EntryIndex(client_param),hash,true))
        std::cerr << "(EE) Cannot update file. Something's wrong." << std::endl;

    mSharedDirectories->notifyTSChanged() ;
}

bool LocalDirectoryUpdater::hash_confirm(uint32_t client_param)
{
    return mSharedDirectories->getEntryType(DirectoryStorage::EntryIndex(client_param)) == DIR_TYPE_FILE ;
}

void LocalDirectoryUpdater::setFileWatchPeriod(int seconds)
{
    mDelayBetweenDirectoryUpdates = seconds ;
}
uint32_t LocalDirectoryUpdater::fileWatchPeriod() const
{
    return mDelayBetweenDirectoryUpdates ;
}

void LocalDirectoryUpdater::setFollowSymLinks(bool b)
{
    if(b != mFollowSymLinks)
        mNeedsFullRecheck = true ;

    mFollowSymLinks = b ;

    forceUpdate();
}

bool LocalDirectoryUpdater::followSymLinks() const
{
    return mFollowSymLinks ;
}

void LocalDirectoryUpdater::setIgnoreLists(const std::list<std::string>& ignored_prefixes,const std::list<std::string>& ignored_suffixes,uint32_t ignore_flags)
{
	mIgnoredPrefixes = ignored_prefixes ;
	mIgnoredSuffixes = ignored_suffixes ;
	mIgnoreFlags = ignore_flags;
}
bool LocalDirectoryUpdater::getIgnoreLists(std::list<std::string>& ignored_prefixes,std::list<std::string>& ignored_suffixes,uint32_t& ignore_flags) const
{
	 ignored_prefixes = mIgnoredPrefixes;
	 ignored_suffixes = mIgnoredSuffixes;
	 ignore_flags     = mIgnoreFlags    ;

	 return true;
}


