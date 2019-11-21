/*******************************************************************************
 * libretroshare/src/retroshare: rsflags.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2012-2019 by Retroshare Team <contact@retroshare.cc>          *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include <type_traits>
#include <ostream>

/** Check if given type is a scoped enum */
template<typename E>
using rs_is_scoped_enum = std::integral_constant< bool,
    std::is_enum<E>::value && !std::is_convertible<E, int>::value >;

/**
 * @brief Register enum class as flags type
 * To use this macro define a scoped enum with your flag values, then register
 * it as flags type passing it as parameter of this macro.
 * The result will be type safe flags, that cannot be mixed up with flag of a
 * different type, but that are very comfortable to operate like plain old
 * integers.
 * This macro support flag fields of different lenght depending on what
 * underlining type (usually from uint8_t up to uint64_t) has been declared for
 * the enum class.
 * If you plan to serialize those flags it is important to specify the
 * underlining type of the enum otherwise different compilers may serialize a
 * flag variable with different lenght, potentially causing interoperability
 * issues between differents builds.
 * Usage example:
@code{.cpp}
enum class RsGrouterItemFlags : uint32_t
{
	NONE               = 0x0,
	ENCRYPTED          = 0x1,
	SERVICE_UNKNOWN    = 0x2
};
RS_REGISTER_ENUM_FLAGS_TYPE(RsGrouterItemFlags)
@endcode
 */
#define RS_REGISTER_ENUM_FLAGS_TYPE(eft) \
template<> struct Rs__BitFlagsOps<eft> \
{ \
	static_assert( std::is_enum<eft>::value, \
	               "Are you trying to register a non-enum type as flags?" ); \
	static_assert( rs_is_scoped_enum<eft>::value, \
	               "Are you trying to register an unscoped enum as flags?" ); \
	static constexpr bool enabled = true; \
};

// By defaults types are not valid flags, so bit flags operators are disabled
template<typename> struct Rs__BitFlagsOps
{ static constexpr bool enabled = false; };

template<typename EFT>
typename std::enable_if<Rs__BitFlagsOps<EFT>::enabled, EFT>::type
/*EFT*/ operator &(EFT lhs, EFT rhs)
{
	using u_t = typename std::underlying_type<EFT>::type;
	return static_cast<EFT>(static_cast<u_t>(lhs) & static_cast<u_t>(rhs));
}

template<typename EFT>
typename std::enable_if<Rs__BitFlagsOps<EFT>::enabled, EFT>::type
/*EFT*/ operator &=(EFT& lhs, EFT rhs) { lhs = lhs & rhs; return lhs; }

template<typename EFT>
typename std::enable_if<Rs__BitFlagsOps<EFT>::enabled, EFT>::type
/*EFT*/ operator |(EFT lhs, EFT rhs)
{
	using u_t = typename std::underlying_type<EFT>::type;
	return static_cast<EFT>(static_cast<u_t>(lhs) | static_cast<u_t>(rhs));
}

template<typename EFT>
typename std::enable_if<Rs__BitFlagsOps<EFT>::enabled, EFT>::type
/*EFT*/ operator |=(EFT& lhs, EFT rhs) { lhs = lhs | rhs; return lhs; }


template<typename EFT>
typename std::enable_if<Rs__BitFlagsOps<EFT>::enabled, EFT>::type
/*EFT*/ operator ^(EFT lhs, EFT rhs)
{
	using u_t = typename std::underlying_type<EFT>::type;
	return static_cast<EFT>(static_cast<u_t>(lhs) ^ static_cast<u_t>(rhs));
}

template<typename EFT>
typename std::enable_if<Rs__BitFlagsOps<EFT>::enabled, EFT>::type
/*EFT*/ operator ^=(EFT& lhs, EFT rhs) { lhs = lhs ^ rhs; return lhs; }

