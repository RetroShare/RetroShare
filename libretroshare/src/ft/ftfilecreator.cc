#include "ftfilecreator.h"
#include <errno.h>
#include <stdio.h>

/*******
 * #define FILE_DEBUG 1
 ******/

#define CHUNK_MAX_AGE 20


/***********************************************************
*
*	ftFileCreator methods
*
***********************************************************/

ftFileCreator::ftFileCreator(std::string path, uint64_t size, std::string hash, uint64_t recvd)
	: ftFileProvider(path,size,hash), chunkMap(size)
{
	/* 
         * FIXME any inits to do?
         */

#ifdef FILE_DEBUG
	std::cerr << "ftFileCreator()";
	std::cerr << std::endl;
	std::cerr << "\tpath: " << path;
	std::cerr << std::endl;
	std::cerr << "\tsize: " << size;
	std::cerr << std::endl;
	std::cerr << "\thash: " << hash;
	std::cerr << std::endl;
#endif

	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
}

bool ftFileCreator::getFileData(uint64_t offset, uint32_t &chunk_size, void *data)
{
	{
		RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
		if (offset + chunk_size > mStart)
		{
			/* don't have the data */
			return false;
		}
	}

	return ftFileProvider::getFileData(offset, chunk_size, data);
}

uint64_t ftFileCreator::getRecvd()
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	return chunkMap.getTotalReceived() ;
}

bool ftFileCreator::addFileData(uint64_t offset, uint32_t chunk_size, void *data)
{
#ifdef FILE_DEBUG
	std::cerr << "ftFileCreator::addFileData(";
	std::cerr << offset;
	std::cerr << ", " << chunk_size;
	std::cerr << ", " << data << ")";
	std::cerr << " this: " << this;
	std::cerr << std::endl;
#endif
	/* dodgey checking outside of mutex...
	 * much check again inside FileAttrs().
	 */
	/* Check File is open */
	if (fd == NULL)
		if (!initializeFileAttrs())
			return false;

	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	/* 
	 * check its at the correct location 
	 */
	if (offset + chunk_size > mSize)
	{
		chunk_size = mSize - offset;
		std::cerr <<"Chunk Size greater than total file size, adjusting chunk "	<< "size " << chunk_size << std::endl;

	}

	/* 
	 * go to the offset of the file 
	 */
	if (0 != fseeko64(this->fd, offset, SEEK_SET))
	{
		std::cerr << "ftFileCreator::addFileData() Bad fseek" << std::endl;
		return 0;
	}
	
	uint64_t pos;
	pos = ftello64(fd);
	/* 
	 * add the data 
 	 */
	//void *data2 = malloc(chunk_size);
        //std::cerr << "data2: " << data2 << std::endl;
	//if (1 != fwrite(data2, chunk_size, 1, this->fd))

	if (1 != fwrite(data, chunk_size, 1, this->fd))
	{
        	std::cerr << "ftFileCreator::addFileData() Bad fwrite" << std::endl;
        	std::cerr << "ERRNO: " << errno << std::endl;

		return 0;
	}

	pos = ftello64(fd);
	
#ifdef FILE_DEBUG
	std::cerr << "ftFileCreator::addFileData() added Data...";
	std::cerr << std::endl;
	std::cerr << " pos: " << pos;
	std::cerr << std::endl;
#endif
	/* 
	 * Notify ftFileChunker about chunks received 
	 */
	locked_notifyReceived(offset,chunk_size);

	/* 
	 * FIXME HANDLE COMPLETION HERE - Any better way?
	 */

	return 1;
}

