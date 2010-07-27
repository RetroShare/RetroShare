#include "ftfilecreator.h"
#include <errno.h>
#include <stdio.h>
#include <util/rsdiscspace.h>
#include <util/rsdir.h>

/*******
 * #define FILE_DEBUG 1
 ******/

#define CHUNK_MAX_AGE 20


/***********************************************************
*
*	ftFileCreator methods
*
***********************************************************/

ftFileCreator::ftFileCreator(std::string path, uint64_t size, std::string hash)
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
	_last_recv_time_t = time(NULL) ;
}

bool ftFileCreator::getFileData(const std::string& peer_id,uint64_t offset, uint32_t &chunk_size, void *data)
{
	// Only send the data if we actually have it.
	//
	bool have_it = false ;
	{
		RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

		have_it = chunkMap.isChunkAvailable(offset, chunk_size) ;
	}

	if(have_it)
		return ftFileProvider::getFileData(peer_id,offset, chunk_size, data);
	else
		return false ;
}

time_t ftFileCreator::lastRecvTimeStamp() 
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	return _last_recv_time_t ;
}
void ftFileCreator::resetRecvTimeStamp() 
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	_last_recv_time_t = time(NULL) ;
}

void ftFileCreator::closeFile()
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	if(fd != NULL)
	{
#ifdef FILE_DEBUG
		std::cerr << "CLOSED FILE " << (void*)fd << " (" << file_name << ")." << std::endl ;
#endif
		fclose(fd) ;
	}

	fd = NULL ;
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
	/* dodgey checking outside of mutex...  much check again inside FileAttrs(). */
	/* Check File is open */

	if(!RsDiscSpace::checkForDiscSpace(RS_PARTIALS_DIRECTORY))
		return false ;

	bool complete = false ;
	{
		RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

		if (fd == NULL)
			if (!locked_initializeFileAttrs())
				return false;

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
			std::cerr << "ftFileCreator::addFileData() Bad fseek at offset " << offset << ", fd=" << (void*)(this->fd) << ", size=" << mSize << ", errno=" << errno << std::endl;
			return 0;
		}

		if (1 != fwrite(data, chunk_size, 1, this->fd))
		{
			std::cerr << "ftFileCreator::addFileData() Bad fwrite." << std::endl;
			std::cerr << "ERRNO: " << errno << std::endl;

			return 0;
		}

#ifdef FILE_DEBUG
		std::cerr << "ftFileCreator::addFileData() added Data...";
		std::cerr << std::endl;
		std::cerr << " pos: " << offset;
		std::cerr << std::endl;
#endif
		/* 
		 * Notify ftFileChunker about chunks received 
		 */
		locked_notifyReceived(offset,chunk_size);

		complete = chunkMap.isComplete();
	}
	if(complete)
	{
#ifdef FILE_DEBUG
		std::cerr << "ftFileCreator::addFileData() File is complete: closing" << std::endl ;
#endif
		closeFile();
	}
  
	/* 
	 * FIXME HANDLE COMPLETION HERE - Any better way?
	 */

	return 1;
}

void ftFileCreator::removeInactiveChunks()
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

#ifdef FILE_DEBUG
	std::cerr << "ftFileCreator::removeInactiveChunks(): looking for old chunks." << std::endl ;
#endif
	std::vector<ftChunk::ChunkId> to_remove ;

	chunkMap.removeInactiveChunks(to_remove) ;

#ifdef FILE_DEBUG
	if(!to_remove.empty())
		std::cerr << "ftFileCreator::removeInactiveChunks(): removing slice ids: " ;
#endif
	// This double loop looks costly, but it's called on very few chunks, and not often, so it's ok.
	//
	for(uint32_t i=0;i<to_remove.size();++i)
	{
#ifdef FILE_DEBUG
		std::cerr << to_remove[i] << " " ;
#endif
		for(std::map<uint64_t,ftChunk>::iterator it(mChunks.begin());it!=mChunks.end();)
			if(it->second.id == to_remove[i])
			{
				std::map<uint64_t,ftChunk>::iterator tmp(it) ;
				++it ;
				mChunks.erase(tmp) ;
			}
			else
				++it ;
	}
#ifdef FILE_DEBUG
	if(!to_remove.empty())
		std::cerr << std::endl ;
#endif
}

void ftFileCreator::removeFileSource(const std::string& peer_id)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
#ifdef FILE_DEBUG
	std::cerr << "ftFileCreator:: removign file source " << peer_id << " from chunkmap." << std::endl ;
#endif
	chunkMap.removeFileSource(peer_id) ;
}

int ftFileCreator::locked_initializeFileAttrs()
{
#ifdef FILE_DEBUG
	std::cerr << "ftFileCreator::initializeFileAttrs() Filename: " << file_name << " this: " << this << std::endl;
#endif

	/* 
	 * check if the file exists 
	 * cant use FileProviders verion because that opens readonly.
	 */

	if (fd)
		return 1;

	/* 
	 * check if the file exists 
	 */

	{
#ifdef FILE_DEBUG
		std::cerr << "ftFileCreator::initializeFileAttrs() trying (r+b) " << file_name << " this: " << this << std::endl;
#endif
		std::cerr << std::endl;
	}

	/* 
	 * attempt to open file 
	 */

	fd = fopen64(file_name.c_str(), "r+b");

	if (!fd)
	{
		std::cerr << "ftFileCreator::initializeFileAttrs() Failed to open (r+b): ";
		std::cerr << file_name << ", errno = " << errno << std::endl;

		std::cerr << "ftFileCreator::initializeFileAttrs() opening w+b";
		std::cerr << std::endl;

		/* try opening for write */
		fd = fopen64(file_name.c_str(), "w+b");
		if (!fd)
		{
			std::cerr << "ftFileCreator::initializeFileAttrs()";
			std::cerr << " Failed to open (w+b): "<< file_name << ", errno = " << errno << std::endl;
			return 0;
		}
	}
#ifdef FILE_DEBUG
	std::cerr << "OPENNED FILE " << (void*)fd << " (" << file_name << "), for r/w." << std::endl ;
#endif

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

	_last_recv_time_t = time(NULL) ;

	/* otherwise there is another earlier block to go
	 */

	return 1;
}

