#include "serialiser/rsserviceids.h"

#include "file_sharing/p3filelists.h"
#include "file_sharing/directory_storage.h"
#include "file_sharing/directory_updater.h"

#include "retroshare/rsids.h"

#define P3FILELISTS_DEBUG() std::cerr << "p3FileLists: "

static const uint32_t P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED     = 0x0000 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED  = 0x0001 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_LOCAL_DIRS_CHANGED  = 0x0002 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_DIRS_CHANGED = 0x0004 ;

#define NOT_IMPLEMENTED() { std::cerr << __PRETTY_FUNCTION__ << ": not implemented!" << std::endl; }

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

p3FileDatabase::~p3FileDatabase()
{
    RS_STACK_MUTEX(mFLSMtx) ;

    for(std::map<RsPeerId,RemoteDirectoryStorage*>::const_iterator it(mRemoteDirectories.begin());it!=mRemoteDirectories.end();++it)
        delete it->second ;

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
    mServCtrl->getPeersConnected(getServiceInfo().mServiceType, friend_set);

    for(std::map<RsPeerId,RemoteDirectoryStorage*>::iterator it(mRemoteDirectories.begin());it!=mRemoteDirectories.end();)
		if(friend_set.find(it->first) == friend_set.end())
		{
			P3FILELISTS_DEBUG() << "  removing file list of non friend " << it->first << std::endl;

			delete it->second ;
			std::map<RsPeerId,RemoteDirectoryStorage*>::iterator tmp(it) ;
			++tmp ;
			mRemoteDirectories.erase(it) ;
			it=tmp ;

			mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED ;
		}
		else
			++it ;

	// look through the list of friends, and add a directory storage when it's missing
	//
	for(std::set<RsPeerId>::const_iterator it(friend_set.begin());it!=friend_set.end();++it)
		if(mRemoteDirectories.find(*it) == mRemoteDirectories.end())
		{
			P3FILELISTS_DEBUG() << "  adding missing remote dir entry for friend " << *it << std::endl;

			mRemoteDirectories[*it] = new RemoteDirectoryStorage(*it);

			mUpdateFlags |= P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED ;
		}

	if(mUpdateFlags)
		IndicateConfigChanged();
}

int p3FileDatabase::RequestDirDetails(void *ref, DirDetails&, FileSearchFlags) const
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
