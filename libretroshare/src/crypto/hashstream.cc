/*******************************************************************************
 * libretroshare/src/crypto: hashstream.cc                                     *
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
#include "hashstream.h"

#include <assert.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <retroshare/rsids.h>

namespace librs
{
	namespace crypto
	{
	HashStream::HashStream(HashType t)
	{
		assert(t == SHA1) ;

		mdctx = EVP_MD_CTX_create();
		EVP_DigestInit_ex(mdctx,EVP_sha1(),NULL);
	}
	HashStream::~HashStream()
	{
		if(mdctx)
			EVP_MD_CTX_destroy(mdctx) ;
	}

	Sha1CheckSum HashStream::hash()
	{
		uint8_t h[EVP_MAX_MD_SIZE] ;
		unsigned int len;
		EVP_DigestFinal_ex(mdctx,h,&len) ;

		EVP_MD_CTX_destroy(mdctx) ;
		mdctx=NULL ;

		return Sha1CheckSum(h);
	}
    template<>
    HashStream& operator<<(HashStream& u,const std::pair<unsigned char *,uint32_t>& p)
    {
        EVP_DigestUpdate(u.mdctx,p.first,p.second) ;
        return u;
    }
	template<>
	HashStream& operator<<(HashStream& u,const std::string& s)
	{
		EVP_DigestUpdate(u.mdctx,s.c_str(),s.length()) ;
		return u;
	}
	template<>
	HashStream& operator<<(HashStream& u,const uint64_t& n)
	{
        unsigned char mem[8] ;
        uint64_t s(n);
        for(int i=0;i<8;++i)
        {
            mem[i] = (uint8_t)s;
            s <<= 8 ;
        }

		EVP_DigestUpdate(u.mdctx,mem,8);
		return u;
	}
	template<>
	HashStream& operator<<(HashStream& u,const uint32_t& n)
	{
        unsigned char mem[4] ;
        uint64_t s(n);
        for(int i=0;i<4;++i)
        {
            mem[i] = (uint8_t)s;
            s <<= 8 ;
        }

		EVP_DigestUpdate(u.mdctx,mem,4);
		return u;
	}
	template<>
	HashStream& operator<<(HashStream& u,const uint8_t& n)
	{
		EVP_DigestUpdate(u.mdctx,&n,1);
		return u;
	}

	}
}
