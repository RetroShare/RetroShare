// This class aims at defining a generic ID type that is a list of bytes. It
// can be converted into a hexadecial string for printing, mainly) or for
// compatibility with old methods.
//
// To use this class, derive your own ID type from it. Examples include:
//
// 	class PGPIdType: public t_RsGenericIdType<8> 
// 	{
// 		[..]
// 	};
//
// 	class PGPFingerprintType: public t_RsGenericIdType<20> 
// 	{
// 		[..]
// 	};
//
// With this, there is no implicit conversion between subtypes, and therefore ID mixup 
// is impossible.
//
// A simpler way to make ID types is to 
// 	typedef t_RsGenericIdType<MySize> MyType ;
//
// ID Types with different lengths will be incompatible on compilation.
//
// Warning: never store references to a t_RsGenericIdType accross threads, since the 
// 			cached string convertion is not thread safe.
//

#pragma once

#include <stdexcept>
#include <string>
#include <stdint.h>
#include <util/rsrandom.h>

template<uint32_t ID_SIZE_IN_BYTES> class t_RsGenericIdType
{
	public:
		t_RsGenericIdType() { memset(bytes,0,ID_SIZE_IN_BYTES) ;}
		virtual ~t_RsGenericIdType() {}

		// Explicit constructor from a hexadecimal string
		//
		explicit t_RsGenericIdType(const std::string& hex_string) ;

		// Explicit constructor from a byte array. The array should have size at least ID_SIZE_IN_BYTES
		//
		explicit t_RsGenericIdType(const unsigned char bytes[]) ;

		// Random initialization. Can be useful for testing.
		//
		static t_RsGenericIdType<ID_SIZE_IN_BYTES> random() 
		{
			t_RsGenericIdType<ID_SIZE_IN_BYTES> id ;

			for(uint32_t i=0;i<ID_SIZE_IN_BYTES;++i)
				id.bytes[i] = RSRandom::random_u32() & 0xff ;

			return id ;
		}

		// Converts to a std::string using cached value. 
		//
		std::string toStdString(bool upper_case = true) const ;
		const unsigned char *toByteArray() const { return &bytes[0] ; }
		static const uint32_t SIZE_IN_BYTES = ID_SIZE_IN_BYTES ;

		bool operator==(const t_RsGenericIdType<ID_SIZE_IN_BYTES>& fp) const
		{
			for(uint32_t i=0;i<ID_SIZE_IN_BYTES;++i)
				if(fp.bytes[i] != bytes[i])
					return false ;
			return true ;
		}
		bool operator!=(const t_RsGenericIdType<ID_SIZE_IN_BYTES>& fp) const
		{
			return !operator==(fp) ;
		}

		bool operator<(const t_RsGenericIdType<ID_SIZE_IN_BYTES>& fp) const
		{
			for(uint32_t i=0;i<ID_SIZE_IN_BYTES;++i)
				if(fp.bytes[i] != bytes[i])
					return (bytes[i] < fp.bytes[i]);
			return false;
		}

	private:
		unsigned char bytes[ID_SIZE_IN_BYTES] ;
};

template<uint32_t ID_SIZE_IN_BYTES> std::string t_RsGenericIdType<ID_SIZE_IN_BYTES>::toStdString(bool upper_case) const
{
	static const char outh[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' } ;
	static const char outl[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' } ;

	std::string res(ID_SIZE_IN_BYTES*2,' ') ;

	for(uint32_t j = 0; j < ID_SIZE_IN_BYTES; j++)
		if(upper_case)
		{
			res[2*j  ] = outh[ (bytes[j]>>4) ] ;
			res[2*j+1] = outh[ bytes[j] & 0xf ] ;
		}
		else
		{
			res[2*j  ] = outl[ (bytes[j]>>4) ] ;
			res[2*j+1] = outl[ bytes[j] & 0xf ] ;
		}

	return res ;
}

template<uint32_t ID_SIZE_IN_BYTES> t_RsGenericIdType<ID_SIZE_IN_BYTES>::t_RsGenericIdType(const std::string& s) 
{
	int n=0;
	if(s.length() != ID_SIZE_IN_BYTES*2)
		throw std::runtime_error("t_RsGenericIdType<>::t_RsGenericIdType(std::string&): supplied string in constructor has wrong size.") ;

	for(uint32_t i = 0; i < ID_SIZE_IN_BYTES; ++i)
	{
		bytes[i] = 0 ;

		for(int k=0;k<2;++k)
		{
			char b = s[n++] ;

			if(b >= 'A' && b <= 'F')
				bytes[i] += (b-'A'+10) << 4*(1-k) ;
			else if(b >= 'a' && b <= 'f')
				bytes[i] += (b-'a'+10) << 4*(1-k) ;
			else if(b >= '0' && b <= '9')
				bytes[i] += (b-'0') << 4*(1-k) ;
			else
				throw std::runtime_error("t_RsGenericIdType<>::t_RsGenericIdType(std::string&): supplied string is not purely hexadecimal") ;
		}
	}
}

template<uint32_t ID_SIZE_IN_BYTES> t_RsGenericIdType<ID_SIZE_IN_BYTES>::t_RsGenericIdType(const unsigned char *mem) 
{
	memcpy(bytes,mem,ID_SIZE_IN_BYTES) ;
}

