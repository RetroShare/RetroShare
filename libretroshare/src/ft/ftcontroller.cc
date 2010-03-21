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

#include "ft/ftcontroller.h"

#include "ft/ftfilecreator.h"
#include "ft/fttransfermodule.h"
#include "ft/ftsearch.h"
#include "ft/ftdatamultiplex.h"
#include "ft/ftextralist.h"

#include "turtle/p3turtle.h"

#include "util/rsdir.h"

#include "pqi/p3connmgr.h"
#include "pqi/pqinotify.h"

#include "serialiser/rsconfigitems.h"
#include <stdio.h>

/******
 * #define CONTROL_DEBUG 1
 * #define DEBUG_DWLQUEUE 1
 *****/
#define DEBUG_DWLQUEUE 1

static const uint32_t SAVE_TRANSFERS_DELAY 			= 61	; // save transfer progress every 61 seconds.
static const uint32_t INACTIVE_CHUNKS_CHECK_DELAY 	= 60	; // time after which an inactive chunk is released
static const uint32_t MAX_TIME_INACTIVE_REQUEUED 	= 60  ; // time after which an inactive ftFileControl is bt-queued

ftFileControl::ftFileControl()
	:mTransfer(NULL), mCreator(NULL),
	 mState(DOWNLOADING), mSize(0), mFlags(0),
	 mPriority(SPEED_NORMAL)
{
	return;
}

ftFileControl::ftFileControl(std::string fname,
		std::string tmppath, std::string dest,
		uint64_t size, std::string hash, uint32_t flags,
		ftFileCreator *fc, ftTransferModule *tm, uint32_t cb)
	:mName(fname), mCurrentPath(tmppath), mDestination(dest),
	 mTransfer(tm), mCreator(fc), mState(DOWNLOADING), mHash(hash),
	 mSize(size), mFlags(flags), mDoCallback(false), mCallbackCode(cb),
	 mPriority(SPEED_NORMAL)	// default priority to normal
{
	if (cb)
		mDoCallback = true;
	return;
}

ftController::ftController(CacheStrapper *cs, ftDataMultiplex *dm, std::string configDir)
	:CacheTransfer(cs), p3Config(CONFIG_TYPE_FT_CONTROL), 
	last_save_time(0),
	last_clean_time(0),
	mDataplex(dm),
	mTurtle(NULL), 
	mFtActive(false),
	mShareDownloadDir(true)
{
	_max_active_downloads = 5 ; // default queue size
	/* TODO */
}

void ftController::setTurtleRouter(p3turtle *pt)
{
	mTurtle = pt ;
}
void ftController::setFtSearchNExtra(ftSearch *search, ftExtraList *list)
{
	mSearch = search;
	mExtraList = list;
}

bool ftController::getFileDownloadChunksDetails(const std::string& hash,FileChunksInfo& info)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	std::map<std::string, ftFileControl*>::iterator it = mDownloads.find(hash) ;

	if(it != mDownloads.end())
	{
		it->second->mCreator->getChunkMap(info) ;
		info.flags = it->second->mFlags ;

		return true ;
	}

	it = mCompleted.find(hash) ;

	if(it != mCompleted.end())
	{
		// This should rather be done as a static method of ChunkMap.
		//
		info.file_size = it->second->mSize ;
		info.chunk_size = ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE ;
		info.flags = it->second->mFlags ;
		uint32_t nb_chunks = it->second->mSize/ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE ;
		if(it->second->mSize % ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE != 0)
			++nb_chunks ;
		info.chunks.resize(nb_chunks,FileChunksInfo::CHUNK_DONE) ;

		return true ;
	}

	return false ;
}

void ftController::addFileSource(const std::string& hash,const std::string& peer_id)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	std::map<std::string, ftFileControl*>::iterator it;
	std::map<std::string, ftFileControl*> currentDownloads = *(&mDownloads);

#ifdef CONTROL_DEBUG
	std::cerr << "ftController: Adding source " << peer_id << " to current download hash=" << hash ;
#endif
	for(it = currentDownloads.begin(); it != currentDownloads.end(); it++)
		if(it->first == hash)
		{
			it->second->mTransfer->addFileSource(peer_id);

//			setPeerState(it->second.mTransfer, peer_id, rate, mConnMgr->isOnline(peer_id));

#ifdef CONTROL_DEBUG
			std::cerr << "... added." << std::endl ;
#endif
			return ;
		}
#ifdef CONTROL_DEBUG
	std::cerr << "... not added: hash not found." << std::endl ;
#endif
}
void ftController::removeFileSource(const std::string& hash,const std::string& peer_id)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	std::map<std::string, ftFileControl*>::iterator it;
	std::map<std::string, ftFileControl*> currentDownloads = *(&mDownloads);

