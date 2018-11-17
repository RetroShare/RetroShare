/*******************************************************************************
 * libretroshare/src/crypto: hashstream.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
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
#pragma once

#include <openssl/evp.h>
#include <util/rsdir.h>

namespace librs
{
	namespace crypto
	{
		// Forward declare the class
		class HashStream;
		// Forward declare the template operator
		template<class T> HashStream& operator<<(HashStream& u, const T&);

		class HashStream
		{
		public:
			enum HashType { UNKNOWN = 0x00,
							SHA1    = 0x01
			};

			HashStream(HashType t);
			~HashStream();

			Sha1CheckSum hash() ;

			template<class T> friend HashStream& operator<<(HashStream& u, const T&) ;

			template<uint32_t ID_SIZE_IN_BYTES,bool UPPER_CASE,uint32_t UNIQUE_IDENTIFIER>
			friend HashStream& operator<<(HashStream& u,const t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>& r)
			{
				EVP_DigestUpdate(u.mdctx,r.toByteArray(),ID_SIZE_IN_BYTES);
				return u;
			}
			private:
				EVP_MD_CTX *mdctx ;
		};
	}
}
