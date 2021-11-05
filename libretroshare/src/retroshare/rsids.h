/*******************************************************************************
 * libretroshare/src/retroshare: rsids.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2013  Cyril Soler <csoler@users.sourceforge.net>              *
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

#include <stdexcept>
#include <string>
#include <iostream>
#include <ostream>
#include <string.h>
#include <cstdint>
#include <vector>
#include <list>
#include <set>
#include <memory>

#include "util/rsdebug.h"
#include "util/rsrandom.h"
#include "util/stacktrace.h"

/**
 * RsGenericIdType values might be random, but must be different, in order to
 * make the various IDs incompatible with each other.
 */
enum class RsGenericIdType
{
	SSL,
	PGP_ID,
	SHA1,
	PGP_FINGERPRINT,
	GXS_GROUP,
	GXS_ID,
	GXS_MSG,
	GXS_CIRCLE,
	GROUTER,
	GXS_TUNNEL,
	DISTANT_CHAT,
	NODE_GROUP,
	SHA256,
	BIAS_20_BYTES
};

/**
 * This class aims at defining a generic ID type that is a list of bytes. It
 * can be converted into a hexadecial string for printing, mainly) or for
 * compatibility with old methods.
 *
 * To use this class, derive your own ID type from it.
 * @see RsPpgFingerprint as an example.
 *
 * Take care to define and use a different @see RsGenericIdType for each ne type
 * of ID you create, to avoid implicit conversion between subtypes, and
 * therefore accidental ID mixup is impossible.
 *
 * ID Types with different lengths are not convertible even using explicit
 * constructor and compilation would fail if that is attempted.
 *
 * Warning: never store references to a t_RsGenericIdType accross threads, since
 * the cached string convertion is not thread safe.
 */
template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
struct t_RsGenericIdType
{
	using Id_t = t_RsGenericIdType<ID_SIZE_IN_BYTES,UPPER_CASE,UNIQUE_IDENTIFIER>;
	using std_list   = std::list<Id_t>;
	using std_vector = std::vector<Id_t>;
	using std_set    = std::set<Id_t>;

	/// by default, ids are set to null()
	t_RsGenericIdType() { memset(bytes, 0, ID_SIZE_IN_BYTES); }

	/// Explicit constructor from a hexadecimal string
	explicit t_RsGenericIdType(const std::string& hex_string);

	/**
	 * @brief Construct from a buffer of at least the size of SIZE_IN_BYTES
	 * This is dangerous if used without being absolutely sure of buffer size,
	 * nothing prevent a buffer of wrong size being passed at runtime!
	 * @param[in] buff pointer to the buffer
	 * @return empty id on failure, an id initialized from the bytes in the
	 *	buffer
	 */
	static Id_t fromBufferUnsafe(const uint8_t* buff)
	{
		Id_t ret;

		if(!buff)
		{
			RsErr() << __PRETTY_FUNCTION__ << " invalid paramethers buff: "
			        << buff << std::endl;
			print_stacktrace();
			return ret;
		}

		memmove(ret.bytes, buff, SIZE_IN_BYTES);
		return ret;
	}

	/**
	 * Explicit constructor from a different type but with same size.
	 *
	 * This is used for conversions such as
	 * 	GroupId -> CircleId
	 * 	GroupId -> GxsId
	 */
	template<bool UPPER_CASE2, RsGenericIdType UNIQUE_IDENTIFIER2>
	explicit t_RsGenericIdType(
	        const t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE2, UNIQUE_IDENTIFIER2>&
	        id )
	{ memmove(bytes, id.toByteArray(), ID_SIZE_IN_BYTES); }

	/// Random initialization. Can be useful for testing and to generate new ids.
	static Id_t random()
	{
		Id_t id;
		RsRandom::random_bytes(id.bytes, ID_SIZE_IN_BYTES);
		return id;
	}

	inline void clear() { memset(bytes, 0, SIZE_IN_BYTES); }

