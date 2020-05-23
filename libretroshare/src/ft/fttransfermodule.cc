/*******************************************************************************
 * libretroshare/src/ft: fttransfermodule.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008 by Robert Fernie <retroshare@lunamutt.com>                   *
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
/******
 * #define FT_DEBUG 1
 *****/

#include "util/rstime.h"

#include "retroshare/rsturtle.h"
#include "fttransfermodule.h"

/*************************************************************************
 * Notes on file transfer strategy.
 * Care must be taken not to overload pipe. best way is to time requests.
 * and according adjust data rate.
 *
 * each peer gets a 'max_rate' which is decided on the type of transfer.
 *  - trickle ...
 *  - stream ...
 *  - max ...
 *
 * Each peer is independently managed.
 *
 * via the functions:
 *
 */

const double   FT_TM_MAX_PEER_RATE 		       = 100 * 1024 * 1024; /* 100MB/s */
const uint32_t FT_TM_MAX_RESETS  		       = 5;
const uint32_t FT_TM_MINIMUM_CHUNK 		       = 1024;              /* ie 1Kb / sec */
const uint32_t FT_TM_DEFAULT_TRANSFER_RATE     = 20*1024;           /* ie 20 Kb/sec */
const uint32_t FT_TM_RESTART_DOWNLOAD 	       = 20;                /* 20 seconds */
const uint32_t FT_TM_DOWNLOAD_TIMEOUT 	       = 10;                /* 10 seconds */

const double FT_TM_RATE_INCREASE_SLOWER  = 0.05 ;
const double FT_TM_RATE_INCREASE_AVERAGE = 0.3 ;
const double FT_TM_RATE_INCREASE_FASTER  = 1.0 ;

#define FT_TM_FLAG_DOWNLOADING 	0
#define FT_TM_FLAG_CANCELED		1
#define FT_TM_FLAG_COMPLETE 		2
#define FT_TM_FLAG_CHECKING 		3
#define FT_TM_FLAG_CHUNK_CRC 		4

peerInfo::peerInfo(const RsPeerId& peerId_in)
    :peerId(peerId_in),state(PQIPEER_NOT_ONLINE),desiredRate(FT_TM_DEFAULT_TRANSFER_RATE),actualRate(FT_TM_DEFAULT_TRANSFER_RATE),
		lastTS(0),
		recvTS(0), lastTransfers(0), nResets(0),
		rtt(0), rttActive(false), rttStart(0), rttOffset(0),
		mRateIncrease(1)
	{
	}
//	peerInfo(const RsPeerId& peerId_in,uint32_t state_in,uint32_t maxRate_in):
//		peerId(peerId_in),state(state_in),desiredRate(maxRate_in),actualRate(0),
//		lastTS(0),
//		recvTS(0), lastTransfers(0), nResets(0),
//		rtt(0), rttActive(false), rttStart(0), rttOffset(0),
//		mRateIncrease(1)
//	{
//		return;
//	}
ftTransferModule::ftTransferModule(ftFileCreator *fc, ftDataMultiplex *dm, ftController *c)
	:mFileCreator(fc), mMultiplexor(dm), mFtController(c), tfMtx("ftTransferModule"), mFlag(FT_TM_FLAG_DOWNLOADING),mPriority(SPEED_NORMAL)
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/

	mHash = mFileCreator->getHash();
	mSize = mFileCreator->getFileSize();
	mFileStatus.hash = mHash;

	_hash_thread = NULL ;

	// Dummy for Testing (should be handled independantly for 
	// each peer.
	//mChunkSize = 10000;
	desiredRate = FT_TM_MAX_PEER_RATE; /* 1MB/s ??? */
	actualRate = 0;

	_last_activity_time_stamp = time(NULL) ;
}

ftTransferModule::~ftTransferModule()
{
	// Prevents deletion while called from another thread.
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
}


bool ftTransferModule::setFileSources(const std::list<RsPeerId>& peerIds)
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/

  mFileSources.clear();

#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::setFileSources()";
	std::cerr << " List of peers: " ;
