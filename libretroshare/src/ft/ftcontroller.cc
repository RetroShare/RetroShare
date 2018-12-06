/*******************************************************************************
 * libretroshare/src/ft: ftcontroller.cc                                       *
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

#ifdef WINDOWS_SYS
#include "util/rsstring.h"
#endif
#include "util/rsdiscspace.h"
#include "util/rsmemory.h"
#include "util/rstime.h"

#include "ft/ftcontroller.h"

#include "ft/ftfilecreator.h"
#include "ft/fttransfermodule.h"
#include "ft/ftsearch.h"
#include "ft/ftdatamultiplex.h"
#include "ft/ftextralist.h"
#include "ft/ftserver.h"

#include "turtle/p3turtle.h"

#include "util/rsdir.h"
#include "rsserver/p3face.h"

#include "pqi/p3linkmgr.h"

#include "retroshare/rsiface.h"
#include "retroshare/rspeers.h"

#include "rsitems/rsconfigitems.h"
#include <stdio.h>
#include <unistd.h>		/* for (u)sleep() */
#include "util/rstime.h"

/******
 * #define CONTROL_DEBUG 1
 * #define DEBUG_DWLQUEUE 1
 *****/

static const int32_t SAVE_TRANSFERS_DELAY 			= 301 ; // save transfer progress every 301 seconds.
static const int32_t INACTIVE_CHUNKS_CHECK_DELAY 	= 240 ; // time after which an inactive chunk is released
static const int32_t MAX_TIME_INACTIVE_REQUEUED 	= 120 ; // time after which an inactive ftFileControl is bt-queued

static const int32_t FT_FILECONTROL_QUEUE_ADD_END 			= 0 ;
static const int32_t FT_FILECONTROL_MAX_UPLOAD_SLOTS_DEFAULT= 0 ;

const uint32_t FT_CNTRL_STANDARD_RATE = 10 * 1024 * 1024;
const uint32_t FT_CNTRL_SLOW_RATE     = 100   * 1024;

ftFileControl::ftFileControl()
	:mTransfer(NULL), mCreator(NULL),
	 mState(DOWNLOADING), mSize(0), mFlags(0), mCreateTime(0), mQueuePriority(0), mQueuePosition(0)
{
	return;
}

ftFileControl::ftFileControl(const std::string& fname, const std::string& tmppath, const std::string& dest
                           , uint64_t size, const RsFileHash &hash, TransferRequestFlags flags
                           , ftFileCreator *fc, ftTransferModule *tm)
  : mName(fname), mCurrentPath(tmppath), mDestination(dest)
  , mTransfer(tm), mCreator(fc), mState(DOWNLOADING), mHash(hash)
  , mSize(size), mFlags(flags), mCreateTime(0), mQueuePriority(0), mQueuePosition(0)
{}

ftController::ftController(ftDataMultiplex *dm, p3ServiceControl *sc, uint32_t ftServiceId)
  : p3Config(),
    last_save_time(0),
    last_clean_time(0),
    mSearch(NULL),
    mDataplex(dm),
    mExtraList(NULL),
    mTurtle(NULL),
    mFtServer(NULL),
    mServiceCtrl(sc),
    mFtServiceType(ftServiceId),
    mDefaultEncryptionPolicy(RS_FILE_CTRL_ENCRYPTION_POLICY_PERMISSIVE),
    mFilePermDirectDLPolicy(RS_FILE_PERM_DIRECT_DL_PER_USER),
    cnt(0),
    ctrlMutex("ftController"),
    doneMutex("ftController"),
    mFtActive(false),
    mFtPendingDone(false),
    mDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE),
    _max_active_downloads(5), // default queue size
    _max_uploads_per_friend(FT_FILECONTROL_MAX_UPLOAD_SLOTS_DEFAULT)
{
}

void ftController::setTurtleRouter(p3turtle *pt) { mTurtle = pt ; }
void ftController::setFtServer(ftServer *ft) { mFtServer = ft ; }

void ftController::setFtSearchNExtra(ftSearch *search, ftExtraList *list)
{
	mSearch = search;
	mExtraList = list;
}

bool ftController::getFileDownloadChunksDetails(const RsFileHash& hash,FileChunksInfo& info)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

    std::map<RsFileHash, ftFileControl*>::iterator it = mDownloads.find(hash) ;

	if(it != mDownloads.end())
	{
		it->second->mCreator->getChunkMap(info) ;
		//info.flags = it->second->mFlags ;

		return true ;
	}

	it = mCompleted.find(hash) ;

	if(it != mCompleted.end())
	{
		// This should rather be done as a static method of ChunkMap.
		//
		// We do this manually, because the file creator has already been destroyed.
		//
		info.file_size = it->second->mSize ;
		info.strategy = mDefaultChunkStrategy ;
		info.chunk_size = ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE ;
		//info.flags = it->second->mFlags ;
		uint32_t nb_chunks = it->second->mSize/ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE ;
		if(it->second->mSize % ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE != 0)
			++nb_chunks ;
		info.chunks.resize(nb_chunks,FileChunksInfo::CHUNK_DONE) ;

		return true ;
	}

	return false ;
}

void ftController::addFileSource(const RsFileHash& hash,const RsPeerId& peer_id)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

    std::map<RsFileHash, ftFileControl*>::iterator it = mDownloads.find(hash);

#ifdef CONTROL_DEBUG
	std::cerr << "ftController: Adding source " << peer_id << " to current download hash=" << hash ;
#endif
	if(it != mDownloads.end())
	{
		it->second->mTransfer->addFileSource(peer_id);
		setPeerState(it->second->mTransfer, peer_id, FT_CNTRL_STANDARD_RATE, mServiceCtrl->isPeerConnected(mFtServiceType, peer_id ));

#ifdef CONTROL_DEBUG
		std::cerr << "... added." << std::endl ;
#endif
		return ;
	}
#ifdef CONTROL_DEBUG
	std::cerr << "... not added: hash not found." << std::endl ;
#endif
}
void ftController::removeFileSource(const RsFileHash& hash,const RsPeerId& peer_id)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

    std::map<RsFileHash, ftFileControl*>::iterator it = mDownloads.find(hash);

#ifdef CONTROL_DEBUG
	std::cerr << "ftController: Adding source " << peer_id << " to current download hash=" << hash ;
#endif
	if(it != mDownloads.end())
	{
		it->second->mTransfer->removeFileSource(peer_id);
		it->second->mCreator->removeFileSource(peer_id);

#ifdef CONTROL_DEBUG
		std::cerr << "... added." << std::endl ;
#endif
		return ;
	}
#ifdef CONTROL_DEBUG
	std::cerr << "... not added: hash not found." << std::endl ;
#endif
}
void ftController::data_tick()
{
	/* check the queues */

		//Waiting 1 sec before start
		rstime::rs_usleep(1*1000*1000); // 1 sec

#ifdef CONTROL_DEBUG
		//std::cerr << "ftController::run()";
		//std::cerr << std::endl;
#endif
		bool doPending = false;
		{
		  	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
			doPending = (mFtActive) && (!mFtPendingDone);
		}

		rstime_t now = time(NULL) ;
		if(now > last_save_time + SAVE_TRANSFERS_DELAY)
		{
			IndicateConfigChanged() ;
			last_save_time = now ;
		}

		if(now > last_clean_time + INACTIVE_CHUNKS_CHECK_DELAY)
		{
			searchForDirectSources() ;

			RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

			for(std::map<RsFileHash,ftFileControl*>::iterator it(mDownloads.begin());it!=mDownloads.end();++it)
				it->second->mCreator->removeInactiveChunks() ;

			last_clean_time = now ;
		}

		if (doPending)
		{
			if (!handleAPendingRequest())
			{
		  		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
				mFtPendingDone = true;
			}
		}

		tickTransfers() ;

		{
            std::list<RsFileHash> files_to_complete ;

			{
				RsStackMutex stack2(doneMutex);
				files_to_complete = mDone ;
				mDone.clear();
			}

            for(std::list<RsFileHash>::iterator it(files_to_complete.begin()); it != files_to_complete.end(); ++it)
				completeFile(*it);
		}

		if(cnt++ % 10 == 0)
			checkDownloadQueue() ;
}

void ftController::searchForDirectSources()
{
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::searchForDirectSources()" << std::endl;
#endif
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	if (!mSearch)
	{
#ifdef CONTROL_DEBUG
		std::cerr << "  search module not available!" << std::endl;
#endif
		return;
	}

	for(std::map<RsFileHash,ftFileControl*>::iterator it(mDownloads.begin()); it != mDownloads.end(); ++it )
		if(it->second->mState != ftFileControl::QUEUED && it->second->mState != ftFileControl::PAUSED )
		{
#ifdef CONTROL_DEBUG
			std::cerr << "  file " << it->first << ":" << std::endl;
#endif
			FileInfo info ;	// Info needs to be re-allocated each time, to start with a clear list of peers (it's not cleared down there)

			if( mSearch->search(it->first, RS_FILE_HINTS_REMOTE | RS_FILE_HINTS_SPEC_ONLY, info) )
				for( std::vector<TransferInfo>::const_iterator pit = info.peers.begin(); pit != info.peers.end(); ++pit )
				{
					bool bAllowDirectDL = false;
                    switch (mFilePermDirectDLPolicy) {
						case RS_FILE_PERM_DIRECT_DL_YES: bAllowDirectDL = true; break;
						case RS_FILE_PERM_DIRECT_DL_NO: bAllowDirectDL = false; break;
						default:bAllowDirectDL = (rsPeers->servicePermissionFlags(pit->peerId) & RS_NODE_PERM_DIRECT_DL); break;
					}
					if( bAllowDirectDL )
						if( it->second->mTransfer->addFileSource(pit->peerId) ) /* if the sources don't exist already - add in */
							setPeerState( it->second->mTransfer, pit->peerId, FT_CNTRL_STANDARD_RATE, mServiceCtrl->isPeerConnected(mFtServiceType, pit->peerId) );
#ifdef CONTROL_DEBUG
					std::cerr << "    found source " << pit->peerId << ", allowDirectDL=" << bAllowDirectDL << ". " << (bAllowDirectDL?"adding":"not adding") << std::endl;
#endif
				}
#ifdef CONTROL_DEBUG
			else
				std::cerr << "    search returned empty.: " << std::endl;
#endif
		}
#ifdef CONTROL_DEBUG
		else
			std::cerr << "  file " << it->first << ": state is " << it->second->mState << ": ignored." << std::endl;
#endif
}

