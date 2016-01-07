#ifndef RSAUTOSERIALIZE_H
#define RSAUTOSERIALIZE_H
/*
 * This file is part of libretroshare.
 *
 * Copyright (C) 2016 Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * libretroshare is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libretroshare is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with libretroshare.  If not, see <http://www.gnu.org/licenses/>.
 */

/* DISCLAIMER!
 *
 * This file has exceptionally long lines that are ugly, this may seems a
 * violation of libretroshare coding style but is not because those lines became
 * even uglier if breaked.
 *
 * For more information about libretroshare coding style see:
 * https://github.com/RetroShare/RetroShare/wiki/Coding
 */

/**
 * @file rsautoserialize.h
 * @brief RetroShare automatic [de]serialization.
 * In RetroShare data is trasmitted between nodes in form of @see RsItem,
 * historically each discendant type of RsItem and his members had his
 * [de]serialization code written by hand, this has generated code and work
 * duplication, subtle bugs and other undesired side effects, the code in this
 * file is aimed to radically change that situation altought some compromise has
 * been necessary (@see RsSerializer) to avoid breaking retrocompatibility.
 *
 * Make sure to read documentation on this file and in @see rstlvitem.h before
 * taking any action about RetroShare [de]serialization.
 */

#include <stdint.h>
#include <string>
#include <vector>
#include <ctime>

#include "util/rsnet.h"

/**
 * @brief The RsSerializable Interface
 * This interface is supposed to be implemented by serializable classes.
 * To inherit from RsSerializable doesn't imply that the serialzation will
 * follow a specific format.
 * If you are about to implement a new serializable class you should not inherit
 * direclty from RsSerializable as more functionalities you are probably
 * interested in are already provided by more specialize classes such as
 * @see RsAutoSerializable and @see RsAutoTlvSerializable.
 */
class RsSerializable
{
public:
	/**
	 * @brief serialize this object to the given buffer
	 * @param data Chunk of memory were to dump the serialized data
	 * @param size Size of memory chunk
	 * @param offset Readed to determine at witch offset start writing data,
	 *        written to inform caller were written data ends, the updated value
	 *        is usually passed by the caller to serialize of another
	 *        RsSerializable so it can write on the same chunk of memory just
	 *        after where this RsSerializable has been serialized.
	 * @return true if serialization successed, false otherwise
	 */
	virtual bool serialize(uint8_t data[], uint32_t size,
	                       uint32_t &offset) const = 0;

	/**
	 * @brief Populate members by deserializing the given buffer
	 * @param data Readonly data to read to generate the deserialized object
	 * @param size Size of memory chunk
	 * @param offset Readed to determine at witch offset start reading data,
	 *        written to inform caller were read ended, the updated value is
	 *        usually passed by the caller to deserialize of another
	 *        RsSerializable so it can read the same chunk of memory just after
	 *        where the deserialization of this RsSerializable has ended.
	 * @return true if deserialization succeded false otherwise
	 */
	virtual bool deserialize(const uint8_t data[], uint32_t size,
	                         uint32_t &offset) = 0;

	/**
	 * @brief serializedSize calculate the size in bytes of minimum buffer
	 * needed to serialize this element
	 * @return the calculated size
	 */
	virtual uint32_t serialSize() const = 0;
};

class RsAutoSerializable;

/**
 * After long discussion we have consensuated to provide retrocompatible
 * [de]serialization of basic type also if not wrapped into an RsSerializable
 * manly to reduce the impact on pre-existent code and make it easier to port it
 * to the new serialization system (based on RsAutoSerializable), for this only
 * purpose we provide non wrapping serializers with static methods.
 * This has been written for RsAutoSerializable internal usage you should not
 * implement more RsSerializer, neither you should inherit direclty from
 * RsSerializer, or use it inside your own RsSerializable.
 * If you have got here you are probably interested in what already provided by
 * more specialized classes such as @see RsAutoTlvSerializable if you need to
 * [de]serialize a basic type that is not already provided as an
 * @see RsSerializer you should write a discendant of @see RsAutoTlvSerializable
 * for that purpose.
 */
