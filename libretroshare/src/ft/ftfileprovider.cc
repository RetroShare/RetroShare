#include "ftfileprovider.h"
#include "ftchunkmap.h"

#include "util/rsdir.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef WINDOWS_SYS
#include "util/rswin.h"
#endif // WINDOWS_SYS


static const time_t UPLOAD_CHUNK_MAPS_TIME = 30 ;	// time to ask for a new chunkmap from uploaders in seconds.

ftFileProvider::ftFileProvider(std::string path, uint64_t size, std::string
hash) : mSize(size), hash(hash), file_name(path), fd(NULL),req_loc(0),transfer_rate(0),total_size(0)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	clients_chunk_maps.clear(); 
#ifdef DEBUG_FT_FILE_PROVIDER
	std::cout << "Creating file provider for " << hash << std::endl ;
#endif
	lastTS = time(NULL) ;
	lastTS_t = lastTS ;
}

ftFileProvider::~ftFileProvider(){
#ifdef DEBUG_FT_FILE_PROVIDER
	std::cout << "Destroying file provider for " << hash << std::endl ;
#endif
	if (fd!=NULL) {
		fclose(fd);
		fd = NULL ;
	}
}

void ftFileProvider::setPeerId(const std::string& id)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	lastRequestor = id ;
}

bool	ftFileProvider::fileOk()
{
        RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	return (fd != NULL);
}

std::string ftFileProvider::getHash()
{
        RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	return hash;
}

uint64_t ftFileProvider::getFileSize()
{
        RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	return mSize;
}

bool    ftFileProvider::FileDetails(FileInfo &info)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	info.hash = hash;
	info.size = mSize;
	info.path = file_name;
	info.fname = RsDirUtil::getTopDir(file_name);
	info.transfered = req_loc ;
	info.lastTS = lastTS;
	info.status = FT_STATE_DOWNLOADING ;

	info.peers.clear() ;

	TransferInfo inf ;
	inf.peerId = lastRequestor ;
	inf.status = FT_STATE_DOWNLOADING ;

	inf.tfRate = transfer_rate/1024.0 ;
	info.tfRate = transfer_rate/1024.0 ;
	info.peers.push_back(inf) ;

	/* Use req_loc / req_size to estimate data rate */

	return true;
}

void ftFileProvider::getAvailabilityMap(CompressedChunkMap& cmap) 
{
	// We are here because the file we deal with is complete. So we return a plain map.
	//
	ChunkMap::buildPlainMap(mSize,cmap) ;
}


bool ftFileProvider::getFileData(uint64_t offset, uint32_t &chunk_size, void *data)
{
	/* dodgey checking outside of mutex...
	 * much check again inside FileAttrs().
	 */
	if (fd == NULL)
		if (!initializeFileAttrs())
			return false;

	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	/* 
	 * FIXME: Warning of comparison between unsigned and signed int?
	 */

	uint32_t data_size    = chunk_size;
	uint64_t base_loc     = offset;
	
	if (base_loc + data_size > mSize)
	{
		data_size = mSize - base_loc;
		chunk_size = mSize - base_loc;
		std::cerr <<"Chunk Size greater than total file size, adjusting chunk size " << data_size << std::endl;
	}

	if (data_size > 0)
	{	
		if(data == NULL)
		{
			std::cerr << "ftFileProvider: Warning ! Re-allocating data, which probably could not be allocated because of weird chunk size." << std::endl ;
			if(NULL == (data = malloc(data_size))) 
			{
				std::cerr << "ftFileProvider: Could not malloc a size of " << data_size << std::endl ;
				return false;
			}
		}	
		/*
                 * seek for base_loc 
                 */
		fseeko64(fd, base_loc, SEEK_SET);

		// Data space allocated by caller.
		//void *data = malloc(chunk_size);
		
		/* 
		 * read the data 
                 */
		
		if (1 != fread(data, data_size, 1, fd))
		{
                        #ifdef DEBUG_FT_FILE_PROVIDER
                        std::cerr << "ftFileProvider::getFileData() Failed to get data. Data_size=" << data_size << ", base_loc=" << base_loc << " !" << std::endl;
                        #endif
			//free(data); No!! It's already freed upwards in ftDataMultiplex::locked_handleServerRequest()
			return 0;
		}

		/* 
		 * Update status of ftFileStatus to reflect last usage (for GUI display)
		 * We need to store.
		 * (a) Id, 
		 * (b) Offset, 
		 * (c) Size, 
		 * (d) timestamp
		 */

		time_t now_t = time(NULL) ;

		long int diff = (long int)now_t - (long int)lastTS_t ;	// in bytes/s. Average over multiple samples

#ifdef DEBUG_FT_FILE_PROVIDER
		std::cout << "diff = " << diff << std::endl ;
#endif

		if(diff > 3)
		{
			transfer_rate = total_size / (float)diff ;
#ifdef DEBUG_FT_FILE_PROVIDER
			std::cout << "updated TR = " << transfer_rate << ", total_size=" << total_size << std::endl ;
#endif
			lastTS_t = now_t ;
			total_size = 0 ;
		}

		req_loc = offset;
		lastTS = time(NULL) ;
		req_size = data_size;
		total_size += req_size ;
	}
	else {
		std::cerr << "No data to read" << std::endl;
		return 0;
	}
	return 1;
}

