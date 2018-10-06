/*******************************************************************************
 * libretroshare/src/ft: ftchunkmap.cc                                         *
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

/********
 * #define DEBUG_FTCHUNK 1
 *********/

#ifdef DEBUG_FTCHUNK
#include <assert.h>
#endif
#include <math.h>
#include <stdlib.h>
#include "retroshare/rspeers.h"
#include "ftchunkmap.h"
#include "util/rstime.h"

static const uint32_t SOURCE_CHUNK_MAP_UPDATE_PERIOD	=   60 ; //! TTL for chunkmap info
static const uint32_t INACTIVE_CHUNK_TIME_LAPSE 		= 3600 ; //! TTL for an inactive chunk
static const uint32_t FT_CHUNKMAP_MAX_CHUNK_JUMP		=   50 ; //! Maximum chunk jump in progressive DL mode
static const uint32_t FT_CHUNKMAP_MAX_SLICE_REASK_DELAY =   10 ; //! Maximum time to re-ask a slice to another peer at end of transfer

std::ostream& operator<<(std::ostream& o,const ftChunk& c)
{
	return o << "\tChunk [" << c.offset << "] size: " << c.size << "  ChunkId: " << c.id << "  Age: " << time(NULL) - c.ts << ", owner: " << c.peer_id ;
}

// Chunk: very bold implementation for now. We should compress the bits to have
// 32 of them per uint32_t value, of course!
//
Chunk::Chunk(uint64_t start,uint32_t size)
	: _start(start),_offset(start),_end( (uint64_t)size + start )
{
}

void Chunk::getSlice(uint32_t size_hint,ftChunk& chunk)
{
	// Take the current offset
	chunk.offset = _offset ;
	chunk.size = std::min(size_hint,(uint32_t)(_end-_offset)) ;
	chunk.id = _offset ;
	chunk.ts = time(NULL) ;
	
	// push the slice marker into currently handled slices.
	_offset += chunk.size ;
}

ChunkMap::ChunkMap(uint64_t s,bool availability)
	:	_file_size(s),
		_chunk_size(CHUNKMAP_FIXED_CHUNK_SIZE), // 1MB chunks
		_assume_availability(availability)
{
	uint64_t n = s/(uint64_t)_chunk_size ;
	if(s% (uint64_t)_chunk_size != 0)
		++n ;

	_map.resize(n,FileChunksInfo::CHUNK_OUTSTANDING) ;
	_strategy = FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE ;
	_total_downloaded = 0 ;
	_file_is_complete = false ;
#ifdef DEBUG_FTCHUNK
	std::cerr << "*** ChunkMap::ChunkMap: starting new chunkmap:" << std::endl ; 
	std::cerr << "   File size: " << s << std::endl ;
	std::cerr << "   Strategy: " << _strategy << std::endl ;
	std::cerr << "   ChunkSize: " << _chunk_size << std::endl ;
	std::cerr << "   Number of Chunks: " << n << std::endl ;
	std::cerr << "   Data: " ;
	for(uint32_t i=0;i<_map.size();++i)
		std::cerr << _map[i] ;
	std::cerr << std::endl ;
#endif
}

void ChunkMap::setAvailabilityMap(const CompressedChunkMap& map)
{
	// do some sanity check
	//
	
	if( (((int)_map.size()-1)>>5) >= (int)map._map.size() )
	{
		std::cerr << "ChunkMap::setPeerAvailabilityMap: Compressed chunkmap received is too small or corrupted." << std::endl;
		return ;
	}

	// copy the map
	//
	_file_is_complete = true ;
	_total_downloaded = 0 ;

	for(uint32_t i=0;i<_map.size();++i)
		if(map[i] > 0)
		{
			_map[i] = FileChunksInfo::CHUNK_DONE ;
			_total_downloaded += sizeOfChunk(i) ;
		}
		else
		{
			_map[i] = FileChunksInfo::CHUNK_OUTSTANDING ;
			_file_is_complete = false ;
		}
}