	/// Converts to a std::string using cached value.
	const uint8_t* toByteArray() const { return &bytes[0]; }

	static constexpr uint32_t SIZE_IN_BYTES = ID_SIZE_IN_BYTES;

	inline bool operator==(const Id_t& fp) const
	{ return !memcmp(bytes, fp.bytes, ID_SIZE_IN_BYTES); }

	inline bool operator!=(const Id_t& fp) const
	{ return !!memcmp(bytes, fp.bytes, ID_SIZE_IN_BYTES); }

	inline bool operator< (const Id_t& fp) const
	{ return (memcmp(bytes, fp.bytes, ID_SIZE_IN_BYTES) < 0); }

	inline Id_t operator~ () const
	{
		Id_t ret;
		for(uint32_t i=0; i < ID_SIZE_IN_BYTES; ++i)
			ret.bytes[i] = ~bytes[i];
		return ret;
	}

	inline Id_t operator| (const Id_t& fp) const
	{
		Id_t ret;
		for(uint32_t i=0; i < ID_SIZE_IN_BYTES; ++i)
			ret.bytes[i] = bytes[i] | fp.bytes[i];
		return ret;
	}

    inline Id_t operator^ (const Id_t& fp) const
    {
        Id_t ret;
        for(uint32_t i=0; i < ID_SIZE_IN_BYTES; ++i)
            ret.bytes[i] = bytes[i] ^ fp.bytes[i];
        return ret;
    }


	inline bool isNull() const
	{
		for(uint32_t i=0; i < SIZE_IN_BYTES; ++i)
		if(bytes[i] != 0) return false;
		return true;
	}

	friend std::ostream& operator<<(std::ostream& out, const Id_t& id)
	{
		switch (UNIQUE_IDENTIFIER)
		{
		case RsGenericIdType::PGP_FINGERPRINT:
		{
			uint8_t index = 0;
			for(char c : id.toStdString())
			{
				out << c;
				if(++index % 4 == 0 && index < id.SIZE_IN_BYTES*2) out << ' ';
			}
		}
		break;
		default: out << id.toStdString(UPPER_CASE); break;
		}

		return out;
	}

	inline std::string toStdString() const { return toStdString(UPPER_CASE); }

	inline static uint32_t serial_size() { return SIZE_IN_BYTES; }

	bool serialise(void* data,uint32_t pktsize,uint32_t& offset) const
	{
		if(offset + SIZE_IN_BYTES > pktsize) return false;
		memmove( &(reinterpret_cast<uint8_t*>(data))[offset],
		         bytes, SIZE_IN_BYTES );
		offset += SIZE_IN_BYTES;
		return true;
	}

	bool deserialise(const void* data, uint32_t pktsize, uint32_t& offset)
	{
		if(offset + SIZE_IN_BYTES > pktsize) return false;
		memmove( bytes,
		         &(reinterpret_cast<const uint8_t*>(data))[offset],
		         SIZE_IN_BYTES );
		offset += SIZE_IN_BYTES;
		return true;
	}

	/** Explicit constructor from a byte array. The array must have size at
	 * least ID_SIZE_IN_BYTES
	 * @deprecated This is too dangerous!
	 * Nothing prevent a buffer of wrong size being passed at runtime!
	 */
	RS_DEPRECATED_FOR("fromBufferUnsafe(const uint8_t* buff)")
	explicit t_RsGenericIdType(const uint8_t bytes[]);

private:
	std::string toStdString(bool upper_case) const;
	uint8_t bytes[ID_SIZE_IN_BYTES];
};

template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
std::string t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>
::toStdString(bool upper_case) const
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

