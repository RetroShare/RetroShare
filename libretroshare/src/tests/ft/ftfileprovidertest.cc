#include "ft/ftfileprovider.h"
#include "ft/ftfilecreator.h"

#include "util/utest.h"
#include "util/rsdir.h"
#include "util/rsdiscspace.h"
#include "common/testutils.h"
#include <stdlib.h>

#include "util/rswin.h"

INITTEST()

int main()
{
	std::string ownId = TestUtils::createRandomSSLId() ;

	RsDiscSpace::setPartialsPath("/tmp") ;
	RsDiscSpace::setDownloadPath("/tmp") ;

	/* create a random file */
	uint64_t size = 10000000;
	uint32_t max_chunk = 10000;
	uint32_t chunk = 1000;
	uint64_t offset = 0;

	std::string filename  = "/tmp/ft_test.dta";
	std::string filename2 = "/tmp/ft_test.dta.dup";

	std::string hash = TestUtils::createRandomFileHash() ;

	/* use creator to make it */

	TestUtils::createRandomFile(filename,size) ;
	
	std::cerr << "Created file: " << filename << " of size: " << size;
	std::cerr << std::endl;

	/* load it with file provider */
	ftFileCreator  *creator  = new ftFileCreator(filename2, size, hash, true);
	ftFileProvider *provider = new ftFileProvider(filename, size, hash);

	/* create duplicate with file creator */
	std::string peer_id = TestUtils::createRandomSSLId() ;
	uint32_t size_hint = 10000;
	bool toOld = false;
	unsigned char *data = new unsigned char[max_chunk];

	while(creator->getMissingChunk(peer_id, size_hint, offset, chunk, toOld))
	{
		if (chunk == 0)
		{
			std::cerr << "All currently allocated .... waiting";
			std::cerr << std::endl;
			sleep(1);
			/* reset chunk size */
			chunk = (uint64_t) max_chunk * (rand() / (1.0 + RAND_MAX));
			std::cerr << "ChunkSize = " << chunk << std::endl;
			continue;
		}

		if (!provider->getFileData(ownId,offset, chunk, data))
		{
			FAILED("Read from Test Data File");
			std::cerr << "Failed to get data";
			std::cerr << std::endl;
		}

		if (!creator->addFileData(offset, chunk, data))
		{
			FAILED("Write to Duplicate");
			std::cerr << "Failed to add data";
			std::cerr << std::endl;
		}

		std::cerr << "Transferred: " << chunk << " @ " << offset;
		std::cerr << std::endl;


		/* reset chunk size */
		chunk = (uint64_t) max_chunk * (rand() / (1.0 + RAND_MAX));

		std::cerr << "ChunkSize = " << chunk << std::endl;
	}
	std::string path,hash2 ;

	RsDirUtil::hashFile(filename ,path,hash,size) ;
	RsDirUtil::hashFile(filename2,path,hash2,size) ;

	std::cerr << "Checking hash1 : " << hash << std::endl;
	std::cerr << "Checking hash2 : " << hash2 << std::endl;

	CHECK(hash == hash2) ;

	FINALREPORT("ftfilecreatortest") ;

	return TESTRESULT();
}

