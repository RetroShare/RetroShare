/*
 * libretroshare/src/ft: ftcontroller.cc
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

#include "serialiser/rsconfigitems.h"
#include <stdio.h>
#include <unistd.h>		/* for (u)sleep() */
#include <time.h>

/******
 * #define CONTROL_DEBUG 1
 * #define DEBUG_DWLQUEUE 1
 *****/

static const int32_t SAVE_TRANSFERS_DELAY 			= 301	; // save transfer progress every 301 seconds.
static const int32_t INACTIVE_CHUNKS_CHECK_DELAY 	= 240	; // time after which an inactive chunk is released
static const int32_t MAX_TIME_INACTIVE_REQUEUED 	= 120 ; // time after which an inactive ftFileControl is bt-queued
static const int32_t TIMOUT_CACHE_FILE_TRANSFER 	= 800 ; // time after which cache transfer gets cancelled if inactive.

static const int32_t FT_FILECONTROL_QUEUE_ADD_END 			= 0 ;
static const int32_t FT_FILECONTROL_QUEUE_ADD_AFTER_CACHE 	= 1 ;

const uint32_t FT_CNTRL_STANDARD_RATE = 10 * 1024 * 1024;
const uint32_t FT_CNTRL_SLOW_RATE     = 100   * 1024;

ftFileControl::ftFileControl()
	:mTransfer(NULL), mCreator(NULL),
	 mState(DOWNLOADING), mSize(0), mFlags(0), mCreateTime(0), mQueuePriority(0), mQueuePosition(0)
{
	return;
}

ftFileControl::ftFileControl(std::string fname,
		std::string tmppath, std::string dest,
        uint64_t size, const RsFileHash &hash, TransferRequestFlags flags,
		ftFileCreator *fc, ftTransferModule *tm)
	:mName(fname), mCurrentPath(tmppath), mDestination(dest),
	 mTransfer(tm), mCreator(fc), mState(DOWNLOADING), mHash(hash),
	 mSize(size), mFlags(flags), mCreateTime(0), mQueuePriority(0), mQueuePosition(0)
{
	return;
}

