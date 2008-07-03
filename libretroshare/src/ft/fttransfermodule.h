/*
 * Retroshare file transfer module: ftTransferModule.h
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
 
#ifndef FT_TRANSFER_MODULE_HEADER
#define FT_TRANSFER_MODULE_HEADER

/*
 * FUNCTION DESCRIPTION
 *
 * Each Transfer Module is paired up with a single File Creator, and responsible for the transfer of one file.
 * The Transfer Module is responsible for sending requests to peers at the correct data rates, and storing the returned data
 * in a FileCreator.
 * There are multiple Transfer Modules in the File Transfer system. Their requests are multiplexed through the Client Module. * The Transfer Module contains all the algorithms for sensible Data Requests.
 * It must be able to cope with varying data rates and dropped peers without flooding the system with too many requests.
 *
 */

#include <map>
#include <list>
#include <ftFileCreator.h>

class Request
{
  uint64_t offset;
  uint32_t chunkSize;
};

class peerInfo
{
  std:string peerId;
  uint16_t state;
  uint32_t desiredRate;
  Request lastRequest;
  uint32_t actualRate;
};

class ftTransferModule 
{
public:
  ftTransferModule(std::string hash, uint64_t size);
  ~ftTransferModule();

  //interface to download controller
  bool setFileSources(std::list<peerInfo> availableSrcs);
  bool requestFile(std::list<std::string> onlineSrcs, uint32_t dataRate);
  bool changePeerState(std::string peerId, uint16_t newState);
  uint32_t getDataRate();
  bool setDataRate(uint32_t dataRate);
  bool stopTransfer();
  bool resumeTransfer();
  bool completeFileTransfer();

  //interface to client module
  bool recvFileData(uint64_t offset, uint32_t chunk_size, void *data);
  void requestData(std::hash, uint64_t offset, uint32_t chunk_size);

  //interface to file creator
  bool getChunk(uint64_t &offset, uint32_t &chunk_size);
  bool storeData(uint64_t offset, uint32_t chunk_size);

  void tick();
  
public:
  ftFileCreator* fc;

private:
  std::string hash;
  uint64_t    size;
  uint32_t    dataRate;   //data transfer rate for current file
  uint16_t    state;      //file transfer state
  std::list<std::string> onlinePeerList;
  std::map<std::string,peerInfo> availablePeerList;
};

#endif  //FT_TRANSFER_MODULE_HEADER