void ftFileProvider::setClientMap(const std::string& peer_id,const CompressedChunkMap& cmap)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	std::pair<CompressedChunkMap,time_t>& map_info(clients_chunk_maps[peer_id]) ;

	map_info.first = cmap ;
	map_info.second = time(NULL) ;
}

void ftFileProvider::getClientMap(const std::string& peer_id,CompressedChunkMap& cmap,bool& map_is_too_old)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/

	std::map<std::string,std::pair<CompressedChunkMap,time_t> >::iterator it(clients_chunk_maps.find(peer_id)) ;
	
	if(it == clients_chunk_maps.end())
	{
		clients_chunk_maps[peer_id] = std::pair<CompressedChunkMap,time_t>(CompressedChunkMap(),0) ;
		it = clients_chunk_maps.find(peer_id) ;
	}
	
	if(time(NULL) - it->second.second > UPLOAD_CHUNK_MAPS_TIME)
	{
		map_is_too_old = true ;
		it->second.second = time(NULL) ;	// to avoid re-asking before the TTL
	}
	else
		map_is_too_old = false ;

	cmap = it->second.first ;
}

int ftFileProvider::initializeFileAttrs()
{
#ifdef DEBUG_FT_FILE_PROVIDER
	std::cerr << "ftFileProvider::initializeFileAttrs() Filename: " << file_name << std::endl;
#endif

	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	if (fd)
		return 1;

	/* 
	 * check if the file exists 
	 */

	{
#ifdef DEBUG_FT_FILE_PROVIDER
		std::cerr << "ftFileProvider::initializeFileAttrs() trying (r+b) " << std::endl;
#endif
	}

	/* 
	 * attempt to open file 
	 */

#ifdef WINDOWS_SYS
	std::wstring wfile_name;
	librs::util::ConvertUtf8ToUtf16(file_name, wfile_name);
	fd = _wfopen(wfile_name.c_str(), L"r+b");
#else
	fd = fopen64(file_name.c_str(), "r+b");
#endif
	if (!fd)
	{
		std::cerr << "ftFileProvider::initializeFileAttrs() Failed to open (r+b): ";
		std::cerr << file_name << std::endl;

		/* try opening read only */
#ifdef WINDOWS_SYS
		fd = _wfopen(wfile_name.c_str(), L"rb");
#else
		fd = fopen64(file_name.c_str(), "rb");
#endif
		if (!fd)
		{
			std::cerr << "ftFileProvider::initializeFileAttrs() Failed to open (rb): ";
			std::cerr << file_name << std::endl;

			/* try opening read only */
			return 0;
		}
	}

	/*
	 * if it opened, find it's length 
	 * move to the end 
	 */

//	if (0 != fseeko64(fd, 0L, SEEK_END))
//	{
//		std::cerr << "ftFileProvider::initializeFileAttrs() Seek Failed" << std::endl;
//		return 0;
//	}
//
//	uint64_t recvdsize = ftello64(fd);
//
//#ifdef DEBUG_FT_FILE_PROVIDER
//	std::cerr << "ftFileProvider::initializeFileAttrs() File Expected Size: " << mSize << " RecvdSize: " << recvdsize << std::endl;
//#endif

	return 1;
}

bool ftFileProvider::getCRC32Map(CRC32Map& crc_map)
{
	if(!initializeFileAttrs())
	{
		std::cerr << "ftFileProvider::getCRC32Map(...): ERROR: can't initialize file !" << std::endl ;
		return false ;
	}

	std::cerr << "ftFileProvider::getClientMap(): computing CRC32 map for file " << file_name << " (" << hash << ")" << std::endl ;
	return RsDirUtil::crc32File(fd,mSize,ChunkMap::CHUNKMAP_FIXED_CHUNK_SIZE,crc_map) ;
}

