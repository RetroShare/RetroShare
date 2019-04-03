/*******************************************************************************
 * libretroshare/src/ft: ftdatamultiplex.cc                                    *
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
#include "util/rsdir.h"
#include "util/rsmemory.h"
#include <retroshare/rsturtle.h>
#include "util/rstime.h"

/* For Thread Behaviour */
const uint32_t DMULTIPLEX_MIN	= 10; /* 10 msec sleep */
const uint32_t DMULTIPLEX_MAX   = 1000; /* 1 sec sleep */
const double   DMULTIPLEX_RELAX = 0.5; /* relax factor to calculate sleep time if not working in /libretroshare/src/util/rsthreads.cc */

static const uint32_t MAX_CHECKING_CHUNK_WAIT_DELAY   = 120 ; //! TTL for an inactive chunk
const uint32_t MAX_SIMULTANEOUS_CRC_REQUESTS = 20 ;

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
const uint32_t FT_SERVER_CHUNK_MAP_REQ	= 0x0004;		// chunk map request to be treated by server
//const uint32_t FT_CRC32MAP_REQ        	= 0x0005;		// crc32 map request to be treated by server
const uint32_t FT_CLIENT_CHUNK_CRC_REQ	= 0x0006;		// chunk sha1 crc request to be treated

ftRequest::ftRequest(uint32_t type, const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunk, void *data)
	:mType(type), mPeerId(peerId), mHash(hash), mSize(size),
	mOffset(offset), mChunk(chunk), mData(data)
{
	return;
}

ftDataMultiplex::ftDataMultiplex(const RsPeerId& ownId, ftDataSend *server, ftSearch *search)
	:RsQueueThread(DMULTIPLEX_MIN, DMULTIPLEX_MAX, DMULTIPLEX_RELAX), dataMtx("ftDataMultiplex"),
	mDataSend(server),  mSearch(search), mOwnId(ownId)
{
	return;
}

bool ftDataMultiplex::getFileData(const RsFileHash& hash, uint64_t offset, uint32_t& requested_size, uint8_t *data)
{
    RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
    ftFileProvider* provider = 0;

    std::map<RsFileHash, ftClient>::iterator cit;
    std::map<RsFileHash, ftFileProvider *>::iterator sit;

    // check if file is currently downloading
    if (mClients.end() != (cit = mClients.find(hash)))
        provider = (cit->second).mCreator;

    // else check if its already uploading
    else if (mServers.end() != (sit = mServers.find(hash)))
        provider = sit->second;

    // else create a new provider
    else
    {
        FileInfo info;
        FileSearchFlags hintflags =   RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_SPEC_ONLY | RS_FILE_HINTS_NETWORK_WIDE;
        if(mSearch->search(hash, hintflags, info))
        {
            provider = new ftFileProvider(info.path, info.size, hash);
            mServers[hash] = provider;
        }
    }

    if(!provider || ! provider->getFileData(mOwnId, offset, requested_size, data, true))
    {
        requested_size = 0 ;
        return false ;
    }
    return true ;
}

bool	ftDataMultiplex::addTransferModule(ftTransferModule *mod, ftFileCreator *f)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
    std::map<RsFileHash, ftClient>::iterator it;
	if (mClients.end() != (it = mClients.find(mod->hash())))
	{
		/* error */
		return false;
	}
	mClients[mod->hash()] = ftClient(mod, f);

	return true;
}
		
bool	ftDataMultiplex::removeTransferModule(const RsFileHash& hash)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

    std::map<RsFileHash, ftClient>::iterator it;
	if (mClients.end() == (it = mClients.find(hash)))
	{
		/* error */
		return false;
	}
	mClients.erase(it);

	// This is very important to delete the hash from servers as well, because
	// after removing the transfer module, ftController will delete the fileCreator.
	// If the file creator is also a server in use, then it will cause a crash
	// at the next server request. 
	//
	// With the current action, the next server request will re-create the server as
	// a ftFileProvider.
	//
    std::map<RsFileHash, ftFileProvider*>::iterator sit = mServers.find(hash) ;

	if(sit != mServers.end())
		mServers.erase(sit);

	return true;
}


bool    ftDataMultiplex::FileUploads(std::list<RsFileHash> &hashs)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
    std::map<RsFileHash, ftFileProvider *>::iterator sit;
	for(sit = mServers.begin(); sit != mServers.end(); ++sit)
	{
		hashs.push_back(sit->first);
	}
	return true;
}
	
