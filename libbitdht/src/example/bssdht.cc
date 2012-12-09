
#include "bootstrap_fn.h"
#include <iostream>
#include <inttypes.h>

int main(int argc, char **argv)
{

	std::string bootstrapfile = "bdboot.txt";
	std::string peerId;
	std::string ip;
	uint16_t port;
	
        std::cerr << "bssdht: starting up";
	std::cerr << std::endl;

	bdSingleShotFindPeer(bootstrapfile, peerId, ip, port);

        std::cerr << "bssdht: finished";
	std::cerr << std::endl;

	return 1;
}


		





