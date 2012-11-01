#include <stdint.h>

template<int n> class t_RsFlags32
{
	public:
		t_RsFlags32() {}

		t_RsFlags32(uint32_t N) : _bits(N) {}
		operator uint32_t() const { return _bits ; }

		t_RsFlags32<n> operator| (const t_RsFlags32<n>& f) const { return t_RsFlags32<n>(_bits | f._bits) ; }
		t_RsFlags32<n> operator^ (const t_RsFlags32<n>& f) const { return t_RsFlags32<n>(_bits ^ f._bits) ; }
		t_RsFlags32<n> operator& (const t_RsFlags32<n>& f) const { return t_RsFlags32<n>(_bits & f._bits) ; }

		t_RsFlags32<n> operator|=(const t_RsFlags32<n>& f) { _bits |= f._bits ; return *this ;}
		t_RsFlags32<n> operator^=(const t_RsFlags32<n>& f) { _bits ^= f._bits ; return *this ;}
		t_RsFlags32<n> operator&=(const t_RsFlags32<n>& f) { _bits &= f._bits ; return *this ;}

		t_RsFlags32<n> operator~() const { return t_RsFlags32<n>(~_bits) ; }
	private:
		uint32_t _bits ;
};

#define TRANSFER_INFO_FLAGS_TAG 	0x8133ea
#define FILE_STORAGE_FLAGS_TAG 	0x184738

typedef t_RsFlags32<TRANSFER_INFO_FLAGS_TAG> TransferInfoFlags ;
typedef t_RsFlags32<FILE_STORAGE_FLAGS_TAG > FileStorageFlags ;			// this makes it a uint32_t class incompatible with other flag class

