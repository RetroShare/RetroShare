
/*
 * libretroshare/src/serialiser: rstlvmsgs.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2014 by Robert Fernie
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

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvmail.h"
#include "serialiser/rsbaseserial.h"

/************************************* RsTlvIpAddress ************************************/

RsTlvMailAddress::RsTlvMailAddress()
	:RsTlvItem()
{
	return;
}

void RsTlvMailAddress::TlvClear()
{
	mAddressType = 0;
	mAddress.clear();
}

uint32_t RsTlvMailAddress::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; 
	s += 4; // TYPE.
	s += GetTlvStringSize(mAddress);
	return s;
}

bool  RsTlvMailAddress::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	/* start at data[offset] */
        /* add mandatory parts first */

	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_MSG_ADDRESS, tlvsize);

	ok &= setRawUInt32(data, tlvend, offset, mAddressType);
	ok &= SetTlvString(data, tlvend, offset, TLV_TYPE_STR_MSG, mAddress);
	return ok;

}


bool  RsTlvMailAddress::GetTlv(void *data, uint32_t size, uint32_t *offset) 
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_MSG_ADDRESS) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	ok &= getRawUInt32(data, tlvend, offset, &(mAddressType));
	ok &= GetTlvString(data, tlvend, offset, TLV_TYPE_STR_MSG, mAddress);

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvIpAddress::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
	
}


std::ostream &RsTlvMailAddress::print(std::ostream &out, uint16_t indent) const
{ 
	printBase(out, "RsTlvMailAddress", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "AddressType:" << mAddressType << std::endl;
	printIndent(out, int_Indent);
	out << "Address:" << mAddress << std::endl;
	
	printEnd(out, "RsTlvMailAddress", indent);
	return out;
}




/************************************* RsTlvIpAddressInfo ************************************/

RsTlvMailId::RsTlvMailId()
	:RsTlvItem(), mSentTime(0)
{
	mMailFrom.TlvClear();
	mMailDest.TlvClear();
	mMailId.clear();
	return;
}

void RsTlvMailId::TlvClear()
{
	mMailFrom.TlvClear();
	mMailDest.TlvClear();
	mSentTime = 0;
	mMailId.clear();
}

uint32_t RsTlvMailId::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; 

	s += mMailFrom.TlvSize();
	s += mMailDest.TlvSize();
	s += 4; // SentTime
	s += mMailId.serial_size();

	return s;

}

bool  RsTlvMailId::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	/* start at data[offset] */
        /* add mandatory parts first */

	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_MSG_ID, tlvsize);

	ok &= mMailFrom.SetTlv(data, tlvend, offset);
	ok &= mMailDest.SetTlv(data, tlvend, offset);
	ok &= setRawUInt32(data, tlvend, offset, mSentTime);
	ok &= mMailId.serialise(data, tlvend, *offset);

	return ok;

}


bool  RsTlvMailId::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_MSG_ID) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	ok &= mMailFrom.GetTlv(data, tlvend, offset);
	ok &= mMailDest.GetTlv(data, tlvend, offset);
	ok &= getRawUInt32(data, tlvend, offset, &(mSentTime));
	ok &= mMailId.deserialise(data, tlvend, *offset);


	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvMailId::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
	
}


std::ostream &RsTlvMailId::print(std::ostream &out, uint16_t indent) const
{ 
	printBase(out, "RsTlvMailId", indent);
	uint16_t int_Indent = indent + 2;

	mMailFrom.print(out, int_Indent);
	mMailDest.print(out, int_Indent);

	printIndent(out, int_Indent);
	out << "SentTime:" << mSentTime;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "mMailId:" << mMailId.toStdString();
	out << std::endl;
	
	printEnd(out, "RsTlvMailId", indent);
	return out;
}

