#pragma once

#include <stdint.h>
#include <string.h>
#include <iostream>
#include <string>

#include "retroshare/rsflags.h"
#include "serialiser/rsserial.h"

class RsItem ;

#define SERIALIZE_ERROR() std::cerr << __PRETTY_FUNCTION__ << " : " 

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





