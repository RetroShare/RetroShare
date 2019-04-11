/*******************************************************************************
 * libretroshare/src/serialiser: rstlvitem.cc                                  *
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
#include "rstlvitem.h"
#include "rstlvbase.h"
#include <iostream>

#if 0
#include "rsbaseserial.h"
#include "util/rsprint.h"
#include <ostream>
#include <sstream>
#include <iomanip>
#endif

// #define TLV_DEBUG 1

void  	RsTlvItem::TlvShallowClear()
{
	TlvClear(); /* unless overloaded! */
}

std::ostream &RsTlvItem::printBase(std::ostream &out, std::string clsName, uint16_t indent) const
{
	printIndent(out, indent);
	out << "RsTlvItem: " << clsName << " Size: " << TlvSize() << "  ***********************";
	out << std::endl;
	return out;
}

std::ostream &RsTlvItem::printEnd(std::ostream &out, std::string clsName, uint16_t indent) const
{
	printIndent(out, indent);
	out << "********************** " << clsName << " *********************";
	out << std::endl;
	return out;
}

std::ostream &printIndent(std::ostream &out, uint16_t indent)
{
	for(int i = 0; i < indent; i++)
	{
		out << " ";
	}
	return out;
}


	
RsTlvUnit::RsTlvUnit(const uint16_t tlv_type)
	:RsTlvItem(), mTlvType(tlv_type)
{
	return;
}

uint32_t RsTlvUnit::TlvSize() const
{
	return TLV_HEADER_SIZE + TlvSizeUnit();
}

/* serialise   */
bool RsTlvUnit::SetTlv(void *data, uint32_t size, uint32_t *offset) const 
{
#ifdef TLV_DEBUG
        std::cerr << "RsTlvUnit::SetTlv()" << std::endl;
#endif

	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
	{
#ifdef TLV_DEBUG
        	std::cerr << "RsTlvImage::SetTlv() ERROR not enough space" << std::endl;
#endif
		return false; /* not enough space */
	}

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_IMAGE , tlvsize);

	if (!ok)
	{
#ifdef TLV_DEBUG
        	std::cerr << "RsTlvUnit::SetTlv() ERROR Setting base" << std::endl;
#endif
		return false;
	}

	ok &= SetTlvUnit(data, tlvend, offset);

#ifdef TLV_DEBUG
	if (!ok)
        	std::cerr << "RsTlvUnit::SetTlv() ERROR in SetTlvUnit" << std::endl;
#endif
	return ok;
}

/* deserialise  */
bool RsTlvUnit::GetTlv(void *data, uint32_t size, uint32_t *offset) 
{
	if (size < *offset + TLV_HEADER_SIZE)
	{
#ifdef TLV_DEBUG
       		std::cerr << "RsTlvUnit::GetTlv() ERROR not enough size for header";
		std::cerr << std::endl;
#endif
		return false;
	}

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
	{
#ifdef TLV_IMG_DEBUG
        	std::cerr << "RsTlvImage::GetTlv() ERROR no space";
		std::cerr << std::endl;
#endif
		return false; /* not enough space */
	}

	if (tlvtype != mTlvType) /* check type */
	{
#ifdef TLV_IMG_DEBUG
        	std::cerr << "RsTlvImage::GetTlv() ERROR wrong type";
		std::cerr << std::endl;
#endif
		return false;
	}

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

	/* extract components */
	ok &= GetTlvUnit(data, tlvend, offset);

#ifdef TLV_IMG_DEBUG
	if (!ok)
	{
        	std::cerr << "RsTlvUnit::GetTlv() ERROR GetTlvUnit() NOK";
		std::cerr << std::endl;
	}
#endif

	/***************************************************************************
	 * NB: extra components could be added (for future expansion of the type).
	 *            or be present (if this code is reading an extended version).
	 *
	 * We must chew up the extra characters to conform with TLV specifications
	 ***************************************************************************/
	if (*offset != tlvend)
	{
#ifdef TLV_DEBUG
		std::cerr << "RsTlvUnit::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;

}


