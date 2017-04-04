#pragma once

#include <stdint.h>
#include <string.h>
#include <iostream>
#include <string>

#include "serialiser/rsserial.h"

#define SERIALIZE_ERROR() std::cerr << __PRETTY_FUNCTION__ << " : " 

class SerializeContext
{
	public:

	SerializeContext(uint8_t *data,uint32_t size)
		: mData(data),mSize(size),mOffset(0),mOk(true) {}

	unsigned char *mData ;
	uint32_t mSize ;
	uint32_t mOffset ;
	bool mOk ;
};

class RsSerializer: public RsSerialType
{
	public:
		RsSerializer(uint16_t service_id) : RsSerialType(service_id) {}

		/*! create_item  
		 * 	should be overloaded to create the correct type of item depending on the data
		 */
		virtual RsItem *create_item(uint16_t service, uint8_t item_sub_id)
		{
			return NULL ;
		}

        // The following functions *should not* be overloaded.
        // They are kept public in order to allow them to be called if needed.

		RsItem *deserialise(const uint8_t *data,uint32_t size) ;
		bool serialise(RsItem *item,uint8_t *const data,uint32_t size) ;
		uint32_t size(RsItem *item) ;
        void print(RsItem *item) ;
};



