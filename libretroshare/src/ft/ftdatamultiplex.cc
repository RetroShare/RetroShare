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

/******
 * #define MPLEX_DEBUG 1
 *****/
 
ftClient::ftClient(ftTransferModule *module, ftFileCreator *creator)
	:mModule(module), mCreator(creator)
{
	return;
}

const uint32_t FT_DATA						= 0x0001;		// data cuhnk to be stored
const uint32_t FT_DATA_REQ					= 0x0002;		// data request to be treated
const uint32_t FT_CLIENT_CHUNK_MAP_REQ	= 0x0003;		// chunk map request to be treated by client
const uint32_t FT_SERVER_CHUNK_MAP_REQ	= 0x0004;		// chunk map reuqest to be treated by server

ftRequest::ftRequest(uint32_t type, std::string peerId, std::string hash, uint64_t size, uint64_t offset, uint32_t chunk, void *data)
	:mType(type), mPeerId(peerId), mHash(hash), mSize(size),
	mOffset(offset), mChunk(chunk), mData(data)
{
	return;
}

ftDataMultiplex::ftDataMultiplex(std::string ownId, ftDataSend *server, ftSearch *search)
	:RsQueueThread(DMULTIPLEX_MIN, DMULTIPLEX_MAX, DMULTIPLEX_RELAX),
	mDataSend(server),  mSearch(search), mOwnId(ownId)
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
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::FileDetails(";
	std::cerr << hash << ", " << hintsflag << ")";
	std::cerr << std::endl;
#endif

	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

	if(hintsflag & RS_FILE_HINTS_DOWNLOAD)
	{
		std::map<std::string, ftClient>::iterator cit;
		if (mClients.end() != (cit = mClients.find(hash)))
		{

#ifdef MPLEX_DEBUG
			std::cerr << "ftDataMultiplex::FileDetails()";
			std::cerr << " Found ftFileCreator!";
			std::cerr << std::endl;
#endif

			//(cit->second).mModule->FileDetails(info);
			(cit->second).mCreator->FileDetails(info);
			return true;
		}
	}

	if(hintsflag & RS_FILE_HINTS_UPLOAD)
	{
		std::map<std::string, ftFileProvider *>::iterator sit;
		sit = mServers.find(hash);
		if (sit != mServers.end())
		{

#ifdef MPLEX_DEBUG
			std::cerr << "ftDataMultiplex::FileDetails()";
			std::cerr << " Found ftFileProvider!";
			std::cerr << std::endl;
#endif

			(sit->second)->FileDetails(info);
			return true;
		}
	}


#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::FileDetails()";
	std::cerr << " Found nothing";
	std::cerr << std::endl;
#endif
	
	return false;
}

	/* data interface */

	/*************** SEND INTERFACE (calls ftDataSend) *******************/

	/* Client Send */
bool	ftDataMultiplex::sendDataRequest(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize)
{
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::sendDataRequest() Client Send";
	std::cerr << std::endl;
#endif
	return mDataSend->sendDataRequest(peerId,hash,size,offset,chunksize);
}

	/* Server Send */
bool	ftDataMultiplex::sendData(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data)
{
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::sendData() Server Send";
	std::cerr << std::endl;
#endif
	return mDataSend->sendData(peerId,hash,size,offset,chunksize,data);
}


	/*************** RECV INTERFACE (provides ftDataRecv) ****************/

	/* Client Recv */
bool	ftDataMultiplex::recvData(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data)
{
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::recvData() Client Recv";
	std::cerr << std::endl;
#endif
	/* Store in Queue */
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	mRequestQueue.push_back(ftRequest(FT_DATA,peerId,hash,size,offset,chunksize,data));

	return true;
}


	/* Server Recv */
bool	ftDataMultiplex::recvDataRequest(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize)
{
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::recvDataRequest() Server Recv";
	std::cerr << std::endl;
#endif
	/* Store in Queue */
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	mRequestQueue.push_back(
		ftRequest(FT_DATA_REQ,peerId,hash,size,offset,chunksize,NULL));

	return true;
}

