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

/* 
 * ftDataMultiplexModule.
 *
 * This multiplexes the data from PQInterface to the ftTransferModules.
 */

#include "ft/ftdatamultiplex.h"
#include "ft/fttransfermodule.h"
#include "ft/ftfilecreator.h"
#include "ft/ftfileprovider.h"
#include "ft/ftsearch.h"

/* For Thread Behaviour */
const uint32_t DMULTIPLEX_MIN	= 10; /* 1ms sleep */
const uint32_t DMULTIPLEX_MAX   = 1000; /* 1 sec sleep */
const double   DMULTIPLEX_RELAX = 0.5; /* ??? */
 
ftClient::ftClient(ftTransferModule *module, ftFileCreator *creator)
	:mModule(module), mCreator(creator)
{
	return;
}

const uint32_t FT_DATA		= 0x0001;
const uint32_t FT_DATA_REQ	= 0x0002;

ftRequest::ftRequest(uint32_t type, std::string peerId, std::string hash, uint64_t size, uint64_t offset, uint32_t chunk, void *data)
	:mType(type), mPeerId(peerId), mHash(hash), mSize(size),
	mOffset(offset), mChunk(chunk), mData(data)
{
	return;
}

ftDataMultiplex::ftDataMultiplex(ftDataSend *server, ftSearch *search)
	:RsQueueThread(DMULTIPLEX_MIN, DMULTIPLEX_MAX, DMULTIPLEX_RELAX),
	mDataSend(server),  mSearch(search)
{
	return;
}

bool	ftDataMultiplex::addTransferModule(ftTransferModule *mod, ftFileCreator *f)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftClient>::iterator it;
	if (mClients.end() != (it = mClients.find(mod->hash())))
	{
		/* error */
		return false;
	}
	mClients[mod->hash()] = ftClient(mod, f);

	return true;
}
		
bool	ftDataMultiplex::removeTransferModule(std::string hash)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftClient>::iterator it;
	if (mClients.end() == (it = mClients.find(hash)))
	{
		/* error */
		return false;
	}
	mClients.erase(it);
	return true;
}


bool    ftDataMultiplex::FileUploads(std::list<std::string> &hashs)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftFileProvider *>::iterator sit;
	for(sit = mServers.begin(); sit != mServers.end(); sit++)
	{
		hashs.push_back(sit->first);
	}
	return true;
}
	
bool    ftDataMultiplex::FileDownloads(std::list<std::string> &hashs)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftClient>::iterator cit;
	for(cit = mClients.begin(); cit != mClients.end(); cit++)
	{
		hashs.push_back(cit->first);
	}
	return true;
}


bool    ftDataMultiplex::FileDetails(std::string hash, uint32_t hintsflag, FileInfo &info)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftFileProvider *>::iterator sit;
	sit = mServers.find(hash);
	if (sit != mServers.end())
	{
		(sit->second)->FileDetails(info);
		return true;
	}

	std::map<std::string, ftClient>::iterator cit;
	if (mClients.end() != (cit = mClients.find(hash)))
	{
		//(cit->second).mModule->FileDetails(info);
		(cit->second).mCreator->FileDetails(info);
		return true;
	}
	
	return false;
}

	/* data interface */

	/*************** SEND INTERFACE (calls ftDataSend) *******************/

	/* Client Send */
bool	ftDataMultiplex::sendDataRequest(std::string peerId, 
	std::string hash, uint64_t size, uint64_t offset, uint32_t chunksize)
{
	return mDataSend->sendDataRequest(peerId,hash,size,offset,chunksize);
}

	/* Server Send */
bool	ftDataMultiplex::sendData(std::string peerId, 
		std::string hash, uint64_t size, 
		uint64_t offset, uint32_t chunksize, void *data)
{
	return mDataSend->sendData(peerId,hash,size,offset,chunksize,data);
}


	/*************** RECV INTERFACE (provides ftDataRecv) ****************/

	/* Client Recv */
bool	ftDataMultiplex::recvData(std::string peerId, 
	std::string hash, uint64_t size, 
	uint64_t offset, uint32_t chunksize, void *data)
{
	/* Store in Queue */
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	mRequestQueue.push_back(
		ftRequest(FT_DATA,peerId,hash,size,offset,chunksize,data));

	return true;
}


	/* Server Recv */
bool	ftDataMultiplex::recvDataRequest(std::string peerId, 
		std::string hash, uint64_t size, 
		uint64_t offset, uint32_t chunksize)
{
	/* Store in Queue */
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	mRequestQueue.push_back(
		ftRequest(FT_DATA_REQ,peerId,hash,size,offset,chunksize,NULL));

	return true;
}