#ifdef CONTROL_DEBUG
	std::cerr << "ftController: Adding source " << peer_id << " to current download hash=" << hash ;
#endif
	for(it = currentDownloads.begin(); it != currentDownloads.end(); it++)
		if(it->first == hash)
		{
			it->second->mTransfer->removeFileSource(peer_id);
			it->second->mCreator->removeFileSource(peer_id);

//			setPeerState(it->second.mTransfer, peer_id, rate, mConnMgr->isOnline(peer_id));

#ifdef CONTROL_DEBUG
			std::cerr << "... added." << std::endl ;
#endif
			return ;
		}
#ifdef CONTROL_DEBUG
	std::cerr << "... not added: hash not found." << std::endl ;
#endif
}
void ftController::run()
{

	/* check the queues */
	uint32_t cnt = 0 ;

	while(1)
	{
#ifdef WIN32
		Sleep(1000);
#else
		sleep(1);
#endif

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
		if((int)now - (int)last_save_time > (int)SAVE_TRANSFERS_DELAY)
		{
			cleanCacheDownloads() ;

			IndicateConfigChanged() ;
			last_save_time = now ;
		}

		if((int)now - (int)last_clean_time > (int)INACTIVE_CHUNKS_CHECK_DELAY)
		{
			RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		  for(std::map<std::string,ftFileControl*>::iterator it(mDownloads.begin());it!=mDownloads.end();++it)
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
			RsStackMutex stack2(doneMutex);

			for(std::list<std::string>::iterator it(mDone.begin()); it != mDone.end(); it++)
				completeFile(*it);
			
			mDone.clear();
		}

		if(cnt++ % 10 == 0)
			checkDownloadQueue() ;
	}

}

void ftController::tickTransfers()
{
	// 1 - sort modules into arrays according to priority
	
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

#ifdef CONTROL_DEBUG
	std::cerr << "ticking transfers." << std::endl ;
#endif
	std::vector<std::vector<ftTransferModule*> >priority_tab (3,std::vector<ftTransferModule*>()) ;

	// Collect all non queued files.
	//
	for(std::map<std::string,ftFileControl*>::iterator it(mDownloads.begin()); it != mDownloads.end(); it++)
		if(it->second->mState != ftFileControl::QUEUED && it->second->mState != ftFileControl::PAUSED)
			priority_tab[it->second->mPriority].push_back(it->second->mTransfer) ;

	// 2 - tick arrays with a probability proportional to priority
	
	// 2.1 - decide based on probability, which category of files we handle.
	
#ifdef CONTROL_DEBUG
	std::cerr << "Priority tabs: " ;
	std::cerr << "Low ("    << priority_tab[SPEED_LOW   ].size() << ") " ;
	std::cerr << "Normal (" << priority_tab[SPEED_NORMAL].size() << ") " ;
	std::cerr << "High ("   << priority_tab[SPEED_HIGH  ].size() << ") " ;
	std::cerr << std::endl ;
#endif

	/* tick the transferModules */

	// start anywhere in the chosen list of transfers, so that not to favor any special transfer
	//

	for(int chosen=2;chosen>=0;--chosen)
		if(!priority_tab[chosen].empty())
		{
			int start = rand() % priority_tab[chosen].size() ;

			for(int i=0;i<(int)priority_tab[chosen].size();++i)
				priority_tab[chosen][(i+start)%(int)priority_tab[chosen].size()]->tick() ;
		}
}

bool ftController::getPriority(const std::string& hash,DwlSpeed& p)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	std::map<std::string,ftFileControl*>::iterator it(mDownloads.find(hash)) ;

	if(it != mDownloads.end())
	{
		p = it->second->mPriority ;
		return true ;
	}
	else
		return false ;
}
void ftController::setPriority(const std::string& hash,DwlSpeed p)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	std::map<std::string,ftFileControl*>::iterator it(mDownloads.find(hash)) ;

	if(it != mDownloads.end())
		it->second->mPriority = p ;
}

