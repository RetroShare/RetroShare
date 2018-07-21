/*******************************************************************************
 * libretroshare/src/rsitems: rsconfigitems.cc                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include "serialiser/rsbaseserial.h"
#include "rsitems/rsconfigitems.h"
#include "retroshare/rspeers.h" // Needed for RsGroupInfo.

#include "serialiser/rsserializable.h"
#include "serialiser/rstypeserializer.h"
/***
 * #define RSSERIAL_DEBUG 		1
 * #define RSSERIAL_ERROR_DEBUG 	1
 ***/

#define RSSERIAL_ERROR_DEBUG 		1

#include <iostream>


/*************************************************************************/

RsItem *RsFileConfigSerialiser::create_item(uint8_t item_type,uint8_t item_subtype) const
{
    if(item_type != RS_PKT_TYPE_FILE_CONFIG)
        return NULL ;

    switch(item_subtype)
    {
    case RS_PKT_SUBTYPE_FILE_TRANSFER: return new RsFileTransfer() ;
    case RS_PKT_SUBTYPE_FILE_ITEM:     return new RsFileConfigItem() ;
    default:
        return NULL ;
    }
}
void 	RsFileTransfer::clear()
{

	file.TlvClear();
	allPeerIds.TlvClear();
	cPeerId.clear() ;
	state = 0;
	in = false;
	transferred = 0;
	crate = 0;
	trate = 0;
	lrate = 0;
	ltransfer = 0;

}

void RsFileTransfer::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,file,"file") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,allPeerIds,"allPeerIds") ;

    RsTypeSerializer::serial_process           (j,ctx,cPeerId,"cPeerId") ;

    RsTypeSerializer::serial_process<uint16_t> (j,ctx,state,"state") ;
    RsTypeSerializer::serial_process<uint16_t> (j,ctx,in,"in") ;

    RsTypeSerializer::serial_process<uint64_t> (j,ctx,transferred,"transferred") ;

    RsTypeSerializer::serial_process<uint32_t> (j,ctx,crate,"crate") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,trate,"trate") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,lrate,"lrate") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,ltransfer,"ltransfer") ;

    RsTypeSerializer::serial_process<uint32_t> (j,ctx,flags,"flags") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,chunk_strategy,"chunk_strategy") ;
	RS_SERIAL_PROCESS(compressed_chunk_map);
}

void RsFileConfigItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,file,"file") ;
    RsTypeSerializer::serial_process<uint32_t> (j,ctx,flags,"flags") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,parent_groups,"parent_groups") ;
}

RsItem *RsGeneralConfigSerialiser::create_item(uint8_t item_type,uint8_t item_subtype) const
{
    if(item_type != RS_PKT_TYPE_GENERAL_CONFIG)
        return NULL ;

    switch(item_subtype)
    {
    case RS_PKT_SUBTYPE_KEY_VALUE: return new RsConfigKeyValueSet();
    default:
        return NULL ;
    }
}

void RsConfigKeyValueSet::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,tlvkvs,"tlvkvs") ;
}

RsItem *RsPeerConfigSerialiser::create_item(uint8_t item_type,uint8_t item_subtype) const
{
    if(item_type != RS_PKT_TYPE_PEER_CONFIG)
        return NULL ;

    switch(item_subtype)
    {
    case RS_PKT_SUBTYPE_PEER_NET: return new RsPeerNetItem();
    case RS_PKT_SUBTYPE_PEER_STUN: return new RsPeerStunItem();
    case RS_PKT_SUBTYPE_NODE_GROUP: return new RsNodeGroupItem() ;
    case RS_PKT_SUBTYPE_PEER_PERMISSIONS: return new RsPeerServicePermissionItem();
    case RS_PKT_SUBTYPE_PEER_BANDLIMITS: return new RsPeerBandwidthLimitsItem();
    default:
        return NULL ;
    }
}

void RsPeerNetItem::clear()
{
	nodePeerId.clear();
	pgpId.clear();
	location.clear();
	netMode = 0;
	vs_disc = 0;
	vs_dht = 0;
	lastContact = 0;

	localAddrV4.TlvClear();
	extAddrV4.TlvClear();
	localAddrV6.TlvClear();
	extAddrV6.TlvClear();

	dyndns.clear();

	localAddrList.TlvClear();
	extAddrList.TlvClear();

	domain_addr.clear();
	domain_port = 0;
}
void RsPeerNetItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
	RsTypeSerializer::serial_process(j,ctx,nodePeerId,"peerId") ;
	RsTypeSerializer::serial_process(j,ctx,pgpId,"pgpId") ;
	RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_LOCATION,location,"location") ;

	RsTypeSerializer::serial_process<uint32_t>(j,ctx,netMode,"netMode") ;
	RsTypeSerializer::serial_process<uint16_t>(j,ctx,vs_disc,"vs_disc") ;
	RsTypeSerializer::serial_process<uint16_t>(j,ctx,vs_dht,"vs_dht") ;
	RsTypeSerializer::serial_process<uint32_t>(j,ctx,lastContact,"lastContact") ;

	RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,localAddrV4,"localAddrV4") ;
	RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,extAddrV4,"extAddrV4") ;
	RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,localAddrV6,"localAddrV6") ;
	RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,extAddrV6,"extAddrV6") ;

	RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DYNDNS,dyndns,"dyndns") ;

	RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,localAddrList,"localAddrList") ;
	RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,extAddrList,"extAddrList") ;

	RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_DOMADDR,domain_addr,"domain_addr") ;
	RsTypeSerializer::serial_process<uint16_t>(j,ctx,domain_port,"domain_port") ;
}

void RsPeerBandwidthLimitsItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,peers,"peers") ;
}

void RsPeerStunItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,stunList,"stunList") ;
}


RsNodeGroupItem::RsNodeGroupItem(const RsGroupInfo& g)
    :RsItem(RS_PKT_VERSION1, RS_PKT_CLASS_CONFIG, RS_PKT_TYPE_PEER_CONFIG, RS_PKT_SUBTYPE_NODE_GROUP)
{
    id = g.id ;
    name = g.name ;
    flag = g.flag ;
    pgpList.ids = g.peerIds;
}

void RsNodeGroupItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    uint32_t v=0 ;

    RsTypeSerializer::serial_process<uint32_t>(j,ctx,v,"dummy field 0") ;
    RsTypeSerializer::serial_process          (j,ctx,id,"id") ;
    RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_NAME,name,"name") ;
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,flag,"flag") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,pgpList,"pgpList") ;
}

void RsPeerServicePermissionItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    // We need to hack this because of backward compatibility. The correct way to do it would be:
    //
    // RsTypeSerializer::serial_process(j,ctx,pgp_ids,"pgp_ids") ;
    // RsTypeSerializer::serial_process(j,ctx,service_flags,"service_flags") ;

    if(j == RsGenericSerializer::DESERIALIZE)
    {
        uint32_t v=0 ;
        RsTypeSerializer::serial_process<uint32_t>(j,ctx,v,"pgp_ids.size()") ;

        pgp_ids.resize(v) ;
        service_flags.resize(v) ;
    }
    else
    {
        uint32_t s = pgp_ids.size();
        RsTypeSerializer::serial_process<uint32_t>(j,ctx,s,"pgp_ids.size()") ;
    }

	for(uint32_t i=0;i<pgp_ids.size();++i)
	{
		RsTypeSerializer::serial_process(j,ctx,pgp_ids[i],"pgp_ids[i]") ;
		RsTypeSerializer::serial_process(j,ctx,service_flags[i],"service_flags[i]") ;
	}
}

