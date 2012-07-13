// COMPILE_LINE: g++ -o test_key_parsing test_key_parsing.cc -g -I../../../openpgpsdk/include  -I../ -L../lib ../../../openpgpsdk/src/lib/libops.a -lssl -lcrypto -lbz2
//
#include <stdlib.h>
#include <iostream>

extern "C"
{
	#include <openpgpsdk/std_print.h>
	#include <openpgpsdk/keyring_local.h>
	#include <openpgpsdk/util.h>
}

#include "argstream.h"

int main(int argc,char *argv[])
{
	try
	{
		// test PGPHandler
		//
		// 0 - init

		bool armoured = false ;
		std::string keyfile ;

		argstream as(argc,argv) ;

		as >> parameter('i',"input-key",keyfile,"input key file.",true)
			>> option('a',"armoured",armoured,"input is armoured")
			>> help() ;

		as.defaultErrorHandling() ;

		ops_keyring_t *kr = (ops_keyring_t*)malloc(sizeof(ops_keyring_t)) ;
		kr->nkeys = 0 ;
		kr->nkeys_allocated = 0 ;
		kr->keys = 0 ;

		if(ops_false == ops_keyring_read_from_file(kr,armoured, keyfile.c_str()))
			throw std::runtime_error("PGPHandler::readKeyRing(): cannot read key file. File corrupted, or missing/superfluous armour parameter.") ;

		for(int i=0;i<kr->nkeys;++i)
		{
			ops_print_public_keydata(&kr->keys[i]) ;
			ops_print_public_keydata_verbose(&kr->keys[i]) ;
			ops_print_public_key(&kr->keys[i].key.pkey) ;
		}

		ops_list_packets(const_cast<char *>(keyfile.c_str()),armoured,kr,NULL) ;

		return 0 ;
	}
	catch(std::exception& e)
	{
		std::cerr << "Caught exception: " << e.what() << std::endl;
		return 1 ;
	}
}