int ftFileCreator::initializeFileAttrs()
{
	std::cerr << "ftFileCreator::initializeFileAttrs() Filename: ";
	std::cerr << file_name;        	
	std::cerr << " this: " << this;
	std::cerr << std::endl;

	/* 
         * check if the file exists 
	 * cant use FileProviders verion because that opens readonly.
         */

        RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	if (fd)
		return 1;

	/* 
         * check if the file exists 
         */
	
	{
		std::cerr << "ftFileCreator::initializeFileAttrs() trying (r+b) ";        	
		std::cerr << std::endl;
	}

	/* 
         * attempt to open file 
         */
	
	fd = fopen64(file_name.c_str(), "r+b");
	if (!fd)
	{
		std::cerr << "ftFileCreator::initializeFileAttrs() Failed to open (r+b): ";
		std::cerr << file_name << std::endl;

		std::cerr << "ftFileCreator::initializeFileAttrs() opening w+b";
		std::cerr << std::endl;

		/* try opening for write */
		fd = fopen64(file_name.c_str(), "w+b");
		if (!fd)
		{
			std::cerr << "ftFileCreator::initializeFileAttrs()";
			std::cerr << " Failed to open (w+b): "<< file_name << std::endl;
			return 0;
		}
	}




	/*
         * if it opened, find it's length 
	 * move to the end 
         */
	
	if (0 != fseeko64(fd, 0L, SEEK_END))
	{
        	std::cerr << "ftFileCreator::initializeFileAttrs() Seek Failed" << std::endl;
		return 0;
	}

	uint64_t recvdsize = ftello64(fd);

        std::cerr << "ftFileCreator::initializeFileAttrs() File Expected Size: " << mSize << " RecvdSize: " << recvdsize << std::endl;

	/* start from there! */
//	mStart = recvdsize;
//	mEnd = recvdsize;
	
	return 1;
}
ftFileCreator::~ftFileCreator()
{
	/*
	 * FIXME Any cleanups specific to filecreator?
	 */
}


int ftFileCreator::locked_notifyReceived(uint64_t offset, uint32_t chunk_size) 
{
	/* ALREADY LOCKED */
#ifdef FILE_DEBUG
	std::cerr << "ftFileCreator::locked_notifyReceived( " << offset;
	std::cerr << ", " << chunk_size << " )";
	std::cerr << " this: " << this;
	std::cerr << std::endl;
#endif

	/* find the chunk */
	std::map<uint64_t, ftChunk>::iterator it = mChunks.find(offset);

//	bool isFirst = false;
	if (it == mChunks.end())
	{
#ifdef FILE_DEBUG
		std::cerr << "ftFileCreator::locked_notifyReceived() ";
		std::cerr << " Failed to match to existing chunk - ignoring";
		std::cerr << std::endl;

		locked_printChunkMap();
#endif
		return 0; /* ignoring */
	}

//	if (it == mChunks.begin())
//	{
//		isFirst = true;
//	}

	ftChunk chunk = it->second;
	mChunks.erase(it);

	if (chunk.size != chunk_size)
	{
		/* partial : shrink chunk */
		chunk.size -= chunk_size;
		chunk.offset += chunk_size;
		mChunks[chunk.offset] = chunk;
	}
	else	// notify the chunkmap that the slice is finished
		chunkMap.dataReceived(chunk.id) ;

//	/* update how much has been completed */
//	if (isFirst)
//	{
//		mStart = offset + chunk_size;
//	}

	// update chunk map

//	if (mChunks.size() == 0)
//	{
//		mStart = mEnd;
//	}

	/* otherwise there is another earlier block to go
	 */

	return 1;
}

void ftFileCreator::setChunkStrategy(FileChunksInfo::ChunkStrategy s)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

#ifdef FILE_DEBUG
	std::cerr << "ftFileCtreator: setting chunk strategy to " << s << std::endl ;
#endif
	chunkMap.setStrategy(s) ;
}

/* Returns true if more to get 
 * But can return size = 0, if we are still waiting for the data.
 */

bool ftFileCreator::getMissingChunk(const std::string& peer_id,uint32_t size_hint,uint64_t &offset, uint32_t& size) 
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
#ifdef FILE_DEBUG
	std::cerr << "ffc::getMissingChunk(...,"<< size_hint << ")";
	std::cerr << " this: " << this;
	std::cerr << std::endl;
	locked_printChunkMap();
