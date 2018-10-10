/*******************************************************************************
 * libretroshare/src/ft: ftfileprovider.h                                      *
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

#ifndef FT_FILE_PROVIDER_HEADER
#define FT_FILE_PROVIDER_HEADER

/* 
 * ftFileProvider.
 *
 */
#include <iostream>
#include <stdint.h>
#include "util/rsthreads.h"
#include "retroshare/rsfiles.h"

class ftFileProvider
{
	public:
		ftFileProvider(const std::string& path, uint64_t size, const RsFileHash& hash);
		virtual ~ftFileProvider();

        /**
         * read a block of data from the file
         * @param peer_id for the uploading stats: to which peer the data will be send
         * @param offset begin of the requested data range
         * @param chunk_size how many bytes to read. Will be set to the number of valid bytes in data on return.
         * @param data pointer to a buffer of size chunk_size. Contains the data on success.
         * @param allow_unverified for ftFileCreator: set to true to return data from unverified chunks. Use it if you want data asap.
         * @return true if data was read
         */
        virtual bool 	getFileData(const RsPeerId& peer_id,uint64_t offset, uint32_t &chunk_size, void *data, bool allow_unverified = false);
		virtual bool    FileDetails(FileInfo &info);
		RsFileHash getHash();
		uint64_t getFileSize();
		bool fileOk();

		// Provides a client for the map of chunks actually present in the file. If the provider is also
		// a file creator, because the file is actually being downloaded, then the map may be partially complete.
		// Otherwize, a plain map is returned.
		//
		virtual void getAvailabilityMap(CompressedChunkMap& cmap) ;

		// a ftFileProvider feeds a distant peer. To display what the peers already has, we need to store/read this info.
		void getClientMap(const RsPeerId& peer_id,CompressedChunkMap& cmap,bool& map_is_too_old) ;
		void setClientMap(const RsPeerId& peer_id,const CompressedChunkMap& cmap) ;

		// Removes inactive peers from the client list. Returns true if all peers have been removed.
		//
		bool purgeOldPeers(rstime_t now,uint32_t max_duration) ;

		const RsFileHash& fileHash() const { return hash ; }
		const std::string& fileName() const { return file_name ; }
		uint64_t fileSize() const { return mSize ; }
	protected:
		virtual	int initializeFileAttrs(); /* does for both */

		uint64_t    mSize;
		RsFileHash hash;
		std::string file_name;
		FILE *fd;

		/* 
		 * Structure to gather statistics FIXME: lastRequestor - figure out a 
		 * way to get last requestor (peerID)
		 */
		class PeerUploadInfo
		{
			public:
				PeerUploadInfo() 
					: req_loc(0),req_size(1),  lastTS_t(0), lastTS(0),transfer_rate(0), total_size(0), client_chunk_map_stamp(0) {}

				void updateStatus(uint64_t offset,uint32_t data_size,rstime_t now) ;

				uint64_t   req_loc;
				uint32_t   req_size;
				rstime_t    lastTS_t; 	// used for estimating transfer rate.
				rstime_t    lastTS;   	// last update time (for purging)

				// these two are used for speed estimation
				float 	  transfer_rate ;
				uint32_t		total_size ;

				// Info about what the downloading peer already has
				CompressedChunkMap client_chunk_map ;
				rstime_t client_chunk_map_stamp ;
		};

		// Contains statistics (speed, peer name, etc.) of all uploading peers for that file.
		//
		std::map<RsPeerId,PeerUploadInfo> uploading_peers ;

		/* 
		 * Mutex Required for stuff below 
		 */
		RsMutex ftcMutex;
};


#endif // FT_FILE_PROVIDER_HEADER
