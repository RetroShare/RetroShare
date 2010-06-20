#include "ft/ftfileprovider.h"
#include "ft/ftfilecreator.h"

#include "util/utest.h"
#include <stdlib.h>

#include "util/rswin.h"

INITTEST()

int main()
{

	/* create a random file */
	uint64_t size = 100000;
	uint32_t max_chunk = 10000;
	uint32_t chunk = 1000;
	uint64_t offset = 0;

	std::string filename  = "/tmp/ft_test.dta";
	std::string filename2 = "/tmp/ft_test.dta.dup";

	/* use creator to make it */

	void *data = malloc(max_chunk);
	for(int i = 0; i < max_chunk; i++)
	{ 
		((uint8_t *) data)[i] = 'a' + i % 27;
		if (i % 27 == 26)
		{
			((uint8_t *) data)[i] = '\n';
		}
	}
	
	ftFileCreator *creator = new ftFileCreator(filename, size, "hash");
	for(offset = 0; offset != size; offset += chunk)
	{
		if (!creator->addFileData(offset, chunk, data))
		{
			FAILED("Create Test Data File");
			std::cerr << "Failed to add data (CREATE)";
			std::cerr << std::endl;
		}
	}
	delete creator;
	
	std::cerr << "Created file: " << filename << " of size: " << size;
	std::cerr << std::endl;

	/* load it with file provider */
	creator = new ftFileCreator(filename2, size, "hash");
	ftFileProvider *provider = new ftFileProvider(filename, size, "hash");

	/* create duplicate with file creator */
	std::string peer_id = "dummyId";
	uint32_t size_hint = 10000;
	bool toOld = false;

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

		if (!provider->getFileData(offset, chunk, data))
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
	return 1;
}