void ftController::tickTransfers()
{
	// 1 - sort modules into arrays according to priority
	
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

#ifdef CONTROL_DEBUG
	//	std::cerr << "ticking transfers." << std::endl ;
#endif
	// Collect all non queued files.
	//
    for(std::map<RsFileHash,ftFileControl*>::iterator it(mDownloads.begin()); it != mDownloads.end(); ++it)
		if(it->second->mState != ftFileControl::QUEUED && it->second->mState != ftFileControl::PAUSED)
			it->second->mTransfer->tick() ;
}

bool ftController::getPriority(const RsFileHash& hash,DwlSpeed& p)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

    std::map<RsFileHash,ftFileControl*>::iterator it(mDownloads.find(hash)) ;

	if(it != mDownloads.end())
	{
		p = it->second->mTransfer->downloadPriority() ;
		return true ;
	}
	else
		return false ;
}

void ftController::setPriority(const RsFileHash& hash,DwlSpeed p)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

    std::map<RsFileHash,ftFileControl*>::iterator it(mDownloads.find(hash)) ;

	if(it != mDownloads.end())
		it->second->mTransfer->setDownloadPriority(p) ;
}

/* Called every 10 seconds or so */
void ftController::checkDownloadQueue()
{
	// We do multiple things here:
	//
    // 0 - make sure all transfers have a consistent value for mDownloadQueue
    //
	// 1 - are there queued files ?
    //
	// 	YES
	// 		1.1 - check for inactive files (see below).
	// 				- select one inactive file
	// 					- close it temporarily
	// 					- remove it from turtle handling
	// 					- move the ftFileControl to queued list
	// 					- set the queue priority to 1+largest in the queue.
	// 		1.2 - pop from the queue the 1st file to come (according to availability of sources, then priority)
	// 				- enable turtle router handling for this hash
	// 				- reset counters
	// 				- set the file as waiting
	// 	NO
	// 		Do nothing, because no files are waiting.
	//
	// 2 - in FileRequest, if the max number of downloaded files is exceeded, put the 
	// 	file in the queue list, and don't enable the turtle router handling.
	//
	// Implementation: 
	// 	- locate a hash in the queue
	// 	- move hashes in the queue up/down/top/bottom
	// 	- now the place of each ftFileControl element in the queue to activate/desactive them
	//
	// So:
	// 	- change mDownloads to be a std::map<hash,ftFileControl*>
	// 	- sort the ftFileControl* into a std::vector
	// 	- store the queue position of each ftFileControl* into its own structure (mQueuePosition member)
	//
	// We don't want the turtle router to keep openning tunnels for queued files, so we only base
	// the notion of inactive on the fact that no traffic happens for the file within 5 mins.
	//
	// How do we detect inactive files?
	// 	For a downld file: - no traffic for 5 min.

	RsStackMutex mtx(ctrlMutex) ;

#ifdef DEBUG_DWLQUEUE
	std::cerr << "Checking download queue." << std::endl ;
#endif
	if(mDownloads.size() <= _max_active_downloads)
		return ;

    std::vector<ftFileControl*> inactive_transfers ;
    std::vector<ftFileControl*> transfers_with_online_sources ;

    std::set<RsPeerId> online_peers ;
    mServiceCtrl->getPeersConnected(mFtServiceType,online_peers) ;

	// Check for inactive transfers, and queued transfers with online sources.
	//
	rstime_t now = time(NULL) ;

    for(std::map<RsFileHash,ftFileControl*>::const_iterator it(mDownloads.begin());it!=mDownloads.end() ;++it)
		if(	it->second->mState != ftFileControl::QUEUED  && (it->second->mState == ftFileControl::PAUSED
                    || now > it->second->mTransfer->lastActvTimeStamp() + (rstime_t)MAX_TIME_INACTIVE_REQUEUED))
        {
			inactive_transfers.push_back(it->second) ;
        }
		else if(it->second->mState == ftFileControl::QUEUED)
        {
            std::list<RsPeerId> srcs ;
			it->second->mTransfer->getFileSources(srcs) ;

            for(std::list<RsPeerId>::const_iterator it2(srcs.begin());it2!=srcs.end();++it2)
                if(online_peers.find(*it2) != online_peers.end())
                {
                    transfers_with_online_sources.push_back(it->second) ;
                    break ;
                }
        }

#ifdef DEBUG_DWLQUEUE
    std::cerr << "Identified " << inactive_transfers.size() << " inactive transfer, and " << transfers_with_online_sources.size() << " queued transfers with online sources." << std::endl;
#endif

    // first swap as many queued transfers with online sources with inactive transfers
    uint32_t i=0;

    for(;i<inactive_transfers.size() && i<transfers_with_online_sources.size();++i)
    {
#ifdef DEBUG_DWLQUEUE
        std::cerr << "  Exchanging queue position of inactive transfer " << inactive_transfers[i]->mName << " at position " << inactive_transfers[i]->mQueuePosition << " with transfer at position " << transfers_with_online_sources[i]->mQueuePosition << " which has available sources." << std::endl;
#endif
		inactive_transfers[i]->mTransfer->resetActvTimeStamp() ;	// very important!
		transfers_with_online_sources[i]->mTransfer->resetActvTimeStamp() ;	// very important!

        locked_swapQueue(inactive_transfers[i]->mQueuePosition,transfers_with_online_sources[i]->mQueuePosition);
    }

    // now if some inactive transfers remain, put them at the end of the queue.

    for(;i<inactive_transfers.size();++i)
	{
#ifdef DEBUG_DWLQUEUE
		std::cerr << "  - Inactive file " << inactive_transfers[i]->mName << " at position " << inactive_transfers[i]->mQueuePosition << " moved to end of the queue. mState=" << inactive_transfers[i]->mState << ", time lapse=" << now - it->second->mCreator->lastActvTimeStamp()  << std::endl ;
#endif
		locked_bottomQueue(inactive_transfers[i]->mQueuePosition) ;
#ifdef DEBUG_DWLQUEUE
		std::cerr << "  new position: " << inactive_transfers[i]->mQueuePosition << std::endl ;
		std::cerr << "  new state: " << inactive_transfers[i]->mState << std::endl ;
#endif
		inactive_transfers[i]->mTransfer->resetActvTimeStamp() ;	// very important!
	}

    // finally, do a full swab over the queue to make sure that the expected number of downloads is met.

    for(uint32_t i=0;i<mDownloadQueue.size();++i)
        locked_checkQueueElement(i) ;
}

void ftController::locked_addToQueue(ftFileControl* ftfc,int add_strategy)
{
#ifdef DEBUG_DWLQUEUE
	std::cerr << "Queueing ftfileControl " << (void*)ftfc << ", name=" << ftfc->mName << std::endl ;
#endif

	switch(add_strategy)
	{
    default:
		// Different strategies for files and cache files:
		// 	- a min number of slots is reserved to user file transfer
		// 	- cache files are always added after this slot.
		//
		case FT_FILECONTROL_QUEUE_ADD_END:			 mDownloadQueue.push_back(ftfc) ;
																 locked_checkQueueElement(mDownloadQueue.size()-1) ;
																 break ;
	}
}

void ftController::locked_queueRemove(uint32_t pos)
{
	for(uint32_t p=pos;p<mDownloadQueue.size()-1;++p)
	{
		mDownloadQueue[p]=mDownloadQueue[p+1] ;
		locked_checkQueueElement(p) ;
	}
	mDownloadQueue.pop_back();
}

void ftController::setQueueSize(uint32_t s)
{
	RsStackMutex mtx(ctrlMutex) ;

	if(s > 0)
	{
		uint32_t old_s = _max_active_downloads ;
		_max_active_downloads = s ;
#ifdef DEBUG_DWLQUEUE
		std::cerr << "Settign new queue size to " << s << std::endl ;
#endif
		for(uint32_t p=std::min(s,old_s);p<=std::max(s,old_s);++p)
			if(p < mDownloadQueue.size())
				locked_checkQueueElement(p);
	}
	else
		std::cerr << "ftController::setQueueSize(): cannot set queue to size " << s << std::endl ;
}
uint32_t ftController::getQueueSize()
{
	RsStackMutex mtx(ctrlMutex) ;
	return _max_active_downloads ;
}

