/*******************************************************************************
 * libretroshare/src/rsitems: rsserviceinfoitems.cc                            *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include "serialiser/rstypeserializer.h"
#include "rsitems/rsserviceinfoitems.h"

/***
#define RSSERIAL_DEBUG 1
***/

#include <iostream>

/*************************************************************************/
/***** RsServiceInfo ****/
/*************************************************************************/

void 	RsServiceInfoListItem::clear()
{
	mServiceInfo.clear();
}

void RsServiceInfoListItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
	RsTlvServiceInfoMapRef map(mServiceInfo);

    RsTypeSerializer::serial_process<RsTlvItem>(j,ctx,map,"map") ;
}

void RsServiceInfoPermissionsItem::serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx)
{
    RsTypeSerializer::serial_process<uint32_t>(j,ctx,allowedBw,"allowedBw") ;
}

RsItem *RsServiceInfoSerialiser::create_item(uint16_t service, uint8_t item_sub_id) const
{
    if(service != RS_SERVICE_TYPE_SERVICEINFO)
        return NULL ;

    switch(item_sub_id)
    {
    case RS_PKT_SUBTYPE_SERVICELIST_ITEM: return new RsServiceInfoListItem() ;
    case RS_PKT_SUBTYPE_SERVICEPERMISSIONS_ITEM: return new RsServiceInfoPermissionsItem() ;
    default:
        return NULL ;
    }
}




template<> std::ostream& RsTlvParamRef<RsServiceInfo>::print(std::ostream &out, uint16_t /*indent*/) const
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

