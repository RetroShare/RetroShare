#include <math.h>
#include "ftchunkmap.h"

ChunkMap::ChunkMap(uint64_t s)
{
	file_size = s ;

	chunk_size = 1024*1024 ;	// 1MB chunks
	uint64_t n = s/chunk_size ;
	if(s% (uint64_t)chunk_size != 0)
		++n ;

	chunks.resize(n,CHUNK_OUTSTANDING) ;
}

void ChunkMap::received(uint64_t offset)
{
	uint64_t n = offset/chunk_size ;

	for(uint64_t i=0;i<n;++i)
		chunks[i] = CHUNK_DONE ;

	if(offset == file_size)
		chunks.back() = CHUNK_DONE ;
}

void ChunkMap::requested(uint64_t offset, uint32_t size)
{
	int start = offset/chunk_size ;
	int end = (int)ceil((offset+size)/chunk_size) ;

	for(int i=start;i<=end;++i)
		chunks[i] = CHUNK_ACTIVE ;
}
