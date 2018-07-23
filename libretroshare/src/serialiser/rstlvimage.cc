/*******************************************************************************
 * libretroshare/src/serialiser: rstlvimage.cc                                 *
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
#include "serialiser/rstlvimage.h"

#if 0
#include <iostream>

#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"
#endif

/***
 * #define TLV_IMG_DEBUG 1
 **/

/************************************* RsTlvImage ************************************/
	
RsTlvImage::RsTlvImage()
	:RsTlvItem(), image_type(0), binData(TLV_TYPE_BIN_IMAGE)
{
	return;
}

void RsTlvImage::TlvClear()
{
	image_type = 0;
	binData.TlvClear();
}


uint32_t RsTlvImage::TlvSize() const
{
	uint32_t s = TLV_HEADER_SIZE; /* header */

	/* collect sizes for both uInts and data length */
	s+= 4;
	s+= binData.TlvSize();

	return s;
}


bool RsTlvImage::SetTlv(void *data, uint32_t size, uint32_t *offset) const
{
#ifdef TLV_IMG_DEBUG
        std::cerr << "RsTlvImage::SetTlv()" << std::endl;
#endif


	/* must check sizes */
	uint32_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
	{
#ifdef TLV_IMG_DEBUG
        	std::cerr << "RsTlvImage::SetTlv() no space" << std::endl;
#endif
		return false; /* not enough space */
	}

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_IMAGE , tlvsize);
#ifdef TLV_IMG_DEBUG
	if (!ok)
        	std::cerr << "RsTlvImage::SetTlv() NOK base" << std::endl;
#endif

	/* add mandatory part */
        ok &= setRawUInt32(data, tlvend, offset, image_type);
#ifdef TLV_IMG_DEBUG
	if (!ok)
        	std::cerr << "RsTlvImage::SetTlv() NOK image" << std::endl;
#endif

	ok &= binData.SetTlv(data, size, offset);
#ifdef TLV_IMG_DEBUG
	if (!ok)
        	std::cerr << "RsTlvImage::SetTlv() NOK binData" << std::endl;
#endif


	return ok;


}

bool RsTlvImage::GetTlv(void *data, uint32_t size, uint32_t *offset) 
{
#ifdef TLV_IMG_DEBUG
       	std::cerr << "RsTlvImage::GetTlv()" << std::endl;
#endif
	if (size < *offset + TLV_HEADER_SIZE)
	{
		return false;
	}

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
    uint32_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
	{
#ifdef TLV_IMG_DEBUG
        	std::cerr << "RsTlvImage::GetTlv() FAIL no space" << std::endl;
#endif
		return false; /* not enough space */
	}

	if (tlvtype != TLV_TYPE_IMAGE) /* check type */
	{
#ifdef TLV_IMG_DEBUG
        	std::cerr << "RsTlvImage::GetTlv() FAIL wrong type" << std::endl;
#endif
		return false;
	}

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += TLV_HEADER_SIZE;

        /* add mandatory parts first */
        ok &= getRawUInt32(data, tlvend, offset, &(image_type));
	ok &= binData.GetTlv(data, size, offset);

#ifdef TLV_IMG_DEBUG
	if (!ok)
        	std::cerr << "RsTlvImage::GetTlv() NOK" << std::endl;
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
		std::cerr << "RsTlvImage::GetTlv() Warning extra bytes at end of item";
		std::cerr << std::endl;
#endif
		*offset = tlvend;
	}

	return ok;

}

/* print it out */
std::ostream &RsTlvImage::print(std::ostream &out, uint16_t indent) const
{
	printBase(out, "RsTlvImage", indent);
	uint16_t int_Indent = indent + 2;

	printIndent(out, int_Indent);
	out << "Image_Type: " << image_type;
	out << std::endl;

	binData.print(out, int_Indent);

	printEnd(out, "RsTlvImage", indent);
	return out;

}

RsTlvImage::RsTlvImage(const RsTlvImage& rightOp)
:RsTlvItem(), image_type(0), binData(TLV_TYPE_BIN_IMAGE)
{
	this->image_type = rightOp.image_type;
	this->binData.setBinData(rightOp.binData.bin_data, rightOp.binData.bin_len);
}
