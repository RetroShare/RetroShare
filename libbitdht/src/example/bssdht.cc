
#include "bootstrap_fn.h"
#include <iostream>
#include <inttypes.h>

void args(char *name)
{
	std::cerr << std::endl;
	std::cerr << "Dht Single Shot Searcher";
	std::cerr << std::endl;
	std::cerr << "Usage:";
	std::cerr << std::endl;
	std::cerr << "\t" << name << " -p <peerId> ";
	std::cerr << std::endl;
	std::cerr << std::endl;
	std::cerr << "NB: The PeerId is Required to Run";
	std::cerr << std::endl;
	std::cerr << std::endl;
}


int main(int argc, char **argv)
{

	std::string bootstrapfile = "bdboot.txt";
	std::string peerId;
	std::string ip;
	uint16_t port;
	int c;
	bool havePeerId = false;
	

	while((c = getopt(argc, argv,"p:")) != -1)
	{
		switch (c)
		{
			case 'p':
				peerId = optarg;
				havePeerId = true;
				break;
			default:
				args(argv[0]);
				return 1;
			break;
		}
	}

	if (!havePeerId)
	{
		args(argv[0]);
		return 1;
	}

	
        std::cerr << "bssdht: starting up";
	std::cerr << std::endl;

	bdSingleShotFindPeer(bootstrapfile, peerId, ip, port);

        std::cerr << "bssdht: finished";
	std::cerr << std::endl;

	return 1;
}


		





