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

#include "retroshare/rsfiles.h"
#include "services/p3service.h"

#include "pqi/p3cfgmgr.h"
#include "pqi/p3linkmgr.h"

class RemoteDirectoryStorage ;
class RemoteSharedDirectoryWatcher ;
class LocalSharedDirectoryWatcher ;
class LocalSharedDirectoryMap ;
class HashCache ;

class p3FileLists: public p3Service, public p3Config //, public RsSharedFileService
{
	public:
		typedef uint64_t EntryIndex ;	// this should probably be defined elsewhere

        struct RsFileListSyncRequest
        {
            RsPeerId peerId ;
            EntryIndex index ;
            // [...] more to add here
        };

        p3FileLists(p3LinkMgr *mpeers) ;
        ~p3FileLists();

		/*   
		 */
		 
		// derived from p3Service
		//
		virtual int tick() ;

		// access to own/remote shared files
		//
		virtual bool findLocalFile(const RsFileHash& hash,FileSearchFlags flags,const RsPeerId& peer_id, std::string &fullpath, uint64_t &size,FileStorageFlags& storage_flags,std::list<std::string>& parent_groups) const;

		virtual int SearchKeywords(const std::list<std::string>& keywords, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& peer_id) ;
		virtual int SearchBoolExp(Expression *exp, std::list<DirDetails>& results,FileSearchFlags flags,const RsPeerId& peer_id) const ;

		// Interface for browsing dir hierarchy
		//
		int RequestDirDetails(EntryIndex, DirDetails&, FileSearchFlags) const ;
		uint32_t getType(const EntryIndex&) const ;
		int RequestDirDetails(const std::string& path, DirDetails &details) const ;

      // set/update shared directories
		void setSharedDirectories(const std::list<SharedDirInfo>& dirs);
		void getSharedDirectories(std::list<SharedDirInfo>& dirs);
		void updateShareFlags(const SharedDirInfo& info) ;

		void forceDirectoryCheck();              // Force re-sweep the directories and see what's changed
		bool inDirectoryCheck();

		// Derived from p3Config
		//
    protected:
        virtual bool loadList(std::list<RsItem *>& items);
        virtual bool saveList(const std::list<RsItem *>& items);
        void cleanup();
        void tickRecv();
        void tickSend();
        void tickWatchers();

    private:
        p3LinkMgr *mLinkMgr ;

		// File sync request queues. The fast one is used for online browsing when friends are connected.
		// The slow one is used for background update of file lists.
		//
		std::list<RsFileListSyncRequest> mFastRequestQueue ;
		std::list<RsFileListSyncRequest> mSlowRequestQueue ;

		// Directory storage hierarchies
		//
		std::map<RsPeerId,RemoteDirectoryStorage *> mRemoteDirectories ;
		
		LocalSharedDirectoryMap *mLocalSharedDirs ;

		RemoteSharedDirectoryWatcher *mRemoteDirWatcher ;
		LocalSharedDirectoryWatcher *mLocalDirWatcher ;

		// We use a shared file cache as well, to avoid re-hashing files with known modification TS and equal name.
		//
		HashCache *mHashCache ;

        // Local flags and mutexes

		RsMutex mFLSMtx ;
        uint32_t mUpdateFlags ;
};

