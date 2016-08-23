// This class is responsible for
//		- maintaining a list of shared file hierarchies for each known friends
//		- talking to the GUI
//			- providing handles for the directory tree listing GUI
//			- providing search handles for FT
//		- keeping these lists up to date
//		- sending our own file list to friends depending on the defined access rights
//		- serving file search requests from other services such as file transfer
//
//	p3FileList does the following micro-tasks:
//		- tick the watchers
//		- get incoming info from the service layer, which can be:
//			- directory content request => the directory content is shared to the friend
//			- directory content         => the directory watcher is notified
//		- keep two queues of update requests: 
//			- fast queue that is handled in highest priority. This one is used for e.g. updating while browsing.
//			- slow queue that is handled slowly. Used in background update of shared directories.
//
// The file lists are not directry updated. A FileListWatcher class is responsible for this
// in every case. 
//
#pragma once

#include "ft/ftsearch.h"
#include "retroshare/rsfiles.h"
#include "services/p3service.h"

#include "file_sharing/hash_cache.h"
#include "file_sharing/directory_storage.h"

#include "pqi/p3cfgmgr.h"
#include "pqi/p3linkmgr.h"

class RemoteDirectoryUpdater ;
class LocalDirectoryUpdater ;

class RemoteDirectoryStorage ;
class LocalDirectoryStorage ;

class RsFileListsSyncRequestItem ;
class RsFileListsSyncResponseItem ;

class HashStorage ;

class p3FileDatabase: public p3Service, public p3Config, public ftSearch //, public RsSharedFileService
{
	public:
        typedef DirectoryStorage::EntryIndex EntryIndex;	// this should probably be defined elsewhere

        virtual RsServiceInfo getServiceInfo();

        struct RsFileListSyncRequest
        {
            RsPeerId peerId ;
            EntryIndex index ;
            // [...] more to add here
        };

        p3FileDatabase(p3ServiceControl *mpeers) ;
        ~p3FileDatabase();

        /*!
        * \brief forceSyncWithPeers
        *
        * Forces the synchronisation of the database with connected peers. This is triggered when e.g. a new group of friend is created, or when
        * a friend was added/removed from a group.
        */
        void forceSyncWithPeers() { NOT_IMPLEMENTED() ; }

		// derived from p3Service
		//
		virtual int tick() ;

		// access to own/remote shared files
		//
		virtual bool findLocalFile(const RsFileHash& hash,FileSearchFlags flags,const RsPeerId& peer_id, std::string &fullpath, uint64_t &size,FileStorageFlags& storage_flags,std::list<std::string>& parent_groups) const;

		virtual int SearchKeywords(const std::list<std::string>& keywords, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& peer_id) ;
		virtual int SearchBoolExp(Expression *exp, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& peer_id) const ;

        // ftSearch
        virtual bool search(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const;

		// Interface for browsing dir hierarchy
		//

        void stopThreads() ;
        void startThreads() ;

        int RequestDirDetails(const RsPeerId& uid, const std::string& path, DirDetails &details)const;
		int RequestDirDetails(const std::string& path, DirDetails &details) const ;

        // void * here is the type expected by the abstract model index from Qt. It gets turned into a DirectoryStorage::EntryIndex internally.

        int RequestDirDetails(void *, DirDetails&, FileSearchFlags) const ;
        uint32_t getType(void *) const ;

        // set/update shared directories

        void setSharedDirectories(const std::list<SharedDirInfo>& dirs);
        void getSharedDirectories(std::list<SharedDirInfo>& dirs);
        void updateShareFlags(const SharedDirInfo& info) ;
        bool convertSharedFilePath(const std::string& path,std::string& fullpath);

        // interface for hash caching

        void setWatchPeriod(uint32_t seconds);
        uint32_t watchPeriod() ;
        void setRememberHashCacheDuration(uint32_t days) ;
        uint32_t rememberHashCacheDuration() ;
        void clearHashCache() ;
        bool rememberHashCache() ;
        void setRememberHashCache(bool) ;

        // interfact for directory parsing

		void forceDirectoryCheck();              // Force re-sweep the directories and see what's changed
		bool inDirectoryCheck();

        // debug
        void full_print();

    protected:

        int filterResults(const std::list<EntryIndex>& firesults,std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& peer_id) const;
        std::string makeRemoteFileName(const RsPeerId& pid) const;

        // Derived from p3Config
        //
        virtual bool loadList(std::list<RsItem *>& items);
        virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
        virtual RsSerialiser *setupSerialiser() { return NULL;}

        void cleanup();
        void tickRecv();
        void tickSend();
        void tickWatchers();

    private:
        p3ServiceControl *mServCtrl ;
        RsPeerId mOwnId ;

        typedef uint64_t DirSyncRequestId ;

        static DirSyncRequestId makeDirSyncReqId(const RsPeerId& peer_id,DirectoryStorage::EntryIndex e) ;

		// File sync request queues. The fast one is used for online browsing when friends are connected.
		// The slow one is used for background update of file lists.
		//
        std::map<DirSyncRequestId,RsFileListSyncRequest> mFastRequestQueue ;
        std::map<DirSyncRequestId,RsFileListSyncRequest> mSlowRequestQueue ;

		// Directory storage hierarchies
		//
        // The remote one is the reference for the PeerId index below:
        //     RemoteDirectories[ getFriendIndex(pid) - 1] = RemoteDirectoryStorage(pid)

        std::vector<RemoteDirectoryStorage *> mRemoteDirectories ;
        LocalDirectoryStorage *mLocalSharedDirs ;

        LocalDirectoryUpdater *mLocalDirWatcher ;

        // utility functions to make/get a pointer out of an (EntryIndex,PeerId) pair. This is further documented in the .cc

        static bool convertEntryIndexToPointer(const EntryIndex &e, uint32_t friend_index, void *& p);
        static bool convertPointerToEntryIndex(const void *p, EntryIndex& e, uint32_t& friend_index) ;
        uint32_t locked_getFriendIndex(const RsPeerId& pid);
        const RsPeerId& locked_getFriendFromIndex(uint32_t indx) const;

        void handleDirSyncRequest (RsFileListsSyncRequestItem *) ;
        void handleDirSyncResponse (RsFileListsSyncResponseItem *) ;

        std::map<RsPeerId,uint32_t> mFriendIndexMap ;
        std::vector<RsPeerId> mFriendIndexTab;

        // Directory synchronization
        //
        struct DirSyncRequestData
        {
            time_t request_TS ;
            uint32_t flags ;
        };

        time_t mLastRemoteDirSweepTS ; // TS for friend list update
        std::map<DirSyncRequestId,DirSyncRequestData> mPendingSyncRequests ; // pending requests, waiting for an answer

        void locked_recursSweepRemoteDirectory(RemoteDirectoryStorage *rds,DirectoryStorage::EntryIndex e);

        // We use a shared file cache as well, to avoid re-hashing files with known modification TS and equal name.
		//
        HashStorage *mHashCache ;

        // Local flags and mutexes

        mutable RsMutex mFLSMtx ;
        uint32_t mUpdateFlags ;
};

