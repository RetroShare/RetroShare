#ifdef DEBUG_FTCHUNK
#include <assert.h>
#endif
#include <math.h>
#include <stdlib.h>
#include <rsiface/rspeers.h>
#include <rsiface/rsturtle.h>
#include "ftchunkmap.h"
#include <time.h>

static const uint32_t SOURCE_CHUNK_MAP_UPDATE_PERIOD	= 60 ; //! TTL for chunkmap info
static const uint32_t INACTIVE_CHUNK_TIME_LAPSE 		= 60 ; //! TTL for an inactive chunk

std::ostream& operator<<(std::ostream& o,const ftChunk& c)
{
	return o << "\tChunk [" << c.offset << "] size: " << c.size << "  ChunkId: " << c.id << "  Age: " << time(NULL) - c.ts ;
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

ChunkMap::ChunkMap(uint64_t s)
	:	_file_size(s),
		_chunk_size(CHUNKMAP_FIXED_CHUNK_SIZE) // 1MB chunks
{
	uint64_t n = s/(uint64_t)_chunk_size ;
	if(s% (uint64_t)_chunk_size != 0)
		++n ;

	_map.resize(n,FileChunksInfo::CHUNK_OUTSTANDING) ;
	_strategy = FileChunksInfo::CHUNK_STRATEGY_STREAMING ;
	_total_downloaded = 0 ;
	_file_is_complete = false ;
#ifdef DEBUG_FTCHUNK
	std::cerr << "*** ChunkMap::ChunkMap: starting new chunkmap:" << std::endl ; 
	std::cerr << "   File size: " << s << std::endl ;
	std::cerr << "   Strategy: " << _strategy << std::endl ;
	std::cerr << "   ChunkSize: " << _chunk_size << std::endl ;
	std::cerr << "   Number of Chunks: " << n << std::endl ;
#endif
}

void ChunkMap::setAvailabilityMap(const CompressedChunkMap& map)
{
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

void ChunkMap::dataReceived(const ftChunk::ChunkId& cid)
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

	std::map<ftChunk::ChunkId,uint32_t>::iterator it(itc->second._slices.find(cid)) ;

	if(it == itc->second._slices.end()) 
	{
		std::cerr << "!!! ChunkMap::dataReceived: chunk " << cid << " is not found in slice lst of chunk number " << n << std::endl ;
#ifdef DEBUG_FTCHUNK
		assert(false) ;
#endif
		return ;
	}

	_total_downloaded += it->second ;
	itc->second._remains -= it->second ;
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
		_map[n] = FileChunksInfo::CHUNK_DONE ;
		_slices_to_download.erase(itc) ;

		// We also check whether the file is complete or not.

		_file_is_complete = true ;

		for(uint32_t i=0;i<_map.size();++i)
			if(_map[i] != FileChunksInfo::CHUNK_DONE)
			{
				_file_is_complete = false ;
				break ;
			}
	}
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
bool ChunkMap::getDataChunk(const std::string& peer_id,uint32_t size_hint,ftChunk& chunk,bool& source_chunk_map_needed)
{
#ifdef DEBUG_FTCHUNK
	std::cerr << "*** ChunkMap::getDataChunk: size_hint = " << size_hint << std::endl ;
#endif
	// 1 - find if this peer already has an active chunk.
	//
	std::map<std::string,Chunk>::iterator it = _active_chunks_feed.find(peer_id) ;

	if(it == _active_chunks_feed.end())		
	{
		// 1 - select an available chunk id for this peer.
		//
		uint32_t c ;

		switch(_strategy)
		{
			case FileChunksInfo::CHUNK_STRATEGY_STREAMING:	c = getAvailableChunk(0,peer_id,source_chunk_map_needed) ;	// very bold!!
																			break ;

			case FileChunksInfo::CHUNK_STRATEGY_RANDOM: 		c = getAvailableChunk(rand()%_map.size(),peer_id,source_chunk_map_needed) ;
																			break ;
			default:
													std::cerr << "!!! ChunkMap::getDataChunk: error!: unknown strategy" << std::endl ;
													return false ;
		}

		if(c >= _map.size()) 
			return false ;

		// 2 - add the chunk in the list of active chunks, and mark it as being downloaded
		//
		uint32_t soc = sizeOfChunk(c) ;
		_active_chunks_feed[peer_id] = Chunk( c*(uint64_t)_chunk_size, soc ) ;
		_map[c] = FileChunksInfo::CHUNK_ACTIVE ;
		_slices_to_download[c]._remains = soc ;			// init the list of slices to download
#ifdef DEBUG_FTCHUNK
		std::cout << "*** ChunkMap::getDataChunk: Allocating new chunk " << c << " for peer " << peer_id << std::endl ;
#endif
	}
#ifdef DEBUG_FTCHUNK
	else
		std::cout << "*** ChunkMap::getDataChunk: Re-using chunk " << it->second._start/_chunk_size << " for peer " << peer_id << std::endl ;
#endif

	// Get the first slice of the chunk, that is at most of length size
	//
	_active_chunks_feed[peer_id].getSlice(size_hint,chunk) ;
	_slices_to_download[chunk.offset/_chunk_size]._slices[chunk.id] = chunk.size ;
	_slices_to_download[chunk.offset/_chunk_size]._last_data_received = time(NULL) ;

	if(_active_chunks_feed[peer_id].empty())
		_active_chunks_feed.erase(_active_chunks_feed.find(peer_id)) ;
	
#ifdef DEBUG_FTCHUNK
	std::cout << "*** ChunkMap::getDataChunk: returning slice " << chunk << " for peer " << peer_id << std::endl ;
#endif
	return true ;
}

void ChunkMap::removeInactiveChunks(std::vector<ftChunk::ChunkId>& to_remove)
{
	to_remove.clear() ;
	time_t now = time(NULL) ;

	for(std::map<ChunkNumber,ChunkDownloadInfo>::iterator it(_slices_to_download.begin());it!=_slices_to_download.end();)
		if(now - it->second._last_data_received > (int)INACTIVE_CHUNK_TIME_LAPSE)
		{
#ifdef DEBUG_FTCHUNK
			std::cerr << "ChunkMap::removeInactiveChunks(): removing inactive chunk " << it->first << ", time lapse=" << now - it->second._last_data_received << std::endl ;
#endif
			// First, remove all slices from this chunk
			//
			std::map<ChunkNumber,ChunkDownloadInfo>::iterator tmp(it) ;

			for(std::map<ftChunk::ChunkId,uint32_t>::const_iterator it2(it->second._slices.begin());it2!=it->second._slices.end();++it2)
				to_remove.push_back(it2->first) ;

			_map[it->first] = FileChunksInfo::CHUNK_OUTSTANDING ;	// reset the chunk

			_total_downloaded -= (sizeOfChunk(it->first) - it->second._remains) ;	// restore completion.

			// Also remove the chunk from the chunk feed, to free the associated peer.
			//
			for(std::map<std::string,Chunk>::iterator it3=_active_chunks_feed.begin();it3!=_active_chunks_feed.end();)
				if(it3->second._start == _chunk_size*uint64_t(it->first))
				{
					std::map<std::string,Chunk>::iterator tmp3 = it3 ;
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
	uint32_t chunk_number_start = offset/(uint64_t)_chunk_size ;
	uint32_t chunk_number_end = (offset+(uint64_t)chunk_size)/(uint64_t)_chunk_size ;

	if((offset+(uint64_t)chunk_size) % (uint64_t)_chunk_size == 0)
		--chunk_number_end ;

	// It's possible that chunk_number_start==chunk_number_end+1, but for this we need to have
	// chunk_size=0, and offset%_chunk_size=0, so the response "true" is still valid.
	//
	for(uint32_t i=chunk_number_start;i!=chunk_number_end;++i)
		if(_map[i] != FileChunksInfo::CHUNK_DONE)
			return false ;

	return true ;
}

void ChunkMap::setPeerAvailabilityMap(const std::string& peer_id,const CompressedChunkMap& cmap)
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
		return _file_size - (_map.size()-1)*_chunk_size ;
	else
		return _chunk_size ;
}

uint32_t ChunkMap::getAvailableChunk(uint32_t start_location,const std::string& peer_id,bool& map_is_too_old) 
{
	// Quite simple strategy: Check for 1st availabe chunk for this peer starting from the given start location.
	//
	std::map<std::string,SourceChunksInfo>::iterator it(_peers_chunks_availability.find(peer_id)) ;
	SourceChunksInfo *peer_chunks = NULL;

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

		bool assume_availability = !rsTurtle->isTurtlePeer(peer_id) ;

		// Ok, we don't have the info, so two cases:
		// 	- peer_id is a not a turtle peer, so he is considered having the full file source, so we init with a plain chunkmap
		// 	- otherwise, a source map needs to be obtained, so we init with a blank chunkmap
		//
		if(assume_availability)
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
	peer_chunks = &(it->second) ;

	// If the info is too old, we ask for a new one. When the map is full, we ask 10 times less, as it's probably not 
	// useful to get a new map that will also be full, but because we need to be careful not to mislead information,
	// we still keep asking.
	//
	time_t now = time(NULL) ;

	if((!peer_chunks->is_full) && ((int)now - (int)peer_chunks->TS > (int)SOURCE_CHUNK_MAP_UPDATE_PERIOD)) 
	{
		map_is_too_old = true ;// We will re-ask but not before some seconds.
		peer_chunks->TS = now ;
	}
	else
		map_is_too_old = false ;// the map is not too old

	for(unsigned int i=0;i<_map.size();++i)
	{
		uint32_t j = (start_location+i)%(int)_map.size() ;	// index of the chunk

		if(_map[j] == FileChunksInfo::CHUNK_OUTSTANDING && (peer_chunks->is_full || peer_chunks->cmap[j]))
		{
#ifdef DEBUG_FTCHUNK
			std::cerr << "ChunkMap::getAvailableChunk: returning chunk " << j << " for peer " << peer_id << std::endl;
#endif
			return j ;
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

	for(std::map<std::string,SourceChunksInfo>::const_iterator it(_peers_chunks_availability.begin());it!=_peers_chunks_availability.end();++it)
		info.compressed_peer_availability_maps[it->first] = it->second.cmap ;
}

void ChunkMap::removeFileSource(const std::string& peer_id)
{
	std::map<std::string,SourceChunksInfo>::iterator it(_peers_chunks_availability.find(peer_id)) ;

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