template<typename T> class RsSerializer {};


/// Template to generate RsSerializer for standard integral types
template<typename N, uint32_t SIZE> class t_SerializerNType
{
public:
	static bool serialize(uint8_t data[], uint32_t size, uint32_t &offset,
	                      const RsAutoSerializable * autoObj,
	                      const int RsAutoSerializable::* member)
	{
		if (size <= offset || size - offset < SIZE) return false;
		const N * absPtr = (const N *) &(autoObj->*member);
		N tmp = hton<N>(*absPtr);
		memcpy(data+offset, &tmp, SIZE);
		offset += SIZE;
		return true;
	}
	static bool deserialize(const uint8_t data[], uint32_t size,
	                        uint32_t &offset, RsAutoSerializable * autoObj,
	                        int RsAutoSerializable::* member)
	{
		if (size <= offset || size - offset < SIZE) return false;
		N * absPtr = (N *) &(autoObj->*member);
		memcpy(absPtr, data+offset, SIZE);
		*absPtr = ntoh<N>(*absPtr);
		offset += SIZE;
		return true;
	}
	static inline uint32_t serializedSize() { return SIZE; }
};

template<> class RsSerializer<uint8_t> : public t_SerializerNType<uint8_t, 1>{};
template<> class RsSerializer<uint16_t> : public t_SerializerNType<uint16_t, 2>{};
template<> class RsSerializer<uint32_t> : public t_SerializerNType<uint32_t, 4>{};
template<> class RsSerializer<uint64_t> : public t_SerializerNType<uint64_t, 8>{};
template<> class RsSerializer<time_t> : public RsSerializer<uint64_t> {};

/// Serializer for <b>non negative</b> float
template<> class RsSerializer<float>
{
public:
	static bool serialize(uint8_t data[], uint32_t size, uint32_t &offset,
	                      const RsAutoSerializable * autoObj,
	                      const int RsAutoSerializable::* member)
	{
		if ( !data || size <= offset ||
		     size - offset < RsSerializer<float>::serializedSize() )
			return false;

		const float * absPtr = (const float *) &(autoObj->*member);
		if(*absPtr < 0.0f)
		{
			std::cerr << "(EE) Cannot serialise invalid negative float value "
			          << *absPtr << " in " << __PRETTY_FUNCTION__ << std::endl;
			return false;
		}

		/* This serialisation is quite accurate. The max relative error is approx.
		 * 0.01% and most of the time less than 1e-05% The error is well distributed
		 * over numbers also. */
		uint32_t n;
		if(*absPtr < 1e-7) n = (~(uint32_t)0);
		else n = ((uint32_t)( (1.0f/(1.0f+*absPtr) * (~(uint32_t)0))));
		n = hton<uint32_t>(n);
		memcpy(data+offset, &n, RsSerializer<float>::serializedSize());
		offset += RsSerializer<float>::serializedSize();
		return true;
	}
	static bool deserialize(const uint8_t data[], uint32_t size,
	                        uint32_t &offset, RsAutoSerializable * autoObj,
	                        int RsAutoSerializable::* member)
	{
		if ( !data || size <= offset ||
		     size - offset < RsSerializer<float>::serializedSize() )
			return false;

		uint32_t n;
		memcpy(&n, data+offset, RsSerializer<float>::serializedSize());
		n = ntoh<uint32_t>(n);
		float * absPtr = (float *) &(autoObj->*member);
		*absPtr = 1.0f/ ( n/(float)(~(uint32_t)0)) - 1.0f;
		return true;
	}
	static inline uint32_t serializedSize() { return 4; }
};