void ftController::moveInQueue(const RsFileHash& hash,QueueMove mv)
{
	RsStackMutex mtx(ctrlMutex) ;

    std::map<RsFileHash,ftFileControl*>::iterator it(mDownloads.find(hash)) ;

	if(it == mDownloads.end())
	{
		std::cerr << "ftController::moveInQueue: can't find hash " << hash << " in the download list." << std::endl ;
		return ;
	}
	uint32_t pos = it->second->mQueuePosition ;

#ifdef DEBUG_DWLQUEUE
	std::cerr << "Moving file " << hash << ", pos=" << pos << " to new pos." << std::endl ;
#endif
	switch(mv)
	{
		case QUEUE_TOP:		locked_topQueue(pos) ;
									break ;

		case QUEUE_BOTTOM:	locked_bottomQueue(pos) ;
									break ;

		case QUEUE_UP:			if(pos > 0)
										locked_swapQueue(pos,pos-1) ;
									break ;

		case QUEUE_DOWN: 		if(pos < mDownloadQueue.size()-1)
										locked_swapQueue(pos,pos+1) ;
									break ;
		default:
									std::cerr << "ftController::moveInQueue: unknown move " << mv << std::endl ;
	}
}

void ftController::locked_topQueue(uint32_t pos)
{
	ftFileControl *tmp=mDownloadQueue[pos] ;

	for(int p=pos;p>0;--p)
	{
		mDownloadQueue[p]=mDownloadQueue[p-1] ;
		locked_checkQueueElement(p) ;
	}
	mDownloadQueue[0]=tmp ;

	locked_checkQueueElement(0) ;
}
void ftController::locked_bottomQueue(uint32_t pos)
{
	ftFileControl *tmp=mDownloadQueue[pos] ;

	for(uint32_t p=pos;p<mDownloadQueue.size()-1;++p)
	{
		mDownloadQueue[p]=mDownloadQueue[p+1] ;
		locked_checkQueueElement(p) ;
	}
	mDownloadQueue[mDownloadQueue.size()-1]=tmp ;
	locked_checkQueueElement(mDownloadQueue.size()-1) ;
}
void ftController::locked_swapQueue(uint32_t pos1,uint32_t pos2)
{
	// Swap the element at position pos with the last element of the queue

	if(pos1==pos2)
		return ;

	ftFileControl *tmp = mDownloadQueue[pos1] ;
	mDownloadQueue[pos1] = mDownloadQueue[pos2] ;
	mDownloadQueue[pos2] = tmp;

	locked_checkQueueElement(pos1) ;
	locked_checkQueueElement(pos2) ;
}

void ftController::locked_checkQueueElement(uint32_t pos)
{
	mDownloadQueue[pos]->mQueuePosition = pos ;

	if(pos < _max_active_downloads && mDownloadQueue[pos]->mState != ftFileControl::PAUSED)
	{
		if(mDownloadQueue[pos]->mState == ftFileControl::QUEUED)
			mDownloadQueue[pos]->mTransfer->resetActvTimeStamp() ;

		mDownloadQueue[pos]->mState = ftFileControl::DOWNLOADING ;

		if(mDownloadQueue[pos]->mFlags & RS_FILE_REQ_ANONYMOUS_ROUTING)
            mFtServer->activateTunnels(mDownloadQueue[pos]->mHash,mDefaultEncryptionPolicy,mDownloadQueue[pos]->mFlags,true);
	}

	if(pos >= _max_active_downloads && mDownloadQueue[pos]->mState != ftFileControl::QUEUED && mDownloadQueue[pos]->mState != ftFileControl::PAUSED)
	{
		mDownloadQueue[pos]->mState = ftFileControl::QUEUED ;
		mDownloadQueue[pos]->mCreator->closeFile() ;

		if(mDownloadQueue[pos]->mFlags & RS_FILE_REQ_ANONYMOUS_ROUTING)
            mFtServer->activateTunnels(mDownloadQueue[pos]->mHash,mDefaultEncryptionPolicy,mDownloadQueue[pos]->mFlags,false);
    }
}

bool ftController::FlagFileComplete(const RsFileHash& hash)
{
	RsStackMutex stack2(doneMutex);
	mDone.push_back(hash);

#ifdef CONTROL_DEBUG
        std::cerr << "ftController:FlagFileComplete(" << hash << ")";
        std::cerr << std::endl;
#endif

	return true;
}

bool ftController::completeFile(const RsFileHash& hash)
{
	/* variables... so we can drop mutex later */
	std::string path;
	std::string name;
	uint64_t    size = 0;
    uint32_t    period = 0;
	TransferRequestFlags flags ;
	TransferRequestFlags extraflags ;
	uint32_t    completeCount = 0;

	{
		RS_STACK_MUTEX(ctrlMutex);

#ifdef CONTROL_DEBUG
		std::cerr << "ftController:completeFile(" << hash << ")";
		std::cerr << std::endl;
#endif
        std::map<RsFileHash, ftFileControl*>::iterator it(mDownloads.find(hash));

		if (it == mDownloads.end())
		{
#ifdef CONTROL_DEBUG
			std::cerr << "ftController:completeFile(" << hash << ")";
			std::cerr << " Not Found!";
			std::cerr << std::endl;
#endif
			return false;
		}

		/* check if finished */
		if (!(it->second)->mCreator->finished())
		{
			/* not done! */
#ifdef CONTROL_DEBUG
			std::cerr << "ftController:completeFile(" << hash << ")";
			std::cerr << " Transfer Not Done";
			std::cerr << std::endl;

			std::cerr << "FileSize: ";
			std::cerr << (it->second)->mCreator->getFileSize();
			std::cerr << " and Recvd: ";
			std::cerr << (it->second)->mCreator->getRecvd();
#endif
			return false;
		}


		ftFileControl *fc = it->second;

		// (csoler) I've postponed this to the end of the block because deleting the
		// element from the map calls the destructor of fc->mTransfer, which
		// makes fc to point to nothing and causes random behavior/crashes.
		//
		// mDataplex->removeTransferModule(fc->mTransfer->hash());
		//
		/* done - cleanup */

		// (csoler) I'm copying this because "delete fc->mTransfer" deletes the hash string!
        RsFileHash hash_to_suppress(fc->mTransfer->hash());

		// This should be done that early, because once the file creator is
		// deleted, it should not be accessed by the data multiplex anymore!
		//
		mDataplex->removeTransferModule(hash_to_suppress) ;

		if (fc->mTransfer)
		{
			delete fc->mTransfer;
			fc->mTransfer = NULL;
		}

		if (fc->mCreator)
		{
			fc->mCreator->closeFile() ;
			delete fc->mCreator;
			fc->mCreator = NULL;
		}

		fc->mState = ftFileControl::COMPLETED;

		std::string dst_dir,src_dir,src_file,dst_file ;

		RsDirUtil::splitDirFromFile(fc->mCurrentPath,src_dir,src_file) ;
		RsDirUtil::splitDirFromFile(fc->mDestination,dst_dir,dst_file) ;

		// We use this intermediate file in case the destination directory is not available, so as to not keep the partial file name.

		std::string intermediate_file_name = src_dir+'/'+dst_file ;

		// I don't know how the size can be zero, but believe me, this happens,
		// and it causes an error on linux because then the file may not even exist.
		//
		if( fc->mSize == 0)
			fc->mState = ftFileControl::ERROR_COMPLETION;
		else
		{
			std::cerr << "CompleteFile(): renaming " << fc->mCurrentPath << " into " << fc->mDestination << std::endl;
			std::cerr << "CompleteFile(): 1 - renaming " << fc->mCurrentPath << " info " << intermediate_file_name << std::endl;

			if(RsDirUtil::moveFile(fc->mCurrentPath,intermediate_file_name) )
			{
				fc->mCurrentPath = intermediate_file_name ;

				std::cerr << "CompleteFile(): 2 - renaming/copying " << intermediate_file_name << " into " << fc->mDestination << std::endl;

				if(RsDirUtil::moveFile(intermediate_file_name,fc->mDestination) )
					fc->mCurrentPath = fc->mDestination;
				else
					fc->mState = ftFileControl::ERROR_COMPLETION;
			}
			else
				fc->mState = ftFileControl::ERROR_COMPLETION;
		}

		/* for extralist additions */
		path    = fc->mDestination;
		name    = fc->mName;
		//hash    = fc->mHash;
		size    = fc->mSize;
		period  = 30 * 24 * 3600; /* 30 days */
		extraflags.clear() ;

#ifdef CONTROL_DEBUG
		std::cerr << "CompleteFile(): size = " << size << std::endl ;
#endif

		flags = fc->mFlags ;

		locked_queueRemove(it->second->mQueuePosition) ;

		/* switch map */
        mCompleted[fc->mHash] = fc;
        completeCount = mCompleted.size();

		mDownloads.erase(it);

		if(flags & RS_FILE_REQ_ANONYMOUS_ROUTING)
            mFtServer->activateTunnels(hash_to_suppress,mDefaultEncryptionPolicy,flags,false);

	} // UNLOCK: RS_STACK_MUTEX(ctrlMutex);


	/******************** NO Mutex from Now ********************
	 * cos Callback can end up back in this class.
	 ***********************************************************/

	/* If it has a callback - do it now */

    if(flags & RS_FILE_REQ_EXTRA)// | RS_FILE_HINTS_MEDIA))
	{
#ifdef CONTROL_DEBUG
		  std::cerr << "ftController::completeFile() adding to ExtraList";
		  std::cerr << std::endl;
#endif

		if(mExtraList)
			mExtraList->addExtraFile(path, hash, size, period, extraflags);
	}
	else
	{
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::completeFile() No callback";
		std::cerr << std::endl;
#endif
	}

	/* Notify GUI */
    RsServer::notify()->AddPopupMessage(RS_POPUP_DOWNLOAD, hash.toStdString(), name, "");

    RsServer::notify()->notifyDownloadComplete(hash.toStdString());
    RsServer::notify()->notifyDownloadCompleteCount(completeCount);

    rsFiles->ForceDirectoryCheck() ;

	IndicateConfigChanged(); /* completed transfer -> save */
	return true;
}

	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

