#include "util/rsprint.h"
#include "serialization/rsserializer.h"
#include "serialization/rstypeserializer.h"

const SerializationFlags RsGenericSerializer::SERIALIZATION_FLAG_NONE      ( 0x0000 );
const SerializationFlags RsGenericSerializer::SERIALIZATION_FLAG_CONFIG    ( 0x0001 );
const SerializationFlags RsGenericSerializer::SERIALIZATION_FLAG_SIGNATURE ( 0x0002 );

RsItem *RsServiceSerializer::deserialise(void *data, uint32_t *size)
{
	uint32_t rstype = getRsItemId(const_cast<void*>((const void*)data)) ;

	RsItem *item = create_item(getRsItemService(rstype),getRsItemSubType(rstype)) ;

	if(!item)
	{
		std::cerr << "(EE) cannot deserialise: unknown item type " << std::hex << rstype << std::dec << std::endl;
        std::cerr << "(EE) Data is: " << RsUtil::BinToHex(static_cast<uint8_t*>(data),std::min(50u,*size)) << ((*size>50)?"...":"") << std::endl;
		return NULL ;
	}

	SerializeContext ctx(const_cast<uint8_t*>(static_cast<uint8_t*>(data)),*size,mFormat,mFlags);
	ctx.mOffset = 8 ;

	item->serial_process(RsItem::DESERIALIZE, ctx) ;

	if(ctx.mSize != ctx.mOffset)
	{
		std::cerr << "RsSerializer::deserialise(): ERROR. offset does not match expected size!" << std::endl;
        delete item ;
		return NULL ;
	}
	if(ctx.mOk)
		return item ;

	delete item ;
	return NULL ;
}
RsItem *RsConfigSerializer::deserialise(void *data, uint32_t *size)
{
	uint32_t rstype = getRsItemId(const_cast<void*>((const void*)data)) ;

	RsItem *item = create_item(getRsItemClass(rstype),getRsItemSubType(rstype)) ;

	if(!item)
	{
		std::cerr << "(EE) cannot deserialise: unknown item type " << std::hex << rstype << std::dec << std::endl;
        std::cerr << "(EE) Data is: " << RsUtil::BinToHex(static_cast<uint8_t*>(data),std::min(50u,*size)) << ((*size>50)?"...":"") << std::endl;
		return NULL ;
	}

	SerializeContext ctx(const_cast<uint8_t*>(static_cast<uint8_t*>(data)),*size,mFormat,mFlags);
	ctx.mOffset = 8 ;

	item->serial_process(RsItem::DESERIALIZE, ctx) ;

	if(ctx.mSize != ctx.mOffset)
	{
		std::cerr << "RsSerializer::deserialise(): ERROR. offset does not match expected size!" << std::endl;
        delete item ;
		return NULL ;
	}
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
		std::cerr << "RsSerializer::serialise(): ERROR. offset does not match expected size!" << std::endl;
		return false ;
	}
	return true ;
}

uint32_t RsGenericSerializer::size(RsItem *item)
{
	SerializeContext ctx(NULL,0,mFormat,mFlags);

	ctx.mOffset = 8 ;	// header size
	item->serial_process(RsItem::SIZE_ESTIMATE, ctx) ;

	return ctx.mOffset ;
}

void RsGenericSerializer::print(RsItem *item)
{
	SerializeContext ctx(NULL,0,mFormat,mFlags);

    std::cerr << "***** RsItem class: \"" << typeid(*item).name() << "\" *****" << std::endl;
	item->serial_process(RsItem::PRINT, ctx) ;
    std::cerr << "******************************" << std::endl;
}