/// Serializer for std::string
template<> class RsSerializer<std::string>
{
public:
	static bool serialize(uint8_t data[], uint32_t size, uint32_t &offset,
	                      const RsAutoSerializable * autoObj,
	                      const int RsAutoSerializable::* member)
	{
		if ( !data || size <= offset ||
		     size - offset < RsSerializer<std::string>::serializedSize(autoObj, member) )
			return false;

		const std::string * absPtr = (const std::string *) &(autoObj->*member);
		uint32_t charsLen = absPtr->length();
		uint32_t netLen = hton<uint32_t>(charsLen);
		memcpy(data+offset, &netLen, 4); offset += 4;
		memcpy(data+offset, absPtr->c_str(), charsLen); offset += charsLen;
		return true;
	}
	static bool deserialize(const uint8_t data[], uint32_t size,
	                        uint32_t &offset, RsAutoSerializable * autoObj,
	                        int RsAutoSerializable::* member)
	{
		if ( !data || size <= offset || size - offset < 4 ) return false;
		uint32_t charsLen;
		memcpy(&charsLen, data+offset, 4); offset += 4;
		charsLen = ntoh<uint32_t>(charsLen);

		if ( size <= offset || size - offset < charsLen ) return false;
		std::string * absPtr = (std::string *) &(autoObj->*member);
		absPtr->clear();
		absPtr->insert(0, (char*)data+offset, charsLen);
		offset += charsLen;
		return true;
	}
	static inline uint32_t serializedSize(const RsAutoSerializable * autoObj,
	                                      const int RsAutoSerializable::* member)
	{
		const std::string * absPtr = (const std::string *) &(autoObj->*member);
		return absPtr->length() + 4;
	}
};

/**
 * @brief Base class for RetroShare automatics [de]serializable classes
 * Provide auto-[de]serialization for members of RsSerializable discendant types
 * and for some standard C++ types.
 * Members that wanna be auto-[de]serialized must be registered in constructor.
 */
class RsAutoSerializable : public RsSerializable
{
public:
	uint32_t serialSize() const
	{
		uint32_t acc = 0;
		std::vector<std::pair<ObjectType,int RsAutoSerializable::*> >::const_iterator it;
		for(it = mToSerialise.begin(); it != mToSerialise.end(); ++it)
		{
			switch((*it).first)
			{
			case OBJECT_TYPE_UINT8:  acc += RsSerializer<uint8_t>::serializedSize(); break;
			case OBJECT_TYPE_UINT16: acc += RsSerializer<uint16_t>::serializedSize(); break;
			case OBJECT_TYPE_UINT32: acc += RsSerializer<uint32_t>::serializedSize(); break;
			case OBJECT_TYPE_UINT64: acc += RsSerializer<uint64_t>::serializedSize(); break;
			case OBJECT_TYPE_UFLOAT32:
			case OBJECT_TYPE_TIME:
			case OBJECT_TYPE_STR: acc += RsSerializer<std::string>::serializedSize(this, (*it).second); break;
			/* For RsSerializable discendant objects. We need to cast to
			 * RsSerializable and call the pure virtual serialize(...) method
			 * and C++ dynamic dispatch will take care of calling serialize(...)
			 * for the original object class */
			case OBJECT_TYPE_SRZ: acc += (this->*reinterpret_cast<RsSerializable RsAutoSerializable::*>((*it).second)).serialSize(); break;
			}
		}
		return acc;
	}
	bool serialize(uint8_t data[], uint32_t size, uint32_t &offset) const
	{
		if ( !data || size <= offset || size - offset < serialSize() )
			return false;

		bool ok = true;
		std::vector<std::pair<ObjectType,int RsAutoSerializable::*> >::const_iterator it;
		for(it = mToSerialise.begin(); ok && it != mToSerialise.end(); ++it)
		{
			switch((*it).first)
			{
			case OBJECT_TYPE_UINT8:  ok &= RsSerializer<uint8_t>::serialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_UINT16: ok &= RsSerializer<uint16_t>::serialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_UINT32: ok &= RsSerializer<uint32_t>::serialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_UINT64: ok &= RsSerializer<uint64_t>::serialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_UFLOAT32:
			case OBJECT_TYPE_TIME:
			case OBJECT_TYPE_STR: ok &= RsSerializer<std::string>::serialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_SRZ: ok &= (this->*reinterpret_cast<RsSerializable RsAutoSerializable::*>((*it).second)).serialize(data, size, offset); break;
			}
		}
		return ok;
	}
	bool deserialize(const uint8_t data[], uint32_t size, uint32_t &offset)
	{
		if ( !data || size <= offset ) return false;
		/* Can't check if there is enough space because serialSize() may return
		 * an unvalid value here as empty variable size members can be presents
		 * this is not unsafe as sub member::deserialize(...) will check it. */

		bool ok = true;
		std::vector<std::pair<ObjectType,int RsAutoSerializable::*> >::const_iterator it;
		for(it = mToSerialise.begin(); ok && it != mToSerialise.end(); ++it)
		{
			switch((*it).first)
			{
			case OBJECT_TYPE_UINT8:  ok &= RsSerializer<uint8_t>::deserialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_UINT16: ok &= RsSerializer<uint16_t>::deserialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_UINT32: ok &= RsSerializer<uint32_t>::deserialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_UINT64: ok &= RsSerializer<uint64_t>::deserialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_UFLOAT32: ok &= RsSerializer<float>::deserialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_TIME: ok &= RsSerializer<time_t>::deserialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_STR: ok &= RsSerializer<std::string>::deserialize(data, size, offset, this, (*it).second); break;
			case OBJECT_TYPE_SRZ: ok &= (this->*reinterpret_cast<RsSerializable RsAutoSerializable::*>((*it).second)).deserialize(data, size, offset); break;
			}
		}
		return ok;
	}

protected:
	/**
	 * Derivative classes must use this function to register members for
	 * automatic [de]serialization in constructor.
	 */
	template<class A, class T> void registerSerializable(T A::* member) { mToSerialise.push_back( std::make_pair(objectType<T>(),reinterpret_cast<int RsAutoSerializable::*>(member))); }

private:
	/**
	 * Used internally to keep track of type of registered for [de]serialization
	 * members.
	 */
	enum ObjectType
	{
		OBJECT_TYPE_UINT8    = 0x01, // Integral C++ base types
		OBJECT_TYPE_UINT16   = 0x02,
		OBJECT_TYPE_UINT32   = 0x03,
		OBJECT_TYPE_UINT64   = 0x04,
		OBJECT_TYPE_UFLOAT32 = 0x05, // Unsigned Float
		OBJECT_TYPE_TIME     = 0x06, // time_t
		OBJECT_TYPE_STR      = 0x20, // C++ std::string
		OBJECT_TYPE_SRZ      = 0x30  // RsSerializable and derivatives
	};