bool	ftController::activate()
{
  	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	mFtActive = true;
	mFtPendingDone = false;
	return true;
}

bool 	ftController::isActiveAndNoPending()
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	return (mFtActive && mFtPendingDone);
}

bool	ftController::handleAPendingRequest()
{
	uint32_t nb_requests_handled = 0 ;
	static const uint32_t MAX_SIMULTANEOUS_PENDING_REQUESTS = 100 ;

	while(!mPendingRequests.empty() && nb_requests_handled++ < MAX_SIMULTANEOUS_PENDING_REQUESTS)
	{
		ftPendingRequest req;
		{
			RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

			req = mPendingRequests.front();
			mPendingRequests.pop_front();
		}
#ifdef CONTROL_DEBUG
		std::cerr << "Requesting pending hash " << req.mHash << std::endl ;
#endif

		FileRequest(req.mName, req.mHash, req.mSize, req.mDest, TransferRequestFlags(req.mFlags), req.mSrcIds, req.mState);

		{
			// See whether there is a pendign chunk map recorded for this hash.
			//
			RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

			std::map<RsFileHash,RsFileTransfer*>::iterator it(mPendingChunkMaps.find(req.mHash)) ;

			if(it != mPendingChunkMaps.end())
			{
				RsFileTransfer *rsft = it->second ;
				std::map<RsFileHash, ftFileControl*>::iterator fit = mDownloads.find(rsft->file.hash);

				if((fit==mDownloads.end() || (fit->second)->mCreator == NULL))
				{
					// This should never happen, because the last call to FileRequest must have created the fileCreator!!
					//
					std::cerr << "ftController::loadList(): Error: could not find hash " << rsft->file.hash << " in mDownloads list !" << std::endl ;
				}
				else
				{
#ifdef CONTROL_DEBUG
					std::cerr << "Hash " << req.mHash << " is in downloads" << std::endl ;
					std::cerr << "  setting chunk strategy to " << rsft->chunk_strategy << std::endl;
#endif
					(fit->second)->mCreator->setAvailabilityMap(rsft->compressed_chunk_map) ;
					(fit->second)->mCreator->setChunkStrategy((FileChunksInfo::ChunkStrategy)(rsft->chunk_strategy)) ;
					(fit->second)->mState=rsft->state;
				}

				delete rsft ;
				mPendingChunkMaps.erase(it) ;
			}
#ifdef CONTROL_DEBUG
			else
				std::cerr << "No pending chunkmap for hash " << req.mHash << std::endl ;
#endif
		}
	}
	return !mPendingRequests.empty();
}

bool ftController::alreadyHaveFile(const RsFileHash& hash, FileInfo &info)
{
	// check for downloads
	if (FileDetails(hash, info) && (info.downloadStatus == FT_STATE_COMPLETE))
		return true ;

	// check for file lists
	if (!mSearch) return false;
	if (mSearch->search(hash, RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_SPEC_ONLY, info))
		return true ;
	
	return false ;
}

bool 	ftController::FileRequest(const std::string& fname, const RsFileHash& hash,
			uint64_t size, const std::string& dest, TransferRequestFlags flags,
                                                                const std::list<RsPeerId> &_srcIds, uint16_t state)
{
    std::list<RsPeerId> srcIds(_srcIds) ;

	/* check if we have the file */

	FileInfo info;
	if(alreadyHaveFile(hash, info))
		return false ;

    // the strategy for requesting encryption is the following:
    //
    // if policy is STRICT
    //	- disable clear, enforce encryption
    // else
    //  - if not specified, use both
    //
    if(mDefaultEncryptionPolicy == RS_FILE_CTRL_ENCRYPTION_POLICY_STRICT)
    {
        flags |=  RS_FILE_REQ_ENCRYPTED ;
        flags &= ~RS_FILE_REQ_UNENCRYPTED ;
    }
    else if(!(flags & ( RS_FILE_REQ_ENCRYPTED |  RS_FILE_REQ_UNENCRYPTED )))
    {
        flags |= RS_FILE_REQ_ENCRYPTED ;
		flags |= RS_FILE_REQ_UNENCRYPTED ;
    }

	if(size == 0)	// we treat this special case because
	{
		/* if no destpath - send to download directory */
		std::string destination ;

		if (dest == "")
			destination = mDownloadPath + "/" + fname;
		else
			destination = dest + "/" + fname;

		// create void file with the target name.
		FILE *f = RsDirUtil::rs_fopen(destination.c_str(),"w") ;
		if(f == NULL)
			std::cerr << "Could not open file " << destination << " for writting." << std::endl ;
		else
			fclose(f) ;

		return true ;
	}

	// Bugs in the cache system can cause files with arbitrary size to be
	// requested, causing a division by zero in ftChunkMap when size > 1MB *
	// (2^32-1). I thus conservatively check for large size values.
	//
	if(size >= 1024ull*1024ull*((1ull << 32) - 1))
	{
		std::cerr << "FileRequest Error: unexpected size. This is probably a bug." << std::endl;
		std::cerr << "  name  = " << fname << std::endl ;
		std::cerr << "  flags = " << flags << std::endl ;
		std::cerr << "  dest  = " << dest << std::endl ;
		std::cerr << "  size  = " << size << std::endl ;
		return false ;
	}

	/* If file transfer is not enabled ....
	 * save request for later. This will also
	 * mean that we will have to copy local files,
	 * or have a callback which says: local file.
	 */

	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
		if (!mFtActive)
		{
			/* store in pending queue */
			ftPendingRequest req(fname, hash, size, dest, flags, srcIds,state);
			mPendingRequests.push_back(req);
			return true;
		}
	}

	// remove the sources from the list, if they don't have clearance for direct transfer. This happens only for non cache files.
	//
	for(std::list<RsPeerId>::iterator it = srcIds.begin(); it != srcIds.end(); )
	{
		bool bAllowDirectDL = false;
        switch (mFilePermDirectDLPolicy) {
			case RS_FILE_PERM_DIRECT_DL_YES: bAllowDirectDL = true; break;
			case RS_FILE_PERM_DIRECT_DL_NO: bAllowDirectDL = false; break;
			default:bAllowDirectDL = (rsPeers->servicePermissionFlags(*it) & RS_NODE_PERM_DIRECT_DL); break;
		}

		if(!bAllowDirectDL)
		{
			std::list<RsPeerId>::iterator tmp(it);
			++tmp;
			srcIds.erase(it);
			it = tmp;
		}
		else
			++it;
	}

	std::list<RsPeerId>::const_iterator it;
	std::vector<TransferInfo>::const_iterator pit;

#ifdef CONTROL_DEBUG
	std::cerr << "ftController::FileRequest(" << fname << ",";
	std::cerr << hash << "," << size << "," << dest << ",";
	std::cerr << flags << ",<";

	for(it = srcIds.begin(); it != srcIds.end(); ++it)
	{
		std::cerr << *it << ",";
	}
	std::cerr << ">)";
	std::cerr << std::endl;
