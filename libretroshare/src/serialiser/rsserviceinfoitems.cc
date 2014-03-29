/*
 * libretroshare/src/serialiser: rsserviceinfoitems.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2014 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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
#include "serialiser/rsserviceinfoitems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/
/***** RsServiceInfo ****/

template<>
std::ostream &RsTlvParamRef<RsServiceInfo>::print(std::ostream &out, uint16_t indent) const
{
	out << "RsServiceInfo: " << mParam.mServiceType << " name " << mParam.mServiceName;
	out << std::endl;
	out << "Version(" << mParam.mVersionMajor << "," << mParam.mVersionMinor << ")";
	out << " MinVersion(" << mParam.mMinVersionMajor << "," << mParam.mMinVersionMinor << ")";
	out << std::endl;
	return out;
}

template<>
uint32_t RsTlvParamRef<RsServiceInfo>::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header + 4 for size */

	s += getRawStringSize(mParam.mServiceName);
	s += 4; // type.
	s += 4; // version.
	s += 4; // min version.
	return s;
}

template<>
void RsTlvParamRef<RsServiceInfo>::TlvClear()
{
	mParam = RsServiceInfo();
	mParam.mServiceName.clear();
}

template<>
bool RsTlvParamRef<RsServiceInfo>::SetTlv(void *data, uint32_t size, uint32_t *offset)  const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
	{
		return false; /* not enough space */
	}

	bool ok = true;

	ok &= SetTlvBase(data, tlvend, offset, mParamType, tlvsize);
	ok &= setRawString(data, tlvend, offset, mParam.mServiceName);
	ok &= setRawUInt32(data, tlvend, offset, mParam.mServiceType);
	ok &= setRawUInt16(data, tlvend, offset, mParam.mVersionMajor);
	ok &= setRawUInt16(data, tlvend, offset, mParam.mVersionMinor);
	ok &= setRawUInt16(data, tlvend, offset, mParam.mMinVersionMajor);
	ok &= setRawUInt16(data, tlvend, offset, mParam.mMinVersionMinor);

	if (!ok)
	{
		std::cerr << "RsTlvParamRef<RsServiceInfo>::SetTlv() Failed";
		std::cerr << std::endl;
	}

	return ok;
}

template<>
bool RsTlvParamRef<RsServiceInfo>::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
	{
		return false; /* not enough space */
	}

	if (tlvtype != mParamType) /* check type */
	{
		return false;
	}

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	ok &= getRawString(data, tlvend, offset, mParam.mServiceName);
	ok &= getRawUInt32(data, tlvend, offset, &(mParam.mServiceType));
	ok &= getRawUInt16(data, tlvend, offset, &(mParam.mVersionMajor));
	ok &= getRawUInt16(data, tlvend, offset, &(mParam.mVersionMinor));
	ok &= getRawUInt16(data, tlvend, offset, &(mParam.mMinVersionMajor));
	ok &= getRawUInt16(data, tlvend, offset, &(mParam.mMinVersionMinor));

	return ok;
}

template class RsTlvParamRef<RsServiceInfo>;

/*************************************************************************/

RsServiceInfoListItem::~RsServiceInfoListItem()
{
	return;
}

void 	RsServiceInfoListItem::clear()
{
	mServiceInfo.clear();
}

std::ostream &RsServiceInfoListItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsServiceInfoListItem", indent);
	uint16_t int_Indent = indent + 2;
	RsTlvServiceInfoMapRef map(mServiceInfo);
	map.print(out, int_Indent);
	out << std::endl;

	printRsItemEnd(out, "RsServiceInfoListItem", indent);
	return out;
}


uint32_t    RsServiceInfoSerialiser::sizeInfo(RsServiceInfoListItem *item)
{
	uint32_t s = 8; /* header */
	RsTlvServiceInfoMapRef map(item->mServiceInfo);
	s += map.TlvSize();

	return s;
}

/* serialise the data to the buffer */
bool     RsServiceInfoSerialiser::serialiseInfo(RsServiceInfoListItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizeInfo(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsServiceInfoSerialiser::serialiseInfo() Header: " << ok << std::endl;
	std::cerr << "RsServiceInfoSerialiser::serialiseInfo() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	RsTlvServiceInfoMapRef map(item->mServiceInfo);
	ok &= map.SetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsServiceInfoSerialiser::serialiseInfo() Size Error! " << std::endl;
#endif
	}
	return ok;
}

