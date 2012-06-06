// COMPILE_LINE: g++ -o test_pgp_handler test_pgp_handler.cc -I../../../openpgpsdk/include  -I../ -L../lib -lretroshare ../../../libbitdht/src/lib/libbitdht.a ../../../openpgpsdk/lib/libops.a -lgnome-keyring -lupnp -lssl -lcrypto -lbz2
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
	PGPIdType id = PGPIdType(std::string("3e5b22140ef56abb")) ;

	//std::cerr << "Id is : " << std::hex << id.toUInt64() << std::endl;
	std::cerr << "Id st : " << id.toStdString() << std::endl;

	// test PGPHandler
	//
	// 0 - init

	static const std::string pubring = "pubring.gpg" ;
	static const std::string secring = "secring.gpg" ;

	PGPHandler::setPassphraseCallback(&passphrase_callback) ;
	PGPHandler pgph(pubring,secring) ;

	std::cerr << "Writing public keyring to file tmp_keyring.asc" << std::endl;
	pgph.writePublicKeyring("tmp_keyring.asc") ;

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

	PGPIdType id2 = PGPIdType(std::string("618E54CF7670FF5E")) ;
	std::cerr << "Now extracting key " << id2.toStdString() << " from keyring:" << std::endl ;
	std::string cert = pgph.SaveCertificateToString(id2,false) ;

	std::cerr << "Now, trying to re-read this cert from the string:" << std::endl;

	PGPIdType id3 ;
	std::string error_string ;
	pgph.LoadCertificateFromString(cert,id3,error_string) ;

	std::cerr << "Loaded cert id: " << id3.toStdString() << ", Error string=\"" << error_string << "\"" << std::endl;

	std::cerr << cert << std::endl;

	std::cerr << "Testing password callback: " << std::endl;

	std::string pass = passphrase_callback(NULL,newid.toStdString().c_str(),"Please enter password: ",false) ;

	std::cerr << "Password = \"" << pass << "\"" << std::endl;

	std::cerr << "Testing signature with keypair " << newid.toStdString() << std::endl;

	static const size_t BUFF_LEN = 25 ;
	unsigned char *test_bin = new unsigned char[BUFF_LEN] ;
	for(size_t i=0;i<BUFF_LEN;++i)
		test_bin[i] = rand()%26 + 'a' ;

	std::cerr << "Text = \"" << std::string((char *)test_bin,BUFF_LEN) << "\"" << std::endl;

	unsigned char sign[1000] ;
	uint32_t signlen = 1000 ;

	if(!pgph.SignDataBin(newid,test_bin,BUFF_LEN,sign,&signlen))
		std::cerr << "Signature error." << std::endl;
	else
		std::cerr << "Signature success." << std::endl;
 
	std::cerr << "Signature length: " << signlen << std::endl;
	std::cerr << "Signature: " << stringFromBytes(sign,signlen) << std::endl;
	std::cerr << "Now verifying signature..." << std::endl;

	PGPFingerprintType fingerprint ;
	if(!pgph.getKeyFingerprint(newid,fingerprint) )
		std::cerr << "Cannot find fingerprint of key id " << newid.toStdString() << std::endl;

	if(!pgph.VerifySignBin(test_bin,BUFF_LEN,sign,signlen,fingerprint))
		std::cerr << "Signature verification failed." << std::endl;
	else
		std::cerr << "Signature verification worked!" << std::endl;

	delete[] test_bin ;

	std::string outfile = "crypted_toto.pgp" ;
	std::string text_to_encrypt = "this is a secret message" ;

	std::cerr << "Checking encrypted file creation: streaming chain \"" << text_to_encrypt << "\" to file " << outfile << " with key " << id2.toStdString() << std::endl;

	if(!pgph.encryptTextToFile(id2,text_to_encrypt,outfile))
		std::cerr << "Encryption failed" << std::endl;
	else
		std::cerr << "Encryption success" << std::endl;

	std::string decrypted_text = "" ;
	outfile = "crypted_toto2.pgp" ;

	if(!pgph.decryptTextFromFile(id2,decrypted_text,outfile))
		std::cerr << "Decryption failed" << std::endl;
	else
		std::cerr << "Decryption success" << std::endl;

	std::cerr << "Decrypted text: \"" << decrypted_text << "\"" << std::endl;

	return 0 ;
}

