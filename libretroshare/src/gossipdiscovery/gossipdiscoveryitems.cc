/*******************************************************************************
 * Gossip discovery service items                                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2007-2008  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include "gossipdiscovery/gossipdiscoveryitems.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstypeserializer.h"
#include "serialiser/rsserializable.h"

#include <iostream>

RsItem *RsDiscSerialiser::create_item(
        uint16_t service, uint8_t item_subtype ) const
{
	if(service != RS_SERVICE_TYPE_DISC) return nullptr;

	switch(static_cast<RsGossipDiscoveryItemType>(item_subtype))
	{
	case RsGossipDiscoveryItemType::PGP_LIST: return new RsDiscPgpListItem();
	case RsGossipDiscoveryItemType::PGP_CERT_BINARY: return new RsDiscPgpKeyItem();
	case RsGossipDiscoveryItemType::PGP_CERT: return new RsDiscPgpCertItem();	// deprecated, hanlde to suppress "unkown item" warning
	case RsGossipDiscoveryItemType::CONTACT:  return new RsDiscContactItem();
	case RsGossipDiscoveryItemType::IDENTITY_LIST: return new RsDiscIdentityListItem();
    default:
        return nullptr;
	}

	return nullptr;
}

/*************************************************************************/

void RsDiscPgpListItem::clear()
{
	mode = RsGossipDiscoveryPgpListMode::NONE;
	pgpIdSet.TlvClear();
}

void RsDiscPgpListItem::serial_process(
        RsGenericSerializer::SerializeJob j,
        RsGenericSerializer::SerializeContext& ctx )
{
	RS_SERIAL_PROCESS(mode);
	RS_SERIAL_PROCESS(pgpIdSet);
}

void RsDiscPgpKeyItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,pgpKeyId,"pgpKeyId") ;

    RsTypeSerializer::TlvMemBlock_proxy prox(bin_data,bin_len) ;
    RsTypeSerializer::serial_process(j,ctx,prox,"keyData") ;
}

void RsDiscPgpKeyItem::clear()
{
	pgpKeyId.clear();
	free(bin_data);
	bin_data = nullptr;
	bin_len = 0;
}

void 	RsDiscContactItem::clear()
{
	pgpId.clear();
	sslId.clear();

	location.clear();
	version.clear();

	netMode = 0;
	vs_disc = 0;
	vs_dht = 0;
	lastContact = 0;

	isHidden = false;
	hiddenAddr.clear();
	hiddenPort = 0;

	localAddrV4.TlvClear();
	extAddrV4.TlvClear();
	localAddrV6.TlvClear();
	extAddrV6.TlvClear();


	dyndns.clear();

	localAddrList.TlvClear();
	extAddrList.TlvClear();
}

void RsDiscContactItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
   RsTypeSerializer::serial_process          (j,ctx,pgpId,"pgpId");
   RsTypeSerializer::serial_process          (j,ctx,sslId,"sslId");
   RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_LOCATION,location,"location");
   RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_VERSION,version,"version");
   RsTypeSerializer::serial_process<uint32_t>(j,ctx,netMode,"netMode");
   RsTypeSerializer::serial_process<uint16_t>(j,ctx,vs_disc,"vs_disc");
   RsTypeSerializer::serial_process<uint16_t>(j,ctx,vs_dht,"vs_dht");
   RsTypeSerializer::serial_process<uint32_t>(j,ctx,lastContact,"lastContact");

   // This is a hack. Normally we should have to different item types, in order to avoid this nonesense.

   if(j == RsGenericSerializer::DESERIALIZE)
	   isHidden = ( GetTlvType( &(((uint8_t *) ctx.mData)[ctx.mOffset])  )==TLV_TYPE_STR_DOMADDR);

   if(isHidden)
   {
	   RsTypeSerializer::serial_process          (j,ctx,TLV_TYPE_STR_DOMADDR,hiddenAddr,"hiddenAddr");
	   RsTypeSerializer::serial_process<uint16_t>(j,ctx,hiddenPort,"hiddenPort");
   }
   else
   {
	   RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,localAddrV4,"localAddrV4");
	   RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,  extAddrV4,"extAddrV4");
	   RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,localAddrV6,"localAddrV6");
	   RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,  extAddrV6,"extAddrV6");
	   RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,currentConnectAddress,"currentConnectAddress");
	   RsTypeSerializer::serial_process           (j,ctx,TLV_TYPE_STR_DYNDNS,dyndns,"dyndns");
	   RsTypeSerializer::serial_process           (j,ctx,localAddrList,"localAddrList");
	   RsTypeSerializer::serial_process           (j,ctx,  extAddrList,"extAddrList");
   }
}

void RsDiscIdentityListItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RS_SERIAL_PROCESS(ownIdentityList);
}

RsDiscItem::RsDiscItem(RsGossipDiscoveryItemType subtype)
    : RsItem( RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_DISC, static_cast<uint8_t>(subtype) )
{
}

RsDiscItem::~RsDiscItem() {}
