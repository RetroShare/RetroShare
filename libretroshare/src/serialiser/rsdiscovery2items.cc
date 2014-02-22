
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

#include "serialiser/rsbaseserial.h"
#include "serialiser/rsbaseserial.h"

#include "serialiser/rsserviceids.h"
#include "serialiser/rsdiscovery2items.h"

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"

/***
 * #define RSSERIAL_DEBUG 		1
 * #define RSSERIAL_ERROR_DEBUG 	1
 ***/

#define RSSERIAL_ERROR_DEBUG 		1

#include <iostream>

/*************************************************************************/

uint32_t    RsDiscSerialiser::size(RsItem *i)
{
	RsDiscPgpListItem *pgplist;
	RsDiscPgpCertItem *pgpcert;
	RsDiscContactItem *contact;
	//RsDiscServicesItem *services;

	if (NULL != (pgplist = dynamic_cast<RsDiscPgpListItem *>(i)))
	{
		return sizePgpList(pgplist);
	}
	else if (NULL != (pgpcert = dynamic_cast<RsDiscPgpCertItem *>(i)))
	{
		return sizePgpCert(pgpcert);
	}
	else if (NULL != (contact = dynamic_cast<RsDiscContactItem *>(i)))
	{
		return sizeContact(contact);
	}
#if 0
	else if (NULL != (services = dynamic_cast<RsDiscServicesItem *>(i)))
	{
		return sizeServices(services);
	}
#endif
	return 0;
}

/* serialise the data to the buffer */
bool    RsDiscSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsDiscPgpListItem *pgplist;
	RsDiscPgpCertItem *pgpcert;
	RsDiscContactItem *contact;
	//RsDiscServicesItem *services;

	if (NULL != (pgplist = dynamic_cast<RsDiscPgpListItem *>(i)))
	{
		return serialisePgpList(pgplist, data, pktsize);
	}
	else if (NULL != (pgpcert = dynamic_cast<RsDiscPgpCertItem *>(i)))
	{
		return serialisePgpCert(pgpcert, data, pktsize);
	}
	else if (NULL != (contact = dynamic_cast<RsDiscContactItem *>(i)))
	{
		return serialiseContact(contact, data, pktsize);
	}
#if 0
	else if (NULL != (services = dynamic_cast<RsDiscServicesItem *>(i)))
	{
		return serialiseServices(services, data, pktsize);
	}
#endif

	return false;
}

RsItem *RsDiscSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)))
	{
		std::cerr << "RsDiscSerialiser::deserialise() Wrong Type" << std::endl;
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_DISC_PGP_LIST:
			return deserialisePgpList(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_DISC_PGP_CERT:
			return deserialisePgpCert(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_DISC_CONTACT:
			return deserialiseContact(data, pktsize);
			break;
#if 0
		case RS_PKT_SUBTYPE_DISC_SERVICES:
			return deserialiseServices(data, pktsize);
			break;
#endif
		default:
			return NULL;
			break;
	}
	return NULL;
}

/*************************************************************************/

RsDiscPgpListItem::~RsDiscPgpListItem()
{
	return;
}

void 	RsDiscPgpListItem::clear()
{
	mode = DISC_PGP_LIST_MODE_NONE;
	pgpIdSet.TlvClear();
}

std::ostream &RsDiscPgpListItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsDiscPgpListItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "mode: " << mode << std::endl;
	pgpIdSet.print(out, int_Indent);

	printRsItemEnd(out, "RsDiscPgpList", indent);
	return out;
}


uint32_t    RsDiscSerialiser::sizePgpList(RsDiscPgpListItem *item)
{
	uint32_t s = 8; /* header */
	s += 4; /* mode */
	s += item->pgpIdSet.TlvSize();
	return s;
}

/* serialise the data to the buffer */
bool     RsDiscSerialiser::serialisePgpList(RsDiscPgpListItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizePgpList(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDiscSerialiser::serialisePgpList() Header: " << ok << std::endl;
	std::cerr << "RsDiscSerialiser::serialisePgpList() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= setRawUInt32(data, tlvsize, &offset, item->mode);
	ok &= item->pgpIdSet.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize) {
		ok = false;
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::serialisePgpList() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsDiscPgpListItem *RsDiscSerialiser::deserialisePgpList(void *data, uint32_t *pktsize) {
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DISC_PGP_LIST != getRsItemSubType(rstype)))
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialisePgpList() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialisePgpList() Not Enough Space" << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsDiscPgpListItem *item = new RsDiscPgpListItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= getRawUInt32(data, rssize, &offset, &(item->mode));
	ok &= item->pgpIdSet.GetTlv(data, rssize, &offset);

	if (offset != rssize) {
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialisePgpList() offset != rssize" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok) {
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialisePgpList() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}


/*************************************************************************/
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


/*************************************************************************/

RsDiscPgpCertItem::~RsDiscPgpCertItem()
{
	return;
}

void 	RsDiscPgpCertItem::clear()
{
	pgpId.clear();
	pgpCert.clear();
}

std::ostream &RsDiscPgpCertItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsDiscPgpCertItem", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "pgpId: " << pgpId << std::endl;
	printIndent(out, int_Indent);
	out << "pgpCert: " << pgpCert << std::endl;

	printRsItemEnd(out, "RsDiscPgpCert", indent);
	return out;
}


