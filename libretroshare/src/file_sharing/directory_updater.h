/*
 * RetroShare C++ Directory parsing code.
 *
 *      file_sharing/directory_updater.h
 *
 * Copyright 2016 by Mr.Alice
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

    void forceUpdate();
    bool inDirectoryCheck() const ;
	void togglePauseHashingProcess();
	bool hashingProcessPaused();

    void setHashSalt(const RsFileHash& hash) { mHashSalt = hash; }
    const RsFileHash& hashSalt() const { return mHashSalt; }

    void setFileWatchPeriod(int seconds) ;
    uint32_t fileWatchPeriod() const ;

    void setFollowSymLinks(bool b) ;
    bool followSymLinks() const ;

    void setEnabled(bool b) ;
    bool isEnabled() const ;

    void setIgnoreLists(const std::list<std::string>& ignored_prefixes,const std::list<std::string>& ignored_suffixes,uint32_t ignore_flags) ;
    bool getIgnoreLists(std::list<std::string>& ignored_prefixes,std::list<std::string>& ignored_suffixes,uint32_t& ignore_flags) const ;

	void setMaxShareDepth(uint32_t i) ;
	int  maxShareDepth() const;

	void setIgnoreDuplicates(bool b) ;
	bool ignoreDuplicates() const;

protected:
    virtual void data_tick() ;

    virtual void hash_callback(uint32_t client_param, const std::string& name, const RsFileHash& hash, uint64_t size);
    virtual bool hash_confirm(uint32_t client_param) ;

    void recursUpdateSharedDir(const std::string& cumulated_path, DirectoryStorage::EntryIndex indx, std::set<std::string>& existing_directories, uint32_t current_depth);
    bool sweepSharedDirectories();

private:
	bool filterFile(const std::string& fname) const ;	// reponds true if the file passes the ignore lists test.

    HashStorage *mHashCache ;
    LocalDirectoryStorage *mSharedDirectories ;

    RsFileHash mHashSalt ;

    time_t mLastSweepTime;
    time_t mLastTSUpdateTime;

    uint32_t mDelayBetweenDirectoryUpdates;
    bool mIsEnabled ;
    bool mFollowSymLinks;
    bool mIgnoreDuplicates;
    bool mNeedsFullRecheck ;
    bool mIsChecking ;
    bool mForceUpdate ;

	uint32_t mIgnoreFlags ;
	uint32_t mMaxShareDepth ;

	std::list<std::string> mIgnoredPrefixes ;
	std::list<std::string> mIgnoredSuffixes ;
};

