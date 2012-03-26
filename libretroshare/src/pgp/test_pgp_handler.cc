// COMPILE_LINE: g++ -o test_pgp_handler test_pgp_handler.cc -I../../../openpgpsdk/include  -I../ -L../lib -lretroshare ../../../openpgpsdk/lib/libops.a -lssl -lcrypto -lbz2
//
#include <iostream>
#include "pgphandler.h"

int main(int argc,char *argv[])
{
	// test pgp ids.
	//
	PGPIdType id("3e5b22140ef56abb") ;

	std::cerr << "Id is : " << std::hex << id.toUInt64() << std::endl;
	std::cerr << "Id st : " << id.toStdString() << std::endl;

	// test PGPHandler
	//
	// 0 - init
	
	static const std::string pubring = "pubring.gpg" ;
	static const std::string secring = "secring.gpg" ;

	PGPHandler pgph(pubring,secring) ;

	return 0 ;
}
