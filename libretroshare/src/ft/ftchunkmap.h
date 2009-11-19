#pragma once

#include <rsiface/rstypes.h>

class ChunkMap: public FileChunksInfo
{
	public:
		ChunkMap(uint64_t size) ;

		virtual void received(uint64_t offset) ;
		virtual void requested(uint64_t offset, uint32_t chunk_size) ;
};