#endif

  std::list<RsPeerId>::const_iterator it;
  for(it = peerIds.begin(); it != peerIds.end(); ++it)
  {

#ifdef FT_DEBUG
	std::cerr << " \t" << *it;
#endif

    peerInfo pInfo(*it);
    mFileSources.insert(std::pair<RsPeerId,peerInfo>(*it,pInfo));
  }

#ifdef FT_DEBUG
	std::cerr << std::endl;
#endif

  return true;
}

bool ftTransferModule::getFileSources(std::list<RsPeerId> &peerIds)
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
    std::map<RsPeerId,peerInfo>::iterator it;
    for(it = mFileSources.begin(); it != mFileSources.end(); ++it)
    {
	peerIds.push_back(it->first);
    }
    return true;
}

bool ftTransferModule::addFileSource(const RsPeerId& peerId)
{
	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
	std::map<RsPeerId,peerInfo>::iterator mit;
	mit = mFileSources.find(peerId);

	if (mit == mFileSources.end())
	{
		/* add in new source */
		peerInfo pInfo(peerId);
		mFileSources.insert(std::pair<RsPeerId,peerInfo>(peerId,pInfo));
		//mit = mFileSources.find(peerId);

		mMultiplexor->sendChunkMapRequest(peerId, mHash,false) ;
#ifdef FT_DEBUG
		std::cerr << "ftTransferModule::addFileSource()";
		std::cerr << " adding peer: " << peerId << " to sourceList";
		std::cerr << std::endl;
#endif
		return true ;

	}
	else
	{
#ifdef FT_DEBUG
		std::cerr << "ftTransferModule::addFileSource()";
		std::cerr << " peer: " << peerId << " already there";
		std::cerr << std::endl;
#endif
		return false;
	}
}

bool ftTransferModule::removeFileSource(const RsPeerId& peerId)
{
	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
	std::map<RsPeerId,peerInfo>::iterator mit;
	mit = mFileSources.find(peerId);

	if (mit != mFileSources.end())
	{
		/* add in new source */
		mFileSources.erase(mit) ;
#ifdef FT_DEBUG
		std::cerr << "ftTransferModule::addFileSource(): removing peer: " << peerId << " from sourceList" << std::endl;
#endif
	}
#ifdef FT_DEBUG
	else
		std::cerr << "ftTransferModule::addFileSource(): Should remove peer: " << peerId << ", but it's not in the source list. " << std::endl;
#endif

	return true;
}

bool ftTransferModule::setPeerState(const RsPeerId& peerId,uint32_t state,uint32_t maxRate)
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::setPeerState()";
	std::cerr << " peerId: " << peerId;
	std::cerr << " state: " << state;
	std::cerr << " maxRate: " << maxRate << std::endl;
#endif

  std::map<RsPeerId,peerInfo>::iterator mit;
  mit = mFileSources.find(peerId);

  if (mit == mFileSources.end())
  {
  	/* add in new source */

#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::setPeerState()";
	std::cerr << " adding new peer to sourceList";
	std::cerr << std::endl;
#endif
	return false;
  }

  (mit->second).state=state;
  (mit->second).desiredRate=maxRate;
  // Start it off at zero....
  // (mit->second).actualRate=maxRate; /* should give big kick in right direction */

  std::list<RsPeerId>::iterator it;
  it = std::find(mOnlinePeers.begin(), mOnlinePeers.end(), peerId);

  if (state!=PQIPEER_NOT_ONLINE) 
  {
    //change to online, add peerId in online peer list
    if (it==mOnlinePeers.end()) mOnlinePeers.push_back(peerId);
  }
  else
  {
    //change to offline, remove peerId in online peer list
    if (it!=mOnlinePeers.end()) mOnlinePeers.erase(it);
  }

  return true;
}


bool ftTransferModule::getPeerState(const RsPeerId& peerId,uint32_t &state,uint32_t &tfRate)
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
  std::map<RsPeerId,peerInfo>::iterator mit;
  mit = mFileSources.find(peerId);

  if (mit == mFileSources.end()) return false;

  state = (mit->second).state;
  tfRate = (uint32_t) (mit->second).actualRate;

#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::getPeerState()";
	std::cerr << " peerId: " << peerId;
	std::cerr << " state: " << state;
	std::cerr << " tfRate: " << tfRate << std::endl;
