#include "ft/ftfilemapper.h"
#include "ft/ftchunkmap.h"
#include "retroshare/rstypes.h"

#include <util/utest.h>
#include <util/rsdir.h>
#include <stdlib.h>
#include <iostream>
#include <stdint.h>

INITTEST();

int main()
{
	/* Use ftfilemapper to create a file with chunks downloaded on a random direction. */

	static const std::string tmpdir = "." ;
	static const std::string input_file = tmpdir+"/"+"input.bin" ;
	static const std::string output_file = tmpdir+"/"+"output.bin" ;
	static const uint64_t size = 1024*1024*12+234;//4357283      ; // some size. Not an integer number of chunks
	static const uint32_t chunk_size = 1024*1024 ; // 1MB

	pid_t pid = getpid() ;
	srand48(pid) ;
	srand(pid) ;
	std::cerr << "Inited random number generator with seed " << pid << std::endl;

	// 0 - create a random file in memory, of size SIZE 

	void *membuf = malloc(size) ;
	CHECK(membuf != NULL) ;

	for(int i=0;i<size;++i)
		((char*)membuf)[i] = lrand48() & 0xff ;

	// also write it to disk
	
	FILE *f = fopen(input_file.c_str(),"w") ;
	CHECK(f != NULL) ;
	CHECK(fwrite(membuf,size,1,f) == 1) ;
	fclose(f) ;
		
	// 1 - allocate a chunkmap for this file
	//
	ChunkMap chunk_map(size,true) ;
	chunk_map.setStrategy(FileChunksInfo::CHUNK_STRATEGY_RANDOM) ;
	ftFileMapper fmapper(size,chunk_size);

	// Get the chunks one by one
	//
	FILE *fout = fopen(output_file.c_str(),"w+") ;

	CHECK(fout != NULL) ;

	ftChunk chunk ;
	bool source_map_needed ;

	while(chunk_map.getDataChunk("virtual peer",1024*200+(lrand48()%1024),chunk,source_map_needed))
	{
		//std::cerr << "Got chunk " << chunk.offset << " + " << chunk.size << " from chunkmap." << std::endl;

		CHECK(fmapper.storeData( (unsigned char *)membuf+chunk.offset,chunk.size,chunk.offset,fout) ) ;
		chunk_map.dataReceived(chunk.id) ;

		fmapper.print() ;

		delete chunk.ref_cnt ;
	}

	fclose(fout) ;

	// Check the sha1 of both source and destination.
	//
	std::string sha1_1,sha1_2 ;
	uint64_t size_1,size_2 ;

	RsDirUtil::getFileHash( input_file,sha1_1,size_1) ;
	RsDirUtil::getFileHash(output_file,sha1_2,size_2) ;

	std::cerr << "Computed hash of file\t " << input_file << "\t :\t" << sha1_1 << ", size=" << size_1 << std::endl;
	std::cerr << "Computed hash of file\t " <<output_file << "\t :\t" << sha1_2 << ", size=" << size_2 << std::endl;

	CHECK(size_1 == size_2) ;
	CHECK(sha1_1 == sha1_2) ;

	FINALREPORT("RsTlvItem Stack Tests");

	return TESTRESULT();
}