template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>
::t_RsGenericIdType(const std::string& s)
{
	std::string::size_type n = 0;
	if(s.length() != ID_SIZE_IN_BYTES*2)
	{
		if(!s.empty())
		{
			RsErr() << __PRETTY_FUNCTION__ << " supplied string in constructor "
			        << "has wrong size. Expected ID size=" << ID_SIZE_IN_BYTES*2
			        << " String=\"" << s << "\" = " << s.length() << std::endl;
			print_stacktrace();
		}
		clear();
		return;
	}

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
			{
				RsErr() << __PRETTY_FUNCTION__ << "supplied string is not "
				        << "purely hexadecimal: s=\"" << s << "\"" << std::endl;
				clear();
				return;
			}
		}
	}
}

template<uint32_t ID_SIZE_IN_BYTES, bool UPPER_CASE, RsGenericIdType UNIQUE_IDENTIFIER>
RS_DEPRECATED_FOR("t_RsGenericIdType::fromBuffer(...)")
t_RsGenericIdType<ID_SIZE_IN_BYTES, UPPER_CASE, UNIQUE_IDENTIFIER>::
t_RsGenericIdType(const uint8_t mem[]) /// @deprecated Too dangerous!
{
	if(mem == nullptr) memset(bytes, 0, ID_SIZE_IN_BYTES);
	else memcpy(bytes, mem, ID_SIZE_IN_BYTES);
}

/**
 * This constants are meant to be used only inside this file.
 * Use @see t_RsGenericIdType::SIZE_IN_BYTES in other places.
 */
namespace _RsIdSize
{
constexpr uint32_t SSL_ID          = 16;   // = CERT_SIGN
constexpr uint32_t CERT_SIGN       = 16;   // = SSL_ID
constexpr uint32_t PGP_ID          =  8;
constexpr uint32_t PGP_FINGERPRINT = 20;
constexpr uint32_t SHA1            = 20;
constexpr uint32_t SHA256          = 32;
}

using RsPeerId          = t_RsGenericIdType<_RsIdSize::SSL_ID         , false, RsGenericIdType::SSL            >;
using RsPgpId           = t_RsGenericIdType<_RsIdSize::PGP_ID         , true,  RsGenericIdType::PGP_ID         >;
using Sha1CheckSum      = t_RsGenericIdType<_RsIdSize::SHA1           , false, RsGenericIdType::SHA1           >;
using Sha256CheckSum    = t_RsGenericIdType<_RsIdSize::SHA256         , false, RsGenericIdType::SHA256         >;
using RsPgpFingerprint  = t_RsGenericIdType<_RsIdSize::PGP_FINGERPRINT, true,  RsGenericIdType::PGP_FINGERPRINT>;
using Bias20Bytes       = t_RsGenericIdType<_RsIdSize::SHA1           , true,  RsGenericIdType::BIAS_20_BYTES  >;
using RsGxsGroupId      = t_RsGenericIdType<_RsIdSize::CERT_SIGN      , false, RsGenericIdType::GXS_GROUP      >;
using RsGxsMessageId    = t_RsGenericIdType<_RsIdSize::SHA1           , false, RsGenericIdType::GXS_MSG        >;
using RsGxsId           = t_RsGenericIdType<_RsIdSize::CERT_SIGN      , false, RsGenericIdType::GXS_ID         >;
using RsGxsCircleId     = t_RsGenericIdType<_RsIdSize::CERT_SIGN      , false, RsGenericIdType::GXS_CIRCLE     >;
using RsGxsTunnelId     = t_RsGenericIdType<_RsIdSize::SSL_ID         , false, RsGenericIdType::GXS_TUNNEL     >;
using DistantChatPeerId = t_RsGenericIdType<_RsIdSize::SSL_ID         , false, RsGenericIdType::DISTANT_CHAT   >;
using RsNodeGroupId     = t_RsGenericIdType<_RsIdSize::CERT_SIGN      , false, RsGenericIdType::NODE_GROUP     >;

/// @deprecated Ugly name kept temporarly only because it is used in many places
using PGPFingerprintType RS_DEPRECATED_FOR(RsPpgFingerprint) = RsPgpFingerprint;
