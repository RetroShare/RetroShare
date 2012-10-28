

#include "bitdht/bdiface.h"
#include "bitdht/bdstddht.h"
#include "bdhandler.h"

int main(int argc, char **argv)
{

	/* startup dht : with a random id! */
        bdNodeId ownId;
        bdStdRandomNodeId(&ownId);

	uint16_t port = 6775;
	std::string appId = "bsId";
	std::string bootstrapfile = "bdboot.txt";

	BitDhtHandler dht(&ownId, port, appId, bootstrapfile);

	/* install search node */
        bdNodeId searchId;
        bdStdRandomNodeId(&searchId);

        std::cerr << "bssdht: searching for Id: ";
        bdStdPrintNodeId(std::cerr, &searchId);
	std::cerr << std::endl;

        dht.FindNode(&searchId);

	/* run your program */
	bdId resultId;
	uint32_t status;

        resultId.id = searchId;

	while(false == dht.SearchResult(&resultId, status))
	{
		sleep(10);
	}

        std::cerr << "bssdht: Found Result:" << std::endl;

        std::cerr << "\tId: ";
        bdStdPrintId(std::cerr, &resultId);
	std::cerr << std::endl;

	std::cerr << "\tstatus: " << status;
	std::cerr << std::endl;

	return 1;
}


		





