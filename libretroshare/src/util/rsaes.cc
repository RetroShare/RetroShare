/*
 * libretroshare/src/utils: rsaes.cc
 *
 * AES crptography for RetroShare.
 *
 * Copyright 2013 by Cyril Soler
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */

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

	EVP_CIPHER_CTX e_ctx ;
	EVP_CIPHER_CTX_init(&e_ctx);
	EVP_EncryptInit_ex(&e_ctx, EVP_aes_256_cbc(), NULL, key, iv);

	/* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
	int c_len = input_data_length + AES_BLOCK_SIZE ;
	int f_len = 0;

	if(output_data_length < (uint32_t)c_len)
		return false ;

	/* update ciphertext, c_len is filled with the length of ciphertext generated,
	 *len is the size of plaintext in bytes */

	EVP_EncryptUpdate(&e_ctx, output_data, &c_len, input_data, input_data_length);

	/* update ciphertext with the final remaining bytes */
	EVP_EncryptFinal_ex(&e_ctx, output_data+c_len, &f_len);

	output_data_length = c_len + f_len;

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

	EVP_CIPHER_CTX e_ctx ;
	EVP_CIPHER_CTX_init(&e_ctx);
	EVP_DecryptInit_ex(&e_ctx, EVP_aes_256_cbc(), NULL, key, iv);

	/* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
	int c_len = input_data_length + AES_BLOCK_SIZE ;
	int f_len = 0;

	if(output_data_length < (uint32_t)c_len)
		return false ;

	output_data_length = c_len ;

	/* update ciphertext, c_len is filled with the length of ciphertext generated,
	 *len is the size of plaintext in bytes */

	EVP_DecryptUpdate(&e_ctx, output_data, &c_len, input_data, input_data_length);

	/* update ciphertext with the final remaining bytes */
	EVP_DecryptFinal_ex(&e_ctx, output_data+c_len, &f_len);

	output_data_length = c_len + f_len;

	return true;
}

