/*
 * libretroshare/src/ft ftFileProvider.h
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

		virtual bool 	getFileData(const RsPeerId& peer_id,uint64_t offset, uint32_t &chunk_size, void *data);
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
		bool purgeOldPeers(time_t now,uint32_t max_duration) ;

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

				void updateStatus(uint64_t offset,uint32_t data_size,time_t now) ;

				uint64_t   req_loc;
				uint32_t   req_size;
				time_t    lastTS_t; 	// used for estimating transfer rate.
				time_t    lastTS;   	// last update time (for purging)

				// these two are used for speed estimation
				float 	  transfer_rate ;
				uint32_t		total_size ;

				// Info about what the downloading peer already has
				CompressedChunkMap client_chunk_map ;
				time_t client_chunk_map_stamp ;
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
