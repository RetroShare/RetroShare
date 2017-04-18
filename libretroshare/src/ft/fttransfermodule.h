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
#include <string>

#include "ft/ftfilecreator.h"
#include "ft/ftdatamultiplex.h"
#include "ft/ftcontroller.h"

#include "util/rsthreads.h"

const uint32_t  PQIPEER_INIT                 = 0x0000;
const uint32_t  PQIPEER_NOT_ONLINE           = 0x0001;
const uint32_t  PQIPEER_DOWNLOADING          = 0x0002;
const uint32_t  PQIPEER_IDLE                 = 0x0004;
const uint32_t  PQIPEER_SUSPEND              = 0x0010;

class HashThread ;

class peerInfo
{
public:
	explicit peerInfo(const RsPeerId& peerId_in):peerId(peerId_in),state(PQIPEER_NOT_ONLINE),desiredRate(0),actualRate(0),
		lastTS(0),
		recvTS(0), lastTransfers(0), nResets(0), 
		rtt(0), rttActive(false), rttStart(0), rttOffset(0),
		mRateIncrease(1)
	{
		return;
	}
	peerInfo(const RsPeerId& peerId_in,uint32_t state_in,uint32_t maxRate_in):
		peerId(peerId_in),state(state_in),desiredRate(maxRate_in),actualRate(0),
		lastTS(0),
		recvTS(0), lastTransfers(0), nResets(0), 
		rtt(0), rttActive(false), rttStart(0), rttOffset(0),
		mRateIncrease(1)
	{
		return;
	}
  	RsPeerId peerId;
  	uint32_t state;
  	double desiredRate;
  	double actualRate;

  	time_t lastTS; /* last Request */
	time_t recvTS; /* last Recv */
	uint32_t lastTransfers; /* data recvd in last second */
	uint32_t nResets; /* count to disable non-existant files */

	/* rrt rate control */
	uint32_t rtt;       /* last rtt */
	bool     rttActive; /* have we initialised an rtt measurement */
	time_t	 rttStart;  /* ts of request */
	uint64_t rttOffset; /* end of request */
	float    mRateIncrease; /* current rate */
};

class ftFileStatus
{
public:
	enum Status {
		PQIFILE_INIT,
		PQIFILE_NOT_ONLINE,
		PQIFILE_DOWNLOADING,
		PQIFILE_COMPLETE,
		PQIFILE_CHECKING,
		PQIFILE_FAIL,
		PQIFILE_FAIL_CANCEL,
		PQIFILE_FAIL_NOT_AVAIL,
		PQIFILE_FAIL_NOT_OPEN,
		PQIFILE_FAIL_NOT_SEEK,
		PQIFILE_FAIL_NOT_WRITE,
		PQIFILE_FAIL_NOT_READ,
		PQIFILE_FAIL_BAD_PATH
	};
        
        ftFileStatus():hash(""),stat(PQIFILE_INIT) {}
	explicit ftFileStatus(const RsFileHash& hash_in):hash(hash_in),stat(PQIFILE_INIT) {}

	RsFileHash hash;
	Status stat;
};

class ftTransferModule 
{
public:
  ftTransferModule(ftFileCreator *fc, ftDataMultiplex *dm, ftController *c);
  ~ftTransferModule();

  //interface to download controller
  bool setFileSources(const std::list<RsPeerId>& peerIds);
  bool addFileSource(const RsPeerId& peerId);
  bool removeFileSource(const RsPeerId& peerId);
  bool setPeerState(const RsPeerId& peerId,uint32_t state,uint32_t maxRate);  //state = ONLINE/OFFLINE
  bool getFileSources(std::list<RsPeerId> &peerIds);
  bool getPeerState(const RsPeerId& peerId,uint32_t &state,uint32_t &tfRate);
  uint32_t getDataRate(const RsPeerId& peerId);
  bool cancelTransfer();
  bool cancelFileTransferUpward();
  bool completeFileTransfer();
  bool isCheckingHash() ;
  void forceCheck() ;

  //interface to multiplex module
  bool recvFileData(const RsPeerId& peerId, uint64_t offset, uint32_t chunk_size, void *data);
  void locked_requestData(const RsPeerId& peerId, uint64_t offset, uint32_t chunk_size);

  //interface to file creator
  bool locked_getChunk(const RsPeerId& peer_id,uint32_t size_hint,uint64_t &offset, uint32_t &chunk_size);
  bool locked_storeData(uint64_t offset, uint32_t chunk_size, void *data);

  int tick();

  const RsFileHash& hash() const { return mHash; }
  uint64_t    size() const { return mSize; }
 
  //internal used functions
  bool queryInactive();
  void adjustSpeed();

  DwlSpeed downloadPriority() const { return mPriority ; }
  void setDownloadPriority(DwlSpeed p) { mPriority =p ; }

  // read/reset the last time the transfer module was active (either wrote data, or was solicitaded by clients)
  time_t lastActvTimeStamp() ;
  void resetActvTimeStamp() ;

private:

  bool locked_tickPeerTransfer(peerInfo &info);
  bool locked_recvPeerData(peerInfo &info, uint64_t offset,
			uint32_t chunk_size, void *data);
  
  bool checkFile() ;
  bool checkCRC() ;
  
  /* These have independent Mutexes / are const locally (no Mutex protection)*/
  ftFileCreator *mFileCreator;
  ftDataMultiplex *mMultiplexor;
  ftController *mFtController;

  RsFileHash mHash;
  uint64_t    mSize;

  RsMutex tfMtx; /* below is mutex protected */

  std::list<RsPeerId>         mOnlinePeers;
  std::map<RsPeerId,peerInfo> mFileSources;
  	
  uint16_t     mFlag;  //2:file canceled, 1:transfer complete, 0: not complete, 3: checking hash, 4: checking chunks
  double desiredRate;
  double actualRate;

  time_t _last_activity_time_stamp ;

  ftFileStatus mFileStatus; //used for pause/resume file transfer

  HashThread *_hash_thread ;
  DwlSpeed mPriority ;	// transfer speed priority
};

#endif  //FT_TRANSFER_MODULE_HEADER