bool    ftDataMultiplex::FileDownloads(std::list<RsFileHash> &hashs)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
    std::map<RsFileHash, ftClient>::iterator cit;
	for(cit = mClients.begin(); cit != mClients.end(); ++cit)
	{
		hashs.push_back(cit->first);
	}
	return true;
}


bool    ftDataMultiplex::FileDetails(const RsFileHash &hash, FileSearchFlags hintsflag, FileInfo &info)
{
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::FileDetails(";
	std::cerr << hash << ", " << hintsflag << ")";
	std::cerr << std::endl;
#endif

	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

	if(hintsflag & RS_FILE_HINTS_DOWNLOAD)
	{
        std::map<RsFileHash, ftClient>::iterator cit;
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
        std::map<RsFileHash, ftFileProvider *>::iterator sit;
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
bool	ftDataMultiplex::sendDataRequest(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize)
{
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::sendDataRequest() Client Send";
	std::cerr << std::endl;
#endif
	return mDataSend->sendDataRequest(peerId,hash,size,offset,chunksize);
}

	/* Server Send */
bool	ftDataMultiplex::sendData(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data)
{
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::sendData() Server Send";
	std::cerr << std::endl;
#endif
	return mDataSend->sendData(peerId,hash,size,offset,chunksize,data);
}


	/*************** RECV INTERFACE (provides ftDataRecv) ****************/

	/* Client Recv */
bool	ftDataMultiplex::recvData(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data)
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
bool	ftDataMultiplex::recvDataRequest(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize)
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

bool	ftDataMultiplex::recvChunkMapRequest(const RsPeerId& peerId, const RsFileHash& hash,bool is_client)
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

bool	ftDataMultiplex::recvSingleChunkCRCRequest(const RsPeerId& peerId, const RsFileHash& hash,uint32_t chunk_number)
{
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::recvChunkMapRequest() Server Recv";
	std::cerr << std::endl;
#endif
	/* Store in Queue */
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

	mRequestQueue.push_back(ftRequest(FT_CLIENT_CHUNK_CRC_REQ,peerId,hash,0,0,chunk_number,NULL));

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
			if (mRequestQueue.empty())
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

			case FT_CLIENT_CHUNK_CRC_REQ:
#ifdef MPLEX_DEBUG
				std::cerr << "ftDataMultiplex::doWork() Handling FT_CLIENT_CHUNK_CRC_REQ";
				std::cerr << std::endl;
#endif
				handleRecvChunkCrcRequest(req.mPeerId,req.mHash,req.mChunk) ;
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
		if (mSearchQueue.empty())
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

bool ftDataMultiplex::recvSingleChunkCRC(const RsPeerId& peerId, const RsFileHash& hash,uint32_t chunk_number,const Sha1CheckSum& crc)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::recvSingleChunkCrc() Received crc of file " << hash << ", from peer id " << peerId << ", chunk " << chunk_number << ", crc=" << crc.toStdString() << std::endl;
#else
	(void) peerId;
#endif
	// remove this chunk from the request list as well.
	
	Sha1CacheEntry& sha1cache(_cached_sha1maps[hash]) ;
	std::map<uint32_t,std::pair<rstime_t,ChunkCheckSumSourceList> >::iterator it2(sha1cache._to_ask.find(chunk_number)) ;

	if(it2 != sha1cache._to_ask.end())
		sha1cache._to_ask.erase(it2) ;

	// update the cache: get size from the client.

    std::map<RsFileHash, ftClient>::iterator it = mClients.find(hash);

	if(it == mClients.end())
	{
		std::cerr << "ftDataMultiplex::recvSingleChunkCrc() ERROR: No matching Client for CRC. This is an error. " << hash << " !" << std::endl;
		/* error */
		return false;
	}

	// store in the cache as well

	if(sha1cache._map.size() == 0)
		sha1cache._map = Sha1Map(it->second.mCreator->fileSize(),ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE) ;

	sha1cache._map.set(chunk_number,crc) ;

	sha1cache._received.push_back(chunk_number) ;

#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::recvSingleChunkCrc() stored in cache. " << std::endl;
#endif

	return true ;
}

bool ftDataMultiplex::dispatchReceivedChunkCheckSum()
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

	uint32_t MAX_CHECKSUM_CHECK_PER_FILE = 25 ;

    for(std::map<RsFileHash,Sha1CacheEntry>::iterator it(_cached_sha1maps.begin());it!=_cached_sha1maps.end();)
	{
        std::map<RsFileHash, ftClient>::iterator itc = mClients.find(it->first);

#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::dispatchReceivedChunkCheckSum(): treating hash " << it->first << std::endl;
#endif

		if(itc == mClients.end())
		{
#ifdef MPLEX_DEBUG
			std::cerr << "ftDataMultiplex::dispatchReceivedChunkCheckSum() ERROR: No matching Client for hash. This is probably a late answer. Dropping the hash. Hash=" << it->first << std::endl;
#endif

            std::map<RsFileHash,Sha1CacheEntry>::iterator tmp(it) ;
			++tmp ;
			_cached_sha1maps.erase(it) ;
			it = tmp ;
			/* error */
			continue ;
		}
		ftFileCreator *client = itc->second.mCreator ;

		for(uint32_t n=0;n<MAX_CHECKSUM_CHECK_PER_FILE && !it->second._received.empty();++n)
		{
			int chunk_number = it->second._received.back() ;

			if(!it->second._map.isSet(chunk_number))
				std::cerr << "ftDataMultiplex::dispatchReceivedChunkCheckSum() ERROR: chunk " << chunk_number << " is supposed to be initialized but it was not received !!" << std::endl;
			else
			{
#ifdef MPLEX_DEBUG
				std::cerr << "ftDataMultiplex::dispatchReceivedChunkCheckSum(): checking chunk " << chunk_number << " with hash " << it->second._map[chunk_number].toStdString() << std::endl;
#endif
				client->verifyChunk(chunk_number,it->second._map[chunk_number]) ;
			}
			it->second._received.pop_back() ;
		}
		++it ;
	}
	return true ;
}

// A chunk map has arrived. It can be two different situations:
// - an uploader has sent his chunk map, so we need to store it in the corresponding ftFileProvider
// - a source for a download has sent his chunk map, so we need to send it to the corresponding ftFileCreator.
//
bool ftDataMultiplex::recvChunkMap(const RsPeerId& peerId, const RsFileHash& hash,const CompressedChunkMap& compressed_map,bool client)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

	if(client)	// is the chunk map for a client, or for a server ?
	{
        std::map<RsFileHash, ftClient>::iterator it = mClients.find(hash);

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
        std::map<RsFileHash, ftFileProvider *>::iterator it = mServers.find(hash) ;

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

bool ftDataMultiplex::handleRecvClientChunkMapRequest(const RsPeerId& peerId, const RsFileHash& hash)
{
	CompressedChunkMap cmap ;

	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

        std::map<RsFileHash, ftClient>::iterator it = mClients.find(hash);

		if(it == mClients.end())
		{
			// If we can't find the client, it's not a problem. Chunk maps from
			// clients are not essential, as they are only used for display.
#ifdef MPLEX_DEBUG
			std::cerr << "ftDataMultiplex::handleRecvClientChunkMapRequest() ERROR: No matching Client for hash " << hash ;
			std::cerr << ". Performing local search." << std::endl;
#endif
			return false;
		}

#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::handleRecvClientChunkMapRequest() Sending map of file " << hash << ", to peer " << peerId << std::endl;
#endif

		(it->second).mCreator->getAvailabilityMap(cmap);
	}

	mDataSend->sendChunkMap(peerId,hash,cmap,false);

	return true ;
}

bool ftDataMultiplex::handleRecvChunkCrcRequest(const RsPeerId& peerId, const RsFileHash& hash, uint32_t chunk_number)
{
	// look into the sha1sum cache
	
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::handleRecvChunkMapReq() looking for chunk " << chunk_number << " for hash " << hash << std::endl;
#endif

	Sha1CheckSum crc ;
	bool found = false ;

	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		Sha1CacheEntry& sha1cache(_cached_sha1maps[hash]) ;
		sha1cache.last_activity = time(NULL) ;	// update time_stamp

		if(sha1cache._map.size() > 0 && sha1cache._map.isSet(chunk_number))
		{
			crc = sha1cache._map[chunk_number] ;
			found = true  ;
		}
	}

	if(found)
	{
#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::handleRecvChunkMapReq() found in cache ! Sending " << crc.toStdString() << std::endl;
#endif
		mDataSend->sendSingleChunkCRC(peerId,hash,chunk_number,crc);
		return true ;
	}

    std::map<RsFileHash, ftFileProvider *>::iterator it ;
	std::string filename ;
	uint64_t filesize =0;
	found = true ;

	// 1 - look into the list of servers.Not clients ! Clients dont' have verified data.
	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		it = mServers.find(hash) ;

		if(it == mServers.end())
			found = false ;
	}

	// 2 - if not found, create a server.
	//
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
		{
				std::cerr << "Could definitely not find a provider for file " << hash << ". Maybe the file does not exist?" << std::endl;
				return false ;				// so it's safer to check again.
		}
		else
		{
			filesize = it->second->fileSize() ;
			filename = it->second->fileName() ;
		}
	}

