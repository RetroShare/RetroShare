/*
 * RetroShare File lists service.
 *
 *     file_sharing/p3filelists.cc
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
#include "serialiser/rsserviceids.h"

#include "file_sharing/p3filelists.h"
#include "file_sharing/directory_storage.h"
#include "file_sharing/directory_updater.h"
#include "file_sharing/rsfilelistitems.h"
#include "file_sharing/file_sharing_defaults.h"

#include "retroshare/rsids.h"
#include "retroshare/rspeers.h"
#include "rsserver/rsaccounts.h"

#include "rsserver/p3face.h"

#define P3FILELISTS_DEBUG() std::cerr << time(NULL)    << " : FILE_LISTS : " << __FUNCTION__ << " : "
#define P3FILELISTS_ERROR() std::cerr << "***ERROR***" << " : FILE_LISTS : " << __FUNCTION__ << " : "

//#define DEBUG_P3FILELISTS 1

static const uint32_t P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED     = 0x0000 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED  = 0x0001 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_LOCAL_DIRS_CHANGED  = 0x0002 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_DIRS_CHANGED = 0x0004 ;

static const uint32_t NB_FRIEND_INDEX_BITS                    = 10 ;
static const uint32_t NB_ENTRY_INDEX_BITS                     = 22 ;
static const uint32_t ENTRY_INDEX_BIT_MASK                    = 0x003fffff ;	// used for storing (EntryIndex,Friend) couples into a 32bits pointer.
static const uint32_t DELAY_BEFORE_DROP_REQUEST               = 55 ; 			// every 55 secs, for debugging. Should be evey 10 minutes or so.

p3FileDatabase::p3FileDatabase(p3ServiceControl *mpeers)
    : mServCtrl(mpeers), mFLSMtx("p3FileLists")
{
    // make sure the base directory exists

    std::string base_dir = rsAccounts->PathAccountDirectory();

    if(base_dir.empty())
        throw std::runtime_error("Cannot create base directory to store/access file sharing files.") ;

    mFileSharingDir = base_dir + "/" + FILE_SHARING_DIR_NAME ;

    if(!RsDirUtil::checkCreateDirectory(mFileSharingDir))
        throw std::runtime_error("Cannot create base directory to store/access file sharing files.") ;

    // loads existing indexes for friends. Some might be already present here.
	// 
    mRemoteDirectories.clear() ;	// we should load them!
    mOwnId = mpeers->getOwnId() ;

    mLocalSharedDirs = new LocalDirectoryStorage("local_file_store.bin",mOwnId);
    mHashCache = new HashStorage(mFileSharingDir + "/" + HASH_CACHE_FILE_NAME) ;

    mLocalDirWatcher = new LocalDirectoryUpdater(mHashCache,mLocalSharedDirs) ;

	mUpdateFlags = P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED ;
    mLastRemoteDirSweepTS = 0 ;
    mLastCleanupTime = 0 ;

    // This is for the transmission of data

    addSerialType(new RsFileListsSerialiser()) ;
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
        mLocalDirWatcher->forceUpdate();
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
	// tick the watchers, possibly create new ones if additional friends do connect.
	//
	tickWatchers();
	
	// tick the input/output list of update items and process them
	//
	tickRecv() ;
	tickSend() ;

    time_t now = time(NULL) ;

    // cleanup
	// 	- remove/delete shared file lists for people who are not friend anymore
	// 	- 

    if(mLastCleanupTime + 5 < now)
    {
        cleanup();
        mLastCleanupTime = now ;
    }

    static time_t last_print_time = 0;

    if(last_print_time + 20 < now)
    {
        RS_STACK_MUTEX(mFLSMtx) ;
        
#ifdef DEBUG_FILE_HIERARCHY
        mLocalSharedDirs->print();
#endif
        last_print_time = now ;

//#warning this should be removed, but it's necessary atm for updating the GUI
        RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_LOCAL, 0);
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
        {
            if(online_peers.find(mRemoteDirectories[i]->peerId()) != online_peers.end())
            {
                P3FILELISTS_DEBUG() << "Launching recurs sweep of friend directory " << mRemoteDirectories[i]->peerId() << ". Content currently is:" << std::endl;
#ifdef DEBUG_FILE_HIERARCHY
                mRemoteDirectories[i]->print();
#endif

                locked_recursSweepRemoteDirectory(mRemoteDirectories[i],mRemoteDirectories[i]->root(),0) ;
            }

            mRemoteDirectories[i]->checkSave() ;
        }

        mLastRemoteDirSweepTS = now;

#warning hack to make loaded directories show up in the GUI, because the GUI isn_t ready at the time they are actually loaded up.
            RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_FRIENDS, 0);
    }

    return 0;
}

void p3FileDatabase::startThreads()
{
    RS_STACK_MUTEX(mFLSMtx) ;
#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "Starting directory watcher thread..." ;
#endif
    mLocalDirWatcher->start();
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

void p3FileDatabase::tickWatchers()
{
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
        RsTlvKeyValue kv;

        kv.key = WATCH_FILE_ENABLED_SS;
        kv.value = watchEnabled()?"YES":"NO" ;

        rskv->tlvkvs.pairs.push_back(kv);
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
    static const FileStorageFlags PERMISSION_MASK = DIR_FLAGS_BROWSABLE_OTHERS | DIR_FLAGS_NETWORK_WIDE_OTHERS | DIR_FLAGS_BROWSABLE_GROUPS | DIR_FLAGS_NETWORK_WIDE_GROUPS ;

#ifdef  DEBUG_FILE_HIERARCHY
    P3FILELISTS_DEBUG() << "Load list" << std::endl;
#endif

    std::list<SharedDirInfo> dirList;

    for(std::list<RsItem *>::iterator it = load.begin(); it != load.end(); ++it)
    {
        RsConfigKeyValueSet *rskv ;

        if (NULL != (rskv = dynamic_cast<RsConfigKeyValueSet *>(*it)))
        {
            /* make into map */
            std::map<std::string, std::string> configMap;
            std::map<std::string, std::string>::const_iterator mit ;

            for(std::list<RsTlvKeyValue>::const_iterator kit = rskv->tlvkvs.pairs.begin(); kit != rskv->tlvkvs.pairs.end(); ++kit)
            if (kit->key == HASH_CACHE_DURATION_SS)
            {
                uint32_t t=0 ;
                if(sscanf(kit->value.c_str(),"%d",&t) == 1)
                    mHashCache->setRememberHashFilesDuration(t);
            }
            else if(kit->key == WATCH_FILE_DURATION_SS)
            {
                int t=0 ;
                if(sscanf(kit->value.c_str(),"%d",&t) == 1)
                    setWatchPeriod(t);
            }
            else if(kit->key == WATCH_FILE_ENABLED_SS)
            {
                setWatchEnabled(kit->value == "YES") ;
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
            info.shareflags &= ~DIR_FLAGS_NETWORK_WIDE_GROUPS ;	// disabling this flag for know, for consistency reasons

            for(std::set<RsNodeGroupId>::const_iterator itt(fi->parent_groups.ids.begin());itt!=fi->parent_groups.ids.end();++itt)
                info.parent_groups.push_back(*itt) ;

            dirList.push_back(info) ;
        }

        delete *it ;
    }

    /* set directories */
    mLocalSharedDirs->setSharedDirectoryList(dirList);
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

        for(uint32_t i=0;i<mRemoteDirectories.size();++i)
            if(friend_set.find(mRemoteDirectories[i]->peerId()) == friend_set.end())
            {

#ifdef DEBUG_P3FILELISTS
                P3FILELISTS_DEBUG() << "  removing file list of non friend " << mRemoteDirectories[i]->peerId() << std::endl;
#endif

                delete mRemoteDirectories[i];
                mRemoteDirectories[i] = NULL ;

                mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED ;

                friend_set.erase(mRemoteDirectories[i]->peerId());

                mFriendIndexMap.erase(mRemoteDirectories[i]->peerId());
                mFriendIndexTab[i].clear();
            }

        // look through the remaining list of friends, which are the ones for which no remoteDirectoryStorage class has been allocated.
        //
        for(std::set<RsPeerId>::const_iterator it(friend_set.begin());it!=friend_set.end();++it)
        {
            // Check if a remote directory exists for that friend, possibly creating the index.

            uint32_t friend_index = locked_getFriendIndex(*it) ;

            if(mRemoteDirectories.size() > friend_index && mRemoteDirectories[friend_index] != NULL)
                continue ;

#ifdef DEBUG_P3FILELISTS
            P3FILELISTS_DEBUG() << "  adding missing remote dir entry for friend " << *it << ", with index " << friend_index << std::endl;
#endif

            if(mRemoteDirectories.size() <= friend_index)
                mRemoteDirectories.resize(friend_index+1,NULL) ;

            mRemoteDirectories[friend_index] = new RemoteDirectoryStorage(*it,makeRemoteFileName(*it));

            mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_DIRS_CHANGED ;
            mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED ;
        }

        // cancel existing requests for which the peer is offline

        std::set<RsPeerId> online_peers ;
        mServCtrl->getPeersConnected(getServiceInfo().mServiceType, online_peers) ;

        time_t now = time(NULL);

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
        for(uint32_t i=0;i<mFriendIndexTab.size();++i)
            if(mFriendIndexTab[i].isNull())
            {
                found = i ;
                break;
            }

        if(!found)
        {
            found = mFriendIndexTab.size();
            mFriendIndexTab.push_back(pid);
        }

        if(mFriendIndexTab.size() >= (1 << NB_FRIEND_INDEX_BITS) )
        {
            std::cerr << "(EE) FriendIndexTab is full. This is weird. Do you really have more than " << (1<<NB_FRIEND_INDEX_BITS) << " friends??" << std::endl;
            return 1 << NB_FRIEND_INDEX_BITS ;
        }

        mFriendIndexTab[found] = pid ;
        mFriendIndexMap[pid] = found;

        if(mRemoteDirectories.size() <= found)
            mRemoteDirectories.resize(found,NULL) ;

        IndicateConfigChanged();

        return found ;
    }
    else
    {
        if(mRemoteDirectories.size() <= it->second)
            mRemoteDirectories.resize(it->second,NULL) ;

        return it->second;
    }
}

