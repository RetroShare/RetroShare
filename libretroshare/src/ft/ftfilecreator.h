/*******************************************************************************
 * libretroshare/src/ft: ftfilecreator.cc                                      *
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

class ZeroInitCounter
{
	public:
		ZeroInitCounter(): cnt(0) {}
		uint32_t cnt ;
};

class ftFileCreator: public ftFileProvider
{
	public:

		ftFileCreator(const std::string& savepath, uint64_t size, const RsFileHash& hash,bool assume_availability);

		~ftFileCreator();

		/* overloaded from FileProvider */
        virtual bool 	getFileData(const RsPeerId& peer_id,uint64_t offset, uint32_t &chunk_size, void *data, bool allow_unverified = false);
		bool	finished() ;
		uint64_t getRecvd();

		/// (temporarily) close the file, to save file descriptors.
		void closeFile() ;

		void getChunkMap(FileChunksInfo& info) ;

		void setChunkStrategy(FileChunksInfo::ChunkStrategy s) ;
		FileChunksInfo::ChunkStrategy getChunkStrategy() ;

		// Computes a sha1sum of the partial file, to check that the data is overall consistent.
		// This function is not mutexed. This is a bit dangerous, but otherwise we might stuck the GUI for a 
		// long time. Therefore, we must pay attention not to call this function
		// at a time file_name nor hash can be modified, which is quite easy.

		bool hashReceivedData(RsFileHash& hash) ;

		// Sets all chunks to checking state
		//
		void forceCheck() ; 

		bool verifyChunk(uint32_t, const Sha1CheckSum&) ;

		// Looks into the chunkmap for downloaded chunks that have not yet been certified.
		// For each of them, returns the chunk number and a source peer to ask the CRC to.
		//
		void getChunksToCheck(std::vector<uint32_t>& chunks_to_ask) ;

		/* 
		 * creation functions for FileCreator 
		 */

		// Gets a new variable-sized chunk of size "size_hint" from the given peer id. The returned size, "size" is
		// at most equal to size_hint. chunk_map_needed is set if 
		// - no chunkmap info is available. In such a case, the chunk info is irrelevant and false is returned.
		// - the chunk info is too old. In tis case, true is returned, and the chunks info can be used.
		//
		bool	getMissingChunk(const RsPeerId& peer_id,uint32_t size_hint,uint64_t& offset, uint32_t& size,bool& is_chunk_map_too_old);

		// Takes care of purging any inactive chunks. This should be called regularly, because some peers may disconnect
		// and let inactive chunks not finished.
		//
		void removeInactiveChunks() ;

		// removes the designated file source from the chunkmap.
		void removeFileSource(const RsPeerId& peer_id) ;

		// Get all available sources for this chunk
		//
		void getSourcesList(uint32_t chunk_number,std::vector<RsPeerId>& sources) ;

		// Returns resets the time stamp of the last data receive.
		rstime_t lastRecvTimeStamp() ;
		rstime_t creationTimeStamp() ;

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
		void setSourceMap(const RsPeerId& peer_id,const CompressedChunkMap& map) ;

		// Returns true id the given file source is complete.
		//
		bool sourceIsComplete(const RsPeerId& peer_id) ;

	protected:

		virtual int locked_initializeFileAttrs(); 

	private:

		bool 	locked_printChunkMap();
		int 	locked_notifyReceived(uint64_t offset, uint32_t chunk_size);
		/* 
		 * structure to track missing chunks 
		 */

		std::map<uint64_t, ftChunk> mChunks;
		std::map<RsPeerId,ZeroInitCounter> mChunksPerPeer ;

		ChunkMap chunkMap ;

		rstime_t _last_recv_time_t ;	/// last time stamp when data was received. Used for queue control.
		rstime_t _creation_time ;		/// time at which the file creator was created. Used to spot long-inactive transfers.
};

#endif // FT_FILE_CREATOR_HEADER
