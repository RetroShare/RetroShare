#pragma once

#include <stdint.h>
#include <string.h>
#include <iostream>
#include <string>

#include "serialiser/rsserial.h"

#define SERIALIZE_ERROR() std::cerr << __PRETTY_FUNCTION__ << " : " 

class RsSerializer: public RsSerialType
{
	public:
		RsSerializer(uint16_t service_id) : RsSerialType(RS_PKT_VERSION_SERVICE,service_id) {}

		/*! create_item  
		 * 	should be overloaded to create the correct type of item depending on the data
		 */
		virtual RsItem *create_item(uint16_t /* service */, uint8_t /* item_sub_id */)
		{
			return NULL ;
		}

        // The following functions overload RsSerialType. They *should not* need to be further overloaded.

		RsItem *deserialise(void *data,uint32_t *size) ;
		bool serialise(RsItem *item,void *data,uint32_t *size) ;
		uint32_t size(RsItem *item) ;
        void print(RsItem *item) ;
};



