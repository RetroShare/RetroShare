#include "ftfilecreator.h"
#include <errno.h>

/*******
 * #define FILE_DEBUG 1
 ******/

#define CHUNK_MAX_AGE 30


/***********************************************************
*
*	ftFileCreator methods
*
***********************************************************/

ftFileCreator::ftFileCreator(std::string path, uint64_t size, std::string
hash, uint64_t recvd): ftFileProvider(path,size,hash) 
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

	/* initialise the Transfer Lists */
	mStart = recvd;
	mEnd = recvd;
}

bool    ftFileCreator::getFileData(uint64_t offset, 
                uint32_t chunk_size, void *data)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	if (offset + chunk_size > mStart)
	{
		/* don't have the data */
		return false;
	}
	return ftFileProvider::getFileData(offset, chunk_size, data);
}

uint64_t ftFileCreator::getRecvd()
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	return mStart;
}

bool ftFileCreator::addFileData(uint64_t offset, uint32_t chunk_size, void *data)
{
#ifdef FILE_DEBUG
	std::cerr << "ftFileCreator::addFileData(";
	std::cerr << offset;
	std::cerr << ", " << chunk_size;
	std::cerr << ", " << data << ")";
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
	if (0 != fseek(this->fd, offset, SEEK_SET))
	{
		std::cerr << "ftFileCreator::addFileData() Bad fseek" << std::endl;
		return 0;
	}
	
	long int pos;
	pos = ftell(fd);
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

	pos = ftell(fd);
	
#ifdef FILE_DEBUG
	std::cerr << "ftFileCreator::addFileData() added Data...";
	std::cerr << std::endl;
	std::cerr << " pos: " << pos;
	std::cerr << std::endl;
#endif
	/* 
	 * Notify ftFileChunker about chunks received 
	 */
	notifyReceived(offset,chunk_size);

	/* 
	 * FIXME HANDLE COMPLETION HERE - Any better way?
	 */

	return 1;
}

int ftFileCreator::initializeFileAttrs()
{
	std::cerr << "ftFileCreator::initializeFileAttrs() Filename: ";
	std::cerr << file_name;        	
	std::cerr << std::endl;

	/* 
         * check if the file exists 
         */

	if (ftFileProvider::initializeFileAttrs())
	{
		return 1;
	}

	
        RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	if (fd)
		return 1;

	{
		std::cerr << "ftFileCreator::initializeFileAttrs() opening w+b";
		std::cerr << std::endl;
	}

	/* 
         * attempt to open file 
         */
	
	fd = fopen(file_name.c_str(), "w+b");
	if (!fd)
	{
		std::cerr << "ftFileCreator::initializeFileAttrs()";
		std::cerr << " Failed to open (w+b): "<< file_name << std::endl;
		return 0;

	}

	/*
         * if it opened, find it's length 
	 * move to the end 
         */
	
	if (0 != fseek(fd, 0L, SEEK_END))
	{
        	std::cerr << "ftFileCreator::initializeFileAttrs() Seek Failed" << std::endl;
		return 0;
	}

	uint64_t recvdsize = ftell(fd);

        std::cerr << "ftFileCreator::initializeFileAttrs() File Expected Size: " << mSize << " RecvdSize: " << recvdsize << std::endl;
	
	return 1;
}
ftFileCreator::~ftFileCreator()
{
	/*
	 * FIXME Any cleanups specific to filecreator?
	 */
}


int ftFileCreator::notifyReceived(uint64_t offset, uint32_t chunk_size) 
{
	/* ALREADY LOCKED */

	/* find the chunk */
	std::map<uint64_t, ftChunk>::iterator it;
	it = mChunks.find(offset);
	bool isFirst = false;
	if (it == mChunks.end())
	{
		return 0; /* ignoring */
	}
	else if (it == mChunks.begin())
	{
		isFirst = true;
	}

	ftChunk chunk = it->second;
	mChunks.erase(it);

	if (chunk.chunk != chunk_size)
	{
		/* partial : shrink chunk */
		chunk.chunk -= chunk_size;
		chunk.offset += chunk_size;
		mChunks[chunk.offset] = chunk;
	}

	/* update how much has been completed */
	if (isFirst)
	{
		mStart = offset + chunk_size;
	}

	if (mChunks.size() == 0)
	{
		mStart = mEnd;
	}

	/* otherwise there is another earlier block to go
	 */
	return 1;
}

/* Returns true if more to get 
 * But can return size = 0, if we are still waiting for the data.
 */

bool ftFileCreator::getMissingChunk(uint64_t &offset, uint32_t &chunk) 
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	std::cerr << "ffc::getMissingChunk(...,"<< chunk << ")"<< std::endl;

	/* check start point */

	if (mStart == mSize)
	{
		std::cerr << "ffc::getMissingChunk() File Done";
		std::cerr << std::endl;
		return false;
	}

	/* check for freed chunks */
	time_t ts = time(NULL);
	time_t old = ts-CHUNK_MAX_AGE;

	std::map<uint64_t, ftChunk>::iterator it;
	for(it = mChunks.begin(); it != mChunks.end(); it++)
	{
		/* very simple algorithm */
		if (it->second.ts < old)
		{
			std::cerr << "ffc::getMissingChunk() ReAlloc";
			std::cerr << std::endl;

			/* retry this one */
			it->second.ts = ts;
			chunk = it->second.chunk;
			offset = it->second.offset;

			return true;
		}
	}

	std::cerr << "ffc::getMissingChunk() new Alloc";
	std::cerr << "  mStart: " << mStart << " mEnd: " << mEnd;
	std::cerr << "mSize: " << mSize;
	std::cerr << std::endl;

	/* else allocate a new chunk */
	if (mSize - mEnd < chunk)
		chunk = mSize - mEnd;

	offset = mEnd;
	mEnd += chunk;

	if (chunk > 0)
	{
		mChunks[offset] = ftChunk(offset, chunk, ts);
	}

	return true; /* cos more data to get */
}

/***********************************************************
*
*	ftChunk methods
*
***********************************************************/

ftChunk::ftChunk(uint64_t ioffset,uint64_t size,time_t now) 
   : offset(ioffset), chunk(size), ts(now)
{

}

ftChunk::~ftChunk()
{

}

