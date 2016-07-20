#include "file_sharing/p3filelists.h"
#include "retroshare/rsids.h"

#define P3FILELISTS_DEBUG() std::cerr << "p3FileLists: " ;

static const uint32_t P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED     = 0x0000 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_MAP_CHANGED  = 0x0001 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_LOCAL_DIRS_CHANGED  = 0x0002 ;
static const uint32_t P3FILELISTS_UPDATE_FLAG_REMOTE_DIRS_CHANGED = 0x0004 ;

p3FileLists::p3FileLists(mPeerPgr *mpeers)
	: mPeers(mpeers)
{
	// loads existing indexes for friends. Some might be already present here.
	// 
	
	// init the directory watchers
	
	// local variables/structures
	//
	mUpdateFlags = P3FILELISTS_UPDATE_FLAG_NOTHING_CHANGED ;
}
void p3FileLists::~p3FileLists()
{
	// delete all pointers
}

void p3FileLists::tick()
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
}

void p3FileLists::tickRecv()
{
}
void p3FileLists::tickSend()
{
	// go through the list of out requests and send them to the corresponding friends, if they are online.
}

void p3FileLists::loadList(std::list<RsItem *>& items)
{
	// This loads
	//
	// 	- list of locally shared directories, and the permissions that go with them
}

void p3FileLists::saveList(const std::list<RsItem *>& items)
{
}

void p3FileLists::cleanup()
{
	// look through the list of friend directories. Remove those who are not our friends anymore.
	//
	P3FILELISTS_DEBUG() << "Cleanup pass." << std::endl;

	std::set<RsPeerId> friend_set ;
	{
#warning we should use a std::set in the first place. Modify rsPeers?
		std::list<RsPeerId> friend_list ;
		mPeers->getFriendList(friend_list) ;

		for(std::list<RsPeerId>::const_iterator it(friend_list.begin());it!=friend_list.end();++it)
			friend_set.insert(*it) ;
	}

	for(std::map<RsPeerId,RemoteDirectoryStorage*>::const_iterator it(mRemoteDirectories.begin());it!=mRemoteDirectories.end();)
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


