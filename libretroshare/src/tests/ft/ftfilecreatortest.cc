#include "ft/ftfilecreator.h"

#include "util/utest.h"
#include <stdlib.h>

#include "util/rswin.h"
#include "pqi/p3cfgmgr.h"
#include "ft/ftserver.h"
#include "turtle/p3turtle.h"

INITTEST();

static int test_timeout(ftFileCreator *creator);
static int test_fill(ftFileCreator *creator);

int main()
{
	/* use ftcreator to create a file on tmp drive */
	ftFileCreator fcreator("/tmp/rs-ftfc-test.dta",100000,"hash", true);

	test_timeout(&fcreator);
	test_fill(&fcreator);

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
	std::string peer_id = "dummyId";
	bool toOld = false;

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

		creator->addFileData(offset, chunk, data);
		free(data);
#ifndef WINDOWS_SYS
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
		usleep(250000); /* 1/4 of sec */
#else
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
		Sleep(250); /* 1/4 of sec */
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

		chunk = 1000; /* reset chunk size */

	}

	uint64_t end_size = creator->getFileSize();
	uint64_t end_trans = creator->getRecvd();
	std::cerr << "End FileSize: " << end_size << std::endl;
	std::cerr << "End Transferred:" << end_trans << std::endl;
        CHECK(init_size == end_size);
        CHECK(end_trans == end_size);

        REPORT("Test Fill");

	return 1;
}