#ifdef MPLEX_DEBUG
	std::cerr << "Computing Sha1 for chunk " << chunk_number<< " of file " << filename << ", hash=" << hash << ", size=" << filesize << std::endl;
#endif

	unsigned char *buf = new unsigned char[ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE] ;
	FILE *fd = RsDirUtil::rs_fopen(filename.c_str(),"rb") ;

	if(fd == NULL)
	{
		std::cerr << "Cannot read file " << filename << ". Something's wrong!" << std::endl;
		delete[] buf ;
		return false ;
	}
	uint32_t len ;
	if(fseeko64(fd,(uint64_t)chunk_number * (uint64_t)ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE,SEEK_SET)!=0 || 0==(len = fread(buf,1,ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE,fd))) 
	{
		std::cerr << "Cannot fseek/read from file " << filename << " at position " << (uint64_t)chunk_number * (uint64_t)ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE << std::endl;
		fclose(fd) ;

		delete[] buf ;
		return false ;
	}
	fclose(fd) ;

	crc = RsDirUtil::sha1sum(buf,len) ;
	delete[] buf ;

	// update cache
	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		Sha1CacheEntry& sha1cache(_cached_sha1maps[hash]) ;

		if(sha1cache._map.size() == 0)
			sha1cache._map = Sha1Map(filesize,ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE) ;

		sha1cache._map.set(chunk_number,crc) ;
	}