#endif
  return true;
}

uint32_t ftTransferModule::getDataRate(const RsPeerId& peerId)
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
  std::map<RsPeerId,peerInfo>::iterator mit;
  mit = mFileSources.find(peerId);
  if (mit == mFileSources.end())
  {
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::getDataRate()";
	std::cerr << " peerId: " << peerId;
	std::cerr << " peer not exist in file sources " << std::endl;
#endif	  
    return 0;
  }
  else
    return (uint32_t) (mit->second).actualRate;
}
void ftTransferModule::resetActvTimeStamp()
{
	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
	_last_activity_time_stamp = time(NULL);
}
rstime_t ftTransferModule::lastActvTimeStamp()
{
	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
	return _last_activity_time_stamp ;
}

  //interface to client module
bool ftTransferModule::recvFileData(const RsPeerId& peerId, uint64_t offset, uint32_t chunk_size, void *data)
{
	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::recvFileData()";
	std::cerr << " peerId: " << peerId;
	std::cerr << " offset: " << offset;
	std::cerr << " chunksize: " << chunk_size;
	std::cerr << " data: " << data;
	std::cerr << std::endl;
#endif

	bool ok = false;

	std::map<RsPeerId,peerInfo>::iterator mit;
	mit = mFileSources.find(peerId);

	if (mit == mFileSources.end())
	{
#ifdef FT_DEBUG
		std::cerr << "ftTransferModule::recvFileData()";
		std::cerr << " peer not found in sources";
		std::cerr << std::endl;
#endif
		return false;
	}
	ok = locked_recvPeerData(mit->second, offset, chunk_size, data);

	locked_storeData(offset, chunk_size, data);

	_last_activity_time_stamp = time(NULL) ;

	free(data) ;
	return ok;
}

void ftTransferModule::locked_requestData(const RsPeerId& peerId, uint64_t offset, uint32_t chunk_size)
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::requestData()";
	std::cerr << " peerId: " << peerId;
	std::cerr << " hash: " << mHash;
	std::cerr << " size: " << mSize;
	std::cerr << " offset: " << offset;
	std::cerr << " chunk_size: " << chunk_size;
	std::cerr << std::endl;
#endif

  mMultiplexor->sendDataRequest(peerId, mHash, mSize, offset,chunk_size);
}

bool ftTransferModule::locked_getChunk(const RsPeerId& peer_id,uint32_t size_hint,uint64_t &offset, uint32_t &chunk_size)
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::locked_getChunk()";
	std::cerr << " hash: " << mHash;
	std::cerr << " size: " << mSize;
	std::cerr << " offset: " << offset;
	std::cerr << " size_hint: " << size_hint;
	std::cerr << " chunk_size: " << chunk_size;
	std::cerr << std::endl;
#endif

	bool source_peer_map_needed ;

  	bool val = mFileCreator->getMissingChunk(peer_id,size_hint,offset, chunk_size,source_peer_map_needed);

	if(source_peer_map_needed)
		mMultiplexor->sendChunkMapRequest(peer_id, mHash,false) ;

#ifdef FT_DEBUG
	if (val)
	{
		std::cerr << "ftTransferModule::locked_getChunk()";
		std::cerr << " Answer: Chunk Available";
	        std::cerr << " hash: " << mHash;
	        std::cerr << " size: " << mSize;
		std::cerr << " offset: " << offset;
		std::cerr << " chunk_size: " << chunk_size;
		std::cerr << " peer map needed = " << source_peer_map_needed << std::endl ;
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "ftTransferModule::locked_getChunk()";
		std::cerr << " Answer: No Chunk Available";
		std::cerr << " peer map needed = " << source_peer_map_needed << std::endl ;
		std::cerr << std::endl;
	}
#endif

	return val;
}

bool ftTransferModule::locked_storeData(uint64_t offset, uint32_t chunk_size,void *data)
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::storeData()";
	std::cerr << " hash: " << mHash;
	std::cerr << " size: " << mSize;
	std::cerr << " offset: " << offset;
	std::cerr << " chunk_size: " << chunk_size;
	std::cerr << std::endl;
