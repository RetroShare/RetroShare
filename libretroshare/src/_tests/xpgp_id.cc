

/***** Extract XPGP Id *****/

#include "pqi/authxpgp.h"

#include <iostream>
#include <sstream>

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <certfile>";
		std::cerr << std::endl;
		exit(1);
	}

        std::string userName, userId;

	if (LoadCheckXPGPandGetName(argv[1], userName, userId))
	{
		std::cerr << "Cert Ok: name: " << userName;
		std::cerr << std::endl;
		std::cerr << "id = \"" << userId << "\"";
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "Cert Check Failed";
		std::cerr << std::endl;
	}
}











