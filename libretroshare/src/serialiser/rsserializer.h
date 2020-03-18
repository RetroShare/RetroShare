/*******************************************************************************
 * libretroshare/src/serialiser: rsserializer.h                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2016  Cyril Soler <csoler@users.sourceforge.net>              *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                           Retroshare Serialization code                                   //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Classes
// =======
//
//          RsSerialiser ----------------+ std::map<uint32_t, RsSerialType*>
//
//	        RsSerialType
//               |
//               +----------- RsRawSerializer
//               |
//               +----------- RsGenericSerializer
//                                    |
//                                    +----------- RsConfigSerializer
//                                    |                     |
//                                    |                     +----------- Specific config serializers
//                                    |                     +-----------             ...
//                                    |
//                                    +----------- RsServiceSerializer
//                                                          |
//                                                          +----------- Specific service serializers
//                                                          +-----------             ...
//                                                          +-----------             ...
//
//
// Steps to derive a serializer for a new service:
// ==============================================
//
//	1 - create a serializer class, and overload create_item() to create a new item of your own service for each item type constant:
//
//			class MyServiceSerializer: public RsServiceSerializer
//			{
//				MyServiceSerializer() : RsServiceSerializer(MY_SERVICE_IDENTIFIER) {}
//
//				RsItem *create_item(uint16_t service,uint8_t item_subtype) const          // mind the "const"!
//				{
//					if(service != MY_SERVICE_IDENTIFIER) return NULL ;
//
//					switch(item_subtype)
//					{
//					case MY_ITEM_SUBTYPE_01: return new MyServiceItem();
//					default:
//					   return NULL ;
//					}
//				}
//			}
//
//  2 - create your own items, and overload serial_process in order to define the serialized structure:
//
//			class MyServiceItem: public RsItem
//			{
//				virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
//				{
//					RsTypeSerializer::serial_process<uint32_t> (j,ctx,count,"count") ;					// uint32_t is not really needed here, except for explicitly avoiding int types convertion
//					RsTypeSerializer::serial_process           (j,ctx,update_times,"update_times") ;	// will serialize the map and its content
//					RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,key,"key") ;						// note the explicit call to TlvItem
//					RsTypeSerializer::serial_process           (j,ctx,dh_key,"dh_key") ;				// template will automatically require serialise/deserialise/size/print_data for your type
//				}
//
//              private:
//                 uint32_t                    count ;				// example of an int type. All int sizes are supported
//                 std::map<uint32_t,rstime_t>   update_times ;		// example of a std::map. All std containers are supported.
//                 RsTlvSecurityKey            key ;				// example of a TlvItem class.
//                 BIGNUM                     *dh_key;				// example of a class that needs its own serializer (see below)
//			};
//
//		Some types may not be already handled by RsTypeSerializer, so in this case, you need to specialise the template for your own type. But this is quite unlikely to
//		happen. In most cases, for instance in your structure types, serialization is directly done by calling RsTypeSerializer::serial_process() on each member of the type.
//		In case you really need a specific serialization for soe particular type, here is how to do it, with the example of BIGNUM* (a crypto primitive):
//
//			template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset, BIGNUM * const & member)
//			{
//				uint32_t s = BN_num_bytes(member) ;
//
//			    if(size < offset + 4 + s)
//			        return false ;
//
//			    bool ok = true ;
//				ok &= setRawUInt32(data, size, &offset, s);
//
//				BN_bn2bin(member,&((unsigned char *)data)[offset]) ;
//				offset += s ;
//
//			    return ok;
//			}
//			template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, BIGNUM *& member)
//			{
//				uint32_t s=0 ;
//			    bool ok = true ;
//			    ok &= getRawUInt32(data, size, &offset, &s);
//
//			    if(s > size || size - s < offset)
//				    return false ;
//
//			    member = BN_bin2bn(&((unsigned char *)data)[offset],s,NULL) ;
//			    offset += s ;
//
//			    return ok;
//			}
//			template<> uint32_t RsTypeSerializer::serial_size(BIGNUM * const & member)
//			{
//				return 4 + BN_num_bytes(member) ;
//			}
//			template<> void     RsTypeSerializer::print_data(const std::string& name,BIGNUM * const & /* member */)
//			{
//			    std::cerr << "[BIGNUM] : " << name << std::endl;
//			}
//
//  3 - in your service, overload the serialiser declaration to add your own:
//
//			MyService::MyService()
//			{
//				addSerialType(new MyServiceSerializer()) ;
//			}
//
//      If needed, you may serialize your own items by calling:
//
//			uint32_t size = MySerializer().size(item) ;
//			uint8_t *data = (uint8_t*)malloc(size);
//			MySerializer().serialise(item,data,size) ;
//
//	4 - in your service, receive and send items by calling recvItem() and sendItem() respectively.
//
#include <stdint.h>
#include <string.h>
#include <iostream>
#include <string>

#include "retroshare/rsflags.h"
#include "serialiser/rsserial.h"
#include "util/rsdeprecate.h"
#include "util/rsjson.h"

struct RsItem;

// This is the base class for serializers.

