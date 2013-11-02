#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"
#include "grouteritems.h"

/**********************************************************************************************/
/*                                         SERIALISATION                                      */
/**********************************************************************************************/

bool RsGRouterItem::serialise_header(void *data,uint32_t& pktsize,uint32_t& tlvsize, uint32_t& offset) const
{
	tlvsize = serial_size() ;
	offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	if(!setRsItemHeader(data, tlvsize, PacketId(), tlvsize))
	{
		std::cerr << "RsFileTransferItem::serialise_header(): ERROR. Not enough size!" << std::endl;
		return false ;
	}
#ifdef RSSERIAL_DEBUG
	std::cerr << "RsFileItemSerialiser::serialiseData() Header: " << ok << std::endl;
#endif
	offset += 8;

	return true ;
}

/* serialise the data to the buffer */
uint32_t RsGRouterPublishKeyItem::serial_size() const
{
	uint32_t s = 8 ; // header
	s += 4  ; // randomized distance
	s += 20 ; // sha1 for published_key
	s += GetTlvStringSize(description_string) ; // description

	return s ;
}
bool RsGRouterPublishKeyItem::serialise(void *data, uint32_t& pktsize) const
{
	uint32_t tlvsize,offset=0;
	bool ok = true;
	
	if(!serialise_header(data,pktsize,tlvsize,offset))
		return false ;

	/* add mandatory parts first */
	ok &= setRawUFloat32(data, tlvsize, &offset, randomized_distance);
	ok &= setRawSha1(data, tlvsize, &offset, published_key);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, description_string);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsFileItemSerialiser::serialiseData() Size Error! " << std::endl;
	}

	return ok;
}

/**********************************************************************************************/
/*                                          SERIALISER STUFF                                  */
/**********************************************************************************************/

RsItem *RsGRouterSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if(RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype) || RS_SERVICE_TYPE_GROUTER != getRsItemService(rstype)) 
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_GROUTER_PUBLISH_KEY:  return deserialise_RsGRouterPublishKeyItem(data, *pktsize);
		case RS_PKT_SUBTYPE_GROUTER_DATA:   		return deserialise_RsGRouterDataItem(data, *pktsize);
		case RS_PKT_SUBTYPE_GROUTER_ACK:    		return deserialise_RsGRouterACKItem(data, *pktsize);
		default:
				std::cerr << "RsGRouterSerialiser::deserialise(): Could not de-serialise item. SubPacket id = " << std::hex << getRsItemSubType(rstype) << " id = " << rstype << std::dec << std::endl;
			return NULL;
	}
	return NULL;
}

RsGRouterItem *RsGRouterSerialiser::deserialise_RsGRouterPublishKeyItem(void *data, uint32_t pktsize) const
{
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	RsGRouterPublishKeyItem *item = new RsGRouterPublishKeyItem() ;

	ok &= getRawUFloat32(data, pktsize, &offset, item->randomized_distance); 	// file hash
	ok &= getRawSha1(data, pktsize, &offset, item->published_key);
	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE,item->description_string);

	if (offset != rssize || !ok)
	{
		std::cerr << __PRETTY_FUNCTION__ << ": error while deserialising! Item will be dropped." << std::endl;
		return NULL ;
	}

	return item;
}

RsGRouterItem *RsGRouterSerialiser::deserialise_RsGRouterDataItem(void *data, uint32_t pktsize) const
{
	std::cerr << "(II) " << __PRETTY_FUNCTION__ << " not yet implemented!" << std::endl;

	return NULL ;
}

RsGRouterItem *RsGRouterSerialiser::deserialise_RsGRouterACKItem(void *data, uint32_t pktsize) const
{
	std::cerr << "(II) " << __PRETTY_FUNCTION__ << " not yet implemented!" << std::endl;

	return NULL ;
}

uint32_t RsGRouterGenericDataItem::serial_size() const
{
	std::cerr << "(II) " << __PRETTY_FUNCTION__ << " not yet implemented!" << std::endl;
	return 0 ;
}
uint32_t RsGRouterACKItem::serial_size() const
{
	std::cerr << "(II) " << __PRETTY_FUNCTION__ << " not yet implemented!" << std::endl;
	return 0 ;
}
bool RsGRouterGenericDataItem::serialise(void *data,uint32_t& size) const
{
	std::cerr << "(II) " << __PRETTY_FUNCTION__ << " not yet implemented!" << std::endl;

	return false ;
}
bool RsGRouterACKItem::serialise(void *data,uint32_t& size) const
{
	std::cerr << "(II) " << __PRETTY_FUNCTION__ << " not yet implemented!" << std::endl;

	return false ;
}

// -----------------------------------------------------------------------------------//
// -------------------------------------  IO  --------------------------------------- // 
// -----------------------------------------------------------------------------------//
//
std::ostream& RsGRouterPublishKeyItem::print(std::ostream& o, uint16_t)
{
	o << "GRouterPublishKeyItem:" << std::endl ;
	o << "  direct origin: \""<< PeerId() << "\"" << std::endl ;
	o << "  Key:            " << published_key.toStdString() << std::endl ;
	o << "  Req. Id:        " << std::hex << diffusion_id << std::dec << std::endl ;
	o << "  Srv. Id:        " << std::hex << service_id << std::dec << std::endl ;
	o << "  Distance:       " << randomized_distance << std::endl ;
	o << "  Description:    " << description_string << std::endl ;

	return o ;
}
std::ostream& RsGRouterACKItem::print(std::ostream& o, uint16_t)
{
	o << "RsGRouterACKItem:" << std::endl ;
	o << "  direct origin: \""<< PeerId() << "\"" << std::endl ;
	o << "  Key:            " << destination_key.toStdString() << std::endl ;
	o << "  State:          " << state << std::endl ;

	return o ;
}
std::ostream& RsGRouterGenericDataItem::print(std::ostream& o, uint16_t)
{
	o << "RsGRouterGenericDataItem:" << std::endl ;
	o << "  direct origin: \""<< PeerId() << "\"" << std::endl ;
	o << "  Key:            " << destination_key.toStdString() << std::endl ;
	o << "  Data size:      " << data_size << std::endl ;

	return o ;
}

