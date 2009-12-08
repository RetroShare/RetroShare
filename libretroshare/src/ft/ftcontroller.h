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

#include "rsiface/rsfiles.h"
#include "serialiser/rsconfigitems.h"

#include <map>


const uint32_t FC_TRANSFER_COMPLETE = 0x0001;

//timeouts in seconds
const int TIMOUT_CACHE_FILE_TRANSFER = 300;

class ftFileControl
{
	public:

        enum {DOWNLOADING,COMPLETED,ERROR_COMPLETION};

	ftFileControl();
        ftFileControl(std::string fname, std::string tmppath, std::string dest,
		uint64_t size, std::string hash, uint32_t flags,
		ftFileCreator *fc, ftTransferModule *tm, uint32_t cb_flags);

	std::string	   mName;
	std::string	   mCurrentPath; /* current full path (including name) */
	std::string	   mDestination; /* final full path (including name) */
	ftTransferModule * mTransfer;
	ftFileCreator *    mCreator;
	uint32_t	   mState;
	std::string	   mHash;
	uint64_t	   mSize;
	uint32_t	   mFlags;
	bool		   mDoCallback;
	uint32_t	   mCallbackCode;
	time_t		   mCreateTime;
};

class ftPendingRequest
{
        public:
        ftPendingRequest(std::string fname, std::string hash,
                        uint64_t size, std::string dest, uint32_t flags,
                        std::list<std::string> &srcIds)
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

void	setShareDownloadDirectory(bool value);
bool	getShareDownloadDirectory();

virtual void run();

	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

bool 	FileRequest(std::string fname, std::string hash,
			uint64_t size, std::string dest, uint32_t flags,
			std::list<std::string> &sourceIds);

bool 	setChunkStrategy(const std::string& hash,FileChunksInfo::ChunkStrategy s);

bool 	FileCancel(std::string hash);
bool 	FileControl(std::string hash, uint32_t flags);
bool 	FileClearCompleted();
bool 	FlagFileComplete(std::string hash);
bool  getFileChunksDetails(const std::string& hash,FileChunksInfo& info);

	/* get Details of File Transfers */
bool 	FileDownloads(std::list<std::string> &hashs);

	/* Directory Handling */
bool 	setDownloadDirectory(std::string path);
bool 	setPartialsDirectory(std::string path);
std::string getDownloadDirectory();
std::string getPartialsDirectory();
bool 	FileDetails(std::string hash, FileInfo &info);

bool moveFile(const std::string& source,const std::string& dest) ;

	/***************************************************************/
	/********************** Cache Transfer *************************/
	/***************************************************************/

protected:

virtual bool RequestCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size);
virtual bool CancelCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size);

	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

	/* pqiMonitor callback (also provided mConnMgr pointer!) */
	public:
virtual void    statusChange(const std::list<pqipeer> &plist);
void addFileSource(const std::string& hash,const std::string& peer_id) ;
void removeFileSource(const std::string& hash,const std::string& peer_id) ;


	/* p3Config Interface */
        protected:
virtual RsSerialiser *setupSerialiser();
virtual std::list<RsItem *> saveList(bool &cleanup);
virtual bool    loadList(std::list<RsItem *> load);
bool	loadConfigMap(std::map<std::string, std::string> &configMap);

	private:

	/* RunTime Functions */
void 	checkDownloadQueue();
bool 	completeFile(std::string hash);
bool    handleAPendingRequest();

bool    setPeerState(ftTransferModule *tm, std::string id,
                        uint32_t maxrate, bool online);

	/* pointers to other components */

	ftSearch *mSearch;
	ftDataMultiplex *mDataplex;
	ftExtraList *mExtraList;
	p3turtle *mTurtle ;

	RsMutex ctrlMutex;

	std::list<FileInfo> incomingQueue;
	std::map<std::string, ftFileControl> mCompleted;


        std::map<std::string, ftFileControl> mDownloads;

	//std::map<std::string, ftTransferModule *> mTransfers;
	//std::map<std::string, ftFileCreator *> mFileCreators;

	std::string mConfigPath;
	std::string mDownloadPath;
	std::string mPartialsPath;

	/**** SPEED QUEUES ****/
	std::list<std::string> mSlowQueue;
	std::list<std::string> mStreamQueue;
	std::list<std::string> mFastQueue;

	/* callback list (for File Completion) */
	RsMutex doneMutex;
	std::list<std::string> mDone;

	/* List to Pause File transfers until Caches are properly loaded */
	bool mFtActive;
        bool mFtPendingDone;
	std::list<ftPendingRequest> mPendingRequests;

	/* share incoming directory */
	bool mShareDownloadDir;
};

#endif
