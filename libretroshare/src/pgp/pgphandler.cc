#include <stdexcept>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdlib.h>

extern "C" {
#include <openpgpsdk/util.h>
}
#include "pgphandler.h"

std::string	PGPIdType::toStdString() const
{
	std::ostringstream tmpout;

	for(int j = 0; j < KEY_ID_SIZE; j++)
		tmpout << std::setw(2) << std::setfill('0') << std::hex << (int)bytes[j] ;

	return tmpout.str() ;
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

uint64_t PGPIdType::toUInt64() const
{
	uint64_t res = 0  ;

	for(int i=0;i<KEY_ID_SIZE;++i)
		res = (res << 8) + bytes[i] ;

	return res ;
}

PGPHandler::PGPHandler(const std::string& pubring, const std::string& secring)
	:_pubring_path(pubring),_secring_path(secring),
	pgphandlerMtx(std::string("PGPHandler"))
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
