/*
 * libretroshare/src/ft/ ftfilecreator.h
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

#ifndef FT_FILE_CREATOR_HEADER
#define FT_FILE_CREATOR_HEADER

/* 
 * ftFileCreator
 *
 * TODO: Serialiser Load / Save.
 *
 */
#include "ftfileprovider.h"
#include "ftchunkmap.h"
#include <map>

class ftFileCreator: public ftFileProvider
{
	public:

		ftFileCreator(std::string savepath, uint64_t size, std::string hash, uint64_t recvd);

		~ftFileCreator();

		/* overloaded from FileProvider */
		virtual bool 	getFileData(uint64_t offset, uint32_t &chunk_size, void *data);
		bool	finished() { return getRecvd() == getFileSize(); }
		uint64_t getRecvd();

		void getChunkMap(FileChunksInfo& info) ;

		void setChunkStrategy(FileChunksInfo::ChunkStrategy s) ;
		FileChunksInfo::ChunkStrategy getChunkStrategy() ;

		/* 
		 * creation functions for FileCreator 
		 */

		// Gets a new variable-sized chunk of size "size_hint" from the given peer id. The returned size, "size" is
		// at most equal to size_hint. chunk_map_needed is set if 
		// - no chunkmap info is available. In such a case, the chunk info is irrelevant and false is returned.
		// - the chunk info is too old. In tis case, true is returned, and the chunks info can be used.
		//
		bool	getMissingChunk(const std::string& peer_id,uint32_t size_hint,uint64_t& offset, uint32_t& size,bool& is_chunk_map_too_old);

		// Takes care of purging any inactive chunks. This should be called regularly, because some peers may disconnect
		// and let inactive chunks not finished.
		//
		void removeInactiveChunks() ;

		// actually store data in the file, and update chunks info
		//
		bool 	addFileData(uint64_t offset, uint32_t chunk_size, void *data);

		// Load/save the availability map for the file being downloaded, in a compact/compressed form.
		// This is used for
		// 	- loading and saving info about the current transfers
		// 	- getting info about current chunks for the GUI
		// 	- sending availability info to the peers for which we also are a source
		//
		virtual void getAvailabilityMap(CompressedChunkMap& cmap) ;
		void setAvailabilityMap(const CompressedChunkMap& cmap) ;

		// This is called when receiving the availability map from a source peer, for the file being handled.
		//
		void setSourceMap(const std::string& peer_id,const CompressedChunkMap& map) ;

	protected:

		virtual int initializeFileAttrs(); 

	private:

		bool 	locked_printChunkMap();
		int 	locked_notifyReceived(uint64_t offset, uint32_t chunk_size);
		/* 
		 * structure to track missing chunks 
		 */

		uint64_t mStart;
		uint64_t mEnd;

		std::map<uint64_t, ftChunk> mChunks;

		ChunkMap chunkMap ;
};

#endif // FT_FILE_CREATOR_HEADER
