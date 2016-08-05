#include "serialiser/rsserviceids.h"

#include "file_sharing/p3filelists.h"
#include "file_sharing/directory_storage.h"
#include "file_sharing/directory_updater.h"

#include "retroshare/rsids.h"
#include "retroshare/rspeers.h"

#define P3FILELISTS_DEBUG() std::cerr << "p3FileLists: "

static const uint32_t P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED     = 0x0000 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED  = 0x0001 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_LOCAL_DIRS_CHANGED  = 0x0002 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_DIRS_CHANGED = 0x0004 ;

static const uint32_t NB_FRIEND_INDEX_BITS = 10 ;
static const uint32_t NB_ENTRY_INDEX_BITS  = 22 ;
static const uint32_t ENTRY_INDEX_BIT_MASK = 0x003fffff ;	// used for storing (EntryIndex,Friend) couples into a 32bits pointer.

p3FileDatabase::p3FileDatabase(p3ServiceControl *mpeers)
    : mServCtrl(mpeers), mFLSMtx("p3FileLists")
{
	// loads existing indexes for friends. Some might be already present here.
	// 
    mRemoteDirectories.clear() ;	// we should load them!

    mLocalSharedDirs = new LocalDirectoryStorage("local_file_store.bin") ;
    mHashCache = new HashStorage("hash_cache.bin") ;

    mLocalDirWatcher = new LocalDirectoryUpdater(mHashCache,mLocalSharedDirs) ;
    mRemoteDirWatcher = NULL;	// not used yet

    mLocalDirWatcher->start();

	mUpdateFlags = P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED ;
}

void p3FileDatabase::setSharedDirectories(const std::list<SharedDirInfo>& shared_dirs)
{
    mLocalSharedDirs->setSharedDirectoryList(shared_dirs) ;
}
void p3FileDatabase::getSharedDirectories(std::list<SharedDirInfo>& shared_dirs)
{
    mLocalSharedDirs->getSharedDirectoryList(shared_dirs) ;
}
void p3FileDatabase::updateShareFlags(const SharedDirInfo& info)
{
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
    delete mRemoteDirWatcher ;
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

	// cleanup
	// 	- remove/delete shared file lists for people who are not friend anymore
	// 	- 
	cleanup();

    return 0;
}

void p3FileDatabase::startThreads()
{
    std::cerr << "Starting hash cache thread..." ;

    mHashCache->start();
}
void p3FileDatabase::stopThreads()
{
    std::cerr << "Stopping hash cache thread..." ; std::cerr.flush() ;

    mHashCache->fullstop();

    std::cerr << "Done." << std::endl;
}

void p3FileDatabase::tickWatchers()
{
    NOT_IMPLEMENTED();
}

void p3FileDatabase::tickRecv()
{
}
void p3FileDatabase::tickSend()
{
	// go through the list of out requests and send them to the corresponding friends, if they are online.
    NOT_IMPLEMENTED();
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
    RS_STACK_MUTEX(mFLSMtx) ;

    // look through the list of friend directories. Remove those who are not our friends anymore.
	//
	P3FILELISTS_DEBUG() << "Cleanup pass." << std::endl;

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
        P3FILELISTS_DEBUG() << "  adding missing remote dir entry for friend " << *it << std::endl;

        uint32_t i;
        for(i=0;i<mRemoteDirectories.size() && mRemoteDirectories[i] != NULL;++i);

        if(i == mRemoteDirectories.size())
            mRemoteDirectories.push_back(NULL) ;

        mRemoteDirectories[i] = new RemoteDirectoryStorage(*it,makeRemoteFileName(*it));
        mFriendIndexTab[i] = *it ;
        mFriendIndexMap[*it] = i;

        mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED ;
    }

	if(mUpdateFlags)
		IndicateConfigChanged();
}

std::string p3FileDatabase::makeRemoteFileName(const RsPeerId& pid) const
{
#warning we should use the default paths here. Ask p3config
    return "dirlist_"+pid.toStdString()+".txt" ;
}

