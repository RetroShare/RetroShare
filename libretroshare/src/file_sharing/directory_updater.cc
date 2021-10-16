/*******************************************************************************
 * libretroshare/src/file_sharing: directory_updater.cc                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2016 by Mr.Alice <mralice@users.sourceforge.net>                  *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/
#include "util/folderiterator.h"
#include "util/rstime.h"
#include "rsserver/p3face.h"

#include "directory_storage.h"
#include "directory_updater.h"
#include "file_sharing_defaults.h"

//#define DEBUG_LOCAL_DIR_UPDATER 1

//=============================================================================================================//
//                                           Local Directory Updater                                           //
//=============================================================================================================//

LocalDirectoryUpdater::LocalDirectoryUpdater(HashStorage *hc,LocalDirectoryStorage *lds)
    : mHashCache(hc), mSharedDirectories(lds)
    , mLastSweepTime(0), mLastTSUpdateTime(0)
    , mDelayBetweenDirectoryUpdates(DELAY_BETWEEN_DIRECTORY_UPDATES)
    , mIsEnabled(false), mFollowSymLinks(FOLLOW_SYMLINKS_DEFAULT)
    , mIgnoreDuplicates(true)
    /* Can be left to false, but setting it to true will force to re-hash any file that has been left unhashed in the last session.*/
    , mNeedsFullRecheck(true)
    , mIsChecking(false), mForceUpdate(false), mIgnoreFlags (0),  mMaxShareDepth(0)
{
}

bool LocalDirectoryUpdater::isEnabled() const
{
    return mIsEnabled ;
}
void LocalDirectoryUpdater::setEnabled(bool b)
{
	if(mIsEnabled == b) return;
	if(!b) RsThread::askForStop();
	else if(!RsThread::isRunning()) start("fs dir updater");
	mIsEnabled = b ;
}