void ChunkMap::dataReceived(const ftChunk::OffsetInFile& cid)
{
	// 1 - find which chunk contains the received data.
	//

	// trick: cid is the chunk offset. So we use it to get the chunk number.
	int n = (uint64_t)cid/_chunk_size ;

	std::map<ChunkNumber,ChunkDownloadInfo>::iterator itc(_slices_to_download.find(n)) ;

	if(itc == _slices_to_download.end()) 
	{
		std::cerr << "!!! ChunkMap::dataReceived: error: ChunkId " << cid << " corresponds to chunk number " << n << ", which is not being downloaded!" << std::endl ;
#ifdef DEBUG_FTCHUNK
		assert(false) ;
#endif
		return ;
	}

	std::map<ftChunk::OffsetInFile,ChunkDownloadInfo::SliceRequestInfo>::iterator it(itc->second._slices.find(cid)) ;

	if(it == itc->second._slices.end()) 
	{
		std::cerr << "!!! ChunkMap::dataReceived: chunk " << cid << " is not found in slice lst of chunk number " << n << std::endl ;
#ifdef DEBUG_FTCHUNK
		assert(false) ;
#endif
		return ;
	}

	_total_downloaded += it->second.size ;
	itc->second._remains -= it->second.size ;
	itc->second._slices.erase(it) ;
	itc->second._last_data_received = time(NULL) ;	// update time stamp

#ifdef DEBUG_FTCHUNK
	std::cerr << "*** ChunkMap::dataReceived: received data chunk " << cid << " for chunk number " << n << ", local remains=" << itc->second._remains << ", total downloaded=" << _total_downloaded << ", remains=" << _file_size - _total_downloaded << std::endl ;
#endif
	if(itc->second._remains == 0) // the chunk was completely downloaded
	{
#ifdef DEBUG_FTCHUNK
		std::cerr << "*** ChunkMap::dataReceived: Chunk is complete. Removing it." << std::endl ;
#endif

		_map[n] = FileChunksInfo::CHUNK_CHECKING ;

		if(n > 0 || _file_size > CHUNKMAP_FIXED_CHUNK_SIZE)	// dont' put <1MB files into checking mode. This is useless.
			_chunks_checking_queue.push_back(n) ;
		else
			_map[n] = FileChunksInfo::CHUNK_DONE ;

		_slices_to_download.erase(itc) ;

		updateTotalDownloaded() ;
	}
}

void ChunkMap::updateTotalDownloaded()
{
	_total_downloaded = 0 ;
	_file_is_complete = true ;

	// First, round over chunk map to get the raw info.
	//
	for(uint32_t i=0;i<_map.size();++i)
		switch(_map[i])
		{
			case FileChunksInfo::CHUNK_CHECKING: _file_is_complete = false ;
				/* fallthrough */
			case FileChunksInfo::CHUNK_DONE:		 _total_downloaded += sizeOfChunk(i) ;
															 break ;
			default:
															 _file_is_complete = false ;
		}

	// Then go through active chunks.
	//
	for(std::map<ChunkNumber,ChunkDownloadInfo>::const_iterator itc(_slices_to_download.begin());itc!=_slices_to_download.end();++itc)
	{
		if(_map[itc->first] == FileChunksInfo::CHUNK_CHECKING)
			_total_downloaded -= sizeOfChunk(itc->first) ;

		_total_downloaded += sizeOfChunk(itc->first) - itc->second._remains ;

		if(_file_is_complete)
			std::cerr << "ChunkMap::updateTotalDownloaded(): ERROR: file still has pending slices but all chunks are marked as DONE !!" << std::endl;
	}
}

void ChunkMap::getChunksToCheck(std::vector<uint32_t>& chunks_crc_to_ask)
{
	chunks_crc_to_ask.clear() ;

	for(uint32_t i=0;i<_chunks_checking_queue.size();)
	{
		chunks_crc_to_ask.push_back(_chunks_checking_queue[i]) ;

		// remove that chunk from the queue

		_chunks_checking_queue[i] = _chunks_checking_queue.back() ;
		_chunks_checking_queue.pop_back() ;
	}
}

