/*
 * RetroShare C++ File lists service.
 *
 *     file_sharing/p3filelists.h
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


//
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

        // ftSearch
        virtual bool search(const RsFileHash &hash, FileSearchFlags hintflags, FileInfo &info) const;
        virtual int  SearchKeywords(const std::list<std::string>& keywords, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& peer_id) ;
        virtual int  SearchBoolExp(RsRegularExpression::Expression *exp, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& peer_id) const ;

		// Interface for browsing dir hierarchy
		//

        void stopThreads() ;
        void startThreads() ;

        bool findChildPointer(void *ref, int row, void *& result, FileSearchFlags flags) const;

        // void * here is the type expected by the abstract model index from Qt. It gets turned into a DirectoryStorage::EntryIndex internally.

        void requestDirUpdate(void *ref) ;	// triggers an update. Used when browsing.
        int RequestDirDetails(void *, DirDetails&, FileSearchFlags) const ;
        uint32_t getType(void *) const ;

        // proxy method used by the web UI. Dont't delete!
        int RequestDirDetails(const RsPeerId& uid, const std::string& path, DirDetails &details)const;

        // set/update shared directories

        void setSharedDirectories(const std::list<SharedDirInfo>& dirs);
        void getSharedDirectories(std::list<SharedDirInfo>& dirs);
        void updateShareFlags(const SharedDirInfo& info) ;
        bool convertSharedFilePath(const std::string& path,std::string& fullpath);

		void setIgnoreLists(const std::list<std::string>& ignored_prefixes,const std::list<std::string>& ignored_suffixes, uint32_t ignore_flags) ;
		bool getIgnoreLists(std::list<std::string>& ignored_prefixes,std::list<std::string>& ignored_suffixes, uint32_t& ignore_flags) ;

		void setIgnoreDuplicates(bool i) ;
		bool ignoreDuplicates() const ;

		void setMaxShareDepth(int i) ;
		int  maxShareDepth() const ;

        // computes/gathers statistics about shared directories

		int getSharedDirStatistics(const RsPeerId& pid,SharedDirStats& stats);

        // interface for hash caching

        void setWatchPeriod(uint32_t seconds);
        uint32_t watchPeriod() ;
        void setWatchEnabled(bool b) ;
        bool watchEnabled() ;

        bool followSymLinks() const;
        void setFollowSymLinks(bool b) ;

        // interfact for directory parsing

		void forceDirectoryCheck();              // Force re-sweep the directories and see what's changed
		bool inDirectoryCheck();
		void togglePauseHashingProcess();
		bool hashingProcessPaused();

    protected:

        int filterResults(const std::list<EntryIndex>& firesults,std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& peer_id) const;
        std::string makeRemoteFileName(const RsPeerId& pid) const;

        // Derived from p3Config
        //
        virtual bool loadList(std::list<RsItem *>& items);
        virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
        virtual RsSerialiser *setupSerialiser() ;

        void cleanup();
        void tickRecv();
        void tickSend();

    private:
        p3ServiceControl *mServCtrl ;
        RsPeerId mOwnId ;

        typedef uint64_t DirSyncRequestId ;

        static DirSyncRequestId makeDirSyncReqId(const RsPeerId& peer_id, const RsFileHash &hash) ;

        // utility functions to send items with some maximum size.

        void splitAndSendItem(RsFileListsSyncResponseItem *ritem);
        RsFileListsSyncResponseItem *recvAndRebuildItem(RsFileListsSyncResponseItem *ritem);

        /*!
         * \brief generateAndSendSyncRequest
         * \param rds	Remote directory storage for the request
         * \param e		Entry index to update
         * \return 		true if the request is correctly sent.
         */
        bool locked_generateAndSendSyncRequest(RemoteDirectoryStorage *rds,const DirectoryStorage::EntryIndex& e);

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

		template<int BYTES> static bool convertEntryIndexToPointer(const EntryIndex &e, uint32_t friend_index, void *& p);
        template<int BYTES> static bool convertPointerToEntryIndex(const void *p, EntryIndex& e, uint32_t& friend_index) ;

        uint32_t locked_getFriendIndex(const RsPeerId& pid);

        void handleDirSyncRequest (RsFileListsSyncRequestItem *) ;
        void handleDirSyncResponse (RsFileListsSyncResponseItem *&) ;

        std::map<RsPeerId,uint32_t> mFriendIndexMap ;
        std::vector<RsPeerId> mFriendIndexTab;

        // Directory synchronization
        //
        struct DirSyncRequestData
        {
            RsPeerId peer_id ;
            time_t request_TS ;
            uint32_t flags ;
        };

        time_t mLastRemoteDirSweepTS ; // TS for friend list update
        std::map<DirSyncRequestId,DirSyncRequestData> mPendingSyncRequests ; // pending requests, waiting for an answer
        std::map<DirSyncRequestId,RsFileListsSyncResponseItem *> mPartialResponseItems;

        void locked_recursSweepRemoteDirectory(RemoteDirectoryStorage *rds, DirectoryStorage::EntryIndex e, int depth);

        // We use a shared file cache as well, to avoid re-hashing files with known modification TS and equal name.
		//
        HashStorage *mHashCache ;

        // Local flags and mutexes

        mutable RsMutex mFLSMtx ;
        uint32_t mUpdateFlags ;
        std::string mFileSharingDir ;
        time_t mLastCleanupTime;
        time_t mLastDataRecvTS ;
};