bool	ftDataMultiplex::recvChunkMapRequest(const std::string& peerId, const std::string& hash,bool is_client)
{
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::recvChunkMapRequest() Server Recv";
	std::cerr << std::endl;
#endif
	/* Store in Queue */
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

	if(is_client)
		mRequestQueue.push_back(ftRequest(FT_CLIENT_CHUNK_MAP_REQ,peerId,hash,0,0,0,NULL));
	else
		mRequestQueue.push_back(ftRequest(FT_SERVER_CHUNK_MAP_REQ,peerId,hash,0,0,0,NULL));

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
#ifdef MPLEX_DEBUG
				std::cerr << "ftDataMultiplex::doWork() Handling FT_DATA";
				std::cerr << std::endl;
#endif
				handleRecvData(req.mPeerId, req.mHash, req.mSize, req.mOffset, req.mChunk, req.mData);
				break;

			case FT_DATA_REQ:
#ifdef MPLEX_DEBUG
				std::cerr << "ftDataMultiplex::doWork() Handling FT_DATA_REQ";
				std::cerr << std::endl;
#endif
				handleRecvDataRequest(req.mPeerId, req.mHash, req.mSize,  req.mOffset, req.mChunk);
				break;

			case FT_CLIENT_CHUNK_MAP_REQ:
#ifdef MPLEX_DEBUG
				std::cerr << "ftDataMultiplex::doWork() Handling FT_CLIENT_CHUNK_MAP_REQ";
				std::cerr << std::endl;
#endif
				handleRecvClientChunkMapRequest(req.mPeerId,req.mHash) ;
				break ;

			case FT_SERVER_CHUNK_MAP_REQ:
#ifdef MPLEX_DEBUG
				std::cerr << "ftDataMultiplex::doWork() Handling FT_CLIENT_CHUNK_MAP_REQ";
				std::cerr << std::endl;
#endif
				handleRecvServerChunkMapRequest(req.mPeerId,req.mHash) ;
				break ;

			default:
#ifdef MPLEX_DEBUG
				std::cerr << "ftDataMultiplex::doWork() Ignoring UNKNOWN";
				std::cerr << std::endl;
#endif
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

#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::doWork() Handling Search Request";
	std::cerr << std::endl;
#endif
	if(handleSearchRequest(req.mPeerId, req.mHash))
		handleRecvDataRequest(req.mPeerId, req.mHash, req.mSize, req.mOffset, req.mChunk) ;

	return true;
}

// A chunk map has arrived. It can be two different situations:
// - an uploader has sent his chunk map, so we need to store it in the corresponding ftFileProvider
// - a source for a download has sent his chunk map, so we need to send it to the corresponding ftFileCreator.
//
bool ftDataMultiplex::recvChunkMap(const std::string& peerId, const std::string& hash,const CompressedChunkMap& compressed_map,bool client)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

	if(client)	// is the chunk map for a client, or for a server ?
	{
		std::map<std::string, ftClient>::iterator it = mClients.find(hash);

		if(it == mClients.end())
		{
#ifdef MPLEX_DEBUG
			std::cerr << "ftDataMultiplex::recvChunkMap() ERROR: No matching Client for hash " << hash << " !";
			std::cerr << std::endl;
#endif
			/* error */
			return false;
		}

#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::recvChunkMap() Passing map of file " << hash << ", to FT Module";
		std::cerr << std::endl;
#endif

		(it->second).mCreator->setSourceMap(peerId, compressed_map);
		return true ;
	}
	else
	{
		std::map<std::string, ftFileProvider *>::iterator it = mServers.find(hash) ;

		if(it == mServers.end())
		{
#ifdef MPLEX_DEBUG
			std::cerr << "ftDataMultiplex::handleRecvChunkMap() ERROR: No matching file Provider for hash " << hash ;
			std::cerr << std::endl;
#endif
			return false;
		}

		it->second->setClientMap(peerId, compressed_map);
		return true ;
	}

	return false;
}

bool ftDataMultiplex::handleRecvClientChunkMapRequest(const std::string& peerId, const std::string& hash)
{
	CompressedChunkMap cmap ;

	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		std::map<std::string, ftClient>::iterator it = mClients.find(hash);

		if(it == mClients.end())
		{
			// If we can't find the client, it's not a problem. Chunk maps from
			// clients are not essential, as they are only used for display.
#ifdef MPLEX_DEBUG
			std::cerr << "ftDataMultiplex::handleRecvServerChunkMapRequest() ERROR: No matching Client for hash " << hash ;
			std::cerr << ". Performing local search." << std::endl;
#endif
			return false;
		}

#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::handleRecvServerChunkMapRequest() Sending map of file " << hash << ", to peer " << peerId << std::endl;
#endif

		(it->second).mCreator->getAvailabilityMap(cmap);
	}

	mDataSend->sendChunkMap(peerId,hash,cmap);

	return true ;
}

