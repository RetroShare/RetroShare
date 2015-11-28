
/*
 * libretroshare/src/serialiser: rsbaseitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <stdexcept>
#include <time.h>
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvbase.h"
#include "util/rsprint.h"

#include "gxstunnel/rsgxstunnelitems.h"

#define GXS_TUNNEL_ITEM_DEBUG 1

std::ostream& RsGxsTunnelDHPublicKeyItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsTunnelDHPublicKeyItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "  Signature Key ID: " << signature.keyId << std::endl ;
	out << "  Public    Key ID: " << gxs_key.keyId << std::endl ;

	printRsItemEnd(out, "RsGxsTunnelMsgItem", indent);
	return out;
}

std::ostream& RsGxsTunnelDataItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsTunnelDataItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "  message id : " << std::hex << unique_item_counter << std::dec << std::endl ;
	out << "  service id : " << std::hex << service_id << std::dec << std::endl ;
	out << "  flags      : " << std::hex << flags << std::dec << std::endl ;
	out << "  size       : " << data_size << std::endl ;
	out << "  data       : " << RsUtil::BinToHex(data,std::min(50u,data_size)) << ((data_size>50u)?"...":"") << std::endl ;

	printRsItemEnd(out, "RsGxsTunnelDataItem", indent);
	return out;
}
std::ostream& RsGxsTunnelDataAckItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsTunnelDataItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "  message id : " << std::hex << unique_item_counter << std::dec << std::endl ;

	printRsItemEnd(out, "RsGxsTunnelDataAckItem", indent);
	return out;
}
std::ostream& RsGxsTunnelStatusItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsGxsTunnelDataItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "  flags      : " << std::hex << flags << std::dec << std::endl ;

	printRsItemEnd(out, "RsGxsTunnelStatusItem", indent);
	return out;
}

/*************************************************************************/

RsGxsTunnelDHPublicKeyItem::~RsGxsTunnelDHPublicKeyItem() 
{
	BN_free(public_key) ;
}

/*************************************************************************/

RsItem *RsGxsTunnelSerialiser::deserialise(void *data, uint32_t *pktsize)
{
    uint32_t rstype = getRsItemId(data);
    uint32_t rssize = getRsItemSize(data);

#ifdef GXS_TUNNEL_ITEM_DEBUG
    std::cerr << "deserializing packet..."<< std::endl ;
#endif
    // look what we have...
    if (*pktsize < rssize)    /* check size */
    {
	    std::cerr << "GxsTunnel deserialisation: not enough size: pktsize=" << *pktsize << ", rssize=" << rssize << std::endl ;
	    return NULL; /* not enough data */
    }

    /* set the packet length */
    *pktsize = rssize;

    /* ready to load */

    if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_GXS_TUNNEL != getRsItemService(rstype))) 
    {
#ifdef GXS_TUNNEL_ITEM_DEBUG
	    std::cerr << "GxsTunnel deserialisation: wrong type !" << std::endl ;
#endif
	    return NULL; /* wrong type */
    }

    switch(getRsItemSubType(rstype))
    {
    case RS_PKT_SUBTYPE_GXS_TUNNEL_DH_PUBLIC_KEY: 	return deserialise_RsGxsTunnelDHPublicKeyItem(data,*pktsize) ;
    case RS_PKT_SUBTYPE_GXS_TUNNEL_DATA: 		return deserialise_RsGxsTunnelDataItem       (data,*pktsize) ;
    case RS_PKT_SUBTYPE_GXS_TUNNEL_DATA_ACK: 		return deserialise_RsGxsTunnelDataAckItem    (data,*pktsize) ;
    case RS_PKT_SUBTYPE_GXS_TUNNEL_STATUS: 		return deserialise_RsGxsTunnelStatusItem     (data,*pktsize) ;
    default:
	    std::cerr << "Unknown packet type in chat!" << std::endl ;
	    return NULL ;
    }
}

/*************************************************************************/

uint32_t RsGxsTunnelDHPublicKeyItem::serial_size()
{
	uint32_t s = 8 ;                // header
	s += 4 ;	               	 // BN size
	s += BN_num_bytes(public_key) ; // public_key
	s += signature.TlvSize() ;      // signature
	s += gxs_key.TlvSize() ;        // gxs_key

	return s ;
}

