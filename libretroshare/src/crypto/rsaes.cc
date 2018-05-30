/*******************************************************************************
 * libretroshare/src/util: rsaes.cc                                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013-2013 Cyril Soler <csoler@users.sourceforge.net>              *
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
#include <iostream>
#include <openssl/evp.h>
#include <openssl/aes.h>

#include "rsaes.h"

uint32_t RsAES::get_buffer_size(uint32_t n)
{
	return n + AES_BLOCK_SIZE ;
}

bool RsAES::aes_crypt_8_16(const uint8_t *input_data,uint32_t input_data_length,uint8_t key_data[16],uint8_t salt[8],uint8_t *output_data,uint32_t& output_data_length)
{
	int nrounds = 5;
	uint8_t key[32], iv[32];

	/*
	 * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
	 * nrounds is the number of times the we hash the material. More rounds are more secure but
	 * slower.
	 */
	int i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, 16, nrounds, key, iv);

	if (i != 32) 
	{
		printf("Key size is %d bits - should be 256 bits\n", i);
		return false ;
	}

	EVP_CIPHER_CTX *e_ctx = EVP_CIPHER_CTX_new();
	EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);

	/* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
	int c_len = input_data_length + AES_BLOCK_SIZE ;
	int f_len = 0;

	if(output_data_length < (uint32_t)c_len)
    {
	    EVP_CIPHER_CTX_free(e_ctx) ;
	    return false ;
    }

	/* update ciphertext, c_len is filled with the length of ciphertext generated,
	 *len is the size of plaintext in bytes */

	if(!EVP_EncryptUpdate(e_ctx, output_data, &c_len, input_data, input_data_length))
	{
		std::cerr << "RsAES: decryption failed at end. Check padding." << std::endl;
        	EVP_CIPHER_CTX_free(e_ctx) ;
		return false ;
	}

	/* update ciphertext with the final remaining bytes */
	if(!EVP_EncryptFinal_ex(e_ctx, output_data+c_len, &f_len))
	{
		std::cerr << "RsAES: decryption failed at end. Check padding." << std::endl;
        	EVP_CIPHER_CTX_free(e_ctx) ;
		return false ;
	}

	output_data_length = c_len + f_len;

	EVP_CIPHER_CTX_free(e_ctx) ;
	return true;
}

bool RsAES::aes_decrypt_8_16(const uint8_t *input_data,uint32_t input_data_length,uint8_t key_data[16],uint8_t salt[8],uint8_t *output_data,uint32_t& output_data_length)
{
	int nrounds = 5;
	uint8_t key[32], iv[32];

	/*
	 * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
	 * nrounds is the number of times the we hash the material. More rounds are more secure but
	 * slower.
	 */
	int i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, 16, nrounds, key, iv);

	if (i != 32) 
	{
		printf("Key size is %d bits - should be 256 bits\n", i);
		return false ;
	}

	EVP_CIPHER_CTX *e_ctx = EVP_CIPHER_CTX_new();
	EVP_DecryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);

	/* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
	int c_len = input_data_length + AES_BLOCK_SIZE ;
	int f_len = 0;

	if(output_data_length < (uint32_t)c_len)
    {
        	EVP_CIPHER_CTX_free(e_ctx) ;
		return false ;
    }

	output_data_length = c_len ;

	/* update ciphertext, c_len is filled with the length of ciphertext generated,
	 *len is the size of plaintext in bytes */

	if(! EVP_DecryptUpdate(e_ctx, output_data, &c_len, input_data, input_data_length))
	{
		std::cerr << "RsAES: decryption failed." << std::endl;
        	EVP_CIPHER_CTX_free(e_ctx) ;
		return false ;
	}

	/* update ciphertext with the final remaining bytes */
	if(!EVP_DecryptFinal_ex(e_ctx, output_data+c_len, &f_len))
	{
		std::cerr << "RsAES: decryption failed at end. Check padding." << std::endl;
        	EVP_CIPHER_CTX_free(e_ctx) ;
		return false ;
	}

	output_data_length = c_len + f_len;

	EVP_CIPHER_CTX_free(e_ctx) ;
	return true;
}