#endif

	/* check start point */

//	if(mStart == mSize)
//	{
//#ifdef FILE_DEBUG
//		std::cerr << "ffc::getMissingChunk() File Done";
//		std::cerr << std::endl;
//#endif
//		return false;
//	}

	/* check for freed chunks */
	time_t ts = time(NULL);
	time_t old = ts-CHUNK_MAX_AGE;

	std::map<uint64_t, ftChunk>::iterator it;
	for(it = mChunks.begin(); it != mChunks.end(); it++)
	{
		/* very simple algorithm */
		if (it->second.ts < old)
		{
#ifdef FILE_DEBUG
			std::cerr << "ffc::getMissingChunk() Re-asking for an old chunk";
			std::cerr << std::endl;
#endif

			/* retry this one */
			it->second.ts = ts;
			size = it->second.size;
			offset = it->second.offset;

			return true;
		}
	}

	/* else allocate a new chunk */

	ftChunk chunk ;

	if(!chunkMap.getDataChunk(peer_id,size_hint,chunk))
		return false ;

//	if (mSize - mEnd < chunk)
//		chunk = mSize - mEnd;
//
//	offset = mEnd;
//	mEnd += chunk;

//	if (chunk > 0)
//	{
#ifdef FILE_DEBUG
	std::cerr << "ffc::getMissingChunk() Retrieved new chunk: " << chunk << std::endl ;
//		std::cerr << std::endl;
//		std::cerr << "  mStart: " << mStart << " mEnd: " << mEnd;
//		std::cerr << "mSize: " << mSize;
//		std::cerr << std::endl;
#endif

	mChunks[chunk.offset] = chunk ;

	offset = chunk.offset ;
	size = chunk.size ;
//	}

	return true; /* cos more data to get */
}

void ftFileCreator::getChunkMap(FileChunksInfo& info)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	chunkMap.getChunksInfo(info) ;
}

bool ftFileCreator::locked_printChunkMap()
{
#ifdef FILE_DEBUG
	std::cerr << "ftFileCreator::locked_printChunkMap()";
	std::cerr << " this: " << this;
	std::cerr << std::endl;
#endif

	/* check start point */
//	std::cerr << "Size: " << mSize << " Start: " << mStart << " End: " << mEnd;
	std::cerr << "\tOutstanding Chunks:";
	std::cerr << std::endl;

	std::map<uint64_t, ftChunk>::iterator it;
	
	for(it = mChunks.begin(); it != mChunks.end(); it++)
		std::cerr << "  " << it->second << std::endl ;

	return true; 
}

void ftFileCreator::loadAvailabilityMap(const std::vector<uint32_t>& map,uint32_t chunk_size,uint32_t chunk_number,uint32_t strategy) 
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	chunkMap = ChunkMap(mSize,map,chunk_size,chunk_number,FileChunksInfo::ChunkStrategy(strategy)) ;
}

void ftFileCreator::storeAvailabilityMap(std::vector<uint32_t>& map,uint32_t& chunk_size,uint32_t& chunk_number,uint32_t& strategy) 
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	FileChunksInfo::ChunkStrategy strat ;
	chunkMap.buildAvailabilityMap(map,chunk_size,chunk_number,strat) ;
	strategy = (uint32_t)strat ;
}

void ftFileCreator::setSourceMap(const std::string& peer_id,uint32_t chunk_size,uint32_t nb_chunks,const std::vector<uint32_t>& compressed_map)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	// At this point, we should cancel all file chunks that are asked to the
	// peer and which this peer actually doesn't possesses. Otherwise, the transfer may get stuck. 
	// This should be done by:
	// 	- first setting the peer availability map
	// 	- then asking the chunkmap which chunks are being downloaded, but actually shouldn't
	// 	- cancelling them in the ftFileCreator, so that they can be re-asked later to another peer.
	//
	chunkMap.setPeerAvailabilityMap(peer_id,chunk_size,nb_chunks,compressed_map) ;
}


