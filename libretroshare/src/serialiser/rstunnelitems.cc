#ifndef WINDOWS_SYS
#include <stdexcept>
#endif
#include <iostream>
#include "rstunnelitems.h"

// -----------------------------------------------------------------------------------//
// --------------------------------  Serialization. --------------------------------- //
// -----------------------------------------------------------------------------------//
//

//
// ---------------------------------- Packet sizes -----------------------------------//
//

uint32_t RsTunnelDataItem::serial_size()
{
#ifdef P3TUNNEL_DEBUG
	std::cerr << "RsTunnelDataItem::serial_size() called." << std::endl ;
#endif
	uint32_t s = 0 ;

	s += 8 ; // header
	s += GetTlvStringSize(sourcePeerId) ;
	s += GetTlvStringSize(relayPeerId) ;
	s += GetTlvStringSize(destPeerId) ;
	s += 4 ;	//connection_accept

	s += 4 ;	//encoded_data_len
	s += encoded_data_len;

	return s ;
}

//
// ---------------------------------- Serialization ----------------------------------//
//
RsItem *RsTunnelSerialiser::deserialise(void *data, uint32_t *size)
{
#ifdef P3TUNNEL_DEBUG
	std::cerr << "RsTunnelDataItem::deserialise() called." << std::endl ;
#endif
	// look what we have...

	/* get the type */
	uint32_t rstype = getRsItemId(data);
#ifdef P3TUNNEL_DEBUG
	std::cerr << "p3tunnel: deserialising packet: " << std::endl ;
#endif
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_TUNNEL != getRsItemService(rstype)))
	{
#ifdef P3TUNNEL_DEBUG
		std::cerr << "  Wrong type !!" << std::endl ;
#endif
		return NULL; /* wrong type */
	}

#ifndef WINDOWS_SYS
	try
	{
#endif
		switch(getRsItemSubType(rstype))
		{
			case RS_TUNNEL_SUBTYPE_DATA		:	return new RsTunnelDataItem(data,*size) ;

			default:
			    std::cerr << "Unknown packet type in Rstunnel!" << std::endl ;
			    return NULL ;
		}
#ifndef WINDOWS_SYS
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception raised: " << e.what() << std::endl ;
		return NULL ;
	}
#endif

}

bool RsTunnelDataItem::serialize(void *data,uint32_t& pktsize)
{
#ifdef P3TUNNEL_DEBUG
	std::cerr << "RsTunnelDataItem::serialize() called." << std::endl ;
#endif
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

#ifdef P3TUNNEL_DEBUG
        std::cerr << "RsTunnelDataItem::serialize() tlvsize : " << tlvsize << std::endl ;
#endif

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, sourcePeerId);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, relayPeerId);
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, destPeerId);
	ok &= setRawUInt32(data, tlvsize, &offset, connection_accepted);

	ok &= setRawUInt32(data, tlvsize, &offset, encoded_data_len) ;
        memcpy((void*)((unsigned char*)data+offset),encoded_data,encoded_data_len) ;

#ifdef P3TUNNEL_DEBUG
        std::cerr << "RsTunnelDataItem::serialise() (offset + encoded_data_len) " << (offset + encoded_data_len) << std::endl;
        std::cerr << "RsTunnelDataItem::serialise() tlvsize " << tlvsize << std::endl;
#endif

        if ((offset + encoded_data_len) != tlvsize )
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
                std::cerr << "RsTunnelDataItem::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

        #ifdef P3TUNNEL_DEBUG
        std::cerr << "RsTunnelDataItem::serialize() packet size inside serialised data : " << getRsItemSize(data) << std::endl ;
        #endif

	return ok;
}

//deserialize in constructor
RsTunnelDataItem::RsTunnelDataItem(void *data,uint32_t pktsize)
: RsTunnelItem(RS_TUNNEL_SUBTYPE_DATA)
{
#ifdef P3TUNNEL_DEBUG
	std::cerr << "RsTunnelDataItem constructor called : deserializing packet." << std::endl ;
#endif
	uint32_t offset = 8; // skip the header
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

#ifdef P3TUNNEL_DEBUG
        std::cerr << "RsTunnelDataItem constructor rssize : " << rssize << std::endl ;
#endif

	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, sourcePeerId);
	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, relayPeerId);
	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, destPeerId);
	ok &= getRawUInt32(data, pktsize, &offset, &connection_accepted);

	ok &= getRawUInt32(data, pktsize, &offset, &encoded_data_len) ;
        encoded_data = (void*)malloc(encoded_data_len) ;
        memcpy(encoded_data, (void*)((unsigned char*)data+offset), encoded_data_len);

#ifdef WINDOWS_SYS // No Exceptions in Windows compile. (drbobs).
#else
        if ((offset + encoded_data_len) != rssize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
#endif
}

std::ostream& RsTunnelDataItem::print(std::ostream& o, uint16_t)
{
	o << "RsTunnelDataItem :" << std::endl ;
	o << "  sourcePeerId : " << sourcePeerId << std::endl ;
	o << "  relayPeerId : " << relayPeerId << std::endl ;
	o << "  destPeerId : " << destPeerId << std::endl ;
	o << "  connection_accepted : " << connection_accepted << std::endl ;
	o << "  encoded_data_len : " << encoded_data_len << std::endl ;
        if (encoded_data_len != 0 ) {
           o << "getRsItemSize(encoded_data)" << getRsItemSize(encoded_data) << std::endl ;
        }
        return o ;
}