void ChunkMap::setChunkCheckingResult(uint32_t chunk_number,bool check_succeeded)
{
	// Find the chunk is the waiting queue. Remove it, and mark it as done.
	//
	if(_map[chunk_number] != FileChunksInfo::CHUNK_CHECKING)
	{
		std::cerr << "(EE) ChunkMap: asked to set checking result of chunk " << chunk_number<< " that is not marked as being checked!!" << std::endl;
		return ;
	}
	
	if(check_succeeded)
	{
		_map[chunk_number] = FileChunksInfo::CHUNK_DONE ;

		// We also check whether the file is complete or not.

		_file_is_complete = true ;

		for(uint32_t i=0;i<_map.size();++i)
			if(_map[i] != FileChunksInfo::CHUNK_DONE)
			{
				_file_is_complete = false ;
				break ;
			}
	}
	else
	{
		_total_downloaded -= sizeOfChunk(chunk_number) ;	// restore completion.
		_map[chunk_number] = FileChunksInfo::CHUNK_OUTSTANDING ;
	}
}

bool ChunkMap::reAskPendingChunk( const RsPeerId& peer_id,
                                  uint32_t /*size_hint*/,
                                  uint64_t& offset, uint32_t& size)
{
	// make sure that we're at the end of the file. No need to be too greedy in the middle of it.

	for(uint32_t i=0;i<_map.size();++i)
		if(_map[i] == FileChunksInfo::CHUNK_OUTSTANDING)
			return false ;

	rstime_t now = time(NULL);

	for(std::map<uint32_t,ChunkDownloadInfo>::iterator it(_slices_to_download.begin());it!=_slices_to_download.end();++it)
		for(std::map<ftChunk::OffsetInFile,ChunkDownloadInfo::SliceRequestInfo >::iterator it2(it->second._slices.begin());it2!=it->second._slices.end();++it2)
			if(it2->second.request_time + FT_CHUNKMAP_MAX_SLICE_REASK_DELAY < now && it2->second.peers.end()==it2->second.peers.find(peer_id))
			{
				offset = it2->first;
				size   = it2->second.size ;

#ifdef DEBUG_FTCHUNK
				std::cerr << "*** ChunkMap::reAskPendingChunk: re-asking slice (" << offset << ", " << size << ") to peer " << peer_id << std::endl;
#endif

				it2->second.request_time = now ;
				it2->second.peers.insert(peer_id) ;

				return true ;
			}

	return false ;
}

