// COMPILE_LINE: g++ -o test_pgp_signature_parsing test_pgp_signature_parsing.cc -g -I../../../openpgpsdk/include  -I../ -L../lib -lretroshare ../../../libbitdht/src/lib/libbitdht.a ../../../openpgpsdk/lib/libops.a -lgnome-keyring -lupnp -lssl -lcrypto -lbz2
//
#include <stdlib.h>
#include <iostream>
#include "pgphandler.h"

static std::string passphrase_callback(void *data,const char *uid_info,const char *what,int prev_was_bad)
{
	return std::string(getpass(what)) ;
}

static std::string stringFromBytes(unsigned char *bytes,size_t len)
{
	static const char out[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' } ;

	std::string res ;

	for(int j = 0; j < len; j++)
	{
		res += out[ (bytes[j]>>4) ] ;
		res += out[ bytes[j] & 0xf ] ;
	}

	return res ;
}

int main(int argc,char *argv[])
{
	// test pgp ids.
	//
	PGPIdType id = PGPIdType::fromUserId_hex("3e5b22140ef56abb") ;

	std::cerr << "Id is : " << std::hex << id.toUInt64() << std::endl;
	std::cerr << "Id st : " << id.toStdString() << std::endl;

	// test PGPHandler
	//
	// 0 - init

	static const std::string pubring = "globo.gpg" ;
	static const std::string secring = "globo.gpg" ;

	PGPHandler::setPassphraseCallback(&passphrase_callback) ;
	PGPHandler pgph(pubring,secring) ;

	pgph.printKeys() ;
}