uint32_t    RsDiscSerialiser::sizePgpCert(RsDiscPgpCertItem *item)
{
	uint32_t s = 8; /* header */
	s += item->pgpId.serial_size();
	s += GetTlvStringSize(item->pgpCert);
	return s;
}

/* serialise the data to the buffer */
bool     RsDiscSerialiser::serialisePgpCert(RsDiscPgpCertItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizePgpCert(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDiscSerialiser::serialisePgpCert() Header: " << ok << std::endl;
	std::cerr << "RsDiscSerialiser::serialisePgpCert() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->pgpId.serialise(data, tlvsize, offset) ;
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_PGPCERT, item->pgpCert);

	if (offset != tlvsize) {
		ok = false;
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::serialisePgpCert() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsDiscPgpCertItem *RsDiscSerialiser::deserialisePgpCert(void *data, uint32_t *pktsize) {
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DISC_PGP_CERT != getRsItemSubType(rstype)))
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialisePgpCert() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialisePgpCert() Not Enough Space" << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsDiscPgpCertItem *item = new RsDiscPgpCertItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= item->pgpId.deserialise(data, rssize, offset) ;
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_PGPCERT, item->pgpCert);

	if (offset != rssize) {
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialisePgpCert() offset != rssize" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok) {
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialisePgpCert() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}


/*************************************************************************/


RsDiscContactItem::~RsDiscContactItem()
{
	return;
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

std::ostream &RsDiscContactItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsDiscContact", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "pgpId:  " << pgpId  << std::endl;

	printIndent(out, int_Indent);
	out << "sslId:  " << sslId  << std::endl;

	printIndent(out, int_Indent);
	out << "location:  " << location  << std::endl;

	printIndent(out, int_Indent);
	out << "version:  " << version  << std::endl;

	printIndent(out, int_Indent);
	out << "netMode: " << netMode << std::endl;

	printIndent(out, int_Indent);
	out << "vs_disc: " << vs_disc << std::endl;

	printIndent(out, int_Indent);
	out << "vs_dht: " << vs_dht << std::endl;

	printIndent(out, int_Indent);
	out << "lastContact:  " << lastContact  << std::endl;

	if (isHidden)
	{
		printIndent(out, int_Indent);
		out << "hiddenAddr: " << hiddenAddr << std::endl;

		printIndent(out, int_Indent);
		out << "hiddenPort: " << hiddenPort << std::endl;
	}
	else
	{
		printIndent(out, int_Indent);
		out << "localAddrV4: " << std::endl;
		localAddrV4.print(out, int_Indent);

		printIndent(out, int_Indent);
		out << "extAddrV4: " << std::endl;
		extAddrV4.print(out, int_Indent);

		printIndent(out, int_Indent);
		out << "localAddrV6: " << std::endl;
		localAddrV6.print(out, int_Indent);

		printIndent(out, int_Indent);
		out << "extAddrV6: " << std::endl;
		extAddrV6.print(out, int_Indent);

		printIndent(out, int_Indent);
		out << "DynDNS: " << dyndns << std::endl;

		printIndent(out, int_Indent);
		out << "localAddrList: " << std::endl;
		localAddrList.print(out, int_Indent);

		printIndent(out, int_Indent);
		out << "extAddrList: " << std::endl;
		extAddrList.print(out, int_Indent);
	}

	printRsItemEnd(out, "RsDiscContact", indent);
	return out;
}


uint32_t    RsDiscSerialiser::sizeContact(RsDiscContactItem *item)
{
	uint32_t s = 8; /* header */
	s += item->pgpId.serial_size();
	s += item->sslId.serial_size();

	s += GetTlvStringSize(item->location);
	s += GetTlvStringSize(item->version);

	s += 4; // netMode
	s += 2; // vs_disc
	s += 2; // vs_dht
	s += 4; // last contact

	if (item->isHidden)
	{
		s += GetTlvStringSize(item->hiddenAddr);
		s += 2; /* hidden port */
	}
	else
	{
		s += item->localAddrV4.TlvSize(); /* localaddr */
		s += item->extAddrV4.TlvSize(); /* remoteaddr */

		s += item->localAddrV6.TlvSize(); /* localaddr */
		s += item->extAddrV6.TlvSize(); /* remoteaddr */

		s += GetTlvStringSize(item->dyndns);

		//add the size of the ip list
		s += item->localAddrList.TlvSize();
		s += item->extAddrList.TlvSize();
	}

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDiscSerialiser::sizeContact() Total Size: " << s << std::endl;
#endif

	return s;
}

