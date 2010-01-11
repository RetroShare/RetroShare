/*
 * libretroshare/src/ft: fttransfermodule.cc
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

/******
 * #define FT_DEBUG 1
 *****/

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

const double FT_TM_MAX_PEER_RATE = 1024 * 1024; /* 1MB/s */
const uint32_t FT_TM_MAX_RESETS  = 5;

const uint32_t FT_TM_MINIMUM_CHUNK = 128; /* ie 1/8Kb / sec */
const uint32_t FT_TM_RESTART_DOWNLOAD = 20; /* 20 seconds */
const uint32_t FT_TM_DOWNLOAD_TIMEOUT = 10; /* 10 seconds */

const double FT_TM_MAX_INCREASE = 1.00;
const double FT_TM_MIN_INCREASE = -0.10;
const int32_t FT_TM_FAST_RTT    = 1.0;
const int32_t FT_TM_STD_RTT     = 5.0;
const int32_t FT_TM_SLOW_RTT    = 9.0;

ftTransferModule::ftTransferModule(ftFileCreator *fc, ftDataMultiplex *dm, ftController *c)
	:mFileCreator(fc), mMultiplexor(dm), mFtController(c), mFlag(0)
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/

	mHash = mFileCreator->getHash();
	mSize = mFileCreator->getFileSize();
        mFileStatus.hash = mHash;

	// Dummy for Testing (should be handled independantly for 
	// each peer.
	//mChunkSize = 10000;
	desiredRate = 1000000; /* 1MB/s ??? */
	actualRate = 0;
	return;
}

ftTransferModule::~ftTransferModule()
{}


bool ftTransferModule::setFileSources(std::list<std::string> peerIds)
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/

  mFileSources.clear();

#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::setFileSources()";
	std::cerr << " List of peers: " ;
#endif

  std::list<std::string>::iterator it;
  for(it = peerIds.begin(); it != peerIds.end(); it++)
  {

#ifdef FT_DEBUG
	std::cerr << " \t" << *it;
#endif

    peerInfo pInfo(*it);
    mFileSources.insert(std::pair<std::string,peerInfo>(*it,pInfo));
  }

#ifdef FT_DEBUG
	std::cerr << std::endl;
#endif

  return true;
}

bool ftTransferModule::getFileSources(std::list<std::string> &peerIds)
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
    std::map<std::string,peerInfo>::iterator it;
    for(it = mFileSources.begin(); it != mFileSources.end(); it++)
    {
	peerIds.push_back(it->first);
    }
    return true;
}

bool ftTransferModule::addFileSource(std::string peerId)
{
  RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
  std::map<std::string,peerInfo>::iterator mit;
  mit = mFileSources.find(peerId);

  if (mit == mFileSources.end())
  {
  	/* add in new source */
  	peerInfo pInfo(peerId);
    	mFileSources.insert(std::pair<std::string,peerInfo>(peerId,pInfo));
	mit = mFileSources.find(peerId);

#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::addFileSource()";
	std::cerr << " adding peer: " << peerId << " to sourceList";
	std::cerr << std::endl;
#endif

  }
  else
  {
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::addFileSource()";
	std::cerr << " peer: " << peerId << " already there";
	std::cerr << std::endl;
#endif
  }
  return true;
}

bool ftTransferModule::removeFileSource(std::string peerId)
{
	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
	std::map<std::string,peerInfo>::iterator mit;
	mit = mFileSources.find(peerId);

	if (mit != mFileSources.end())
	{
		/* add in new source */
		mFileSources.erase(mit) ;
#ifdef FT_DEBUG
		std::cerr << "ftTransferModule::addFileSource(): removing peer: " << peerId << " from sourceList" << std::cerr << std::endl;
#endif
	}
#ifdef FT_DEBUG
	else
		std::cerr << "ftTransferModule::addFileSource(): Should remove peer: " << peerId << ", but it's not in the source list. " << std::cerr << std::endl;
#endif

	return true;
}

bool ftTransferModule::setPeerState(std::string peerId,uint32_t state,uint32_t maxRate) 
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::setPeerState()";
	std::cerr << " peerId: " << peerId;
	std::cerr << " state: " << state;
	std::cerr << " maxRate: " << maxRate << std::endl;
#endif

  std::map<std::string,peerInfo>::iterator mit;
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

  std::list<std::string>::iterator it;
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