#ifdef MPLEX_DEBUG
	std::cerr << "Sending CRC of chunk " << chunk_number<< " of file " << filename << ", hash=" << hash << ", size=" << filesize << ", crc=" << crc.toStdString() << std::endl;
#endif

	mDataSend->sendSingleChunkCRC(peerId,hash,chunk_number,crc);
	return true ;
}

bool ftDataMultiplex::handleRecvServerChunkMapRequest(const RsPeerId& peerId, const RsFileHash& hash)
{
	CompressedChunkMap cmap ;
    std::map<RsFileHash, ftFileProvider *>::iterator it ;
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
		{
			std::cerr << "ftDataMultiplex::handleRecvChunkMapReq() : weird state: search request succeeded, but no server available!" << std::endl;
			return false ;				// so it's safer to check again.
		}
		else
			it->second->getAvailabilityMap(cmap);
	}

	mDataSend->sendChunkMap(peerId,hash,cmap,true);

	return true;
}

bool	ftDataMultiplex::handleRecvData(const RsPeerId& peerId, const RsFileHash& hash, uint64_t /*size*/, uint64_t offset, uint32_t chunksize, void *data)
{
	ftTransferModule *transfer_module = NULL ;

	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
        std::map<RsFileHash, ftClient>::iterator it;
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

		transfer_module = (it->second).mModule ;
	}
	transfer_module->recvFileData(peerId, offset, chunksize, data);

	return true;
}


	/* called by ftTransferModule */
