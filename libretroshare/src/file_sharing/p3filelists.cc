/*******************************************************************************
 * libretroshare/src/file_sharing: p3filelists.cc                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 by Mr.Alice <mralice@users.sourceforge.net>                  *
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
#include "rsitems/rsserviceids.h"

#include "file_sharing/p3filelists.h"
#include "file_sharing/directory_storage.h"
#include "file_sharing/directory_updater.h"
#include "file_sharing/rsfilelistitems.h"
#include "file_sharing/file_sharing_defaults.h"

#include "retroshare/rsids.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsinit.h"
#include "util/cxx17retrocompat.h"
#include "rsserver/p3face.h"

#define P3FILELISTS_DEBUG() std::cerr << time(NULL)    << " : FILE_LISTS : " << __FUNCTION__ << " : "
#define P3FILELISTS_ERROR() std::cerr << "***ERROR***" << " : FILE_LISTS : " << __FUNCTION__ << " : "

//#define DEBUG_P3FILELISTS 1
//#define DEBUG_CONTENT_FILTERING 1
//#define DEBUG_FILE_HIERARCHY 1

static const uint32_t P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED     = 0x0000 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED  = 0x0001 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_LOCAL_DIRS_CHANGED  = 0x0002 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_DIRS_CHANGED = 0x0004 ;

p3FileDatabase::p3FileDatabase(p3ServiceControl *mpeers)
    : mServCtrl(mpeers), mFLSMtx("p3FileLists")
{
    // make sure the base directory exists

    std::string base_dir = RsAccounts::AccountDirectory();

    if(base_dir.empty())
        throw std::runtime_error("Cannot create base directory to store/access file sharing files.") ;

    mFileSharingDir = base_dir + "/" + FILE_SHARING_DIR_NAME ;

    if(!RsDirUtil::checkCreateDirectory(mFileSharingDir))
        throw std::runtime_error("Cannot create base directory to store/access file sharing files.") ;

    // loads existing indexes for friends. Some might be already present here.
    //
    mRemoteDirectories.clear() ;	// we should load them!
    mOwnId = mpeers->getOwnId() ;

    mBannedFileListNeedsUpdate = false;
    mLocalSharedDirs = new LocalDirectoryStorage(mFileSharingDir + "/" + LOCAL_SHARED_DIRS_FILE_NAME,mOwnId);
    mHashCache = new HashStorage(mFileSharingDir + "/" + HASH_CACHE_FILE_NAME) ;

    mLocalDirWatcher = new LocalDirectoryUpdater(mHashCache,mLocalSharedDirs) ;

    mUpdateFlags = P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED ;
    mLastRemoteDirSweepTS = 0 ;
    mLastExtraFilesCacheUpdate = 0;
    mLastCleanupTime = 0 ;
    mLastDataRecvTS = 0 ;
    mTrustFriendNodesForBannedFiles = TRUST_FRIEND_NODES_FOR_BANNED_FILES_DEFAULT;
	mLastPrimaryBanListChangeTimeStamp = 0;

    // This is for the transmission of data

    addSerialType(new RsFileListsSerialiser()) ;
}

void p3FileDatabase::setIgnoreLists(const std::list<std::string>& ignored_prefixes,const std::list<std::string>& ignored_suffixes, uint32_t ignore_flags)
{
    RS_STACK_MUTEX(mFLSMtx) ;
    mLocalDirWatcher->setIgnoreLists(ignored_prefixes,ignored_suffixes,ignore_flags) ;

    IndicateConfigChanged();
}
bool p3FileDatabase::getIgnoreLists(std::list<std::string>& ignored_prefixes,std::list<std::string>& ignored_suffixes, uint32_t& ignore_flags)
{
    RS_STACK_MUTEX(mFLSMtx) ;
    return mLocalDirWatcher->getIgnoreLists(ignored_prefixes,ignored_suffixes,ignore_flags) ;
}

RsSerialiser *p3FileDatabase::setupSerialiser()
{
    // This one is for saveList/loadList

    RsSerialiser *rss = new RsSerialiser ;
    rss->addSerialType(new RsFileListsSerialiser()) ;
    rss->addSerialType(new RsGeneralConfigSerialiser());
    rss->addSerialType(new RsFileConfigSerialiser());

    return rss ;
}
void p3FileDatabase::setSharedDirectories(const std::list<SharedDirInfo>& shared_dirs)
{
    {
        RS_STACK_MUTEX(mFLSMtx) ;

        mLocalSharedDirs->setSharedDirectoryList(shared_dirs) ;
        mLocalDirWatcher->forceUpdate(false);

    }

    IndicateConfigChanged();
}
void p3FileDatabase::getSharedDirectories(std::list<SharedDirInfo>& shared_dirs)
{
    RS_STACK_MUTEX(mFLSMtx) ;
    mLocalSharedDirs->getSharedDirectoryList(shared_dirs) ;
}
void p3FileDatabase::updateShareFlags(const SharedDirInfo& info)
{
    {
        RS_STACK_MUTEX(mFLSMtx) ;
        mLocalSharedDirs->updateShareFlags(info) ;
    }

    RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
    IndicateConfigChanged();
}

p3FileDatabase::~p3FileDatabase()
{
    RS_STACK_MUTEX(mFLSMtx) ;

    for(uint32_t i=0;i<mRemoteDirectories.size();++i)
        delete mRemoteDirectories[i];

    mRemoteDirectories.clear(); // just a precaution, not to leave deleted pointers around.

    delete mLocalSharedDirs ;
    delete mLocalDirWatcher ;
    delete mHashCache ;
}

const std::string FILE_DB_APP_NAME = "file_database";
const uint16_t FILE_DB_APP_MAJOR_VERSION	= 	1;
const uint16_t FILE_DB_APP_MINOR_VERSION  = 	0;
const uint16_t FILE_DB_MIN_MAJOR_VERSION  = 	1;
const uint16_t FILE_DB_MIN_MINOR_VERSION	=	0;

RsServiceInfo p3FileDatabase::getServiceInfo()
{
    return RsServiceInfo(RS_SERVICE_TYPE_FILE_DATABASE,
        FILE_DB_APP_NAME,
        FILE_DB_APP_MAJOR_VERSION,
        FILE_DB_APP_MINOR_VERSION,
        FILE_DB_MIN_MAJOR_VERSION,
        FILE_DB_MIN_MINOR_VERSION);
}
int p3FileDatabase::tick()
{
    // tick the input/output list of update items and process them
    //
    tickRecv() ;
    tickSend() ;

    rstime_t now = time(NULL) ;

    // cleanup
    // 	- remove/delete shared file lists for people who are not friend anymore
    // 	-

    if(mLastCleanupTime + 5 < now)
    {
        cleanup();
        mLastCleanupTime = now ;
    }

    static rstime_t last_print_time = 0;

    if(last_print_time + 20 < now)
    {
		{
			RS_STACK_MUTEX(mFLSMtx) ;

#ifdef DEBUG_FILE_HIERARCHY
			mLocalSharedDirs->print();
#endif
			last_print_time = now ;
		}

#warning mr-alice 2016-08-19: "This should be removed, but it's necessary atm for updating the GUI"
        RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);

        checkSendBannedFilesInfo();
    }

    if(mUpdateFlags)
    {
        IndicateConfigChanged();

        if(mUpdateFlags & P3FILELISTS_UPDATE_FLAG_LOCAL_DIRS_CHANGED)
            RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);

        if(mUpdateFlags & P3FILELISTS_UPDATE_FLAG_REMOTE_DIRS_CHANGED)
            RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_FRIENDS, 0);

        mUpdateFlags = P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED ;
    }

    if(mLastRemoteDirSweepTS + 5 < now)
    {
        RS_STACK_MUTEX(mFLSMtx) ;

        std::set<RsPeerId> online_peers ;
        mServCtrl->getPeersConnected(getServiceInfo().mServiceType, online_peers) ;

        for(uint32_t i=0;i<mRemoteDirectories.size();++i)
            if(mRemoteDirectories[i] != NULL)
            {
               if(online_peers.find(mRemoteDirectories[i]->peerId()) != online_peers.end() && mRemoteDirectories[i]->lastSweepTime() + DELAY_BETWEEN_REMOTE_DIRECTORIES_SWEEP < now)
               {
#ifdef DEBUG_FILE_HIERARCHY
                  P3FILELISTS_DEBUG() << "Launching recurs sweep of friend directory " << mRemoteDirectories[i]->peerId() << ". Content currently is:" << std::endl;
                  mRemoteDirectories[i]->print();
#endif

                  locked_recursSweepRemoteDirectory(mRemoteDirectories[i],mRemoteDirectories[i]->root(),0) ;
                  mRemoteDirectories[i]->lastSweepTime() = now ;
               }

               mRemoteDirectories[i]->checkSave() ;
            }

        mLastRemoteDirSweepTS = now;
		mLocalSharedDirs->checkSave() ;

        // This is a hack to make loaded directories show up in the GUI, because the GUI generally isn't ready at the time they are actually loaded up,
        // so the first notify is ignored, and no other notify will happen next. We only do it if no data was received in the last 5 secs, in order to
        // avoid syncing the GUI at every dir sync which kills performance.

		if(mLastDataRecvTS + 5 < now && mLastDataRecvTS + 20 > now)
			RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_FRIENDS, 0);						 	 	 // notify the GUI if the hierarchy has changed
    }

    return 0;
}

void p3FileDatabase::startThreads()
{
    RS_STACK_MUTEX(mFLSMtx) ;
#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "Starting directory watcher thread..." ;
#endif
    mLocalDirWatcher->start("fs dir watcher");
#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "Done." << std::endl;
#endif
}
void p3FileDatabase::stopThreads()
{
    RS_STACK_MUTEX(mFLSMtx) ;
#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "Stopping hash cache thread..." ; std::cerr.flush() ;
#endif
    mHashCache->fullstop();
#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "Done." << std::endl;
    P3FILELISTS_DEBUG() << "Stopping directory watcher thread..." ; std::cerr.flush() ;
#endif
    mLocalDirWatcher->fullstop();
#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "Done." << std::endl;
#endif
}

bool p3FileDatabase::saveList(bool &cleanup, std::list<RsItem *>& sList)
{
cleanup = true;

#ifdef DEBUG_FILE_HIERARCHY
    P3FILELISTS_DEBUG() << "Save list" << std::endl;
#endif

    /* get list of directories */
    std::list<SharedDirInfo> dirList;
    {
        RS_STACK_MUTEX(mFLSMtx) ;
        mLocalSharedDirs->getSharedDirectoryList(dirList);
    }

	{
		RS_STACK_MUTEX(mFLSMtx) ;
		for(std::list<SharedDirInfo>::iterator it = dirList.begin(); it != dirList.end(); ++it)
		{
			RsFileConfigItem *fi = new RsFileConfigItem();

			fi->file.path = (*it).filename ;
			fi->file.name = (*it).virtualname ;
			fi->flags = (*it).shareflags.toUInt32() ;

			for(std::list<RsNodeGroupId>::const_iterator it2( (*it).parent_groups.begin());it2!=(*it).parent_groups.end();++it2)
				fi->parent_groups.ids.insert(*it2) ;

			sList.push_back(fi);
		}
	}

    {
        RS_STACK_MUTEX(mFLSMtx) ;

        RsFileListsBannedHashesConfigItem *item = new RsFileListsBannedHashesConfigItem ;

        item->primary_banned_files_list = mPrimaryBanList;
        sList.push_back(item) ;
    }

    RsConfigKeyValueSet *rskv = new RsConfigKeyValueSet();

    /* basic control parameters */
    {
        RS_STACK_MUTEX(mFLSMtx) ;
        std::string s ;
        rs_sprintf(s, "%lu", mHashCache->rememberHashFilesDuration()) ;

        RsTlvKeyValue kv;

        kv.key = HASH_CACHE_DURATION_SS;
        kv.value = s ;

        rskv->tlvkvs.pairs.push_back(kv);
    }

    {
        std::string s ;
        rs_sprintf(s, "%d", watchPeriod()) ;

        RsTlvKeyValue kv;

        kv.key = WATCH_FILE_DURATION_SS;
        kv.value = s ;

        rskv->tlvkvs.pairs.push_back(kv);
    }
	{
        std::string s ;
        rs_sprintf(s, "%d", maxShareDepth()) ;

        RsTlvKeyValue kv;

        kv.key = MAX_SHARE_DEPTH;
        kv.value = s ;

        rskv->tlvkvs.pairs.push_back(kv);
    }
	{
        RsTlvKeyValue kv;

        kv.key = FOLLOW_SYMLINKS_SS;
        kv.value = followSymLinks()?"YES":"NO" ;

        rskv->tlvkvs.pairs.push_back(kv);
    }
	{
        RsTlvKeyValue kv;

        kv.key = IGNORE_DUPLICATES;
        kv.value = ignoreDuplicates()?"YES":"NO" ;

        rskv->tlvkvs.pairs.push_back(kv);
    }
    {
        RsTlvKeyValue kv;

        kv.key = WATCH_FILE_ENABLED_SS;
        kv.value = watchEnabled()?"YES":"NO" ;

        rskv->tlvkvs.pairs.push_back(kv);
    }
    {
        RsTlvKeyValue kv;

        kv.key = TRUST_FRIEND_NODES_FOR_BANNED_FILES_SS;
        kv.value = trustFriendNodesForBannedFiles()?"YES":"NO" ;

        rskv->tlvkvs.pairs.push_back(kv);
    }
    {
        RsTlvKeyValue kv;

        kv.key = WATCH_HASH_SALT_SS;
        kv.value = mLocalDirWatcher->hashSalt().toStdString();

        rskv->tlvkvs.pairs.push_back(kv);
    }
	{
		std::list<std::string> prefix_list,suffix_list;
		uint32_t flags ;

		mLocalDirWatcher->getIgnoreLists(prefix_list,suffix_list,flags) ;

		std::string prefix_string, suffix_string ;

		for(auto it(prefix_list.begin());it!=prefix_list.end();++it) prefix_string += *it + ";" ;
		for(auto it(suffix_list.begin());it!=suffix_list.end();++it) suffix_string += *it + ";" ;

        RsTlvKeyValue kv;

        kv.key = IGNORED_PREFIXES_SS; kv.value = prefix_string; rskv->tlvkvs.pairs.push_back(kv);
        kv.key = IGNORED_SUFFIXES_SS; kv.value = suffix_string; rskv->tlvkvs.pairs.push_back(kv);

        std::string s ;
        rs_sprintf(s, "%lu", flags) ;

        kv.key = IGNORE_LIST_FLAGS_SS; kv.value = s; rskv->tlvkvs.pairs.push_back(kv);
	}

    /* Add KeyValue to saveList */
    sList.push_back(rskv);

    return true;
}

