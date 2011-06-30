

#include "bitdht/bdiface.h"
#include "bitdht/bdstddht.h"
#include "bdhandler.h"

int main(int argc, char **argv)
{

	/* startup dht : with a random id! */
        bdNodeId ownId;
        bdStdRandomNodeId(&ownId);

	uint16_t port = 6775;
	std::string appId = "exId";
	std::string bootstrapfile = "bdboot.txt";

	BitDhtHandler dht(&ownId, port, appId, bootstrapfile);

	/* run your program */
	while(1)
	{
        	bdNodeId searchId;
        	bdStdRandomNodeId(&searchId);

		dht.FindNode(&searchId);

		sleep(180);

		dht.DropNode(&searchId);
	}

	return 1;
}


		