uint32_t p3FileDatabase::getFriendIndex(const RsPeerId& pid)
{
    std::map<RsPeerId,uint32_t>::const_iterator it = mFriendIndexMap.find(pid) ;

    if(it == mFriendIndexMap.end())
    {
        // allocate a new index for that friend, and tell that we should save.
        uint32_t found = 0 ;
        for(uint32_t i=1;i<mFriendIndexTab.size();++i)
            if(mFriendIndexTab[i].isNull())
            {
                found = i ;
                break;
            }

        if(!found)
        {
            std::cerr << "(EE) FriendIndexTab is full. This is weird. Do you really have more than 1024 friends??" << std::endl;
            return 1024 ;
        }

        mFriendIndexTab[found] = pid ;
        mFriendIndexMap[pid] = found;

        IndicateConfigChanged();

        return found ;
    }
    else
        return it->second;
}

const RsPeerId& p3FileDatabase::getFriendFromIndex(uint32_t indx) const
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
bool p3FileDatabase::convertPointerToEntryIndex(void *p, EntryIndex& e, uint32_t& friend_index)
{
    // trust me, I can do this ;-)

    e   = EntryIndex(  *reinterpret_cast<uint32_t*>(&p) & ENTRY_INDEX_BIT_MASK ) ;
    friend_index = (*reinterpret_cast<uint32_t*>(&p)) >> NB_ENTRY_INDEX_BITS ;

    return true;
}
bool p3FileDatabase::convertEntryIndexToPointer(EntryIndex& e, uint32_t fi, void *& p)
{
    // the pointer is formed the following way:
    //
    //		[ 10 bits   |  22 bits ]
    //
    // This means that the whoel software has the following build-in limitation:
    //	  * 1024 friends
    //	  * 4M shared files.

    uint32_t fe = (uint32_t)e ;

    if(fi >= (1<<NB_FRIEND_INDEX_BITS) || fe >= (1<< NB_ENTRY_INDEX_BITS))
    {
        std::cerr << "(EE) cannot convert entry index " << e << " of friend with index " << fi << " to pointer." << std::endl;
        return false ;
    }

    p = reinterpret_cast<void*>( (fi << NB_ENTRY_INDEX_BITS ) + (fe & ENTRY_INDEX_BIT_MASK)) ;

    return true;
}
int p3FileDatabase::RequestDirDetails(void *ref, DirDetails&, FileSearchFlags) const
{
    uint32_t fi;
    EntryIndex e ;

    convertPointerToEntryIndex(ref,e,fi) ;
#warning code needed here

    return 0;
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
    NOT_IMPLEMENTED();
    return 0;
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
    std::list<EntryIndex> firesults;
    mLocalSharedDirs->searchHash(hash,firesults) ;

    NOT_IMPLEMENTED();
    return false;
}

int p3FileDatabase::SearchKeywords(const std::list<std::string>& keywords, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& client_peer_id)
{
    std::list<EntryIndex> firesults;
    mLocalSharedDirs->searchTerms(keywords,firesults) ;

    return filterResults(firesults,results,flags,client_peer_id) ;
}
int p3FileDatabase::SearchBoolExp(Expression *exp, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& client_peer_id) const
{
    std::list<EntryIndex> firesults;
    mLocalSharedDirs->searchBoolExp(exp,firesults) ;

    return filterResults(firesults,results,flags,client_peer_id) ;
}
bool p3FileDatabase::search(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const
{
    if(hintflags & RS_FILE_HINTS_LOCAL)
    {
        std::list<EntryIndex> res;
        mLocalSharedDirs->searchHash(hash,res) ;

        if(res.empty())
            return false;

        EntryIndex indx = *res.begin() ; // no need to report dupicates

        mLocalSharedDirs->getFileInfo(indx,info) ;

        return true;
    }
    else
    {
        NOT_IMPLEMENTED();
        return false;
    }
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
        RequestDirDetails ((void*)*rit,cdetails,FileSearchFlags(0u));
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
    return mLocalSharedDirs->convertSharedFilePath(path,fullpath) ;
}