bool p3FileDatabase::loadList(std::list<RsItem *>& load)
{
    /* for each item, check it exists ....
     * - remove any that are dead (or flag?)
     */
    static const FileStorageFlags PERMISSION_MASK = DIR_FLAGS_PERMISSIONS_MASK;

#ifdef  DEBUG_FILE_HIERARCHY
    P3FILELISTS_DEBUG() << "Load list" << std::endl;
#endif

    std::list<SharedDirInfo> dirList;
	std::list<std::string> ignored_prefixes,ignored_suffixes ;
	uint32_t ignore_flags = RS_FILE_SHARE_FLAGS_IGNORE_PREFIXES | RS_FILE_SHARE_FLAGS_IGNORE_SUFFIXES ;
	uint32_t max_share_depth = 0;

	// OS-dependent default ignore lists
#ifdef WINDOWS_SYS
	ignored_suffixes.push_back( ".bak" );
#else
	ignored_prefixes.push_back( "." );
	ignored_suffixes.push_back( "~" );
	ignored_suffixes.push_back( ".part" );
#endif

    for(std::list<RsItem *>::iterator it = load.begin(); it != load.end(); ++it)
    {
        RsConfigKeyValueSet *rskv ;

        if (NULL != (rskv = dynamic_cast<RsConfigKeyValueSet *>(*it)))
        {
            /* make into map */
            //std::map<std::string, std::string> configMap;
            //std::map<std::string, std::string>::const_iterator mit ;

            for(std::list<RsTlvKeyValue>::const_iterator kit = rskv->tlvkvs.pairs.begin(); kit != rskv->tlvkvs.pairs.end(); ++kit)
            if (kit->key == HASH_CACHE_DURATION_SS)
            {
                uint32_t t=0 ;
                if(sscanf(kit->value.c_str(),"%u",&t) == 1)
                    mHashCache->setRememberHashFilesDuration(t);
            }
            else if(kit->key == WATCH_FILE_DURATION_SS)
            {
                int t=0 ;
                if(sscanf(kit->value.c_str(),"%d",&t) == 1)
                    setWatchPeriod(t);
            }
			else if(kit->key == FOLLOW_SYMLINKS_SS)
            {
                setFollowSymLinks(kit->value == "YES") ;
            }
            else if(kit->key == IGNORE_DUPLICATES)
            {
                setIgnoreDuplicates(kit->value == "YES") ;
            }
            else if(kit->key == WATCH_FILE_ENABLED_SS)
            {
                setWatchEnabled(kit->value == "YES") ;
            }
            else if(kit->key == TRUST_FRIEND_NODES_FOR_BANNED_FILES_SS)
            {
                setTrustFriendNodesForBannedFiles(kit->value == "YES") ;
            }
            else if(kit->key == WATCH_HASH_SALT_SS)
            {
                std::cerr << "Initing directory watcher with saved secret salt..." << std::endl;
                mLocalDirWatcher->setHashSalt(RsFileHash(kit->value)) ;
            }
			else if(kit->key == IGNORED_PREFIXES_SS)
			{
				ignored_prefixes.clear();

				std::string b ;
				for(uint32_t i=0;i<kit->value.size();++i)
					if(kit->value[i] == ';')
					{
						if(!b.empty())		// security!
							ignored_prefixes.push_back(b) ;

						b.clear();
					}
					else
						b.push_back(kit->value[i]) ;
			}
			else if(kit->key == IGNORED_SUFFIXES_SS)
			{
				ignored_suffixes.clear();

				std::string b ;
				for(uint32_t i=0;i<kit->value.size();++i)
					if(kit->value[i] == ';')
					{
						if(!b.empty())		// security!
							ignored_suffixes.push_back(b) ;
						b.clear();
					}
					else
						b.push_back(kit->value[i]) ;
			}
			else if(kit->key == IGNORE_LIST_FLAGS_SS)
			{
                int t=0 ;
                if(sscanf(kit->value.c_str(),"%d",&t) == 1)
                    ignore_flags = (uint32_t)t ;
			}
			else if(kit->key == MAX_SHARE_DEPTH)
			{
                int t=0 ;
                if(sscanf(kit->value.c_str(),"%d",&t) == 1)
                    max_share_depth = (uint32_t)t ;
			}

            delete *it ;
            continue ;
        }

        RsFileConfigItem *fi = dynamic_cast<RsFileConfigItem *>(*it);

        if (fi)
        {
            /* ensure that it exists? */

            SharedDirInfo info ;
            info.filename = RsDirUtil::convertPathToUnix(fi->file.path);
            info.virtualname = fi->file.name;
            info.shareflags = FileStorageFlags(fi->flags) ;
            info.shareflags &= PERMISSION_MASK ;

            for(std::set<RsNodeGroupId>::const_iterator itt(fi->parent_groups.ids.begin());itt!=fi->parent_groups.ids.end();++itt)
                info.parent_groups.push_back(*itt) ;

            dirList.push_back(info) ;
        }

        RsFileListsBannedHashesConfigItem *fb = dynamic_cast<RsFileListsBannedHashesConfigItem*>(*it) ;

        if(fb)
        {
            mPrimaryBanList = fb->primary_banned_files_list ;
            mBannedFileListNeedsUpdate = true;
            mLastPrimaryBanListChangeTimeStamp = time(NULL);
        }

        delete *it ;
    }

    /* set directories */
    mLocalSharedDirs->setSharedDirectoryList(dirList);
	mLocalDirWatcher->setIgnoreLists(ignored_prefixes,ignored_suffixes,ignore_flags) ;
	mLocalDirWatcher->setMaxShareDepth(max_share_depth);

    load.clear() ;

    return true;
}