// Warning: a chunk may be empty, but still being downloaded, so asking new slices from it
// will produce slices of size 0. This happens at the end of each chunk.
// --> I need to get slices from the next chunk, in such a case.
// --> solution:
// 	- have too chunks maps:
// 		1 indexed by peer id to feed the getChunk method
// 			- chunks pushed when new chunks are needed
// 			- chunks removed when empty
// 		1 indexed by chunk id to account for chunks being downloaded
// 			- chunks pushed when new chunks are needed
// 			- chunks removed when completely downloaded
//
bool ChunkMap::getDataChunk(const RsPeerId& peer_id,uint32_t size_hint,ftChunk& chunk,bool& source_chunk_map_needed)
{
#ifdef DEBUG_FTCHUNK
	std::cerr << "*** ChunkMap::getDataChunk: size_hint = " << size_hint << std::endl ;
#endif
	// 1 - find if this peer already has an active chunk.
	//
	std::map<RsPeerId,Chunk>::iterator it = _active_chunks_feed.find(peer_id) ;
	std::map<RsPeerId,Chunk>::iterator falsafe_it = _active_chunks_feed.end() ;

	if(it == _active_chunks_feed.end())		
	{
		SourceChunksInfo *sci = getSourceChunksInfo(peer_id) ;

		// 0 - Look into other pending chunks and slice from here. We only consider chunks with size smaller than 
		//    the requested size,
		//
		for(std::map<RsPeerId,Chunk>::iterator pit(_active_chunks_feed.begin());pit!=_active_chunks_feed.end();++pit)
		{
			uint32_t c = pit->second._start / _chunk_size ;

			if(!(sci->is_full || sci->cmap[c]))	// check that the chunk is available for requested peer.
				continue ;

			ChunkDownloadInfo& cdi(_slices_to_download[c]) ;
			falsafe_it = pit ;											// let's keep this one just in case.

			if(cdi._slices.rbegin() != cdi._slices.rend() && cdi._slices.rbegin()->second.size*0.7 <= (float)size_hint)
			{
				it = pit ;
#ifdef DEBUG_FTCHUNK
				std::cerr << "*** ChunkMap::getDataChunk: Sharing slice " << pit->second._start << " of peer " << pit->first << " for peer " << peer_id << std::endl;
#endif
				break ;
			}
#ifdef DEBUG_FTCHUNK
 		else
			std::cerr << "*** ChunkMap::getDataChunk: Not Sharing slice " << pit->second._start << " of peer " << pit->first << " for peer " << peer_id << ", because current peer is too slow" << std::endl;
#endif
		}

		if(it == _active_chunks_feed.end())	// not found. Find a new chunk.
		{
			// 1 - select an available chunk id for this peer.
			//
			uint32_t c = getAvailableChunk(peer_id,source_chunk_map_needed) ;	

			if(c >= _map.size()) 
				if(falsafe_it != _active_chunks_feed.end())	// no chunk available. Let's see if we can still take an active--faster--chunk.
				{
					it = falsafe_it ;
#ifdef DEBUG_FTCHUNK
					std::cerr << "*** ChunkMap::getDataChunk: Using falsafe chunk " << it->second._start << " of peer " << it->first << " for peer " << peer_id << std::endl;
#endif
				}
				else
					return false ;		// no more availabel chunks, no falsafe case.
			else
			{
				// 2 - add the chunk in the list of active chunks, and mark it as being downloaded
				//
				uint32_t soc = sizeOfChunk(c) ;
				_active_chunks_feed[peer_id] = Chunk( c*(uint64_t)_chunk_size, soc ) ;
				_map[c] = FileChunksInfo::CHUNK_ACTIVE ;
				_slices_to_download[c]._remains = soc ;			// init the list of slices to download
				it = _active_chunks_feed.find(peer_id) ;
#ifdef DEBUG_FTCHUNK
				std::cout << "*** ChunkMap::getDataChunk: Allocating new chunk " << c << " for peer " << peer_id << std::endl ;
#endif
			}
		}
	}
#ifdef DEBUG_FTCHUNK
	else
		std::cout << "*** ChunkMap::getDataChunk: Re-using chunk " << it->second._start/_chunk_size << " for peer " << peer_id << std::endl ;
#endif

	// Get the first slice of the chunk, that is at most of length size
	//
	it->second.getSlice(size_hint,chunk) ;
	_slices_to_download[chunk.offset/_chunk_size]._last_data_received = time(NULL) ;

	ChunkDownloadInfo::SliceRequestInfo& r(_slices_to_download[chunk.offset/_chunk_size]._slices[chunk.id]);

	r.size = chunk.size ;
	r.request_time = time(NULL);
	r.peers.insert(peer_id);

	chunk.peer_id = peer_id ;

#ifdef DEBUG_FTCHUNK
	std::cout << "*** ChunkMap::getDataChunk: returning slice " << chunk << " for peer " << it->first << std::endl ;
#endif
	if(it->second.empty())
		_active_chunks_feed.erase(it) ;

	return true ;
}

