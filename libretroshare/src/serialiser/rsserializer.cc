/*******************************************************************************
 * libretroshare/src/serialiser: rsserializer.cc                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2016  Cyril Soler <csoler@users.sourceforge.net>              *
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
#include <typeinfo>

#include "rsitems/rsitem.h"

#include "util/rsprint.h"
#include "serialiser/rsserializer.h"
#include "serialiser/rstypeserializer.h"
#include "util/stacktrace.h"
#include "util/rsdebug.h"

const SerializationFlags RsGenericSerializer::SERIALIZATION_FLAG_NONE        ( 0x0000 );
const SerializationFlags RsGenericSerializer::SERIALIZATION_FLAG_CONFIG      ( 0x0001 );
const SerializationFlags RsGenericSerializer::SERIALIZATION_FLAG_SIGNATURE   ( 0x0002 );
const SerializationFlags RsGenericSerializer::SERIALIZATION_FLAG_SKIP_HEADER ( 0x0004 );
const SerializationFlags RsGenericSerializer::SERIALIZATION_FLAG_YIELDING    ( 0x0008 );

RsItem *RsServiceSerializer::deserialise(void *data, uint32_t *size)
{
	if(!data || !size || !*size)
	{
		RsErr() << __PRETTY_FUNCTION__ << " Called with null paramethers data: "
		        << data << " size: " << static_cast<void*>(size) << " *size: "
		        << (size ? *size : 0) << " this should never happen!"
		        << std::endl;
		print_stacktrace();
		return nullptr;
	}

    if(mFlags & SERIALIZATION_FLAG_SKIP_HEADER)
    {
        std::cerr << "(EE) Cannot deserialise item with flags SERIALIZATION_FLAG_SKIP_HEADER. Check your code!" << std::endl;
        return NULL ;
    }

	uint32_t rstype = getRsItemId(const_cast<void*>((const void*)data)) ;

	RsItem *item = create_item(getRsItemService(rstype),getRsItemSubType(rstype)) ;

	if(!item)
	{
		std::cerr << "(EE) " << typeid(*this).name() << ": cannot deserialise unknown item subtype " << std::hex << (int)getRsItemSubType(rstype) << std::dec << std::endl;
        std::cerr << "(EE) Data is: " << RsUtil::BinToHex(static_cast<uint8_t*>(data),std::min(50u,*size)) << ((*size>50)?"...":"") << std::endl;
		return NULL ;
	}

	SerializeContext ctx(const_cast<uint8_t*>(static_cast<uint8_t*>(data)),*size,mFormat,mFlags);
	ctx.mOffset = 8 ;

	item->serial_process(RsGenericSerializer::DESERIALIZE, ctx) ;

	if(ctx.mSize < ctx.mOffset)
	{
		std::cerr << "RsSerializer::deserialise(): ERROR. offset does not match expected size!" << std::endl;
        delete item ;
		return NULL ;
	}
    *size = ctx.mOffset ;

	if(ctx.mOk)
		return item ;

	delete item ;
	return NULL ;
}
RsItem *RsConfigSerializer::deserialise(void *data, uint32_t *size)
{
    if(mFlags & SERIALIZATION_FLAG_SKIP_HEADER)
    {
        std::cerr << "(EE) Cannot deserialise item with flags SERIALIZATION_FLAG_SKIP_HEADER. Check your code!" << std::endl;
        return NULL ;
    }

	uint32_t rstype = getRsItemId(const_cast<void*>((const void*)data)) ;

	RsItem *item = create_item(getRsItemType(rstype),getRsItemSubType(rstype)) ;

	if(!item)
	{
		std::cerr << "(EE) " << typeid(*this).name() << ": cannot deserialise unknown item subtype " << std::hex << (int)getRsItemSubType(rstype) << std::dec << std::endl;
        std::cerr << "(EE) Data is: " << RsUtil::BinToHex(static_cast<uint8_t*>(data),std::min(50u,*size)) << ((*size>50)?"...":"") << std::endl;
		return NULL ;
	}

	SerializeContext ctx(const_cast<uint8_t*>(static_cast<uint8_t*>(data)),*size,mFormat,mFlags);
	ctx.mOffset = 8 ;

	item->serial_process(DESERIALIZE, ctx) ;

	if(ctx.mSize < ctx.mOffset)
	{
		std::cerr << "RsSerializer::deserialise(): ERROR. offset does not match expected size!" << std::endl;
        delete item ;
		return NULL ;
	}
    *size = ctx.mOffset ;

	if(ctx.mOk)
		return item ;

	delete item ;
	return NULL ;
}
bool RsGenericSerializer::serialise(RsItem *item,void *data,uint32_t *size)
{
	SerializeContext ctx(static_cast<uint8_t*>(data),0,mFormat,mFlags);

	uint32_t tlvsize = this->size(item) ;

	if(tlvsize > *size)
		throw std::runtime_error("Cannot serialise: not enough room.") ;


    if(mFlags & SERIALIZATION_FLAG_SKIP_HEADER)
		ctx.mOffset = 0;
	else
	{
		if(!setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize))
		{
			std::cerr << "RsSerializer::serialise_item(): ERROR. Not enough size!" << std::endl;
			return false ;
		}
		ctx.mOffset = 8;
	}

	ctx.mSize = tlvsize;

	item->serial_process(RsGenericSerializer::SERIALIZE,ctx) ;

	if(ctx.mSize != ctx.mOffset)
	{
		std::cerr << "RsSerializer::serialise(): ERROR. offset does not match expected size!" << std::endl;
		return false ;
	}
    *size = ctx.mOffset ;

	return true ;
}

uint32_t RsGenericSerializer::size(RsItem *item)
{
	SerializeContext ctx(NULL,0,mFormat,mFlags);

    if(mFlags & SERIALIZATION_FLAG_SKIP_HEADER)
		ctx.mOffset = 0;
	else
		ctx.mOffset = 8 ;	// header size
	item->serial_process(SIZE_ESTIMATE, ctx) ;

	return ctx.mOffset ;
}

void RsGenericSerializer::print(RsItem *item)
{
	SerializeContext ctx(NULL,0,mFormat,mFlags);

    std::cerr << "***** RsItem class: \"" << typeid(*item).name() << "\" *****" << std::endl;
	item->serial_process(PRINT, ctx) ;
    std::cerr << "******************************" << std::endl;
}

uint32_t    RsRawSerialiser::size(RsItem *i)
{
	RsRawItem *item = dynamic_cast<RsRawItem *>(i);

	if (item)
	{
		return item->getRawLength();
	}
	return 0;
}

/* serialise the data to the buffer */
bool    RsRawSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
        RsRawItem *item = dynamic_cast<RsRawItem *>(i);
	if (!item)
	{
		return false;
	}

        #ifdef RSSERIAL_DEBUG
                std::cerr << "RsRawSerialiser::serialise() serializing raw item. pktsize : " << *pktsize;
        #endif

	uint32_t tlvsize = item->getRawLength();
        #ifdef RSSERIAL_DEBUG
                std::cerr << "tlvsize : " << tlvsize << std::endl;
        #endif

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	if (tlvsize > getRsPktMaxSize())
    	{
	    std::cerr << "(EE) Serialised packet is too big. Maximum allowed size is " << getRsPktMaxSize() << ". Serialised size is " << tlvsize << ". Please tune your service to correctly split packets" << std::endl;
	    return false; /* packet too big */
    	}

	*pktsize = tlvsize;

	/* its serialised already!!! */
	memcpy(data, item->getRawData(), tlvsize);

	return true;
}

