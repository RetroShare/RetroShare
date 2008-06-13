
/*
 * libretroshare/src/serialiser: rstlvfileitem.cc
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

#include <iostream>

#include "serialiser/rstlvbase.h"
#include "serialiser/rstlvtypes.h"
#include "serialiser/rsbaseserial.h"

/***
 * #define TLV_FI_DEBUG 1
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


uint16_t RsTlvImage::TlvSize()
{
	uint32_t s = 4; /* header */

	/* collect sizes for both uInts and data length */
	s+= 4;
	s+= binData.TlvSize();

	return s;
}


bool RsTlvImage::SetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	/* must check sizes */
	uint16_t tlvsize = TlvSize();
	uint32_t tlvend  = *offset + tlvsize;

	if (size < tlvend)
		return false; /* not enough space */

	bool ok = true;

		/* start at data[offset] */
	ok &= SetTlvBase(data, tlvend, offset, TLV_TYPE_IMAGE , tlvsize);

	/* add mandatory part */
        ok &= setRawUInt32(data, tlvend, offset, image_type);
	ok &= binData.SetTlv(data, size, offset);

	return ok;


}

bool RsTlvImage::GetTlv(void *data, uint32_t size, uint32_t *offset) /* serialise   */
{
	if (size < *offset + 4)
	{
		return false;
	}

	uint16_t tlvtype = GetTlvType( &(((uint8_t *) data)[*offset])  );
	uint16_t tlvsize = GetTlvSize( &(((uint8_t *) data)[*offset])  );
	uint32_t tlvend = *offset + tlvsize;

	if (size < tlvend)    /* check size */
		return false; /* not enough space */

	if (tlvtype != TLV_TYPE_IMAGE) /* check type */
		return false;

	bool ok = true;

	/* ready to load */
	TlvClear();

	/* skip the header */
	(*offset) += 4;

        /* add mandatory parts first */
        ok &= getRawUInt32(data, tlvend, offset, &(image_type));
	ok &= binData.GetTlv(data, size, offset);

	return ok;

}

/* print it out */
std::ostream &RsTlvImage::print(std::ostream &out, uint16_t indent)
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

