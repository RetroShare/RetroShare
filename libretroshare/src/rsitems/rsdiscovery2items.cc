/*******************************************************************************
 * libretroshare/src/rsitems: rsdiscitems.cc                                   *
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
#include "rsitems/rsdiscovery2items.h"
#include "serialiser/rsbaseserial.h"

#include "serialiser/rstypeserializer.h"

#if 0

#include "rsitems/rsserviceids.h"

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#endif

/***
 * #define RSSERIAL_DEBUG 		1
 * #define RSSERIAL_ERROR_DEBUG 	1
 ***/

#define RSSERIAL_ERROR_DEBUG 		1

#include <iostream>

RsItem *RsDiscSerialiser::create_item(uint16_t service,uint8_t item_subtype) const
{
    if(service != RS_SERVICE_TYPE_DISC)
        return NULL ;

    switch(item_subtype)
    {
	case RS_PKT_SUBTYPE_DISC_PGP_LIST           : return new RsDiscPgpListItem() ; //= 0x01;
	case RS_PKT_SUBTYPE_DISC_PGP_CERT           : return new RsDiscPgpCertItem() ;      //= 0x02;
	case RS_PKT_SUBTYPE_DISC_CONTACT_deprecated : return NULL ;                         //= 0x03;
#if 0
	case RS_PKT_SUBTYPE_DISC_SERVICES           : return new RsDiscServicesItem();      //= 0x04;
#endif
	case RS_PKT_SUBTYPE_DISC_CONTACT            : return new RsDiscContactItem();       //= 0x05;
	case RS_PKT_SUBTYPE_DISC_IDENTITY_LIST      : return new RsDiscIdentityListItem();  //= 0x06;
    default:
    return NULL ;
    }
}

/*************************************************************************/

void 	RsDiscPgpListItem::clear()
{
	mode = DISC_PGP_LIST_MODE_NONE;
	pgpIdSet.TlvClear();
}

void RsDiscPgpListItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,mode,"mode") ;
    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,pgpIdSet,"pgpIdSet") ;
}

void 	RsDiscPgpCertItem::clear()
{
	pgpId.clear();
	pgpCert.clear();
}


void RsDiscPgpCertItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process(j,ctx,pgpId,"pgpId") ;
    RsTypeSerializer::serial_process(j,ctx,TLV_TYPE_STR_PGPCERT,pgpCert,"pgpCert") ;
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

