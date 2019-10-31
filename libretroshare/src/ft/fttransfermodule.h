/*******************************************************************************
 * libretroshare/src/ft: fttransfermodule.h                                    *
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
	explicit peerInfo(const RsPeerId& peerId_in);

//	peerInfo(const RsPeerId& peerId_in,uint32_t state_in,uint32_t maxRate_in):
//		peerId(peerId_in),state(state_in),desiredRate(maxRate_in),actualRate(0),
//		lastTS(0),
//		recvTS(0), lastTransfers(0), nResets(0),
//		rtt(0), rttActive(false), rttStart(0), rttOffset(0),
//		mRateIncrease(1)
//	{
//		return;
//	}
  	RsPeerId peerId;
  	uint32_t state;
  	double desiredRate;        /* speed at which the data should be requested */
  	double actualRate;	       /* actual speed at which the data is received  */

  	rstime_t lastTS;           /* last Request */
	rstime_t recvTS;           /* last Recv */
	uint32_t lastTransfers;    /* data recvd in last second */
	uint32_t nResets;          /* count to disable non-existant files */

	uint32_t rtt;              /* last rtt */
	bool     rttActive;        /* have we initialised an rtt measurement */
	rstime_t	 rttStart;     /* ts of request */
	uint64_t rttOffset;        /* end of request */
	float    mRateIncrease;    /* current rate increase factor */
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
  rstime_t lastActvTimeStamp() ;
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

  rstime_t _last_activity_time_stamp ;

  ftFileStatus mFileStatus; //used for pause/resume file transfer

  HashThread *_hash_thread ;
  DwlSpeed mPriority ;	// transfer speed priority
};

#endif  //FT_TRANSFER_MODULE_HEADER
