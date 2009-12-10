#include <math.h>
#include <assert.h>
#include <stdlib.h>
#include "ftchunkmap.h"

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

//uint32_t Chunk::dataReceived(const ftChunk::ChunkId cid)
//{
//#ifdef DEBUG_FTCHUNK
//	std::cerr << "*** Chunk::dataReceived: slice " << cid << " finished" << std::endl ;
//#endif
//	std::map<ftChunk::ChunkId,uint32_t>::iterator it( _slices_to_download.find(cid) ) ;
//
//	if(it == _slices_to_download.end())
//	{
//		std::cerr << "!!! Chunk::dataReceived: could not find chunk " << cid << ": probably a fatal error" << std::endl ;
//		return 0 ;
//	}
//	else
//	{
//		uint32_t n = it->second ;
//		_slices_to_download.erase(it) ;
//		return n ;
//	}
//}

ChunkMap::ChunkMap(uint64_t s)
	:_file_size(s),_chunk_size(1024*1024) 	// 1MB chunks
{
	uint64_t n = s/(uint64_t)_chunk_size ;
	if(s% (uint64_t)_chunk_size != 0)
		++n ;

	_map.resize(n,FileChunksInfo::CHUNK_OUTSTANDING) ;
	_total_downloaded = 0 ;
	_strategy = FileChunksInfo::CHUNK_STRATEGY_STREAMING ;
#ifdef DEBUG_FTCHUNK
	std::cerr << "*** ChunkMap::ChunkMap: starting new chunkmap:" << std::endl ; 
	std::cerr << "   File size: " << s << std::endl ;
	std::cerr << "   Strategy: " << _strategy << std::endl ;
	std::cerr << "   ChunkSize: " << _chunk_size << std::endl ;
	std::cerr << "   Number of Chunks: " << n << std::endl ;
#endif
}

ChunkMap::ChunkMap(uint64_t file_size,
							const std::vector<uint32_t>& map,
							uint32_t chunk_size,
							uint32_t chunk_number,
							FileChunksInfo::ChunkStrategy strategy) 

	:_file_size(file_size),_chunk_size(chunk_size),_strategy(strategy)
{
#ifdef DEBUG_FTCHUNK
	std::cerr << "ChunkMap:: loading availability map of size " << map.size() << ", chunk_size=" << chunk_size << ", chunknumber = " << chunk_number << std::endl ;
#endif

	_map.clear() ;
	_map.resize(chunk_number) ;
	_total_downloaded = 0 ;

	for(uint32_t i=0;i<_map.size();++i)
	{
		uint32_t j = i & 31 ;	// i%32
		uint32_t k = i >> 5 ;	// i/32

		_map[i] = ( (map[k] & (1<<j)) > 0)?(FileChunksInfo::CHUNK_DONE) : (FileChunksInfo::CHUNK_OUTSTANDING) ;

		if(_map[i] == FileChunksInfo::CHUNK_DONE)
			_total_downloaded += _chunk_size ;
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
		assert(false) ;
		return ;
	}

	std::map<ftChunk::ChunkId,uint32_t>::iterator it(itc->second._slices.find(cid)) ;

	if(it == itc->second._slices.end()) 
	{
		std::cerr << "!!! ChunkMap::dataReceived: chunk " << cid << " is not found in slice lst of chunk number " << n << std::endl ;
		assert(false) ;
		return ;
	}

	_total_downloaded += it->second ;
	itc->second._remains -= it->second ;
	itc->second._slices.erase(it) ;

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
bool ChunkMap::getDataChunk(const std::string& peer_id,uint32_t size_hint,ftChunk& chunk)
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
			case FileChunksInfo::CHUNK_STRATEGY_STREAMING:	c = getAvailableChunk(0,peer_id) ;	// very bold!!
																			break ;

			case FileChunksInfo::CHUNK_STRATEGY_RANDOM: 		c = getAvailableChunk(rand()%_map.size(),peer_id) ;
																			break ;
			default:
#ifdef DEBUG_FTCHUNK
													std::cerr << "!!! ChunkMap::getDataChunk: error!: unknown strategy" << std::endl ;
#endif
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

	if(_active_chunks_feed[peer_id].empty())
		_active_chunks_feed.erase(_active_chunks_feed.find(peer_id)) ;
	
#ifdef DEBUG_FTCHUNK
	std::cout << "*** ChunkMap::getDataChunk: returning slice " << chunk << " for peer " << peer_id << std::endl ;
#endif
	return true ;
}

#ifdef A_FAIRE
void setPeerAvailabilityMap(const std::string& peer_id,const std::vector<uint32_t>& peer_map)
{
#ifdef DEBUG_FTCHUNK
	std::cout << "Receiving new availability map for peer " << peer_id << std::endl ;
#endif

	_peers_chunks_availability[peer_id] = peer_map ;
}
#endif

uint32_t ChunkMap::sizeOfChunk(uint32_t cid) const
{
	if(cid == _map.size()-1)
		return _file_size - (_map.size()-1)*_chunk_size ;
	else
		return _chunk_size ;
}

uint32_t ChunkMap::getAvailableChunk(uint32_t start_location,const std::string& peer_id) const
{
	// Very bold algorithm: checks for 1st availabe chunk for this peer starting
	// from the given start location.
#ifdef A_FAIRE
	std::map<std::string,std::vector<uint32_t> >::const_iterator it(_peers_chunks_availability.find(peer_id)) ;

	// Do we have records for this file source ?
	//
	if(it == _peers_chunks_availability.end())
	{
#ifdef DEBUG_FTCHUNK
		std::cout << "No chunk map for peer " << peer_id << ": returning false" << endl ;
#endif
		return _map.size() ;
	}
	const std::vector<uint32_t> peer_chunks(it->second) ;
#endif

	for(unsigned int i=0;i<_map.size();++i)
	{
		uint32_t j = (start_location+i)%_map.size() ;

		if(_map[j] == FileChunksInfo::CHUNK_OUTSTANDING /*&& peers_chunks[j] == CHUNK_DONE*/)
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
}

void ChunkMap::buildAvailabilityMap(std::vector<uint32_t>& map,uint32_t& chunk_size,uint32_t& chunk_number,FileChunksInfo::ChunkStrategy& strategy) const 
{
	chunk_size = _chunk_size ;
	chunk_number = _map.size() ;
	strategy = _strategy ;

	map.clear() ;
	map.reserve((chunk_number >> 5)+1) ;

	uint32_t r=0 ;
	for(uint32_t i=0;i<_map.size();++i)
	{
		uint32_t j = i & 31 ;
		r |= (_map[i]==FileChunksInfo::CHUNK_DONE)?(1<<j):0 ;

		if(j==31 || i==_map.size()-1)
		{
			map.push_back(r);
			r=0 ;
		}
	}
#ifdef DEBUG_FTCHUNK
	std::cerr << "ChunkMap:: built availability map of size " << map.size() << ", chunk_size=" << chunk_size << ", chunknumber = " << chunk_number << ", strategy=" << _strategy << std::endl ;
#endif
}



