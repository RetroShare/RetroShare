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
	pgph.printKeys() ;

	std::cerr << std::endl ;
	std::cerr << std::endl ;

	std::cerr << "Looking for keys with complete secret/public key pair: " << std::endl;

	std::list<PGPIdType> lst ;	
	pgph.availableGPGCertificatesWithPrivateKeys(lst) ;

	for(std::list<PGPIdType>::const_iterator it(lst.begin());it!=lst.end();++it)
		std::cerr << "Found id : " << (*it).toStdString() << std::endl;

	std::string email_str("test@gmail.com") ;
	std::string  name_str("test") ;
	std::string passw_str("test00") ;

	std::cerr << "Now generating a new PGP certificate: " << std::endl;
	std::cerr << "   email: " << email_str << std::endl;
	std::cerr << "   passw: " << passw_str << std::endl;
	std::cerr << "   name : " <<  name_str << std::endl;

	PGPIdType newid ;
	std::string errString ;

	if(!pgph.GeneratePGPCertificate(name_str, email_str, passw_str, newid, errString))
		std::cerr << "Generation of certificate returned error: " << errString << std::endl;
	else
		std::cerr << "Certificate generation success. New id = " << newid.toStdString() << std::endl;

	return 0 ;
}

