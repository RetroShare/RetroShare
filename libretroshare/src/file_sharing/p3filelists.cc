#include "serialiser/rsserviceids.h"

#include "file_sharing/p3filelists.h"
#include "file_sharing/directory_storage.h"
#include "file_sharing/directory_updater.h"
#include "file_sharing/rsfilelistitems.h"

#include "retroshare/rsids.h"
#include "retroshare/rspeers.h"

#include "rsserver/p3face.h"

#define P3FILELISTS_DEBUG() std::cerr << "p3FileLists: "

static const uint32_t P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED     = 0x0000 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED  = 0x0001 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_LOCAL_DIRS_CHANGED  = 0x0002 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_DIRS_CHANGED = 0x0004 ;

static const uint32_t NB_FRIEND_INDEX_BITS                    = 10 ;
static const uint32_t NB_ENTRY_INDEX_BITS                     = 22 ;
static const uint32_t ENTRY_INDEX_BIT_MASK                    = 0x003fffff ;	// used for storing (EntryIndex,Friend) couples into a 32bits pointer.
static const uint32_t DELAY_BETWEEN_REMOTE_DIRECTORY_SYNC_REQ = 60 ; 			// every minute, for debugging. Should be evey 10 minutes or so.
static const uint32_t DELAY_BEFORE_DROP_REQUEST               = 55 ; 			// every 55 secs, for debugging. Should be evey 10 minutes or so.

p3FileDatabase::p3FileDatabase(p3ServiceControl *mpeers)
    : mServCtrl(mpeers), mFLSMtx("p3FileLists")
{
	// loads existing indexes for friends. Some might be already present here.
	// 
    mRemoteDirectories.clear() ;	// we should load them!
    mOwnId = mpeers->getOwnId() ;

    mLocalSharedDirs = new LocalDirectoryStorage("local_file_store.bin",mOwnId);
    mHashCache = new HashStorage("hash_cache.bin") ;

    mLocalDirWatcher = new LocalDirectoryUpdater(mHashCache,mLocalSharedDirs) ;

	mUpdateFlags = P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED ;
    mLastRemoteDirSweepTS = 0 ;

    addSerialType(new RsFileListsSerialiser()) ;
}

RsSerialiser *p3FileDatabase::setupSerialiser()
{
    RsSerialiser *rss = new RsSerialiser ;
    rss->addSerialType(new RsFileListsSerialiser()) ;
    rss->addSerialType(new RsGeneralConfigSerialiser());

    return rss ;
}
void p3FileDatabase::setSharedDirectories(const std::list<SharedDirInfo>& shared_dirs)
{
    RS_STACK_MUTEX(mFLSMtx) ;

    mLocalSharedDirs->setSharedDirectoryList(shared_dirs) ;
    mLocalDirWatcher->forceUpdate();
}
void p3FileDatabase::getSharedDirectories(std::list<SharedDirInfo>& shared_dirs)
{
    RS_STACK_MUTEX(mFLSMtx) ;
    mLocalSharedDirs->getSharedDirectoryList(shared_dirs) ;
}
void p3FileDatabase::updateShareFlags(const SharedDirInfo& info)
{
    RS_STACK_MUTEX(mFLSMtx) ;
    mLocalSharedDirs->updateShareFlags(info) ;
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

    static time_t last_cleanup_time = 0;

#warning we should use members here, not static
    if(last_cleanup_time + 5 < now)
    {
        cleanup();
        last_cleanup_time = now ;
    }

    static time_t last_print_time = 0;

    if(last_print_time + 20 < now)
    {
        RS_STACK_MUTEX(mFLSMtx) ;
        
        mLocalSharedDirs->print();
        last_print_time = now ;

//#warning this should be removed, but it's necessary atm for updating the GUI
        RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_FRIENDS, 0);
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
#warning we need to make sure that one req per directory will not cause to keep re-asking the top level dirs.
    if(mLastRemoteDirSweepTS + 5 < now)
    {
        RS_STACK_MUTEX(mFLSMtx) ;

        std::set<RsPeerId> online_peers ;
        mServCtrl->getPeersConnected(getServiceInfo().mServiceType, online_peers) ;

        for(uint32_t i=0;i<mRemoteDirectories.size();++i)
            if(online_peers.find(mRemoteDirectories[i]->peerId()) != online_peers.end())
            {
                std::cerr << "Launching recurs sweep of friend directory " << mRemoteDirectories[i]->peerId() << ". Content currently is:" << std::endl;
                mRemoteDirectories[i]->print();

                locked_recursSweepRemoteDirectory(mRemoteDirectories[i],mRemoteDirectories[i]->root()) ;
            }

        mLastRemoteDirSweepTS = now;
    }

    return 0;
}