void ftController::cleanCacheDownloads()
{
	std::vector<std::string> toCancel ;

	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		for(std::map<std::string,ftFileControl*>::iterator it(mDownloads.begin());it!=mDownloads.end();++it)
			if ((it->second)->mFlags & RS_FILE_HINTS_CACHE) //check if a cache file is downloaded, if the case, timeout the transfer after TIMOUT_CACHE_FILE_TRANSFER
			{
#ifdef CONTROL_DEBUG
				std::cerr << "ftController::run() cache transfer found. age of this tranfer is :" << (int)(time(NULL) - (it->second)->mCreateTime);
				std::cerr << std::endl;
#endif
				if ((time(NULL) - (it->second)->mCreateTime) > TIMOUT_CACHE_FILE_TRANSFER) 
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

	for(std::map<std::string,ftFileControl*>::const_iterator it(mDownloads.begin());it!=mDownloads.end() && nb_moved <= _max_active_downloads;++it)
		if(	it->second->mState != ftFileControl::QUEUED 
			&& it->second->mState != ftFileControl::PAUSED 
			&& now > it->second->mCreator->lastRecvTimeStamp() + (time_t)MAX_TIME_INACTIVE_REQUEUED)
		{
#ifdef DEBUG_DWLQUEUE
			std::cerr << "  - Inactive file " << it->second->mName << " at position " << it->second->mQueuePosition << " moved to end of the queue. mState=" << it->second->mState << ", time lapse=" << now - it->second->mCreator->lastRecvTimeStamp()  << std::endl ;
#endif
			locked_bottomQueue(it->second->mQueuePosition) ;
#ifdef DEBUG_DWLQUEUE
			std::cerr << "  new position: " << it->second->mQueuePosition << std::endl ;
			std::cerr << "  new state: " << it->second->mState << std::endl ;
#endif
			it->second->mCreator->resetRecvTimeStamp() ;	// very important!
			++nb_moved ;
		}
}

void ftController::locked_addToQueue(ftFileControl* ftfc)
{
#ifdef DEBUG_DWLQUEUE
	std::cerr << "Queueing ftfileControl " << (void*)ftfc << ", name=" << ftfc->mName << std::endl ;
#endif
	_queue.push_back(ftfc) ;
	locked_checkQueueElement(_queue.size()-1) ;
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

void ftController::moveInQueue(const std::string& hash,QueueMove mv)
{
	RsStackMutex mtx(ctrlMutex) ;

	std::map<std::string,ftFileControl*>::iterator it(mDownloads.find(hash)) ;

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

	if(pos < _max_active_downloads)
	{
		_queue[pos]->mState = ftFileControl::DOWNLOADING ;

		if(_queue[pos]->mFlags & RS_FILE_HINTS_NETWORK_WIDE)
			mTurtle->monitorFileTunnels(_queue[pos]->mName,_queue[pos]->mHash,_queue[pos]->mSize) ;
	}

	if(pos >= _max_active_downloads && _queue[pos]->mState != ftFileControl::QUEUED && _queue[pos]->mState != ftFileControl::PAUSED)
	{
		_queue[pos]->mState = ftFileControl::QUEUED ;
		_queue[pos]->mCreator->closeFile() ;

		if(_queue[pos]->mFlags & RS_FILE_HINTS_NETWORK_WIDE)
			mTurtle->stopMonitoringFileTunnels(_queue[pos]->mHash) ;
	}
}

bool ftController::FlagFileComplete(std::string hash)
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
	if (0 == rename(source.c_str(), dest.c_str()))
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

	std::string error ;

	static const int BUFF_SIZE = 10485760 ; // 10 MB buffer to speed things up.
	void *buffer = malloc(BUFF_SIZE) ;
	FILE *in = fopen(source.c_str(),"rb") ;

	if(in == NULL)
	{
		getPqiNotify()->AddSysMessage(0, RS_SYS_WARNING, "File copy error", "Error while copying file " + dest + "\nCannot open input file "+source);
		return false ;
	}

	FILE *out = fopen(dest.c_str(),"wb") ;

	if(out == NULL)
	{
		getPqiNotify()->AddSysMessage(0, RS_SYS_WARNING, "File copy error", "Error while copying file " + dest + "\nCheck for disk full, or write permission ?\nOriginal file kept under the name "+source);
		return false ;
	}

	size_t s=0;
	size_t T=0;

	while( (s = fread(buffer,1,BUFF_SIZE,in)) > 0)
	{
		size_t t = fwrite(buffer,1,s,out) ;
		T += t ;

		if(t != s)
		{
			getPqiNotify()->AddSysMessage(0, RS_SYS_WARNING, "File copy error", "Error while copying file " + dest + "\nIs your disc full ?\nOriginal file kept under the name "+source);
			return false ;
		}
	}

	fclose(in) ;
	fclose(out) ;

	// copy was successfull, let's delete the original
	std::cerr << "deleting original file " << source << std::endl ;

	free(buffer) ;

	if(0 == remove(source.c_str()))
		return true ;
	else
	{
		getPqiNotify()->AddSysMessage(0, RS_SYS_WARNING, "File erase error", "Error while removing hash file " + dest + "\nRead-only file system ?");
		return false ;
	}
}


bool ftController::completeFile(std::string hash)
{
	/* variables... so we can drop mutex later */
	std::string path;
	uint64_t    size = 0;
	uint32_t    state = 0;
	uint32_t    period = 0;
	uint32_t    flags = 0;

	bool doCallback = false;
	uint32_t callbackCode = 0;


	{
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

#ifdef CONTROL_DEBUG
		std::cerr << "ftController:completeFile(" << hash << ")";
		std::cerr << std::endl;
#endif
		std::map<std::string, ftFileControl*>::iterator it(mDownloads.find(hash));

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
		std::string hash_to_suppress(fc->mTransfer->hash());

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
		//hash    = fc->mHash;
		size    = fc->mSize;
		state   = fc->mState;
		period  = 30 * 24 * 3600; /* 30 days */
		flags   = 0;

#ifdef CONTROL_DEBUG
		std::cerr << "CompleteFile(): size = " << size << std::endl ;
#endif

		doCallback = fc->mDoCallback;
		callbackCode = fc->mCallbackCode;

		mDataplex->removeTransferModule(hash_to_suppress) ;
		uint32_t flgs = fc->mFlags ;

		locked_queueRemove(it->second->mQueuePosition) ;

		/* switch map */
		if (!(fc->mFlags & RS_FILE_HINTS_CACHE)) /* clean up completed cache files automatically */
			mCompleted[fc->mHash] = fc;
		else
			delete fc ;

		mDownloads.erase(it);

		if(flgs & RS_FILE_HINTS_NETWORK_WIDE)
			mTurtle->stopMonitoringFileTunnels(hash_to_suppress) ;

	} /******* UNLOCKED ********/


	/******************** NO Mutex from Now ********************
	 * cos Callback can end up back in this class.
	 ***********************************************************/

	/* If it has a callback - do it now */
	if (doCallback)
	{
#ifdef CONTROL_DEBUG
	  std::cerr << "ftController::completeFile() doing Callback, callbackCode:" << callbackCode;
	  std::cerr << std::endl;
#endif
	  switch (callbackCode)
	  {
	    case CB_CODE_CACHE:
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
		break;
	    case CB_CODE_EXTRA:
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::completeFile() adding to ExtraList";
		std::cerr << std::endl;
#endif

		mExtraList->addExtraFile(path, hash, size, period, flags);


		break;
	    case CB_CODE_MEDIA:
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::completeFile() NULL MEDIA callback";
		std::cerr << std::endl;
#endif
		break;
	  }
	}
	else
	{
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::completeFile() No callback";
		std::cerr << std::endl;
#endif


	}

	IndicateConfigChanged(); /* completed transfer -> save */
	return true;

}

	/***************************************************************/
	/********************** Controller Access **********************/
	/***************************************************************/

const uint32_t FT_CNTRL_STANDARD_RATE = 1024 * 1024;
const uint32_t FT_CNTRL_SLOW_RATE     = 10   * 1024;

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

	FileRequest(req.mName, req.mHash, req.mSize, req.mDest, req.mFlags, req.mSrcIds);

	{ 
		// See whether there is a pendign chunk map recorded for this hash.
		//
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		std::map<std::string,RsFileTransfer*>::iterator it(mPendingChunkMaps.find(req.mHash)) ;

		if(it != mPendingChunkMaps.end())
		{
			RsFileTransfer *rsft = it->second ;
			std::map<std::string, ftFileControl*>::iterator fit = mDownloads.find(rsft->file.hash);

			if((fit==mDownloads.end() || (fit->second)->mCreator == NULL))
			{
				// This should never happen, because the last call to FileRequest must have created the fileCreator!!
				//
				std::cerr << "ftController::loadList(): Error: could not find hash " << rsft->file.hash << " in mDownloads list !" << std::endl ;
			}
			else
			{
				(fit->second)->mCreator->setAvailabilityMap(rsft->compressed_chunk_map) ;
				(fit->second)->mCreator->setChunkStrategy((FileChunksInfo::ChunkStrategy)(rsft->chunk_strategy)) ;
			}

			delete rsft ;
			mPendingChunkMaps.erase(it) ;
		}
	}

	return true ;
}

