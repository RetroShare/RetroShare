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
