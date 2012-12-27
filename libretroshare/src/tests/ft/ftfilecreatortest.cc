#include "ft/ftfilecreator.h"

#include "util/utest.h"
#include "common/testutils.h"
#include <stdlib.h>

#include "util/rswin.h"
#include "util/rsdir.h"
#include "util/rsdiscspace.h"
#include "pqi/p3cfgmgr.h"
#include "ft/ftserver.h"
#include "turtle/p3turtle.h"

INITTEST();

static int test_timeout(ftFileCreator *creator);
static int test_fill(ftFileCreator *creator);

int main()
{
	RsDiscSpace::setDownloadPath("/tmp") ;
	RsDiscSpace::setPartialsPath("/tmp") ;

	/* use ftcreator to create a file on tmp drive */
	std::string hash = TestUtils::createRandomFileHash() ;
	uint64_t size = 12943090 ;

	ftFileCreator fcreator1("/tmp/rs-ftfc-test.dta",size,hash,true);
	ftFileCreator fcreator2("/tmp/rs-ftfc-test.dta",size,hash,true);

	test_timeout(&fcreator1);
	test_fill(&fcreator2);

	FINALREPORT("RsTlvItem Stack Tests");

	return TESTRESULT();
}


int test_timeout(ftFileCreator *creator)
{
	uint32_t chunk = 1000;
	uint64_t offset = 0;
	int max_timeout = 5;
	int max_offset = chunk * max_timeout;
	int i;
	std::cerr << "60 second test of chunk queue.";
	std::cerr << std::endl;

	uint32_t size_hint = 1000;
	std::string peer_id = "dummyId";
	bool toOld = false;

	for(i = 0; i < max_timeout; i++)
	{
		creator->getMissingChunk(peer_id, size_hint, offset, chunk, toOld);
		std::cerr << "Allocated Offset: " << offset << " chunk: " << chunk << std::endl;

                CHECK(offset <= max_offset);
	//	sleep(1);
	}

	std::cerr << "Expect Repeats now";
	std::cerr << std::endl;

	for(i = 0; i < max_timeout; i++)
	{
	//	sleep(1);
		creator->getMissingChunk(peer_id, size_hint, offset, chunk, toOld);
		std::cerr << "Allocated Offset: " << offset << " chunk: " << chunk << std::endl;

                CHECK(offset <= max_offset);
	}
        REPORT("Chunk Queue");

	return 1;
}


int test_fill(ftFileCreator *creator)
{
	uint32_t chunk = 1000;
	uint64_t offset = 0;

	uint64_t init_size = creator->getFileSize();
	uint64_t init_trans = creator->getRecvd();
	std::cerr << "Initial FileSize: " << init_size << std::endl;
	std::cerr << "Initial Transferred:" << init_trans << std::endl;

	uint32_t size_hint = 1000;
	std::string peer_id = TestUtils::createRandomSSLId();
	bool toOld = false;
	std::cerr << "Allocating data size in memory for " << creator->fileSize() << " bytes." << std::endl;
	unsigned char *total_file_data = new unsigned char[creator->fileSize()] ;

	while(creator->getMissingChunk(peer_id, size_hint, offset, chunk, toOld))
	{
		if (chunk == 0)
		{
			std::cerr << "Missing Data already Alloced... wait";
			std::cerr << std::endl;
			sleep(1);
			chunk = 1000;
			continue;
		}

		/* give it to them */
		void *data = malloc(chunk);
		/* fill with ascending numbers */
		for(int i = 0; i < chunk; i++)
		{
			((uint8_t *) data)[i] = 'a' + i % 27;
			if (i % 27 == 26)
			{
				((uint8_t *) data)[i] = '\n';
			}
		}
		memcpy(total_file_data+offset,data,chunk) ;

		//std::cerr << "  adding file data at offset " << offset << ", size = " << chunk << std::endl;

		creator->addFileData(offset, chunk, data);
		free(data);
//#ifndef WINDOWS_SYS
///********************************** WINDOWS/UNIX SPECIFIC PART ******************/
//		usleep(250000); /* 1/4 of sec */
//#else
///********************************** WINDOWS/UNIX SPECIFIC PART ******************/
//		Sleep(250); /* 1/4 of sec */
//#endif
///********************************** WINDOWS/UNIX SPECIFIC PART ******************/

		chunk = 1000; /* reset chunk size */

	}
	// validate all chunks
	
	std::vector<uint32_t> chunks_to_check ;
	creator->getChunksToCheck(chunks_to_check) ;

	std::cerr << "Validating " << chunks_to_check.size() << " chunks." << std::endl;

	uint32_t CHUNKMAP_SIZE = 1024*1024 ;

	for(uint32_t i=0;i<chunks_to_check.size();++i)
	{
		uint32_t chunk_size = std::min(CHUNKMAP_SIZE, (uint32_t)(creator->fileSize() - chunks_to_check[i]*CHUNKMAP_SIZE)) ;
		Sha1CheckSum crc = RsDirUtil::sha1sum(total_file_data+chunks_to_check[i]*CHUNKMAP_SIZE,chunk_size) ;
		std::cerr << "  Checking crc for chunk " << chunks_to_check[i] << " of size " << chunk_size << ". Reference is " << crc.toStdString() << std::endl;
		creator->verifyChunk(chunks_to_check[i],crc) ;
	}
	delete[] total_file_data ;

	uint64_t end_size = creator->getFileSize();
	uint64_t end_trans = creator->getRecvd();
	std::cerr << "End FileSize: " << end_size << std::endl;
	std::cerr << "End Transferred:" << end_trans << std::endl;
        CHECK(init_size == end_size);
        CHECK(end_trans == end_size);

        REPORT("Test Fill");

	return 1;
}

