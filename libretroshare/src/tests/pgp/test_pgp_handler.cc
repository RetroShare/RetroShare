// COMPILE_LINE: g++ -o test_pgp_handler test_pgp_handler.cc -I../../../openpgpsdk/include  -I../ -L../lib -lretroshare ../../../libbitdht/src/lib/libbitdht.a ../../../openpgpsdk/lib/libops.a -lgnome-keyring -lupnp -lssl -lcrypto -lbz2
//
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <pgp/pgphandler.h>
#include <util/utest.h>
#include <common/argstream.h>

INITTEST() ;

static std::string static_passphrase = "" ;
static std::string passphrase_callback(void *data,const char *uid_info,const char *what,int prev_was_bad)
{
	if(!static_passphrase.empty())
		return static_passphrase ;

	if(prev_was_bad)
		std::cerr << "Bad passphrase." << std::endl;

	std::string wt = std::string("Please enter passphrase for key id ") + uid_info + " :";

	return std::string(getpass(wt.c_str())) ;
}

static std::string stringFromBytes(unsigned char *bytes,size_t len)
{
	static const char out[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' } ;

	std::string res ;

	for(uint32_t j = 0; j < len; j++)
	{
		res += out[ (bytes[j]>>4) ] ;
		res += out[ bytes[j] & 0xf ] ;
	}

	return res ;
}

static std::string askForKeyId(PGPHandler& pgph)
{
	// 0 - print keys and key types
	
	std::list<PGPIdType> lst ;	
	pgph.availableGPGCertificatesWithPrivateKeys(lst) ;

	std::cerr << lst.size() << " available key pairs: " << std::endl;
	int i=0 ;

	for(std::list<PGPIdType>::const_iterator it(lst.begin());it!=lst.end();++it,++i)
	{
		std::cerr << "(" << i << ") ID=" << (*it).toStdString() << ", type = " ;

		const PGPCertificateInfo *cert = pgph.getCertificateInfo(*it) ;

		switch(cert->_type)
		{
			case PGPCertificateInfo::PGP_CERTIFICATE_TYPE_DSA: std::cerr << "DSA" << std::endl;
													 break ;
			case PGPCertificateInfo::PGP_CERTIFICATE_TYPE_RSA: std::cerr << "RSA" << std::endl;
													 break ;
			default: 								std::cerr << "Unknown" << std::endl;
		}
	}

	if(i == 0)
		throw std::runtime_error("No key pair available in supplied keyring.") ;

	// 1 - ask for which key to use.
	
	int num = -1 ;

	while(true)
	{
		std::cerr << "Please enter selected key number (0 - " << i-1 << ") : " ;
		std::cerr.flush() ;

		char buf[10];
		fgets(buf,10,stdin) ;

		if(sscanf(buf,"%d",&num) == 1 && num >= 0 && num < i)
			break ;
	}

	// 2 - return the id string.

	std::list<PGPIdType>::const_iterator it(lst.begin()) ;
	for(int i=0;i<num;++i,++it) ;

	return (*it).toStdString() ;
}

int main(int argc,char *argv[])
{
	argstream as(argc,argv) ;

	bool test_pgpid_type = false ;
	bool test_keyring_read = false ;
	bool test_file_encryption = false ;
	bool test_gen_key = false ;
	bool test_output = false ;
	bool test_signature = false ;
	bool test_passphrase_callback = false ;
	bool full_test = false ;

	std::string key_id_string = "" ;
	std::string secring_file = "" ;
	std::string pubring_file = "" ;
	std::string file_to_encrypt = "" ;

	as >> option('1',"pgptype",test_pgpid_type,"Test pgp ID type")
		>> option('2',"keyring-read",test_keyring_read,"Test keyring read")
		>> option('3',"file-encryption",test_file_encryption,"Test file encryption. Needs -f, -i, -p and -s")
		>> option('4',"keygen",test_gen_key,"Test key generation.")
		>> option('5',"signature",test_signature,"Test signature.")
		>> option('6',"output",test_output,"Test output.")
		>> option('F',"fulltest",full_test,"Test everything.")
		>> parameter('f',"file",file_to_encrypt,"File to encrypt. Used with -3",false)
		>> parameter('p',"pubring",pubring_file,"Public keyring file.",false)
		>> parameter('s',"secring",secring_file,"Secret keyring file.",false)
		>> parameter('i',"keyid",key_id_string,"Key id to use. If not supplied, will be asked.",false)
		>> help() ;

	as.defaultErrorHandling() ;

	if(!full_test)
		if(test_pgpid_type + test_keyring_read + test_file_encryption + test_gen_key + test_signature + test_output != 1)
		{
			std::cerr << "Options 1 to 6 are mutually exclusive." << std::endl;
			return 1; 
		}
	if(full_test || test_pgpid_type)
	{
		// test pgp ids.
		//
		std::string st("3e5b22140ef56abb") ;
		PGPIdType id = PGPIdType(std::string(st)) ;

		//std::cerr << "Id is : " << std::hex << id.toUInt64() << std::endl;
		std::cerr << "Id st : " << id.toStdString() << std::endl;

		CHECK(id.toStdString(false) == st) ;
	}

	// test PGPHandler
	//
	// 0 - init

	std::string pubring = pubring_file.empty()?"pubring.gpg":pubring_file ;
	std::string secring = secring_file.empty()?"secring.gpg":secring_file ;

	static const std::string trustdb = "trustdb.gpg" ;
	static const std::string lockfile = "lock" ;

	PGPHandler::setPassphraseCallback(&passphrase_callback) ;
	PGPHandler pgph(pubring,secring,trustdb,lockfile) ;

	std::string email_str("test@gmail.com") ;
	std::string  name_str("test") ;
	std::string passw_str("test00") ;
	PGPIdType cert_id ; ;

	if(full_test || test_gen_key)
	{
		std::cerr << "Now generating a new PGP certificate: " << std::endl;
		std::cerr << "   email: " << email_str << std::endl;
		std::cerr << "   passw: " << passw_str << std::endl;
		std::cerr << "   name : " <<  name_str << std::endl;

		PGPIdType newid ;
		std::string errString ;
		static_passphrase = passw_str ;

		bool res = pgph.GeneratePGPCertificate(name_str, email_str, passw_str, newid, errString) ;

		cert_id = newid ;

		CHECK(res) ;

		if(!res)
			std::cerr << "Generation of certificate returned error: " << errString << std::endl;
		else
			std::cerr << "Certificate generation success. New id = " << newid.toStdString() << std::endl;
	}

	if(full_test)
		key_id_string = cert_id.toStdString(true) ;

	if(full_test || test_keyring_read)
	{
		pgph.printKeys() ;

		std::cerr << std::endl ;
		std::cerr << std::endl ;

		std::cerr << "Looking for keys with complete secret/public key pair: " << std::endl;

		std::list<PGPIdType> lst ;	
		pgph.availableGPGCertificatesWithPrivateKeys(lst) ;

		bool found = false ;

		for(std::list<PGPIdType>::const_iterator it(lst.begin());it!=lst.end();++it)
		{
			std::cerr << "Found id : " << (*it).toStdString() << std::endl;
			if(cert_id == *it)
				found = true ;
		}
		CHECK(found) ;
	}

	if(full_test || test_output)
	{
		PGPIdType id2( (key_id_string.empty())?askForKeyId(pgph):key_id_string) ;

		std::cerr << "Now extracting key " << id2.toStdString() << " from keyring:" << std::endl ;
		std::string cert = pgph.SaveCertificateToString(id2,false) ;

		std::cerr << "Now, trying to re-read this cert from the string:" << std::endl;

		PGPIdType id3 ;
		std::string error_string ;
		pgph.LoadCertificateFromString(cert,id3,error_string) ;

		std::cerr << "Loaded cert id: " << id3.toStdString() << ", Error string=\"" << error_string << "\"" << std::endl;

		CHECK(id3 == id2) ;

		std::cerr << cert << std::endl;
	}

	if(test_passphrase_callback)
	{
		std::cerr << "Testing password callback: " << std::endl;
		std::string newid = "XXXXXXXXXXXXXXXX" ;
		std::string pass = passphrase_callback(NULL,newid.c_str(),"Please enter password: ",false) ;
		std::cerr << "Password = \"" << pass << "\"" << std::endl;
	}

	if(full_test || test_signature)
	{
		if(key_id_string.empty())
			key_id_string = askForKeyId(pgph) ;

		PGPIdType key_id(key_id_string) ;
		std::cerr << "Testing signature with keypair " << key_id_string << std::endl;

		static const size_t BUFF_LEN = 25 ;
		unsigned char *test_bin = new unsigned char[BUFF_LEN] ;
		for(size_t i=0;i<BUFF_LEN;++i)
			test_bin[i] = rand()%26 + 'a' ;

		std::cerr << "Text = \"" << std::string((char *)test_bin,BUFF_LEN) << "\"" << std::endl;

		unsigned char sign[1000] ;
		uint32_t signlen = 1000 ;

		bool res = pgph.SignDataBin(key_id,test_bin,BUFF_LEN,sign,&signlen) ;

		CHECK(res) ;

		if(!res)
			std::cerr << "Signature error." << std::endl;
		else
			std::cerr << "Signature success." << std::endl;

		std::cerr << "Signature length: " << signlen << std::endl;
		std::cerr << "Signature: " << stringFromBytes(sign,signlen) << std::endl;
		std::cerr << "Now verifying signature..." << std::endl;

		PGPFingerprintType fingerprint ;
		res = pgph.getKeyFingerprint(key_id,fingerprint);

		CHECK(res) ;

		if(!res)
			std::cerr << "Cannot find fingerprint of key id " << key_id.toStdString() << std::endl;

		res = pgph.VerifySignBin(test_bin,BUFF_LEN,sign,signlen,fingerprint) ;
		CHECK(res) ;

		if(!res)
			std::cerr << "Signature verification failed." << std::endl;
		else
			std::cerr << "Signature verification worked!" << std::endl;

		delete[] test_bin ;
	}

	if(full_test || test_file_encryption)
	{
		if(key_id_string.empty())
			key_id_string = askForKeyId(pgph) ;

		PGPIdType key_id(key_id_string) ;
		std::string outfile = "crypted_toto.pgp" ;
		std::string text_to_encrypt = "this is a secret message" ;

		std::cerr << "Checking encrypted file creation: streaming chain \"" << text_to_encrypt << "\" to file " << outfile << " with key " << key_id.toStdString() << std::endl;

		bool res = pgph.encryptTextToFile(key_id,text_to_encrypt,outfile) ;

		CHECK(res) ;

		if(!res)
			std::cerr << "Encryption failed" << std::endl;
		else
			std::cerr << "Encryption success" << std::endl;

		std::string decrypted_text = "" ;
		outfile = "crypted_toto.pgp" ;

		res = pgph.decryptTextFromFile(key_id,decrypted_text,outfile) ;

		CHECK(res) ;

		if(!res)
			std::cerr << "Decryption failed" << std::endl;
		else
			std::cerr << "Decryption success" << std::endl;

		std::cerr << "Decrypted text: \"" << decrypted_text << "\"" << std::endl;

		CHECK(decrypted_text == text_to_encrypt) ;
	}

	FINALREPORT("PGP Handler test") ;
	return TESTRESULT() ;
}