uint32_t RsGxsTunnelDataItem::serial_size()
{
	uint32_t s = 8 ;               // header
	s += 8 ;	               	// counter
	s += 4 ;	               	// flags
	s += 4 ;	               	// service id
	s += 4 ;	               	// data_size
	s += data_size;			// data

	return s ;
}

uint32_t RsGxsTunnelDataAckItem::serial_size()
{
	uint32_t s = 8 ;               // header
	s += 8 ;	               	// counter

	return s ;
}

uint32_t RsGxsTunnelStatusItem::serial_size()
{
	uint32_t s = 8 ;               // header
	s += 4 ;	               	// flags

	return s ;
}
/*************************************************************************/

bool RsGxsTunnelDHPublicKeyItem::serialise(void *data,uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	uint32_t s = BN_num_bytes(public_key) ;

	ok &= setRawUInt32(data, tlvsize, &offset, s);

	BN_bn2bin(public_key,&((unsigned char *)data)[offset]) ;
	offset += s ;

	ok &= signature.SetTlv(data, tlvsize, &offset);
	ok &= gxs_key.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsGxsTunnelDHPublicKeyItem::serialiseItem() Size Error! offset=" << offset << ", tlvsize=" << tlvsize << std::endl;
	}
	return ok ;
}

bool RsGxsTunnelStatusItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef GXS_TUNNEL_ITEM_DEBUG
	std::cerr << "RsGxsTunnelSerialiser serialising chat status item." << std::endl;
	std::cerr << "RsGxsTunnelSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsGxsTunnelSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, flags);

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsGxsTunnelSerialiser::serialiseItem() Size Error! " << std::endl;
	}
#ifdef GXS_TUNNEL_ITEM_DEBUG
	std::cerr << "computed size: " << 256*((unsigned char*)data)[6]+((unsigned char*)data)[7] << std::endl ;
#endif

	return ok;
}

bool RsGxsTunnelDataItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef GXS_TUNNEL_ITEM_DEBUG
	std::cerr << "RsGxsTunnelSerialiser serialising chat status item." << std::endl;
	std::cerr << "RsGxsTunnelSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsGxsTunnelSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt64(data, tlvsize, &offset, unique_item_counter);
	ok &= setRawUInt32(data, tlvsize, &offset, flags);
	ok &= setRawUInt32(data, tlvsize, &offset, service_id);
	ok &= setRawUInt32(data, tlvsize, &offset, data_size);
    
    	if(offset + data_size <= tlvsize)
        {
            memcpy(&((uint8_t*)data)[offset],data,data_size) ;
            offset += data_size ;
        }
        else
            ok = false ;

	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsGxsTunnelSerialiser::serialiseItem() Size Error! " << std::endl;
	}

	return ok;
}
bool RsGxsTunnelDataAckItem::serialise(void *data, uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size() ;
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, PacketId(), tlvsize);

#ifdef GXS_TUNNEL_ITEM_DEBUG
	std::cerr << "RsGxsTunnelSerialiser serialising chat status item." << std::endl;
	std::cerr << "RsGxsTunnelSerialiser::serialiseItem() Header: " << ok << std::endl;
	std::cerr << "RsGxsTunnelSerialiser::serialiseItem() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt64(data, tlvsize, &offset, unique_item_counter);
    
	if (offset != tlvsize)
	{
		ok = false;
		std::cerr << "RsGxsTunnelSerialiser::serialiseItem() Size Error! " << std::endl;
	}

	return ok;
}

/*************************************************************************/

