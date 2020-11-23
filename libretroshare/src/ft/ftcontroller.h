/*******************************************************************************
 * libretroshare/src/ft: ftcontroller.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008 by Robert Fernie <drbob@lunamutt.com>                        *
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
 *******************************************************************************/

#ifndef FT_CONTROLLER_HEADER
#define FT_CONTROLLER_HEADER

/*
 * ftController
 *
 * Top level download controller.
 *
 * inherits configuration (save downloading files)
 * inherits pqiMonitor (knows which peers are online).
 * inherits CacheTransfer (transfers cache files too)
 * inherits RsThread (to control transfers)
 *
 */

class ftFileCreator;
class ftTransferModule;
class ftFileProvider;
class ftSearch;
class ftServer;
class ftExtraList;
class ftDataMultiplex;
class p3turtle ;
class p3ServiceControl;

#include "util/rsthreads.h"
#include "pqi/pqiservicemonitor.h"
#include "pqi/p3cfgmgr.h"

#include "retroshare/rsfiles.h"
#include "rsitems/rsconfigitems.h"

#include <map>


const uint32_t FC_TRANSFER_COMPLETE = 0x0001;

class ftFileControl
{
	public:

		enum { 	DOWNLOADING      = 0, 
					COMPLETED        = 1, 
					ERROR_COMPLETION = 2, 
					QUEUED           = 3,
					PAUSED           = 4,
					CHECKING_HASH    = 5
		};

		ftFileControl();
		ftFileControl( const std::string& fname, const std::string& tmppath, const std::string& dest
		             , uint64_t size, const RsFileHash& hash, TransferRequestFlags flags
		             , ftFileCreator *fc, ftTransferModule *tm);

		std::string	   mName;
		std::string	   mCurrentPath; /* current full path (including name) */
		std::string	   mDestination; /* final full path (including name) */
		ftTransferModule * mTransfer;
		ftFileCreator *    mCreator;
		uint32_t	   mState;
        RsFileHash	   mHash;
		uint64_t	   mSize;
		TransferRequestFlags mFlags;
		rstime_t		mCreateTime;
		uint32_t		mQueuePriority ;
		uint32_t		mQueuePosition ;
};

class ftPendingRequest
{
        public:
        ftPendingRequest(const std::string& fname, const RsFileHash& hash,
                        uint64_t size, const std::string& dest, TransferRequestFlags flags,
									 const std::list<RsPeerId> &srcIds, uint16_t state)
        : mName(fname), mHash(hash), mSize(size),
			mDest(dest), mFlags(flags), mSrcIds(srcIds), mState(state) { return; }

	ftPendingRequest() : mSize(0), mFlags(0), mState(0) { return; }

        std::string mName;
        RsFileHash mHash;
        uint64_t mSize;
        std::string mDest;
        TransferRequestFlags mFlags;
        std::list<RsPeerId> mSrcIds;
	uint16_t mState;
};


