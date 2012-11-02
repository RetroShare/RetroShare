#pragma once

#include <stdint.h>

// This class provides a representation for flags that can be combined with bitwise
// operations. However, because the class is templated with an id, it's not possible to 
// mixup flags belonging to different classes. This avoids many bugs due to confusion of flags types
// that occur when all flags are uint32_t values.
//
// To use this class, define an ID that is different than other flags classes, and do a typedef:
//
//    #define TRANSFER_INFO_FLAGS_TAG 	0x8133ea
//    typedef t_RsFlags32<TRANSFER_INFO_FLAGS_TAG> TransferInfoFlags ;
//
// Implementation details:
// 	- we cannot have at the same time a implicit contructor from uint32_t and a bool operator, otherwise c++
// 	 mixes up operators and transforms flags into booleans before combining them further.
//
// 	 So I decided to have:
// 	 	- an explicit constructor from uint32_t
// 	 	- an implicit bool operator, that allows test like if(flags & FLAGS_VALUE)
//
template<int n> class t_RsFlags32
{
	public:
		inline t_RsFlags32() { _bits=0; }
		inline explicit t_RsFlags32(uint32_t N) : _bits(N) {}					// allows initialization from a set of uint32_t

		inline t_RsFlags32<n> operator| (const t_RsFlags32<n>& f) const { return t_RsFlags32<n>(_bits | f._bits) ; }
		inline t_RsFlags32<n> operator^ (const t_RsFlags32<n>& f) const { return t_RsFlags32<n>(_bits ^ f._bits) ; }

		inline bool operator!=(const t_RsFlags32<n>& f) const { return _bits != f._bits ; }
		inline bool operator& (const t_RsFlags32<n>& f) const { return (_bits & f._bits)>0 ; }

		inline t_RsFlags32<n> operator|=(const t_RsFlags32<n>& f) { _bits |= f._bits ; return *this ;}
		inline t_RsFlags32<n> operator^=(const t_RsFlags32<n>& f) { _bits ^= f._bits ; return *this ;}
		inline t_RsFlags32<n> operator&=(const t_RsFlags32<n>& f) { _bits &= f._bits ; return *this ;}

		inline t_RsFlags32<n> operator~() const { return t_RsFlags32<n>(~_bits) ; }

		//inline explicit operator bool() 	const { return _bits>0; }
		inline uint32_t toUInt32() const { return _bits ; }

		void clear() { _bits = 0 ; }

		friend std::ostream& operator<<(std::ostream& o,const t_RsFlags32<n>& f) 	// friendly print with 0 and I
		{
			for(int i=31;i>=0;--i) { o << ( (f._bits&(1<<i))?"I":"0") ; if(i%8==0) o << " " ; }
			return o ;
		}
	private:
		uint32_t _bits ;
};

#define FLAGS_TAG_FILE_SEARCH 	0xf29ba5
#define FLAGS_TAG_PERMISSION  	0x8133ea
#define FLAGS_TAG_TRANSFER_REQS 	0x4228af
#define FLAGS_TAG_FILE_STORAGE 	0x184738

typedef t_RsFlags32<FLAGS_TAG_PERMISSION> 	FilePermissionFlags ;
typedef t_RsFlags32<FLAGS_TAG_TRANSFER_REQS> TransferRequestFlags ;
typedef t_RsFlags32<FLAGS_TAG_FILE_STORAGE > FileStorageFlags ;			// this makes it a uint32_t class incompatible with other flag class
typedef t_RsFlags32<FLAGS_TAG_FILE_SEARCH  > FileSearchFlags ;			// this makes it a uint32_t class incompatible with other flag class

