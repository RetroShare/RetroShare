/*******************************************************************************
 * libretroshare/src/gxstunnel: rsgxstunnelitems.cc                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2015 by Cyril Soler <csoler@users.sourceforge.net>                *
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

#include <stdexcept>
#include "util/rstime.h"
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

RS_TYPE_SERIALIZER_TO_JSON_NOT_IMPLEMENTED_DEF(BIGNUM*)
RS_TYPE_SERIALIZER_FROM_JSON_NOT_IMPLEMENTED_DEF(BIGNUM*)

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

