void p3FileDatabase::cleanup()
{
    {
        RS_STACK_MUTEX(mFLSMtx) ;
#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "p3FileDatabase::cleanup()" << std::endl;
#endif

        // look through the list of friend directories. Remove those who are not our friends anymore.
        //
        std::set<RsPeerId> friend_set ;
        {
            std::list<RsPeerId> friend_lst ;

            rsPeers->getFriendList(friend_lst);

            for(std::list<RsPeerId>::const_iterator it(friend_lst.begin());it!=friend_lst.end();++it)
                friend_set.insert(*it) ;
        }
        rstime_t now = time(NULL);

        for(uint32_t i=0;i<mRemoteDirectories.size();++i)
            if(mRemoteDirectories[i] != NULL)
            {
                rstime_t recurs_mod_time ;
                mRemoteDirectories[i]->getDirectoryRecursModTime(0,recurs_mod_time) ;

                rstime_t last_contact = 0 ;
                RsPeerDetails pd ;
                if(rsPeers->getPeerDetails(mRemoteDirectories[i]->peerId(),pd))
                    last_contact = pd.lastConnect ;

                // We remove directories in the following situations:
                //	- the peer is not a friend
                //  - the dir list is non empty but the peer is offline since more than 60 days
                //  - the dir list is empty and the peer is ffline since more than 5 days

                bool should_remove =  friend_set.find(mRemoteDirectories[i]->peerId()) == friend_set.end()
                        			|| (recurs_mod_time == 0 && last_contact + DELAY_BEFORE_DELETE_EMPTY_REMOTE_DIR     < now )
                        			|| (recurs_mod_time != 0 && last_contact + DELAY_BEFORE_DELETE_NON_EMPTY_REMOTE_DIR < now );

                if(!should_remove)
                    continue ;

#ifdef DEBUG_P3FILELISTS
                P3FILELISTS_DEBUG() << "  removing file list of non friend " << mRemoteDirectories[i]->peerId() << std::endl;
#endif

                mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED ;

                friend_set.erase(mRemoteDirectories[i]->peerId());

                mFriendIndexMap.erase(mRemoteDirectories[i]->peerId());

                // also remove the existing file

                remove(mRemoteDirectories[i]->filename().c_str()) ;

                delete mRemoteDirectories[i];
                mRemoteDirectories[i] = NULL ;

                // now, in order to avoid empty seats, just move the last one here, and update indexes

                while(i < mRemoteDirectories.size() && mRemoteDirectories[i] == NULL)
                {
                    mRemoteDirectories[i] = mRemoteDirectories.back();
                    mRemoteDirectories.pop_back();
                }

                if(i < mRemoteDirectories.size() && mRemoteDirectories[i] != NULL)	// this test is needed in the case we have deleted the last element
                    mFriendIndexMap[mRemoteDirectories[i]->peerId()] = i;

                mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_DIRS_CHANGED ;
            }

        // look through the remaining list of friends, which are the ones for which no remoteDirectoryStorage class has been allocated.
        //
        for(std::set<RsPeerId>::const_iterator it(friend_set.begin());it!=friend_set.end();++it)
        {
            // Check if a remote directory exists for that friend, possibly creating the index if the file does not but the friend is online.

            if(rsPeers->isOnline(*it) || RsDirUtil::fileExists(makeRemoteFileName(*it)))
				locked_getFriendIndex(*it) ;
        }

        // cancel existing requests for which the peer is offline

        std::set<RsPeerId> online_peers ;
        mServCtrl->getPeersConnected(getServiceInfo().mServiceType, online_peers) ;

        for(std::map<DirSyncRequestId,DirSyncRequestData>::iterator it = mPendingSyncRequests.begin();it!=mPendingSyncRequests.end();)
            if(online_peers.find(it->second.peer_id) == online_peers.end() || it->second.request_TS + DELAY_BEFORE_DROP_REQUEST < now)
            {
#ifdef DEBUG_P3FILELISTS
                P3FILELISTS_DEBUG() << "  removing pending request " << std::hex << it->first << std::dec << " for peer " << it->second.peer_id << ", because peer is offline or request is too old." << std::endl;
#endif
                std::map<DirSyncRequestId,DirSyncRequestData>::iterator tmp(it);
                ++tmp;
                mPendingSyncRequests.erase(it) ;
                it = tmp;
            }
            else
            {
#ifdef DEBUG_P3FILELISTS
                P3FILELISTS_DEBUG() << "  keeping request " << std::hex << it->first << std::dec << " for peer " << it->second.peer_id << std::endl;
#endif
                ++it ;
            }

		// This is needed at least here, because loadList() might never have been called, if there is no config file present.

		if(mLocalDirWatcher->hashSalt().isNull())
		{
			std::cerr << "(WW) Initialising directory watcher salt to some random value " << std::endl;
			mLocalDirWatcher->setHashSalt(RsFileHash::random()) ;

            IndicateConfigChanged();
		}
    }
}

std::string p3FileDatabase::makeRemoteFileName(const RsPeerId& pid) const
{
    return mFileSharingDir + "/" + "dirlist_"+pid.toStdString()+".bin" ;
}

uint32_t p3FileDatabase::locked_getFriendIndex(const RsPeerId& pid)
{
    std::map<RsPeerId,uint32_t>::const_iterator it = mFriendIndexMap.find(pid) ;

    if(it == mFriendIndexMap.end())
    {
        // allocate a new index for that friend, and tell that we should save.
        uint32_t found = 0 ;
        for(uint32_t i=0;i<mRemoteDirectories.size();++i)
            if(mRemoteDirectories[i] == NULL)
            {
                found = i ;
                break;
            }

        if(!found)
        {
            found = mRemoteDirectories.size();
            mRemoteDirectories.push_back(new RemoteDirectoryStorage(pid,makeRemoteFileName(pid)));

            mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_DIRS_CHANGED ;
            mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED ;

#ifdef DEBUG_P3FILELISTS
            P3FILELISTS_DEBUG() << "  adding missing remote dir entry for friend " << pid << ", with index " << it->second << std::endl;
#endif
        }

        if(sizeof(void*)==4 && mRemoteDirectories.size() >= (1u << NB_FRIEND_INDEX_BITS_32BITS) )
        {
            std::cerr << "(EE) FriendIndexTab is full. This is weird. Do you really have more than " << (1<<NB_FRIEND_INDEX_BITS_32BITS) << " friends??" << std::endl;
            return 1 << NB_FRIEND_INDEX_BITS_32BITS ;
        }
        else if(sizeof(void*)==8 && mRemoteDirectories.size() >= (1ull << NB_FRIEND_INDEX_BITS_64BITS) )
        {
            std::cerr << "(EE) FriendIndexTab is full. This is weird. Do you really have more than " << (1<<NB_FRIEND_INDEX_BITS_64BITS) << " friends??" << std::endl;
            return 1 << NB_FRIEND_INDEX_BITS_64BITS ;
        }

        mFriendIndexMap[pid] = found;

        IndicateConfigChanged();

        return found ;
    }
    else
    {
        if(mRemoteDirectories.size() <= it->second)
        {
            mRemoteDirectories.resize(it->second+1,NULL) ;
            mRemoteDirectories[it->second] = new RemoteDirectoryStorage(pid,makeRemoteFileName(pid));

            mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_DIRS_CHANGED ;
            mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED ;

#ifdef DEBUG_P3FILELISTS
            P3FILELISTS_DEBUG() << "  adding missing remote dir entry for friend " << pid << ", with index " << it->second << std::endl;
#endif
        }

        return it->second;
    }
}

template<> bool p3FileDatabase::convertPointerToEntryIndex<4>(const void *p, EntryIndex& e, uint32_t& friend_index)
{
    // trust me, I can do this ;-)

#if defined(__GNUC__) && !defined(__clang__)
#	pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif // defined(__GNUC__) && !defined(__clang__)
    e   = EntryIndex(  *reinterpret_cast<uint32_t*>(&p) & ENTRY_INDEX_BIT_MASK_32BITS ) ;
    friend_index = (*reinterpret_cast<uint32_t*>(&p)) >> NB_ENTRY_INDEX_BITS_32BITS ;
#if defined(__GNUC__) && !defined(__clang__)
#	pragma GCC diagnostic pop
#endif // defined(__GNUC__) && !defined(__clang__)

    if(friend_index == 0)
    {
        std::cerr << "(EE) Cannot find friend index in pointer. Encoded value is zero!" << std::endl;
        return false;
    }
    friend_index--;

    return true;
}
template<> bool p3FileDatabase::convertEntryIndexToPointer<4>(const EntryIndex& e, uint32_t fi, void *& p)
{
    // the pointer is formed the following way:
    //
    //		[ 10 bits   |  22 bits ]
    //
    // This means that the whoel software has the following build-in limitation:
    //	  * 1023 friends
    //	  * 4M shared files.

    uint32_t fe = (uint32_t)e ;

    if(fi+1 >= (1u<<NB_FRIEND_INDEX_BITS_32BITS) || fe >= (1u<< NB_ENTRY_INDEX_BITS_32BITS))
    {
        P3FILELISTS_ERROR() << "(EE) cannot convert entry index " << e << " of friend with index " << fi << " to pointer." << std::endl;
        return false ;
    }

    p = reinterpret_cast<void*>( ( (1u+fi) << NB_ENTRY_INDEX_BITS_32BITS ) + (fe & ENTRY_INDEX_BIT_MASK_32BITS)) ;

    return true;
}
template<> bool p3FileDatabase::convertPointerToEntryIndex<8>(const void *p, EntryIndex& e, uint32_t& friend_index)
{
    // trust me, I can do this ;-)

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif // defined(__GNUC__) && !defined(__clang__)
    e   = EntryIndex(  *reinterpret_cast<uint64_t*>(&p) & ENTRY_INDEX_BIT_MASK_64BITS ) ;
    friend_index = (*reinterpret_cast<uint64_t*>(&p)) >> NB_ENTRY_INDEX_BITS_64BITS ;
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif // defined(__GNUC__) && !defined(__clang__)

    if(friend_index == 0)
    {
        std::cerr << "(EE) Cannot find friend index in pointer. Encoded value is zero!" << std::endl;
        return false;
    }
    friend_index--;

#ifdef DEBUG_P3FILELISTS
	std::cerr << "Generated index " << e <<" friend " << friend_index << " from pointer " << p << std::endl;
#endif
    return true;
}
template<> bool p3FileDatabase::convertEntryIndexToPointer<8>(const EntryIndex& e, uint32_t fi, void *& p)
{
    // the pointer is formed the following way:
    //
    //		[ 24 bits   |  40 bits ]
    //
    // This means that the whoel software has the following build-in limitation:
    //	  * 1.6M friends
    //	  * 1T shared files.

    uint64_t fe = (uint64_t)e ;

    if(fi+1 >= (1ull<<NB_FRIEND_INDEX_BITS_64BITS) || fe >= (1ull<< NB_ENTRY_INDEX_BITS_64BITS))
    {
        P3FILELISTS_ERROR() << "(EE) cannot convert entry index " << e << " of friend with index " << fi << " to pointer." << std::endl;
        return false ;
    }

    p = reinterpret_cast<void*>( ( (1ull+fi) << NB_ENTRY_INDEX_BITS_64BITS ) + (fe & ENTRY_INDEX_BIT_MASK_64BITS)) ;

#ifdef DEBUG_P3FILELISTS
	std::cerr << "Generated pointer " << p << " from friend " << fi << " and index " << e << std::endl;
#endif

    return true;
}
void p3FileDatabase::requestDirUpdate(void *ref)
{
    uint32_t fi;
    DirectoryStorage::EntryIndex e ;

    convertPointerToEntryIndex<sizeof(void*)>(ref,e,fi);

    if(fi == 0)
        return ;	// not updating current directory (should we?)

#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "Trying to force sync of entry ndex " << e << " to friend " << mRemoteDirectories[fi-1]->peerId() << std::endl;
#endif

    RS_STACK_MUTEX(mFLSMtx) ;

    if(locked_generateAndSendSyncRequest(mRemoteDirectories[fi-1],e))
    {
#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "  Succeed." << std::endl;
#endif
    }
}