/*********** BACKGROUND THREAD OPERATIONS ***********/
bool 	ftDataMultiplex::workQueued()
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	if (mRequestQueue.size() > 0)
	{
		return true;
	}

	if (mSearchQueue.size() > 0)
	{
		return true;
	}

	return false;
}
	
bool 	ftDataMultiplex::doWork()
{
	bool doRequests = true;

	/* Handle All the current Requests */		
	while(doRequests)
	{
		ftRequest req;

		{
			RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
			if (mRequestQueue.size() == 0)
			{
				doRequests = false;
				continue;
			}

			req = mRequestQueue.front();
			mRequestQueue.pop_front();
		}

		/* MUTEX FREE */

		switch(req.mType)
		{
		  case FT_DATA:
			handleRecvData(req.mPeerId, req.mHash, req.mSize,
				req.mOffset, req.mChunk, req.mData);
			break;

		  case FT_DATA_REQ:
			handleRecvDataRequest(req.mPeerId, req.mHash,
				req.mSize,  req.mOffset, req.mChunk);
			break;

		  default:
			break;
		}
	}

	/* Only Handle One Search Per Period.... 
   	 * Lower Priority
	 */
	ftRequest req;

	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
		if (mSearchQueue.size() == 0)
		{
			/* Finished */
			return true;
		}

		req = mSearchQueue.front();
		mSearchQueue.pop_front();
	}

	handleSearchRequest(req.mPeerId, req.mHash, req.mSize, 
					req.mOffset, req.mChunk);

	return true;
}


bool	ftDataMultiplex::handleRecvData(std::string peerId, 
			std::string hash, uint64_t size, 
			uint64_t offset, uint32_t chunksize, void *data)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftClient>::iterator it;
	if (mClients.end() == (it = mClients.find(hash)))
	{
		/* error */
		return false;
	}
	
	(it->second).mModule->recvFileData(peerId, offset, chunksize, data);

	return true;
}


	/* called by ftTransferModule */
bool	ftDataMultiplex::handleRecvDataRequest(std::string peerId, 
			std::string hash, uint64_t size, 
			uint64_t offset, uint32_t chunksize)
{
	/**** Find Files *****/

	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftClient>::iterator cit;
	if (mClients.end() != (cit = mClients.find(hash)))
	{
		locked_handleServerRequest((cit->second).mCreator, 
					peerId, hash, size, offset, chunksize);
		return true;
	}
	
	std::map<std::string, ftFileProvider *>::iterator sit;
	if (mServers.end() != (sit = mServers.find(hash)))
	{
		locked_handleServerRequest(sit->second,
					peerId, hash, size, offset, chunksize);
		return true;
	}


	/* Add to Search Queue */
	mSearchQueue.push_back(
		ftRequest(FT_DATA_REQ, peerId, hash, 
				size, offset, chunksize, NULL));

	return true;
}

bool	ftDataMultiplex::locked_handleServerRequest(ftFileProvider *provider,
		std::string peerId, std::string hash, uint64_t size, 
			uint64_t offset, uint32_t chunksize)
{
	void *data = malloc(size);
	if (provider->getFileData(offset, chunksize, data))
	{
		/* send data out */
		sendData(peerId, hash, size, offset, chunksize, data);
		return true;
	}
	return false;
}


bool	ftDataMultiplex::handleSearchRequest(std::string peerId, 
			std::string hash, uint64_t size, 
			uint64_t offset, uint32_t chunksize)
{

	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		/* Check for bad requests */
		std::map<std::string, time_t>::iterator bit;
		if (mUnknownHashs.end() != (bit = mUnknownHashs.find(hash)))
		{
			/* We've previously rejected this one, so ignore */
			return false;
		}
	}


	/* 
	 * Do Actual search 
	 * Could be Cache File, Local or Extra
	 * (anywhere but remote really)
	 */

	FileInfo info;
	uint32_t hintflags = (RS_FILE_HINTS_CACHE |
				RS_FILE_HINTS_EXTRA |
				RS_FILE_HINTS_LOCAL |
				RS_FILE_HINTS_SPEC_ONLY);

	if (mSearch->search(hash, size, hintflags, info))
	{

		/* setup a new provider */
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		ftFileProvider *provider = 
			new ftFileProvider(info.path, size, hash);

		mServers[hash] = provider;

		/* handle request finally */
		locked_handleServerRequest(provider,
					peerId, hash, size, offset, chunksize);
		return true;
	}
	return false;
}





