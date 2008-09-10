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

#define FT_DEBUG 1

#include "fttransfermodule.h"

ftTransferModule::ftTransferModule(ftFileCreator *fc, ftDataMultiplex *dm, ftController *c)
	:mFileCreator(fc), mMultiplexor(dm), mFtController(c), mFlag(0)
{
	mHash = mFileCreator->getHash();
	mSize = mFileCreator->getFileSize();
        mFileStatus.hash = mHash;

	// Dummy for Testing (should be handled independantly for 
	// each peer.
	//mChunkSize = 10000;
	return;
}

ftTransferModule::~ftTransferModule()
{}

bool ftTransferModule::setFileSources(std::list<std::string> peerIds)
{
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

bool ftTransferModule::setPeerState(std::string peerId,uint32_t state,uint32_t maxRate) 
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::setPeerState()";
	std::cerr << " peerId: " << peerId;
	std::cerr << " state: " << state;
	std::cerr << " maxRate: " << maxRate << std::endl;
#endif

  std::map<std::string,peerInfo>::iterator mit;
  mit = mFileSources.find(peerId);

  if (mit == mFileSources.end()) return false;

  (mit->second).state=state;
  (mit->second).desiredRate=maxRate;

  std::list<std::string>::iterator it;
  it=mOnlinePeers.begin();
  while((it!=mOnlinePeers.end())&&(*it!=peerId)) it++;

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

uint32_t ftTransferModule::getDataRate(std::string peerId)
{
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
bool ftTransferModule::recvFileData(std::string peerId, uint64_t offset, 
			uint32_t chunk_size, void *data)
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::recvFileData()";
	std::cerr << " peerId: " << peerId;
	std::cerr << " offset: " << offset;
	std::cerr << " chunksize: " << chunk_size;
	std::cerr << std::endl;
#endif

  std::map<std::string,peerInfo>::iterator mit;
  mit = mFileSources.find(peerId);

  if (mit == mFileSources.end())
    return false;

  if ((mit->second).state != PQIPEER_DOWNLOADING)
    return false;

  if (offset != ((mit->second).offset + (mit->second).receivedSize))
  {
    //fix me
    //received data not expected
    return false;
  }

  (mit->second).receivedSize += chunk_size;
  (mit->second).state = PQIPEER_IDLE;

  return storeData(offset, chunk_size, data);
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

bool ftTransferModule::getChunk(uint64_t &offset, uint32_t &chunk_size)
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::getChunk()";
	std::cerr << " hash: " << mHash;
	std::cerr << " size: " << mSize;
	std::cerr << " offset: " << offset;
	std::cerr << " chunk_size: " << chunk_size;
	std::cerr << std::endl;
#endif

  	bool val = mFileCreator->getMissingChunk(offset, chunk_size);

#ifdef FT_DEBUG
	if (val)
	{
		std::cerr << "ftTransferModule::getChunk()";
		std::cerr << " Answer: Chunk Available";
	        std::cerr << " hash: " << mHash;
	        std::cerr << " size: " << mSize;
		std::cerr << " offset: " << offset;
		std::cerr << " chunk_size: " << chunk_size;
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "ftTransferModule::getChunk()";
		std::cerr << " Answer: No Chunk Available";
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

  int ts = time(NULL);
  uint64_t req_offset;
  uint32_t req_size;
  int delta;  

  std::map<std::string,peerInfo>::iterator mit;
  for(mit = mFileSources.begin(); mit != mFileSources.end(); mit++)
  {
    std::string peerId = mit->first;
    peerInfo* pInfo = &mit->second;
    switch (pInfo->state) 
    {
    	//Peer side has change from online to offline during transfer
      case PQIPEER_NOT_ONLINE:
/*      	
        if (ts - (pInfo->lastTS) > PQIPEER_OFFLINE_CHECK)
      	{//start to request data
          req_size = TRANSFER_START_MIN;
          if (getChunk(req_offset,req_size))  
      	  { 
      		pInfo->offset = req_offset;
      		pInfo->chunkSize = req_size;
      		pInfo->lastTS = ts;
      		pInfo->state = PQIPEER_DOWNLOADING;
      		requestData(peerId, req_offset,req_size);
      	  }
      	  else mFlag = 1;
      	}
*/
        break;
      
      //file request has been sent to peer side, but no response received yet  
      case PQIPEER_DOWNLOADING:
      	if (ts - (pInfo->lastTS) > PQIPEER_DOWNLOAD_CHECK)
      	  requestData(peerId, pInfo->offset,pInfo->chunkSize);  //give a push

        actualRate += pInfo->actualRate;

        break;
      
      //file response has been received or peer side is just ready for download  
      case PQIPEER_IDLE:
     	pInfo->actualRate = pInfo->chunkSize/(ts-(pInfo->lastTS));
        if (pInfo->actualRate < pInfo->desiredRate/2)
        {
          req_size = pInfo->chunkSize * 2 ;
        }
        else
        {
          req_size = (uint32_t ) (pInfo->chunkSize * 0.9) ;
        }

     	if (getChunk(req_offset,req_size))  
     	{
     		pInfo->offset = req_offset;
     		pInfo->chunkSize = req_size;
     		pInfo->lastTS = ts;
     		pInfo->state = PQIPEER_DOWNLOADING;
     		requestData(peerId,req_offset,req_size);
     	}
        else mFlag = 1;      

        actualRate += pInfo->actualRate;
      	break;
      
      //file transfer has been stopped
      case PQIPEER_SUSPEND:
        break;

      default:
        break;
    }//switch
  }//for

  return true; 
}

bool ftTransferModule::pauseTransfer()
{
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
  mFileStatus.stat=ftFileStatus::PQIFILE_FAIL_CANCEL;

  return 1;
}

bool ftTransferModule::completeFileTransfer()
{
	//mFtController->completeFile(mHash);
	return true;
}

int ftTransferModule::tick()
{
#ifdef FT_DEBUG
	std::cerr << "ftTransferModule::tick()";
	std::cerr << " mFlag: " << mFlag;
	std::cerr << std::endl;
#endif

  queryInactive();
  switch (mFlag)
  {
	  case 0:
		  adjustSpeed();
		  break;
	  case 1:
		  completeFileTransfer();
		  break;
	  case 2:
		  /* tell me what to do here */
		  break;
	  default:
		  break;
  }
    
  return 0;
}


void ftTransferModule::adjustSpeed()
{
  std::map<std::string,peerInfo>::iterator mit;
  for(mit = mFileSources.begin(); mit != mFileSources.end(); mit++)
  {
    if (((mit->second).state == PQIPEER_DOWNLOADING) 
        || ((mit->second).state == PQIPEER_IDLE))
    {
        if ((actualRate < desiredRate) && ((mit->second).actualRate >= (mit->second).desiredRate))
        {
          (mit->second).desiredRate *= 1.1;
        }

        if ((actualRate > desiredRate) && ((mit->second).actualRate < (mit->second).desiredRate))
        {
          (mit->second).desiredRate *= 0.9;
        }
    }
  }
	return;
}


