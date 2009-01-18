#include "ftfileprovider.h"

#include "util/rsdir.h"
#include <stdlib.h>

ftFileProvider::ftFileProvider(std::string path, uint64_t size, std::string
hash) : mSize(size), hash(hash), file_name(path), fd(NULL) 
{
}

ftFileProvider::~ftFileProvider(){
	if (fd!=NULL) {
		fclose(fd);
	}
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
        info.lastTS = lastTS;
	/* Use req_loc / req_size to estimate data rate */

	return true;
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
	 * Use private data, which has a pointer to file, total size etc
	 */

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
		/*
                 * seek for base_loc 
                 */
		fseek(fd, base_loc, SEEK_SET);

		// Data space allocated by caller.
		//void *data = malloc(chunk_size);
		
		/* 
		 * read the data 
                 */
		
		if (1 != fread(data, data_size, 1, fd))
		{
			std::cerr << "ftFileProvider::getFileData() Failed to get data!";
			free(data);
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

		time_t now = time(NULL);		
		req_loc = offset;
		req_size = data_size;
		lastTS = now;
	}
	else {
		std::cerr << "No data to read" << std::endl;
		return 0;
	}
	return 1;
}

int ftFileProvider::initializeFileAttrs()
{
	std::cerr << "ftFileProvider::initializeFileAttrs() Filename: ";
	std::cerr << file_name;        	
	std::cerr << std::endl;
	
        RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	if (fd)
		return 1;

	/* 
         * check if the file exists 
         */
	
	{
		std::cerr << "ftFileProvider::initializeFileAttrs() trying (r+b) ";        	
		std::cerr << std::endl;
	}

	/* 
         * attempt to open file 
         */
	
	fd = fopen(file_name.c_str(), "rb");
	if (!fd)
	{
		std::cerr << "ftFileProvider::initializeFileAttrs() Failed to open (r+b): ";
		std::cerr << file_name << std::endl;

		/* try opening read only */
		fd = fopen(file_name.c_str(), "rb");
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
	
	if (0 != fseek(fd, 0L, SEEK_END))
	{
        	std::cerr << "ftFileProvider::initializeFileAttrs() Seek Failed" << std::endl;
		return 0;
	}

	uint64_t recvdsize = ftell(fd);

        std::cerr << "ftFileProvider::initializeFileAttrs() File Expected Size: " << mSize << " RecvdSize: " << recvdsize << std::endl;
	
	return 1;
}