void p3FileDatabase::startThreads()
{
    RS_STACK_MUTEX(mFLSMtx) ;
    std::cerr << "Starting directory watcher thread..." ;
    mLocalDirWatcher->start();
    std::cerr << "Done." << std::endl;
}
void p3FileDatabase::stopThreads()
{
    RS_STACK_MUTEX(mFLSMtx) ;
    std::cerr << "Stopping hash cache thread..." ; std::cerr.flush() ;
    mHashCache->fullstop();
    std::cerr << "Done." << std::endl;

    std::cerr << "Stopping directory watcher thread..." ; std::cerr.flush() ;
    mLocalDirWatcher->fullstop();
    std::cerr << "Done." << std::endl;
}

void p3FileDatabase::tickWatchers()
{
}
bool p3FileDatabase::loadList(std::list<RsItem *>& items)
{
	// This loads
	//
	// 	- list of locally shared directories, and the permissions that go with them

    NOT_IMPLEMENTED();

    return true ;
}

bool p3FileDatabase::saveList(bool &cleanup, std::list<RsItem *>&)
{
    NOT_IMPLEMENTED();
    return true ;
}

void p3FileDatabase::cleanup()
{
    {
        RS_STACK_MUTEX(mFLSMtx) ;

        std::cerr << "p3FileDatabase::cleanup()" << std::endl;

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
                P3FILELISTS_DEBUG() << "  removing file list of non friend " << mRemoteDirectories[i]->peerId() << std::endl;

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

            P3FILELISTS_DEBUG() << "  adding missing remote dir entry for friend " << *it << ", with index " << friend_index << std::endl;

            if(mRemoteDirectories.size() <= friend_index)
                mRemoteDirectories.resize(friend_index+1,NULL) ;

            mRemoteDirectories[friend_index] = new RemoteDirectoryStorage(*it,makeRemoteFileName(*it));

            mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED ;
        }

        // cancel existing requests for which the peer is offline

        std::set<RsPeerId> online_peers ;
        mServCtrl->getPeersConnected(getServiceInfo().mServiceType, online_peers) ;

        time_t now = time(NULL);

        for(std::map<DirSyncRequestId,DirSyncRequestData>::iterator it = mPendingSyncRequests.begin();it!=mPendingSyncRequests.end();)
            if(online_peers.find(it->second.peer_id) == online_peers.end() || it->second.request_TS + DELAY_BEFORE_DROP_REQUEST < now)
            {
                std::cerr << "  removing pending request " << std::hex << it->first << std::dec << " for peer " << it->second.peer_id << ", because peer is offline or request is too old." << std::endl;

                std::map<DirSyncRequestId,DirSyncRequestData>::iterator tmp(it);
                ++tmp;
                mPendingSyncRequests.erase(it) ;
                it = tmp;
            }
            else
            {
                std::cerr << "  keeping request " << std::hex << it->first << std::dec << " for peer " << it->second.peer_id << std::endl;
                ++it ;
            }
    }
}

