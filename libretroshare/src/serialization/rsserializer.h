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

		RsItem *deserialise(const uint8_t *data,uint32_t size) 
		{
			uint32_t rstype = getRsItemId(const_cast<void*>((const void*)data)) ;

			RsItem *item = create_item(getRsItemService(rstype),getRsItemSubType(rstype)) ;

			if(!item)
			{
				std::cerr << "(EE) cannot deserialise: unknown item type " << std::hex << rstype << std::dec << std::endl;
				return NULL ;
			}
			
			SerializeContext ctx(const_cast<uint8_t*>(data),size);
			ctx.mOffset = 8 ;

			item->serial_process(RsItem::DESERIALIZE, ctx) ;

			if(ctx.mOk)
				return item ;

			delete item ;
			return NULL ;
		}

		bool serialise(RsItem *item,uint8_t *const data,uint32_t size) 
		{
			SerializeContext ctx(data,0);

			uint32_t tlvsize = this->size(item) ;

			if(tlvsize > size)
				throw std::runtime_error("Cannot serialise: not enough room.") ;

			if(!setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize))
			{
				std::cerr << "RsSerializer::serialise_item(): ERROR. Not enough size!" << std::endl;
				return false ;
			}
			ctx.mOffset = 8;
			ctx.mSize = tlvsize;

			item->serial_process(RsItem::SERIALIZE,ctx) ;

			if(ctx.mSize != ctx.mOffset)
			{
				std::cerr << "RsSerializer::serialise_item(): ERROR. offset does not match expected size!" << std::endl;
				return false ;
			}
			return true ;
		}

		uint32_t size(RsItem *item) 
		{
			SerializeContext ctx(NULL,0);

			ctx.mSize = 8 ;	// header size
			item->serial_process(RsItem::SIZE_ESTIMATE, ctx) ;

			return ctx.mSize ;
		}
};



