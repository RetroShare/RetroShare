/*******************************************************************************
 * libretroshare/src/serialiser: rstlvidset.cc                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie,Chris Parker <retroshare@lunamutt.com> *
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
#include "rstlvidset.h"

// #define TLV_DEBUG 1

/************************************* Service Id Set ************************************/

void RsTlvServiceIdSet::TlvClear()
{
	ids.clear();

}

uint32_t RsTlvServiceIdSet::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	/* determine the total size of ids strings in list */
	std::list<uint32_t>::const_iterator it;
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		if (*it > 0)
			s += GetTlvUInt32Size();
	}

	return s;
}

bool     RsTlvServiceIdSet::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

	
	/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_SERVICESET , tlvsize);

	/* determine the total size of ids strings in list */
	std::list<uint32_t>::const_iterator it;
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		if (*it > 0)
			ok &= SetTlvUInt32(data, tlvend, offset, TLV_TYPE_UINT32_SERID, *it);
	}

	return ok;

}

bool     RsTlvServiceIdSet::GetTlv(void *data, uint32_t size, uint32_t *offset)
{
	if (size < *offset + TLV_HEADER_SIZE)
		return false;	
	
	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_SERVICESET) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

/* while there is TLV  */
	while((*offset) + 2 < tlvend)
	{
		/* get the next type */
		uint16_t tlvsubtype = GetTlvType( &(((uint8_t *) data)[*offset]) );
		
		if (tlvsubtype == TLV_TYPE_UINT32_SERID)
		{
			uint32_t newIds;
			ok &= GetTlvUInt32(data, tlvend, offset, TLV_TYPE_UINT32_SERID, &newIds);
			if(ok)
			{
				ids.push_back(newIds);
				
			}
		}
		else
		{
			/* Step past unknown TLV TYPE */
			ok &= SkipUnknownTlv(data, tlvend, offset);
		}

		if (!ok)
		{
			break;
		}
	}

        /***************************************************************************
         * NB: extra components could be added (for future expansion of the type).
         *            or be present (if this code is reading an extended version).
         *
         * We must chew up the extra characters to conform with TLV specifications
         ***************************************************************************/
        if (*offset != tlvend)
        {
#ifdef TLV_DEBUG
                std::cerr << "RsTlvServiceIdSet::GetTlv() Warning extra bytes at end of item";
                std::cerr << std::endl;
#endif
                *offset = tlvend;
        }

	
	return ok;
}

/// print to screen RsTlvServiceSet contents
std::ostream &RsTlvServiceIdSet::print(std::ostream &out, uint16_t indent) const
{
	printBase(out, "RsTlvServiceIdSet", indent);
	uint16_t int_Indent = indent + 2;

	std::list<uint32_t>::const_iterator it;
	for(it = ids.begin(); it != ids.end() ; ++it)
	{
		printIndent(out, int_Indent);
		out << "id:" << *it;
		out << std::endl;
	}

	printEnd(out, "RsTlvServiceIdSet", indent);
	return out;

}