RsServiceInfoListItem *RsServiceInfoSerialiser::deserialiseInfo(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t tlvsize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_SERVICEINFO != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_SERVICELIST_ITEM != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < tlvsize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = tlvsize;

	bool ok = true;

	/* ready to load */
	RsServiceInfoListItem *item = new RsServiceInfoListItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	RsTlvServiceInfoMapRef map(item->mServiceInfo);
	ok &= map.GetTlv(data, tlvsize, &offset);

	if (offset != tlvsize)
	{
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}

/*************************************************************************/
/*************************************************************************/

RsServiceInfoPermissionsItem::~RsServiceInfoPermissionsItem()
{
	return;
}

void 	RsServiceInfoPermissionsItem::clear()
{
	allowedBw = 0;
}

std::ostream &RsServiceInfoPermissionsItem::print(std::ostream &out, uint16_t indent)
{
	printRsItemBase(out, "RsServiceInfoPermissionsItem", indent);
	uint16_t int_Indent = indent + 2;
	printIndent(out, int_Indent);
	out << "AllowedBw: " << allowedBw;
	out << std::endl;

	printRsItemEnd(out, "RsServiceInfoPermissionsItem", indent);
	return out;
}


uint32_t    RsServiceInfoSerialiser::sizePermissions(RsServiceInfoPermissionsItem * /*item*/)
{
	uint32_t s = 8; /* header */
	s += GetTlvUInt32Size();

	return s;
}

/* serialise the data to the buffer */
bool     RsServiceInfoSerialiser::serialisePermissions(RsServiceInfoPermissionsItem *item, void *data, uint32_t *pktsize)
{
	uint32_t tlvsize = sizePermissions(item);
	uint32_t offset = 0;

	if (*pktsize < tlvsize)
		return false; /* not enough space */

	*pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

#ifdef RSSERIAL_DEBUG
	std::cerr << "RsServiceInfoSerialiser::serialisePermissions() Header: " << ok << std::endl;
	std::cerr << "RsServiceInfoSerialiser::serialisePermissions() Size: " << tlvsize << std::endl;
#endif

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */
	ok &= SetTlvUInt32(data, tlvsize, &offset, TLV_TYPE_UINT32_BW, item->allowedBw);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsServiceInfoSerialiser::serialisePermissions() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsServiceInfoPermissionsItem *RsServiceInfoSerialiser::deserialisePermissions(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);
	uint32_t tlvsize = getRsItemSize(data);

	uint32_t offset = 0;


	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_SERVICEINFO != getRsItemService(rstype)) ||
		(RS_PKT_SUBTYPE_SERVICEPERMISSIONS_ITEM != getRsItemSubType(rstype)))
	{
		return NULL; /* wrong type */
	}

	if (*pktsize < tlvsize)    /* check size */
		return NULL; /* not enough data */

	/* set the packet length */
	*pktsize = tlvsize;

	bool ok = true;

	/* ready to load */
	RsServiceInfoPermissionsItem *item = new RsServiceInfoPermissionsItem();
	item->clear();

	/* skip the header */
	offset += 8;

	/* get mandatory parts first */
	ok &= GetTlvUInt32(data, tlvsize, &offset, TLV_TYPE_UINT32_BW, &(item->allowedBw));


	if (offset != tlvsize)
	{
		/* error */
		delete item;
		return NULL;
	}

	if (!ok)
	{
		delete item;
		return NULL;
	}

	return item;
}

/*************************************************************************/

uint32_t    RsServiceInfoSerialiser::size(RsItem *i)
{
	RsServiceInfoListItem *sli;
	RsServiceInfoPermissionsItem *spi;

	if (NULL != (sli = dynamic_cast<RsServiceInfoListItem *>(i)))
	{
		return sizeInfo(sli);
	}
	if (NULL != (spi = dynamic_cast<RsServiceInfoPermissionsItem *>(i)))
	{
		return sizePermissions(spi);
	}
	return 0;
}

bool     RsServiceInfoSerialiser::serialise(RsItem *i, void *data, uint32_t *pktsize)
{
	RsServiceInfoListItem *sli;
	RsServiceInfoPermissionsItem *spi;

	if (NULL != (sli = dynamic_cast<RsServiceInfoListItem *>(i)))
	{
		return serialiseInfo(sli, data, pktsize);
	}
	if (NULL != (spi = dynamic_cast<RsServiceInfoPermissionsItem *>(i)))
	{
		return serialisePermissions(spi, data, pktsize);
	}
	return false;
}

RsItem *RsServiceInfoSerialiser::deserialise(void *data, uint32_t *pktsize)
{
	/* get the type and size */
	uint32_t rstype = getRsItemId(data);

	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		(RS_SERVICE_TYPE_SERVICEINFO != getRsItemService(rstype)))
	{
		return NULL; /* wrong type */
	}

	switch(getRsItemSubType(rstype))
	{
		case RS_PKT_SUBTYPE_SERVICELIST_ITEM:
			return deserialiseInfo(data, pktsize);
			break;
		case RS_PKT_SUBTYPE_SERVICEPERMISSIONS_ITEM:
			return deserialisePermissions(data, pktsize);
			break;
		default:
			return NULL;
			break;
	}
}

/*************************************************************************/