bool	ftDataMultiplex::handleRecvDataRequest(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize)
{
	/**** Find Files *****/

	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/
    std::map<RsFileHash, ftClient>::iterator cit;
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
	
    std::map<RsFileHash, ftFileProvider *>::iterator sit;
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

bool	ftDataMultiplex::locked_handleServerRequest(ftFileProvider *provider, const RsPeerId& peerId, const RsFileHash& hash, uint64_t size,
			uint64_t offset, uint32_t chunksize)
{
	if(chunksize > uint32_t(10*1024*1024))
	{
		std::cerr << "Warning: peer " << peerId << " is asking a large chunk (s=" << chunksize << ") for hash " << hash << ", filesize=" << size << ". This is unexpected." << std::endl ;
		return false ;
	}
	void *data = rs_malloc(chunksize);

	if(data == NULL)
		return false ;
	
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::locked_handleServerRequest()";
	std::cerr << "\t peer: " << peerId << " hash: " << hash;
	std::cerr << " size: " << size;
	std::cerr << std::endl;
	std::cerr << "\t offset: " << offset;
	std::cerr << " chunksize: " << chunksize << " data: " << data;
	std::cerr << std::endl;
#endif

	if (provider->getFileData(peerId,offset, chunksize, data))
	{
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

bool ftDataMultiplex::getClientChunkMap(const RsFileHash& upload_hash,const RsPeerId& peerId,CompressedChunkMap& cmap)
{
	bool too_old = false;
	{
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

        std::map<RsFileHash,ftFileProvider *>::iterator sit = mServers.find(upload_hash);

		if(mServers.end() == sit)
			return false ;

		sit->second->getClientMap(peerId,cmap,too_old) ;
	}

	// If the map is too old then we should ask an other map to the peer.
	//
	if(too_old)
		sendChunkMapRequest(peerId,upload_hash,true);

	return true ;
}

bool ftDataMultiplex::sendChunkMapRequest(const RsPeerId& peer_id,const RsFileHash& hash,bool is_client)
{
	return mDataSend->sendChunkMapRequest(peer_id,hash,is_client);
}
bool ftDataMultiplex::sendSingleChunkCRCRequests(const RsFileHash& hash, const std::vector<uint32_t>& to_ask)
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

	// Put all requested chunks in the request queue.
	
	Sha1CacheEntry& ce(_cached_sha1maps[hash]) ;

	for(uint32_t i=0;i<to_ask.size();++i)
	{
		std::pair<rstime_t,ChunkCheckSumSourceList>& list(ce._to_ask[to_ask[i]]) ;
		list.first = 0 ; // set last request time to 0
	}
	return true ;
}

void ftDataMultiplex::handlePendingCrcRequests()
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

	rstime_t now = time(NULL) ;
	uint32_t n=0 ;

	// Go through the list of currently handled hashes. For each of them,
	// look for pending chunk crc requests. 
	// 	- if the last request is too old, re-ask:
	// 		- ask the file creator about the possible sources for this chunk => returns a list of active sources
	//			- among active sources, pick the one that has the smallest request time stamp, in the request list.
	//
	// With this, only active sources are querried.
	//

    for(std::map<RsFileHash,Sha1CacheEntry>::iterator it(_cached_sha1maps.begin());it!=_cached_sha1maps.end();++it)
		for(std::map<uint32_t,std::pair<rstime_t,ChunkCheckSumSourceList> >::iterator it2(it->second._to_ask.begin());it2!=it->second._to_ask.end();++it2)
			if(it2->second.first + MAX_CHECKING_CHUNK_WAIT_DELAY < now)	// is the last request old enough?
			{
#ifdef MPLEX_DEBUG
				std::cerr << "ftDataMultiplex::handlePendingCrcRequests():  Requesting sources for chunk " << it2->first << ", hash " << it->first << std::endl;
#endif
				// 0 - ask which sources can be used for this chunk
				//
                std::map<RsFileHash,ftClient>::const_iterator it4(mClients.find(it->first)) ;

				if(it4 == mClients.end())
					continue ;

				std::vector<RsPeerId> sources ;
				it4->second.mCreator->getSourcesList(it2->first,sources) ;

				// 1 - go through all sources. Take the oldest one.
				//

				RsPeerId best_source ;
				rstime_t oldest_timestamp = now ;

				for(uint32_t i=0;i<sources.size();++i)
				{
#ifdef MPLEX_DEBUG
					std::cerr << "ftDataMultiplex::handlePendingCrcRequests():    Examining source " << sources[i] << std::endl;
#endif
					std::map<RsPeerId,rstime_t>::const_iterator it3(it2->second.second.find(sources[i])) ;

					if(it3 == it2->second.second.end()) // source not found. So this one is surely the oldest one to have been requested.
					{
#ifdef MPLEX_DEBUG
						std::cerr << "ftDataMultiplex::handlePendingCrcRequests():    not found! So using it directly." << std::endl;
#endif
						best_source = sources[i] ;
						break ;
					}
					else if(it3->second <= oldest_timestamp) // do nothing, otherwise, ask again
					{
#ifdef MPLEX_DEBUG
						std::cerr << "ftDataMultiplex::handlePendingCrcRequests():    not found! So using it directly." << std::endl;
#endif
						best_source = sources[i] ;
						oldest_timestamp = it3->second ;
					}
#ifdef MPLEX_DEBUG
					else
						std::cerr << "ftDataMultiplex::handlePendingCrcRequests():    Source too recently used! So using it directly." << std::endl;
#endif
				}
				if(!best_source.isNull())
				{
#ifdef MPLEX_DEBUG
					std::cerr << "ftDataMultiplex::handlePendingCrcRequests(): Asking crc of chunk " << it2->first << " to peer " << best_source << " for hash " << it->first << std::endl;
#endif
					// Use the source to ask the CRC.
					//
					// 	sendSingleChunkCRCRequest(peer_id, hash, chunk_id)
					//
					mDataSend->sendSingleChunkCRCRequest(best_source,it->first,it2->first);
					it2->second.second[best_source] = now ;
					it2->second.first = now ;

					if(++n > MAX_SIMULTANEOUS_CRC_REQUESTS)
						return ;
				}
#ifdef MPLEX_DEBUG
				else
					std::cerr << "ftDataMultiplex::handlePendingCrcRequests(): no source for chunk " << it2->first << std::endl;
#endif
			}
}

void ftDataMultiplex::deleteUnusedServers()
{
	RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

	//scan the uploads list in ftdatamultiplex and delete the items which time out
	rstime_t now = time(NULL);

    for(std::map<RsFileHash, ftFileProvider *>::iterator sit(mServers.begin());sit != mServers.end();)
		if(sit->second->purgeOldPeers(now,10))
		{
#ifdef MPLEX_DEBUG
			std::cerr << "ftDataMultiplex::deleteUnusedServers(): provider " << (void*)sit->second << " has no active peers. Removing. Now=" << now << std::endl ;
#endif
			// We don't delete servers that are clients at the same time !
			if(dynamic_cast<ftFileCreator*>(sit->second) == NULL)
			{
#ifdef MPLEX_DEBUG
				std::cerr << "ftDataMultiplex::deleteUnusedServers(): deleting file provider " << (void*)sit->second << std::endl ;
#endif
				delete sit->second;
			}
#ifdef MPLEX_DEBUG
			else
				std::cerr << "ftDataMultiplex::deleteUnusedServers(): " << (void*)sit->second << " was not deleted because it's also a file creator." << std::endl ;
#endif

            std::map<RsFileHash, ftFileProvider *>::iterator tmp(sit);
			++tmp ;

			mServers.erase(sit);

			sit = tmp ;
		}
		else
			++sit ;
}

bool	ftDataMultiplex::handleSearchRequest(const RsPeerId& peerId, const RsFileHash& hash)
{
#ifdef MPLEX_DEBUG
	std::cerr << "ftDataMultiplex::handleSearchRequest(";
	std::cerr << peerId << ", " << hash << "...)";
	std::cerr << std::endl;
#endif

	/* 
	 * Do Actual search 
	 * Could be Cache File, Local or Extra
	 * (anywhere but remote really)
	 *
	 * the network wide and browsable flags are needed, otherwise results get filtered.
	 * For tunnel creation, the check of browsable/network wide flag is already done, so
	 * if we get a file download packet here, the source is already allowed to download it.
	 * That is why we don't call the search function with a peer id.
	 *
	 */

	FileInfo info;
	FileSearchFlags hintflags =   RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_SPEC_ONLY ;

	if(rsTurtle->isTurtlePeer(peerId))
		hintflags |= RS_FILE_HINTS_NETWORK_WIDE ;

	if(mSearch->search(hash, hintflags, info))
	{

		/* setup a new provider */
		RsStackMutex stack(dataMtx); /******* LOCK MUTEX ******/

		// We might already have a file provider, if two requests have got stacked in the request queue. So let's
		// check that before.

#ifdef MPLEX_DEBUG
		std::cerr << "ftDataMultiplex::handleSearchRequest(";
		std::cerr << " Found Local File, sharing...";
#endif
        std::map<RsFileHash,ftFileProvider*>::const_iterator it = mServers.find(hash) ;
		ftFileProvider *provider ;

		if(it == mServers.end())
		{
			provider = new ftFileProvider(info.path, info.size, hash);
			mServers[hash] = provider;
#ifdef MPLEX_DEBUG
			std::cerr << " created new file provider " << (void*)provider << std::endl;
#endif
		}
		else
		{
#ifdef MPLEX_DEBUG
			std::cerr << " re-using existing file provider " << (void*)it->second << std::endl;
#endif
		}

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

        std::map<RsFileHash,ftClient>::const_iterator it(mClients.find(hash)) ;
		
		if(it != mClients.end())
		{
			mServers[hash] = it->second.mCreator ;
			return true;
		}
	}
	
	return false;
}





