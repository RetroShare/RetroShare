#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <string.h>

extern "C" {
#include <openpgpsdk/util.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/keyring.h>
}
#include "pgphandler.h"

std::string	PGPIdType::toStdString() const
{
	static const char out[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' } ;

	std::string res ;

	for(int j = 0; j < KEY_ID_SIZE; j++)
	{
		res += out[ (bytes[j]>>4) ] ;
		res += out[ bytes[j] & 0xf ] ;
	}

	return res ;
}

PGPIdType::PGPIdType(const std::string& s)
{
	int n=0;
	if(s.length() != KEY_ID_SIZE*2)
		throw std::runtime_error("PGPIdType::PGPIdType: can only init from 16 chars hexadecimal string") ;

	for(int i = 0; i < KEY_ID_SIZE; ++i)
	{
		bytes[i] = 0 ;

		for(int k=0;k<2;++k)
		{
			char b = s[n++] ;

			if(b >= 'A' && b <= 'F')
				bytes[i] += (b-'A'+10) << 4*(1-k) ;
			else if(b >= 'a' && b <= 'f')
				bytes[i] += (b-'a'+10) << 4*(1-k) ;
			else if(b >= '0' && b <= '9')
				bytes[i] += (b-'0') << 4*(1-k) ;
			else
				throw std::runtime_error("PGPIdType::Sha1CheckSum: can't init from non pure hexadecimal string") ;
		}
	}
}

PGPIdType::PGPIdType(const unsigned char b[])
{
	memcpy(bytes,b,8) ;
}

uint64_t PGPIdType::toUInt64() const
{
	uint64_t res = 0  ;

	for(int i=0;i<KEY_ID_SIZE;++i)
		res = (res << 8) + bytes[i] ;

	return res ;
}

PGPHandler::PGPHandler(const std::string& pubring, const std::string& secring)
	: pgphandlerMtx(std::string("PGPHandler")), _pubring_path(pubring),_secring_path(secring)
{
	// Allocate public and secret keyrings.
	// 
	_pubring = (ops_keyring_t*)malloc(sizeof(ops_keyring_t)) ;
	_secring = (ops_keyring_t*)malloc(sizeof(ops_keyring_t)) ;

	// Read public and secret keyrings from supplied files.
	//
	if(ops_false == ops_keyring_read_from_file(_pubring, false, pubring.c_str()))
		throw std::runtime_error("PGPHandler::readKeyRing(): cannot read pubring.") ;

	std::cerr << "Pubring read successfully." << std::endl;

	if(ops_false == ops_keyring_read_from_file(_secring, false, secring.c_str()))
		throw std::runtime_error("PGPHandler::readKeyRing(): cannot read secring.") ;

	std::cerr << "Secring read successfully." << std::endl;
}

PGPHandler::~PGPHandler()
{
	std::cerr << "Freeing PGPHandler. Deleting keyrings." << std::endl;

	ops_keyring_free(_pubring) ;
	ops_keyring_free(_secring) ;

	free(_pubring) ;
	free(_secring) ;
}

void PGPHandler::printKeys() const
{
	std::cerr << "Public keyring: " << std::endl;
	ops_keyring_list(_pubring) ;

	std::cerr << "Secret keyring: " << std::endl;
	ops_keyring_list(_secring) ;
}

bool PGPHandler::availableGPGCertificatesWithPrivateKeys(std::list<PGPIdType>& ids)
{
	// go through secret keyring, and check that we have the pubkey as well.
	//
	
	const ops_keydata_t *keydata = NULL ;
	int i=0 ;

	while( (keydata = ops_keyring_get_key_by_index(_secring,i++)) != NULL )
	{
		// check that the key is in the pubring as well

		if(ops_keyring_find_key_by_id(_pubring,keydata->key_id) != NULL)
			ids.push_back(PGPIdType(keydata->key_id)) ;
	}

	return true ;
}

static ops_parse_cb_return_t cb_get_passphrase(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo __attribute__((unused)))
{
	const ops_parser_content_union_t *content=&content_->content;
	//    validate_key_cb_arg_t *arg=ops_parse_cb_get_arg(cbinfo);
	//    ops_error_t **errors=ops_parse_cb_get_errors(cbinfo);

	switch(content_->tag)
	{
		case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
			/*
				Doing this so the test can be automated.
			 */
			*(content->secret_key_passphrase.passphrase)=ops_malloc_passphrase("hello");
			return OPS_KEEP_MEMORY;
			break;

		default:
			break;
	}

	return OPS_RELEASE_MEMORY;
}

bool PGPHandler::GeneratePGPCertificate(const std::string& name, const std::string& email, const std::string& passwd, PGPIdType& pgpId, std::string& errString) 
{
	static const int KEY_NUMBITS = 2048 ;

	ops_user_id_t uid ;
	const char *s = (name + " " + email).c_str() ;
	uid.user_id = (unsigned char *)s ;
	unsigned long int e = 44497 ; // some prime number

	ops_keydata_t *key = ops_rsa_create_selfsigned_keypair(KEY_NUMBITS,e,&uid) ;

	if(!key)
		return false ;

	pgpId = PGPIdType(key->key_id) ;

	ops_keydata_free(key) ;
	return true ;
}