/* serialise the data to the buffer */
bool     RsDiscSerialiser::serialiseContact(RsDiscContactItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeContact(item);
	uint32_t offset = 0;

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDiscSerialiser::serialiseContact() tlvsize: " << tlvsize;
	std::cerr << std::endl;
#endif

	if (*pktsize < tlvsize)
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::serialiseContact() ERROR not enough space" << std::endl;
		std::cerr << "RsDiscSerialiser::serialiseContact() ERROR *pktsize: " << *pktsize << " tlvsize: " << tlvsize;
		std::cerr << std::endl;
#endif
		return false; /* not enough space */
	}

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDiscSerialiser::serialiseContact() Header: " << ok << std::endl;
	std::cerr << "RsDiscSerialiser::serialiseContact() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= item->pgpId.serialise(data, tlvsize, offset) ;
	ok &= item->sslId.serialise(data, tlvsize, offset) ;

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_LOCATION, item->location); 
	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VERSION, item->version); 

	ok &= setRawUInt32(data, tlvsize, &offset, item->netMode); 
	ok &= setRawUInt16(data, tlvsize, &offset, item->vs_disc); 
	ok &= setRawUInt16(data, tlvsize, &offset, item->vs_dht); 
	ok &= setRawUInt32(data, tlvsize, &offset, item->lastContact); /* Mandatory */

	if (item->isHidden)
	{
		ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_DOMADDR, item->hiddenAddr);
		ok &= setRawUInt16(data, tlvsize, &offset, item->hiddenPort); 
	}
	else
	{
		ok &= item->localAddrV4.SetTlv(data, tlvsize, &offset);
		ok &= item->extAddrV4.SetTlv(data, tlvsize, &offset);
		ok &= item->localAddrV6.SetTlv(data, tlvsize, &offset);
		ok &= item->extAddrV6.SetTlv(data, tlvsize, &offset);

		ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_DYNDNS, item->dyndns);

		ok &= item->localAddrList.SetTlv(data, tlvsize, &offset);
		ok &= item->extAddrList.SetTlv(data, tlvsize, &offset);
	}


	if (offset != tlvsize) 
	{
		ok = false;
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::serialiseContact() Size Error: " << tlvsize << " != " << offset << std::endl;
#endif
	}

	return ok;
}

RsDiscContactItem *RsDiscSerialiser::deserialiseContact(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t rssize = getRsItemSize(data);

	uint32_t offset = 0;

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsDiscSerialiser::deserialiseContact() Pkt Type: " << std::hex << rstype << std::dec;
	std::cerr << "RsDiscSerialiser::deserialiseContact() Pkt Size: " << rssize << std::endl;
#endif

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_DISC != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_DISC_CONTACT != getRsItemSubType(rstype)))
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseContact() Wrong Type" << std::endl;
#endif
		return NULL; /* wrong type */
	}

	if (*pktsize < rssize)    /* check size */
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseContact() Not enough space" << std::endl;
#endif
		return NULL; /* not enough data */
	}

	/* set the packet length */
	*pktsize = rssize;

	bool ok = true;

	/* ready to load */
	RsDiscContactItem *item = new RsDiscContactItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= item->pgpId.deserialise(data, rssize, offset) ;
	ok &= item->sslId.deserialise(data, rssize, offset) ;

	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_LOCATION, item->location); 
	ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_VERSION, item->version); 

	ok &= getRawUInt32(data, rssize, &offset, &(item->netMode)); /* Mandatory */
	ok &= getRawUInt16(data, rssize, &offset, &(item->vs_disc)); /* Mandatory */
	ok &= getRawUInt16(data, rssize, &offset, &(item->vs_dht)); /* Mandatory */
	ok &= getRawUInt32(data, rssize, &offset, &(item->lastContact));

	if (rssize < offset + TLV_HEADER_SIZE)
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseContact() missized" << std::endl;
#endif
		/* no extra */
		delete item;
		return NULL;
	}

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[offset])  );

	if (tlvtype == TLV_TYPE_STR_DOMADDR)
	{
		item->isHidden = true;

		ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_DOMADDR, item->hiddenAddr);
		ok &= getRawUInt16(data, rssize, &offset, &(item->hiddenPort)); /* Mandatory */

	}
	else
	{
		item->isHidden = false;

		ok &= item->localAddrV4.GetTlv(data, rssize, &offset);
		ok &= item->extAddrV4.GetTlv(data, rssize, &offset);
		ok &= item->localAddrV6.GetTlv(data, rssize, &offset);
		ok &= item->extAddrV6.GetTlv(data, rssize, &offset);

		ok &= GetTlvString(data, rssize, &offset, TLV_TYPE_STR_DYNDNS, item->dyndns);
		ok &= item->localAddrList.GetTlv(data, rssize, &offset);
		ok &= item->extAddrList.GetTlv(data, rssize, &offset);
	}


	if (offset != rssize) 
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseContact() offset != rssize" << std::endl;
#endif
		/* error */
		delete item;
		return NULL;
	}

	if (!ok) 
	{
#ifdef RSSERIAL_ERROR_DEBUG
		std::cerr << "RsDiscSerialiser::deserialiseContact() ok = false" << std::endl;
#endif
		delete item;
		return NULL;
	}

	return item;
}

/*************************************************************************/

