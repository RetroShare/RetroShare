#include "ftfilecreator.h"

main(){
	/*Testing default file chunker*/
	uint32_t total_size = 1000000; /*100KB*/
	ftFileChunker fc(total_size);
	fc.setMonitorPeriod(10);
	fc.splitFile();
	std::cout << "Starting fc monitor thread\n";
	fc.start();
	
	
	/*Simulate calls from transfer module*/
	uint64_t offset;
	uint32_t csize = 40000;      /* 40KB*/
	if (fc.getMissingChunk(offset, csize)){
		std::cout << "Missing Chunk's offset=" << offset << " chunk_size=" << csize << std::endl;
	}
	sleep(5);
	csize = 10000;
	if (fc.getMissingChunk(offset, csize)){
		std::cout << "Missing Chunk's offset=" << offset << " chunk_size=" << csize << std::endl;
	}
	
	csize = 15000;             /* Ask more than the multiple of std_chunk_size*/
	if (fc.getMissingChunk(offset, csize)){
		std::cout << "Missing Chunk's offset=" << offset << " chunk_size=" << csize << std::endl;
	}
	
	sleep(10);
	/*Test file creator*/
	ftFileCreator fcreator("somefile",100000,"hash","default");
	csize = 40000;
	if (fcreator.getMissingChunk(offset, csize)){
		std::cout << "Missing Chunk's offset=" << offset << " chunk_size=" << csize << std::endl;
	}
	
	csize = 10000;
	if (fcreator.getMissingChunk(offset, csize)){
		std::cout << "Missing Chunk's offset=" << offset << " chunk_size=" << csize << std::endl;
	}
	
	csize = 15000;
	if (fcreator.getMissingChunk(offset, csize)){
		std::cout << "Missing Chunk's offset=" << offset << " chunk_size=" << csize << std::endl;
	}
	
	/* Test file creator adding data to file out-of-order*/
	char* alpha = "abcdefghij";
	std::cerr << "Call to addFileData =" << fcreator.addFileData(10,10,alpha );
	char* numerical = "1234567890";
	std::cerr << "Call to addFileData =" << fcreator.addFileData(0,10,numerical );
	

	/* Test file creator can write out of order/randomized chunker */
	ftFileCreator franc("somefile1",50000,"hash", "randomize");
	csize = 30000;
	if (franc.getMissingChunk(offset, csize)){
		std::cout << "Offset " << offset << " Csize " << csize << std::endl;
	}

	char* allA = (char*) malloc(csize);
	for(int i=0;i<csize;i++)
		allA[i]='a';
	
	std::cerr << "ADD DATA RETUNED " << franc.addFileData(offset,csize,allA );

	csize = 10000;
	if (franc.getMissingChunk(offset, csize)){
		std::cout << "Offset " << offset << " Csize " << csize << std::endl;
	}
	
	char* allB = (char*) malloc(csize);
	for(int i=0;i<csize;i++)
		allB[i]='b';



	std::cerr << "ADD DATA RETUNED " << franc.addFileData(offset,csize,allB );
	
	if (franc.getMissingChunk(offset, csize)){
		std::cout << "Offset " << offset << " Csize " << csize << std::endl;
	}
	else {
		std::cout << "getMissing chunk returned nothing" << std::endl;	
	}
	
	
	csize = 10000;
	if (franc.getMissingChunk(offset, csize)){
		std::cout << "Offset " << offset << " Csize " << csize << std::endl;
	}
	
	char* allC = (char*) malloc(csize);
	for(int i=0;i<csize;i++)
		allC[i]='c';



	std::cerr << "ADD DATA RETUNED " << franc.addFileData(offset,csize,allC );
	
	if (franc.getMissingChunk(offset, csize)){
		std::cout << "Offset " << offset << " Csize " << csize << std::endl;
	}
	else {
		std::cout << "getMissing chunk returned nothing" << std::endl;	
	}

	fc.join();
	free(allA);
	free(allB);
}
