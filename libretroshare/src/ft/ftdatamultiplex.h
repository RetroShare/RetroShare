/*
 * libretroshare/src/ft: ftdatamultiplex.h
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

#ifndef FT_DATA_MULTIPLEX_HEADER
#define FT_DATA_MULTIPLEX_HEADER

/* 
 * ftDataMultiplexModule.
 *
 * This multiplexes the data from PQInterface to the ftTransferModules.
 */

class ftTransferModule;
class ftFileProvider;
class ftFileCreator;
class ftSearch;

#include <string>
#include <list>
#include <map>
#include <inttypes.h>

#include "util/rsthreads.h"

#include "ft/ftdata.h"
#include "rsiface/rsfiles.h"


class ftClient
{
	public:

	ftClient() :mModule(NULL), mCreator(NULL) { return; }
	ftClient(ftTransferModule *module, ftFileCreator *creator);

	ftTransferModule *mModule;
	ftFileCreator    *mCreator;
};

class ftRequest
{
	public:

	ftRequest(uint32_t type, std::string peerId, std::string hash, uint64_t size, uint64_t offset, uint32_t chunk, void *data);

	ftRequest()
	:mType(0), mSize(0), mOffset(0), mChunk(0), mData(NULL) { return; }

	uint32_t mType;
	std::string mPeerId;
	std::string mHash;
	uint64_t mSize;
	uint64_t mOffset;
	uint32_t mChunk;
	void *mData;
};

	

class ftDataMultiplex: public ftDataRecv, public RsQueueThread
{

	public:

	ftDataMultiplex(std::string ownId, ftDataSend *server, ftSearch *search);

	/* ftController Interface */
bool	addTransferModule(ftTransferModule *mod, ftFileCreator *f);
bool	removeTransferModule(std::string hash);

	/* data interface */
        /* get Details of File Transfers */
bool    FileUploads(std::list<std::string> &hashs);
bool    FileDownloads(std::list<std::string> &hashs);
bool    FileDetails(std::string hash, uint32_t hintsflag, FileInfo &info);

	/*************** SEND INTERFACE (calls ftDataSend) *******************/

	/* Client Send */
bool	sendDataRequest(std::string peerId, std::string hash, uint64_t size, 
			uint64_t offset, uint32_t chunksize);

	/* Server Send */
bool	sendData(std::string peerId, std::string hash, uint64_t size, 
			uint64_t offset, uint32_t chunksize, void *data);


	/*************** RECV INTERFACE (provides ftDataRecv) ****************/

	/* Client Recv */
virtual bool	recvData(std::string peerId, std::string hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data);

	/* Server Recv */
virtual bool	recvDataRequest(std::string peerId, std::string hash, uint64_t size, uint64_t offset, uint32_t chunksize);


	protected:

	/* Overloaded from RsQueueThread */
virtual bool workQueued();
virtual bool doWork();

	private:

	/* Handling Job Queues */
bool    handleRecvData(std::string peerId,
                std::string hash, uint64_t size, 
		uint64_t offset, uint32_t chunksize, void *data);

bool    handleRecvDataRequest(std::string peerId,
                        std::string hash, uint64_t size,
			uint64_t offset, uint32_t chunksize);

bool    handleSearchRequest(std::string peerId,
                        std::string hash, uint64_t size,
			uint64_t offset, uint32_t chunksize);

	/* We end up doing the actual server job here */
bool    locked_handleServerRequest(ftFileProvider *provider, 
	std::string peerId, std::string hash, uint64_t size, 
	uint64_t offset, uint32_t chunksize);

	RsMutex dataMtx;

	std::map<std::string, ftClient> mClients;
	std::map<std::string, ftFileProvider *> mServers;

	std::list<ftRequest> mRequestQueue;
	std::list<ftRequest> mSearchQueue;
	std::map<std::string, time_t> mUnknownHashs;

	ftDataSend *mDataSend;
	ftSearch   *mSearch;
	std::string mOwnId;

        friend class ftServer;
};

#endif