#endif

	return mFileCreator -> addFileData(offset, chunk_size, data);
}

bool ftTransferModule::queryInactive()
{
	/* NB: Not sure about this lock... might cause deadlock.
	 */
	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/

#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::queryInactive()" << std::endl;
#endif

	if (mFileStatus.stat == ftFileStatus::PQIFILE_INIT)
		mFileStatus.stat = ftFileStatus::PQIFILE_DOWNLOADING;

	if (mFileStatus.stat != ftFileStatus::PQIFILE_DOWNLOADING)
	{
		if (mFileStatus.stat == ftFileStatus::PQIFILE_FAIL_CANCEL)
			mFlag = FT_TM_FLAG_COMPLETE; //file canceled by user
		return false;
	}

	if (mFileStatus.stat == ftFileStatus::PQIFILE_CHECKING)
		return false ;

	std::map<RsPeerId,peerInfo>::iterator mit;
	for(mit = mFileSources.begin(); mit != mFileSources.end(); ++mit)
	{
		locked_tickPeerTransfer(mit->second);
	}
	if(mFileCreator->finished())	// transfer is complete
	{
		mFileStatus.stat = ftFileStatus::PQIFILE_CHECKING ;
		mFlag = FT_TM_FLAG_CHECKING;      
	}
	else
	{
		// request for CRCs to ask
		std::vector<uint32_t> chunks_to_ask ;

#ifdef FT_DEBUG
		std::cerr << "ftTransferModule::queryInactive() : getting chunks to check." << std::endl;
#endif

		mFileCreator->getChunksToCheck(chunks_to_ask) ;
#ifdef FT_DEBUG
		std::cerr << "ftTransferModule::queryInactive() : got " << chunks_to_ask.size() << " chunks." << std::endl;
#endif

		mMultiplexor->sendSingleChunkCRCRequests(mHash,chunks_to_ask);
	}

	return true; 
}

bool ftTransferModule::cancelTransfer()
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
  mFileStatus.stat=ftFileStatus::PQIFILE_FAIL_CANCEL;

  return 1;
}

bool ftTransferModule::cancelFileTransferUpward()
{
	if (mFtController)
		mFtController->FileCancel(mHash);
	return true;
}
bool ftTransferModule::completeFileTransfer()
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::completeFileTransfer()";
	std::cerr << std::endl;
#endif
	if (mFtController)
		mFtController->FlagFileComplete(mHash);
	return true;
}

int ftTransferModule::tick()
{
  queryInactive();
#ifdef FT_DEBUG
  {
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/

	std::cerr << "ftTransferModule::tick()";
	std::cerr << " mFlag: " << mFlag;
	std::cerr << " mHash: " << mHash;
	std::cerr << " mSize: " << mSize;
	std::cerr << std::endl;

	std::cerr << "Peers: ";
  	std::map<RsPeerId,peerInfo>::iterator it;
  	for(it = mFileSources.begin(); it != mFileSources.end(); ++it)
	{
		std::cerr << " " << it->first;
	}
	std::cerr << std::endl;
		
		
  }
#endif

  uint32_t flags = 0;
  {
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
	flags = mFlag;
  }

  switch (flags)
  {
	  case FT_TM_FLAG_DOWNLOADING: //file transfer not complete
		  adjustSpeed();
		  break;
	  case FT_TM_FLAG_COMPLETE: //file transfer complete
		  completeFileTransfer();
		  break;
	  case FT_TM_FLAG_CANCELED: //file transfer canceled
		  break;
	  case FT_TM_FLAG_CHECKING: // Check if file hash matches the hashed data
		  checkFile() ;
		  break ;
	  default:
		  break;
  }
    
  return 0;
}

bool ftTransferModule::isCheckingHash()
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
#ifdef FT_DEBUG
	std::cerr << "isCheckingHash(): mFlag=" << mFlag << std::endl;
#endif
	return mFlag == FT_TM_FLAG_CHECKING || mFlag == FT_TM_FLAG_CHUNK_CRC;
}

class HashThread: public RsThread
{
	public:
		explicit HashThread(ftFileCreator *m)
			: _hashThreadMtx("HashThread"), _m(m),_finished(false),_hash("") {}