#endif

	uint32_t rate = 0;
	if (flags & RS_FILE_REQ_BACKGROUND)
		rate = FT_CNTRL_SLOW_RATE;
	else
		rate = FT_CNTRL_STANDARD_RATE;

	/* First check if the file is already being downloaded....
	 * This is important as some guis request duplicate files regularly.
	 */

	{ 
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

        std::map<RsFileHash, ftFileControl*>::const_iterator dit = mDownloads.find(hash);

		if (dit != mDownloads.end())
		{
			/* we already have it! */

#ifdef CONTROL_DEBUG
			std::cerr << "ftController::FileRequest() Already Downloading File";
			std::cerr << std::endl;
			std::cerr << "\tNo need to download";
			std::cerr << std::endl;
#endif
			/* but we should add this peer - if they don't exist!
			 * (needed for channels).
			 */

			for(it = srcIds.begin(); it != srcIds.end(); ++it)
			{
				bool bAllowDirectDL = false;
                switch (mFilePermDirectDLPolicy) {
					case RS_FILE_PERM_DIRECT_DL_YES: bAllowDirectDL = true; break;
					case RS_FILE_PERM_DIRECT_DL_NO: bAllowDirectDL = false; break;
					default:bAllowDirectDL = (rsPeers->servicePermissionFlags(*it) & RS_NODE_PERM_DIRECT_DL); break;
				}
				if(bAllowDirectDL)
				{
					uint32_t i, j;
					if ((dit->second)->mTransfer->getPeerState(*it, i, j))
					{
#ifdef CONTROL_DEBUG
						std::cerr << "ftController::FileRequest() Peer Existing";
						std::cerr << std::endl;
#endif
						continue; /* already added peer */
					}

#ifdef CONTROL_DEBUG
					std::cerr << "ftController::FileRequest() Adding Peer: " << *it;
					std::cerr << std::endl;
#endif
					(dit->second)->mTransfer->addFileSource(*it);
					setPeerState(dit->second->mTransfer, *it, rate, mServiceCtrl->isPeerConnected(mFtServiceType, *it));
				}
			}

			if (srcIds.empty())
			{
#ifdef CONTROL_DEBUG
				std::cerr << "ftController::FileRequest() WARNING: No Src Peers";
				std::cerr << std::endl;
#endif
			}

			return true;
		}
	} /******* UNLOCKED ********/


	if(mSearch && !(flags & RS_FILE_REQ_NO_SEARCH))
	{
		/* do a source search - for any extra sources */
		// add sources only in direct mode
		//
		if(/* (flags & RS_FILE_HINTS_BROWSABLE) && */ mSearch->search(hash, RS_FILE_HINTS_REMOTE | RS_FILE_HINTS_SPEC_ONLY, info))
		{
			/* do something with results */
#ifdef CONTROL_DEBUG
			std::cerr << "ftController::FileRequest() Found Other Sources";
			std::cerr << std::endl;
#endif

			/* if the sources don't exist already - add in */
			for(pit = info.peers.begin(); pit != info.peers.end(); ++pit)
			{
#ifdef CONTROL_DEBUG
                                std::cerr << "\tSource: " << pit->peerId;
				std::cerr << std::endl;
#endif
				// Because this is auto-add, we only add sources that we allow to DL from using direct transfers.

				bool bAllowDirectDL = false;
                switch (mFilePermDirectDLPolicy) {
					case RS_FILE_PERM_DIRECT_DL_YES: bAllowDirectDL = true; break;
					case RS_FILE_PERM_DIRECT_DL_NO: bAllowDirectDL = false; break;
					default:bAllowDirectDL = (rsPeers->servicePermissionFlags(pit->peerId) & RS_NODE_PERM_DIRECT_DL); break;
				}

				if ((srcIds.end() == std::find( srcIds.begin(), srcIds.end(), pit->peerId)) && bAllowDirectDL)
				{
					srcIds.push_back(pit->peerId);

#ifdef CONTROL_DEBUG
					std::cerr << "\tAdding in: " << pit->peerId;
					std::cerr << std::endl;
#endif
				}
			}
		}
	}

	/* add in new item for download */
	std::string savepath;
	std::string destination;

	{ 
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

        savepath = mPartialsPath + "/" + hash.toStdString();
		destination = dest + "/" + fname;

		/* if no destpath - send to download directory */
		if (dest == "")
			destination = mDownloadPath + "/" + fname;
	} /******* UNLOCKED ********/

  // We check that flags are consistent.  
  
  	if(flags & RS_FILE_REQ_ANONYMOUS_ROUTING)
        mFtServer->activateTunnels(hash,mDefaultEncryptionPolicy,flags,true);

    bool assume_availability = false;

	ftFileCreator *fc = new ftFileCreator(savepath, size, hash,assume_availability);
	ftTransferModule *tm = new ftTransferModule(fc, mDataplex,this);

#ifdef CONTROL_DEBUG
	std::cerr << "Note: setting chunk strategy to " << mDefaultChunkStrategy <<std::endl ;
#endif
	fc->setChunkStrategy(mDefaultChunkStrategy) ;

	/* add into maps */
	ftFileControl *ftfc = new ftFileControl(fname, savepath, destination, size, hash, flags, fc, tm);
	ftfc->mCreateTime = time(NULL);

	/* now add source peers (and their current state) */
	tm->setFileSources(srcIds);

	/* get current state for transfer module */
	for(it = srcIds.begin(); it != srcIds.end(); ++it)
	{
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::FileRequest() adding peer: " << *it;
		std::cerr << std::endl;
#endif
		setPeerState(tm, *it, rate, mServiceCtrl->isPeerConnected(mFtServiceType, *it));
	}

	/* add structures into the accessible data. Needs to be locked */
	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
		locked_addToQueue(ftfc, FT_FILECONTROL_QUEUE_ADD_END ) ;

#ifdef CONTROL_DEBUG
		std::cerr << "ftController::FileRequest() Created ftFileCreator @: " << fc;
		std::cerr << std::endl;
		std::cerr << "ftController::FileRequest() Created ftTransModule @: " << tm;
		std::cerr << std::endl;
		std::cerr << "ftController::FileRequest() Created ftFileControl." ;
		std::cerr << std::endl;
#endif

		/* add to ClientModule */
		mDataplex->addTransferModule(tm, fc);
		mDownloads[hash] = ftfc;
	}

	IndicateConfigChanged(); /* completed transfer -> save */
	return true;
}

bool 	ftController::setPeerState(ftTransferModule *tm, const RsPeerId& id, uint32_t maxrate, bool online)
{
	if (id == mServiceCtrl->getOwnId())
	{
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::setPeerState() is Self";
		std::cerr << std::endl;
#endif
		tm->setPeerState(id, PQIPEER_IDLE, maxrate);
	}
	else if (online || mTurtle->isOnline(id))
	{
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::setPeerState()";
		std::cerr <<  " Peer is Online";
		std::cerr << std::endl;
#endif
		tm->setPeerState(id, PQIPEER_IDLE, maxrate);
	}
	else
	{
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::setPeerState()";
		std::cerr << " Peer is Offline";
		std::cerr << std::endl;
#endif
		tm->setPeerState(id, PQIPEER_NOT_ONLINE, maxrate);
	}
	return true;
}


bool ftController::setChunkStrategy(const RsFileHash& hash,FileChunksInfo::ChunkStrategy s)
{
    std::map<RsFileHash,ftFileControl*>::iterator mit=mDownloads.find(hash);
	if (mit==mDownloads.end())
	{
#ifdef CONTROL_DEBUG
		std::cerr<<"ftController::setChunkStrategy file is not found in mDownloads"<<std::endl;
#endif
		return false;
	}

	mit->second->mCreator->setChunkStrategy(s) ;
	return true ;
}

bool 	ftController::FileCancel(const RsFileHash& hash)
{
    mFtServer->activateTunnels(hash,mDefaultEncryptionPolicy,TransferRequestFlags(0),false);

#ifdef CONTROL_DEBUG
	std::cerr << "ftController::FileCancel" << std::endl;
#endif
	/*check if the file in the download map*/

	{
		RsStackMutex mtx(ctrlMutex) ;

        std::map<RsFileHash,ftFileControl*>::iterator mit=mDownloads.find(hash);
		if (mit==mDownloads.end())
		{
#ifdef CONTROL_DEBUG
			std::cerr<<"ftController::FileCancel file is not found in mDownloads"<<std::endl;
#endif
			return false;
		}

		/* check if finished */
		if ((mit->second)->mCreator->finished())
		{
#ifdef CONTROL_DEBUG
			std::cerr << "ftController:FileCancel(" << hash << ")";
			std::cerr << " Transfer Already finished";
			std::cerr << std::endl;

			std::cerr << "FileSize: ";
			std::cerr << (mit->second)->mCreator->getFileSize();
			std::cerr << " and Recvd: ";
			std::cerr << (mit->second)->mCreator->getRecvd();
#endif
			return false;
		}

		/*find the point to transfer module*/
		ftTransferModule* ft=(mit->second)->mTransfer;
		ft->cancelTransfer();

		ftFileControl *fc = mit->second;
		mDataplex->removeTransferModule(fc->mTransfer->hash());

		if (fc->mTransfer)
		{
			delete fc->mTransfer;
			fc->mTransfer = NULL;
		}

		if (fc->mCreator)
		{
			delete fc->mCreator;
			fc->mCreator = NULL;
		}

		/* delete the temporary file */
		if (0 == remove(fc->mCurrentPath.c_str()))
		{
#ifdef CONTROL_DEBUG
			std::cerr << "ftController::FileCancel() remove temporary file ";
			std::cerr << fc->mCurrentPath;
			std::cerr << std::endl;
#endif
		}
		else
		{
#ifdef CONTROL_DEBUG
			std::cerr << "ftController::FileCancel() fail to remove file ";
			std::cerr << fc->mCurrentPath;
			std::cerr << std::endl;
#endif
		}

		locked_queueRemove(fc->mQueuePosition) ;
		delete fc ;
		mDownloads.erase(mit);
	}

	IndicateConfigChanged(); /* completed transfer -> save */
	return true;
}

bool 	ftController::FileControl(const RsFileHash& hash, uint32_t flags)
{
#ifdef CONTROL_DEBUG
 std::cerr << "ftController::FileControl(" << hash << ",";
 std::cerr << flags << ")"<<std::endl;
#endif
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	/*check if the file in the download map*/
    std::map<RsFileHash,ftFileControl*>::iterator mit=mDownloads.find(hash);
	if (mit==mDownloads.end())
	{
#ifdef CONTROL_DEBUG
		std::cerr<<"ftController::FileControl file is not found in mDownloads"<<std::endl;
#endif
		return false;
	}

	/*find the point to transfer module*/
	switch (flags)
	{
		case RS_FILE_CTRL_PAUSE:
			mit->second->mState = ftFileControl::PAUSED ;
			std::cerr << "setting state to " << ftFileControl::PAUSED << std::endl ;
			break;

		case RS_FILE_CTRL_START:
			mit->second->mState = ftFileControl::DOWNLOADING ;
			std::cerr << "setting state to " << ftFileControl::DOWNLOADING << std::endl ;
			break;

		case RS_FILE_CTRL_FORCE_CHECK:
			mit->second->mTransfer->forceCheck();
			std::cerr << "setting state to " << ftFileControl::CHECKING_HASH << std::endl ;
			break;

		default:
			return false;
	}
	IndicateConfigChanged() ;

	return true;
}