bool ftDataMultiplex::handleRecvServerChunkMapRequest(const std::string& peerId, const std::string& hash)
{
	CompressedChunkMap cmap ;
	std::map<std::string, ftFileProvider *>::iterator it ;
	bool found = true ;

	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		it = mServers.find(hash) ;

		if(it == mServers.end())
			found = false ;
	}

	if(!found)
	{
#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::handleRecvChunkMapReq() ERROR: No matching file Provider for hash " << hash ;
		std::cerr << std::endl;
#endif
		if(!handleSearchRequest(peerId,hash))	
			return false ;

#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::handleRecvChunkMapReq() A new file Provider has been made up for hash " << hash ;
		std::cerr << std::endl;
#endif
	}

	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		it = mServers.find(hash) ;

		if(it == mServers.end())	// handleSearchRequest should have filled mServers[hash], but we have been off-mutex since,
			return false ;				// so it's safer to check again.
		else
			it->second->getAvailabilityMap(cmap);
	}

	mDataSend->sendChunkMap(peerId,hash,cmap);

	return true;
}

bool	ftDataMultiplex::handleRecvData(const std::string& peerId, 
			const std::string& hash, uint64_t size, 
			uint64_t offset, uint32_t chunksize, void *data)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftClient>::iterator it;
	if (mClients.end() == (it = mClients.find(hash)))
	{
#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::handleRecvData() ERROR: No matching Client!";
		std::cerr << std::endl;
#endif
		/* error */
		return false;
	}

#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::handleRecvData() Passing to Module";
	std::cerr << std::endl;
#endif
	
	(it->second).mModule->recvFileData(peerId, offset, chunksize, data);

	return true;
}


	/* called by ftTransferModule */
bool	ftDataMultiplex::handleRecvDataRequest(const std::string& peerId, const std::string& hash, uint64_t size, uint64_t offset, uint32_t chunksize)
{
	/**** Find Files *****/

	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
	std::map<std::string, ftClient>::iterator cit;
	if (mOwnId == peerId)
	{
		/* own requests must be passed to Servers */
#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::handleRecvData() OwnId, so skip Clients...";
		std::cerr << std::endl;
#endif
	}
	else if (mClients.end() != (cit = mClients.find(hash)))
	{
#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::handleRecvData() Matched to a Client.";
		std::cerr << std::endl;
#endif
		locked_handleServerRequest((cit->second).mCreator, peerId, hash, size, offset, chunksize);
		return true;
	}
	
	std::map<std::string, ftFileProvider *>::iterator sit;
	if (mServers.end() != (sit = mServers.find(hash)))
	{
#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::handleRecvData() Matched to a Provider.";
		std::cerr << std::endl;
#endif
		locked_handleServerRequest(sit->second, peerId, hash, size, offset, chunksize);
		return true;
	}

#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::handleRecvData() No Match... adding to Search Queue.";
	std::cerr << std::endl;
#endif

	/* Add to Search Queue */
	mSearchQueue.push_back( ftRequest(FT_DATA_REQ, peerId, hash, size, offset, chunksize, NULL));

	return true;
}

bool	ftDataMultiplex::locked_handleServerRequest(ftFileProvider *provider,
		std::string peerId, std::string hash, uint64_t size, 
			uint64_t offset, uint32_t chunksize)
{
	if(chunksize > std::min(size,uint64_t(10*1024*1024)))
	{
		std::cerr << "Warning: peer " << peerId << " is asking a large chunk (s=" << chunksize << ") for hash " << hash << ", filesize=" << size << ". This is unexpected." << std::endl ;
		return false ;
	}
	void *data = malloc(chunksize);

	if(data == NULL)
	{
		std::cerr << "WARNING: Could not allocate data for a chunksize of " << chunksize << std::endl ;
		return false ;
	}
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::locked_handleServerRequest()";
	std::cerr << "\t peer: " << peerId << " hash: " << hash;
	std::cerr << " size: " << size;
	std::cerr << std::endl;
	std::cerr << "\t offset: " << offset;
	std::cerr << " chunksize: " << chunksize << " data: " << data;
	std::cerr << std::endl;
#endif

	if (provider->getFileData(offset, chunksize, data))
	{
		// setup info
		provider->setPeerId(peerId) ;
		/* send data out */
		sendData(peerId, hash, size, offset, chunksize, data);
		return true;
	}
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::locked_handleServerRequest()";
	std::cerr << " FAILED";
	std::cerr << std::endl;
#endif
	free(data);

	return false;
}

