#include "ftfilecreator.h"


ftFileCreator::ftFileCreator(std::string path, uint64_t size, std::string
hash, std::string chunker="default"): ftFileProvider(path,size,hash) {
	/* any inits to do?*/
	initialize(chunker);
}

void ftFileCreator::initialize(std::string chunker) {
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	if (chunker == "default")
		fileChunker = new ftFileChunker(total_size);
	else 
		fileChunker = new ftFileRandomizeChunker(total_size);
		
	fileChunker->splitFile();
}

int ftFileCreator::initializeFileAttrs() //not override?
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	/* check if the file exists */
	
	{
		std::cout <<
		 "ftFileProvider::initializeFileAttrs() Filename: " << file_name;        	
	}

	/* attempt to open file in writing mode*/
	
	fd = fopen(file_name.c_str(), "w+b");
	if (!fd)
	{
		std::cout <<
		    "ftFileProvider::initializeFileAttrs() Failed to open (w+b): "
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
	recv_size  = ftell(fd); /* get the file length */
	/*total_size is unchanged its set at construction*/
	
	
	return 1;
}

bool ftFileCreator::addFileData(uint64_t offset, uint32_t chunk_size, void *data)
{
	RsStackMutex stack(ftcMutex); /********** STACK LOCKED MTX ******/
	/* check the status */

	if (fd==NULL) {
		int init = initializeFileAttrs();
		if (init ==0) {
			std::cerr <<"Initialization Failed" << std::endl;
			return 0;
		}
	}
	/* check its at the correct location */
	if (offset + chunk_size > this->total_size)
	{
		chunk_size = total_size - offset;
		std::cerr <<"Chunk Size greater than total file size, adjusting chunk "
		<< "size " << chunk_size << std::endl;

	}

	/* go to the offset of the file */
	if (0 != fseek(this->fd, offset, SEEK_SET))
	{
		std::cerr << "ftFileCreator::addFileData() Bad fseek" << std::endl;
		return 0;
	}
	
	long int pos;
	pos = ftell(fd);
	std::cerr << pos << " BEFORE RECV SIZE "<< recv_size << std::endl;

	/* add the data */
	if (1 != fwrite(data, chunk_size, 1, this->fd))
	{
        std::cerr << "ftFileCreator::addFileData() Bad fwrite" << std::endl;
		return 0;
	}

	this->recv_size += chunk_size;

	pos = ftell(fd);
	std::cerr << pos << " RECV SIZE "<< recv_size << std::endl;
	//Notify ftFileChunker that all are received
	fileChunker->notifyReceived(offset,chunk_size);

	/* if we've completed the request this time */
	/*if (s->req_loc + s->req_size == s->recv_size)
	{
		s->lastDelta = time(NULL) - s->lastTS;
	}

	if (s->recv_size == s->total_size)
	{
        	pqioutput(PQL_DEBUG_BASIC, ftfilerzone,
	       		       "ftfiler::addFileData() File Complete!");
		s->status = PQIFILE_COMPLETE;

		///* HANDLE COMPLETION HERE 
		//completeFileTransfer(s); Notify ftFileChunker that all are received
	}*/

	return 1;
}





ftFileCreator::~ftFileCreator(){
}


bool ftFileCreator::getMissingChunk(uint64_t &offset, uint32_t &chunk) {
	fileChunker->getMissingChunk(offset, chunk);
}

/***********************************************************
*
*	FtFileChunker methods
*
***********************************************************/

ftFileChunker::ftFileChunker(uint64_t size): file_size(size),  std_chunk_size(10000), monitorPeriod(30) {
	/* any inits to do?*/
 	std::cout << "Constructor ftFileChunker\n";
	srand ( time(NULL) );
	aggregate_status = 0;
}

ftFileChunker::~ftFileChunker(){
	std::cout << "Destructor of ftFileChunker\n";
	std::vector<ftChunk>::iterator it;
	for(int i=0; i<allocationTable.size();i++) {
		delete allocationTable.at(i); /* Does this need a check? */		
	}
	if(!allocationTable.empty()){
		std::cerr << "Empty table\n";
		allocationTable.clear();
	}
}


int ftFileChunker::splitFile(){
	RsStackMutex stack(chunkerMutex); /********** STACK LOCKED MTX ******/
	num_chunks = file_size/std_chunk_size; /* all in bytes */
	uint64_t rem = file_size % std_chunk_size;
				   
	int index=0;
	std::cout << "splitFile\n";
	
	uint64_t max_chunk_size = file_size - (index * std_chunk_size);
	for(index=0;index<num_chunks;index++){
		std::cout << "INDEX " << index << std::endl;
  		uint64_t offset = index * std_chunk_size;
		time_t now = time(NULL);
		max_chunk_size = file_size - (index * std_chunk_size);
		ftChunk *f = new ftChunk(offset,max_chunk_size,now, ftChunk::AVAIL);
		allocationTable.push_back(f);
	}

	for(int j=0;j<allocationTable.size();j++){
 		std::cout << "SIZE " << allocationTable.at(j)->max_chunk_size << " " << allocationTable.at(j)->chunk_status << std::endl;
	}	
	return 1;
}


