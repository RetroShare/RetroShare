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

ftTransferModule::ftTransferModule(ftFileCreator *fc, ftDataMultiplex *dm)
	:mFileCreator(fc), mMultiplexor(dm), mFlag(0)
{
	mHash = mFileCreator->getHash();
	mSize = mFileCreator->getFileSize();

	// Dummy for Testing (should be handled independantly for 
	// each peer.
	//mChunkSize = 10000;
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

  if (!found) mFileSources.push_back(peerId);

  std::map<std::string,peerInfo>::iterator mit;
  mit = mOnlinePeers.find(peerId);
  if (mit == mOnlinePeers.end())
  {
    peerInfo pInfo(peerId,state,maxRate);
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
  mit = mOnlinePeers.find(peerId);
  if (mit == mOnlinePeers.end())
    return false;
  if ((mit->second).state != PQIPEER_DOWNLOADING)
    return false;
  if (offset != ((mit->second).offset + (mit->second).receivedSize))
    return false;
  (mit->second).receivedSize += chunk_size;
  (mit->second).state = PQIPEER_IDLE;

	return storeData(offset, chunk_size, data);
}

void ftTransferModule::requestData(std::string peerId, uint64_t offset, uint32_t chunk_size)
{
	std::cerr << "ftTransferModule::requestData()";
	std::cerr << " peerId: " << peerId;
	std::cerr << " offset: " << offset;
	std::cerr << " chunk_size: " << chunk_size;
	std::cerr << std::endl;

  mMultiplexor->sendDataRequest(peerId, mHash, mSize, offset,chunk_size);
}

bool ftTransferModule::getChunk(uint64_t &offset, uint32_t &chunk_size)
{
	std::cerr << "ftTransferModule::getChunk()";
	std::cerr << " Request: offset: " << offset;
	std::cerr << " chunk_size: " << chunk_size;
	std::cerr << std::endl;

  	bool val = mFileCreator->getMissingChunk(offset, chunk_size);

	if (val)
	{
		std::cerr << "ftTransferModule::getChunk()";
		std::cerr << " Answer: offset: " << offset;
		std::cerr << " chunk_size: " << chunk_size;
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "ftTransferModule::getChunk()";
		std::cerr << " Answer: No Chunk Available";
		std::cerr << std::endl;
	}

	return val;
}

bool ftTransferModule::storeData(uint64_t offset, uint32_t chunk_size,void *data)
{
	std::cerr << "ftTransferModule::storeData()";
	std::cerr << " offset: " << offset;
	std::cerr << " chunk_size: " << chunk_size;
	std::cerr << std::endl;

	return mFileCreator -> addFileData(offset, chunk_size, data);
}

void ftTransferModule::queryInactive()
{
#ifdef FT_DEBUG
  std::ostringstream out;
  out<<"ftTransferModule::queryInactive()";
  out<<std:endl;
  std::cerr << out.str();
#endif
  if (mFileStatus.stat == ftFileStatus::PQIFILE_INIT)
	  mFileStatus.stat = ftFileStatus::PQIFILE_DOWNLOADING;

  if (mFileStatus.stat != ftFileStatus::PQIFILE_DOWNLOADING)
	  return;

  int ts = time(NULL);
  uint64_t offset;
  uint32_t size;
  int delta;  

  std::map<std::string,peerInfo>::iterator mit;
  for(mit = mOnlinePeers.begin(); mit != mOnlinePeers.end(); mit++)
  {
    switch ((mit->second).state) 
    {
    	//Peer side has change from online to offline during transfer
      case PQIPEER_NOT_ONLINE:
      	if (ts - ((mit->second).lastTS) > PQIPEER_OFFLINE_CHECK)
      	{//start to request data
          size = TRANSFER_START_MIN;
      		if (getChunk(offset,size))  
      		{ 
      			(mit->second).offset = offset;
      			(mit->second).chunkSize = size;
      			(mit->second).lastTS = ts;
      			(mit->second).state = PQIPEER_DOWNLOADING;
      			requestData(mit->first, offset,size);
      		}
      	  else mFlag = 1;
      	}
        break;
      
      //file request has been sent to peer side, but no response received yet  
      case PQIPEER_DOWNLOADING:
      	if (ts - ((mit->second).lastTS) > PQIPEER_DOWNLOAD_CHECK)
      		requestData(mit->first, (mit->second).offset,(mit->second).chunkSize);  //give a push

        actualRate += (mit->second).actualRate;
        break;
      
      //file response has been received or peer side is just ready for download  
      case PQIPEER_IDLE:
     		(mit->second).actualRate = (mit->second).chunkSize/(ts-(mit->second).lastTS);
        if ((mit->second).actualRate < (mit->second).desiredRate)
        {
          size = (mit->second).chunkSize * 2 ;
        }
        else
        {
          size = (uint32_t ) ((mit->second).chunkSize * 0.9) ;
        }
     		if (getChunk(offset,size))  
     		{
     			(mit->second).offset = offset;
     			(mit->second).chunkSize = size;
     			(mit->second).lastTS = ts;
     			(mit->second).state = PQIPEER_DOWNLOADING;
     			requestData(mit->first,offset,size);
     		}
     	  else mFlag = 1;      

        actualRate += (mit->second).actualRate;
      	break;
      
      //file transfer has been stopped
      case PQIPEER_SUSPEND:
        break;

      default:
        break;
    }//switch
  }//for
  
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
}

bool ftTransferModule::completeFileTransfer()
{
	return true;
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
  std::map<std::string,peerInfo>::iterator mit;
  for(mit = mOnlinePeers.begin(); mit != mOnlinePeers.end(); mit++)
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