        virtual void run()
		{
#ifdef FT_DEBUG
			std::cerr << "hash thread is running for file " << std::endl;
#endif
			RsFileHash tmphash ;
			_m->hashReceivedData(tmphash) ;

			RsStackMutex stack(_hashThreadMtx) ;
			_hash = tmphash ;
			_finished = true ;
		}
		RsFileHash hash() 
		{
			RsStackMutex stack(_hashThreadMtx) ;
			return _hash ;
		}
		bool finished() 
		{
			RsStackMutex stack(_hashThreadMtx) ;
			return _finished ;
		}
	private:
		RsMutex _hashThreadMtx ;
		ftFileCreator *_m ;
		bool _finished ;
		RsFileHash _hash ;
};

bool ftTransferModule::checkFile()
{
	{
		RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
#ifdef FT_DEBUG
		std::cerr << "ftTransferModule::checkFile(): checking File " << mHash << std::endl ;
#endif

		// if we don't have a hashing thread, create one.

		if(_hash_thread == NULL)
		{
			// Note: using new is really important to avoid copy and write errors in the thread.
			//
			_hash_thread = new HashThread(mFileCreator) ;
			_hash_thread->start("ft hash") ;
#ifdef FT_DEBUG
			std::cerr << "ftTransferModule::checkFile(): launched hashing thread for file " << mHash << std::endl ;
#endif
			return false ;
		}

		if(!_hash_thread->finished())
		{
#ifdef FT_DEBUG
			std::cerr << "ftTransferModule::checkFile(): file " << mHash << " is being hashed.?" << std::endl ;
#endif
			return false ;
		}

		RsFileHash check_hash( _hash_thread->hash() ) ;

		delete _hash_thread ;
		_hash_thread = NULL ;

		if(check_hash == mHash)
		{
			mFlag = FT_TM_FLAG_COMPLETE ;	// Transfer is complete.
#ifdef FT_DEBUG
			std::cerr << "ftTransferModule::checkFile(): hash finished. File verification complete ! Setting mFlag to 1" << std::endl ;
#endif
			return true ;
		}
	}


	forceCheck() ;
	return false ;
}

void ftTransferModule::forceCheck()
{
	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::forceCheck(): setting flags to force check." << std::endl ;
#endif

	mFileCreator->forceCheck() ;
	mFlag = FT_TM_FLAG_DOWNLOADING ;	// Ask for CRC map.
	mFileStatus.stat = ftFileStatus::PQIFILE_DOWNLOADING;
}

void ftTransferModule::adjustSpeed()
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/

  std::map<RsPeerId,peerInfo>::iterator mit;


  actualRate = 0;
  for(mit = mFileSources.begin(); mit != mFileSources.end(); ++mit)
  {
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::adjustSpeed()";
	std::cerr << "Peer: " << mit->first; 
	std::cerr << " Desired Rate: " << (mit->second).desiredRate;
	std::cerr << " Actual Rate: " << (mit->second).actualRate;
	std::cerr << std::endl;
#endif
    actualRate += mit->second.actualRate;
  }

#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::adjustSpeed() Totals:";
	std::cerr << "Desired Rate: " << desiredRate << " Actual Rate: " << actualRate;
	std::cerr << std::endl;
#endif

  return;
}


/*******************************************************************************
 * Actual Peer Transfer Management Code.
 *
 * request very tick, at rate
 *
 *
 **/


/* NOTEs on this function...
 * 1) This is the critical function for deciding the rate at which ft takes place.
 * 2) Some of the peers might not have the file... care must be taken avoid deadlock.
 *
 * Eg. A edge case which fails badly.
 *     Small 1K file (one chunk), with 3 sources (A,B,C). A doesn't have file.
 *      (a) request data from A. B & C pause cos no more data needed.
 *	(b) all timeout, chunk reset... then back to request again (a) and repeat.
 *	(c) all timeout x 5 and are disabled.... no transfer, while B&C had it all the time.
 *
 *  To solve this we might need random waiting periods, so each peer can 
 *  be tried.
 *
 *
 */

