/*
 * libretroshare/src/utils: rsaescrypt.h
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

#include <stdint.h>

class RsAes
{
	public:
		// Crypt/decrypt data using a 16 bytes key and a 8 bytes salt.
		//
		//		output_data allocation is left to the client. The size should be at least input_data_length+16 bytes.
		//
		//	Return value:
		//		true: encryption/decryption ok
		//
		//		false: encryption/decryption went bad. Check buffer size.
		//
		static bool   aes_crypt_8_16(const uint8_t *input_data,uint32_t input_data_length,uint8_t key[16],uint8_t salt[8],uint8_t *output_data,uint32_t& output_data_length) ;
		static bool aes_decrypt_8_16(const uint8_t *input_data,uint32_t input_data_length,uint8_t key[16],uint8_t salt[8],uint8_t *output_data,uint32_t& output_data_length) ;
};