class ftController:
        public RsTickingThread, public pqiServiceMonitor, public p3Config
{
public:

		/* Setup */
        ftController(ftDataMultiplex *dm, p3ServiceControl *sc, uint32_t ftServiceId);

		void	setFtSearchNExtra(ftSearch *, ftExtraList *);
		void	setTurtleRouter(p3turtle *) ;
		void	setFtServer(ftServer *) ;
		bool    activate();
		bool 	isActiveAndNoPending();

	void threadTick() override; /// @see RsTickingThread

		/***************************************************************/
		/********************** Controller Access **********************/
		/***************************************************************/

        bool 	FileRequest(const std::string& fname, const RsFileHash& hash,
				uint64_t size, const std::string& dest, TransferRequestFlags flags,
										const std::list<RsPeerId> &sourceIds, uint16_t state = ftFileControl::DOWNLOADING);

		/// Do we already have this file, either in download or in file lists ?
        bool  alreadyHaveFile(const RsFileHash& hash, FileInfo &info);

        bool 	setChunkStrategy(const RsFileHash& hash,FileChunksInfo::ChunkStrategy s);
		void 	setDefaultChunkStrategy(FileChunksInfo::ChunkStrategy s);
        void 	setDefaultEncryptionPolicy(uint32_t s);
        FileChunksInfo::ChunkStrategy	defaultChunkStrategy();
		uint32_t freeDiskSpaceLimit() const ;
		void setFreeDiskSpaceLimit(uint32_t size_in_mb) ;
        uint32_t defaultEncryptionPolicy();

        void setMaxUploadsPerFriend(uint32_t m) ;
        uint32_t getMaxUploadsPerFriend() ;

		void setFilePermDirectDL(uint32_t perm) ;
		uint32_t filePermDirectDL() ;

        bool 	FileCancel(const RsFileHash& hash);
        bool 	FileControl(const RsFileHash& hash, uint32_t flags);
		bool 	FileClearCompleted();
        bool 	FlagFileComplete(const RsFileHash& hash);
        bool  getFileDownloadChunksDetails(const RsFileHash& hash,FileChunksInfo& info);
        bool  setDestinationName(const RsFileHash& hash,const std::string& dest_name) ;
        bool  setDestinationDirectory(const RsFileHash& hash,const std::string& dest_name) ;

		// Download speed
        bool getPriority(const RsFileHash& hash,DwlSpeed& p);
        void setPriority(const RsFileHash& hash,DwlSpeed p);

		// Action on queue position
		//
        void moveInQueue(const RsFileHash& hash,QueueMove mv) ;
		void clearQueue() ;
		void setQueueSize(uint32_t size) ;
		uint32_t getQueueSize() ;

		/* get Details of File Transfers */
        void FileDownloads(std::list<RsFileHash> &hashs);

		/* Directory Handling */
		bool setDownloadDirectory(std::string path);
		bool setPartialsDirectory(std::string path);
		std::string getDownloadDirectory();
		std::string getPartialsDirectory();
        bool 	FileDetails(const RsFileHash &hash, FileInfo &info);

		/***************************************************************/
		/********************** Cache Transfer *************************/
		/***************************************************************/

		/// Returns true is full source availability can be assumed for this peer.
		///
		bool assumeAvailability(const RsPeerId& peer_id) const ;

		/* pqiMonitor callback (also provided mConnMgr pointer!) */
		virtual void    statusChange(const std::list<pqiServicePeer> &plist);

        void addFileSource(const RsFileHash& hash,const RsPeerId& peer_id) ;
        void removeFileSource(const RsFileHash& hash,const RsPeerId& peer_id) ;

	protected:

		void searchForDirectSources() ;
		void tickTransfers() ;

		/***************************************************************/
		/********************** Controller Access **********************/
		/***************************************************************/


		/* p3Config Interface */
		virtual RsSerialiser *setupSerialiser();
		virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
		virtual bool    loadList(std::list<RsItem *>& load);
		bool	loadConfigMap(std::map<std::string, std::string> &configMap);

	private:

		/* RunTime Functions */
		void 	checkDownloadQueue();							// check the whole queue for inactive files

		void  locked_addToQueue(ftFileControl*,int strategy) ;// insert this one into the queue
		void  locked_bottomQueue(uint32_t pos) ; 					// bottom queue file which is at this position
		void  locked_topQueue(uint32_t pos) ; 						// top queue file which is at this position
		void  locked_checkQueueElement(uint32_t pos) ;			// check the state of this element in the queue
		void  locked_queueRemove(uint32_t pos) ;					// delete this element from the queue
		void  locked_swapQueue(uint32_t pos1,uint32_t pos2) ; 	// swap position of the two elements

        bool 	completeFile(const RsFileHash& hash);
		bool    handleAPendingRequest();

		bool    setPeerState(ftTransferModule *tm, const RsPeerId& id,
				uint32_t maxrate, bool online);

		rstime_t last_save_time ;
		rstime_t last_clean_time ;
		/* pointers to other components */

		ftSearch *mSearch;
		ftDataMultiplex *mDataplex;
		ftExtraList *mExtraList;
		p3turtle *mTurtle ;
		ftServer *mFtServer ;
		p3ServiceControl *mServiceCtrl;
		uint32_t mFtServiceType;
		uint32_t mDefaultEncryptionPolicy;
		uint32_t mFilePermDirectDLPolicy;

        uint32_t cnt ;
		RsMutex ctrlMutex;

        std::map<RsFileHash, ftFileControl*> mCompleted;
        std::map<RsFileHash, ftFileControl*> mDownloads;
		std::vector<ftFileControl*> mDownloadQueue ;

		std::string mConfigPath;
		std::string mDownloadPath;
		std::string mPartialsPath;

		/**** SPEED QUEUES ****/

		/* callback list (for File Completion) */
		RsMutex doneMutex;
        std::list<RsFileHash> mDone;

		/* List to Pause File transfers until Caches are properly loaded */
		bool mFtActive;
		bool mFtPendingDone;
		std::list<ftPendingRequest> mPendingRequests;
        std::map<RsFileHash,RsFileTransfer*> mPendingChunkMaps ;

		FileChunksInfo::ChunkStrategy mDefaultChunkStrategy ;

		uint32_t _max_active_downloads ;   // maximum number of simultaneous downloads
		uint32_t _max_uploads_per_friend ; // maximum number of uploads per friend. 0 means unlimited.
};

#endif