void ChunkMap::removeInactiveChunks(std::vector<ftChunk::OffsetInFile>& to_remove)
{
	to_remove.clear() ;
	rstime_t now = time(NULL) ;

	for(std::map<ChunkNumber,ChunkDownloadInfo>::iterator it(_slices_to_download.begin());it!=_slices_to_download.end();)
		if(now - it->second._last_data_received > (int)INACTIVE_CHUNK_TIME_LAPSE)
		{
#ifdef DEBUG_FTCHUNK
			std::cerr << "ChunkMap::removeInactiveChunks(): removing inactive chunk " << it->first << ", time lapse=" << now - it->second._last_data_received << std::endl ;
#endif
			// First, remove all slices from this chunk
			//
			std::map<ChunkNumber,ChunkDownloadInfo>::iterator tmp(it) ;

			for(std::map<ftChunk::OffsetInFile,ChunkDownloadInfo::SliceRequestInfo>::const_iterator it2(it->second._slices.begin());it2!=it->second._slices.end();++it2)
				to_remove.push_back(it2->first) ;

			_map[it->first] = FileChunksInfo::CHUNK_OUTSTANDING ;	// reset the chunk

			_total_downloaded -= (sizeOfChunk(it->first) - it->second._remains) ;	// restore completion.

			// Also remove the chunk from the chunk feed, to free the associated peer.
			//
			for(std::map<RsPeerId,Chunk>::iterator it3=_active_chunks_feed.begin();it3!=_active_chunks_feed.end();)
				if(it3->second._start == _chunk_size*uint64_t(it->first))
				{
					std::map<RsPeerId,Chunk>::iterator tmp3 = it3 ;
					++it3 ;
					_active_chunks_feed.erase(tmp3) ;
				}
				else
					++it3 ;

			++it ;
			_slices_to_download.erase(tmp) ;
		}
		else
			++it ;
}

bool ChunkMap::isChunkAvailable(uint64_t offset, uint32_t chunk_size) const
{
    return hasChunkState(offset, chunk_size, FileChunksInfo::CHUNK_DONE);
}

bool ChunkMap::isChunkOutstanding(uint64_t offset, uint32_t chunk_size) const
{
    return hasChunkState(offset, chunk_size, FileChunksInfo::CHUNK_OUTSTANDING);
}

bool ChunkMap::hasChunkState(uint64_t offset, uint32_t chunk_size, FileChunksInfo::ChunkState state) const
{
	uint32_t chunk_number_start = offset/(uint64_t)_chunk_size ;
	uint32_t chunk_number_end = (offset+(uint64_t)chunk_size)/(uint64_t)_chunk_size ;

	if((offset+(uint64_t)chunk_size) % (uint64_t)_chunk_size != 0)
		++chunk_number_end ;

	// It's possible that chunk_number_start==chunk_number_end+1, but for this we need to have
	// chunk_size=0, and offset%_chunk_size=0, so the response "true" is still valid.
	//
	for(uint32_t i=chunk_number_start;i<chunk_number_end;++i)
        if(_map[i] != state)
		{
#ifdef DEBUG_FTCHUNK
            std::cerr << "ChunkMap::hasChunkState(): (" << offset << "," << chunk_size << ") has different state" << std::endl;
#endif
			return false ;
		}

#ifdef DEBUG_FTCHUNK
    std::cerr << "ChunkMap::hasChunkState(): (" << offset << "," << chunk_size << ") check returns true" << std::endl;
#endif
	return true ;
}

void ChunkMap::setPeerAvailabilityMap(const RsPeerId& peer_id,const CompressedChunkMap& cmap)
{
#ifdef DEBUG_FTCHUNK
	std::cout << "ChunkMap::Receiving new availability map for peer " << peer_id << std::endl ;
#endif

	if(cmap._map.size() != _map.size()/32+(_map.size()%32 != 0))
	{
		std::cerr << "ChunkMap::setPeerAvailabilityMap: chunk size / number of chunks is not correct. Dropping the info. cmap.size()=" << cmap._map.size() << ", _map/32+0/1 = " << _map.size()/32+(_map.size()%32 != 0) << std::endl ;
		return ;
	}

	// sets the map.
	//
	SourceChunksInfo& mi(_peers_chunks_availability[peer_id]) ;
	mi.cmap = cmap ;
	mi.TS = time(NULL) ;
	mi.is_full = true ;

	// Checks wether the map is full of not.
	//
	for(uint32_t i=0;i<_map.size();++i)
		if(!cmap[i])
		{
			mi.is_full = false ;
			break ;
		}

#ifdef DEBUG_FTCHUNK
	std::cerr << "ChunkMap::setPeerAvailabilityMap: Setting chunk availability info for peer " << peer_id << std::endl ;
#endif
}