bool ftFileChunker::getMissingChunk(uint64_t &offset, uint32_t &chunk) {
	//This method sets the offset, chunk may be reset if needed
	RsStackMutex stack(chunkerMutex); /********** STACK LOCKED MTX ******/
	std::cerr << "Calling getMissingChunk with chunk="<< chunk << std::endl;
	int i =0;
	bool found = false;
	int chunks_after = 0;
	int chunks_rem = 0;

	if(aggregate_status == num_chunks * ftChunk::RECEIVED)
		return found;


	while(i<allocationTable.size()) {
		if(allocationTable.at(i)->max_chunk_size >=chunk){
			offset = allocationTable.at(i)->offset;
			chunks_after = chunk/std_chunk_size; //10KB
			chunks_rem   = chunk % std_chunk_size;
			chunk -= chunks_rem;
			std::cout << "Found " << chunk <<  " at " << i <<  " "<< chunks_after << std::endl;
			allocationTable.at(i)->max_chunk_size=0;
			allocationTable.at(i)->timestamp = time(NULL);
			allocationTable.at(i)->chunk_status = ftChunk::ALLOCATED;
			found = true;
			break;
		}
		i++;
	}

	if (!found) {
		i=0;
		uint64_t min = allocationTable.at(i)->max_chunk_size - chunk;
		uint64_t diff = min;
		int mini = -1;
		while(i<allocationTable.size()) {			
			diff = allocationTable.at(i)->max_chunk_size-chunk;
			if(diff <= min && diff >0){
				min = allocationTable.at(i)->max_chunk_size - chunk;
				mini = i;
			}
			i++;
		}
		if (min > -1) {
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

	if (found) {
	std::cout << "Chunks remaining " << chunks_rem << std::endl;
	// update all previous chunks max available size
	for(int j=0;j<i;j++){
		if (allocationTable.at(j)->max_chunk_size >0)
		allocationTable.at(j)->max_chunk_size -= chunk;
	}
	
	for(int j=i;j<i+chunks_after;j++){
		allocationTable.at(j)->max_chunk_size = 0;
		allocationTable.at(j)->chunk_status = ftChunk::ALLOCATED;
			
	}
	
	for(int j=0;j<allocationTable.size();j++){
 		std::cout << "After allocation " << allocationTable.at(j)->max_chunk_size << " " << allocationTable.at(j)->chunk_status << std::endl;
	}
	}
	return found;
}

/* This should run on a separate thread when ftFileChunker is initialized*/
int ftFileChunker::monitor() {
	int reset = 0;
	std::cout<<"Running monitor.."<<std::endl;
	for(int j=0;j<allocationTable.size();j++){
 		if(allocationTable.at(j)->chunk_status == ftChunk::ALLOCATED && allocationTable.at(j)->timestamp - time(NULL) > 30){
			allocationTable.at(j)->chunk_status = ftChunk::AVAIL;
				reset++;
		}
	}
	return reset;
}

void ftFileChunker::setMonitorPeriod(int period) {
	monitorPeriod = period;
}

void ftFileChunker::run(){
	
	while(1)
	{

		for(int i = 0; i < monitorPeriod; i++)
		{

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
			sleep(1);
#else

                	Sleep(1000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
		}
		monitor();
	}

}


ftFileRandomizeChunker::ftFileRandomizeChunker(uint64_t size):
ftFileChunker(size) {

}

ftFileRandomizeChunker::~ftFileRandomizeChunker(){
	std::cout << "Destructor of ftFileRandomizeChunker\n";
}



bool ftFileRandomizeChunker::getMissingChunk(uint64_t &offset, uint32_t &chunk) {
	//This method sets the offset, chunk may be reset if needed
	RsStackMutex stack(chunkerMutex); /********** STACK LOCKED MTX ******/
	std::cerr << "Calling getMissingChunk with chunk="<< chunk << std::endl;
	int i =0;
	bool found = false;
	int chunks_after = 0;
	int chunks_rem = 0;

	if(aggregate_status == num_chunks * ftChunk::RECEIVED)
		return found;

	std::vector<int> randomIndex;
	while(i<allocationTable.size()) {
		if(allocationTable.at(i)->max_chunk_size >=chunk){
			randomIndex.push_back(i);			
		}
		i++;
	}
	
	/* test to make sure its picking every index*/
	if (randomIndex.size()>0) {
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

	if (!found) {
		i=0;
		uint64_t min = allocationTable.at(i)->max_chunk_size - chunk;
		uint64_t diff = min;
		int mini = -1;
		while(i<allocationTable.size()) {			
			diff = allocationTable.at(i)->max_chunk_size-chunk;
			if(diff <= min && diff >0){
				min = allocationTable.at(i)->max_chunk_size - chunk;
				mini = i;
			}
			i++;
		}
		if (min > -1) {
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

	if (found) {
	std::cout << "Chunks remaining " << chunks_rem << std::endl;
	// update all previous chunks max available size
	for(int j=0;j<i;j++){
		if (allocationTable.at(j)->max_chunk_size >0)
		allocationTable.at(j)->max_chunk_size -= chunk;
	}
	
	for(int j=i;j<i+chunks_after;j++){
		allocationTable.at(j)->max_chunk_size = 0;
		allocationTable.at(j)->chunk_status = ftChunk::ALLOCATED;
			
	}
	
/*	for(int j=0;j<allocationTable.size();j++){
 		std::cout << "After allocation " << j << " " << allocationTable.at(j)->max_chunk_size << " " << allocationTable.at(j)->chunk_status << std::endl;
	}*/
	}
	return found;
}



int ftFileChunker::notifyReceived(uint64_t offset, uint32_t chunk_size) {
	RsStackMutex stack(chunkerMutex); /********** STACK LOCKED MTX ******/
	int index = offset / std_chunk_size;
	std::cout << "INDEX " << index << std::endl;
	if(allocationTable.at(index)->chunk_status == ftChunk::ALLOCATED){
		allocationTable.at(index)->chunk_status = ftChunk::RECEIVED;
		aggregate_status += ftChunk::RECEIVED;
	}
}

ftChunk::ftChunk(uint64_t offset,uint64_t chunk_size,time_t time, ftChunk::Status s) : offset(offset), max_chunk_size(chunk_size), timestamp(time), chunk_status(s){

}
