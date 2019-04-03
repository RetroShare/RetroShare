
/*******************************************************************************
 * libretroshare/src/crypto: crypto.h                                          *
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

#include "crypto/chacha20.h"

namespace librs
{
namespace crypto
{
/*!
 * \brief encryptAuthenticateData
 			Encrypts/decrypts data, using a autenticated construction + chacha20, based on a given 32 bytes master key. The actual encryption using a randomized key
            based on a 96 bits IV. Input values are not touched (memory is not released). Memory ownership of outputs is left to the client.
 * \param clear_data				input data to encrypt
 * \param clear_data_size			length of input data
 * \param encryption_master_key		encryption master key of length 32 bytes.
 * \param encrypted_data			encrypted data, allocated using malloc
 * \param encrypted_data_len		length of encrypted data
 * \return
 * 			true if everything went well.
 */
bool encryptAuthenticateData(const unsigned char *clear_data,uint32_t clear_data_size,uint8_t *encryption_master_key,unsigned char *& encrypted_data,uint32_t& encrypted_data_size);

/*!
 * \brief decryptAuthenticateData
 			Encrypts/decrypts data, using a autenticated construction + chacha20, based on a given 32 bytes master key. The actual encryption using a randomized key
            based on a 96 bits IV. Input values are not touched (memory is not released). Memory ownership of outputs is left to the client.
 * \param encrypted_data			input encrypted data
 * \param encrypted_data_size		input encrypted data length
 * \param encryption_master_key		encryption master key of length 32 bytes.
 * \param decrypted_data			decrypted data, allocated using malloc.
 * \param decrypted_data_size		length of allocated decrypted data.
 * \return
 * 			true if decryption + authentication are ok.
 */
bool decryptAuthenticateData(const unsigned char *encrypted_data,uint32_t encrypted_data_size, uint8_t* encryption_master_key, unsigned char *& decrypted_data,uint32_t& decrypted_data_size);
}
}