bool ftTransferModule::locked_tickPeerTransfer(peerInfo &info)
{
	/* how long has it been? */
	rstime_t ts = time(NULL);

	int ageRecv = ts - info.recvTS;
	int ageReq = ts - info.lastTS;

	/* if offline - ignore */
	if(info.state == PQIPEER_SUSPEND) 
		return false;

	if (ageReq > (int) (FT_TM_RESTART_DOWNLOAD * (info.nResets + 1)))
	{
		// The succession of ifs, makes the process continue every 6 * FT_TM_RESTART_DOWNLOAD * FT_TM_MAX_RESETS seconds
		// on average, which is one attempt every 600 seconds in the least, which corresponds to once every 10 minutes in
		// average.
		//
		if (info.nResets > 1) /* 3rd timeout */
		{
			/* 90% chance of return false...
			 * will mean variations in which peer
			 * starts first. hopefully stop deadlocks.
			 */
			if (rand() % 12 != 0)
				return false;
		}

		info.state = PQIPEER_DOWNLOADING;
		info.recvTS = ts; /* reset to activate */
		info.nResets = std::min(FT_TM_MAX_RESETS,info.nResets + 1);
		ageRecv = 0;
	}

	if (ageRecv > (int) FT_TM_DOWNLOAD_TIMEOUT)
	{
		info.state = PQIPEER_IDLE;
		return false;
	}
#ifdef FT_DEBUG
	std::cerr << "locked_tickPeerTransfer() actual rate (before): " << info.actualRate << ", lastTransfers=" << info.lastTransfers << std::endl ;
	std::cerr << mHash<< " - actual rate: " << info.actualRate << " lastTransfers=" << info.lastTransfers << ". AgeReq = " << ageReq << std::endl;
#endif
	/* update rate */

    if( (info.lastTransfers > 0 && ageReq > 0) || ageReq > 2)
	{
		info.actualRate = info.actualRate * 0.75 + 0.25 * info.lastTransfers / (float)ageReq;
		info.lastTransfers = 0;
		info.lastTS = ts;
	}

	/****************
	 * NOTE: If we continually increase the request rate thus: ...
	 * uint32_t next_req = info.actualRate * 1.25;
	 *
	 * then we will achieve max data rate, but we will fill up 
	 * peers out queue and/or network buffers.....
	 *
	 * we must therefore monitor the RTT to tell us if this is happening.
	 */

	/* emergency shutdown if we are stuck in x 1.25 mode
	 * probably not needed
	 */

// csoler: I commented this out because that tends to make some sources 
//  get stuck into minimal 128 B/s rate, when multiple sources are competiting into  the 
//  same limited bandwidth. I don't think this emergency shutdown is necessary anyway.
//
//  	if ((info.rttActive) && (ts - info.rttStart > FT_TM_SLOW_RTT))
//	{
//		if (info.mRateIncrease > 0)
//		{
//#ifdef FT_DEBUG
//			std::cerr << "!!! - Emergency shutdown because rttActive is true, and age is " << ts - info.rttStart << std::endl ;
//#endif
//			info.mRateIncrease = 0;
//			info.rttActive = false ; // I've added this to avoid being stuck when rttActive is true
//		}
//	}

	/* request at more than current rate */
	uint32_t next_req = info.actualRate * (1.0 + info.mRateIncrease);
#ifdef FT_DEBUG
	std::cerr << "locked_tickPeerTransfer() actual rate (after): " << actualRate 
				<< " info.desiredRate=" << info.desiredRate 
				<< " info.actualRate=" << info.actualRate 
				<< ", next_req=" << next_req ;

	std::cerr << std::endl;
#endif

	if (next_req > info.desiredRate * 1.1)
	{
		next_req = info.desiredRate * 1.1;
#ifdef FT_DEBUG
		std::cerr << "locked_tickPeerTransfer() Reached MaxRate: next_req: " << next_req;
		std::cerr << std::endl;
#endif
	}


	if (next_req > FT_TM_MAX_PEER_RATE)
	{
		next_req = FT_TM_MAX_PEER_RATE;
#ifdef FT_DEBUG
		std::cerr << "locked_tickPeerTransfer() Reached AbsMaxRate: next_req: " << next_req;
		std::cerr << std::endl;
#endif
	}


	if (next_req < FT_TM_MINIMUM_CHUNK)
	{
		next_req = FT_TM_MINIMUM_CHUNK;
#ifdef FT_DEBUG
		std::cerr << "locked_tickPeerTransfer() small chunk: next_req: " << next_req;
		std::cerr << std::endl;
#endif
	}

#ifdef FT_DEBUG
	std::cerr << "locked_tickPeerTransfer() desired  next_req: " << next_req;
	std::cerr << std::endl;
#endif
	
	/* do request */
	uint64_t req_offset = 0;
	uint32_t req_size =0 ;

	// Loop over multiple calls to the file creator: for some reasons the file creator might not be able to
	// give a plain chunk of the requested size (size hint larger than the fixed chunk size, priority given to 
	// an old pending chunk, etc).
	//
	while(next_req > 0 && locked_getChunk(info.peerId,next_req,req_offset,req_size))
		if(req_size > 0)
		{
			info.state = PQIPEER_DOWNLOADING;
			locked_requestData(info.peerId,req_offset,req_size);

			/* start next rtt measurement */
			if (!info.rttActive)
			{
				info.rttStart = ts;
				info.rttActive = true;
				info.rttOffset = req_offset + req_size;
			}
			next_req -= std::min(req_size,next_req) ;
		}
		else
		{
			std::cerr << "transfermodule::Waiting for available data";
			std::cerr << std::endl;
			break ;
		}

	return true;
}

	
	
  //interface to client module
