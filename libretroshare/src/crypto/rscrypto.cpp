/*******************************************************************************
 * libretroshare/src/crypto: crypto.cc                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 by Cyril Soler <csoler@users.sourceforge.net>                *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "rscrypto.h"
#include "util/rsrandom.h"

//#define CRYPTO_DEBUG 1

namespace librs {
namespace crypto {

#define RSCRYPTO_DEBUG() std::cerr << time(NULL) << " : RSCRYPTO : " << __FUNCTION__ << " : "
#define RSCRYPTO_ERROR() std::cerr << "(EE) RSCRYPTO ERROR : "

static const uint32_t ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE = 12 ;
static const uint32_t ENCRYPTED_MEMORY_AUTHENTICATION_TAG_SIZE    = 16 ;
static const uint32_t ENCRYPTED_MEMORY_HEADER_SIZE                =  4 ;
static const uint32_t ENCRYPTED_MEMORY_EDATA_SIZE                 =  4 ;

static const uint8_t  ENCRYPTED_MEMORY_FORMAT_AEAD_CHACHA20_POLY1305 = 0x01 ;
static const uint8_t  ENCRYPTED_MEMORY_FORMAT_AEAD_CHACHA20_SHA256   = 0x02 ;

bool encryptAuthenticateData(const unsigned char *clear_data,uint32_t clear_data_size,uint8_t *encryption_master_key,unsigned char *& encrypted_data,uint32_t& encrypted_data_len)
{
	uint8_t initialization_vector[ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE] ;

	RSRandom::random_bytes(initialization_vector,ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE) ;

#ifdef CRYPTO_DEBUG
	RSCRYPTO_DEBUG() << "ftServer::Encrypting ft item." << std::endl;
	RSCRYPTO_DEBUG() << "  random nonce    : " << RsUtil::BinToHex(initialization_vector,ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE) << std::endl;
#endif

	uint32_t item_serialized_size = clear_data_size;//RsGenericSerializer().size(clear_item) ;
	uint32_t total_data_size = ENCRYPTED_MEMORY_HEADER_SIZE + ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE + ENCRYPTED_MEMORY_EDATA_SIZE + item_serialized_size + ENCRYPTED_MEMORY_AUTHENTICATION_TAG_SIZE  ;

#ifdef CRYPTO_DEBUG
	RSCRYPTO_DEBUG() << "  clear part size : " << size(clear_item) << std::endl;
	RSCRYPTO_DEBUG() << "  total item size : " << total_data_size << std::endl;
#endif

	encrypted_data = (unsigned char*)rs_malloc( total_data_size ) ;
	encrypted_data_len  = total_data_size ;

	if(encrypted_data == NULL)
		return false ;

	uint8_t *edata = (uint8_t*)encrypted_data;
	uint32_t edata_size = item_serialized_size;
	uint32_t offset = 0;

	edata[0] = 0xae ;
	edata[1] = 0xad ;
	edata[2] = ENCRYPTED_MEMORY_FORMAT_AEAD_CHACHA20_SHA256 ;       // means AEAD_chacha20_sha256
	edata[3] = 0x01 ;

	offset += ENCRYPTED_MEMORY_HEADER_SIZE;
	uint32_t aad_offset = offset ;
	uint32_t aad_size = ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE + ENCRYPTED_MEMORY_EDATA_SIZE ;

	memcpy(&edata[offset], initialization_vector, ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE) ;
	offset += ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE ;

	edata[offset+0] = (edata_size >>  0) & 0xff ;
	edata[offset+1] = (edata_size >>  8) & 0xff ;
	edata[offset+2] = (edata_size >> 16) & 0xff ;
	edata[offset+3] = (edata_size >> 24) & 0xff ;

	offset += ENCRYPTED_MEMORY_EDATA_SIZE ;

	memcpy(&edata[offset],clear_data,clear_data_size);

#ifdef CRYPTO_DEBUG
	RSCRYPTO_DEBUG() << "  clear item      : " << RsUtil::BinToHex(&edata[offset],std::min(50,(int)total_data_size-(int)offset)) << "(...)" << std::endl;
#endif

	uint32_t clear_item_offset = offset ;
	offset += edata_size ;

	uint32_t authentication_tag_offset = offset ;
	assert(ENCRYPTED_MEMORY_AUTHENTICATION_TAG_SIZE + offset == total_data_size) ;

	//uint8_t encryption_key[32] ;
	//deriveEncryptionKey(hash,encryption_key) ;

	if(edata[2] == ENCRYPTED_MEMORY_FORMAT_AEAD_CHACHA20_POLY1305)
		librs::crypto::AEAD_chacha20_poly1305(encryption_master_key,initialization_vector,&edata[clear_item_offset],edata_size, &edata[aad_offset],aad_size, &edata[authentication_tag_offset],true) ;
	else if(edata[2] == ENCRYPTED_MEMORY_FORMAT_AEAD_CHACHA20_SHA256)
		librs::crypto::AEAD_chacha20_sha256  (encryption_master_key,initialization_vector,&edata[clear_item_offset],edata_size, &edata[aad_offset],aad_size, &edata[authentication_tag_offset],true) ;
	else
		return false ;

#ifdef CRYPTO_DEBUG
	RSCRYPTO_DEBUG() << "  encryption key  : " << RsUtil::BinToHex(encryption_key,32) << std::endl;
	RSCRYPTO_DEBUG() << "  authen. tag     : " << RsUtil::BinToHex(&edata[authentication_tag_offset],ENCRYPTED_MEMORY_AUTHENTICATION_TAG_SIZE) << std::endl;
	RSCRYPTO_DEBUG() << "  final item      : " << RsUtil::BinToHex(&edata[0],std::min(50u,total_data_size)) << "(...)" << std::endl;
#endif

	return true ;
}

// Decrypts the given item using aead-chacha20-poly1305
bool decryptAuthenticateData(const unsigned char *encrypted_data,uint32_t encrypted_data_len,uint8_t *encryption_master_key, unsigned char *& decrypted_data, uint32_t& decrypted_data_size)
{
	//uint8_t encryption_key[32] ;
	//deriveEncryptionKey(hash,encryption_key) ;

	uint8_t *edata = (uint8_t*)encrypted_data;
	uint32_t offset = 0;

	if(encrypted_data_len < ENCRYPTED_MEMORY_HEADER_SIZE + ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE + ENCRYPTED_MEMORY_EDATA_SIZE) return false ;

	if(edata[0] != 0xae) return false ;
	if(edata[1] != 0xad) return false ;
	if(edata[2] != ENCRYPTED_MEMORY_FORMAT_AEAD_CHACHA20_POLY1305 && edata[2] != ENCRYPTED_MEMORY_FORMAT_AEAD_CHACHA20_SHA256) return false ;
	if(edata[3] != 0x01) return false ;

	offset += ENCRYPTED_MEMORY_HEADER_SIZE ;
	uint32_t aad_offset = offset ;
	uint32_t aad_size = ENCRYPTED_MEMORY_EDATA_SIZE + ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE ;

	uint8_t *initialization_vector = &edata[offset] ;

#ifdef CRYPTO_DEBUG
	RSCRYPTO_DEBUG() << "ftServer::decrypting ft item." << std::endl;
	RSCRYPTO_DEBUG() << "  item data       : " << RsUtil::BinToHex(edata,std::min(50u,encrypted_data_len) << "(...)" << std::endl;
	RSCRYPTO_DEBUG() << "  hash            : " << hash << std::endl;
	RSCRYPTO_DEBUG() << "  encryption key  : " << RsUtil::BinToHex(encryption_key,32) << std::endl;
	RSCRYPTO_DEBUG() << "  random nonce    : " << RsUtil::BinToHex(initialization_vector,ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE) << std::endl;
#endif

	offset += ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE ;

	uint32_t edata_size = 0 ;
	edata_size += ((uint32_t)edata[offset+0]) <<  0 ;
	edata_size += ((uint32_t)edata[offset+1]) <<  8 ;
	edata_size += ((uint32_t)edata[offset+2]) << 16 ;
	edata_size += ((uint32_t)edata[offset+3]) << 24 ;

	if(edata_size + ENCRYPTED_MEMORY_EDATA_SIZE + ENCRYPTED_MEMORY_AUTHENTICATION_TAG_SIZE + ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE + ENCRYPTED_MEMORY_HEADER_SIZE != encrypted_data_len)
	{
		RSCRYPTO_ERROR() << "  ERROR: encrypted data size is " << edata_size << ", should be " << encrypted_data_len - (ENCRYPTED_MEMORY_EDATA_SIZE + ENCRYPTED_MEMORY_AUTHENTICATION_TAG_SIZE + ENCRYPTED_MEMORY_INITIALIZATION_VECTOR_SIZE + ENCRYPTED_MEMORY_HEADER_SIZE ) << std::endl;
		return false ;
	}

	offset += ENCRYPTED_MEMORY_EDATA_SIZE ;
	uint32_t clear_item_offset = offset ;

	uint32_t authentication_tag_offset = offset + edata_size ;
#ifdef CRYPTO_DEBUG
	RSCRYPTO_DEBUG() << "  authen. tag     : " << RsUtil::BinToHex(&edata[authentication_tag_offset],ENCRYPTED_MEMORY_AUTHENTICATION_TAG_SIZE) << std::endl;
#endif

	bool result ;

	if(edata[2] == ENCRYPTED_MEMORY_FORMAT_AEAD_CHACHA20_POLY1305)
		result = librs::crypto::AEAD_chacha20_poly1305(encryption_master_key,initialization_vector,&edata[clear_item_offset],edata_size, &edata[aad_offset],aad_size, &edata[authentication_tag_offset],false) ;
	else if(edata[2] == ENCRYPTED_MEMORY_FORMAT_AEAD_CHACHA20_SHA256)
		result = librs::crypto::AEAD_chacha20_sha256  (encryption_master_key,initialization_vector,&edata[clear_item_offset],edata_size, &edata[aad_offset],aad_size, &edata[authentication_tag_offset],false) ;
	else
		return false ;

#ifdef CRYPTO_DEBUG
	RSCRYPTO_DEBUG() << "  authen. result  : " << result << std::endl;
	RSCRYPTO_DEBUG() << "  decrypted daya  : " << RsUtil::BinToHex(&edata[clear_item_offset],std::min(50u,edata_size)) << "(...)" << std::endl;
#endif

	if(!result)
	{
		RSCRYPTO_ERROR() << "(EE) decryption/authentication went wrong." << std::endl;
		return false ;
	}

	decrypted_data_size = edata_size ;
	decrypted_data = (unsigned char*)rs_malloc(edata_size) ;

	if(decrypted_data == NULL)
	{
		std::cerr << "Failed to allocate memory for decrypted data chunk of size " << edata_size << std::endl;
		return false ;
	}
	memcpy(decrypted_data,&edata[clear_item_offset],edata_size) ;

	return true ;
}

}
}

