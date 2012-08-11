#include <fstream>

#include "argstream.h"
#include <pgp/rscertificate.h>

int main(int argc,char *argv[])
{
	try
	{
		argstream as(argc,argv) ;
		std::string keyfile ;

		as >> parameter('i',"input",keyfile,"input certificate file (old or new formats)",true)
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
		std::cerr << "Parsing..." << std::endl;

		RsCertificate cert(res) ;

		std::cerr << "Output from certificate:" << std::endl;

		std::cerr << cert.toStdString_oldFormat() << std::endl ;

		std::cerr << "Output from certificate (new format):" << std::endl;
		std::cerr << cert.toStdString() << std::endl ;

		return 0;
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception never handled: " << e.what() << std::endl;
		return 1 ;
	}
}