class RsSerialType
{
public:
	RsSerialType(uint32_t t); /* only uses top 24bits */
	RsSerialType(uint8_t ver, uint8_t cls, uint8_t t);
	RsSerialType(uint8_t ver, uint16_t service);

	virtual     ~RsSerialType();

	virtual	uint32_t    size(RsItem *)=0;
	virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size)=0;
	virtual	RsItem *    deserialise(void *data, uint32_t *size)=0;

	uint32_t    PacketId() const;
private:
	uint32_t type;
};

// This class is only used internally to p3service. It should not be used explicitely otherwise.

class RsRawSerialiser: public RsSerialType
{
	public:
		RsRawSerialiser() :RsSerialType(RS_PKT_VERSION_SERVICE, 0, 0) {}
		virtual     ~RsRawSerialiser() { }

		virtual	uint32_t    size(RsItem *);
		virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
		virtual	RsItem *    deserialise(void *data, uint32_t *size);
};

/** These are convenience flags to be used by the items when processing the
 * data. The names of the flags are not very important. What matters is that
 * the serial_process() method of each item correctly deals with the data
 * when it sees the flags, if the serialiser sets them.
 * By default the flags are not set and shouldn't be handled.
 * When deriving a new serializer, the user can set his own flags, using
 * compatible values
 */
enum class RsSerializationFlags
{
	NONE               =  0,
	CONFIG             =  1,
	SIGNATURE          =  2,
	SKIP_HEADER        =  4,

	/** Used for JSON deserialization in JSON API, it causes the deserialization
	 * to continue even if some field is missing (or incorrect), this way the
	 * API is more user friendly as some methods need just part of the structs
	 * they take as parameters. */
	YIELDING           =  8,

	/** When set integers typer are serialized/deserialized in Variable Length
	 * Quantity mode
	 * @see https://en.wikipedia.org/wiki/Variable-length_quantity
	 * This type of encoding is efficent when absoulte value is usually much
	 * smaller then the maximum representable with the original type.
	 * This encoding is also capable of representing big values at expences of a
	 * one more byte used.
	 */
	INTEGER_VLQ        = 16
};
RS_REGISTER_ENUM_FLAGS_TYPE(RsSerializationFlags);

/// Top class for all services and config serializers.
struct RsGenericSerializer : RsSerialType
{
	typedef enum
	{
		SIZE_ESTIMATE = 0x01,
		SERIALIZE     = 0x02,
		DESERIALIZE   = 0x03,
		PRINT         = 0x04, /// @deprecated use rsdebug.h << operator instead
		TO_JSON,
		FROM_JSON
	} SerializeJob;

	struct SerializeContext
	{
		/** Allow shared allocator usage to avoid costly JSON deepcopy for
		 *  nested RsSerializable */
		SerializeContext(
		        uint8_t* data = nullptr, uint32_t size = 0,
		        RsSerializationFlags flags = RsSerializationFlags::NONE,
		        RsJson::AllocatorType* allocator = nullptr);

		unsigned char *mData;
		uint32_t mSize;
		uint32_t mOffset;
		bool mOk;
		RsSerializationFlags mFlags;
		RsJson mJson;
	};

	/**
	 * The following functions overload RsSerialType.
	 * They *should not* need to be further overloaded.
	 */
	RsItem *deserialise(void *data,uint32_t *size) = 0;
	bool serialise(RsItem *item,void *data,uint32_t *size);
	uint32_t size(RsItem *item);
	void print(RsItem *item);

protected:
	RsGenericSerializer(
	        uint8_t serial_class, uint8_t serial_type,
	        RsSerializationFlags flags ):
	    RsSerialType( RS_PKT_VERSION1, serial_class, serial_type),
	    mFlags(flags) {}

	RsGenericSerializer(
	        uint16_t service, RsSerializationFlags flags ):
	    RsSerialType( RS_PKT_VERSION_SERVICE, service ), mFlags(flags) {}

	RsSerializationFlags mFlags;
};


/** Top class for service serializers.
 * Derive your on service serializer from this class and overload creat_item().
 */
struct RsServiceSerializer : RsGenericSerializer
{
	RsServiceSerializer(
	        uint16_t service_id,
	        RsSerializationFlags flags = RsSerializationFlags::NONE ) :
	    RsGenericSerializer(service_id, flags) {}

	/*! should be overloaded to create the correct type of item depending on the
	 * data */
	virtual RsItem *create_item( uint16_t /* service */,
	                             uint8_t /* item_sub_id */ ) const = 0;

	RsItem *deserialise(void *data, uint32_t *size);
};


/** Top class for config serializers.
 * Config serializers are only used internally by RS core.
 * The development of new services or plugins do not need this.
 */
struct RsConfigSerializer : RsGenericSerializer
{
	RsConfigSerializer(
	        uint8_t config_class, uint8_t config_type,
	        RsSerializationFlags flags = RsSerializationFlags::NONE ) :
	    RsGenericSerializer(config_class, config_type, flags) {}

	/*! should be overloaded to create the correct type of item depending on the
	 * data */
	virtual RsItem *create_item(uint8_t /* item_type */,
	                            uint8_t /* item_sub_type */) const = 0;

	RsItem *deserialise(void *data,uint32_t *size);
};

