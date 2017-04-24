#pragma once

#include <stdint.h>
#include <string.h>
#include <iostream>
#include <string>

#include "serialiser/rsserial.h"
#include "serialization/rstypeserializer.h"

#define SERIALIZE_ERROR() std::cerr << __PRETTY_FUNCTION__ << " : " 

class RsServiceSerializer: public RsSerialType
{
	public:
    	// These are convenience flags to be used by the items when processing the data. The names of the flags
    	// are not very important. What matters is that the serial_process() method of each item correctly
    	// deals with the data when it sees the flags, if the serialiser sets them. By default the flags are not
    	// set and shouldn't be handled.
    	// When deriving a new serializer, the user can set his own flags, using compatible values

        static const SerializationFlags SERIALIZATION_FLAG_NONE ;			// 0x0000
        static const SerializationFlags SERIALIZATION_FLAG_CONFIG ;			// 0x0001
        static const SerializationFlags SERIALIZATION_FLAG_SIGNATURE ;		// 0x0002

		RsServiceSerializer(uint16_t service_id,
                     SerializeContext::SerializationFormat format = SerializeContext::FORMAT_BINARY,
                     SerializationFlags                    flags  = SERIALIZATION_FLAG_NONE)

            : RsSerialType(RS_PKT_VERSION_SERVICE,service_id),mFormat(format),mFlags(flags) {}

		/*! create_item  
		 * 	should be overloaded to create the correct type of item depending on the data
		 */
		virtual RsItem *create_item(uint16_t /* service */, uint8_t /* item_sub_id */) const=0;

        // The following functions overload RsSerialType. They *should not* need to be further overloaded.

		RsItem *deserialise(void *data,uint32_t *size) ;
		bool serialise(RsItem *item,void *data,uint32_t *size) ;
		uint32_t size(RsItem *item) ;
        void print(RsItem *item) ;

private:
        SerializeContext::SerializationFormat mFormat ;
        SerializationFlags mFlags ;
};



