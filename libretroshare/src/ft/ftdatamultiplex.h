/*******************************************************************************
 * libretroshare/src/ft: ftdatamultiplex.h                                     *
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
#include "retroshare/rsfiles.h"


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

	ftRequest(uint32_t type, const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunk, void *data);

	ftRequest()
	:mType(0), mSize(0), mOffset(0), mChunk(0), mData(NULL) { return; }

	uint32_t mType;
	RsPeerId mPeerId;
	RsFileHash mHash;
	uint64_t mSize;
	uint64_t mOffset;
	uint32_t mChunk;
	void *mData;
};

typedef std::map<RsPeerId,rstime_t> ChunkCheckSumSourceList ;

class Sha1CacheEntry
{
	public:
		Sha1Map _map ; 												// Map of available sha1 sums for every chunk.
		rstime_t last_activity ;										// This is used for removing unused entries.
		std::vector<uint32_t> _received ;						// received chunk ids. To bedispatched.
		std::map<uint32_t,std::pair<rstime_t,ChunkCheckSumSourceList> > _to_ask ;		// Chunks to ask to sources.
};
	
class ftDataMultiplex: public ftDataRecv, public RsQueueThread
{

	public:

		ftDataMultiplex(const RsPeerId& ownId, ftDataSend *server, ftSearch *search);

        /**
         * @see RsFiles::getFileData
     *
     * data should be pre-allocated by the client with size sufficient gfor requested_size bytes.
     * requested_size will be changed so as to contain the actual number of bytes copied from the file,
     * in case where the full size wasn't available.
     * False is returned if no data can be obtained from that file.
         */
        bool getFileData(const RsFileHash& hash, uint64_t offset,uint32_t& requested_size, uint8_t *data);

		/* ftController Interface */
		bool	addTransferModule(ftTransferModule *mod, ftFileCreator *f);
		bool	removeTransferModule(const RsFileHash& hash);

		/* data interface */
		/* get Details of File Transfers */
		bool    FileUploads(std::list<RsFileHash> &hashs);
		bool    FileDownloads(std::list<RsFileHash> &hashs);
		bool    FileDetails(const RsFileHash &hash, FileSearchFlags hintsflag, FileInfo &info);

        void deleteUnusedServers() ;
        bool deleteServer(const RsFileHash& hash);	// deletes FtServers for the given hash. Used when removing an extra file from shares.
        void handlePendingCrcRequests() ;

		/*************** SEND INTERFACE (calls ftDataSend) *******************/

		/* Client Send */
		bool	sendDataRequest(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize);

		/* Server Send */
		bool	sendData(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data);

		/* Server/client Send */
		bool	sendChunkMapRequest(const RsPeerId& peerId, const RsFileHash& hash,bool is_client) ;


		/* called from a separate thread */
		bool sendSingleChunkCRCRequests(const RsFileHash& hash, const std::vector<uint32_t>& to_ask) ;

		bool dispatchReceivedChunkCheckSum() ;

		/*************** RECV INTERFACE (provides ftDataRecv) ****************/

		/* Client Recv */
		virtual bool recvData(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data);
		/* Server Recv */
		virtual bool	recvDataRequest(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize);

		/// Receive a request for a chunk map
		virtual bool recvChunkMapRequest(const RsPeerId& peer_id,const RsFileHash& hash,bool is_client) ;
		/// Receive a chunk map
		virtual bool recvChunkMap(const RsPeerId& peer_id,const RsFileHash& hash,const CompressedChunkMap& cmap,bool is_client) ;

		virtual bool recvSingleChunkCRCRequest(const RsPeerId& peer_id,const RsFileHash& hash,uint32_t chunk_id) ;
		virtual bool recvSingleChunkCRC(const RsPeerId& peer_id,const RsFileHash& hash,uint32_t chunk_id,const Sha1CheckSum& sum) ;
		
		// Returns the chunk map from the file uploading client. Also initiates a chunk map request if this 
		// map is too old. This supposes that the caller will ask again in a few seconds.
		//
		bool getClientChunkMap(const RsFileHash& upload_hash,const RsPeerId& peer_id,CompressedChunkMap& map) ;

	protected:

		/* Overloaded from RsQueueThread */
		virtual bool workQueued();
		virtual bool doWork();

	private:

		/* Handling Job Queues */
		bool handleRecvData(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize, void *data);
		bool handleRecvDataRequest(const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize);
		bool handleSearchRequest(const RsPeerId& peerId, const RsFileHash& hash);
		bool handleRecvClientChunkMapRequest(const RsPeerId& peerId, const RsFileHash& hash) ;
		bool handleRecvServerChunkMapRequest(const RsPeerId& peerId, const RsFileHash& hash) ;
		bool handleRecvChunkCrcRequest(const RsPeerId& peerId, const RsFileHash& hash,uint32_t chunk_id) ;

		/* We end up doing the actual server job here */
		bool    locked_handleServerRequest(ftFileProvider *provider, const RsPeerId& peerId, const RsFileHash& hash, uint64_t size, uint64_t offset, uint32_t chunksize);

		RsMutex dataMtx;

		std::map<RsFileHash, ftClient> mClients;
		std::map<RsFileHash, ftFileProvider *> mServers;

		std::list<ftRequest> mRequestQueue;
		std::list<ftRequest> mSearchQueue;

		std::map<RsFileHash,Sha1CacheEntry> _cached_sha1maps ;						// one cache entry per file hash. Handled dynamically.

		ftDataSend *mDataSend;
		ftSearch   *mSearch;
		RsPeerId mOwnId;

		friend class ftServer;
};

#endif