bool ftController::alreadyHaveFile(const std::string& hash)
{
	FileInfo info ;

	// check for downloads
	if(FileDetails(hash, info))
		return true ;

	// check for file lists
	if (mSearch->search(hash, RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_SPEC_ONLY, info))
		return true ;
	
	return false ;
}

bool 	ftController::FileRequest(std::string fname, std::string hash,
			uint64_t size, std::string dest, uint32_t flags,
			std::list<std::string> &srcIds)
{
	/* check if we have the file */

	if(alreadyHaveFile(hash))
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
		FILE *f = fopen(destination.c_str(),"w") ;

		if(f == NULL)
			std::cerr << "Could not open file " << destination << " for writting." << std::endl ;
		else
			fclose(f) ;

		return true ;
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
			ftPendingRequest req(fname, hash, size, dest, flags, srcIds);
			mPendingRequests.push_back(req);
			return true;
		}
	}

	FileInfo info;
	std::list<std::string>::iterator it;
	std::list<TransferInfo>::iterator pit;

#ifdef CONTROL_DEBUG
	std::cerr << "ftController::FileRequest(" << fname << ",";
	std::cerr << hash << "," << size << "," << dest << ",";
	std::cerr << flags << ",<";

	for(it = srcIds.begin(); it != srcIds.end(); it++)
	{
		std::cerr << *it << ",";
	}
	std::cerr << ">)";
	std::cerr << std::endl;
