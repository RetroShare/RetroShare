#include "ftfileprovider.h"

ftFileProvider::ftFileProvider(std::string path, uint64_t size, std::string
hash) : total_size(size), hash(hash), file_name(path), fd(NULL) {
	//open a file and read it into a binary array!
	
}
ftFileProvider::~ftFileProvider(){
	std::cout << "ftFileProvider d'tor" << std::endl;
	if (fd!=NULL) {
		std::cout <<"fd is not null"<<std::endl;
		fclose(fd);
	}
	else {
		std::cout <<"fd is null"<<std::endl;
	}
}

bool ftFileProvider::getFileData(uint64_t offset, uint32_t chunk_size, void *data)
{
        RsStackMutex stack(ftPMutex); /********** STACK LOCKED MTX ******/
	if (fd==NULL) {
		int init = initializeFileAttrs();
		if (init ==0) {
			std::cerr <<"Initialization Failed" << std::endl;
			return 0;
		}
	}
	//Use private data, which has a pointer to file, total size etc

	int data_size    = chunk_size;
	long base_loc    = offset;
	
	if (base_loc + data_size > total_size)
	{
		data_size = total_size - base_loc;
		std::cerr <<"Chunk Size greater than total file size, adjusting chunk size " << data_size << std::endl;
	}

	if (data_size > 0)
	{
		
		/* seek for base_loc */
		fseek(fd, base_loc, SEEK_SET);

		void *data = malloc(chunk_size);
		/* read the data */
		if (1 != fread(data, data_size, 1, fd))
		{
			std::cerr << "ftFileProvider::getFileData() Failed to get data!";
			free(data);
			return 0;
		}


		/* Update status of ftFileStatus to reflect last usage (for GUI display)
		 * We need to store.
		 * (a) Id, 
		 * (b) Offset, 
		 * (c) Size, 
		 * (d) timestamp
		 */

		time_t now = time(NULL);
		//s->id = id;
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
	/* check if the file exists */
	
	{
		std::cout <<
		 "ftFileProvider::initializeFileAttrs() Filename: " << file_name;        	
	}

	/* attempt to open file */
	
	fd = fopen(file_name.c_str(), "r+b");
	if (!fd)
	{
		std::cout <<
		    "ftFileProvider::initializeFileAttrs() Failed to open (r+b): "
			<< file_name << std::endl;
		return 0;

	}


	/* if it opened, find it's length */
	/* move to the end */
	if (0 != fseek(fd, 0L, SEEK_END))
	{
        	std::cerr << "ftFileProvider::initializeFileAttrs() Seek Failed" << std::endl;
			//s->status = (PQIFILE_FAIL | PQIFILE_FAIL_NOT_SEEK);*/
		return 0;
	}

	/*s->recv_size  = ftell(fd); /* get the file length */
	//s->total_size = s->size; /* save the total length */
	//s->fd = fd;*/
	total_size = ftell(fd);
	std::cout <<"exit init\n";
	return 1;
}
