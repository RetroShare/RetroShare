
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
#include "serialiser/rstypeserializer.h"
#include "util/rsprint.h"
#include "util/rsmemory.h"

#include "gxstunnel/rsgxstunnelitems.h"

//#define GXS_TUNNEL_ITEM_DEBUG 1

RsItem *RsGxsTunnelSerialiser::create_item(uint16_t service,uint8_t item_subtype) const
{
    if(service != RS_SERVICE_TYPE_GXS_TUNNEL)
        return NULL ;

    switch(item_subtype)
    {
    case RS_PKT_SUBTYPE_GXS_TUNNEL_DATA:          return new RsGxsTunnelDataItem();
    case RS_PKT_SUBTYPE_GXS_TUNNEL_DATA_ACK:      return new RsGxsTunnelDataAckItem();
    case RS_PKT_SUBTYPE_GXS_TUNNEL_DH_PUBLIC_KEY: return new RsGxsTunnelDHPublicKeyItem();
    case RS_PKT_SUBTYPE_GXS_TUNNEL_STATUS:        return new RsGxsTunnelStatusItem();
    default:
        return NULL ;
    }
}

RsGxsTunnelDHPublicKeyItem::~RsGxsTunnelDHPublicKeyItem() 
{
	BN_free(public_key) ;
}

void RsGxsTunnelDHPublicKeyItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process           (j,ctx,public_key,"public_key") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,signature,"signature") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,gxs_key,"gxs_key") ;
}

template<> bool RsTypeSerializer::serialize(uint8_t data[], uint32_t size, uint32_t &offset, BIGNUM * const & member)
{
	uint32_t s = BN_num_bytes(member) ;

    if(size < offset + 4 + s)
        return false ;

    bool ok = true ;
	ok &= setRawUInt32(data, size, &offset, s);

	BN_bn2bin(member,&((unsigned char *)data)[offset]) ;
	offset += s ;

    return ok;
}
template<> bool RsTypeSerializer::deserialize(const uint8_t data[], uint32_t size, uint32_t &offset, BIGNUM *& member)
{
	uint32_t s=0 ;
    bool ok = true ;
    ok &= getRawUInt32(data, size, &offset, &s);

    if(s > size || size - s < offset)
	    return false ;

    member = BN_bin2bn(&((unsigned char *)data)[offset],s,NULL) ;
    offset += s ;

    return ok;
}
template<> uint32_t RsTypeSerializer::serial_size(BIGNUM * const & member)
{
	return 4 + BN_num_bytes(member) ;
}
template<> void     RsTypeSerializer::print_data(const std::string& name,BIGNUM * const & /* member */)
{
    std::cerr << "[BIGNUM] : " << name << std::endl;
}

void RsGxsTunnelStatusItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,status,"status") ;
}

void RsGxsTunnelDataItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,unique_item_counter,"unique_item_counter") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,flags              ,"flags") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,service_id         ,"service_id") ;

    RsTypeSerializer::TlvMemBlock_proxy mem(data,data_size) ;
    RsTypeSerializer::serial_process(j,ctx,mem,"data") ;
}
void RsGxsTunnelDataAckItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint64_t>(j,ctx,unique_item_counter,"unique_item_counter") ;
}

















