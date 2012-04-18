#include <iostream>
#include <iomanip>
#include "rstunnelitems.h"
#include "util/rsstring.h"

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

	s += 4 ;	//encoded_data_len
	s += encoded_data_len;

	return s ;
}

uint32_t RsTunnelHandshakeItem::serial_size()
{
#ifdef P3TUNNEL_DEBUG
        std::cerr << "RsTunnelHandshakeItem::serial_size() called." << std::endl ;
#endif
        uint32_t s = 0 ;

        s += 8 ; // header
        s += GetTlvStringSize(sourcePeerId) ;
        s += GetTlvStringSize(relayPeerId) ;
        s += GetTlvStringSize(destPeerId) ;
        s += GetTlvStringSize(sslCertPEM) ;
        s += 4 ;	//connection_accept


        return s ;
}

//
// ---------------------------------- Serialization ----------------------------------//
//
RsItem *RsTunnelSerialiser::deserialise(void *data, uint32_t *size)
{
#ifdef P3TUNNEL_DEBUG
	std::cerr << "RsTunnelSerialiser::deserialise() called." << std::endl ;
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

	switch(getRsItemSubType(rstype))
	{
		case RS_TUNNEL_SUBTYPE_DATA		:	return RsTunnelDataItem::deserialise(data,*size) ;
		case RS_TUNNEL_SUBTYPE_HANDSHAKE	:	return RsTunnelHandshakeItem::deserialise(data,*size) ;

		default:
														std::cerr << "Unknown packet type in Rstunnel!" << std::endl ;
														return NULL ;
	}
}

RsTunnelDataItem::~RsTunnelDataItem()
{
	if(encoded_data != NULL)
		free(encoded_data) ;
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

	ok &= setRawUInt32(data, tlvsize, &offset, encoded_data_len) ;

	if(encoded_data != NULL)
		memcpy((void*)((unsigned char*)data+offset),encoded_data,encoded_data_len) ;

#ifdef P3TUNNEL_DEBUG
	std::cerr << "RsTunnelDataItem::serialise() (offset + encoded_data_len) " << (offset + encoded_data_len) << std::endl;
	std::cerr << "RsTunnelDataItem::serialise() tlvsize " << tlvsize << std::endl;
#endif

	if ((offset + encoded_data_len) != tlvsize )
	{
		ok = false;
		std::cerr << "RsTunnelDataItem::serialiseTransfer() Size Error! " << std::endl;
	}

#ifdef P3TUNNEL_DEBUG
	std::cerr << "RsTunnelDataItem::serialize() packet size inside serialised data : " << getRsItemSize(data) << std::endl ;
#endif

	return ok;
}

bool RsTunnelHandshakeItem::serialize(void *data,uint32_t& pktsize)
{
#ifdef P3TUNNEL_DEBUG
        std::cerr << "RsTunnelHandshakeItem::serialize() called." << std::endl ;
#endif
        uint32_t tlvsize = serial_size();
        uint32_t offset = 0;

        if (pktsize < tlvsize)
                return false; /* not enough space */

        pktsize = tlvsize;

        bool ok = true;

#ifdef P3TUNNEL_DEBUG
        std::cerr << "RsTunnelHandshakeItem::serialize() tlvsize : " << tlvsize << std::endl ;
#endif

        ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

        /* skip the header */
        offset += 8;

        /* add mandatory parts first */
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, sourcePeerId);
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, relayPeerId);
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, destPeerId);
        ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_CERT_SSL, sslCertPEM);
        ok &= setRawUInt32(data, tlvsize, &offset, connection_accepted);

        if (offset != tlvsize )
        {
                ok = false;
                std::cerr << "RsTunnelHandshakeItem::serialiseTransfer() Size Error! " << std::endl;
        }

        return ok;
}

#ifdef P3TUNNEL_DEBUG
void displayRawPacket(std::ostream &out, void *data, uint32_t size)
{
	uint32_t i;
	std::string sout;
	rs_sprintf(sout, "DisplayRawPacket: Size: %ld", size);

	for(i = 0; i < size; i++)
	{
		if (i % 16 == 0)
		{
			sout += "\n";
		}
		rs_sprintf_append(sout, "%02x:", (int) (((unsigned char *) data)[i]));
	}

	out << sout << std::endl;
}
#endif

