
/*
 * libretroshare/src/serialiser: rstlvtypes.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie, Chris Parker
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

#include "serialiser/rstlvbanlist.h"
#include "serialiser/rstlvbase.h"

#include "serialiser/rsbaseserial.h"


/************************************* RsTlvBanListEntry ************************************/

RsTlvBanListEntry::RsTlvBanListEntry()
	:RsTlvItem(), level(0), reason(0), age(0)
{
	return;
}

void RsTlvBanListEntry::TlvClear()
{
	addr.TlvClear();
	level = 0;
	reason = 0;
	age = 0;
}

uint32_t RsTlvBanListEntry::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; 

	s += addr.TlvSize();
	s += 4; // level;
	s += 4; // reason;
	s += 4; // age;

	return s;

}

bool  RsTlvBanListEntry::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	/* start at data[offset] */
        /* add mandatory parts first */

	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_BAN_ENTRY, tlvsize);

	ok &= addr.SetTlv(data, tlvend, offset);
	ok &= setRawUInt32(data, tlvend, offset, level);
	ok &= setRawUInt32(data, tlvend, offset, reason);
	ok &= setRawUInt32(data, tlvend, offset, age);
	return ok;

}


bool  RsTlvBanListEntry::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_BAN_ENTRY) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	ok &= addr.GetTlv(data, tlvend, offset);
	ok &= getRawUInt32(data, tlvend, offset, &(level));
	ok &= getRawUInt32(data, tlvend, offset, &(reason));
	ok &= getRawUInt32(data, tlvend, offset, &(age));

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvBanListEntry::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;
	
}


std::ostream &RsTlvBanListEntry::print(std::ostream &out, uint16_t indent) const
{ 
	printBase(out, "RsTlvBanListEntry", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "addr:" << std::endl;
	addr.print(out, int_Indent);

	printIndent(out, int_Indent);
	out << "level:" << level;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "reason:" << reason;
	out << std::endl;

	printIndent(out, int_Indent);
	out << "age:" << age;
	out << std::endl;

	printEnd(out, "RsTlvBanListEntry", indent);
	return out;
}