bool ftTransferModule::getPeerState(std::string peerId,uint32_t &state,uint32_t &tfRate)
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
  std::map<std::string,peerInfo>::iterator mit;
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

uint32_t ftTransferModule::getDataRate(std::string peerId)
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
  std::map<std::string,peerInfo>::iterator mit;
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


  //interface to client module
bool ftTransferModule::recvFileData(std::string peerId, uint64_t offset, uint32_t chunk_size, void *data)
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::recvFileData()";
	std::cerr << " peerId: " << peerId;
	std::cerr << " offset: " << offset;
	std::cerr << " chunksize: " << chunk_size;
	std::cerr << " data: " << data;
	std::cerr << std::endl;
#endif

  bool ok = false;

  {
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/

  	std::map<std::string,peerInfo>::iterator mit;
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

  } /***** STACK MUTEX END ****/

  if (ok)
  	storeData(offset, chunk_size, data);

  free(data) ;
  return ok;
}

void ftTransferModule::requestData(std::string peerId, uint64_t offset, uint32_t chunk_size)
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

bool ftTransferModule::getChunk(const std::string& peer_id,uint32_t size_hint,uint64_t &offset, uint32_t &chunk_size)
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::getChunk()";
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
		mMultiplexor->sendChunkMapRequest(peer_id, mHash) ;

#ifdef FT_DEBUG
	if (val)
	{
		std::cerr << "ftTransferModule::getChunk()";
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
		std::cerr << "ftTransferModule::getChunk()";
		std::cerr << " Answer: No Chunk Available";
		std::cerr << " peer map needed = " << source_peer_map_needed << std::endl ;
		std::cerr << std::endl;
	}
#endif

	return val;
}

bool ftTransferModule::storeData(uint64_t offset, uint32_t chunk_size,void *data)
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
			mFlag = 2; //file canceled by user
		return false;
	}

  	std::map<std::string,peerInfo>::iterator mit;
  	for(mit = mFileSources.begin(); mit != mFileSources.end(); mit++)
  	{
		locked_tickPeerTransfer(mit->second);
	}
  	return true; 
}

bool ftTransferModule::pauseTransfer()
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/

/*
  std::map<std::string,peerInfo>::iterator mit;
  for(mit = mOnlinePeers.begin(); mit != mOnlinePeers.end(); mit++)
  {
    (mit->second).state = PQIPEER_SUSPEND;
  }
*/
  mFileStatus.stat=ftFileStatus::PQIFILE_PAUSE;
  
  return 1;
}

bool ftTransferModule::resumeTransfer()
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
/*
  std::map<std::string,peerInfo>::iterator mit;
  for(mit = mOnlinePeers.begin(); mit != mOnlinePeers.end(); mit++)
  {
    (mit->second).state = PQIPEER_IDLE;
  }
*/
  mFileStatus.stat=ftFileStatus::PQIFILE_DOWNLOADING;

  return 1;
}

bool ftTransferModule::cancelTransfer()
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/
  mFileStatus.stat=ftFileStatus::PQIFILE_FAIL_CANCEL;

  return 1;
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
  	std::map<std::string,peerInfo>::iterator it;
  	for(it = mFileSources.begin(); it != mFileSources.end(); it++)
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
	  case 0: //file transfer not complete
		  adjustSpeed();
		  break;
	  case 1: //file transfer complete
		  completeFileTransfer();
		  break;
	  case 2: //file transfer canceled
		  break;
	  default:
		  break;
  }
    
  return 0;
}


