
/*
 * libretroshare/src/serialiser: rsdiscitems.cc
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
	case RS_PKT_SUBTYPE_DISC_PGP_CERT           : return new RsDiscPgpCertItem() ; //= 0x02;
	case RS_PKT_SUBTYPE_DISC_CONTACT_deprecated : return NULL ;                    //= 0x03;
#if 0
	case RS_PKT_SUBTYPE_DISC_SERVICES           : return new RsDiscServicesItem(); //= 0x04;
#endif
	case RS_PKT_SUBTYPE_DISC_CONTACT            : return new RsDiscContactItem();  //= 0x05;
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

/*************************************************************************/
#if 0

RsDiscServicesItem::~RsDiscServicesItem()
{
	return;
}

void 	RsDiscServicesItem::clear()
{
	version.clear();
	mServiceIdMap.TlvClear();
}

std::ostream &RsDiscServicesItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsDiscServicesItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "version: " << version << std::endl;
	mServiceIdMap.print(out, int_Indent);

	printRsItemEnd(out, "RsDiscServicesItem", indent);
	return out;
}


uint32_t    RsDiscSerialiser::sizeServices(RsDiscServicesItem *item)
{
	uint32_t s = 8; /* header */
	s += GetTlvStringSize(item->version); /* version */
	s += item->mServiceIdMap.TlvSize();
	return s;
}

/* serialise the data to the buffer */
bool     RsDiscSerialiser::serialiseServices(RsDiscServicesItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeServices(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDiscSerialiser::serialiseServices() Header: " << ok << std::endl;
	std::cerr << "RsDiscSerialiser::serialiseServices() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VERSION, item->version);
	ok &= item->mServiceIdMap.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize) {
		ok = false;
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::serialiseServices() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsDiscServicesItem *RsDiscSerialiser::deserialiseServices(void *data, uint32_t *pktsize) {
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DISC_PGP_LIST != getRsItemSubType(rstype)))
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseServices() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseServices() Not Enough Space" << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsDiscServicesItem *item = new RsDiscServicesItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VERSION, item->version);
	ok &= item->mServiceIdMap.GetTlv(data, rssize, &offset);

	if (offset != rssize) {
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseServices() offset != rssize" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok) {
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseServices() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}

#endif