std::string p3FileDatabase::makeRemoteFileName(const RsPeerId& pid) const
{
#warning we should use the default paths here. Ask p3config
    return "dirlist_"+pid.toStdString()+".txt" ;
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
        std::cerr << "(EE) cannot convert entry index " << e << " of friend with index " << fi << " to pointer." << std::endl;
        return false ;
    }

    p = reinterpret_cast<void*>( ( (1+fi) << NB_ENTRY_INDEX_BITS ) + (fe & ENTRY_INDEX_BIT_MASK)) ;

    return true;
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
        std::cerr << "ExtractData: ref=" << ref << ", flags=" << flags << " : returning this: " << std::endl;
        std::cerr << d << std::endl;
#endif

        return true ;
    }

    uint32_t fi;
    DirectoryStorage::EntryIndex e ;

    convertPointerToEntryIndex(ref,e,fi);

    // check consistency
    if( (fi == 0 && !(flags & RS_FILE_HINTS_LOCAL)) ||  (fi > 0 && (flags & RS_FILE_HINTS_LOCAL)))
    {
        std::cerr << "remote request on local index or local request on remote index. This should not happen." << std::endl;
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
    std::cerr << "ExtractData: ref=" << ref << ", flags=" << flags << " : returning this: " << std::endl;
    std::cerr << d << std::endl;
#endif

    return true;
}

int p3FileDatabase::RequestDirDetails(const RsPeerId& uid,const std::string& path, DirDetails &details) const
{
    NOT_IMPLEMENTED();
    return 0;
}
int p3FileDatabase::RequestDirDetails(const std::string& path, DirDetails &details) const
{
    NOT_IMPLEMENTED();
    return 0;
}
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
    NOT_IMPLEMENTED();
}
bool p3FileDatabase::inDirectoryCheck()
{
    NOT_IMPLEMENTED();
    return 0;
}
void p3FileDatabase::setWatchPeriod(uint32_t seconds)
{
    NOT_IMPLEMENTED();
}
uint32_t p3FileDatabase::watchPeriod()
{
    NOT_IMPLEMENTED();
    return 0;
}
void p3FileDatabase::setRememberHashCacheDuration(uint32_t days)
{
    NOT_IMPLEMENTED();
}
uint32_t p3FileDatabase::rememberHashCacheDuration()
{
    NOT_IMPLEMENTED();
    return 0;
}
void p3FileDatabase::clearHashCache()
{
    NOT_IMPLEMENTED();
}
bool p3FileDatabase::rememberHashCache()
{
    NOT_IMPLEMENTED();
    return false;
}
void p3FileDatabase::setRememberHashCache(bool)
{
    NOT_IMPLEMENTED();
}

bool p3FileDatabase::findLocalFile(const RsFileHash& hash,FileSearchFlags flags,const RsPeerId& peer_id, std::string &fullpath, uint64_t &size,FileStorageFlags& storage_flags,std::list<std::string>& parent_groups) const
{
    RS_STACK_MUTEX(mFLSMtx) ;

    std::list<EntryIndex> firesults;
    mLocalSharedDirs->searchHash(hash,firesults) ;

    NOT_IMPLEMENTED();
    return false;
}