void ftTransferModule::adjustSpeed()
{
  	RsStackMutex stack(tfMtx); /******* STACK LOCKED ******/

  std::map<std::string,peerInfo>::iterator mit;


  actualRate = 0;
  for(mit = mFileSources.begin(); mit != mFileSources.end(); mit++)
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
	time_t ts = time(NULL);

	int ageRecv = ts - info.recvTS;
	int ageReq = ts - info.lastTS;

	/* if offline - ignore */
	if ((info.state == PQIPEER_SUSPEND) ||
	    (info.state == PQIPEER_NOT_ONLINE))
	{

		return false;
	}

	if (ageReq > (int) (FT_TM_RESTART_DOWNLOAD * (info.nResets + 1)))
	{
		if (info.nResets > 1) /* 3rd timeout */
		{
			/* 90% chance of return false...
			 * will mean variations in which peer
			 * starts first. hopefully stop deadlocks.
			 */
			if (rand() % 10 != 0)
			{
				return false;
			}
		}

		info.state = PQIPEER_DOWNLOADING;
		info.recvTS = ts; /* reset to activate */
		info.nResets++;
		ageRecv = 0;

		if (info.nResets >= FT_TM_MAX_RESETS)
		{
			/* for this file anyway */
			info.state = PQIPEER_NOT_ONLINE;
			return false;
		}
	}

	if (ageRecv > (int) FT_TM_DOWNLOAD_TIMEOUT)
	{
		info.state = PQIPEER_IDLE;
		return false;
	}
#ifdef FT_DEBUG
	std::cerr << "locked_tickPeerTransfer() actual rate (before): " << info.actualRate << ", lastTransfers=" << info.lastTransfers << std::endl ;
#endif
	/* update rate */
	info.actualRate = info.actualRate * 0.75 + 0.25 * info.lastTransfers;
	info.lastTransfers = 0;

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
  	if ((info.rttActive) && (ts - info.rttStart > FT_TM_SLOW_RTT))
	{
		if (info.mRateIncrease > 0)
		{
#ifdef FT_DEBUG
			std::cerr << "!!! - Emergency shutdown because rttActive is true, and age is " << ts - info.rttStart << std::endl ;
#endif
			info.mRateIncrease = 0;
			info.rttActive = false ; // I've added this to avoid being stuck when rttActive is true
		}
	}

	/* request at more than current rate */
	uint32_t next_req = info.actualRate * (1.0 + info.mRateIncrease);
#ifdef FT_DEBUG
	std::cerr << "locked_tickPeerTransfer() actual rate (after): " << actualRate 
				<< " increase factor=" << 1.0 + info.mRateIncrease 
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

	info.lastTS = ts;

#ifdef FT_DEBUG
	std::cerr << "locked_tickPeerTransfer() desired  next_req: " << next_req;
	std::cerr << std::endl;
#endif
	
	/* do request */
	uint64_t req_offset = 0;
	uint32_t req_size =0 ;

	if (getChunk(info.peerId,next_req,req_offset,req_size))
	{
		if (req_size > 0)
		{
			info.state = PQIPEER_DOWNLOADING;
			requestData(info.peerId,req_offset,req_size);

			/* start next rtt measurement */
			if (!info.rttActive)
			{
				info.rttStart = ts;
				info.rttActive = true;
				info.rttOffset = req_offset + req_size;
			}
		}
		else
		{
			std::cerr << "transfermodule::Waiting for available data";
			std::cerr << std::endl;
		}
	}
	else 
		mFlag = 1;      

	return true;
}

	
	
  //interface to client module
bool ftTransferModule::locked_recvPeerData(peerInfo &info, uint64_t offset, 
			uint32_t chunk_size, void *data)
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::locked_recvPeerData()";
	std::cerr << " peerId: " << info.peerId;
	std::cerr << " rttOffset: " << info.rttOffset;
	std::cerr << " lastTransfers: " << info.lastTransfers;
	std::cerr << " offset: " << offset;
	std::cerr << " chunksize: " << chunk_size;
	std::cerr << " data: " << data;
	std::cerr << std::endl;
#endif

  time_t ts = time(NULL);
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
		* FT_TM_SLOW_RTT = 9 sec. mRateIncrease =  0
		* 		   11 sec. mRateIncrease = -0.25
		* if it is slower than this allow fast data increase.
		* initial guess - linear with rtt.
		* change if this leads to wild oscillations 
		*
		*/

	  info.mRateIncrease = (FT_TM_SLOW_RTT - rtt) * 
		  (FT_TM_MAX_INCREASE / (FT_TM_SLOW_RTT - FT_TM_FAST_RTT));

	  if (info.mRateIncrease > FT_TM_MAX_INCREASE)
		  info.mRateIncrease = FT_TM_MAX_INCREASE;

	  if (info.mRateIncrease < FT_TM_MIN_INCREASE)
		  info.mRateIncrease = FT_TM_MIN_INCREASE;

	  info.rtt = rtt;
	  info.rttActive = false;

#ifdef FT_DEBUG
	  std::cerr << "ftTransferModule::locked_recvPeerData()";
	  std::cerr << "Updated Rate based on RTT: " << rtt;
	  std::cerr << " Rate: " << info.mRateIncrease;
	  std::cerr << std::endl;
#endif

  }
  return true;
}