// Finds the pointer to the sub-element #row under element ref.

bool p3FileDatabase::findChildPointer( void *ref, int row, void *& result,
                                       FileSearchFlags flags ) const
{
    if (ref == NULL)
    {
        if(flags & RS_FILE_HINTS_LOCAL)
        {
            if(row != 0 && row != 1)
                return false ;

            convertEntryIndexToPointer<sizeof(void*)>(0,row,result);

            return true ;
        }
        else if((uint32_t)row < mRemoteDirectories.size())
        {
            convertEntryIndexToPointer<sizeof(void*)>(mRemoteDirectories[row]->root(),row+1,result);
            return true;
        }
        else
            return false;
    }

    uint32_t fi;
    DirectoryStorage::EntryIndex e ;

    convertPointerToEntryIndex<sizeof(void*)>(ref,e,fi);

    // check consistency
    if( (fi == 0 && !(flags & RS_FILE_HINTS_LOCAL)) ||  (fi > 1 && (flags & RS_FILE_HINTS_LOCAL)))
    {
        P3FILELISTS_ERROR() << "(EE) remote request on local index or local request on remote index. This should not happen." << std::endl;
        return false ;
    }

    if(fi==1 && (flags & RS_FILE_HINTS_LOCAL)) // extra list
    {
		convertEntryIndexToPointer<sizeof(void*)>(row+1,1,result);
        return true;
    }
    DirectoryStorage *storage = (flags & RS_FILE_HINTS_LOCAL)? ((DirectoryStorage*)mLocalSharedDirs) : ((DirectoryStorage*)mRemoteDirectories[fi-1]);

    // Case where the index is the top of a single person. Can be us, or a friend.

    EntryIndex c = 0;
    bool res = storage->getChildIndex(e,row,c);

    convertEntryIndexToPointer<sizeof(void*)>(c,fi,result) ;

    return res;
}

// This function returns statistics about the entire directory

int p3FileDatabase::getSharedDirStatistics(const RsPeerId& pid,SharedDirStats& stats)
{
    RS_STACK_MUTEX(mFLSMtx) ;

    if(pid == mOwnId)
    {
        mLocalSharedDirs->getStatistics(stats) ;
        return true ;
    }
    else
    {
        uint32_t fi = locked_getFriendIndex(pid);
        mRemoteDirectories[fi]->getStatistics(stats) ;

        return true ;
    }
}

bool p3FileDatabase::removeExtraFile(const RsFileHash& hash)
{
	bool ret = false;
    {
    RS_STACK_MUTEX(mFLSMtx) ;

	ret = mExtraFiles->removeExtraFile(hash);
    mLastExtraFilesCacheUpdate = 0 ; // forced cache reload
    }

    RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
	return ret;
}

void p3FileDatabase::getExtraFilesDirDetails(void *ref,DirectoryStorage::EntryIndex e,DirDetails& d) const
{
    // update the cache of extra files if last requested too long ago

    rstime_t now = time(NULL);

    if(mLastExtraFilesCacheUpdate + DELAY_BETWEEN_EXTRA_FILES_CACHE_UPDATES <= now)
    {
        mExtraFiles->getExtraFileList(mExtraFilesCache);
        mLastExtraFilesCacheUpdate = now;
    }

    //

	if(e == 0)	// "virtual extra files directory" => create a dir with as many child as they are extra files
	{
		d.parent = NULL ;

		d.prow = 0;//fi-1 ;
		d.type = DIR_TYPE_PERSON;
		d.hash.clear() ;
        d.size   = mExtraFilesCache.size();
		d.max_mtime = time(NULL);
		d.mtime     = time(NULL);
		d.name = "[Extra List]";
		d.path    = "/";
		d.ref     = ref ;

        for(uint32_t i=0;i<mExtraFilesCache.size();++i)
		{
			DirStub stub;
			stub.type = DIR_TYPE_FILE;
			stub.name = mExtraFilesCache[i].fname;
			convertEntryIndexToPointer<sizeof(void*)>(i+1,1,stub.ref);	// local shared files from extra list

			d.children.push_back(stub);
		}
	}
	else	// extra file. Just query the corresponding data from ftExtra
	{
		d.prow = 1;//fi-1 ;
		d.type = DIR_TYPE_EXTRA_FILE;

        FileInfo& f(mExtraFilesCache[(int)e-1]) ;

		d.hash      = f.hash;
        d.size     = f.size;
		d.max_mtime = 0;			// this is irrelevant
		d.mtime     = 0;			// this is irrelevant
		d.name      = f.path;		// so that the UI shows the complete path, since the parent directory is not really a directory.
		d.path      = f.path;
		d.ref       = ref ;

		convertEntryIndexToPointer<sizeof(void*)>(0,1,d.parent) ;
	}

    d.flags = DIR_FLAGS_ANONYMOUS_DOWNLOAD ;
	d.id = RsPeerId();
}

// This function converts a pointer into directory details, to be used by the AbstractItemModel for browsing the files.
int p3FileDatabase::RequestDirDetails(void *ref, DirDetails& d, FileSearchFlags flags) const
{
    RS_STACK_MUTEX(mFLSMtx) ;

    d.children.clear();

    // Case where the pointer is NULL, which means we're at the top of the list of shared directories for all friends (including us)
    // or at the top of our own list of shared directories, depending on the flags.

    // Friend index is used as follows:
    //	0		: own id
    //  1...n	: other friends
    //
    // entry_index: starts at 0.
    //
    // The point is: we cannot use (0,0) because it encodes to NULL. No existing combination should encode to NULL.
    // So we need to properly convert the friend index into 0 or into a friend tab index in mRemoteDirectories.
    //
    // We should also check the consistency between flags and the content of ref.

    if (ref == NULL)
    {
        d.ref = NULL ;
        d.type = DIR_TYPE_ROOT;
        d.parent = NULL;
        d.prow = -1;
        d.name = "root";
        d.hash.clear() ;
        d.path = "";
        d.mtime = 0;
        d.flags.clear() ;
        d.max_mtime = 0 ;

        if(flags & RS_FILE_HINTS_LOCAL)
        {
            void *p;

            {
			convertEntryIndexToPointer<sizeof(void*)>(0,0,p);	// root of own directories
            DirStub stub;
            stub.type = DIR_TYPE_PERSON;
            stub.name = mServCtrl->getOwnId().toStdString();
            stub.ref  = p;
            d.children.push_back(stub);
			}

            if(mExtraFiles->size() > 0)
            {
            convertEntryIndexToPointer<sizeof(void*)>(0,1,p);	// local shared files from extra list
            DirStub stub;
            stub.type = DIR_TYPE_PERSON;						// not totally exact, but used as a trick.
            stub.name = "[Extra List]";
            stub.ref  = p;

            d.children.push_back(stub);
            }


        }
        else for(uint32_t i=0;i<mRemoteDirectories.size();++i)
            if(mRemoteDirectories[i] != NULL)
            {
               void *p;
               convertEntryIndexToPointer<sizeof(void*)>(mRemoteDirectories[i]->root(),i+1,p);

               DirStub stub;
               stub.type = DIR_TYPE_PERSON;
               stub.name = mRemoteDirectories[i]->peerId().toStdString();
               stub.ref  = p;
               d.children.push_back(stub);
            }

        d.size = 0; // non documented

#ifdef DEBUG_FILE_HIERARCHY
        P3FILELISTS_DEBUG() << "ExtractData: ref=" << ref << ", flags=" << flags << " : returning this: " << std::endl;
		P3FILELISTS_DEBUG() << d << std::endl;
#endif

        return true ;
    }

    uint32_t fi;
    DirectoryStorage::EntryIndex e ;

    convertPointerToEntryIndex<sizeof(void*)>(ref,e,fi);

    // check consistency
    if( (fi == 0 && !(flags & RS_FILE_HINTS_LOCAL)) ||  (fi > 1 && (flags & RS_FILE_HINTS_LOCAL)))
    {
        P3FILELISTS_ERROR() << "(EE) remote request on local index or local request on remote index. This should not happen." << std::endl;
        return false ;
    }

    if((flags & RS_FILE_HINTS_LOCAL) && fi == 1) // extra list
    {
        getExtraFilesDirDetails(ref,e,d);

#ifdef DEBUG_FILE_HIERARCHY
		P3FILELISTS_DEBUG() << "ExtractData: ref=" << ref << ", flags=" << flags << " : returning this: " << std::endl;
		P3FILELISTS_DEBUG() << d << std::endl;
#endif
        return true;
    }


    DirectoryStorage *storage = (flags & RS_FILE_HINTS_LOCAL)? ((DirectoryStorage*)mLocalSharedDirs) : ((DirectoryStorage*)mRemoteDirectories[fi-1]);

    // Case where the index is the top of a single person. Can be us, or a friend.

    if(storage==NULL || !storage->extractData(e,d))
    {
#ifdef DEBUG_FILE_HIERARCHY
        P3FILELISTS_DEBUG() << "(WW) request on index " << e << ", for directory ID=" << ((storage==NULL)?("[NULL]"):(storage->peerId().toStdString())) << " failed. This should not happen." << std::endl;
#endif
        return false ;
    }

    // update indexes. This is a bit hacky, but does the job. The cast to intptr_t is the proper way to convert
    // a pointer into an int.

    convertEntryIndexToPointer<sizeof(void*)>((intptr_t)d.ref,fi,d.ref) ;

    for(uint32_t i=0;i<d.children.size();++i)
        convertEntryIndexToPointer<sizeof(void*)>((intptr_t)d.children[i].ref,fi,d.children[i].ref);

    if(e == 0)	// root
    {
        d.prow = 0;//fi-1 ;
        d.parent = NULL ;
    }
    else
    {
        if(d.parent == 0)	 // child of root node
            d.prow = (flags & RS_FILE_HINTS_LOCAL)?0:(fi-1);
        else
            d.prow = storage->parentRow(e) ;

        convertEntryIndexToPointer<sizeof(void*)>((intptr_t)d.parent,fi,d.parent) ;
    }

    d.id = storage->peerId();

#ifdef DEBUG_FILE_HIERARCHY
    P3FILELISTS_DEBUG() << "ExtractData: ref=" << ref << ", flags=" << flags << " : returning this: " << std::endl;
    P3FILELISTS_DEBUG() << d << std::endl;
#endif

    return true;
}