ftController::ftController(CacheStrapper *cs, ftDataMultiplex *dm, p3ServiceControl *sc, uint32_t ftServiceId)
	:CacheTransfer(cs), p3Config(), 
	last_save_time(0),
	last_clean_time(0),
	mDataplex(dm),
	mTurtle(NULL), 
	mFtServer(NULL), 
	mServiceCtrl(sc),
	mFtServiceId(ftServiceId),
	ctrlMutex("ftController"),
	doneMutex("ftController"),
	mFtActive(false),
	mDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE) 
{
	_max_active_downloads = 5 ; // default queue size
	_min_prioritized_transfers = 3 ;
	/* TODO */
    cnt = 0 ;
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
		setPeerState(it->second->mTransfer, peer_id, FT_CNTRL_STANDARD_RATE, mServiceCtrl->isPeerConnected(mFtServiceId, peer_id ));

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
		usleep(1*1000*1000); // 1 sec

#ifdef CONTROL_DEBUG
		std::cerr << "ftController::run()";
		std::cerr << std::endl;
#endif
		bool doPending = false;
		{
		  	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
			doPending = (mFtActive) && (!mFtPendingDone);
		}

		time_t now = time(NULL) ;
		if(now > last_save_time + SAVE_TRANSFERS_DELAY)
		{
			cleanCacheDownloads() ;
			searchForDirectSources() ;

			IndicateConfigChanged() ;
			last_save_time = now ;
		}

		if(now > last_clean_time + INACTIVE_CHUNKS_CHECK_DELAY)
		{
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
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

    for(std::map<RsFileHash,ftFileControl*>::iterator it(mDownloads.begin()); it != mDownloads.end(); ++it)
		if(it->second->mState != ftFileControl::QUEUED && it->second->mState != ftFileControl::PAUSED)
			if(! (it->second->mFlags & RS_FILE_REQ_CACHE))
			{
				FileInfo info ;	// info needs to be re-allocated each time, to start with a clear list of peers (it's not cleared down there)

				if(mSearch->search(it->first, RS_FILE_HINTS_REMOTE | RS_FILE_HINTS_SPEC_ONLY, info))
					for(std::list<TransferInfo>::const_iterator pit = info.peers.begin(); pit != info.peers.end(); ++pit)
                        if(rsPeers->servicePermissionFlags(pit->peerId) & RS_NODE_PERM_DIRECT_DL)
							if(it->second->mTransfer->addFileSource(pit->peerId)) /* if the sources don't exist already - add in */
								setPeerState(it->second->mTransfer, pit->peerId, FT_CNTRL_STANDARD_RATE, mServiceCtrl->isPeerConnected(mFtServiceId, pit->peerId));
			}
}

void ftController::tickTransfers()
{
	// 1 - sort modules into arrays according to priority
	
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

#ifdef CONTROL_DEBUG
	std::cerr << "ticking transfers." << std::endl ;
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

void ftController::cleanCacheDownloads()
{
    std::vector<RsFileHash> toCancel ;
	time_t now = time(NULL) ;

	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

        for(std::map<RsFileHash,ftFileControl*>::iterator it(mDownloads.begin());it!=mDownloads.end();++it)
			if (((it->second)->mFlags & RS_FILE_REQ_CACHE) && it->second->mState != ftFileControl::DOWNLOADING)
				// check if a cache file is downloaded, if the case, timeout the transfer after TIMOUT_CACHE_FILE_TRANSFER
			{
#ifdef CONTROL_DEBUG
				std::cerr << "ftController::run() cache transfer found. age of this tranfer is :" << (int)(time(NULL) - (it->second)->mCreateTime);
				std::cerr << std::endl;
#endif
				if (now > (it->second)->mCreator->creationTimeStamp() + TIMOUT_CACHE_FILE_TRANSFER) 
				{
#ifdef CONTROL_DEBUG
					std::cerr << "ftController::run() cache transfer to old. Cancelling transfer. Hash :" << (it->second)->mHash << ", time=" << (it->second)->mCreateTime << ", now = " << time(NULL) ;
					std::cerr << std::endl;
#endif
					toCancel.push_back((it->second)->mHash);
				}
			}
	}

	for(uint32_t i=0;i<toCancel.size();++i)
		FileCancel(toCancel[i]);
}

/* Called every 10 seconds or so */
void ftController::checkDownloadQueue()
{
	// We do multiple things here:
	//
	// 1 - are there queued files ?
	// 	YES
	// 		1.1 - check for inactive files (see below).
	// 				- select one inactive file
	// 					- close it temporarily
	// 					- remove it from turtle handling
	// 					- move the ftFileControl to queued list
	// 					- set the queue priority to 1+largest in the queue.
	// 		1.2 - pop from the queue the 1st file to come (according to priority)
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

	// Check for inactive transfers.
	//
	time_t now = time(NULL) ;
	uint32_t nb_moved = 0 ;	// don't move more files than the size of the queue.

    for(std::map<RsFileHash,ftFileControl*>::const_iterator it(mDownloads.begin());it!=mDownloads.end() && nb_moved <= _max_active_downloads;++it)
		if(	it->second->mState != ftFileControl::QUEUED 
                && (it->second->mState == ftFileControl::PAUSED
                    || now > it->second->mTransfer->lastActvTimeStamp() + (time_t)MAX_TIME_INACTIVE_REQUEUED))
		{
#ifdef DEBUG_DWLQUEUE
			std::cerr << "  - Inactive file " << it->second->mName << " at position " << it->second->mQueuePosition << " moved to end of the queue. mState=" << it->second->mState << ", time lapse=" << now - it->second->mCreator->lastActvTimeStamp()  << std::endl ;
#endif
			locked_bottomQueue(it->second->mQueuePosition) ;
#ifdef DEBUG_DWLQUEUE
			std::cerr << "  new position: " << it->second->mQueuePosition << std::endl ;
			std::cerr << "  new state: " << it->second->mState << std::endl ;
#endif
			it->second->mTransfer->resetActvTimeStamp() ;	// very important!
			++nb_moved ;
		}

	// Check that at least _min_prioritized_transfers are assigned to non cache transfers

	std::cerr << "Asserting that at least " << _min_prioritized_transfers << " are dedicated to user transfers." << std::endl;

	int user_transfers = 0 ;
	std::vector<uint32_t> to_move_before ;	
	std::vector<uint32_t> to_move_after ;

	for(uint32_t p=0;p<_queue.size();++p)
	{
		if(p < _min_prioritized_transfers)
			if(_queue[p]->mFlags & RS_FILE_REQ_CACHE)		// cache file. add to potential move list
				to_move_before.push_back(p) ;
			else
				++user_transfers ;									// count one more user file in the prioritized range.
		else
		{
			if(to_move_after.size() + user_transfers >= _min_prioritized_transfers)	// we caught enough transfers to move back to the top of the queue.
				break ;

			if(!(_queue[p]->mFlags & RS_FILE_REQ_CACHE))	// non cache file. add to potential move list
				to_move_after.push_back(p) ;
		}
	}
	uint32_t to_move = (uint32_t)std::max(0,(int)_min_prioritized_transfers - (int)user_transfers) ;	// we move as many transfers as needed to get _min_prioritized_transfers user transfers.

	std::cerr << "  collected " << to_move << " transfers to move." << std::endl;

	for(uint32_t i=0;i<to_move && i < to_move_after.size() && i<to_move_before.size();++i)
		locked_swapQueue(to_move_before[i],to_move_after[i]) ;
}

void ftController::locked_addToQueue(ftFileControl* ftfc,int add_strategy)
{
#ifdef DEBUG_DWLQUEUE
	std::cerr << "Queueing ftfileControl " << (void*)ftfc << ", name=" << ftfc->mName << std::endl ;
#endif

	switch(add_strategy)
	{
		// Different strategies for files and cache files:
		// 	- a min number of slots is reserved to user file transfer
		// 	- cache files are always added after this slot.
		//
		case FT_FILECONTROL_QUEUE_ADD_END:			 _queue.push_back(ftfc) ;
																 locked_checkQueueElement(_queue.size()-1) ;
																 break ;
		case FT_FILECONTROL_QUEUE_ADD_AFTER_CACHE:
																 {
																	 // We add the transfer just before the first non cache transfer.
																	 // This is costly, so only use this in case we really need it.
																	 //
																	 uint32_t pos =0;
																	 while(pos < _queue.size() && (pos < _min_prioritized_transfers || (_queue[pos]->mFlags & RS_FILE_REQ_CACHE)>0) )
																		 ++pos ;

																	 _queue.push_back(NULL) ;

																	 for(int i=int(_queue.size())-1;i>(int)pos;--i)
																	 {
																		 _queue[i] = _queue[i-1] ;
																		 locked_checkQueueElement(i) ;
																	 }

																	 _queue[pos] = ftfc ;
																	 locked_checkQueueElement(pos) ;
																 }
																 break ;
	}
}

void ftController::locked_queueRemove(uint32_t pos)
{
	for(uint32_t p=pos;p<_queue.size()-1;++p)
	{
		_queue[p]=_queue[p+1] ;
		locked_checkQueueElement(p) ;
	}
	_queue.pop_back();
}

void ftController::setMinPrioritizedTransfers(uint32_t s)
{
	RsStackMutex mtx(ctrlMutex) ;
	_min_prioritized_transfers = s ;
}
uint32_t ftController::getMinPrioritizedTransfers()
{
	RsStackMutex mtx(ctrlMutex) ;
	return _min_prioritized_transfers ;
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
			if(p < _queue.size())
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

		case QUEUE_DOWN: 		if(pos < _queue.size()-1)
										locked_swapQueue(pos,pos+1) ;
									break ;
		default:
									std::cerr << "ftController::moveInQueue: unknown move " << mv << std::endl ;
	}
}