RsGxsTunnelDHPublicKeyItem *RsGxsTunnelSerialiser::deserialise_RsGxsTunnelDHPublicKeyItem(void *data,uint32_t /*size*/)
{
    uint32_t offset = 8; // skip the header 
    uint32_t rssize = getRsItemSize(data);
    bool ok = true ;

    RsGxsTunnelDHPublicKeyItem *item = new RsGxsTunnelDHPublicKeyItem() ;

    uint32_t s=0 ;
    /* get mandatory parts first */
    ok &= getRawUInt32(data, rssize, &offset, &s);

    item->public_key = BN_bin2bn(&((unsigned char *)data)[offset],s,NULL) ;
    offset += s ;

    ok &= item->signature.GetTlv(data, rssize, &offset) ;
    ok &= item->gxs_key.GetTlv(data, rssize, &offset) ;

    if (offset != rssize)
    {
	    std::cerr << "RsGxsTunnelDHPublicKeyItem::() Size error while deserializing." << std::endl ;
            delete item ;
	    return NULL ;
    }
    if (!ok)
    {
	    std::cerr << "RsGxsTunnelDHPublicKeyItem::() Unknown error while deserializing." << std::endl ;
	    delete item ;
	    return NULL ;
    }

    return item ;
}

RsGxsTunnelDataItem *RsGxsTunnelSerialiser::deserialise_RsGxsTunnelDataItem(void *dat,uint32_t size)
{
    uint32_t offset = 8; // skip the header 
    uint32_t rssize = getRsItemSize(dat);
    bool ok = true ;

    RsGxsTunnelDataItem *item = new RsGxsTunnelDataItem();

    /* get mandatory parts first */

    ok &= getRawUInt64(dat, rssize, &offset, &item->unique_item_counter);
    ok &= getRawUInt32(dat, rssize, &offset, &item->flags);
    ok &= getRawUInt32(dat, rssize, &offset, &item->service_id);
    ok &= getRawUInt32(dat, rssize, &offset, &item->data_size);

    if(offset + item->data_size <= size)
    {
	    item->data = (unsigned char*)malloc(item->data_size) ;

	    if(dat == NULL)
	{
		delete item ;
		return NULL ;
	}

	    memcpy(item->data,&((uint8_t*)dat)[offset],item->data_size) ;
	    offset += item->data_size ;
    }
    else
	    ok = false ;


    if (offset != rssize)
    {
	    std::cerr << "RsGxsTunnelDHPublicKeyItem::() Size error while deserializing." << std::endl ;
	    delete item ;
	    return NULL ;
    }
    if (!ok)
    {
	    std::cerr << "RsGxsTunnelDHPublicKeyItem::() Unknown error while deserializing." << std::endl ;
	    delete item ;
	    return NULL ;
    }

    return item ;
}

RsGxsTunnelDataAckItem *RsGxsTunnelSerialiser::deserialise_RsGxsTunnelDataAckItem(void *dat,uint32_t /* size */)
{
    uint32_t offset = 8; // skip the header 
    uint32_t rssize = getRsItemSize(dat);
    bool ok = true ;

    RsGxsTunnelDataAckItem *item = new RsGxsTunnelDataAckItem();

    /* get mandatory parts first */

    ok &= getRawUInt64(dat, rssize, &offset, &item->unique_item_counter);

    if (offset != rssize)
    {
	    std::cerr << "RsGxsTunnelDHPublicKeyItem::() Size error while deserializing." << std::endl ;
	    delete item ;
	    return NULL ;
    }
    if (!ok)
    {
	    std::cerr << "RsGxsTunnelDHPublicKeyItem::() Unknown error while deserializing." << std::endl ;
	    delete item ;
	    return NULL ;
    }

    return item ;
}

RsGxsTunnelStatusItem *RsGxsTunnelSerialiser::deserialise_RsGxsTunnelStatusItem(void *dat,uint32_t size)
{
    uint32_t offset = 8; // skip the header 
    uint32_t rssize = getRsItemSize(dat);
    bool ok = true ;

    RsGxsTunnelStatusItem *item = new RsGxsTunnelStatusItem();

    /* get mandatory parts first */

    ok &= getRawUInt32(dat, rssize, &offset, &item->flags);

    if (offset != rssize)
    {
	    std::cerr << "RsGxsTunnelDHPublicKeyItem::() Size error while deserializing." << std::endl ;
	    delete item ;
	    return NULL ;
    }
    if (!ok)
    {
	    std::cerr << "RsGxsTunnelDHPublicKeyItem::() Unknown error while deserializing." << std::endl ;
	    delete item ;
	    return NULL ;
    }

    return item ;
}
















