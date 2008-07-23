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

ftClient::ftClient(ftTransferModule *module, ftFileCreator *creator)
	:mModule(module), mCreator(creator)
{
	return;
}

const uint32_t FT_DATA		= 0x0001;
const uint32_t FT_DATA_REQ	= 0x0002;

ftRequest::ftRequest(uint32_t type, std::string peerId, std::string hash, uint64_t offset, uint32_t chunk, void *data)
	:mType(type), mPeerId(peerId), mHash(hash),
	mOffset(offset), mChunk(chunk), mData(data)
{
	return;
}

ftDataMultiplex::ftDataMultiplex(ftDataSend *server, ftSearch *search)
	:mDataSend(server), mSearch(search)
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
		
bool	ftDataMultiplex::removeTransferModule(ftTransferModule *mod, ftFileCreator *f)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftClient>::iterator it;
	if (mClients.end() == (it = mClients.find(mod->hash())))
	{
		/* error */
		return false;
	}
	mClients.erase(it);
	return true;
}


	/* data interface */

	/*************** SEND INTERFACE (calls ftDataSend) *******************/

	/* Client Send */
bool	ftDataMultiplex::sendDataRequest(std::string peerId, std::string hash, uint64_t offset, uint32_t size)
{
	return mDataSend->sendDataRequest(peerId, hash, offset, size);
}

	/* Server Send */
bool	ftDataMultiplex::sendData(std::string peerId, std::string hash, uint64_t offset, uint32_t size, void *data)
{
	return mDataSend->sendData(peerId, hash, offset, size, data);
}


	/*************** RECV INTERFACE (provides ftDataRecv) ****************/

	/* Client Recv */
bool	ftDataMultiplex::recvData(std::string peerId, std::string hash, uint64_t offset, uint32_t size, void *data)
{
	/* Store in Queue */
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	mRequestQueue.push_back(
		ftRequest(FT_DATA, peerId, hash, offset, size, data));

	return true;
}


	/* Server Recv */
bool	ftDataMultiplex::recvDataRequest(std::string peerId, std::string hash, uint64_t offset, uint32_t size)
{
	/* Store in Queue */
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	mRequestQueue.push_back(
		ftRequest(FT_DATA_REQ, peerId, hash, offset, size, NULL));

	return true;
}


/*********** BACKGROUND THREAD OPERATIONS ***********/


bool	ftDataMultiplex::handleRecvData(std::string peerId, 
		std::string hash, uint64_t offset, uint32_t size, void *data)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftClient>::iterator it;
	if (mClients.end() == (it = mClients.find(hash)))
	{
		/* error */
		return false;
	}
	
	(it->second).mModule->recvFileData(peerId, offset, size, data);

	return true;
}


	/* called by ftTransferModule */
bool	ftDataMultiplex::handleRecvDataRequest(std::string peerId, 
			std::string hash, uint64_t offset, uint32_t size)
{
	/**** Find Files *****/

	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftClient>::iterator cit;
	if (mClients.end() != (cit = mClients.find(hash)))
	{
		locked_handleServerRequest((cit->second).mCreator, 
					peerId, hash, offset, size);
		return true;
	}
	
	std::map<std::string, ftFileProvider *>::iterator sit;
	if (mServers.end() != (sit = mServers.find(hash)))
	{
		locked_handleServerRequest(sit->second,
					peerId, hash, offset, size);
		return true;
	}


	/* Add to Search Queue */
	mSearchQueue.push_back(
		ftRequest(FT_DATA_REQ, peerId, hash, offset, size, NULL));

	return true;
}

bool	ftDataMultiplex::locked_handleServerRequest(ftFileProvider *provider,
	std::string peerId, std::string hash, uint64_t offset, uint32_t size)
{
	void *data = malloc(size);
	if (provider->getFileData(offset, size, data))
	{
		/* send data out */
		sendData(peerId, hash, offset, size, data);
		return true;
	}
	return false;
}


bool	ftDataMultiplex::handleSearchRequest(std::string peerId, 
			std::string hash, uint64_t offset, uint32_t size)
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
					peerId, hash, offset, size);
		return true;
	}
	return false;
}