#endif

	std::string ownId = mConnMgr->getOwnId();
	uint32_t rate = 0;
	if (flags & RS_FILE_HINTS_BACKGROUND)
		rate = FT_CNTRL_SLOW_RATE;
	else
		rate = FT_CNTRL_STANDARD_RATE;

	/* First check if the file is already being downloaded....
	 * This is important as some guis request duplicate files regularly.
	 */

	{ 
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		std::map<std::string, ftFileControl*>::iterator dit = mDownloads.find(hash);

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

			for(it = srcIds.begin(); it != srcIds.end(); it++)
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
				setPeerState(dit->second->mTransfer, *it, rate, mConnMgr->isOnline(*it));

				IndicateConfigChanged(); /* new peer for transfer -> save */
			}

			if (srcIds.size() == 0)
			{
#ifdef CONTROL_DEBUG
				std::cerr << "ftController::FileRequest() WARNING: No Src Peers";
				std::cerr << std::endl;
#endif
			}

			return true;
		}
	} /******* UNLOCKED ********/

	bool doCallback = false;
	uint32_t callbackCode = 0;
	if (flags & RS_FILE_HINTS_NO_SEARCH)
	{
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::FileRequest() Flags for NO_SEARCH ";
		std::cerr << std::endl;
#endif
		/* no search */
		if (flags & RS_FILE_HINTS_CACHE)
		{
			doCallback = true;
			callbackCode = CB_CODE_CACHE;
		}
		else if (flags & RS_FILE_HINTS_EXTRA)
		{
			doCallback = true;
			callbackCode = CB_CODE_EXTRA;
		}
	}
	else
	{
		/* do a source search - for any extra sources */
		// add sources only in direct mode
		//
		if ((!(flags & RS_FILE_HINTS_NETWORK_WIDE)) && mSearch->search(hash, RS_FILE_HINTS_REMOTE | RS_FILE_HINTS_SPEC_ONLY, info))
		{
			/* do something with results */
#ifdef CONTROL_DEBUG
			std::cerr << "ftController::FileRequest() Found Other Sources";
			std::cerr << std::endl;
#endif

			/* if the sources don't exist already - add in */
			for(pit = info.peers.begin(); pit != info.peers.end(); pit++)
			{
#ifdef CONTROL_DEBUG
                                std::cerr << "\tSource: " << pit->peerId;
				std::cerr << std::endl;
#endif

				if (srcIds.end() == std::find( srcIds.begin(), srcIds.end(), pit->peerId))
				{
					srcIds.push_back(pit->peerId);

#ifdef CONTROL_DEBUG
                                        std::cerr << "\tAdding in: " << pit->peerId;
					std::cerr << std::endl;
#endif
                                }
			}
		}

		if (flags & RS_FILE_HINTS_EXTRA)
		{
			doCallback = true;
			callbackCode = CB_CODE_EXTRA;
		}
		else if (flags & RS_FILE_HINTS_MEDIA)
		{
			doCallback = true;
			callbackCode = CB_CODE_MEDIA;
		}
	}

	//std::map<std::string, ftTransferModule *> mTransfers;
	//std::map<std::string, ftFileCreator *> mFileCreators;

	/* add in new item for download */
	std::string savepath;
	std::string destination;

	{ 
		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		savepath = mPartialsPath + "/" + hash;
		destination = dest + "/" + fname;

		/* if no destpath - send to download directory */
		if (dest == "")
			destination = mDownloadPath + "/" + fname;
	} /******* UNLOCKED ********/

  // We check that flags are consistent.  In particular, for know
  // we can't send chunkmaps through normal traffic, so availability must be assumed whenever the traffic is not
  // a turtle traffic:
  
  	if(flags & RS_FILE_HINTS_NETWORK_WIDE)
		mTurtle->monitorFileTunnels(fname,hash,size) ;

	ftFileCreator *fc = new ftFileCreator(savepath, size, hash);
	ftTransferModule *tm = new ftTransferModule(fc, mDataplex,this);

	fc->setChunkStrategy(mDefaultChunkStrategy) ;

	/* add into maps */
	ftFileControl *ftfc = new ftFileControl(fname, savepath, destination, size, hash, flags, fc, tm, callbackCode);
	ftfc->mCreateTime = time(NULL);

	locked_addToQueue(ftfc) ;

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

	/* now add source peers (and their current state) */
	tm->setFileSources(srcIds);

	/* get current state for transfer module */
	for(it = srcIds.begin(); it != srcIds.end(); it++)
	{
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::FileRequest() adding peer: " << *it;
		std::cerr << std::endl;
#endif
		setPeerState(tm, *it, rate, mConnMgr->isOnline(*it));
	}


	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	mDownloads[hash] = ftfc;

	IndicateConfigChanged(); /* completed transfer -> save */
	return true;
}