const RsPeerId& p3FileDatabase::locked_getFriendFromIndex(uint32_t indx) const
{
    static const RsPeerId null_id ;

    if(indx >= mFriendIndexTab.size())
        return null_id ;

    if(mFriendIndexTab[indx].isNull())
    {
        std::cerr << "(EE) null friend id requested from index " << indx << ": this is a bug, most likely" << std::endl;
        return null_id ;
    }

    return mFriendIndexTab[indx];
}
bool p3FileDatabase::convertPointerToEntryIndex(const void *p, EntryIndex& e, uint32_t& friend_index)
{
    // trust me, I can do this ;-)

    e   = EntryIndex(  *reinterpret_cast<uint32_t*>(&p) & ENTRY_INDEX_BIT_MASK ) ;
    friend_index = (*reinterpret_cast<uint32_t*>(&p)) >> NB_ENTRY_INDEX_BITS ;

    if(friend_index == 0)
    {
        std::cerr << "(EE) Cannot find friend index in pointer. Encoded value is zero!" << std::endl;
        return false;
    }
    friend_index--;

    return true;
}
bool p3FileDatabase::convertEntryIndexToPointer(const EntryIndex& e, uint32_t fi, void *& p)
{
    // the pointer is formed the following way:
    //
    //		[ 10 bits   |  22 bits ]
    //
    // This means that the whoel software has the following build-in limitation:
    //	  * 1023 friends
    //	  * 4M shared files.

    uint32_t fe = (uint32_t)e ;

    if(fi+1 >= (1<<NB_FRIEND_INDEX_BITS) || fe >= (1<< NB_ENTRY_INDEX_BITS))
    {
        P3FILELISTS_ERROR() << "(EE) cannot convert entry index " << e << " of friend with index " << fi << " to pointer." << std::endl;
        return false ;
    }

    p = reinterpret_cast<void*>( ( (1+fi) << NB_ENTRY_INDEX_BITS ) + (fe & ENTRY_INDEX_BIT_MASK)) ;

    return true;
}