uint32_t p3FileDatabase::getType(void *ref,FileSearchFlags flags) const
{
    RS_STACK_MUTEX(mFLSMtx) ;

    EntryIndex e ;
    uint32_t fi;

    if(ref == NULL)
        return DIR_TYPE_ROOT ;

    convertPointerToEntryIndex<sizeof(void*)>(ref,e,fi);

    if(e == 0)
        return DIR_TYPE_PERSON ;

    if(flags & RS_FILE_HINTS_LOCAL)
    {
		if(fi == 0 && mLocalSharedDirs != NULL)
			return mLocalSharedDirs->getEntryType(e) ;

        if(fi == 1)
            return DIR_TYPE_EXTRA_FILE;

        P3FILELISTS_ERROR() << " Cannot determine type of entry " << ref  << std::endl;
        return DIR_TYPE_ROOT ;// some failure case. Should not happen
    }
    else
    {
		if(fi-1 < mRemoteDirectories.size() && mRemoteDirectories[fi-1]!=NULL)
			return mRemoteDirectories[fi-1]->getEntryType(e) ;

        P3FILELISTS_ERROR() << " Cannot determine type of entry " << ref  << std::endl;
        return DIR_TYPE_ROOT ;// some failure case. Should not happen
    }
}

void p3FileDatabase::forceDirectoryCheck(bool add_safe_delay)              // Force re-sweep the directories and see what's changed
{
    mLocalDirWatcher->forceUpdate(add_safe_delay);
}
void p3FileDatabase::togglePauseHashingProcess()
{
    RS_STACK_MUTEX(mFLSMtx) ;
    mLocalDirWatcher->togglePauseHashingProcess();
}
bool p3FileDatabase::hashingProcessPaused()
{
    RS_STACK_MUTEX(mFLSMtx) ;
    return  mLocalDirWatcher->hashingProcessPaused();
}
bool p3FileDatabase::inDirectoryCheck()
{
    RS_STACK_MUTEX(mFLSMtx) ;
    return  mLocalDirWatcher->inDirectoryCheck();
}
void p3FileDatabase::setFollowSymLinks(bool b)
{
    RS_STACK_MUTEX(mFLSMtx) ;
    mLocalDirWatcher->setFollowSymLinks(b) ;
    IndicateConfigChanged();
}
bool p3FileDatabase::followSymLinks() const
{
    RS_STACK_MUTEX(mFLSMtx) ;
    return mLocalDirWatcher->followSymLinks() ;
}
void p3FileDatabase::setIgnoreDuplicates(bool i)
{
    RS_STACK_MUTEX(mFLSMtx) ;
    mLocalDirWatcher->setIgnoreDuplicates(i) ;
    IndicateConfigChanged();
}
bool p3FileDatabase::ignoreDuplicates() const
{
    RS_STACK_MUTEX(mFLSMtx) ;
    return mLocalDirWatcher->ignoreDuplicates() ;
}
void p3FileDatabase::setMaxShareDepth(int i)
{
    RS_STACK_MUTEX(mFLSMtx) ;
    mLocalDirWatcher->setMaxShareDepth(i) ;
    IndicateConfigChanged();
}
int  p3FileDatabase::maxShareDepth() const
{
    RS_STACK_MUTEX(mFLSMtx) ;
    return mLocalDirWatcher->maxShareDepth() ;
}
void p3FileDatabase::setWatchEnabled(bool b)
{
    RS_STACK_MUTEX(mFLSMtx) ;
    mLocalDirWatcher->setEnabled(b) ;
    IndicateConfigChanged();
}
bool p3FileDatabase::watchEnabled()
{
    RS_STACK_MUTEX(mFLSMtx) ;
    return mLocalDirWatcher->isEnabled() ;
}
void p3FileDatabase::setWatchPeriod(uint32_t seconds)
{
    RS_STACK_MUTEX(mFLSMtx) ;

    mLocalDirWatcher->setFileWatchPeriod(seconds);
    IndicateConfigChanged();
}
uint32_t p3FileDatabase::watchPeriod()
{
    RS_STACK_MUTEX(mFLSMtx) ;
    return mLocalDirWatcher->fileWatchPeriod();
}

int p3FileDatabase::SearchKeywords(const std::list<std::string>& keywords, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& client_peer_id)
{
    if(flags & RS_FILE_HINTS_LOCAL)
    {
        std::list<EntryIndex> firesults;
        std::list<void *> pointers;

        {
            RS_STACK_MUTEX(mFLSMtx) ;

            mLocalSharedDirs->searchTerms(keywords,firesults) ;

            for(std::list<EntryIndex>::iterator it(firesults.begin());it!=firesults.end();++it)
            {
                void *p=NULL;
                convertEntryIndexToPointer<sizeof(void*)>(*it,0,p);
				pointers.push_back(p) ;
            }
        }

        filterResults(pointers,results,flags,client_peer_id) ;
    }

    if(flags & RS_FILE_HINTS_REMOTE)
    {
		std::list<void*> pointers ;

        {
            RS_STACK_MUTEX(mFLSMtx) ;

            for(uint32_t i=0;i<mRemoteDirectories.size();++i)
                if(mRemoteDirectories[i] != NULL)
                {
                    std::list<EntryIndex> local_results;
                    mRemoteDirectories[i]->searchTerms(keywords,local_results) ;

                    for(std::list<EntryIndex>::iterator it(local_results.begin());it!=local_results.end();++it)
                    {
                        void *p=NULL;
                        convertEntryIndexToPointer<sizeof(void*)>(*it,i+1,p);

						pointers.push_back(p) ;
					}
                }
        }

		for(auto it(pointers.begin());it!=pointers.end();++it)
		{
			DirDetails dd;

			if(!RequestDirDetails (*it,dd,RS_FILE_HINTS_REMOTE))
				continue ;

			results.push_back(dd);
		}
    }

    return !results.empty() ;
}

int p3FileDatabase::SearchBoolExp(RsRegularExpression::Expression *exp, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& client_peer_id) const
{
    if(flags & RS_FILE_HINTS_LOCAL)
    {
        std::list<EntryIndex> firesults;
        std::list<void*> pointers;
        {
            RS_STACK_MUTEX(mFLSMtx) ;

            mLocalSharedDirs->searchBoolExp(exp,firesults) ;

            for(std::list<EntryIndex>::iterator it(firesults.begin());it!=firesults.end();++it)
            {
                void *p=NULL;
                convertEntryIndexToPointer<sizeof(void*)>(*it,0,p);
                pointers.push_back(p);
            }
        }

        filterResults(pointers,results,flags,client_peer_id) ;
    }

    if(flags & RS_FILE_HINTS_REMOTE)
    {
		std::list<void*> pointers ;
        {
            RS_STACK_MUTEX(mFLSMtx) ;

            for(uint32_t i=0;i<mRemoteDirectories.size();++i)
                if(mRemoteDirectories[i] != NULL)
                {
                    std::list<EntryIndex> local_results;
                    mRemoteDirectories[i]->searchBoolExp(exp,local_results) ;

                    for(std::list<EntryIndex>::iterator it(local_results.begin());it!=local_results.end();++it)
                    {
                        void *p=NULL;
                        convertEntryIndexToPointer<sizeof(void*)>(*it,i+1,p);
						pointers.push_back(p) ;
					}
                }
        }

		for(auto it(pointers.begin());it!=pointers.end();++it)
		{
			DirDetails dd;

			if(!RequestDirDetails (*it,dd,RS_FILE_HINTS_REMOTE))
				continue ;

			results.push_back(dd);
		}
    }

    return !results.empty() ;

}

