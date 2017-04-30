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
//				RsItem *create_item(uint16_t service,uint8_t item_subtype)
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
//                 std::map<uint32_t,time_t>   update_times ;		// example of a std::map. All std containers are supported.
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

class RsItem ;

#define SERIALIZE_ERROR() std::cerr << __PRETTY_FUNCTION__ << " : " 

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


class RsRawSerialiser: public RsSerialType
{
	public:
		RsRawSerialiser() :RsSerialType(RS_PKT_VERSION_SERVICE, 0, 0) {}
		virtual     ~RsRawSerialiser() { }

		virtual	uint32_t    size(RsItem *);
		virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
		virtual	RsItem *    deserialise(void *data, uint32_t *size);
};

class RsGenericSerializer: public RsSerialType
{
public:
		typedef enum { SIZE_ESTIMATE = 0x01, SERIALIZE   = 0x02, DESERIALIZE  = 0x03, PRINT=0x04 } SerializeJob ;
		typedef enum { FORMAT_BINARY = 0x01, FORMAT_JSON = 0x02 }                                  SerializationFormat ;

		class SerializeContext
		{
		public:


			SerializeContext(uint8_t *data,uint32_t size,SerializationFormat format,SerializationFlags flags)
			    : mData(data),mSize(size),mOffset(0),mOk(true),mFormat(format),mFlags(flags) {}

			unsigned char *mData ;
			uint32_t mSize ;
			uint32_t mOffset ;
			bool mOk ;
			SerializationFormat mFormat ;
			SerializationFlags mFlags ;
		};

    	// These are convenience flags to be used by the items when processing the data. The names of the flags
    	// are not very important. What matters is that the serial_process() method of each item correctly
    	// deals with the data when it sees the flags, if the serialiser sets them. By default the flags are not
    	// set and shouldn't be handled.
    	// When deriving a new serializer, the user can set his own flags, using compatible values

        static const SerializationFlags SERIALIZATION_FLAG_NONE ;			// 0x0000
        static const SerializationFlags SERIALIZATION_FLAG_CONFIG ;			// 0x0001
        static const SerializationFlags SERIALIZATION_FLAG_SIGNATURE ;		// 0x0002
        static const SerializationFlags SERIALIZATION_FLAG_SKIP_HEADER ;	// 0x0004

        // The following functions overload RsSerialType. They *should not* need to be further overloaded.

		RsItem *deserialise(void *data,uint32_t *size) =0;
		bool serialise(RsItem *item,void *data,uint32_t *size) ;
		uint32_t size(RsItem *item) ;
        void print(RsItem *item) ;

protected:
    	RsGenericSerializer(uint8_t serial_class,
                            uint8_t serial_type,
                            SerializationFormat format,
                            SerializationFlags                    flags  )
            : RsSerialType(RS_PKT_VERSION1,serial_class,serial_type), mFormat(format),mFlags(flags)
        {}

    	RsGenericSerializer(uint16_t service,
                            SerializationFormat format,
                            SerializationFlags                    flags  )
            : RsSerialType(RS_PKT_VERSION_SERVICE,service), mFormat(format),mFlags(flags)
        {}

        SerializationFormat mFormat ;
        SerializationFlags mFlags ;

};

class RsServiceSerializer: public RsGenericSerializer
{
public:
		RsServiceSerializer(uint16_t service_id,
		                    SerializationFormat format = FORMAT_BINARY,
		                    SerializationFlags  flags  = SERIALIZATION_FLAG_NONE)

		    : RsGenericSerializer(service_id,format,flags) {}

		/*! create_item
		 * 	should be overloaded to create the correct type of item depending on the data
		 */
		virtual RsItem *create_item(uint16_t /* service */, uint8_t /* item_sub_id */) const=0;

		RsItem *deserialise(void *data,uint32_t *size) ;
};

class RsConfigSerializer: public RsGenericSerializer
{
public:
	RsConfigSerializer(uint8_t config_class,
	                   uint8_t config_type,
	                   SerializationFormat format = FORMAT_BINARY,
	                   SerializationFlags  flags  = SERIALIZATION_FLAG_NONE)

	    : RsGenericSerializer(config_class,config_type,format,flags) {}

		/*! create_item
		 * 	should be overloaded to create the correct type of item depending on the data
		 */
		virtual RsItem *create_item(uint8_t /* class */, uint8_t /* item_type */) const=0;

		RsItem *deserialise(void *data,uint32_t *size) ;
};