void p3FileDatabase::requestDirUpdate(void *ref)
{
    uint32_t fi;
    DirectoryStorage::EntryIndex e ;

    convertPointerToEntryIndex(ref,e,fi);

    if(fi == 0)
        return ;	// not updating current directory (should we?)

#ifdef DEBUG_P3FILELISTS
    P3FILELISTS_DEBUG() << "Trying to force sync of entry ndex " << e << " to friend " << mRemoteDirectories[fi-1]->peerId() << std::endl;
#endif

    if(generateAndSendSyncRequest(mRemoteDirectories[fi-1],e))
    {
#ifdef DEBUG_P3FILELISTS
        P3FILELISTS_DEBUG() << "  Succeed." << std::endl;
#endif
    }
}

bool p3FileDatabase::findChildPointer(void *ref, int row, void *& result, FileSearchFlags flags) const
{
    RS_STACK_MUTEX(mFLSMtx) ;

    result = NULL ;

    if (ref == NULL)
        if(flags & RS_FILE_HINTS_LOCAL)
        {
            if(row != 0)
                return false ;

            convertEntryIndexToPointer(0,0,result);

            return true ;
        }
        else if(row < mRemoteDirectories.size())
        {
            convertEntryIndexToPointer(mRemoteDirectories[row]->root(),row+1,result);
            return true;
        }
        else
            return false;

    uint32_t fi;
    DirectoryStorage::EntryIndex e ;

    convertPointerToEntryIndex(ref,e,fi);

    // check consistency
    if( (fi == 0 && !(flags & RS_FILE_HINTS_LOCAL)) ||  (fi > 0 && (flags & RS_FILE_HINTS_LOCAL)))
    {
        P3FILELISTS_ERROR() << "(EE) remote request on local index or local request on remote index. This should not happen." << std::endl;
        return false ;
    }
    DirectoryStorage *storage = (fi==0)? ((DirectoryStorage*)mLocalSharedDirs) : ((DirectoryStorage*)mRemoteDirectories[fi-1]);

    // Case where the index is the top of a single person. Can be us, or a friend.

    EntryIndex c = 0;
    bool res = storage->getChildIndex(e,row,c);

    convertEntryIndexToPointer(c,fi,result) ;

    return res;
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
        d.count = 1;
        d.parent = NULL;
        d.prow = -1;
        d.ref = NULL;
        d.name = "root";
        d.hash.clear() ;
        d.path = "";
        d.age = 0;
        d.flags.clear() ;
        d.min_age = 0 ;

        if(flags & RS_FILE_HINTS_LOCAL)
        {
            void *p;
            convertEntryIndexToPointer(0,0,p);

            DirStub stub;
            stub.type = DIR_TYPE_PERSON;
            stub.name = mServCtrl->getOwnId().toStdString();
            stub.ref  = p;
            d.children.push_back(stub);
        }
        else for(uint32_t i=0;i<mRemoteDirectories.size();++i)
        {
            void *p;
            convertEntryIndexToPointer(mRemoteDirectories[i]->root(),i+1,p);

            DirStub stub;
            stub.type = DIR_TYPE_PERSON;
            stub.name = mRemoteDirectories[i]->peerId().toStdString();
            stub.ref  = p;
            d.children.push_back(stub);
        }

        d.count = d.children.size();

#ifdef DEBUG_FILE_HIERARCHY
        P3FILELISTS_DEBUG() << "ExtractData: ref=" << ref << ", flags=" << flags << " : returning this: " << std::endl;
#endif

        return true ;
    }

    uint32_t fi;
    DirectoryStorage::EntryIndex e ;

    convertPointerToEntryIndex(ref,e,fi);

    // check consistency
    if( (fi == 0 && !(flags & RS_FILE_HINTS_LOCAL)) ||  (fi > 0 && (flags & RS_FILE_HINTS_LOCAL)))
    {
        P3FILELISTS_ERROR() << "(EE) remote request on local index or local request on remote index. This should not happen." << std::endl;
        return false ;
    }
    DirectoryStorage *storage = (fi==0)? ((DirectoryStorage*)mLocalSharedDirs) : ((DirectoryStorage*)mRemoteDirectories[fi-1]);

    // Case where the index is the top of a single person. Can be us, or a friend.

    bool res =  storage->extractData(e,d);

    // update indexes. This is a bit hacky, but does the job. The cast to intptr_t is the proper way to convert
    // a pointer into an int.

    convertEntryIndexToPointer((intptr_t)d.ref,fi,d.ref) ;

    for(uint32_t i=0;i<d.children.size();++i)
        convertEntryIndexToPointer((intptr_t)d.children[i].ref,fi,d.children[i].ref);

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

        convertEntryIndexToPointer((intptr_t)d.parent,fi,d.parent) ;
    }

    d.id = storage->peerId();