bool p3FileDatabase::search(
        const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const
{
	RS_STACK_MUTEX(mFLSMtx);

	if( (hintflags & RS_FILE_HINTS_EXTRA) &&
	        mExtraFiles->search(hash, hintflags, info) )
		return true;

    if(hintflags & RS_FILE_HINTS_LOCAL)
    {
        RsFileHash real_hash ;
        EntryIndex indx;

        if(mLocalSharedDirs->searchHash(hash,real_hash,indx))
		{
			mLocalSharedDirs->getFileInfo(indx,info) ;

			if(!real_hash.isNull())
			{
				info.hash = real_hash ;
				info.transfer_info_flags |= RS_FILE_REQ_ENCRYPTED ;
			}

			return true;
		}
    }

    if(hintflags & RS_FILE_HINTS_REMOTE)
    {
        EntryIndex indx;
		bool found = false ;
		
		for(uint32_t i=0;i<mRemoteDirectories.size();++i)
			if(mRemoteDirectories[i] != NULL && mRemoteDirectories[i]->searchHash(hash,indx))
			{
				TransferInfo ti ;
				ti.peerId = mRemoteDirectories[i]->peerId();
				        
				info.hash = hash ;
				info.peers.push_back(ti) ;

				found = true ;
			}
		
		if(found)
			return true;
    }
    return false;
}

int p3FileDatabase::filterResults(
        const std::list<void*>& firesults, std::list<DirDetails>& results,
        FileSearchFlags flags, const RsPeerId& peer_id ) const
{
    results.clear();

    flags &= RS_FILE_HINTS_PERMISSION_MASK;	// remove other flags.

    /* translate/filter results */

	for(void* rit: std::as_const(firesults))
	{
		DirDetails cdetails;

		if(!RequestDirDetails(rit, cdetails, RS_FILE_HINTS_LOCAL))
		{
			RS_ERR("Cannot retrieve dir details for entry: ", rit);
			print_stacktrace();
			continue ;
		}

		RS_DBG( "Filtering candidate: ", rit,
		        ", name: ", cdetails.name, ", path: ", cdetails.path,
		        ", flags: ", cdetails.flags, ", peer: ", peer_id );

		if(!peer_id.isNull())
		{
			FileSearchFlags permission_flags =
			        rsPeers->computePeerPermissionFlags(
			            peer_id, cdetails.flags, cdetails.parent_groups );

			if (cdetails.type == DIR_TYPE_FILE && ( permission_flags & flags ))
			{
				cdetails.id.clear();
				results.push_back(cdetails);
#ifdef DEBUG_P3FILELISTS
				std::cerr << ": kept" << std::endl ;
#endif
            }
#ifdef DEBUG_P3FILELISTS
            else
                std::cerr << ": discarded" << std::endl ;
#endif
        }
        else
            results.push_back(cdetails);
    }

	for(auto& res: results)
	{
		/* Most file will just have file name stored, but single file shared
		 * without a shared dir will contain full path instead of just the
		 * name, so purify it to perform the search */
		if(res.name.find("/") != std::string::npos )
		{
			std::string _tDirPath;
			RsDirUtil::splitDirFromFile(res.name, _tDirPath, res.name);
		}
	}

	return !results.empty();
}

bool p3FileDatabase::convertSharedFilePath(const std::string& path,std::string& fullpath)
{
    RS_STACK_MUTEX(mFLSMtx) ;
    return mLocalSharedDirs->convertSharedFilePath(path,fullpath) ;
}

//==============================================================================================================================//
//                                               Update of remote directories                                                   //
//==============================================================================================================================//

// Algorithm:
//
//	Local dirs store the last modif time of the file, in local time
//  	- the max time is computed upward until the root of the hierarchy
//		- because the hash is performed late, the last modf time upward is updated only when the hash is obtained.
//
//	Remote dirs store
//		1 - recursive max modif time of the files/dir in the friend's time
//		2 - modif time of the files/dir in the friend's time
//		3 - update TS in the local time
//
//  The update algorithm is the following:
//
//	[Client side]
//		- every 30 sec, send a sync packet for the root of the hierarchy containing the remote known max recurs TS.
//		- crawl through all directories and if the update TS is 0, ask for sync too.
//		- when a sync respnse is received:
//			- if response is ACK, set the update time to now. Nothing changed.
//			- if response is an update
//				* locally update the subfiles
//				* for all subdirs, set the local update time to 0 (so as to force a sync)
//
//  [Server side]
//		- when sync req is received, compare to local recurs TS, and send ACK or full update accordingly
//
//  Directories are designated by their hash, instead of their index. This allows to hide the non shared directories
//  behind a layer of abstraction, at the cost of a logarithmic search, which is acceptable as far as dir sync-ing between
//  friends is concerned (We obviously could not do that for GUI display, which has a small and constant cost).

void p3FileDatabase::tickRecv()
{
   RsItem *item ;

   while( NULL != (item = recvItem()) )
   {
      switch(item->PacketSubType())
      {
      case RS_PKT_SUBTYPE_FILELISTS_SYNC_REQ_ITEM:       handleDirSyncRequest( dynamic_cast<RsFileListsSyncRequestItem*>(item) ) ;     break ;
      case RS_PKT_SUBTYPE_FILELISTS_BANNED_HASHES_ITEM : handleBannedFilesInfo( dynamic_cast<RsFileListsBannedHashesItem*>(item) ) ;   break ;
      case RS_PKT_SUBTYPE_FILELISTS_SYNC_RSP_ITEM:
	  {
          RsFileListsSyncResponseItem *sitem = dynamic_cast<RsFileListsSyncResponseItem*>(item);
		  handleDirSyncResponse(sitem) ;
          item = sitem ;
      }
		  break ;
      default:
         P3FILELISTS_ERROR() << "(EE) unhandled packet subtype " << item->PacketSubType() << " in " << __PRETTY_FUNCTION__ << std::endl;
      }

      delete item ;
   }
}

void p3FileDatabase::tickSend()
{
    // go through the list of out requests and send them to the corresponding friends, if they are online.
}

void p3FileDatabase::handleDirSyncRequest(RsFileListsSyncRequestItem *item)
{
    RsFileListsSyncResponseItem *ritem = new RsFileListsSyncResponseItem;

    // look at item TS. If local is newer, send the full directory content.
    {
        RS_STACK_MUTEX(mFLSMtx) ;

#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "Received directory sync request from peer " << item->PeerId() << ". hash=" << item->entry_hash << ", flags=" << (void*)(intptr_t)item->flags << ", request id: " << std::hex << item->request_id << std::dec << ", last known TS: " << item->last_known_recurs_modf_TS << std::endl;
#endif

        EntryIndex entry_index = DirectoryStorage::NO_INDEX;

        if(!mLocalSharedDirs->getIndexFromDirHash(item->entry_hash,entry_index))
        {
#ifdef DEBUG_P3FILELISTS
            P3FILELISTS_DEBUG() << "  (EE) Cannot find entry index for hash " << item->entry_hash << ": cannot respond to sync request." << std::endl;
#endif
            return;
        }

        uint32_t entry_type = mLocalSharedDirs->getEntryType(entry_index) ;
        ritem->PeerId(item->PeerId()) ;
        ritem->request_id = item->request_id;
        ritem->entry_hash = item->entry_hash ;

        std::list<RsNodeGroupId> node_groups;
        FileStorageFlags node_flags;

        if(entry_type != DIR_TYPE_DIR)
        {
#ifdef DEBUG_P3FILELISTS
            P3FILELISTS_DEBUG() << "  Directory does not exist anymore, or is not a directory, or permission denied. Answering with proper flags." << std::endl;
#endif
            ritem->flags = RsFileListsItem::FLAGS_SYNC_RESPONSE | RsFileListsItem::FLAGS_ENTRY_WAS_REMOVED ;
        }
        else if(entry_index != 0 && (!mLocalSharedDirs->getFileSharingPermissions(entry_index,node_flags,node_groups) || !(rsPeers->computePeerPermissionFlags(item->PeerId(),node_flags,node_groups) & RS_FILE_HINTS_BROWSABLE)))
        {
            P3FILELISTS_ERROR() << "(EE) cannot get file permissions for entry index " << (void*)(intptr_t)entry_index << ", or permission denied." << std::endl;
            ritem->flags = RsFileListsItem::FLAGS_SYNC_RESPONSE | RsFileListsItem::FLAGS_ENTRY_WAS_REMOVED ;
        }
        else
        {
            rstime_t local_recurs_max_time ;
            mLocalSharedDirs->getDirectoryRecursModTime(entry_index,local_recurs_max_time) ;

            if(item->last_known_recurs_modf_TS != local_recurs_max_time)	// normally, should be "<", but since we provided the TS it should be equal, so != is more robust.
            {
#ifdef DEBUG_P3FILELISTS
                P3FILELISTS_DEBUG() << "  Directory is more recent than what the friend knows. Sending full dir content as response." << std::endl;
#endif

                ritem->flags = RsFileListsItem::FLAGS_SYNC_RESPONSE | RsFileListsItem::FLAGS_SYNC_DIR_CONTENT;
                ritem->last_known_recurs_modf_TS = local_recurs_max_time;

                // We supply the peer id, in order to possibly remove some subdirs, if entries are not allowed to be seen by this peer.
                mLocalSharedDirs->serialiseDirEntry(entry_index,ritem->directory_content_data,item->PeerId()) ;
            }
            else
            {
#ifdef DEBUG_P3FILELISTS
                P3FILELISTS_DEBUG() << "  Directory is up to date w.r.t. what the friend knows. Sending ACK." << std::endl;
#endif

                ritem->flags = RsFileListsItem::FLAGS_SYNC_RESPONSE | RsFileListsItem::FLAGS_ENTRY_UP_TO_DATE ;
                ritem->last_known_recurs_modf_TS = local_recurs_max_time ;
            }
        }
    }

    // sends the response.

    splitAndSendItem(ritem) ;
}

void p3FileDatabase::splitAndSendItem(RsFileListsSyncResponseItem *ritem)
{
    ritem->checksum = RsDirUtil::sha1sum((uint8_t*)ritem->directory_content_data.bin_data,ritem->directory_content_data.bin_len);

    while(ritem->directory_content_data.bin_len > MAX_DIR_SYNC_RESPONSE_DATA_SIZE)
    {
#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "Chopping off partial chunk of size " << MAX_DIR_SYNC_RESPONSE_DATA_SIZE << " from item data of size " << ritem->directory_content_data.bin_len << std::endl;
#endif

        ritem->flags |= RsFileListsItem::FLAGS_SYNC_PARTIAL ;

        RsFileListsSyncResponseItem *subitem = new RsFileListsSyncResponseItem() ;

        subitem->entry_hash                = ritem->entry_hash;
        subitem->flags                     = ritem->flags ;
        subitem->last_known_recurs_modf_TS = ritem->last_known_recurs_modf_TS;
        subitem->request_id                = ritem->request_id;
        subitem->checksum                  = ritem->checksum ;

        // copy a subpart of the data
        subitem->directory_content_data.tlvtype = ritem->directory_content_data.tlvtype ;
        subitem->directory_content_data.setBinData(ritem->directory_content_data.bin_data, MAX_DIR_SYNC_RESPONSE_DATA_SIZE) ;

        // update ritem to chop off the data that was sent.
        memmove(ritem->directory_content_data.bin_data, &((unsigned char*)ritem->directory_content_data.bin_data)[MAX_DIR_SYNC_RESPONSE_DATA_SIZE], ritem->directory_content_data.bin_len - MAX_DIR_SYNC_RESPONSE_DATA_SIZE) ;
        ritem->directory_content_data.bin_len -= MAX_DIR_SYNC_RESPONSE_DATA_SIZE ;

        // send
        subitem->PeerId(ritem->PeerId()) ;

        sendItem(subitem) ;

        // fix up last chunk
        if(ritem->directory_content_data.bin_len <= MAX_DIR_SYNC_RESPONSE_DATA_SIZE)
            ritem->flags |= RsFileListsItem::FLAGS_SYNC_PARTIAL_END ;
    }

    sendItem(ritem) ;
}

// This function should not take memory ownership of ritem, so it makes copies.
// The item that is returned is either created (if different from ritem) or equal to ritem.

RsFileListsSyncResponseItem *p3FileDatabase::recvAndRebuildItem(RsFileListsSyncResponseItem *ritem)
{
    if(!(ritem->flags & RsFileListsItem::FLAGS_SYNC_PARTIAL ))
        return ritem ;

    // item is a partial item. Look first for a starting entry

#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "Item from peer " << ritem->PeerId() << " is partial. Size = " << ritem->directory_content_data.bin_len << std::endl;
#endif

    RS_STACK_MUTEX(mFLSMtx) ;

    bool is_ending = (ritem->flags & RsFileListsItem::FLAGS_SYNC_PARTIAL_END);
    std::map<DirSyncRequestId,RsFileListsSyncResponseItem*>::iterator it = mPartialResponseItems.find(ritem->request_id) ;

    if(it == mPartialResponseItems.end())
    {
        if(is_ending)
        {
            P3FILELISTS_ERROR() << "Impossible situation: partial item ended right away. Dropping..." << std::endl;
            return NULL;
        }
#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "Creating new item buffer" << std::endl;
#endif

        mPartialResponseItems[ritem->request_id] = new RsFileListsSyncResponseItem(*ritem) ;
        return NULL ;
    }
    else if(it->second->checksum != ritem->checksum)
    {
        P3FILELISTS_ERROR() << "Impossible situation: partial items with different checksums. Dropping..." << std::endl;
        mPartialResponseItems.erase(it);
        return NULL;
    }

    // collapse the item at the end of the existing partial item. Dont delete ritem as it will be by the caller.

    uint32_t old_len = it->second->directory_content_data.bin_len ;
    uint32_t added   = ritem->directory_content_data.bin_len;

    it->second->directory_content_data.bin_data = realloc(it->second->directory_content_data.bin_data,old_len + added) ;
    memcpy(&((unsigned char*)it->second->directory_content_data.bin_data)[old_len],ritem->directory_content_data.bin_data,added);
    it->second->directory_content_data.bin_len = old_len + added ;

#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "Added new chunk of length " << added << ". New size is " << old_len + added << std::endl;
#endif

    // if finished, return the item

    if(is_ending)
    {
#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "Item is complete. Returning it" << std::endl;
#endif

        RsFileListsSyncResponseItem *ret = it->second ;
        mPartialResponseItems.erase(it) ;

        ret->flags &= ~RsFileListsItem::FLAGS_SYNC_PARTIAL_END ;
        ret->flags &= ~RsFileListsItem::FLAGS_SYNC_PARTIAL ;

        return ret ;
    }
    else
        return NULL ;
}

// We employ a trick in this function:
// - if recvAndRebuildItem(item) returns the same item, it has not created memory, so the incoming item should be the one to
//   delete, which is done by the caller in every case.
// - if it returns a different item, it means that the item has been created below when collapsing items, so we should delete both.
//   to do so, we first delete the incoming item, and replace the pointer by the new created one.

void p3FileDatabase::handleDirSyncResponse(RsFileListsSyncResponseItem*& sitem)
{
    RsFileListsSyncResponseItem *item = recvAndRebuildItem(sitem) ;

    if(!item)
        return ;

    if(item != sitem)
    {
        delete sitem ;
        sitem = item ;
    }

    rstime_t now = time(NULL);

    // check the hash. If anything goes wrong (in the chunking for instance) the hash will not match

    if(RsDirUtil::sha1sum((uint8_t*)item->directory_content_data.bin_data,item->directory_content_data.bin_len) != item->checksum)
    {
        P3FILELISTS_ERROR() << "Checksum error in response item " << std::hex << item->request_id << std::dec << " . This is unexpected, and might be due to connection problems." << std::endl;
        return ;
    }

#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "Handling sync response for directory with hash " << item->entry_hash << std::endl;
#endif

    EntryIndex entry_index = DirectoryStorage::NO_INDEX;

    // find the correct friend entry

    uint32_t fi = 0 ;

    {
        RS_STACK_MUTEX(mFLSMtx) ;
        fi = locked_getFriendIndex(item->PeerId());

#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "  friend index is " << fi ;
#endif

        // make sure we have a remote directory for that friend.

        if(!mRemoteDirectories[fi]->getIndexFromDirHash(item->entry_hash,entry_index))
        {
            std::cerr << std::endl;
            P3FILELISTS_ERROR() << "  (EE) cannot find index from hash " << item->entry_hash << ". Dropping the response." << std::endl;
            return ;
        }
#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "  entry index is " << entry_index << " " ;
#endif
    }

    if(item->flags & RsFileListsItem::FLAGS_ENTRY_WAS_REMOVED)
    {
        RS_STACK_MUTEX(mFLSMtx) ;
#ifdef DEBUG_P3FILELISTS
        std::cerr << "  removing directory with index " << entry_index << " because it does not exist." << std::endl;
#endif
        mRemoteDirectories[fi]->removeDirectory(entry_index);

        mRemoteDirectories[fi]->print();
    }
    else if(item->flags & RsFileListsItem::FLAGS_ENTRY_UP_TO_DATE)
    {
        RS_STACK_MUTEX(mFLSMtx) ;
#ifdef DEBUG_P3FILELISTS
        std::cerr << "  Directory is up to date. Setting local TS." << std::endl;
#endif

        mRemoteDirectories[fi]->setDirectoryUpdateTime(entry_index,now) ;
    }
    else if(item->flags & RsFileListsItem::FLAGS_SYNC_DIR_CONTENT)
    {
#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "  Item contains directory data. Deserialising/Updating." << std::endl;
#endif
        RS_STACK_MUTEX(mFLSMtx) ;

        if(mLastDataRecvTS + 1 < now) // avoid notifying the GUI too often as it kills performance.
		{
			RsServer::notify()->notifyListPreChange(NOTIFY_LIST_DIRLIST_FRIENDS, 0);						 	 	 // notify the GUI if the hierarchy has changed
			mLastDataRecvTS = now;
		}
#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "Performing update of directory index " << std::hex << entry_index << std::dec << " from friend " << item->PeerId() << std::endl;
#endif

        if(mRemoteDirectories[fi]->deserialiseUpdateDirEntry(entry_index,item->directory_content_data))
			mRemoteDirectories[fi]->lastSweepTime() = now - DELAY_BETWEEN_REMOTE_DIRECTORIES_SWEEP + 10 ;  // force re-sweep in 10 secs, so as to fasten updated
        else
            P3FILELISTS_ERROR() << "(EE) Cannot deserialise dir entry. ERROR. "<< std::endl;

#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "  new content after update: " << std::endl;
        mRemoteDirectories[fi]->print();
#endif
    }

	// remove the original request from pending list in the end. doing this here avoids that a new request is added while the previous response is not treated.

    {
        RS_STACK_MUTEX(mFLSMtx) ;

        std::map<DirSyncRequestId,DirSyncRequestData>::iterator it = mPendingSyncRequests.find(item->request_id) ;

        if(it == mPendingSyncRequests.end())
        {
            P3FILELISTS_ERROR() << "  request " << std::hex << item->request_id << std::dec << " cannot be found. ERROR!" << std::endl;
            return ;
        }
        mPendingSyncRequests.erase(it) ;
    }


}

void p3FileDatabase::locked_recursSweepRemoteDirectory(RemoteDirectoryStorage *rds,DirectoryStorage::EntryIndex e,int depth)
{
   rstime_t now = time(NULL) ;

   //std::string indent(2*depth,' ') ;

   // if not up to date, request update, and return (content is not certified, so no need to recurs yet).
   // if up to date, return, because TS is about the last modif TS below, so no need to recurs either.

   // get the info for this entry

#ifdef DEBUG_P3FILELISTS
   P3FILELISTS_DEBUG() << "currently at entry index " << e << std::endl;
#endif

   rstime_t local_update_TS;

   if(!rds->getDirectoryUpdateTime(e,local_update_TS))
   {
       P3FILELISTS_ERROR() << "  (EE) lockec_recursSweepRemoteDirectory(): cannot get update TS for directory with index " << e << ". This is a consistency bug." << std::endl;
       return;
   }

   // compare TS

   if((e == 0 && now > local_update_TS + DELAY_BETWEEN_REMOTE_DIRECTORY_SYNC_REQ) || local_update_TS == 0)	// we need to compare local times only. We cannot compare local (now) with remote time.
       if(locked_generateAndSendSyncRequest(rds,e))
       {
#ifdef DEBUG_P3FILELISTS
           P3FILELISTS_DEBUG() << "  Asking for sync of directory " << e << " to peer " << rds->peerId() << " because it's " << (now - local_update_TS) << " secs old since last check." << std::endl;
#endif
       }

   for(DirectoryStorage::DirIterator it(rds,e);it;++it)
       locked_recursSweepRemoteDirectory(rds,*it,depth+1);
}

p3FileDatabase::DirSyncRequestId p3FileDatabase::makeDirSyncReqId(const RsPeerId& peer_id,const RsFileHash& hash)
{
    static uint64_t random_bias = RSRandom::random_u64();

    uint8_t mem[RsPeerId::SIZE_IN_BYTES + RsFileHash::SIZE_IN_BYTES];
    memcpy(mem,peer_id.toByteArray(),RsPeerId::SIZE_IN_BYTES) ;
    memcpy(&mem[RsPeerId::SIZE_IN_BYTES],hash.toByteArray(),RsFileHash::SIZE_IN_BYTES) ;

    RsFileHash tmp = RsDirUtil::sha1sum(mem,RsPeerId::SIZE_IN_BYTES + RsFileHash::SIZE_IN_BYTES) ;

    // This is kind of arbitrary. The important thing is that the same ID needs to be generated every time for a given (peer_id,entry index) pair, in a way
    // that cannot be brute-forced or reverse-engineered, which explains the random bias and the usage of the hash, that is itself random.

    uint64_t tmp2 ;
    memcpy(&tmp2,tmp.toByteArray(),sizeof(uint64_t));

    uint64_t r = random_bias ^ tmp2;

#ifdef DEBUG_P3FILELISTS
    std::cerr << "Creating ID " << std::hex << r << std::dec << " from peer id " << peer_id << " and hash " << hash << std::endl;
#endif

    return r ;
}

bool p3FileDatabase::locked_generateAndSendSyncRequest(RemoteDirectoryStorage *rds,const DirectoryStorage::EntryIndex& e)
{
    RsFileHash entry_hash ;
    rstime_t now = time(NULL) ;

    rstime_t max_known_recurs_modf_time ;

    if(!rds->getDirectoryRecursModTime(e,max_known_recurs_modf_time))
    {
        P3FILELISTS_ERROR() << "  (EE) cannot find recurs mod time for entry index " << e << ". This is very unexpected." << std::endl;
        return false;
    }
    if(!rds->getDirHashFromIndex(e,entry_hash) )
    {
        P3FILELISTS_ERROR() << "  (EE) cannot find hash for entry index " << e << ". This is very unexpected." << std::endl;
        return false;
    }

    // check if a request already exists and is not too old either: no need to re-ask.

    DirSyncRequestId sync_req_id = makeDirSyncReqId(rds->peerId(),entry_hash) ;

    std::map<DirSyncRequestId,DirSyncRequestData>::iterator it = mPendingSyncRequests.find(sync_req_id) ;

    if(it != mPendingSyncRequests.end())
    {
#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "  Not asking for sync of directory " << e << " to friend " << rds->peerId() << " because a recent pending request still exists." << std::endl;
#endif
        return false ;
    }

    RsFileListsSyncRequestItem *item = new RsFileListsSyncRequestItem ;

    item->entry_hash = entry_hash ;
    item->flags = RsFileListsItem::FLAGS_SYNC_REQUEST ;
    item->request_id = sync_req_id ;
    item->last_known_recurs_modf_TS = max_known_recurs_modf_time ;
    item->PeerId(rds->peerId()) ;

    DirSyncRequestData data ;

    data.request_TS = now ;
    data.peer_id = item->PeerId();
    data.flags = item->flags;

#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "  Pushing req " << std::hex << sync_req_id << std::dec <<  " in pending list with peer id " << data.peer_id << std::endl;
#endif

    mPendingSyncRequests[sync_req_id] = data ;

    sendItem(item) ;	// at end! Because item is destroyed by the process.

    return true;
}


//=========================================================================================================================//
//                                          Unwanted content filtering system                                              //
//=========================================================================================================================//

bool p3FileDatabase::banFile(const RsFileHash& real_file_hash, const std::string& filename, uint64_t file_size)
{
#ifdef DEBUG_CONTENT_FILTERING
    P3FILELISTS_DEBUG() << "  setting file \"" << filename << "\" size=" << file_size << " hash=" << real_file_hash << " as banned." << std::endl;
#endif
	{
		RS_STACK_MUTEX(mFLSMtx) ;
		BannedFileEntry& entry(mPrimaryBanList[real_file_hash]) ;	// primary list (user controlled) of files banned from FT search and forwarding. map<real hash, BannedFileEntry>

        if(entry.mBanTimeStamp == 0)
		{
			entry.mFilename = filename ;
			entry.mSize = file_size ;
			entry.mBanTimeStamp = time(NULL);

			RsFileHash hash_of_hash ;
			ftServer::encryptHash(real_file_hash,hash_of_hash) ;

			mBannedFileList.insert(real_file_hash) ;
			mBannedFileList.insert(hash_of_hash) ;

			mLastPrimaryBanListChangeTimeStamp = time(NULL);
            mBannedFileListNeedsUpdate = true ;
		}
	}

    IndicateConfigChanged();
	return true;
}
bool p3FileDatabase::unbanFile(const RsFileHash& real_file_hash)
{
#ifdef DEBUG_CONTENT_FILTERING
    P3FILELISTS_DEBUG() << " unbanning file with hash " << real_file_hash << std::endl;
#endif
    {
		RS_STACK_MUTEX(mFLSMtx) ;
		mPrimaryBanList.erase(real_file_hash) ;
		mLastPrimaryBanListChangeTimeStamp = time(NULL);
        mBannedFileListNeedsUpdate = true ;
    }

    IndicateConfigChanged();
    return true;
}

bool p3FileDatabase::isFileBanned(const RsFileHash& hash)
{
	RS_STACK_MUTEX(mFLSMtx) ;

    if(mBannedFileList.empty())	// quick exit
        return false ;

    RsFileHash hash_of_hash ;
    ftServer::encryptHash(hash,hash_of_hash) ;

    bool res = mBannedFileList.find(hash) != mBannedFileList.end() || mBannedFileList.find(hash_of_hash) != mBannedFileList.end() ;

#ifdef DEBUG_CONTENT_FILTERING
    if(res)
		P3FILELISTS_DEBUG() << " is file banned(" << hash << "): " << (res?"YES":"NO") << std::endl;
#endif
    return res ;
}

bool p3FileDatabase::getPrimaryBannedFilesList(std::map<RsFileHash,BannedFileEntry>& banned_files)
{
	RS_STACK_MUTEX(mFLSMtx) ;
	banned_files = mPrimaryBanList;

    return true ;
}

bool p3FileDatabase::trustFriendNodesForBannedFiles() const
{
	RS_STACK_MUTEX(mFLSMtx) ;
	return mTrustFriendNodesForBannedFiles;
}

void p3FileDatabase::setTrustFriendNodesForBannedFiles(bool b)
{
	if(b != mTrustFriendNodesForBannedFiles)
    {
		IndicateConfigChanged();
        mBannedFileListNeedsUpdate = true;
    }

	RS_STACK_MUTEX(mFLSMtx) ;
	mTrustFriendNodesForBannedFiles = b;
}

void p3FileDatabase::checkSendBannedFilesInfo()
{
	RS_STACK_MUTEX(mFLSMtx) ;

    // 1 - compare records to list of online friends, send own info of not already

#ifdef DEBUG_CONTENT_FILTERING
    P3FILELISTS_DEBUG() << "  Checking banned files information: " << std::endl;
#endif

    rstime_t now = time(NULL);
    std::list<RsPeerId> online_friends ;
	rsPeers->getOnlineList(online_friends);

    std::set<RsPeerId> peers ;
    for(auto it(online_friends.begin());it!=online_friends.end();++it)			// convert to std::set for efficient search
        peers.insert(*it) ;

    for(auto it(mPeerBannedFiles.begin());it!=mPeerBannedFiles.end();)
    {
        if(peers.find(it->first) == peers.end())	// friend not online, remove his record
        {
            it = mPeerBannedFiles.erase(it) ;
#ifdef DEBUG_CONTENT_FILTERING
			P3FILELISTS_DEBUG() << "    Peer " << it->first << " is offline: removign record." << std::endl;
#endif
            continue;
        }

        if(it->second.mLastSent < mLastPrimaryBanListChangeTimeStamp)				// has ban info already been sent? If not do it.
        {
#ifdef DEBUG_CONTENT_FILTERING
			P3FILELISTS_DEBUG() << "    Peer " << it->first << " is online and hasn't been sent since last change: sending..." << std::endl;
#endif
            locked_sendBanInfo(it->first);
            it->second.mLastSent = now;
        }

        peers.erase(it->first);	 // friend has been handled -> remove from list
        ++it;
    }

    // 2 - add a new record for friends not already in the record map

    for(auto it(peers.begin());it!=peers.end();++it)
    {
        locked_sendBanInfo(*it);
        mPeerBannedFiles[*it].mLastSent = now;
#ifdef DEBUG_CONTENT_FILTERING
		P3FILELISTS_DEBUG() << "    Peer " << *it << " is online and hasn't been sent info at all: sending..." << std::endl;
#endif
    }

    // 3 - update list of banned hashes if it has changed somehow

    if(mBannedFileListNeedsUpdate)
    {
        mBannedFileList.clear();

#ifdef DEBUG_CONTENT_FILTERING
		P3FILELISTS_DEBUG() << "  Creating local banned file list: " << std::endl;
#endif
        // Add all H(H(f)) from friends

        if(mTrustFriendNodesForBannedFiles)
			for(auto it(mPeerBannedFiles.begin());it!=mPeerBannedFiles.end();++it)
				for(auto it2(it->second.mBannedHashOfHash.begin());it2!=it->second.mBannedHashOfHash.end();++it2)
                {
					mBannedFileList.insert(*it2);

#ifdef DEBUG_CONTENT_FILTERING
					P3FILELISTS_DEBUG() << "      from " << it->first << ": H(H(f)) = " << *it2 << std::endl;
#endif
                }

        // Add H(f) and H(H(f)) from our primary list

        for(auto it(mPrimaryBanList.begin());it!=mPrimaryBanList.end();++it)
        {
            mBannedFileList.insert(it->first) ;

			RsFileHash hash_of_hash ;
			ftServer::encryptHash(it->first,hash_of_hash) ;

            mBannedFileList.insert(hash_of_hash) ;

#ifdef DEBUG_CONTENT_FILTERING
			P3FILELISTS_DEBUG() << "      primary: H(f) = " << it->first << ": H(H(f)) = " << hash_of_hash << std::endl;
#endif
		}

        mBannedFileListNeedsUpdate = false ;
    }

#ifdef DEBUG_CONTENT_FILTERING
	P3FILELISTS_DEBUG() << " Final list of locally banned hashes contains: " << mBannedFileList.size() << " elements." << std::endl;
#endif
}

void p3FileDatabase::locked_sendBanInfo(const RsPeerId& peer)
{
    RsFileListsBannedHashesItem *item = new RsFileListsBannedHashesItem ;
	uint32_t session_id = RSRandom::random_u32();

    item->session_id = session_id ;
	item->PeerId(peer);

	for(auto it(mPrimaryBanList.begin());it!=mPrimaryBanList.end();++it)
	{
		RsFileHash hash_of_hash ;

		ftServer::encryptHash(it->first,hash_of_hash) ;

        if(!item)
        {
			item = new RsFileListsBannedHashesItem ;

			item->PeerId(peer);
			item->session_id = session_id ;
        }

		item->encrypted_hashes.insert(hash_of_hash) ;

        if(item->encrypted_hashes.size() >= 200)
        {
 			sendItem(item);
            item = NULL ;
        }
	}

    if(item)
		sendItem(item);
}

void p3FileDatabase::handleBannedFilesInfo(RsFileListsBannedHashesItem *item)
{
	RS_STACK_MUTEX(mFLSMtx) ;

#ifdef DEBUG_CONTENT_FILTERING
	P3FILELISTS_DEBUG() << " received banned files info from peer " << item->PeerId() << ", session id = " << std::hex << item->session_id << std::dec << ": " << item->encrypted_hashes.size() << " files:" << std::endl;
#endif
    // 1 - localize the friend in the banned file map

    PeerBannedFilesEntry& pbfe(mPeerBannedFiles[item->PeerId()]) ;

    if(pbfe.mSessionId != item->session_id)
        pbfe.mBannedHashOfHash.clear();

    pbfe.mSessionId = item->session_id ;

    // 2 - replace/update the list, depending on the session_id

    for(auto it(item->encrypted_hashes.begin());it!=item->encrypted_hashes.end();++it)
    {
        pbfe.mBannedHashOfHash.insert(*it);
#ifdef DEBUG_CONTENT_FILTERING
		P3FILELISTS_DEBUG() << "   added H(H(f)) = " << *it << std::endl;
#endif
    }

    // 3 - tell the updater that the banned file list has changed

    mBannedFileListNeedsUpdate = true ;
}