bool ftDataMultiplex::getClientChunkMap(const std::string& upload_hash,const std::string& peerId,CompressedChunkMap& cmap)
{
	bool too_old = false;
	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		std::map<std::string,ftFileProvider *>::iterator sit = mServers.find(upload_hash); 

		if(mServers.end() == sit)
			return false ;

		sit->second->getClientMap(peerId,cmap,too_old) ;
	}

	// If the map is too old then we should ask an other map to the peer.
	//
	if(too_old)
		sendChunkMapRequest(peerId,upload_hash);

	return true ;
}

bool ftDataMultiplex::sendChunkMapRequest(const std::string& peer_id,const std::string& hash)
{
	return mDataSend->sendChunkMapRequest(peer_id,hash);
}

void ftDataMultiplex::deleteUnusedServers()
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

	//scan the uploads list in ftdatamultiplex and delete the items which time out
	time_t now = time(NULL);
	
	for(std::map<std::string, ftFileProvider *>::iterator sit(mServers.begin());sit != mServers.end();)
			if ((now - sit->second->lastTS) > 10)
			{
#ifdef SERVER_DEBUG
				std::cout << "info.lastTS = " << info.lastTS << ", now=" << now << std::endl ;
#endif
				// We don't delete servers that are clients at the same time !
				if(dynamic_cast<ftFileCreator*>(sit->second) == NULL)
					delete sit->second;

				std::map<std::string, ftFileProvider *>::iterator tmp(sit);
				++tmp ;

				mServers.erase(sit);

				sit = tmp ;
			}
			else
				++sit ;
}

bool	ftDataMultiplex::handleSearchRequest(const std::string& peerId, const std::string& hash)
{
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::handleSearchRequest(";
	std::cerr << peerId << ", " << hash << "...)";
	std::cerr << std::endl;
#endif

//	{
//		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
//
//		/* Check for bad requests */
//		std::map<std::string, time_t>::iterator bit;
//		if (mUnknownHashs.end() != (bit = mUnknownHashs.find(hash)))
//		{
//
//#ifdef MPLEX_DEBUG
//		std::cerr << "ftDataMultiplex::handleSearchRequest(";
//		std::cerr << " Found Ignore Hash ... done";
//		std::cerr << std::endl;
//#endif
//
//			/* We've previously rejected this one, so ignore */
//			return false;
//		}
//	}


	/* 
	 * Do Actual search 
	 * Could be Cache File, Local or Extra
	 * (anywhere but remote really)
	 */

	FileInfo info;
	uint32_t hintflags = (RS_FILE_HINTS_CACHE | RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_SPEC_ONLY);

	if (mSearch->search(hash, hintflags, info))
	{

#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::handleSearchRequest(";
		std::cerr << " Found Local File, sharing...";
		std::cerr << std::endl;
#endif

		/* setup a new provider */
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		ftFileProvider *provider = new ftFileProvider(info.path, info.size, hash);

		mServers[hash] = provider;

		return true;
	}
	// Now check wether the required file is actually being downloaded. In such a case, 
	// setup the file provider to be the file creator itself. Warning: this server should not 
	// be deleted when not used anymore. We need to restrict this to client peers that are
	// not ourself, since the file transfer also handles the local cache traffic (this
	// is something to be changed soon!!)
	//

	if(peerId != mOwnId)
	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		std::map<std::string,ftClient>::const_iterator it(mClients.find(hash)) ;
		
		if(it != mClients.end())
		{
			mServers[hash] = it->second.mCreator ;
			return true;
		}
	}
	
	return false;
}