RsItem *RsRawSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	if (RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
		return NULL; /* not enough data */

	if (rssize > getRsPktMaxSize())
		return NULL; /* packet too big */

	/* set the packet length */
	*pktsize = rssize;

	RsRawItem *item = new RsRawItem(rstype, rssize);
	void *item_data = item->getRawData();

	memcpy(item_data, data, rssize);

	return item;
}


RsGenericSerializer::SerializeContext::SerializeContext(
        uint8_t* data, uint32_t size, SerializationFlags flags,
        RsJson::AllocatorType* allocator ) :
    mData(data), mSize(size), mOffset(0), mOk(true), mFlags(flags),
    mJson(rapidjson::kObjectType, allocator)
{
	if(data)
	{
		if(size == 0)
		{
			std::cerr << __PRETTY_FUNCTION__ << " data passed without "
			          << "size! This make no sense report to developers!"
			          << std::endl;
			print_stacktrace();
		}

		if(flags & SERIALIZATION_FLAG_YIELDING)
		{
			std::cerr << __PRETTY_FUNCTION__ << " Attempt to create a "
			          << "binary serialization context with "
			          << "SERIALIZATION_FLAG_YIELDING! "
			          << "This make no sense report to developers!"
			          << std::endl;
			print_stacktrace();
		}
	}
}