template<typename EFT>
typename std::enable_if<Rs__BitFlagsOps<EFT>::enabled, EFT>::type
operator ~(EFT val)
{
	using u_t = typename std::underlying_type<EFT>::type;
	return static_cast<EFT>(~static_cast<u_t>(val));
}

template<typename EFT>
typename std::enable_if<Rs__BitFlagsOps<EFT>::enabled, bool>::type
operator !(EFT val)
{
	using u_t = typename std::underlying_type<EFT>::type;
	return static_cast<u_t>(val) == 0;
}

/// Nicely print flags bits as 1 and 0
template<typename EFT>
typename std::enable_if<Rs__BitFlagsOps<EFT>::enabled, std::ostream>::type&
operator <<(std::ostream& stream, EFT flags)
{
	using u_t = typename std::underlying_type<EFT>::type;

	for(int i = sizeof(u_t); i>=0; --i)
	{
		stream << (flags & ( 1 << i ) ? "1" : "0");
		if( i % 8 == 0 ) stream << " ";
	}
	return stream;
}

#include <cstdint>
#include "util/rsdeprecate.h"


/**
 * @deprecated t_RsFlags32 has been deprecated because the newer
 * @see RS_REGISTER_ENUM_FLAGS_TYPE provide more convenient flags facilities.
 *
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
*/
template<int n> class RS_DEPRECATED_FOR(RS_REGISTER_ENUM_FLAGS_TYPE) t_RsFlags32
{
	public:
	    inline t_RsFlags32() : _bits(0) {}
		inline explicit t_RsFlags32(uint32_t N) : _bits(N) {}					// allows initialization from a set of uint32_t

		inline t_RsFlags32<n> operator| (const t_RsFlags32<n>& f) const { return t_RsFlags32<n>(_bits | f._bits) ; }
		inline t_RsFlags32<n> operator^ (const t_RsFlags32<n>& f) const { return t_RsFlags32<n>(_bits ^ f._bits) ; }
		inline t_RsFlags32<n> operator* (const t_RsFlags32<n>& f) const { return t_RsFlags32<n>(_bits & f._bits) ; }

		inline bool operator!=(const t_RsFlags32<n>& f) const { return _bits != f._bits ; }
		inline bool operator==(const t_RsFlags32<n>& f) const { return _bits == f._bits ; }
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
			for(int i=31;i>=0;--i) {
				std::string res = f._bits&(1<<i)?"I":"0" ;
				std::string blank = " " ;
				o << res ;
				if(i%8==0) o << blank ;
			}
			return o ;
		}
	private:
		uint32_t _bits ;
};

#define FLAGS_TAG_TRANSFER_REQS 0x4228af
#define FLAGS_TAG_FILE_STORAGE 	0x184738
#define FLAGS_TAG_FILE_SEARCH 	0xf29ba5
#define FLAGS_TAG_SERVICE_PERM 	0x380912
#define FLAGS_TAG_SERVICE_CHAT 	0x839042
#define FLAGS_TAG_SERIALIZER   	0xa0338d

// Flags for requesting transfers, ask for turtle, cache, speed, etc.
//
typedef t_RsFlags32<FLAGS_TAG_TRANSFER_REQS> TransferRequestFlags ;

// Flags for file storage. Mainly permissions like BROWSABLE/NETWORK_WIDE for groups and peers.
//
typedef t_RsFlags32<FLAGS_TAG_FILE_STORAGE > FileStorageFlags ;			

// Flags for searching in files that could be local, downloads, remote, etc.
//
typedef t_RsFlags32<FLAGS_TAG_FILE_SEARCH  > FileSearchFlags ;			

// Service permissions. Will allow each user to use or not use each service.
//
typedef t_RsFlags32<FLAGS_TAG_SERVICE_PERM > ServicePermissionFlags ;			

// Flags for chat lobbies
//
typedef t_RsFlags32<FLAGS_TAG_SERVICE_CHAT > ChatLobbyFlags ;			

// Flags for serializer
//
typedef t_RsFlags32<FLAGS_TAG_SERIALIZER > SerializationFlags ;

