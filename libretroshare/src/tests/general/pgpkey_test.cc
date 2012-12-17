#ifdef LINUX
#include <fenv.h>
#endif
#include <iostream>
#include <stdexcept>
#include <vector>
#include <stdlib.h>
#include <math.h>
#include "util/utest.h"
#include "pgp/pgpkeyutil.h"

INITTEST();

int main(int argc, char **argv)
{
#ifdef LINUX
	feenableexcept(FE_INVALID) ;
	feenableexcept(FE_DIVBYZERO) ;
#endif
	try
	{
	if(argc < 2)
	{
		std::cerr << argv[0] << ": test gpg certificate cleaning method. " << std::endl;
		std::cerr << "   Usage: " << argv[0] << " certificate.asc" << std::endl;          
		return 0 ;
	}

	FILE *f = fopen(argv[1],"r") ;

	if(f == NULL)
		throw std::runtime_error(std::string("Could not open file ") + argv[1]) ;

	std::string cert ;
	int c ;

	while((c = getc(f) ) != EOF)
		cert += (char)c ;

	std::cerr << "got this certificate: " << std::endl;
	std::cerr << cert << std::endl;

	std::cerr << "Calling cert simplification code..." << std::endl;
	std::string cleaned_key ;

	PGPKeyManagement::createMinimalKey(cert,cleaned_key) ;

	std::cerr << "Minimal key produced: " << std::endl;
	std::cerr << cleaned_key << std::endl;
	FINALREPORT("pgpkey_test");
	exit(TESTRESULT());
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception never handled: " << e.what() << std::endl ;
		return 1 ;
	}
}

