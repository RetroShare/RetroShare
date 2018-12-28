/*******************************************************************************
 * libretroshare/src/ft: ftchunkmap.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2010 by Cyril Soler <csoler@users.sourceforge.net>                *
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
 ******************************************************************************/

#pragma once

#include <map>
#include "retroshare/rstypes.h"

// ftChunkMap: 
// 	- handles chunk map over a complete file
// 	- mark down which chunk is being downloaded by which peer
// 	- allocate data ranges of any requested size for a given peer
// 		- continuing an existing chunk
// 		- allocating a new chunk
//
// Download mecanism:
// 	- ftFileCreator handles a list of active slices, and periodically resends requests every 20 sec.
// 		Slices have arbitrary size (less than a chunk), depending on the transfer rate.
// 		When receiving data, ftFileCreator shrinks its slices until they get complete. When a slice is finished, it
// 		notifies ftChunkMap that this slice is done.
//
// 	- ftChunkMap maintains two levels:
// 		- the chunk level (Chunks a 1MB long) with a map of who has which chunk and what locally is the state of 
// 		each chunk
// 		- the slice level: each active chunk is cut into slices (basically a list of intervalls) being downloaded, and
// 		a remaining slice to cut off new candidates. When notified for a complete slice, ftChunkMap removed the
// 		corresponding acive slice. When asked a slice, ftChunkMap cuts out a slice from the remaining part of the chunk
// 		to download, sends the slice's coordinates and gives a unique slice id (such as the slice offset).


// This class handles a slice of a chunk of arbitrary uint32_t size, at the level of ftFileCreator

class ftController ;

class ftChunk 
{
	public:
		typedef uint64_t OffsetInFile ;

		ftChunk():offset(0), size(0), id(0), ts(0),ref_cnt(NULL) {}

		friend std::ostream& operator<<(std::ostream& o,const ftChunk& f) ;

		uint64_t offset;	// current offset of the slice
		uint64_t size;		// size remaining to download
		OffsetInFile  id ;		// id of the chunk. Equal to the starting offset of the chunk
		rstime_t   ts;		// time of last data received
		int	  *ref_cnt; // shared counter of number of sub-blocks. Used when a slice gets split.
		RsPeerId peer_id ;
};

// This class handles a single fixed-sized chunk. Although each chunk is requested at once,
// it may be sent back into sub-chunks because of file transfer rate constraints. 
// So the dataReceived function should be called to progressively complete the chunk,
// and the getChunk method should ask for a sub-chunk of a given size.
//
class Chunk
{
	public: 
		Chunk(): _start(0),_offset(0),_end(0) {}	// only used in default std::map fillers

		Chunk(uint64_t start,uint32_t size) ;

		void getSlice(uint32_t size_hint,ftChunk& chunk) ;

		// Returns true when the chunk is complete
		bool empty() const { return _offset == _end ; }

		// Array of intervalls of bytes to download.
		//
		uint64_t _start ;		// const
		uint64_t _offset ;	// not const: handles the current offset within the chunk.
		uint64_t _end ;		// const
};

struct ChunkDownloadInfo
{
	public:
		struct SliceRequestInfo
		{
			uint32_t           size ;              // size of the slice
			rstime_t             request_time ;      // last request time
			std::set<RsPeerId> peers ;             // peers the slice was requested to. Normally only one, except at the end of the file.
		};

		std::map<ftChunk::OffsetInFile,SliceRequestInfo> _slices ;
		uint32_t _remains ;
		rstime_t _last_data_received ;
};

class SourceChunksInfo
{
	public:
		CompressedChunkMap cmap ;	//! map of what the peer has/doens't have
		rstime_t TS ;						//! last update time for this info
		bool is_full ;					//! is the map full ? In such a case, re-asking for it is unnecessary.

		// Returns true if the offset is starting in a mapped chunk.
		//
		bool hasData(uint64_t offset,uint32_t fixed_chunk_size) const
		{
			if(is_full)
				return true ;

			return cmap[offset / (uint64_t)fixed_chunk_size ] ;
		}
};

class ChunkMap
{
   public:
		static const uint32_t CHUNKMAP_FIXED_CHUNK_SIZE = 1024*1024 ; // 1 MB chunk
		typedef uint32_t ChunkNumber ;

		/// Constructor. Decides what will be the size of chunks and how many there will be.

		ChunkMap(uint64_t file_size,bool assume_availability) ;

		/// destructor
		virtual ~ChunkMap() {}

		/// Returns an slice of data to be asked to the peer within a chunk.
		/// If a chunk is already been downloaded by this peer, take a slice at
		/// the beginning of this chunk, or at least where it starts.
		/// If not, randomly/streamly select a new chunk depending on the strategy.
		/// adds an entry in the chunk_ids map, and sets up 1 interval for it.
		/// the chunk should be available from the designated peer.
		/// On return:
		/// 	- source_chunk_map_needed 	= true if the source map should be asked

      virtual bool getDataChunk(const RsPeerId& peer_id,uint32_t size_hint,ftChunk& chunk,bool& source_chunk_map_needed) ; 

