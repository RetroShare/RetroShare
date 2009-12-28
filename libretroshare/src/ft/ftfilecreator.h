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

		/* 
		 * creation functions for FileCreator 
		 */

		// Gets a new variable-sized chunk of size "size_hint" from the given peer id. The returned size, "size" is
		// at most equal to size_hint.
		//
		bool	getMissingChunk(const std::string& peer_id,uint32_t size_hint,uint64_t& offset, uint32_t& size);

		// actually store data in the file, and update chunks info
		//
		bool 	addFileData(uint64_t offset, uint32_t chunk_size, void *data);

		// Load/save the availability map for the file being downloaded, in a compact/compressed form.
		// This is used for
		// 	- loading and saving info about the current transfers
		// 	- getting info about current chunks for the GUI
		// 	- sending availability info to the peers for which we also are a source
		//
		void loadAvailabilityMap(const std::vector<uint32_t>& map,uint32_t chunk_size,uint32_t chunk_number,uint32_t chunk_strategy) ;
		void storeAvailabilityMap(std::vector<uint32_t>& map,uint32_t& chunk_size,uint32_t& chunk_number,uint32_t& chunk_strategy) ;

		// This is called when receiving the availability map from a source peer, for the file being handled.
		//
		void setSourceMap(const std::string& peer_id,uint32_t chunk_size,uint32_t nb_chunks,const std::vector<uint32_t>& map) ;

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