int p3FileDatabase::SearchKeywords(const std::list<std::string>& keywords, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& client_peer_id)
{
    RS_STACK_MUTEX(mFLSMtx) ;

    std::list<EntryIndex> firesults;
    mLocalSharedDirs->searchTerms(keywords,firesults) ;

    return filterResults(firesults,results,flags,client_peer_id) ;
}
int p3FileDatabase::SearchBoolExp(Expression *exp, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& client_peer_id) const
{
    RS_STACK_MUTEX(mFLSMtx) ;

    std::list<EntryIndex> firesults;
    mLocalSharedDirs->searchBoolExp(exp,firesults) ;

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

#ifdef P3FILELISTS_DEBUG
    if((flags & ~RS_FILE_HINTS_PERMISSION_MASK) > 0)
        std::cerr << "(EE) ***** FileIndexMonitor:: Flags ERROR in filterResults!!" << std::endl;
#endif
    /* translate/filter results */

    for(std::list<EntryIndex>::const_iterator rit(firesults.begin()); rit != firesults.end(); ++rit)
    {
        DirDetails cdetails ;
        RequestDirDetails ((void*)(intptr_t)*rit,cdetails,FileSearchFlags(0u));
#ifdef P3FILELISTS_DEBUG
        std::cerr << "Filtering candidate " << (*rit) << ", flags=" << cdetails.flags << ", peer=" << peer_id ;
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
//	Remote dirs store the last modif time of the files/dir in the friend's time
//		- local node sends the last known modf time to friends,
//		- friends respond with either a full directory content, or an acknowledge that the time is right
//

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
         std::cerr << "(EE) unhandled packet subtype " << item->PacketSubType() << " in " << __PRETTY_FUNCTION__ << std::endl;
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

        P3FILELISTS_DEBUG() << "Received directory sync request. index=" << item->entry_index << ", flags=" << (void*)(intptr_t)item->flags << ", request id: " << std::hex << item->request_id << std::dec << ", last known TS: " << item->last_known_recurs_modf_TS << std::endl;

        uint32_t entry_type = mLocalSharedDirs->getEntryType(item->entry_index) ;
        ritem->PeerId(item->PeerId()) ;
        ritem->request_id = item->request_id;
        ritem->entry_index = item->entry_index ;

        if(entry_type != DIR_TYPE_DIR)
        {
            P3FILELISTS_DEBUG() << "  Directory does not exist anymore, or is not a directory. Answering with proper flags." << std::endl;

            ritem->flags = RsFileListsItem::FLAGS_SYNC_RESPONSE | RsFileListsItem::FLAGS_ENTRY_WAS_REMOVED ;
        }
        else
        {
            time_t local_recurs_max_time,local_update_time;
            mLocalSharedDirs->getDirUpdateTS(item->entry_index,local_recurs_max_time,local_update_time);

            if(item->last_known_recurs_modf_TS < local_recurs_max_time)
            {
                P3FILELISTS_DEBUG() << "  Directory is more recent than what the friend knows. Sending full dir content as response." << std::endl;

                ritem->flags = RsFileListsItem::FLAGS_SYNC_RESPONSE | RsFileListsItem::FLAGS_SYNC_DIR_CONTENT;
                ritem->last_known_recurs_modf_TS = local_recurs_max_time;

                mLocalSharedDirs->serialiseDirEntry(item->entry_index,ritem->directory_content_data) ;
            }
            else
            {
                P3FILELISTS_DEBUG() << "  Directory is up to date w.r.t. what the friend knows. Sending ACK." << std::endl;

                ritem->flags = RsFileListsItem::FLAGS_SYNC_RESPONSE | RsFileListsItem::FLAGS_ENTRY_UP_TO_DATE ;
                ritem->last_known_recurs_modf_TS = local_recurs_max_time ;
            }
        }
    }

    // sends the response.

    sendItem(ritem);
}

