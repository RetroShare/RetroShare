/*******************************************************************************
 * libretroshare/src/util: rsaes.h                                             *
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
#include <stdint.h>

class RsAES
{
	public:
		// Crypt/decrypt data using a 16 bytes key and a 8 bytes salt.
		//
		//		output_data allocation is left to the client. The size should be at least RsAES::get_buffer_size(input_data_length)
		//
		//	Return value:
		//		true: encryption/decryption ok
		//
		//		false: encryption/decryption went bad. Check buffer size.
		//
		static bool   aes_crypt_8_16(const uint8_t *input_data,uint32_t input_data_length,uint8_t key[16],uint8_t salt[8],uint8_t *output_data,uint32_t& output_data_length) ;
		static bool aes_decrypt_8_16(const uint8_t *input_data,uint32_t input_data_length,uint8_t key[16],uint8_t salt[8],uint8_t *output_data,uint32_t& output_data_length) ;

		// computes the safe buffer size to store encrypted/decrypted data for the given input stream size
		//
		static uint32_t get_buffer_size(uint32_t size) ;
};