#ifdef DEBUG_FILE_HIERARCHY
    P3FILELISTS_DEBUG() << "ExtractData: ref=" << ref << ", flags=" << flags << " : returning this: " << std::endl;
    P3FILELISTS_DEBUG() << d << std::endl;
#endif

    return true;
}

int p3FileDatabase::RequestDirDetails(const RsPeerId& uid,const std::string& path, DirDetails &details) const
{
    NOT_IMPLEMENTED();
    return 0;
}
//int p3FileDatabase::RequestDirDetails(const std::string& path, DirDetails &details) const
//{
//    NOT_IMPLEMENTED();
//    return 0;
//}
uint32_t p3FileDatabase::getType(void *ref) const
{
    RS_STACK_MUTEX(mFLSMtx) ;

    EntryIndex e ;
    uint32_t fi;

    if(ref == NULL)
        return DIR_TYPE_ROOT ;

    convertPointerToEntryIndex(ref,e,fi);

    if(e == 0)
        return DIR_TYPE_PERSON ;

    if(fi == 0)
        return mLocalSharedDirs->getEntryType(e) ;
    else
        return mRemoteDirectories[fi-1]->getEntryType(e) ;
}

void p3FileDatabase::forceDirectoryCheck()              // Force re-sweep the directories and see what's changed
{
    mLocalDirWatcher->forceUpdate();
}
bool p3FileDatabase::inDirectoryCheck()
{
    return  mLocalDirWatcher->inDirectoryCheck();
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
    std::list<EntryIndex> firesults;
    {
        RS_STACK_MUTEX(mFLSMtx) ;

        mLocalSharedDirs->searchTerms(keywords,firesults) ;

        for(std::list<EntryIndex>::iterator it(firesults.begin());it!=firesults.end();++it)
        {
            void *p=NULL;
            convertEntryIndexToPointer(*it,0,p);
            *it = (intptr_t)p ;
        }
    }

    return filterResults(firesults,results,flags,client_peer_id) ;
}
int p3FileDatabase::SearchBoolExp(RsRegularExpression::Expression *exp, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& client_peer_id) const
{
    std::list<EntryIndex> firesults;
    {
        RS_STACK_MUTEX(mFLSMtx) ;

        mLocalSharedDirs->searchBoolExp(exp,firesults) ;

        for(std::list<EntryIndex>::iterator it(firesults.begin());it!=firesults.end();++it)
        {
            void *p=NULL;
            convertEntryIndexToPointer(*it,0,p);
            *it = (intptr_t)p ;
        }
    }

    return filterResults(firesults,results,flags,client_peer_id) ;
}
bool p3FileDatabase::search(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const
{
    RS_STACK_MUTEX(mFLSMtx) ;

    if(hintflags & RS_FILE_HINTS_LOCAL)
    {
        std::list<EntryIndex> res;
        mLocalSharedDirs->searchHash(hash,res) ;

        if(res.empty())
            return false;

        EntryIndex indx = *res.begin() ; // no need to report duplicates

        mLocalSharedDirs->getFileInfo(indx,info) ;

        return true;
    }

    if(hintflags & RS_FILE_HINTS_REMOTE)
    {
        NOT_IMPLEMENTED();
        return false;
    }
    return false;
}

int p3FileDatabase::filterResults(const std::list<EntryIndex>& firesults,std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& peer_id) const
{
    results.clear();

    flags &= RS_FILE_HINTS_PERMISSION_MASK;	// remove other flags.

    /* translate/filter results */

    for(std::list<EntryIndex>::const_iterator rit(firesults.begin()); rit != firesults.end(); ++rit)
    {
        DirDetails cdetails ;

        if(!RequestDirDetails ((void*)(intptr_t)*rit,cdetails,RS_FILE_HINTS_LOCAL))
        {
            P3FILELISTS_ERROR() << "(EE) Cannot get dir details for entry " << (void*)(intptr_t)*rit << std::endl;
            continue ;
        }
#ifdef P3FILELISTS_DEBUG
        P3FILELISTS_DEBUG() << "Filtering candidate " << (void*)(intptr_t)(*rit) << ", flags=" << cdetails.flags << ", peer=" << peer_id ;
#endif

        if(!peer_id.isNull())
        {
            FileSearchFlags permission_flags = rsPeers->computePeerPermissionFlags(peer_id,cdetails.flags,cdetails.parent_groups) ;

            if (cdetails.type == DIR_TYPE_FILE && ( permission_flags & flags ))
            {
                cdetails.id.clear() ;
                results.push_back(cdetails);
#ifdef P3FILELISTS_DEBUG
                std::cerr << ": kept" << std::endl ;
#endif
            }
#ifdef P3FILELISTS_DEBUG
            else
                std::cerr << ": discarded" << std::endl ;
#endif
        }
        else
            results.push_back(cdetails);
    }

    return !results.empty() ;
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
//		1 - recursive modif time of the files/dir in the friend's time
//		2 - update TS in the local time
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
      case RS_PKT_SUBTYPE_FILELISTS_SYNC_REQ_ITEM: handleDirSyncRequest( dynamic_cast<RsFileListsSyncRequestItem*>(item) ) ;
         break ;
      case RS_PKT_SUBTYPE_FILELISTS_SYNC_RSP_ITEM: handleDirSyncResponse( dynamic_cast<RsFileListsSyncResponseItem*>(item) ) ;
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
        P3FILELISTS_DEBUG() << "Received directory sync request. hash=" << item->entry_hash << ", flags=" << (void*)(intptr_t)item->flags << ", request id: " << std::hex << item->request_id << std::dec << ", last known TS: " << item->last_known_recurs_modf_TS << std::endl;
#endif

        EntryIndex entry_index = DirectoryStorage::NO_INDEX;

        if(!mLocalSharedDirs->getIndexFromDirHash(item->entry_hash,entry_index))
        {
            P3FILELISTS_ERROR() << "  (EE) Cannot find entry index for hash " << item->entry_hash << ": cannot respond to sync request." << std::endl;
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
            time_t local_recurs_max_time ;
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

void p3FileDatabase::handleDirSyncResponse(RsFileListsSyncResponseItem *sitem)
{
    RsFileListsSyncResponseItem *item = recvAndRebuildItem(sitem) ;

    if(!item)
        return ;

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

    // remove the original request from pending list

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

    // find the correct friend entry

    uint32_t fi = 0 ;

    {
        RS_STACK_MUTEX(mFLSMtx) ;
        fi = locked_getFriendIndex(item->PeerId());

        std::cerr << "  friend index is " << fi ;

        // make sure we have a remote directory for that friend.

        if(mRemoteDirectories.size() <= fi)
            mRemoteDirectories.resize(fi+1,NULL) ;

        if(mRemoteDirectories[fi] == NULL)
            mRemoteDirectories[fi] = new RemoteDirectoryStorage(item->PeerId(),makeRemoteFileName(item->PeerId()));

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
        std::cerr << "  removing directory with index " << entry_index << " because it does not exist." << std::endl;
        mRemoteDirectories[fi]->removeDirectory(entry_index);

        mRemoteDirectories[fi]->print();
    }
    else if(item->flags & RsFileListsItem::FLAGS_ENTRY_UP_TO_DATE)
    {
        std::cerr << "  Directory is up to date. Setting local TS." << std::endl;

        mRemoteDirectories[fi]->setDirectoryUpdateTime(entry_index,time(NULL)) ;
    }
    else if(item->flags & RsFileListsItem::FLAGS_SYNC_DIR_CONTENT)
    {
        std::cerr << "  Item contains directory data. Deserialising/Updating." << std::endl;

        if(mRemoteDirectories[fi]->deserialiseUpdateDirEntry(entry_index,item->directory_content_data))
            RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_FRIENDS, 0);						 // notify the GUI if the hierarchy has changed
        else
            P3FILELISTS_ERROR() << "(EE) Cannot deserialise dir entry. ERROR. "<< std::endl;

#ifdef DEBUG_FILE_HIERARCHY
        P3FILELISTS_DEBUG() << "  new content after update: " << std::endl;
        mRemoteDirectories[fi]->print();
#endif
    }
}

void p3FileDatabase::locked_recursSweepRemoteDirectory(RemoteDirectoryStorage *rds,DirectoryStorage::EntryIndex e,int depth)
{
   time_t now = time(NULL) ;

   std::string indent(2*depth,' ') ;

   // if not up to date, request update, and return (content is not certified, so no need to recurs yet).
   // if up to date, return, because TS is about the last modif TS below, so no need to recurs either.

   // get the info for this entry

#ifdef DEBUG_P3FILELISTS
   P3FILELISTS_DEBUG() << "currently at entry index " << e << std::endl;
#endif

   time_t local_update_TS;

   if(!rds->getDirectoryUpdateTime(e,local_update_TS))
   {
       P3FILELISTS_ERROR() << "  (EE) lockec_recursSweepRemoteDirectory(): cannot get update TS for directory with index " << e << ". This is a consistency bug." << std::endl;
       return;
   }

   // compare TS

   if((e == 0 && now > local_update_TS + DELAY_BETWEEN_REMOTE_DIRECTORY_SYNC_REQ) || local_update_TS == 0)	// we need to compare local times only. We cannot compare local (now) with remote time.
       if(generateAndSendSyncRequest(rds,e))
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
    uint64_t r = 0 ;

    // This is kind of arbitrary. The important thing is that the same ID needs to be generated every time for a given (peer_id,entry index) pair, in a way
    // that cannot be brute-forced or reverse-engineered, which explains the random bias and the usage of the hash, that is itself random.

    for(uint32_t i=0;i<RsPeerId::SIZE_IN_BYTES;++i)
    {
        r ^= (0x011933ff92892a94 + peer_id.toByteArray()[i] * 0x1001fff92ee640f9) ;
        r <<= 8 ;
        r += 0xf392843890321808;
    }
    for(uint32_t i=0;i<RsFileHash::SIZE_IN_BYTES;++i)
    {
        r ^= (0x011933ff92892a94 + hash.toByteArray()[i] * 0x1001fff92ee640f9) ;
        r <<= 8 ;
        r += 0xf392843890321808;
    }

    return r ^ random_bias;
}

bool p3FileDatabase::generateAndSendSyncRequest(RemoteDirectoryStorage *rds,const DirectoryStorage::EntryIndex& e)
{
    RsFileHash entry_hash ;
    time_t now = time(NULL) ;

    time_t max_known_recurs_modf_time ;

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
    P3FILELISTS_DEBUG() << "  Pushing req in pending list with peer id " << data.peer_id << std::endl;
#endif

    mPendingSyncRequests[sync_req_id] = data ;

    sendItem(item) ;	// at end! Because item is destroyed by the process.

    return true;
}