FileChunksInfo::ChunkStrategy ftFileCreator::getChunkStrategy()
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	return chunkMap.getStrategy() ;
}
void ftFileCreator::setChunkStrategy(FileChunksInfo::ChunkStrategy s)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	// Let's check, for safety.
	if(s != FileChunksInfo::CHUNK_STRATEGY_STREAMING && s != FileChunksInfo::CHUNK_STRATEGY_RANDOM)
	{
		std::cerr << "ftFileCreator::ERROR: invalid chunk strategy " << s << "!" << " setting default value " << FileChunksInfo::CHUNK_STRATEGY_STREAMING << std::endl ;
		s = FileChunksInfo::CHUNK_STRATEGY_STREAMING ;
	}

#ifdef FILE_DEBUG
	std::cerr << "ftFileCtreator: setting chunk strategy to " << s << std::endl ;
#endif
	chunkMap.setStrategy(s) ;
}

/* Returns true if more to get 
 * But can return size = 0, if we are still waiting for the data.
 */

bool ftFileCreator::getMissingChunk(const std::string& peer_id,uint32_t size_hint,uint64_t &offset, uint32_t& size,bool& source_chunk_map_needed) 
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
#ifdef FILE_DEBUG
	std::cerr << "ffc::getMissingChunk(...,"<< size_hint << ")";
	std::cerr << " this: " << this;
	std::cerr << std::endl;
	locked_printChunkMap();
#endif
	source_chunk_map_needed = false ;

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

	if(!chunkMap.getDataChunk(peer_id,size_hint,chunk,source_chunk_map_needed))
		return false ;

#ifdef FILE_DEBUG
	std::cerr << "ffc::getMissingChunk() Retrieved new chunk: " << chunk << std::endl ;
#endif

	mChunks[chunk.offset] = chunk ;

	offset = chunk.offset ;
	size = chunk.size ;

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
	std::cerr << "\tOutstanding Chunks:";
	std::cerr << std::endl;

	std::map<uint64_t, ftChunk>::iterator it;
	
	for(it = mChunks.begin(); it != mChunks.end(); it++)
		std::cerr << "  " << it->second << std::endl ;

	return true; 
}

void ftFileCreator::setAvailabilityMap(const CompressedChunkMap& cmap) 
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	chunkMap.setAvailabilityMap(cmap) ;
}

void ftFileCreator::getAvailabilityMap(CompressedChunkMap& map) 
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	chunkMap.getAvailabilityMap(map) ;
}

void ftFileCreator::setSourceMap(const std::string& peer_id,const CompressedChunkMap& compressed_map)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	// At this point, we should cancel all file chunks that are asked to the
	// peer and which this peer actually doesn't possesses. Otherwise, the transfer may get stuck. 
	// This should be done by:
	// 	- first setting the peer availability map
	// 	- then asking the chunkmap which chunks are being downloaded, but actually shouldn't
	// 	- cancelling them in the ftFileCreator, so that they can be re-asked later to another peer.
	//
	chunkMap.setPeerAvailabilityMap(peer_id,compressed_map) ;
}

bool ftFileCreator::finished() 
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	return chunkMap.isComplete() ;
}

bool ftFileCreator::hashReceivedData(std::string& hash)
{
	// csoler: No mutex here please !
	//
	// This is a bit dangerous, but otherwise we might stuck the GUI for a 
	// long time. Therefore, we must pay attention not to call this function
	// at a time file_name nor hash can be modified, which is easy.
	//
	if(!finished())
		return false ;

	return RsDirUtil::hashFile(file_name,hash) ;
}

bool ftFileCreator::crossCheckChunkMap(const CRC32Map& ref,uint32_t& bad_chunks,uint32_t& incomplete_chunks)
{
	{
		RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

		CompressedChunkMap map ;
		chunkMap.getAvailabilityMap(map) ;
		uint32_t nb_chunks = ref.size() ;
		static const uint32_t chunk_size = ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE ;

		std::cerr << "ftFileCreator::crossCheckChunkMap(): comparing chunks..." << std::endl;

		if(!locked_initializeFileAttrs() )
			return false ;

		unsigned char *buff = new unsigned char[chunk_size] ;
		incomplete_chunks = 0 ;
		bad_chunks = 0 ;
		uint32_t len = 0 ;

		for(uint32_t i=0;i<nb_chunks;++i)
			if(map[i])
				if(fseek(fd,(uint64_t)i * (uint64_t)chunk_size,SEEK_SET)==0 && (len = fread(buff,1,chunk_size,fd)) > 0)
				{
					if( RsDirUtil::rs_CRC32(buff,len) != ref[i])
					{
						++bad_chunks ;
						++incomplete_chunks ;
						map.reset(i) ;
					}
				}
				else
					return false ;
			else
				++incomplete_chunks ;

		delete[] buff ;

		if(bad_chunks > 0)
			chunkMap.setAvailabilityMap(map) ;
	}
	closeFile() ;
	return true ;
}