bool ftController::FileClearCompleted()
{
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::FileClearCompleted()" <<std::endl;
#endif
	{
		RS_STACK_MUTEX(ctrlMutex);

		for(auto it(mCompleted.begin()); it != mCompleted.end(); ++it)
			delete it->second;

		mCompleted.clear();

		IndicateConfigChanged();
	}

	RsServer::notify()->notifyDownloadCompleteCount(0);

	return true;
}

	/* get Details of File Transfers */
void 	ftController::FileDownloads(std::list<RsFileHash> &hashs)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

    std::map<RsFileHash, ftFileControl*>::iterator it;
	for(it = mDownloads.begin(); it != mDownloads.end(); ++it)
	{
		hashs.push_back(it->second->mHash);
	}
	for(it = mCompleted.begin(); it != mCompleted.end(); ++it)
	{
		hashs.push_back(it->second->mHash);
	}
}


	/* Directory Handling */
bool 	ftController::setDownloadDirectory(std::string path)
{
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::setDownloadDirectory(" << path << ")";
	std::cerr << std::endl;
#endif
	/* check if it exists */
        if (RsDirUtil::checkCreateDirectory(path))
	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		mDownloadPath = RsDirUtil::convertPathToUnix(path);

		RsDiscSpace::setDownloadPath(mDownloadPath) ;
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::setDownloadDirectory() Okay!";
		std::cerr << std::endl;
#endif
        IndicateConfigChanged();
		return true;
	}

#ifdef CONTROL_DEBUG
	std::cerr << "ftController::setDownloadDirectory() Failed";
	std::cerr << std::endl;
#endif
	return false;
}

bool 	ftController::setPartialsDirectory(std::string path)
{

	/* check it is not a subdir of download / shared directories (BAD) - TODO */
	{
		RsStackMutex stack(ctrlMutex);

		path = RsDirUtil::convertPathToUnix(path);

		if (path.find(mDownloadPath) != std::string::npos) {
			return false;
		}

		if (rsFiles) {
			std::list<SharedDirInfo>::iterator it;
			std::list<SharedDirInfo> dirs;
			rsFiles->getSharedDirectories(dirs);
			for (it = dirs.begin(); it != dirs.end(); ++it) {
				if (path.find((*it).filename) != std::string::npos) {
					return false;
				}
			}
		}
	}

	/* check if it exists */

        if (RsDirUtil::checkCreateDirectory(path))
	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		mPartialsPath = path;

		RsDiscSpace::setPartialsPath(path) ;

#if 0 /*** FIX ME !!!**************/
		/* move all existing files! */
        std::map<RsFileHash, ftFileControl>::iterator it;
		for(it = mDownloads.begin(); it != mDownloads.end(); ++it)
		{
			(it->second).mCreator->changePartialDirectory(mPartialPath);
		}
#endif
        IndicateConfigChanged();
		return true;
	}

	return false;
}

std::string ftController::getDownloadDirectory()
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	return mDownloadPath;
}

std::string ftController::getPartialsDirectory()
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	return mPartialsPath;
}

bool ftController::setDestinationDirectory(const RsFileHash& hash,const std::string& dest_dir)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
    std::map<RsFileHash, ftFileControl*>::iterator it = mDownloads.find(hash);

	if (it == mDownloads.end())
		return false;

	std::cerr << "(II) Changing destination of file " << it->second->mDestination << std::endl;

	it->second->mDestination = dest_dir + '/' + it->second->mName ;

	std::cerr << "(II) ...to " << it->second->mDestination << std::endl;

	return true ;
}
bool ftController::setDestinationName(const RsFileHash& hash,const std::string& dest_name)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
    std::map<RsFileHash, ftFileControl*>::iterator it = mDownloads.find(hash);

	if (it == mDownloads.end())
		return false;

	std::cerr << "(II) Changing destination of file " << it->second->mDestination << std::endl;

	std::string dest_path ;
	RsDirUtil::removeTopDir(it->second->mDestination, dest_path); /* remove fname */

	it->second->mDestination = dest_path + '/' + dest_name ;
	it->second->mName = dest_name ;

	std::cerr << "(II) ...to " << it->second->mDestination << std::endl;

	return true ;
}

bool 	ftController::FileDetails(const RsFileHash &hash, FileInfo &info)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	bool completed = false;
    std::map<RsFileHash, ftFileControl*>::iterator it = mDownloads.find(hash);
	if (it == mDownloads.end())
	{
		/* search completed files too */
		it = mCompleted.find(hash);
		if (it == mCompleted.end())
		{
			/* Note: mTransfer & mCreator
			 * are both NULL
			 */
			return false;
		}
		completed = true;
	}

	/* extract details */
	info.hash = hash;
	info.fname = it->second->mName;
	info.storage_permission_flags.clear() ;
	info.transfer_info_flags = it->second->mFlags ;
	info.priority = SPEED_NORMAL ;
	RsDirUtil::removeTopDir(it->second->mDestination, info.path); /* remove fname */
	info.queue_position = it->second->mQueuePosition ;

	if(it->second->mFlags & RS_FILE_REQ_ANONYMOUS_ROUTING)
        info.storage_permission_flags |= DIR_FLAGS_ANONYMOUS_DOWNLOAD ;	// file being downloaded anonymously are always anonymously available.

	/* get list of sources from transferModule */
    std::list<RsPeerId> peerIds;
    std::list<RsPeerId>::iterator pit;

	if (!completed)
	{
		it->second->mTransfer->getFileSources(peerIds);
		info.priority = it->second->mTransfer->downloadPriority() ;
		info.lastTS = it->second->mCreator->lastRecvTimeStamp();	// last time the file was actually written
	}
	else
		info.lastTS = 0; 

	double totalRate = 0;
	uint32_t tfRate = 0;
	uint32_t state = 0;

	bool isDownloading = false;
	bool isSuspended = false;

	info.peers.clear();

	for(pit = peerIds.begin(); pit != peerIds.end(); ++pit)
	{
		if (it->second->mTransfer->getPeerState(*pit, state, tfRate))
		{
			TransferInfo ti;
			switch(state)
			{
			  case PQIPEER_INIT:
				ti.status = FT_STATE_OKAY;
				break;
			  case PQIPEER_NOT_ONLINE:
				ti.status = FT_STATE_WAITING;
				break;
			  case PQIPEER_DOWNLOADING:
				isDownloading = true;
				ti.status = FT_STATE_DOWNLOADING;
				break;
			  case PQIPEER_IDLE:
				ti.status = FT_STATE_OKAY;
				break;
			  default:
			  case PQIPEER_SUSPEND:
				isSuspended = true;
				ti.status = FT_STATE_FAILED;
				break;
			}

			ti.tfRate = tfRate / 1024.0;
			ti.peerId = *pit;
			info.peers.push_back(ti);
			totalRate += tfRate / 1024.0;
		}
	}

	if ((completed) || ((it->second)->mCreator->finished()))
	{
		info.downloadStatus = FT_STATE_COMPLETE;
	}
	else if (isDownloading)
	{
		info.downloadStatus = FT_STATE_DOWNLOADING;
	}
	else if (isSuspended)
	{
		info.downloadStatus = FT_STATE_FAILED;
	}
	else
	{
		info.downloadStatus = FT_STATE_WAITING;
	}

	if(it->second->mState == ftFileControl::QUEUED)
		info.downloadStatus = FT_STATE_QUEUED ;

	if(it->second->mState == ftFileControl::PAUSED)
		info.downloadStatus = FT_STATE_PAUSED ;

	if((!completed) && it->second->mTransfer->isCheckingHash())
		info.downloadStatus = FT_STATE_CHECKING_HASH ;

	info.tfRate = totalRate;
	info.size = (it->second)->mSize;

	if (completed)
	{
		info.transfered  = info.size;
		info.avail = info.transfered;
	}
	else
	{
		info.transfered  = (it->second)->mCreator->getRecvd();
		info.avail = info.transfered;
	}

	return true;



}


	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

	/* pqiMonitor callback:
	 * Used to tell TransferModules new available peers
	 */
void    ftController::statusChange(const std::list<pqiServicePeer> &plist)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	uint32_t rate = FT_CNTRL_STANDARD_RATE;

	/* add online to all downloads */
    std::map<RsFileHash, ftFileControl*>::iterator it;
	std::list<pqiServicePeer>::const_iterator pit;

#ifdef CONTROL_DEBUG
	std::cerr << "ftController::statusChange()";
	std::cerr << std::endl;
