#include "ftfileprovider.h"

#include "util/rsdir.h"

ftFileProvider::ftFileProvider(std::string path, uint64_t size, std::string
hash) : total_size(size), hash(hash), file_name(path), fd(NULL) {
		
}

ftFileProvider::~ftFileProvider(){
	if (fd!=NULL) {
		fclose(fd);
	}
}

std::string ftFileProvider::getHash()
{
	return hash;
}

uint64_t ftFileProvider::getFileSize()
{
	return total_size;
}

bool    ftFileProvider::FileDetails(FileInfo &info)
{
	info.hash = hash;
	info.size = total_size;
	info.path = file_name;
	info.fname = RsDirUtil::getTopDir(file_name);

	/* Use req_loc / req_size to estimate data rate */

	return true;
}


bool ftFileProvider::getFileData(uint64_t offset, uint32_t chunk_size, void *data)
{
        RsStackMutex stack(ftPMutex); /********** STACK LOCKED MTX ******/
	if (fd==NULL) 
	{
		int init = initializeFileAttrs();
		if (init ==0) 
		{
			std::cerr <<"Initialization Failed" << std::endl;
			return 0;
		}
	}
	
	/*
	 * Use private data, which has a pointer to file, total size etc
	 */

	/* 
	 * FIXME: Warning of comparison between unsigned and signed int?
	 */
	int data_size    = chunk_size;
	long base_loc    = offset;
	
	if (base_loc + data_size > total_size)
	{
		data_size = total_size - base_loc;
		std::cerr <<"Chunk Size greater than total file size, adjusting chunk size " << data_size << std::endl;
	}

	if (data_size > 0)
	{		
		/*
                 * seek for base_loc 
                 */
		fseek(fd, base_loc, SEEK_SET);

		void *data = malloc(chunk_size);
		
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
	/* 
         * check if the file exists 
         */
	
	{
		std::cout <<
		 "ftFileProvider::initializeFileAttrs() Filename: " << file_name;        	
	}

	/* 
         * attempt to open file 
         */
	
	fd = fopen(file_name.c_str(), "r+b");
	if (!fd)
	{
		std::cout <<
		    "ftFileProvider::initializeFileAttrs() Failed to open (r+b): "<< file_name << std::endl;
		return 0;

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

	total_size = ftell(fd);
	return 1;
}