void LocalDirectoryUpdater::threadTick()
{
    rstime_t now = time(NULL) ;

    if (mIsEnabled || mForceUpdate)
    {
        if(now > mDelayBetweenDirectoryUpdates + mLastSweepTime)
        {
            bool some_files_not_ready = false ;

            auto ev = std::make_shared<RsSharedDirectoriesEvent>();
            ev->mEventCode = RsSharedDirectoriesEventCode::STARTING_DIRECTORY_SWEEP;
            if(rsEvents)
                rsEvents->postEvent(ev);
            if(sweepSharedDirectories(some_files_not_ready))
            {
                if(some_files_not_ready)
                {
					mNeedsFullRecheck = true ;
					mLastSweepTime = now - mDelayBetweenDirectoryUpdates + 60 ; // retry 20 secs from now

					std::cerr << "(II) some files being modified. Will re-scan in 60 secs." << std::endl;
                }
				else
                {
					mNeedsFullRecheck = false ;
					mLastSweepTime = now ;
                }

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

	for(uint32_t i=0;i<10;++i)
	{
		rstime::rs_usleep(1*1000*1000);

		{
		if(mForceUpdate)
			break ;
		}
	}
}

void LocalDirectoryUpdater::forceUpdate(bool add_safe_delay)
{
    mForceUpdate = true ;
	mLastSweepTime = rstime_t(time(NULL)) - rstime_t(mDelayBetweenDirectoryUpdates) ;

    if(add_safe_delay)
        mLastSweepTime += rstime_t(MIN_TIME_AFTER_LAST_MODIFICATION);

	if(mHashCache != NULL && mHashCache->hashingProcessPaused())
		mHashCache->togglePauseHashingProcess();
}

bool LocalDirectoryUpdater::sweepSharedDirectories(bool& some_files_not_ready)
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
		existing_dirs.insert(RsDirUtil::removeSymLinks(stored_dir_it.name()));

        recursUpdateSharedDir(stored_dir_it.name(), *stored_dir_it,existing_dirs,1,some_files_not_ready) ;		// here we need to use the list that was stored, instead of the shared dir list, because the two
                                                                            									// are not necessarily in the same order.
    }

    RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
    mIsChecking = false ;
    auto ev = std::make_shared<RsSharedDirectoriesEvent>();
    ev->mEventCode = RsSharedDirectoriesEventCode::DIRECTORY_SWEEP_ENDED;
    if(rsEvents)
        rsEvents->postEvent(ev);

    return true ;
}

void LocalDirectoryUpdater::recursUpdateSharedDir(const std::string& cumulated_path, DirectoryStorage::EntryIndex indx,std::set<std::string>& existing_directories,uint32_t current_depth,bool& some_files_not_ready)
{
#ifdef DEBUG_LOCAL_DIR_UPDATER
    std::cerr << "[directory storage]   parsing directory " << cumulated_path << ", index=" << indx << std::endl;
#endif

    // make sure list of subdirs is the same
    // make sure list of subfiles is the same
    // request all hashes to the hashcache

    librs::util::FolderIterator dirIt(cumulated_path,mFollowSymLinks,false);	// disallow symbolic links and files from the future.

    rstime_t dir_local_mod_time ;
    if(!mSharedDirectories->getDirectoryLocalModTime(indx,dir_local_mod_time))
    {
        std::cerr << "(EE) Cannot get local mod time for dir index " << indx << std::endl;
        return;
    }

    rstime_t now = time(NULL) ;

    if(mNeedsFullRecheck || dirIt.dir_modtime() > dir_local_mod_time)	// the > is because we may have changed the virtual name, and therefore the TS wont match.
																		// we only want to detect when the directory has changed on the disk
    {
       // collect subdirs and subfiles

       std::map<std::string,DirectoryStorage::FileTS> subfiles ;
       std::set<std::string> subdirs ;

       for(;dirIt.isValid();dirIt.next())
		   if(filterFile(dirIt.file_name()))
		   {
			   switch(dirIt.file_type())
			   {
			   case librs::util::FolderIterator::TYPE_FILE:

                   if(dirIt.file_modtime() + MIN_TIME_AFTER_LAST_MODIFICATION < now)
				   {
					   subfiles[dirIt.file_name()].modtime = dirIt.file_modtime() ;
					   subfiles[dirIt.file_name()].size = dirIt.file_size();
#ifdef DEBUG_LOCAL_DIR_UPDATER
					   std::cerr << "  adding sub-file \"" << dirIt.file_name() << "\"" << std::endl;
#endif
				   }
                   else
                   {
                       some_files_not_ready = true ;

                       std::cerr << "(WW) file " << dirIt.file_fullpath() << " is probably being written to. Keeping it for later." << std::endl;
                   }

				   break;

			   case librs::util::FolderIterator::TYPE_DIR:
			   {
				   bool dir_is_accepted = true ;

				   if( (mMaxShareDepth > 0u && current_depth > mMaxShareDepth) || (mMaxShareDepth==0 && current_depth >= 64))	// 64 is here as a safe limit, to make loops impossible.
					   dir_is_accepted = false ;

				   if(dir_is_accepted && mFollowSymLinks && mIgnoreDuplicates)
				   {
					   std::string real_path = RsDirUtil::removeSymLinks(cumulated_path + "/" + dirIt.file_name()) ;

					   if(existing_directories.end() != existing_directories.find(real_path))
					   {
						   std::cerr << "(WW) Directory " << cumulated_path << " has real path " << real_path << " which already belongs to another shared directory. Ignoring" << std::endl;
						   dir_is_accepted = false ;
					   }
					   else
						   existing_directories.insert(real_path) ;
				   }

				   if(dir_is_accepted)
					   subdirs.insert(dirIt.file_name());

#ifdef DEBUG_LOCAL_DIR_UPDATER
				   std::cerr << "  adding sub-dir \"" << dirIt.file_name() << "\"" << std::endl;
#endif
			   }
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
			recursUpdateSharedDir(cumulated_path + "/" + stored_dir_it.name(), *stored_dir_it,existing_directories,current_depth+1,some_files_not_ready) ;
		}
}

bool LocalDirectoryUpdater::filterFile(const std::string& fname) const
{
	if(mIgnoreFlags & RS_FILE_SHARE_FLAGS_IGNORE_SUFFIXES)
		for(auto it(mIgnoredSuffixes.begin());it!=mIgnoredSuffixes.end();++it)
			if(fname.size() >= (*it).size() && fname.substr( fname.size() - (*it).size()) == *it)
			{
				std::cerr << "(II) ignoring file " << fname << ", because it matches suffix \"" << *it << "\"" << std::endl;
				return false ;
			}

	if(mIgnoreFlags & RS_FILE_SHARE_FLAGS_IGNORE_PREFIXES)
		for(auto it(mIgnoredPrefixes.begin());it!=mIgnoredPrefixes.end();++it)
			if(fname.size() >= (*it).size() && fname.substr( 0,(*it).size()) == *it)
			{
				std::cerr << "(II) ignoring file " << fname << ", because it matches prefix \"" << *it << "\"" << std::endl;
				return false ;
			}

	return true ;
}

void LocalDirectoryUpdater::togglePauseHashingProcess()
{
	mHashCache->togglePauseHashingProcess() ;
}
bool LocalDirectoryUpdater::hashingProcessPaused()
{
	return mHashCache->hashingProcessPaused();
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

    forceUpdate(false);
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

int LocalDirectoryUpdater::maxShareDepth() const
{
	return mMaxShareDepth ;
}

void LocalDirectoryUpdater::setMaxShareDepth(uint32_t d)
{
	if(d != mMaxShareDepth)
        mNeedsFullRecheck = true ;

	mMaxShareDepth = d ;
}

bool LocalDirectoryUpdater::ignoreDuplicates() const
{
	return mIgnoreDuplicates;
}

void LocalDirectoryUpdater::setIgnoreDuplicates(bool b)
{
	if(b != mIgnoreDuplicates)
        mNeedsFullRecheck = true ;

	mIgnoreDuplicates = b ;
}