#endif

	for(it = mDownloads.begin(); it != mDownloads.end(); ++it)
	{
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::statusChange() Updating Hash:";
		std::cerr << it->first;
		std::cerr << std::endl;
#endif
		for(pit = plist.begin(); pit != plist.end(); ++pit)
		{
#ifdef CONTROL_DEBUG
			std::cerr << "Peer: " << pit->id;
#endif
			if (pit->actions & RS_SERVICE_PEER_CONNECTED)
			{
#ifdef CONTROL_DEBUG
				std::cerr << " is Newly Connected!";
				std::cerr << std::endl;
#endif
				setPeerState(it->second->mTransfer, pit->id, rate, true);
			}
			else if (pit->actions & RS_SERVICE_PEER_DISCONNECTED)
			{
#ifdef CONTROL_DEBUG
				std::cerr << " is Just disconnected!";
				std::cerr << std::endl;
#endif
				setPeerState(it->second->mTransfer, pit->id, rate, false);
			}
			else // Added or Removed.
			{
#ifdef CONTROL_DEBUG
				std::cerr << " had something happen to it: ";
				std::cerr << pit-> actions;
				std::cerr << std::endl;
#endif
				setPeerState(it->second->mTransfer, pit->id, rate, false);
			}
		}

		// Now also look at turtle virtual peers, for ongoing downloads only.
		//
		if(it->second->mState != ftFileControl::DOWNLOADING)
            continue ;

		std::list<pqipeer> vlist ;
		std::list<pqipeer>::const_iterator vit;
		mTurtle->getSourceVirtualPeersList(it->first,vlist) ;

#ifdef CONTROL_DEBUG
		std::cerr << "vlist.size() = " << vlist.size() << std::endl;
#endif

		for(vit = vlist.begin(); vit != vlist.end(); ++vit)
		{
#ifdef CONTROL_DEBUG
			std::cerr << "Peer: " << vit->id;
#endif
			if (vit->actions & RS_PEER_CONNECTED)
			{
#ifdef CONTROL_DEBUG
				std::cerr << " is Newly Connected!";
				std::cerr << std::endl;
#endif
				setPeerState(it->second->mTransfer, vit->id, rate, true);
			}
			else if (vit->actions & RS_PEER_DISCONNECTED)
			{
#ifdef CONTROL_DEBUG
				std::cerr << " is Just disconnected!";
				std::cerr << std::endl;
#endif
				setPeerState(it->second->mTransfer, vit->id, rate, false);
			}
			else
			{
#ifdef CONTROL_DEBUG
				std::cerr << " had something happen to it: ";
				std::cerr << vit-> actions;
				std::cerr << std::endl;
#endif
				setPeerState(it->second->mTransfer, vit->id, rate, false);
			}
		}
	}
}

const std::string active_downloads_size_ss("MAX_ACTIVE_DOWNLOADS");
const std::string download_dir_ss("DOWN_DIR");
const std::string partial_dir_ss("PART_DIR");
const std::string max_uploads_per_friend_ss("MAX_UPLOADS_PER_FRIEND");
const std::string default_chunk_strategy_ss("DEFAULT_CHUNK_STRATEGY");
const std::string free_space_limit_ss("FREE_SPACE_LIMIT");
const std::string default_encryption_policy_ss("DEFAULT_ENCRYPTION_POLICY");
const std::string file_perm_direct_dl_ss("FILE_PERM_DIRECT_DL");


	/* p3Config Interface */
RsSerialiser *ftController::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();

	/* add in the types we need! */
	rss->addSerialType(new RsFileConfigSerialiser());
	rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss;
}

bool ftController::saveList(bool &cleanup, std::list<RsItem *>& saveData)
{


	/* it can delete them! */
	cleanup = true;

	/* create a key/value set for most of the parameters */
	std::map<std::string, std::string> configMap;
	std::map<std::string, std::string>::iterator mit;
    std::list<RsFileHash>::iterator it;

	/* basic control parameters */
	std::string s ;
	rs_sprintf(s, "%lu", getQueueSize()) ;
	configMap[active_downloads_size_ss] = s ;
	configMap[download_dir_ss] = getDownloadDirectory();
	configMap[partial_dir_ss] = getPartialsDirectory();

	switch(mDefaultChunkStrategy)
	{
		case FileChunksInfo::CHUNK_STRATEGY_STREAMING: 	configMap[default_chunk_strategy_ss] =  "STREAMING" ;
																	  	break ;
		case FileChunksInfo::CHUNK_STRATEGY_RANDOM:		configMap[default_chunk_strategy_ss] =  "RANDOM" ;
																		break ;

		default:
		case FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE:configMap[default_chunk_strategy_ss] =  "PROGRESSIVE" ;
																		break ;
	}

	rs_sprintf(s,"%lu",_max_uploads_per_friend) ;
    configMap[max_uploads_per_friend_ss] = s ;

    configMap[default_encryption_policy_ss] = (mDefaultEncryptionPolicy==RS_FILE_CTRL_ENCRYPTION_POLICY_PERMISSIVE)?"PERMISSIVE":"STRICT" ;

	switch (mFilePermDirectDLPolicy) {
		case RS_FILE_PERM_DIRECT_DL_YES: configMap[file_perm_direct_dl_ss] = "YES" ;
		break;
		case RS_FILE_PERM_DIRECT_DL_NO: configMap[file_perm_direct_dl_ss] = "NO" ;
		break;
		default: configMap[file_perm_direct_dl_ss] = "PER_USER" ;
		break;
	}

	rs_sprintf(s, "%lu", RsDiscSpace::freeSpaceLimit());
	configMap[free_space_limit_ss] = s ;

	RsConfigKeyValueSet *rskv = new RsConfigKeyValueSet();

	/* Convert to TLV */
	for(mit = configMap.begin(); mit != configMap.end(); ++mit)
	{
		RsTlvKeyValue kv;
		kv.key = mit->first;
		kv.value = mit->second;

		rskv->tlvkvs.pairs.push_back(kv);
	}

	/* Add KeyValue to saveList */
	saveData.push_back(rskv);

	/* get list of Downloads ....
	 * strip out Caches / ExtraList / Channels????
	 * (anything with a callback?)
	 * - most systems will restart missing files.
	 */


	/* get Details of File Transfers */
    std::list<RsFileHash> hashs;
	FileDownloads(hashs);

	for(it = hashs.begin(); it != hashs.end(); ++it)
	{
		/* stack mutex released each loop */
  		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

        std::map<RsFileHash, ftFileControl*>::iterator fit = mDownloads.find(*it);
		if (fit == mDownloads.end())
			continue;

		/* ignore cache files. As this is small files, better download them again from scratch at restart.*/

		// Node: We still save finished transfers. This keeps transfers that are 
		// in checking mode. Finished or checked transfers will restart and
		// immediately terminate/recheck at next startup.
		//
		//		if ((fit->second)->mCreator->finished())
		//			continue;

		/* make RsFileTransfer item for save list */
		RsFileTransfer *rft = new RsFileTransfer();

		/* what data is important? */

		rft->file.name = fit->second->mName;
		rft->file.hash  = fit->second->mHash;
		rft->file.filesize = fit->second->mSize;
		RsDirUtil::removeTopDir(fit->second->mDestination, rft->file.path); /* remove fname */
		rft->flags = fit->second->mFlags.toUInt32();
		rft->state = fit->second->mState;

		std::list<RsPeerId> lst ;
		fit->second->mTransfer->getFileSources(lst);

		// Remove turtle peers from sources, as they are not supposed to survive a reboot of RS, since they are dynamic sources.
		// Otherwize, such sources are unknown from the turtle router, at restart, and never get removed.
		//
		for(std::list<RsPeerId>::const_iterator it(lst.begin());it!=lst.end();++it) 
			if(!mTurtle->isTurtlePeer(*it))
                rft->allPeerIds.ids.insert(*it) ;

		rft->transferred = fit->second->mCreator->getRecvd();
		fit->second->mCreator->getAvailabilityMap(rft->compressed_chunk_map) ;
		rft->chunk_strategy = fit->second->mCreator->getChunkStrategy() ;

		saveData.push_back(rft);
	}

	{
		/* Save pending list of downloads */
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		std::list<ftPendingRequest>::iterator pit;
		for (pit = mPendingRequests.begin(); pit != mPendingRequests.end(); ++pit)
		{
			/* make RsFileTransfer item for save list */
			RsFileTransfer *rft = NULL;

            std::map<RsFileHash,RsFileTransfer*>::iterator rit = mPendingChunkMaps.find(pit->mHash);
			if (rit != mPendingChunkMaps.end()) {
				/* use item from the not loaded pending list */
				rft = new RsFileTransfer(*(rit->second));
			} else {
				rft = new RsFileTransfer();

				/* what data is important? */

				rft->file.name = pit->mName;
				rft->file.hash  = pit->mHash;
				rft->file.filesize = pit->mSize;
				RsDirUtil::removeTopDir(pit->mDest, rft->file.path); /* remove fname */
				rft->flags = pit->mFlags.toUInt32();
				rft->state = pit->mState;

				rft->allPeerIds.ids.clear() ;
				for(std::list<RsPeerId>::const_iterator it(pit->mSrcIds.begin());it!=pit->mSrcIds.end();++it)
                    rft->allPeerIds.ids.insert( *it ) ;
			}

			// Remove turtle peers from sources, as they are not supposed to survive a reboot of RS, since they are dynamic sources.
			// Otherwize, such sources are unknown from the turtle router, at restart, and never get removed. We do that in post
			// process since the rft object may have been created from mPendingChunkMaps
			//
            for(std::set<RsPeerId>::iterator sit(rft->allPeerIds.ids.begin());sit!=rft->allPeerIds.ids.end();)
				if(mTurtle->isTurtlePeer(RsPeerId(*sit)))
                {
                    std::set<RsPeerId>::iterator sittmp(sit) ;
            ++sittmp ;
                    rft->allPeerIds.ids.erase(sit) ;
            sit = sittmp ;
                }
				else
					++sit ;

			saveData.push_back(rft);
		}
	}

	/* list completed! */
	return true;
}