void p3FileDatabase::handleDirSyncResponse(RsFileListsSyncResponseItem *item)
{
    P3FILELISTS_DEBUG() << "Handling sync response for directory with index " << item->entry_index << std::endl;

    // remove the original request from pending list

    {
        RS_STACK_MUTEX(mFLSMtx) ;

        std::map<DirSyncRequestId,DirSyncRequestData>::iterator it = mPendingSyncRequests.find(item->request_id) ;

        if(it == mPendingSyncRequests.end())
        {
            std::cerr << "  request " << std::hex << item->request_id << std::dec << " cannot be found. ERROR!" << std::endl;
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
    }

    if(item->flags & RsFileListsItem::FLAGS_ENTRY_WAS_REMOVED)
    {
        P3FILELISTS_DEBUG() << "  removing directory with index " << item->entry_index << " because it does not exist." << std::endl;
        mRemoteDirectories[fi]->removeDirectory(item->entry_index);
    }
    else if(item->flags & RsFileListsItem::FLAGS_ENTRY_UP_TO_DATE)
    {
        P3FILELISTS_DEBUG() << "  Directory is up to date. Setting local TS." << std::endl;

        mRemoteDirectories[fi]->setDirUpdateTS(item->entry_index,item->last_known_recurs_modf_TS,time(NULL));
    }
    else if(item->flags & RsFileListsItem::FLAGS_SYNC_DIR_CONTENT)
    {
        P3FILELISTS_DEBUG() << "  Item contains directory data. Deserialising/Updating." << std::endl;

        if(mRemoteDirectories[fi]->deserialiseDirEntry(item->entry_index,item->directory_content_data))
            RsServer::notify()->notifyListChange(NOTIFY_LIST_DIRLIST_FRIENDS, 0);						 // notify the GUI if the hierarchy has changed
        else
            std::cerr << "(EE) Cannot deserialise dir entry. ERROR. "<< std::endl;

        std::cerr << "  new content after update: " << std::endl;
        mRemoteDirectories[fi]->print();
    }
}

void p3FileDatabase::locked_recursSweepRemoteDirectory(RemoteDirectoryStorage *rds,DirectoryStorage::EntryIndex e)
{
   time_t now = time(NULL) ;

   // if not up to date, request update, and return (content is not certified, so no need to recurs yet).
   // if up to date, return, because TS is about the last modif TS below, so no need to recurs either.

   // get the info for this entry

   time_t recurs_max_modf_TS_remote_time,local_update_TS;

   if(!rds->getDirUpdateTS(e,recurs_max_modf_TS_remote_time,local_update_TS))
   {
       std::cerr << "(EE) lockec_recursSweepRemoteDirectory(): cannot get update TS for directory with index " << e << ". This is a consistency bug." << std::endl;
       return;
   }

   // compare TS

   if(now > local_update_TS + DELAY_BETWEEN_REMOTE_DIRECTORY_SYNC_REQ)	// we need to compare local times only. We cannot compare local (now) with remote time.
   {
        // check if a request already exists and is not too old either: no need to re-ask.

        DirSyncRequestId sync_req_id = makeDirSyncReqId(rds->peerId(),e) ;

        std::map<DirSyncRequestId,DirSyncRequestData>::iterator it = mPendingSyncRequests.find(sync_req_id) ;

        if(it != mPendingSyncRequests.end())
        {
            P3FILELISTS_DEBUG() << "Not asking for sync of directory " << e << " to friend " << rds->peerId() << " because a recent pending request still exists." << std::endl;
            return ;
        }

        P3FILELISTS_DEBUG() << "Asking for sync of directory " << e << " to peer " << rds->peerId() << " because it's " << (now - local_update_TS) << " secs old since last check." << std::endl;

        RsFileListsSyncRequestItem *item = new RsFileListsSyncRequestItem ;
        item->entry_index =  e ;
        item->flags = RsFileListsItem::FLAGS_SYNC_REQUEST ;
        item->request_id = sync_req_id ;
        item->last_known_recurs_modf_TS = recurs_max_modf_TS_remote_time ;
        item->PeerId(rds->peerId()) ;

        DirSyncRequestData data ;

        data.request_TS = now ;
        data.peer_id = item->PeerId();
        data.flags = item->flags;

        std::cerr << "Pushing req in pending list with peer id " << data.peer_id << std::endl;

        mPendingSyncRequests[sync_req_id] = data ;

        sendItem(item) ;	// at end! Because item is destroyed by the process.

        // Dont recurs into sub-directories, since we dont know yet were to go.

        return ;
   }

   for(DirectoryStorage::DirIterator it(rds,e);it;++it)
       locked_recursSweepRemoteDirectory(rds,*it);
}

p3FileDatabase::DirSyncRequestId p3FileDatabase::makeDirSyncReqId(const RsPeerId& peer_id,DirectoryStorage::EntryIndex e)
{
    uint64_t r = e ;

    // This is kind of arbitrary. The important thing is that the same ID needs to be generated for a given (peer_id,e) pair.

    for(uint32_t i=0;i<RsPeerId::SIZE_IN_BYTES;++i)
        r += (0x3ff92892a94 * peer_id.toByteArray()[i] + r) ^ 0x39e83784378aafe3;

    return r;
}