		/// Returns an already pending slice that was being downloaded but hasn't arrived yet. This is mostly used at the end of the file
		/// in order to re-ask pendign slices to active peers while slow peers take a lot of time to send their remaining slices.
		///
		bool reAskPendingChunk(const RsPeerId& peer_id,uint32_t size_hint,uint64_t& offset,uint32_t& size);

		/// Notify received a slice of data. This needs to
		///   - carve in the map of chunks what is received, what is not.
		///   - tell which chunks are finished. For this, each interval must know what chunk number it has been attributed
		///    when the interval is split in the middle, the number of intervals for the chunk is increased. If the interval is
		///    completely covered by the data, the interval number is decreased.

		virtual void dataReceived(const ftChunk::OffsetInFile& c_id) ;

      /// Decides how chunks are selected. 
      ///    STREAMING: the 1st chunk is always returned
      ///       RANDOM: a uniformly random chunk is selected among available chunks for the current source.
      ///              

		void setStrategy(FileChunksInfo::ChunkStrategy s) { _strategy = s ; }
		FileChunksInfo::ChunkStrategy getStrategy() const { return _strategy ; }

      /// Properly fills an vector of fixed size chunks with availability or download state.
      /// chunks is given with the proper number of chunks and we have to adapt to it. This can be used
      /// to display square chunks in the gui or display a blue bar of availability by collapsing info from all peers.
		/// The set method is not virtual because it has no reason to exist in the parent ftFileProvider

      virtual void getAvailabilityMap(CompressedChunkMap& cmap) const ;
		void setAvailabilityMap(const CompressedChunkMap& cmap) ;

		/// Removes the source availability map. The map
		void removeFileSource(const RsPeerId& peer_id) ;

		/// This function fills in a plain map for a file of the given size. This
		/// is used to ensure that the chunk size will be consistent with the rest
		/// of the code.
		//
		static void buildPlainMap(uint64_t size,CompressedChunkMap& map) ;

		/// Computes the number of chunks for the given file size.
		static uint32_t getNumberOfChunks(uint64_t size) ;
		
		/// This function is used by the parent ftFileProvider to know whether the chunk can be sent or not.
		bool isChunkAvailable(uint64_t offset, uint32_t chunk_size) const ;

        bool isChunkOutstanding(uint64_t offset, uint32_t chunk_size) const ;

		/// Remove active chunks that have not received any data for the last 60 seconds, and return
		/// the list of slice numbers that should be canceled.
		void removeInactiveChunks(std::vector<ftChunk::OffsetInFile>& to_remove) ;

		/// Updates the peer's availablility map
		//
		void setPeerAvailabilityMap(const RsPeerId& peer_id,const CompressedChunkMap& peer_map) ;

		/// Returns a pointer to the availability chunk map of the given source, and possibly
		/// allocates it if necessary.
		//
		SourceChunksInfo *getSourceChunksInfo(const RsPeerId& peer_id) ;

		/// Returns the total size of downloaded data in the file.
		uint64_t getTotalReceived() const { return _total_downloaded ; }

		/// returns true is the file is complete
		bool isComplete() const { return _file_is_complete ; }

		/// returns info about currently downloaded chunks
		void getChunksInfo(FileChunksInfo& info) const ;

		/// input the result of the chunk hash checking 
		void setChunkCheckingResult(uint32_t chunk_number, bool succeed) ;

		/// returns the current list of chunks to ask for a CRC, and a proposed source for each
		void getChunksToCheck(std::vector<uint32_t>& chunks_to_ask) ;

		/// Get all available sources for this chunk
		void getSourcesList(uint32_t chunk_number,std::vector<RsPeerId>& sources) ;

		/// sets all chunks to checking state
		void forceCheck() ;

		/// Goes through all structures and computes the actual file completion. The true completion
		/// gets lost when force checking the file.
		void updateTotalDownloaded() ;

	protected:
		/// handles what size the last chunk has.
		uint32_t sizeOfChunk(uint32_t chunk_number) const ;

		/// Returns a chunk available for this peer_id, depending on the chunk strategy.
		//
		uint32_t getAvailableChunk(const RsPeerId& peer_id,bool& chunk_map_too_old) ;

	private:
        bool hasChunkState(uint64_t offset, uint32_t chunk_size, FileChunksInfo::ChunkState state) const;

		uint64_t												_file_size ;						//! total size of the file in bytes.
		uint32_t												_chunk_size ;						//! Size of chunks. Common to all chunks.
		FileChunksInfo::ChunkStrategy 				_strategy ;							//! how do we allocate new chunks
		std::map<RsPeerId,Chunk>					   _active_chunks_feed ; 			//! vector of chunks being downloaded. Exactly 1 chunk per peer.
		std::map<ChunkNumber,ChunkDownloadInfo>	_slices_to_download ; 			//! list of (slice offset,slice size) currently being downloaded
		std::vector<FileChunksInfo::ChunkState>	_map ;								//! vector of chunk state over the whole file
		std::map<RsPeerId,SourceChunksInfo>		_peers_chunks_availability ;	//! what does each source peer have
		uint64_t												_total_downloaded ;				//! completion for the file
		bool													_file_is_complete ;           //! set to true when the file is complete.
		bool													_assume_availability ;			//! true if all sources always have the complete file.
		std::vector<uint32_t>							_chunks_checking_queue ;		//! Queue of downloaded chunks to be checked.
};


