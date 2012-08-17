#include <fstream>
#include <string.h>

#include "argstream.h"
//#include <pqi/cleanupxpgp.h>
#include <retroshare/rspeers.h>
#include <pgp/rscertificate.h>
#include <pgp/pgphandler.h>

int main(int argc,char *argv[])
{
	try
	{
		argstream as(argc,argv) ;
		std::string keyfile ;
		bool clean_cert = false ;

		as >> parameter('i',"input",keyfile,"input certificate file (old or new formats)",true)
			>> option('c',"clean",clean_cert,"clean cert before parsing")
			>> help() ;

		as.defaultErrorHandling() ;

		FILE *f = fopen(keyfile.c_str(),"rb") ;

		if(f == NULL)
			throw std::runtime_error("Cannot open file. Sorry.") ;

		std::string res ;
		char c ;

		while( (c = fgetc(f)) != EOF)
			res += c ;

		fclose(f) ;

		std::cerr << "Read this string from the file:" << std::endl;
		std::cerr << "==========================================" << std::endl;
		std::cerr << res << std::endl;
		std::cerr << "==========================================" << std::endl;

		if(clean_cert)
		{
			std::string res2 ;
			int err ;

			//res2 = cleanUpCertificate(res,err) ;

			//if(res2 == "")
			//	std::cerr << "Error while cleaning: " << err << std::endl;
			//else
			//	res = res2 ;

			std::cerr << "Certificate after cleaning:" << std::endl;
			std::cerr << "==========================================" << std::endl;
			std::cerr << res << std::endl;
			std::cerr << "==========================================" << std::endl;
		}
		std::cerr << "Parsing..." << std::endl;

		RsCertificate cert(res) ;

		std::cerr << "Output from certificate:" << std::endl;

		std::cerr << cert.toStdString_oldFormat() << std::endl ;

		std::cerr << "Output from certificate (new format):" << std::endl;
		std::cerr << cert.toStdString() << std::endl ;

		std::string key_id ;
		std::string name ;
		std::list<std::string> signers ;

		PGPHandler handler("toto1","toto2","toto3","toto4") ;
handler.getGPGDetailsFromBinaryBlock(cert.pgp_key(),cert.pgp_key_size(),key_id,name,signers) ;

		std::cerr << "Details loaded from certificate:" << std::endl;
		std::cerr << "PGP id\t: " << key_id << std::endl;
		std::cerr << "Key name\t: " << name << std::endl;
		std::cerr << "Signers\t:" << std::endl;

		for(std::list<std::string>::const_iterator it(signers.begin());it!=signers.end();++it)
			std::cerr << "        " << *it << std::endl;

		return 0;
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception never handled: " << e.what() << std::endl;
		return 1 ;
	}
}