uint32_t ChunkMap::sizeOfChunk(uint32_t cid) const
{
	if(cid == _map.size()-1)
		return _file_size - cid*_chunk_size ;
	else
		return _chunk_size ;
}

SourceChunksInfo *ChunkMap::getSourceChunksInfo(const RsPeerId& peer_id)
{
	std::map<RsPeerId,SourceChunksInfo>::iterator it(_peers_chunks_availability.find(peer_id)) ;

	// Do we have a chunk map for this file source ?
	//  - if yes, we use it
	//  - if no, 
	//  	- if availability is assumed, let's build a plain chunkmap for it
	//  	- otherwise, refuse the transfer, but still ask for the chunkmap
	//
	//  We first test whether the source has a record of not. If not, we fill a new record. 
	//  For availability sources we fill it plain, otherwise, we fill it blank.
	//
	if(it == _peers_chunks_availability.end())
	{
		SourceChunksInfo& pchunks(_peers_chunks_availability[peer_id]) ;

		// Ok, we don't have the info, so two cases:
		// 	- peer_id is a not a turtle peer, so he is considered having the full file source, so we init with a plain chunkmap
		// 	- otherwise, a source map needs to be obtained, so we init with a blank chunkmap
		//
		if(_assume_availability)
		{
			pchunks.cmap._map.resize( CompressedChunkMap::getCompressedSize(_map.size()),~(uint32_t)0 ) ;
			pchunks.TS = 0 ;
			pchunks.is_full = true ;
		}
		else
		{
			pchunks.cmap._map.resize( CompressedChunkMap::getCompressedSize(_map.size()),0 ) ;
			pchunks.TS = 0 ;
			pchunks.is_full = false ;
		}

		it = _peers_chunks_availability.find(peer_id) ;
	}
	return &(it->second) ;
}

void ChunkMap::getSourcesList(uint32_t chunk_number,std::vector<RsPeerId>& sources) 
{
	sources.clear() ;

	for(std::map<RsPeerId,SourceChunksInfo>::const_iterator it(_peers_chunks_availability.begin());it!=_peers_chunks_availability.end();++it)
		if(it->second.cmap[chunk_number])
			sources.push_back(it->first) ;
}

uint32_t ChunkMap::getAvailableChunk(const RsPeerId& peer_id,bool& map_is_too_old) 
{
	// Quite simple strategy: Check for 1st availabe chunk for this peer starting from the given start location.
	//
	SourceChunksInfo *peer_chunks = getSourceChunksInfo(peer_id) ;

	// If the info is too old, we ask for a new one. When the map is full, we ask 10 times less, as it's probably not 
	// useful to get a new map that will also be full, but because we need to be careful not to mislead information,
	// we still keep asking.
	//
	rstime_t now = time(NULL) ;

	if((!peer_chunks->is_full) && ((int)now - (int)peer_chunks->TS > (int)SOURCE_CHUNK_MAP_UPDATE_PERIOD)) 
	{
		map_is_too_old = true ;// We will re-ask but not before some seconds.
		peer_chunks->TS = now ;
	}
	else
		map_is_too_old = false ;// the map is not too old

	uint32_t available_chunks = 0 ;
	uint32_t available_chunks_before_max_dist = 0 ;

	for(unsigned int i=0;i<_map.size();++i)
		if(_map[i] == FileChunksInfo::CHUNK_OUTSTANDING)
		{
			if(peer_chunks->is_full || peer_chunks->cmap[i])
				++available_chunks ;
		}
		else
			available_chunks_before_max_dist = available_chunks ;

	if(available_chunks > 0)
	{
		uint32_t chosen_chunk_number ;

		switch(_strategy)
		{
			case FileChunksInfo::CHUNK_STRATEGY_STREAMING:   chosen_chunk_number = 0 ;
																		    break ;
			case FileChunksInfo::CHUNK_STRATEGY_RANDOM:      chosen_chunk_number = rand() % available_chunks ;
																		    break ;
			case FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE: chosen_chunk_number = rand() % std::min(available_chunks, available_chunks_before_max_dist+FT_CHUNKMAP_MAX_CHUNK_JUMP) ;
																		    break ;
			default:
																			 chosen_chunk_number = 0 ;
		}
		uint32_t j=0 ;

		for(uint32_t i=0;i<_map.size();++i)
			if(_map[i] == FileChunksInfo::CHUNK_OUTSTANDING && (peer_chunks->is_full || peer_chunks->cmap[i]))
			{
				if(j == chosen_chunk_number)
				{
#ifdef DEBUG_FTCHUNK
					std::cerr << "ChunkMap::getAvailableChunk: returning chunk " << i << " for peer " << peer_id << std::endl;
#endif
					return i ;
				}
				else
					++j ;
			}
	}

#ifdef DEBUG_FTCHUNK
	std::cout << "!!! ChunkMap::getAvailableChunk: No available chunk from peer " << peer_id << ": returning false" << std::endl ;
#endif
	return _map.size() ;
}

