#include <fstream>
#include <string.h>

#include <util/utest.h>
#include <util/argstream.h>
//#include <pqi/cleanupxpgp.h>
#include <retroshare/rspeers.h>
#include <pgp/rscertificate.h>
#include <pgp/pgphandler.h>

INITTEST() ;

static std::string pgp_pwd_cb(void * /*hook*/, const char *uid_hint, const char * /*passphrase_info*/, int prev_was_bad)
{
#define GPG_DEBUG2
#ifdef GPG_DEBUG2
	fprintf(stderr, "pgp_pwd_callback() called.\n");
#endif
	std::string password;

	std::cerr << "SHould not be called!" << std::endl;
	return password ;
}

int main(int argc,char *argv[])
{
	try
	{
		argstream as(argc,argv) ;
		std::string idfile ;
		bool clean_cert = false ;

		as >> parameter('i',"input",idfile,"input retroshare identity file, ascii format",true)
			>> help() ;

		as.defaultErrorHandling() ;
		
		PGPHandler handler("toto1","toto2","toto3","toto4") ;

		PGPIdType imported_key_id ;
		std::string import_error_string ;

		bool res = handler.importGPGKeyPair(idfile,imported_key_id,import_error_string) ;

		handler.syncDatabase() ;

		if(!res)
			std::cerr << "Identity Import error: " << import_error_string << std::endl;

		CHECK(res) ;
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception never handled: " << e.what() << std::endl;
		return 1 ;
	}
}