bool ftTransferModule::locked_recvPeerData(peerInfo &info, uint64_t offset, uint32_t chunk_size, void *)
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::locked_recvPeerData()";
	std::cerr << " peerId: " << info.peerId;
	std::cerr << " rttOffset: " << info.rttOffset;
	std::cerr << " lastTransfers: " << info.lastTransfers;
	std::cerr << " offset: " << offset;
	std::cerr << " chunksize: " << chunk_size;
	std::cerr << std::endl;
#endif

  rstime_t ts = time(NULL);
  info.recvTS = ts;
  info.nResets = 0;
  info.state = PQIPEER_DOWNLOADING;
  info.lastTransfers += chunk_size;

   if ((info.rttActive) && (info.rttOffset == offset + chunk_size))
   {
 	  /* update tip */
 	  int32_t rtt = time(NULL) - info.rttStart;
 
 	  /* 
 		* FT_TM_FAST_RTT = 1 sec. mRateIncrease =  1.00
 		* FT_TM_SLOW_RTT =20 sec. mRateIncrease =  0
 		* 		   11 sec. mRateIncrease = -0.25
 		* if it is slower than this allow fast data increase.
 		* initial guess - linear with rtt.
 		* change if this leads to wild oscillations 
 		*
 		*/
 
// 	  info.mRateIncrease = (FT_TM_SLOW_RTT - rtt) * 
// 		  (FT_TM_MAX_INCREASE / (FT_TM_SLOW_RTT - FT_TM_FAST_RTT));
// 
// 	  if (info.mRateIncrease > FT_TM_MAX_INCREASE)
// 		  info.mRateIncrease = FT_TM_MAX_INCREASE;
// 
// 	  if (info.mRateIncrease < FT_TM_MIN_INCREASE)
// 		  info.mRateIncrease = FT_TM_MIN_INCREASE;
 
	  switch(mPriority)
	  {
		  case SPEED_LOW  	: info.mRateIncrease = FT_TM_RATE_INCREASE_SLOWER ; break ;
		  case SPEED_NORMAL	: info.mRateIncrease = FT_TM_RATE_INCREASE_AVERAGE; break ;
		  case SPEED_HIGH  	: info.mRateIncrease = FT_TM_RATE_INCREASE_FASTER ; break ;
	  }
 	  info.rtt = rtt;
 	  info.rttActive = false;

#ifdef FT_DEBUG
	  std::cerr << "ftTransferModule::locked_recvPeerData()";
	  std::cerr << "Updated Rate based on RTT: " << rtt;
	  std::cerr << " Rate increase: " << 1.0+info.mRateIncrease;
	  std::cerr << std::endl;
#endif

  }
  return true;
}