bool ftController::loadList(std::list<RsItem *>& load)
{
	std::list<RsItem *>::iterator it;
	std::list<RsTlvKeyValue>::iterator kit;
	RsConfigKeyValueSet *rskv;
	RsFileTransfer      *rsft;

#ifdef CONTROL_DEBUG
	std::cerr << "ftController::loadList() Item Count: " << load.size();
	std::cerr << std::endl;
#endif

	for(it = load.begin(); it != load.end(); ++it)
	{
		/* switch on type */
		if (NULL != (rskv = dynamic_cast<RsConfigKeyValueSet *>(*it)))
		{
			/* make into map */
			std::map<std::string, std::string> configMap;
			for(kit = rskv->tlvkvs.pairs.begin();
				kit != rskv->tlvkvs.pairs.end(); ++kit)
			{
				configMap[kit->key] = kit->value;
			}

			loadConfigMap(configMap);

		}
		else if (NULL != (rsft = dynamic_cast<RsFileTransfer *>(*it)))
		{
			/* This will get stored on a waiting list - until the
			 * config files are fully loaded
			 */
#ifdef TO_REMOVE
			(csoler) I removed this because RS_FILE_HINTS_NETWORK_WIDE is actually equal to RS_FILE_REQ_ENCRYPTED, so this test removed the encrypted flag when loading!!
			// Compatibility with previous versions. 
			if(rsft->flags & RS_FILE_HINTS_NETWORK_WIDE.toUInt32())
			{
				std::cerr << "Ensuring compatibility, replacing RS_FILE_HINTS_NETWORK_WIDE with RS_FILE_REQ_ANONYMOUS_ROUTING" << std::endl;
				rsft->flags &= ~RS_FILE_HINTS_NETWORK_WIDE.toUInt32() ;
				rsft->flags |=  RS_FILE_REQ_ANONYMOUS_ROUTING.toUInt32() ;
			}
#endif

#ifdef CONTROL_DEBUG
			std::cerr << "ftController::loadList(): requesting " << rsft->file.name << ", " << rsft->file.hash << ", " << rsft->file.filesize << std::endl ;
#endif
			std::list<RsPeerId> src_lst ;
            for(std::set<RsPeerId>::const_iterator it(rsft->allPeerIds.ids.begin());it!=rsft->allPeerIds.ids.end();++it)
                src_lst.push_back(*it) ;

			FileRequest(rsft->file.name, rsft->file.hash, rsft->file.filesize, rsft->file.path, TransferRequestFlags(rsft->flags), src_lst, rsft->state);

			{
				RsStackMutex mtx(ctrlMutex) ;

                std::map<RsFileHash, ftFileControl*>::iterator fit = mDownloads.find(rsft->file.hash);

				if((fit==mDownloads.end() || (fit->second)->mCreator == NULL))
				{
					std::cerr << "ftController::loadList(): Error: could not find hash " << rsft->file.hash << " in mDownloads list !" << std::endl ;
					std::cerr << "Storing the map in a wait list." << std::endl ;

					mPendingChunkMaps[rsft->file.hash] = rsft ;

					continue ;	// i.e. don't delete the item!
				}
				else
				{
					(fit->second)->mCreator->setAvailabilityMap(rsft->compressed_chunk_map) ;
					(fit->second)->mCreator->setChunkStrategy((FileChunksInfo::ChunkStrategy)(rsft->chunk_strategy)) ;
				}
			}
		}

		/* cleanup */
		delete (*it);
	}
    load.clear() ;
	return true;

}

bool  ftController::loadConfigMap(std::map<std::string, std::string> &configMap)
{
	std::map<std::string, std::string>::iterator mit;

	//std::string str_true("true");
	//std::string empty("");
	//std::string dir = "notempty";

	if (configMap.end() != (mit = configMap.find(download_dir_ss)))
		setDownloadDirectory(mit->second);

	if (configMap.end() != (mit = configMap.find(active_downloads_size_ss)))
	{
		int n=5 ;
		sscanf(mit->second.c_str(), "%d", &n);
		std::cerr << "Note: loading active max downloads: " << n << std::endl;
		setQueueSize(n);
	}
	if (configMap.end() != (mit = configMap.find(partial_dir_ss)))
	{
		setPartialsDirectory(mit->second);
	}

    if (configMap.end() != (mit = configMap.find(default_encryption_policy_ss)))
    {
        if(mit->second == "STRICT")
        {
            mDefaultEncryptionPolicy = RS_FILE_CTRL_ENCRYPTION_POLICY_STRICT ;
            std::cerr << "Note: loading default value for encryption policy: STRICT" << std::endl;
        }
        else if(mit->second == "PERMISSIVE")
        {
            mDefaultEncryptionPolicy = RS_FILE_CTRL_ENCRYPTION_POLICY_PERMISSIVE ;
            std::cerr << "Note: loading default value for encryption policy: PERMISSIVE" << std::endl;
        }
        else
        {
            std::cerr << "(EE) encryption policy not recognized: \"" << mit->second << "\"" << std::endl;
            mDefaultEncryptionPolicy = RS_FILE_CTRL_ENCRYPTION_POLICY_PERMISSIVE ;
        }
    }

    if (configMap.end() != (mit = configMap.find(default_chunk_strategy_ss)))
	{
		if(mit->second == "STREAMING")
		{
			setDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_STREAMING) ;
			std::cerr << "Note: loading default value for chunk strategy: streaming" << std::endl;
		}
		else if(mit->second == "RANDOM")
		{
			setDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_RANDOM) ;
			std::cerr << "Note: loading default value for chunk strategy: random" << std::endl;
		}
		else if(mit->second == "PROGRESSIVE")
		{
			setDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE) ;
			std::cerr << "Note: loading default value for chunk strategy: progressive" << std::endl;
		}
		else
			std::cerr << "**** ERROR ***: Unknown value for default chunk strategy in keymap." << std::endl ;
	}

	if (configMap.end() != (mit = configMap.find(free_space_limit_ss)))
	{
		uint32_t size ;
		if (sscanf(mit->second.c_str(), "%u", &size) == 1) {
			std::cerr << "have read a size limit of " << size <<" MB" << std::endl ;

			RsDiscSpace::setFreeSpaceLimit(size) ;
		}
	}
    if(configMap.end() != (mit = configMap.find(max_uploads_per_friend_ss)))
    {
		uint32_t n ;
		if (sscanf(mit->second.c_str(), "%u", &n) == 1) {
			std::cerr << "have read a max upload slots limit of " << n << std::endl ;

            _max_uploads_per_friend = n ;
		}
    }

	if(configMap.end() != (mit = configMap.find(file_perm_direct_dl_ss)))
	{
		if(mit->second == "YES")
		{
			mFilePermDirectDLPolicy = RS_FILE_PERM_DIRECT_DL_YES ;
			std::cerr << "Note: loading default value for file permission direct download: YES" << std::endl;
		}
		else if(mit->second == "NO")
		{
			mFilePermDirectDLPolicy = RS_FILE_PERM_DIRECT_DL_NO ;
			std::cerr << "Note: loading default value for file permission direct download: NO" << std::endl;
		}
		else if(mit->second == "PER_USER")
		{
			mFilePermDirectDLPolicy = RS_FILE_PERM_DIRECT_DL_PER_USER ;
			std::cerr << "Note: loading default value for file permission direct download: PER_USER" << std::endl;
		}
	}

	return true;
}

void ftController::setMaxUploadsPerFriend(uint32_t m)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	_max_uploads_per_friend = m ;
	IndicateConfigChanged();
}
uint32_t ftController::getMaxUploadsPerFriend()
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	return _max_uploads_per_friend ;
}
void ftController::setDefaultEncryptionPolicy(uint32_t p)
{
    RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
    mDefaultEncryptionPolicy = p ;
    IndicateConfigChanged();
}
uint32_t ftController::defaultEncryptionPolicy()
{
    RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
    return mDefaultEncryptionPolicy ;
}

void ftController::setFilePermDirectDL(uint32_t perm)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	if (mFilePermDirectDLPolicy != perm)
	{
		mFilePermDirectDLPolicy = perm;
		IndicateConfigChanged();
	}
}
uint32_t ftController::filePermDirectDL()
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	return mFilePermDirectDLPolicy;
}

void ftController::setFreeDiskSpaceLimit(uint32_t size_in_mb)
{
	RsDiscSpace::setFreeSpaceLimit(size_in_mb) ;

	IndicateConfigChanged() ;
}


uint32_t ftController::freeDiskSpaceLimit() const
{
	return RsDiscSpace::freeSpaceLimit() ;
}

FileChunksInfo::ChunkStrategy ftController::defaultChunkStrategy()
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	return mDefaultChunkStrategy ;
}
void ftController::setDefaultChunkStrategy(FileChunksInfo::ChunkStrategy S)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
#ifdef CONTROL_DEBUG
	std::cerr << "Note: in frController: setting chunk strategy to " << S << std::endl ;
#endif
	mDefaultChunkStrategy = S ;
	IndicateConfigChanged() ;
}
