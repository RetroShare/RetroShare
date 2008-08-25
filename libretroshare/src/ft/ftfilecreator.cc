#include "ftfilecreator.h"

#define FILE_DEBUG 1

/***********************************************************
*
*	ftFileCreator methods
*
***********************************************************/

ftFileCreator::ftFileCreator(std::string path, uint64_t size, std::string
hash, std::string chunker): ftFileProvider(path,size,hash) 
{
	/* 
         * FIXME any inits to do?
         */
	initialize(chunker);
}

void ftFileCreator::initialize(std::string chunker) 
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	
	if (chunker == "default")
		fileChunker = new ftFileChunker(total_size);
	else 
		fileChunker = new ftFileRandomizeChunker(total_size);
		
	fileChunker->splitFile();
}

int ftFileCreator::initializeFileAttrs()
{
	//RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	
	/* 
	 * check if the file exists 
	 */
	std::cout << "ftFileProvider::initializeFileAttrs() Filename: " << file_name;
	
	/* 
         * attempt to open file in writing mode
         */
	
	fd = fopen(file_name.c_str(), "w+b");
	if (!fd)
	{
		std::cout << "ftFileProvider::initializeFileAttrs() Failed to open (w+b): " << file_name << std::endl;
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
	/* 
	 * get the file length 
	 */
	recv_size  = ftell(fd); 	
	return 1;
}

bool ftFileCreator::addFileData(uint64_t offset, uint32_t chunk_size, void *data)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	/* check the status */

	if (fd==NULL) 
	{
		int init = initializeFileAttrs();
		if (init ==0) {
			std::cerr <<"Initialization Failed" << std::endl;
			return 0;
		}
	}

	/* 
	 * check its at the correct location 
	 */
	if (offset + chunk_size > this->total_size)
	{
		chunk_size = total_size - offset;
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
	std::cerr << pos << " BEFORE RECV SIZE "<< recv_size << std::endl;

	/* 
	 * add the data 
 	 */
	if (1 != fwrite(data, chunk_size, 1, this->fd))
	{
        	std::cerr << "ftFileCreator::addFileData() Bad fwrite" << std::endl;
		return 0;
	}

	this->recv_size += chunk_size;

	pos = ftell(fd);
	
	/* 
	 * Notify ftFileChunker about chunks received 
	 */
	fileChunker->notifyReceived(offset,chunk_size);

	/* 
	 * FIXME HANDLE COMPLETION HERE - Any better way?
	 */

	return 1;
}

ftFileCreator::~ftFileCreator()
{
	/*
	 * FIXME Any cleanups specific to filecreator?
	 */
}


bool ftFileCreator::getMissingChunk(uint64_t &offset, uint32_t &chunk) 
{
#ifdef FILE_DEBUG
	std::cerr << "ftFileCreator::getMissingChunk(???," << chunk << ")";
	std::cerr << std::endl;
#endif
	return fileChunker->getMissingChunk(offset, chunk);
}

/***********************************************************
*
*	ftFileChunker methods
*
***********************************************************/

ftFileChunker::ftFileChunker(uint64_t size): file_size(size),  std_chunk_size(10000), monitorPeriod(30) 
{
	/*
	 * srand for randomized version - move it to the sub-class?
	 */
	srand ( time(NULL) );
	aggregate_status = 0;
}

ftFileChunker::~ftFileChunker()
{
	std::vector<ftChunk>::iterator it;
	for(unsigned int i=0; i<allocationTable.size();i++) {
		delete allocationTable.at(i); /* Does this need a check? */		
	}
	if(!allocationTable.empty()){
		allocationTable.clear();
	}
}


int ftFileChunker::splitFile(){
	RsStackMutex stack(chunkerMutex); /********** STACK LOCKED MTX ******/

	/* 
	 * all in bytes 
	 */
	num_chunks = file_size/std_chunk_size; 
	/*
	 * FIXME - Remainder should go into last chunk
	 */
	uint64_t rem = file_size % std_chunk_size;
	unsigned int index=0;
	uint64_t max_chunk_size = file_size - (index * std_chunk_size);


#ifdef FILE_DEBUG
	std::cerr << "ftFileChunker::splitFile()";
	std::cerr << std::endl;
	std::cerr << "\tnum_chunks: " << num_chunks;
	std::cerr << std::endl;
	std::cerr << "\trem: " << rem;
	std::cerr << std::endl;
#endif
	
	time_t now = time(NULL);
	for(index=0;index<num_chunks;index++)
	{
  		uint64_t offset = index * std_chunk_size;
		max_chunk_size = file_size - (index * std_chunk_size);
		ftChunk *f = new ftChunk(offset,max_chunk_size,now, ftChunk::AVAIL);
		allocationTable.push_back(f);
	}

	if (rem != 0)
	{
		ftChunk *f = new ftChunk(file_size-rem,rem,now, ftChunk::AVAIL);
		allocationTable.push_back(f);
		num_chunks++;
	}

	/*
	 * DEBUGGER
	 * for(int j=0;j<allocationTable.size();j++)
	 * {
 	 *	std::cout << "SIZE " << allocationTable.at(j)->max_chunk_size << " " << allocationTable.at(j)->chunk_status << std::endl;
	 * }
	 */	
	return 1;
}

/*
 * 	This method sets the offset, chunk may be reset if needed
 */

bool ftFileChunker::getMissingChunk(uint64_t &offset, uint32_t &chunk) 
{	
	RsStackMutex stack(chunkerMutex); /********** STACK LOCKED MTX ******/

	unsigned int i =0;
	bool found = false;
	int chunks_after = 0;
	int chunks_rem = 0;

#ifdef FILE_DEBUG
	std::cerr << "ftFileChunker::getMissingChunk(???," << chunk << ")";
	std::cerr << std::endl;
#endif

	/*
	 * This signals file completion 
         * FIXME Does it need to be more explicit
	 */
 
	if(aggregate_status == num_chunks * ftChunk::RECEIVED)
	{
#ifdef FILE_DEBUG
		std::cerr << "completed ??";
		std::cerr << std::endl;
#endif
		return found;
	}


	while(i<allocationTable.size()) 
	{

#ifdef FILE_DEBUG
		std::cerr << "ftFileChunker checking allocTable(" << i << ")";
		std::cerr << " of " << allocationTable.size();
		std::cerr << std::endl;
#endif
		
		if(allocationTable.at(i)->max_chunk_size >=chunk)
		{

#ifdef FILE_DEBUG
			std::cerr << "ftFileChunker pre alloc(" << i << ")";
			std::cerr << std::endl;
			std::cerr << "\toffset: " << allocationTable.at(i)->offset;
			std::cerr << std::endl;
			std::cerr << "\tmax_chunk: " << allocationTable.at(i)->max_chunk_size;
			std::cerr << std::endl;
			std::cerr << "\treq_chunk: " << chunk;
			std::cerr << std::endl;
#endif


			offset = allocationTable.at(i)->offset;
			chunks_after = chunk/std_chunk_size; //10KB

			/*
			 * FIXME Handling remaining chunk < 10KB
			 */

			//if (chunk <
			chunks_rem   = chunk % std_chunk_size;
			chunk -= chunks_rem;
			/*std::cout << "Found " << chunk <<  " at " << i <<  " "<< chunks_after << std::endl;*/
			allocationTable.at(i)->max_chunk_size=0;
			allocationTable.at(i)->timestamp = time(NULL);
			allocationTable.at(i)->chunk_status = ftChunk::ALLOCATED;


#ifdef FILE_DEBUG
			std::cerr << "ftFileChunker postalloc(" << i << ")";
			std::cerr << std::endl;
			std::cerr << "\tchunks_after: " << chunks_after;
			std::cerr << std::endl;
			std::cerr << "\tchunks_rem: " << chunks_after;
			std::cerr << std::endl;
			std::cerr << "\tchunk: " << chunks_after;
			std::cerr << std::endl;
#endif

			found = true;
			break;
		}
		i++;
	}

	/* if we get here, there is no available chunk bigger 
	 * than requested ... 
	 * NB: Request size should be a larger than std_chunk_size.
	 * So Edge (sub chunk allocation) condition is handled here.
	 *
	 * Find largest remaining chunk.
	 */

	if (!found) 
	{
		i=0;
		uint64_t max = allocationTable.at(i)->max_chunk_size;
		uint64_t size = max;
		int maxi = -1;
		while(i<allocationTable.size()) 
		{
			size = allocationTable.at(i)->max_chunk_size;
			if(size > max)
			{
				max = allocationTable.at(i)->max_chunk_size;
				maxi = i;
			}
			i++;
		}
		if (maxi > -1) //maxi or max
		{
			offset = allocationTable.at(maxi)->offset;
			chunk  = allocationTable.at(maxi)->max_chunk_size;	
			chunks_after = chunk/std_chunk_size; //10KB

			/* Handle end condition ...
			 * max_chunk_size < std_chunk_size
			 * Trim if not end condition.
			 */
			if (chunks_after > 0)
			{
				chunks_rem   = chunk % std_chunk_size;
				chunk -= chunks_rem;
			}
			else
			{
				/* end condition */
				chunks_after = 1;
			}

			allocationTable.at(maxi)->max_chunk_size=0;
			allocationTable.at(maxi)->timestamp = time(NULL);
			allocationTable.at(maxi)->chunk_status = ftChunk::ALLOCATED;
			found = true;		
		}
		
	} //if not found 


	if (found)
	{
		std::cout << "Chunks remaining " << chunks_rem << std::endl;
		/*
		 * update all previous chunks max available size
		 * Expensive? Can it be smarter FIXME
		 */

		/* drbob: Think this is wrong?
		 * disabling...
		 *
		for(unsigned int j=0;j<i;j++)
		{
			if (allocationTable.at(j)->max_chunk_size >0)
				allocationTable.at(j)->max_chunk_size -= chunk;
		}
		 *
		 */

	
		for(unsigned int j=i;j<i+chunks_after;j++)
		{
			allocationTable.at(j)->max_chunk_size = 0;
			allocationTable.at(j)->chunk_status = ftChunk::ALLOCATED;
		}
	
		// DEBUGGER - Uncomment 
		for(unsigned int j=0;j<allocationTable.size();j++)
		{
 			std::cout << "After allocation " << allocationTable.at(j)->max_chunk_size << " " << allocationTable.at(j)->chunk_status << std::endl;
		}
		
	}
	return found;
}

/* 
 * This should run on a separate thread when ftFileChunker is initialized
 * FIXME Implemet DrBob's suggestion of request-fired check instead of dedicated
 * thread
 */
int ftFileChunker::monitor() 
{
	int reset = 0;
	uint32_t prev_size = 0;
	uint32_t size = 0;

	std::cout<<"Running monitor.."<<std::endl;

	RsStackMutex stack(chunkerMutex); /********** STACK LOCKED MTX ******/
	for(unsigned int j=allocationTable.size()-1 ;j>= 0;)
	{
 		if((allocationTable.at(j)->chunk_status == ftChunk::ALLOCATED) && 
		   (allocationTable.at(j)->timestamp - time(NULL) > 30))
		{
			allocationTable.at(j)->chunk_status = ftChunk::AVAIL;
			if (j == allocationTable.size()-1)
			{
				/* at end */
				prev_size = 0;
				size = file_size % std_chunk_size;
				if (size == 0)
				{
					size = std_chunk_size;
				}
			}
			else
			{
				prev_size = allocationTable.at(j+1)->max_chunk_size;
				size = std_chunk_size;
			}

			allocationTable.at(j)->max_chunk_size = size + prev_size;
			prev_size = allocationTable.at(j)->max_chunk_size;
			
			for(j--; j >= 0; j--)
			{
				if (allocationTable.at(j)->chunk_status != ftChunk::AVAIL)
					break;
			
				allocationTable.at(j)->max_chunk_size += prev_size;
				prev_size = allocationTable.at(j)->max_chunk_size;
				reset++;
			}
		}
		else
		{
			j--;
		}
	}
	return reset;
}

void ftFileChunker::setMonitorPeriod(int period) 
{
	monitorPeriod = period;
}

void ftFileChunker::run()
{
	
	while(1)
	{

		for(int i = 0; i < monitorPeriod; i++)
		{

/******************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
			sleep(1);
#else

                	Sleep(1000);
#endif
/******************* WINDOWS/UNIX SPECIFIC PART ******************/
		}
		monitor();
	}

}

int ftFileChunker::notifyReceived(uint64_t offset, uint32_t chunk_size) 
{
	RsStackMutex stack(chunkerMutex); /********** STACK LOCKED MTX ******/
	int index = offset / std_chunk_size;
	
	if(allocationTable.at(index)->chunk_status == ftChunk::ALLOCATED){
		allocationTable.at(index)->chunk_status = ftChunk::RECEIVED;
		aggregate_status += ftChunk::RECEIVED;
	}
	return aggregate_status;
}

/***********************************************************
*
*	ftFileRandomizeChunker methods
*
***********************************************************/

ftFileRandomizeChunker::ftFileRandomizeChunker(uint64_t size):
ftFileChunker(size) 
{

}

ftFileRandomizeChunker::~ftFileRandomizeChunker()
{

}

bool ftFileRandomizeChunker::getMissingChunk(uint64_t &offset, uint32_t &chunk) {
	
	RsStackMutex stack(chunkerMutex); /********** STACK LOCKED MTX ******/
	std::cerr << "Calling getMissingChunk with chunk="<< chunk << std::endl;
	unsigned int i =0;
	bool found = false;
	int chunks_after = 0;
	int chunks_rem = 0;

	if(aggregate_status == num_chunks * ftChunk::RECEIVED)
		return found;

	std::vector<int> randomIndex;
	while(i<allocationTable.size()) 
	{
		if(allocationTable.at(i)->max_chunk_size >=chunk){
			randomIndex.push_back(i);			
		}
		i++;
	}
	
	/* 
	 * FIXME test sufficiently to make sure its picking every index
	 */
	if (randomIndex.size()>0) 
	{
		int rnum = rand() % randomIndex.size();
		i = randomIndex.at(rnum);
		std::cout << "i=" <<i << " rnum " << rnum << std::endl;
		offset = allocationTable.at(i)->offset;
		chunks_after = chunk/std_chunk_size; //10KB
		chunks_rem   = chunk % std_chunk_size;
		chunk -= chunks_rem;
		std::cout << "Found " << chunk <<  " at index =" << i <<  " "<< chunks_after << std::endl;
		allocationTable.at(i)->max_chunk_size=0;
		allocationTable.at(i)->timestamp = time(NULL);
		allocationTable.at(i)->chunk_status = ftChunk::ALLOCATED;
		found = true;
	}

	if (!found) 
	{
		i=0;
		uint64_t min = allocationTable.at(i)->max_chunk_size - chunk;
		uint64_t diff = min;
		int mini = -1;
		while(i<allocationTable.size()) 
		{
			diff = allocationTable.at(i)->max_chunk_size-chunk;
			if(diff <= min && diff >0)
			{
				min = allocationTable.at(i)->max_chunk_size - chunk;
				mini = i;
			}
			i++;
		}
		if (mini > -1) 
		{
			offset = allocationTable.at(mini)->offset;
			chunk  = allocationTable.at(mini)->max_chunk_size;	
			chunks_after = chunk/std_chunk_size; //10KB
			chunks_rem   = chunk % std_chunk_size;
			chunk -= chunks_rem;
			allocationTable.at(mini)->max_chunk_size=0;
			allocationTable.at(mini)->timestamp = time(NULL);
			allocationTable.at(mini)->chunk_status = ftChunk::ALLOCATED;
			found = true;		
		}
		
	} //if not found 

	if (found) 
	{
		// update all previous chunks max available size
		for(unsigned int j=0;j<i;j++)
		{
			if (allocationTable.at(j)->max_chunk_size >0)
			allocationTable.at(j)->max_chunk_size -= chunk;
		}
	
		for(unsigned int j=i;j<i+chunks_after;j++)
		{
			allocationTable.at(j)->max_chunk_size = 0;
			allocationTable.at(j)->chunk_status = ftChunk::ALLOCATED;
		}
	
		/*	for(int j=0;j<allocationTable.size();j++){
 			std::cout << "After allocation " << j << " " << allocationTable.at(j)->max_chunk_size << " " << allocationTable.at(j)->chunk_status << std::endl;
		}*/
	}
	return found;
}
/***********************************************************
*
*	ftChunk methods
*
***********************************************************/

ftChunk::ftChunk(uint64_t offset,uint64_t chunk_size,time_t time, ftChunk::Status s) : offset(offset), max_chunk_size(chunk_size), timestamp(time), chunk_status(s)
{

}

ftChunk::~ftChunk()
{

}