	/// Map C++ types into corresponding ObjectType
	template<class T> ObjectType objectType();

	/**
	 * Keep track of members and associated ObjectType registered for automatic
	 * [de]serialization.
	 */
	std::vector<std::pair<ObjectType,int RsAutoSerializable::*> > mToSerialise;
};

template<> inline RsAutoSerializable::ObjectType RsAutoSerializable::objectType<uint8_t>()        { return OBJECT_TYPE_UINT8; }
template<> inline RsAutoSerializable::ObjectType RsAutoSerializable::objectType<uint16_t>()       { return OBJECT_TYPE_UINT16; }
template<> inline RsAutoSerializable::ObjectType RsAutoSerializable::objectType<uint32_t>()       { return OBJECT_TYPE_UINT32; }
template<> inline RsAutoSerializable::ObjectType RsAutoSerializable::objectType<uint64_t>()       { return OBJECT_TYPE_UINT64; }
template<> inline RsAutoSerializable::ObjectType RsAutoSerializable::objectType<float>()          { return OBJECT_TYPE_UFLOAT32; }
template<> inline RsAutoSerializable::ObjectType RsAutoSerializable::objectType<time_t>()         { return OBJECT_TYPE_TIME; }
template<> inline RsAutoSerializable::ObjectType RsAutoSerializable::objectType<std::string>()    { return OBJECT_TYPE_STR; }
template<> inline RsAutoSerializable::ObjectType RsAutoSerializable::objectType<RsSerializable>() { return OBJECT_TYPE_SRZ; }

#endif // RSAUTOSERIALIZE_H
