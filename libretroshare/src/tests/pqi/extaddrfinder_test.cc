#include "util/utest.h"

#include <iostream>
#include "pqi/pqinetwork.h"

#include <tcponudp/extaddrfinder.h>
#include <pqi/authgpg.h>

INITTEST();

int main()
{
	std::cerr << "Testing the ext address finder service. This might take up to 10 secs..." << std::endl ;

	ExtAddrFinder fnd ;
	in_addr addr ;
	uint32_t tries = 0 ;

	while(! fnd.hasValidIP( &addr ))
	{
		sleep(1) ;

		if(++tries > 20)
		{
			std::cerr << "Failed !" << std::endl ;
			CHECK(false) ;
		}
	}

	std::cerr << "Found the following IP: " << inet_ntoa(addr) << std::endl ;

	FINALREPORT("extaddrfinder_test");

	return TESTRESULT() ;
}


