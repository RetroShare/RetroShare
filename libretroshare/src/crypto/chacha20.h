/*******************************************************************************
 * libretroshare/src/crypto: chacha20.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2016 by Mr.Alice                                                  *
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

namespace librs
{
	namespace crypto
	{
        /*!
         * \brief chacha20_encrypt
         *          Performs in place encryption/decryption of the supplied data, using chacha20, using the supplied key and nonce.
         *
         * \param key           	secret encryption key. *Should never* be re-used.
         * \param block_counter		any integer. 0 is fine.
         * \param nonce 			acts as an initialzation vector. /!\ it is extremely important to make sure that this nounce *is* everytime different. Using a purely random value is fine.
         * \param data				data that gets encrypted/decrypted in place
         * \param size				size of the data.
         */
        void chacha20_encrypt(uint8_t key[32], uint32_t block_counter, uint8_t nonce[12], uint8_t *data, uint32_t size) ;

        /*!
         * \brief poly1305_tag
         *           Computes an authentication tag for the supplied data, using the given secret key.
         * \param key				secret key. *Should not* be used multiple times.
         * \param message			message to generate a tag for
         * \param size				size of the message
         * \param tag				place where the tag is stored.
         */

        void poly1305_tag(uint8_t key[32],uint8_t *message,uint32_t size,uint8_t tag[16]);

        /*!
         * \brief AEAD_chacha20_poly1305
         * 			 Provides in-place authenticated encryption using the AEAD construction as described in RFC7539.
         * 			The data is first encrypted in place then 16-padded and concatenated to its size, than concatenated to the
         * 			16-padded AAD (additional authenticated data) and its size, authenticated using poly1305.
         *
         * \param key			key that is used to derive a one time secret key for poly1305 and that is also used to encrypt the data
         * \param nonce			nonce. *Should be unique* in order to make the chacha20 stream cipher unique.
         * \param data			data that is encrypted/decrypted in place.
         * \param size			size of the data
         * \param aad           additional authenticated data. Can be used to authenticate the nonce.
         * \param aad_size
         * \param tag			generated poly1305 tag.
         * \param encrypt		true to encrypt, false to decrypt and check the tag.
         * \return
         * 			always true for encryption.
         * 			authentication result for decryption. data is *always* xored to the cipher stream whatever the authentication result is.
         */
        bool AEAD_chacha20_poly1305(uint8_t key[32], uint8_t nonce[12],uint8_t *data,uint32_t data_size,uint8_t *aad,uint32_t aad_size,uint8_t tag[16],bool encrypt_or_decrypt) ;

        /*!
         * \brief AEAD_chacha20_sha256
         * 			 Provides authenticated encryption using a simple construction that associates chacha20 encryption with HMAC authentication using
         *          the same 32 bytes key. The authenticated tag is the 16 first bytes of the sha256 HMAC.
         *
         * \param key           encryption/authentication key
         * \param nonce         nonce. *Should be unique* in order to make chacha20 stream cipher unique.
         * \param data          data that is encrypted/decrypted in place
         * \param data_size     size of data to encrypt/authenticate
         * \param aad           additional authenticated data. Can be used to authenticate the nonce.
         * \param aad_size
         * \param tag           16 bytes authentication tag result
         * \param encrypt		true to encrypt, false to decrypt and check the tag.
         * \return
         * 			always true for encryption.
         * 			authentication result for decryption. data is *always* xored to the cipher stream whatever the authentication result is.
         */
        bool AEAD_chacha20_sha256(uint8_t key[32], uint8_t nonce[12],uint8_t *data,uint32_t data_size,uint8_t *aad,uint32_t aad_size,uint8_t tag[16],bool encrypt_or_decrypt) ;

        /*!
         * \brief constant_time_memcmp
         * 			Provides a constant time comparison of two memory chunks. Calls CRYPTO_memcmp.
         *
         * \param m1    memory block 1
         * \param m2	memory block 2
         * \param size  common size of m1 and m2
         * \return
         * 			false  if the two chunks are different
         * 			true   if the two chunks are identical
         */
        bool constant_time_memory_compare(const uint8_t *m1,const uint8_t *m2,uint32_t size) ;

        /*!
         * \brief perform_tests
         *          Tests all methods in this class, using the tests supplied in RFC7539
         * \return
         * 			true is all tests pass
         */

        bool perform_tests() ;
	}
}