void ftController::locked_topQueue(uint32_t pos)
{
	ftFileControl *tmp=_queue[pos] ;

	for(int p=pos;p>0;--p)
	{
		_queue[p]=_queue[p-1] ;
		locked_checkQueueElement(p) ;
	}
	_queue[0]=tmp ;

	locked_checkQueueElement(0) ;
}
void ftController::locked_bottomQueue(uint32_t pos)
{
	ftFileControl *tmp=_queue[pos] ;

	for(uint32_t p=pos;p<_queue.size()-1;++p)
	{
		_queue[p]=_queue[p+1] ;
		locked_checkQueueElement(p) ;
	}
	_queue[_queue.size()-1]=tmp ;
	locked_checkQueueElement(_queue.size()-1) ;
}
void ftController::locked_swapQueue(uint32_t pos1,uint32_t pos2)
{
	// Swap the element at position pos with the last element of the queue

	if(pos1==pos2)
		return ;

	ftFileControl *tmp = _queue[pos1] ;
	_queue[pos1] = _queue[pos2] ;
	_queue[pos2] = tmp;

	locked_checkQueueElement(pos1) ;
	locked_checkQueueElement(pos2) ;
}

void ftController::locked_checkQueueElement(uint32_t pos)
{
	_queue[pos]->mQueuePosition = pos ;

	if(pos < _max_active_downloads && _queue[pos]->mState != ftFileControl::PAUSED)
	{
		if(_queue[pos]->mState == ftFileControl::QUEUED)
			_queue[pos]->mTransfer->resetActvTimeStamp() ;

		_queue[pos]->mState = ftFileControl::DOWNLOADING ;

		if(_queue[pos]->mFlags & RS_FILE_REQ_ANONYMOUS_ROUTING)
            mTurtle->monitorTunnels(_queue[pos]->mHash,mFtServer,true) ;
	}

	if(pos >= _max_active_downloads && _queue[pos]->mState != ftFileControl::QUEUED && _queue[pos]->mState != ftFileControl::PAUSED)
	{
		_queue[pos]->mState = ftFileControl::QUEUED ;
		_queue[pos]->mCreator->closeFile() ;

		if(_queue[pos]->mFlags & RS_FILE_REQ_ANONYMOUS_ROUTING)
			mTurtle->stopMonitoringTunnels(_queue[pos]->mHash) ;
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

bool ftController::moveFile(const std::string& source,const std::string& dest)
{
	// First try a rename
	//

#ifdef WINDOWS_SYS
	std::wstring sourceW;
	std::wstring destW;
	librs::util::ConvertUtf8ToUtf16(source,sourceW);
	librs::util::ConvertUtf8ToUtf16(dest,destW);

	if( 0 != MoveFileW(sourceW.c_str(), destW.c_str()))
#else
	if (0 == rename(source.c_str(), dest.c_str()))
#endif
	{
#ifdef CONTROL_DEBUG
                std::cerr << "ftController::completeFile() renaming to: ";
                std::cerr << dest;
                std::cerr << std::endl;
#endif

		return true ;
	}
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::completeFile() FAILED mv to: ";
	std::cerr << dest;
	std::cerr << std::endl;
	std::cerr << "trying copy" << std::endl ;
#endif
	// We could not rename, probably because we're dealing with different file systems.
	// Let's copy then.

#ifdef WINDOWS_SYS
	if(CopyFileW(sourceW.c_str(), destW.c_str(), FALSE) == 0)
#else
	if(!copyFile(source,dest))
#endif
		return false ;

	// copy was successfull, let's delete the original
	std::cerr << "deleting original file " << source << std::endl ;

#ifdef WINDOWS_SYS
	if(0 != DeleteFileW(sourceW.c_str()))
#else
	if(0 == remove(source.c_str()))
#endif
		return true ;
	else
	{
		RsServer::notify()->AddSysMessage(0, RS_SYS_WARNING, "File erase error", "Error while removing hash file " + dest + "\nRead-only file system ?");
		return false ;
	}
}

bool ftController::copyFile(const std::string& source,const std::string& dest)
{
	FILE *in = RsDirUtil::rs_fopen(source.c_str(),"rb") ;

	if(in == NULL)
	{
		//RsServer::notify()->AddSysMessage(0, RS_SYS_WARNING, "File copy error", "Error while copying file " + dest + "\nCannot open input file "+source);
		std::cerr << "******************** FT CONTROLLER ERROR ************************" << std::endl;
		std::cerr << "Error while copying file " + dest + "\nCannot open input file "+source << std::endl;
		std::cerr << "*****************************************************************" << std::endl;
		return false ;
	}

	FILE *out = RsDirUtil::rs_fopen(dest.c_str(),"wb") ;

	if(out == NULL)
	{
		RsServer::notify()->AddSysMessage(0, RS_SYS_WARNING, "File copy error", "Error while copying file " + dest + "\nCheck for disk full, or write permission ?\nOriginal file kept under the name "+source);
		fclose (in);
		return false ;
	}

	size_t s=0;
	size_t T=0;

	static const int BUFF_SIZE = 10485760 ; // 10 MB buffer to speed things up.
	void *buffer = rs_malloc(BUFF_SIZE) ;

    	if(buffer == NULL)
        {
	    fclose (in);
	    fclose (out);
            return false ;
        }
	bool bRet = true;

	while( (s = fread(buffer,1,BUFF_SIZE,in)) > 0)
	{
		size_t t = fwrite(buffer,1,s,out) ;
		T += t ;

		if(t != s)
		{
			RsServer::notify()->AddSysMessage(0, RS_SYS_WARNING, "File copy error", "Error while copying file " + dest + "\nIs your disc full ?\nOriginal file kept under the name "+source);
			bRet = false ;
			break;
		}
	}

	fclose(in) ;
	fclose(out) ;

	free(buffer) ;

	return bRet ;
}



bool ftController::completeFile(const RsFileHash& hash)
{
	/* variables... so we can drop mutex later */
	std::string path;
	std::string name;
	uint64_t    size = 0;
	uint32_t    state = 0;
	uint32_t    period = 0;
	TransferRequestFlags flags ;
	TransferRequestFlags extraflags ;
	uint32_t    completeCount = 0;

	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

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

		// I don't know how the size can be zero, but believe me, this happens,
		// and it causes an error on linux because then the file may not even exist.
		//
		if( fc->mSize > 0 && moveFile(fc->mCurrentPath,fc->mDestination) )
			fc->mCurrentPath = fc->mDestination;
		else
			fc->mState = ftFileControl::ERROR_COMPLETION;

		/* for extralist additions */
		path    = fc->mDestination;
		name    = fc->mName;
		//hash    = fc->mHash;
		size    = fc->mSize;
		state   = fc->mState;
		period  = 30 * 24 * 3600; /* 30 days */
		extraflags.clear() ;

#ifdef CONTROL_DEBUG
		std::cerr << "CompleteFile(): size = " << size << std::endl ;
#endif

		flags = fc->mFlags ;

		locked_queueRemove(it->second->mQueuePosition) ;

		/* switch map */
		if (!(fc->mFlags & RS_FILE_REQ_CACHE)) /* clean up completed cache files automatically */
		{
			mCompleted[fc->mHash] = fc;
			completeCount = mCompleted.size();
		} else
			delete fc ;

		mDownloads.erase(it);

		if(flags & RS_FILE_REQ_ANONYMOUS_ROUTING)
			mTurtle->stopMonitoringTunnels(hash_to_suppress) ;

	} /******* UNLOCKED ********/


	/******************** NO Mutex from Now ********************
	 * cos Callback can end up back in this class.
	 ***********************************************************/

	/* If it has a callback - do it now */

	if(flags & ( RS_FILE_REQ_CACHE | RS_FILE_REQ_EXTRA))// | RS_FILE_HINTS_MEDIA))
	{
#ifdef CONTROL_DEBUG
	  std::cerr << "ftController::completeFile() doing Callback, callbackflags:" << (flags & ( RS_FILE_REQ_CACHE | RS_FILE_REQ_EXTRA ));//| RS_FILE_HINTS_MEDIA)) ;
	  std::cerr << std::endl;
#endif
	  if(flags & RS_FILE_REQ_CACHE)
	  {
		  /* callback */
		  if (state == ftFileControl::COMPLETED)
		  {
#ifdef CONTROL_DEBUG
			  std::cerr << "ftController::completeFile() doing Callback : Success";
			  std::cerr << std::endl;
#endif

			  CompletedCache(hash);
		  }
		  else
		  {
#ifdef CONTROL_DEBUG
			  std::cerr << "ftController::completeFile() Cache Callback : Failed";
			  std::cerr << std::endl;
#endif
			  FailedCache(hash);
		  }
	  }

	  if(flags & RS_FILE_REQ_EXTRA)
	  {
#ifdef CONTROL_DEBUG
		  std::cerr << "ftController::completeFile() adding to ExtraList";
		  std::cerr << std::endl;
#endif

		  mExtraList->addExtraFile(path, hash, size, period, extraflags);
	  }

//	  if(flags & RS_FILE_HINTS_MEDIA)
//	  {
//#ifdef CONTROL_DEBUG
//		std::cerr << "ftController::completeFile() NULL MEDIA callback";
//		std::cerr << std::endl;
//#endif
//	  }
	}
	else
	{
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::completeFile() No callback";
		std::cerr << std::endl;
#endif
	}

	/* Notify GUI */
	if ((flags & RS_FILE_REQ_CACHE) == 0) {
        RsServer::notify()->AddPopupMessage(RS_POPUP_DOWNLOAD, hash.toStdString(), name, "");

        RsServer::notify()->notifyDownloadComplete(hash.toStdString());
		RsServer::notify()->notifyDownloadCompleteCount(completeCount);

		rsFiles->ForceDirectoryCheck() ;
	}

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
	ftPendingRequest req;
	{ 
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		if (mPendingRequests.size() < 1)
		{
			return false;
		}
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

	return true ;
}

bool ftController::alreadyHaveFile(const RsFileHash& hash, FileInfo &info)
{
	// check for downloads
	if (FileDetails(hash, info) && (info.downloadStatus == FT_STATE_COMPLETE))
		return true ;

	// check for file lists
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
	if(!(flags & RS_FILE_REQ_CACHE))
        for(std::list<RsPeerId>::iterator it = srcIds.begin(); it != srcIds.end(); )
            if(!(rsPeers->servicePermissionFlags(*it) & RS_NODE_PERM_DIRECT_DL))
			{
                std::list<RsPeerId>::iterator tmp(it) ;
				++tmp ;
				srcIds.erase(it) ;
				it = tmp ;
			}
			else
				++it ;
	
    std::list<RsPeerId>::const_iterator it;
	std::list<TransferInfo>::const_iterator pit;

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
                if(rsPeers->servicePermissionFlags(*it) & RS_NODE_PERM_DIRECT_DL)
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
					setPeerState(dit->second->mTransfer, *it, rate, mServiceCtrl->isPeerConnected(mFtServiceId, *it));
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


	if(!(flags & RS_FILE_REQ_NO_SEARCH))
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

                if ((srcIds.end() == std::find( srcIds.begin(), srcIds.end(), pit->peerId)) && (RS_NODE_PERM_DIRECT_DL & rsPeers->servicePermissionFlags(pit->peerId)))
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
        mTurtle->monitorTunnels(hash,mFtServer,true) ;

	bool assume_availability = flags & RS_FILE_REQ_CACHE ;	// assume availability for cache files

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
		setPeerState(tm, *it, rate, mServiceCtrl->isPeerConnected(mFtServiceId, *it));
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
	rsTurtle->stopMonitoringTunnels(hash) ;

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

bool 	ftController::FileClearCompleted()
{
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::FileClearCompleted()" <<std::endl;
#endif
	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

        for(std::map<RsFileHash, ftFileControl*>::iterator it(mCompleted.begin());it!=mCompleted.end();++it)
			delete it->second ;

		mCompleted.clear();

		IndicateConfigChanged();
	}  /******* UNLOCKED ********/

	RsServer::notify()->notifyDownloadCompleteCount(0);

	return false;
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

        if (!path.find(mDownloadPath)) {
            return false;
        }

        if (rsFiles) {
            std::list<SharedDirInfo>::iterator it;
            std::list<SharedDirInfo> dirs;
            rsFiles->getSharedDirectories(dirs);
            for (it = dirs.begin(); it != dirs.end(); ++it) {
                if (!path.find((*it).filename)) {
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
		info.storage_permission_flags |= DIR_FLAGS_NETWORK_WIDE_OTHERS ;	// file being downloaded anonymously are always anonymously available.

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
		if(it->second->mState == ftFileControl::DOWNLOADING)
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

		// Now also look at turtle virtual peers.
		//
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

	/* Cache Interface */
bool ftController::RequestCacheFile(const RsPeerId& id, std::string path, const RsFileHash& hash, uint64_t size)
{
#ifdef CONTROL_DEBUG
 std::cerr << "ftController::RequestCacheFile(" << id << ",";
 std::cerr << path << "," << hash << "," << size << ")";
 std::cerr << std::endl;
#endif

	/* Request File */
    std::list<RsPeerId> ids;
	ids.push_back(id);

	FileInfo info ;
	if(mSearch->search(hash, RS_FILE_HINTS_CACHE, info))
	{
#ifdef CONTROL_DEBUG
		std::cerr << "I already have this file:" << std::endl ;
		std::cerr << "  path: " << info.path << std::endl ;
		std::cerr << "  fname: " << info.fname << std::endl ;
		std::cerr << "  hash: " << info.hash << std::endl ;

		std::cerr << "Copying it !!" << std::endl ;
#endif

        if(info.size > 0 && copyFile(info.path,path+"/"+hash.toStdString()))
		{
			CompletedCache(hash);
			return true ;
		}
		else
			return false ;
	}

    FileRequest(hash.toStdString(), hash, size, path, RS_FILE_REQ_CACHE | RS_FILE_REQ_NO_SEARCH, ids);

	return true;
}


bool ftController::CancelCacheFile(const RsPeerId& id, std::string path, const RsFileHash& hash, uint64_t size)
{
	std::cerr << "ftController::CancelCacheFile(" << id << ",";
	std::cerr << path << "," << hash << "," << size << ")";
	std::cerr << std::endl;
#ifdef CONTROL_DEBUG
#endif

	return FileCancel(hash);

}

const std::string active_downloads_size_ss("MAX_ACTIVE_DOWNLOADS");
const std::string min_prioritized_downl_ss("MIN_PRORITIZED_DOWNLOADS");
const std::string download_dir_ss("DOWN_DIR");
const std::string partial_dir_ss("PART_DIR");
const std::string default_chunk_strategy_ss("DEFAULT_CHUNK_STRATEGY");
const std::string free_space_limit_ss("FREE_SPACE_LIMIT");


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
	rs_sprintf(s, "%lu", getMinPrioritizedTransfers()) ;
	configMap[min_prioritized_downl_ss] = s ;
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

		if (fit->second->mFlags & RS_FILE_REQ_CACHE)
		{
#ifdef CONTROL_DEBUG
			std::cerr << "ftcontroller::saveList(): Not saving (callback) file entry " << fit->second->mName << ", " << fit->second->mHash << ", " << fit->second->mSize << std::endl ;
#endif
			continue;
		}

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
			
			// Compatibility with previous versions. 
			if(rsft->flags & RS_FILE_HINTS_NETWORK_WIDE.toUInt32())
			{
				std::cerr << "Ensuring compatibility, replacing RS_FILE_HINTS_NETWORK_WIDE with RS_FILE_REQ_ANONYMOUS_ROUTING" << std::endl;
				rsft->flags &= ~RS_FILE_HINTS_NETWORK_WIDE.toUInt32() ;
				rsft->flags |=  RS_FILE_REQ_ANONYMOUS_ROUTING.toUInt32() ;
			}

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

	std::string str_true("true");
	std::string empty("");
	std::string dir = "notempty";

	if (configMap.end() != (mit = configMap.find(download_dir_ss)))
		setDownloadDirectory(mit->second);

	if (configMap.end() != (mit = configMap.find(active_downloads_size_ss)))
	{
		int n=5 ;
		sscanf(mit->second.c_str(), "%d", &n);
		std::cerr << "Note: loading active max downloads: " << n << std::endl;
		setQueueSize(n);
	}
	if (configMap.end() != (mit = configMap.find(min_prioritized_downl_ss)))
	{
		int n=3 ;
		sscanf(mit->second.c_str(), "%d", &n);
		std::cerr << "Note: loading min prioritized downloads: " << n << std::endl;
		setMinPrioritizedTransfers(n);
	}
	if (configMap.end() != (mit = configMap.find(partial_dir_ss)))
	{
		setPartialsDirectory(mit->second);
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
	return true;
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