bool 	ftController::setPeerState(ftTransferModule *tm, std::string id, uint32_t maxrate, bool online)
{
	if (id == mConnMgr->getOwnId())
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


bool ftController::setChunkStrategy(const std::string& hash,FileChunksInfo::ChunkStrategy s)
{
	std::map<std::string,ftFileControl*>::iterator mit=mDownloads.find(hash);
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

bool 	ftController::FileCancel(std::string hash)
{
	rsTurtle->stopMonitoringFileTunnels(hash) ;

#ifdef CONTROL_DEBUG
	std::cerr << "ftController::FileCancel" << std::endl;
#endif
	/*check if the file in the download map*/

	{
		RsStackMutex mtx(ctrlMutex) ;

		std::map<std::string,ftFileControl*>::iterator mit=mDownloads.find(hash);
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

bool 	ftController::FileControl(std::string hash, uint32_t flags)
{
//#ifdef CONTROL_DEBUG
	std::cerr << "ftController::FileControl(" << hash << ",";
	std::cerr << flags << ")"<<std::endl;
//#endif
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	/*check if the file in the download map*/
	std::map<std::string,ftFileControl*>::iterator mit=mDownloads.find(hash);
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
		default:
			return false;
	}
	return true;
}

bool 	ftController::FileClearCompleted()
{
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::FileClearCompleted()" <<std::endl;
#endif
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	for(std::map<std::string, ftFileControl*>::iterator it(mCompleted.begin());it!=mCompleted.end();++it)
		delete it->second ;

	mCompleted.clear();

	IndicateConfigChanged();

	return false;
}

	/* get Details of File Transfers */
bool 	ftController::FileDownloads(std::list<std::string> &hashs)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	std::map<std::string, ftFileControl*>::iterator it;
	for(it = mDownloads.begin(); it != mDownloads.end(); it++)
	{
		hashs.push_back(it->second->mHash);
	}
	for(it = mCompleted.begin(); it != mCompleted.end(); it++)
	{
		hashs.push_back(it->second->mHash);
	}
	return true;
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

		mDownloadPath = path;
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

        if (!path.find(mDownloadPath)) {
            return false;
        }

        if (rsFiles) {
            std::list<SharedDirInfo>::iterator it;
            std::list<SharedDirInfo> dirs;
            rsFiles->getSharedDirectories(dirs);
            for (it = dirs.begin(); it != dirs.end(); it++) {
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

#if 0 /*** FIX ME !!!**************/
		/* move all existing files! */
		std::map<std::string, ftFileControl>::iterator it;
		for(it = mDownloads.begin(); it != mDownloads.end(); it++)
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

bool 	ftController::FileDetails(std::string hash, FileInfo &info)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

	bool completed = false;
	std::map<std::string, ftFileControl*>::iterator it = mDownloads.find(hash);
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
	info.flags = it->second->mFlags;
	info.path = RsDirUtil::removeTopDir(it->second->mDestination); /* remove fname */
	info.priority = it->second->mPriority ;
	info.queue_position = it->second->mQueuePosition ;

	/* get list of sources from transferModule */
	std::list<std::string> peerIds;
	std::list<std::string>::iterator pit;

	if (!completed)
		it->second->mTransfer->getFileSources(peerIds);

	double totalRate = 0;
	uint32_t tfRate = 0;
	uint32_t state = 0;

	bool isDownloading = false;
	bool isSuspended = false;

	for(pit = peerIds.begin(); pit != peerIds.end(); pit++)
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
void    ftController::statusChange(const std::list<pqipeer> &plist)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	uint32_t rate = FT_CNTRL_STANDARD_RATE;

	/* add online to all downloads */
	std::map<std::string, ftFileControl*>::iterator it;
	std::list<pqipeer>::const_iterator pit;

	std::list<pqipeer> vlist ;
	mTurtle->getVirtualPeersList(vlist) ;

#ifdef CONTROL_DEBUG
	std::cerr << "ftController::statusChange()";
	std::cerr << std::endl;
#endif

	for(it = mDownloads.begin(); it != mDownloads.end(); it++)
	{
#ifdef CONTROL_DEBUG
		std::cerr << "ftController::statusChange() Updating Hash:";
		std::cerr << it->first;
		std::cerr << std::endl;
#endif
		for(pit = plist.begin(); pit != plist.end(); pit++)
		{
#ifdef CONTROL_DEBUG
			std::cerr << "Peer: " << pit->id;
#endif
			if (pit->actions & RS_PEER_CONNECTED)
			{
#ifdef CONTROL_DEBUG
				std::cerr << " is Newly Connected!";
				std::cerr << std::endl;
#endif
				setPeerState(it->second->mTransfer, pit->id, rate, true);
			}
			else if (pit->actions & RS_PEER_DISCONNECTED)
			{
#ifdef CONTROL_DEBUG
				std::cerr << " is Just disconnected!";
				std::cerr << std::endl;
#endif
				setPeerState(it->second->mTransfer, pit->id, rate, false);
			}
			else
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
		for(pit = vlist.begin(); pit != vlist.end(); pit++)
		{
#ifdef CONTROL_DEBUG
			std::cerr << "Peer: " << pit->id;
#endif
			if (pit->actions & RS_PEER_CONNECTED)
			{
#ifdef CONTROL_DEBUG
				std::cerr << " is Newly Connected!";
				std::cerr << std::endl;
#endif
				setPeerState(it->second->mTransfer, pit->id, rate, true);
			}
			else if (pit->actions & RS_PEER_DISCONNECTED)
			{
#ifdef CONTROL_DEBUG
				std::cerr << " is Just disconnected!";
				std::cerr << std::endl;
#endif
				setPeerState(it->second->mTransfer, pit->id, rate, false);
			}
			else
			{
#ifdef CONTROL_DEBUG
				std::cerr << " had something happen to it: ";
				std::cerr << pit-> actions;
				std::cerr << std::endl;
#endif
				setPeerState(it->second->mTransfer, pit->id, rate, false);
			}
		}
	}
}

	/* Cache Interface */
bool ftController::RequestCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size)
{
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::RequestCacheFile(" << id << ",";
	std::cerr << path << "," << hash << "," << size << ")";
	std::cerr << std::endl;
#endif

	/* Request File */
	std::list<std::string> ids;
	ids.push_back(id);

	FileRequest(hash, hash, size, path, RS_FILE_HINTS_CACHE | RS_FILE_HINTS_NO_SEARCH, ids);

	return true;
}


bool ftController::CancelCacheFile(RsPeerId id, std::string path, std::string hash, uint64_t size)
{
#ifdef CONTROL_DEBUG
	std::cerr << "ftController::CancelCacheFile(" << id << ",";
	std::cerr << path << "," << hash << "," << size << ")";
	std::cerr << std::endl;
#endif

	return true;
}

const std::string download_dir_ss("DOWN_DIR");
const std::string partial_dir_ss("PART_DIR");
const std::string share_dwl_dir("SHARE_DWL_DIR");
const std::string default_chunk_strategy_ss("DEFAULT_CHUNK_STRATEGY");


	/* p3Config Interface */
RsSerialiser *ftController::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();

	/* add in the types we need! */
	rss->addSerialType(new RsFileConfigSerialiser());
	rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss;
}


std::list<RsItem *> ftController::saveList(bool &cleanup)
{
	std::list<RsItem *> saveData;

	/* it can delete them! */
	cleanup = true;

	/* create a key/value set for most of the parameters */
	std::map<std::string, std::string> configMap;
	std::map<std::string, std::string>::iterator mit;
	std::list<std::string>::iterator it;

	/* basic control parameters */
	configMap[download_dir_ss] = getDownloadDirectory();
	configMap[partial_dir_ss] = getPartialsDirectory();
	configMap[share_dwl_dir] = mShareDownloadDir ? "YES" : "NO";
	configMap[default_chunk_strategy_ss] = (mDefaultChunkStrategy==FileChunksInfo::CHUNK_STRATEGY_STREAMING) ? "STREAMING" : "RANDOM";

	RsConfigKeyValueSet *rskv = new RsConfigKeyValueSet();

	/* Convert to TLV */
	for(mit = configMap.begin(); mit != configMap.end(); mit++)
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
	std::list<std::string> hashs;
	FileDownloads(hashs);

	for(it = hashs.begin(); it != hashs.end(); it++)
	{
		/* stack mutex released each loop */
  		RsStackMutex stack(ctrlMutex); /******* LOCKED ********/

		std::map<std::string, ftFileControl*>::iterator fit = mDownloads.find(*it);
		if (fit == mDownloads.end())
			continue;

		/* ignore callback ones */
		if (fit->second->mDoCallback)
			continue;

		if ((fit->second)->mCreator->finished())
			continue;

		/* make RsFileTransfer item for save list */
		RsFileTransfer *rft = new RsFileTransfer();

		/* what data is important? */

		rft->file.name = fit->second->mName;
		rft->file.hash  = fit->second->mHash;
		rft->file.filesize = fit->second->mSize;
		rft->file.path = RsDirUtil::removeTopDir(fit->second->mDestination); /* remove fname */
		rft->flags = fit->second->mFlags;
		rft->state = fit->second->mState;
		fit->second->mTransfer->getFileSources(rft->allPeerIds.ids);

		// just avoid uninitialised memory reads
		rft->in = 0 ;
		rft->cPeerId = "" ;
		rft->transferred = fit->second->mCreator->getRecvd();
		rft->crate = 0 ;
		rft->lrate = 0 ;
		rft->trate = 0 ;
		rft->ltransfer = 0 ;

		// Remove turtle peers from sources, as they are not supposed to survive a reboot of RS, since they are dynamic sources.
		// Otherwize, such sources are unknown from the turtle router, at restart, and never get removed.
		//
		for(std::list<std::string>::iterator sit(rft->allPeerIds.ids.begin());sit!=rft->allPeerIds.ids.end();)
			if(mTurtle->isTurtlePeer(*sit))
			{
				std::list<std::string>::iterator sittmp(sit) ;
				++sittmp ;
				rft->allPeerIds.ids.erase(sit) ;
				sit = sittmp ;
			}
			else
				++sit ;

		fit->second->mCreator->getAvailabilityMap(rft->compressed_chunk_map) ;
		rft->chunk_strategy = fit->second->mCreator->getChunkStrategy() ;

		saveData.push_back(rft);
	}

	/* list completed! */
	return saveData;
}


bool ftController::loadList(std::list<RsItem *> load)
{
	std::list<RsItem *>::iterator it;
	std::list<RsTlvKeyValue>::iterator kit;
	RsConfigKeyValueSet *rskv;
	RsFileTransfer      *rsft;

#ifdef CONTROL_DEBUG
	std::cerr << "ftController::loadList() Item Count: " << load.size();
	std::cerr << std::endl;
#endif

	for(it = load.begin(); it != load.end(); it++)
	{
		/* switch on type */
		if (NULL != (rskv = dynamic_cast<RsConfigKeyValueSet *>(*it)))
		{
			/* make into map */
			std::map<std::string, std::string> configMap;
			for(kit = rskv->tlvkvs.pairs.begin();
				kit != rskv->tlvkvs.pairs.end(); kit++)
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
			FileRequest(rsft->file.name, rsft->file.hash, rsft->file.filesize, rsft->file.path, rsft->flags, rsft->allPeerIds.ids);

			{
				RsStackMutex mtx(ctrlMutex) ;

				std::map<std::string, ftFileControl*>::iterator fit = mDownloads.find(rsft->file.hash);

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
	return true;

}

bool  ftController::loadConfigMap(std::map<std::string, std::string> &configMap)
{
	std::map<std::string, std::string>::iterator mit;

	std::string str_true("true");
	std::string empty("");
	std::string dir = "notempty";

	if (configMap.end() != (mit = configMap.find(download_dir_ss)))
	{
		setDownloadDirectory(mit->second);
	}

	if (configMap.end() != (mit = configMap.find(partial_dir_ss)))
	{
		setPartialsDirectory(mit->second);
	}

	if (configMap.end() != (mit = configMap.find(share_dwl_dir)))
	{
		if (mit->second == "YES")
		{
			setShareDownloadDirectory(true);
		}
		else if (mit->second == "NO")
		{
			setShareDownloadDirectory(false);
		}
	}

	if (configMap.end() != (mit = configMap.find(default_chunk_strategy_ss)))
	{
		if(mit->second == "STREAMING")
			setDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_STREAMING) ;
		else if(mit->second == "RANDOM")
			setDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_RANDOM) ;
	}

	return true;
}

FileChunksInfo::ChunkStrategy ftController::defaultChunkStrategy()
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	return mDefaultChunkStrategy ;
}
void ftController::setDefaultChunkStrategy(FileChunksInfo::ChunkStrategy S)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	mDefaultChunkStrategy = S ;
	IndicateConfigChanged() ;
}

void ftController::setShareDownloadDirectory(bool value)
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	mShareDownloadDir = value;
	IndicateConfigChanged() ;
}

bool ftController::getShareDownloadDirectory()
{
	RsStackMutex stack(ctrlMutex); /******* LOCKED ********/
	return mShareDownloadDir;
}

