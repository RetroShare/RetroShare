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

#include "fttransfermodule.h"

const uint32_t PQIPEER_OFFLINE_CHECK  = 120; /* check every 2 minutes */
const uint32_t PQIPEER_DOWNLOAD_TIMEOUT  = 60; /* time it out, -> offline after 60 secs */
const uint32_t PQIPEER_DOWNLOAD_CHECK    = 10; /* desired delta = 10 secs */
const uint32_t PQIPEER_DOWNLOAD_TOO_FAST = 8; /* 8 secs */
const uint32_t PQIPEER_DOWNLOAD_TOO_SLOW = 12; /* 12 secs */
const uint32_t PQIPEER_DOWNLOAD_MIN_DELTA = 5; /* 5 secs */

ftTransferModule::ftTransferModule(ftFileCreator *fc, ftDataMultiplex *dm)
	:mFileCreator(fc), mMultiplexor(dm), mFlag(0)
{
	mHash = mFileCreator->getHash();
	mSize = mFileCreator->getFileSize();

	// Dummy for Testing (should be handled independantly for 
	// each peer.
	mChunkSize = 10000;
	return;
}

ftTransferModule::~ftTransferModule()
{}

bool ftTransferModule::setFileSources(std::list<std::string> peerIds)
{
  std::list<std::string>::iterator it;
  for(it = peerIds.begin(); it != peerIds.end(); it++)
  {
    mFileSources.push_back(*it);
  }

  return 1;
}

bool ftTransferModule::setPeerState(std::string peerId,uint32_t state,uint32_t maxRate)  //state = ONLINE/OFFLINE
{
  bool found = false;
  std::list<std::string>::iterator it;
  it = mFileSources.begin();
  while (( it != mFileSources.end())&&(!found))
  {
    if ((*it) == peerId) 
      found = true;
    it++;
  }

  if (!found) mFileSources.push_back(*it);

  std::map<std::string,peerInfo>::iterator mit;
  mit = mOnlinePeers.find(peerId);
  if (mit == mOnlinePeers.end())
  {
    peerInfo pInfo;
    pInfo.state = state;
    pInfo.desiredRate = maxRate;
    mOnlinePeers[peerId] = pInfo;
  }
  else
  {
    (mit->second).state = state;
    (mit->second).desiredRate = maxRate;
  }

  return 1;
}

uint32_t ftTransferModule::getDataRate(std::string peerId)
{
  std::map<std::string,peerInfo>::iterator mit;
  mit = mOnlinePeers.find(peerId);
  if (mit == mOnlinePeers.end())
    return 0;
  else
    return (mit->second).actualRate;
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

	return storeData(offset, chunk_size, data);
}

void ftTransferModule::requestData(std::string peerId, uint64_t offset, uint32_t chunk_size)
{
  mMultiplexor->sendDataRequest(peerId, mHash, mSize, offset,chunk_size);
}

bool ftTransferModule::getChunk(uint64_t &offset, uint32_t &chunk_size)
{
  mFileCreator->getMissingChunk(offset, chunk_size);
}

bool ftTransferModule::storeData(uint64_t offset, uint32_t chunk_size,void *data)
{
	mFileCreator -> addFileData(offset, chunk_size, data);
}

void ftTransferModule::queryInactive()
{
#ifdef FT_DEBUG
  std::ostringstream out;
  out<<"ftTransferModule::queryInactive()";
  out<<std:endl;
#endif

  int ts = time(NULL);
  
  std::map<std::string,peerInfo>::iterator mit;
  for(mit = mOnlinePeers.begin(); mit != mOnlinePeers.end(); mit++)
  {
    switch ((mit->second).state) 
    {
    	//Peer side has change from online to offline during transfer
      case PQIPEER_NOT_ONLINE:
      	if (ts - ((mit->second).lastTS) > PQIPEER_OFFLINE_CHECK)
      	{//start to request data
      		getChunk(mOffset,mChunkSize);
      		if (mChunkSize != 0)  
      		{ 
      			(mit->second).req_loc = mOffset;
      			(mit->second).req_size = mChunkSize;
      			(mit->second).lastTS = ts;
      			(mit->second).state = PQIPEER_DOWNLOADING;
      			requestData(mit->first, mOffset,mChunkSize);
      		}
      	  else mFlag = 1;
      	}
        break;
      
      //file request has been sent to peer side, but no response received yet  
      case PQIPEER_DOWNLOADING:
      	if (ts - ((mit->second).lastTS) > PQIPEER_DOWNLOAD_CHECK)
      		requestData(mit->first, (mit->second).req_loc,(mit->second).req_size);  //give a push
        break;
      
      //file response has been received or peer side is just ready for download  
      case PQIPEER_IDLE:
     		getChunk(mOffset,mChunkSize);
     		if (mChunkSize != 0)  
     		{
     			(mit->second).req_loc = mOffset;
     			(mit->second).req_size = mChunkSize;
     			(mit->second).lastTS = ts;
     			(mit->second).state = PQIPEER_DOWNLOADING;
     			requestData(mit->first, mOffset,mChunkSize);
     		}
     	  else mFlag = 1;        	
      	break;
      
      //file transfer has been stopped
      case PQIPEER_SUSPEND:
        break;

      default:
        break;
    }//switch
  }//for
  
}

bool ftTransferModule::stopTransfer()
{
  std::map<std::string,peerInfo>::iterator mit;
  for(mit = mOnlinePeers.begin(); mit != mOnlinePeers.end(); mit++)
  {
    (mit->second).state = PQIPEER_SUSPEND;
  }

  return 1;
}

bool ftTransferModule::resumeTransfer()
{
  std::map<std::string,peerInfo>::iterator mit;
  for(mit = mOnlinePeers.begin(); mit != mOnlinePeers.end(); mit++)
  {
    (mit->second).state = PQIPEER_IDLE;
  }

  return 1;
}

bool ftTransferModule::completeFileTransfer()
{
}	

int ftTransferModule::tick()
{
  queryInactive();
  if (mFlag != 1) adjustSpeed();
  else
    completeFileTransfer();  	
    
  return 0;
}


void ftTransferModule::adjustSpeed()
{
	return;
}


