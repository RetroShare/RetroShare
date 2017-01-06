#pragma once

#include <openssl/evp.h>
#include <util/rsdir.h>

namespace librs
{
	namespace crypto
	{
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