void ChunkMap::getChunksInfo(FileChunksInfo& info) const 
{
	info.file_size = _file_size ;
	info.chunk_size = _chunk_size ;
	info.chunks = _map ;
	info.strategy = _strategy ;

	info.active_chunks.clear() ;

	for(std::map<ChunkNumber,ChunkDownloadInfo>::const_iterator it(_slices_to_download.begin());it!=_slices_to_download.end();++it)
		info.active_chunks.push_back(std::pair<uint32_t,uint32_t>(it->first,it->second._remains)) ;

	info.compressed_peer_availability_maps.clear() ;

	for(std::map<RsPeerId,SourceChunksInfo>::const_iterator it(_peers_chunks_availability.begin());it!=_peers_chunks_availability.end();++it)
		info.compressed_peer_availability_maps[it->first] = it->second.cmap ;
}

void ChunkMap::removeFileSource(const RsPeerId& peer_id)
{
	std::map<RsPeerId,SourceChunksInfo>::iterator it(_peers_chunks_availability.find(peer_id)) ;

	if(it == _peers_chunks_availability.end())
		return ;

	_peers_chunks_availability.erase(it) ;
}

void ChunkMap::getAvailabilityMap(CompressedChunkMap& compressed_map) const 
{
	compressed_map = CompressedChunkMap(_map) ; 

#ifdef DEBUG_FTCHUNK
	std::cerr << "ChunkMap:: retrieved availability map of size " << _map.size() << ", chunk_size=" << _chunk_size << std::endl ;
#endif
}

void ChunkMap::forceCheck()
{
	for(uint32_t i=0;i<_map.size();++i)
	{
		_map[i] = FileChunksInfo::CHUNK_CHECKING ;
		_chunks_checking_queue.push_back(i) ;
	}

	updateTotalDownloaded() ;
}

uint32_t ChunkMap::getNumberOfChunks(uint64_t size)
{
	uint64_t n = size/(uint64_t)CHUNKMAP_FIXED_CHUNK_SIZE ;

	if(size % (uint64_t)CHUNKMAP_FIXED_CHUNK_SIZE != 0)
		++n ;

	uint32_t value = n & 0xffffffffull ;

	if((uint64_t)value != n)
	{
		std::cerr << "ERROR: number of chunks is a totally absurd value. File size is " << size << ", chunks are " << n << "!!" << std::endl ;
		return 0 ;
	}
	return value ;
}

void ChunkMap::buildPlainMap(uint64_t size, CompressedChunkMap& map)
{
	uint32_t n = getNumberOfChunks(size) ;

	map = CompressedChunkMap(n,~uint32_t(0)) ;
}