//deserialize in constructor
RsTunnelDataItem *RsTunnelDataItem::deserialise(void *data,uint32_t pktsize) 
{
	RsTunnelDataItem *item = new RsTunnelDataItem ;
#ifdef P3TUNNEL_DEBUG
	std::cerr << "RsTunnelDataItem constructor called : deserializing packet." << std::endl ;
#endif
	uint32_t offset = 8; // skip the header
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

#ifdef P3TUNNEL_DEBUG
	std::cerr << "RsTunnelDataItem constructor rssize : " << rssize << std::endl ;
#endif

	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, item->sourcePeerId);
	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, item->relayPeerId);
	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, item->destPeerId);

	ok &= getRawUInt32(data, pktsize, &offset, &item->encoded_data_len) ;

	if(!ok)					// return early to avoid calling malloc with invalid size
		return NULL ;

	if(item->encoded_data_len > 0)
	{
		item->encoded_data = (void*)malloc(item->encoded_data_len) ;
		memcpy(item->encoded_data, (void*)((unsigned char*)data+offset), item->encoded_data_len);
	}
	else
		item->encoded_data = NULL ;

	if ((offset + item->encoded_data_len) != rssize)
	{
		std::cerr << "Size error while deserializing a RsTunnelHandshakeItem."  << std::endl;
#ifdef P3TUNNEL_DEBUG
		displayRawPacket(std::cerr,data,rssize) ;
#endif
		return NULL ;
	}
	if (!ok)
	{
		std::cerr << "Error while deserializing a RstunnelDataItem." << std::endl ;
		return NULL ;
	}
	return item ;
}

//deserialize in constructor
RsTunnelHandshakeItem *RsTunnelHandshakeItem::deserialise(void *data,uint32_t pktsize) 
{
	RsTunnelHandshakeItem *item = new RsTunnelHandshakeItem ;
#ifdef P3TUNNEL_DEBUG
	std::cerr << "RsTunnelHandshakeItem constructor called : deserializing packet." << std::endl ;
#endif
	uint32_t offset = 8; // skip the header
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

#ifdef P3TUNNEL_DEBUG
	std::cerr << "RsTunnelHandshakeItem constructor rssize : " << rssize << std::endl ;
#endif

	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, item->sourcePeerId);
	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, item->relayPeerId);
	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, item->destPeerId);
	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_CERT_SSL, item->sslCertPEM);
	ok &= getRawUInt32(data, pktsize, &offset, &item->connection_accepted);

	if (offset != rssize)
	{
		std::cerr << "Size error while deserializing a RsTunnelHandshakeItem."  << std::endl;
		return NULL ;
	}
	if (!ok)
	{
		std::cerr << "Unknown error while deserializing a RsTunnelHandshakeItem." << std::endl ;
		return NULL ;
	}
	return item ;
}

std::ostream& RsTunnelDataItem::print(std::ostream& o, uint16_t)
{
	o << "RsTunnelDataItem :" << std::endl ;
	o << "  sourcePeerId : " << sourcePeerId << std::endl ;
	o << "  relayPeerId : " << relayPeerId << std::endl ;
        o << "  destPeerId : " << destPeerId << std::endl ;
	o << "  encoded_data_len : " << encoded_data_len << std::endl ;
        return o ;
}

std::ostream& RsTunnelHandshakeItem::print(std::ostream& o, uint16_t)
{
        o << "RsTunnelHandshakeItem :" << std::endl ;
        o << "  sourcePeerId : " << sourcePeerId << std::endl ;
        o << "  relayPeerId : " << relayPeerId << std::endl ;
        o << "  destPeerId : " << destPeerId << std::endl ;
        o << "  sslCertPEM : " << sslCertPEM << std::endl ;
        o << "  connection_accepted : " << connection_accepted << std::endl ;
        return o ;
}
