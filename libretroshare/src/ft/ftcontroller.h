/*
 * libretroshare/src/ft: ftcontroller.h
 *
 * File Transfer for RetroShare.
 *
 * Copyright 2008 by Robert Fernie.
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
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

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
class ftExtraList;
class ftDataMultiplex;
class p3turtle ;

#include "dbase/cachestrapper.h"
#include "util/rsthreads.h"
#include "pqi/pqimonitor.h"
#include "pqi/p3cfgmgr.h"

#include "retroshare/rsfiles.h"
#include "serialiser/rsconfigitems.h"

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
		ftFileControl(std::string fname, std::string tmppath, std::string dest,
							uint64_t size, std::string hash, uint32_t flags,
							ftFileCreator *fc, ftTransferModule *tm);

		std::string	   mName;
		std::string	   mCurrentPath; /* current full path (including name) */
		std::string	   mDestination; /* final full path (including name) */
		ftTransferModule * mTransfer;
		ftFileCreator *    mCreator;
		uint32_t	   mState;
		std::string	   mHash;
		uint64_t	   mSize;
		uint32_t	   mFlags;
		time_t		mCreateTime;
		DwlSpeed 	mPriority ;
		uint32_t		mQueuePriority ;
		uint32_t		mQueuePosition ;
};

class ftPendingRequest
{
        public:
        ftPendingRequest(const std::string& fname, const std::string& hash,
                        uint64_t size, const std::string& dest, uint32_t flags,
                        const std::list<std::string> &srcIds)
        : mName(fname), mHash(hash), mSize(size),
        mDest(dest), mFlags(flags),mSrcIds(srcIds) { return; }

        ftPendingRequest() : mSize(0), mFlags(0) { return; }

        std::string mName;
        std::string mHash;
        uint64_t mSize;
        std::string mDest;
        uint32_t mFlags;
        std::list<std::string> mSrcIds;
};


class ftController: public CacheTransfer, public RsThread, public pqiMonitor, public p3Config
{
	public:

		/* Setup */
		ftController(CacheStrapper *cs, ftDataMultiplex *dm, std::string configDir);

		void	setFtSearchNExtra(ftSearch *, ftExtraList *);
		void	setTurtleRouter(p3turtle *) ;
		bool    activate();
		bool 	isActiveAndNoPending();

		virtual void run();

		/***************************************************************/
		/********************** Controller Access **********************/
		/***************************************************************/

		bool 	FileRequest(const std::string& fname, const std::string& hash,
				uint64_t size, const std::string& dest, uint32_t flags,
				const std::list<std::string> &sourceIds);

		/// Do we already have this file, either in download or in file lists ?
		bool  alreadyHaveFile(const std::string& hash, FileInfo &info);

		bool 	setChunkStrategy(const std::string& hash,FileChunksInfo::ChunkStrategy s);
		void 	setDefaultChunkStrategy(FileChunksInfo::ChunkStrategy s);
		FileChunksInfo::ChunkStrategy	defaultChunkStrategy();
		uint32_t freeDiskSpaceLimit() const ;
		void setFreeDiskSpaceLimit(uint32_t size_in_mb) ;

		bool 	FileCancel(const std::string& hash);
		bool 	FileControl(const std::string& hash, uint32_t flags);
		bool 	FileClearCompleted();
		bool 	FlagFileComplete(std::string hash);
		bool  getFileDownloadChunksDetails(const std::string& hash,FileChunksInfo& info);

		// Download speed
		bool getPriority(const std::string& hash,DwlSpeed& p);
		void setPriority(const std::string& hash,DwlSpeed p);

		// Action on queue position
		//
		void moveInQueue(const std::string& hash,QueueMove mv) ;
		void clearQueue() ;
		void setQueueSize(uint32_t size) ;
		uint32_t getQueueSize() ;

		/* get Details of File Transfers */
		bool 	FileDownloads(std::list<std::string> &hashs);

		/* Directory Handling */
		bool 	setDownloadDirectory(std::string path);
		bool 	setPartialsDirectory(std::string path);
		std::string getDownloadDirectory();
		std::string getPartialsDirectory();
		bool 	FileDetails(const std::string &hash, FileInfo &info);

		bool moveFile(const std::string& source,const std::string& dest) ;
		bool copyFile(const std::string& source,const std::string& dest) ;

		/***************************************************************/
		/********************** Cache Transfer *************************/
		/***************************************************************/

		/// Returns true is full source availability can be assumed for this peer.
		///
		bool assumeAvailability(const std::string& peer_id) const ;

		/* pqiMonitor callback (also provided mConnMgr pointer!) */
		virtual void    statusChange(const std::list<pqipeer> &plist);
		void addFileSource(const std::string& hash,const std::string& peer_id) ;
		void removeFileSource(const std::string& hash,const std::string& peer_id) ;

	protected:

		virtual bool RequestCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size);
		virtual bool CancelCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size);

		void cleanCacheDownloads() ;
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

		bool 	completeFile(std::string hash);
		bool    handleAPendingRequest();

		bool    setPeerState(ftTransferModule *tm, std::string id,
				uint32_t maxrate, bool online);

		time_t last_save_time ;
		time_t last_clean_time ;
		/* pointers to other components */

		ftSearch *mSearch;
		ftDataMultiplex *mDataplex;
		ftExtraList *mExtraList;
		p3turtle *mTurtle ;

		RsMutex ctrlMutex;

		std::map<std::string, ftFileControl*> mCompleted;
		std::map<std::string, ftFileControl*> mDownloads;
		std::vector<ftFileControl*> _queue ;

		std::string mConfigPath;
		std::string mDownloadPath;
		std::string mPartialsPath;

		/**** SPEED QUEUES ****/

		/* callback list (for File Completion) */
		RsMutex doneMutex;
		std::list<std::string> mDone;

		/* List to Pause File transfers until Caches are properly loaded */
		bool mFtActive;
		bool mFtPendingDone;
		std::list<ftPendingRequest> mPendingRequests;
		std::map<std::string,RsFileTransfer*> mPendingChunkMaps ;

		FileChunksInfo::ChunkStrategy mDefaultChunkStrategy ;

		uint32_t _max_active_downloads ; // maximum number of simultaneous downloads
};

#endif
