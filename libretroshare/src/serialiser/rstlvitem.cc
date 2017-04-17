
/*
 * libretroshare/src/serialiser: rstlvtypes.cc
 *
 * RetroShare Serialiser.
 *
 * Copyright 2007-2008 by Robert Fernie, Chris Parker
 * Copyright (C) 2016 Gioacchino Mazzurco <gio@eigenlab.org>
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

#include "rstlvitem.h"
#include "rstlvbase.h"
#include "util/stacktrace.h"
#include <iostream>

bool RsAutoTlvSerializable::serialize(uint8_t data[], uint32_t size, uint32_t &offset) const
{
	if ( !data || size <= offset || size - offset < TLV_HEADER_SIZE )
		return false;

	uint32_t origOffset = offset;
	uint32_t * lenghtPtr = (uint32_t *)(data + offset + TLV_HEADER_TYPE_SIZE);

	if(!RsAutoSerializable::serialize(data, size, offset))
	{
		std::cerr << "RsAutoTlvItem::serialize(...) failed! Inform Developers!"
		          << std::endl;
		print_stacktrace();
		return false;
	}

	uint32_t expectedSize = RsAutoSerializable::serialSize();
	uint32_t writtenSize = offset - origOffset;
	if (writtenSize != expectedSize)
	{
		std::cerr << "RsAutoTlvItem::serialize(...) writtenSize and "
		          << "expectedSize doesn't match. " << writtenSize << " "
		          << expectedSize << " Inform Developers!" << std::endl;
	}

	*lenghtPtr = hton<uint32_t>(writtenSize - TLV_HEADER_SIZE);

	return true;
}

bool RsAutoTlvSerializable::deserialize(const uint8_t data[], uint32_t size,
                                uint32_t &offset)
{
	if ( !data || size <= offset || size - offset < TLV_HEADER_SIZE )
		return false;

	bool ok = true;
	uint32_t origOffest = offset; // Backup offset
	uint16_t tlvType = ntoh<uint16_t>(*((uint16_t *)(data + offset)));

	if( (at_type & CHECK_TYPE) == CHECK_TYPE)
	{
		if(!ok)
		{
			std::cerr << "RsAutoTlvItem::deserialize(...) failed deserializing "
			          << "type. Inform Developers!" << std::endl;
			print_stacktrace();
			return false;
		}

		if ( at_type != tlvType )
		{
			std::cerr << "RsAutoTlvItem::deserialize(...) Warning wrong TLV "
			          << "type while CHECK_TYPE is set got: " << tlvType
			          << " expected: " << at_type << " ";
			if ( (at_type & MANDATORY) == MANDATORY )
			{
				std::cerr << "which has MANDATORY flags set skipping this TLV"
				          << "and try to populate this member with next TLV."
				          << std::endl;

				ok = ok && skipTlv(data, size, offset);
				return ok && deserialize(data, size, offset);
			}
			else
			{
				std::cerr << "which has MANDATORY unset skipping member not TLV"
				          << std::endl;
				return ok;
			}

			std::cerr << "Inform Developers!" << std::endl;
			print_stacktrace();
		}
	}

	ok = ok && RsAutoSerializable::deserialize(data, size, offset);

	uint32_t tlvEnd = origOffest + at_lenght;
	if (tlvEnd < offset)
	{
		std::cerr << "RsAutoTlvItem::deserialize(...) Warning ignored extra"
		          << "bytes at end of item. Inform Developers!" << std::endl;
		print_stacktrace();
		offset = tlvEnd;
	}

	return ok;
}

bool RsAutoTlvSerializable::skipTlv(const uint8_t data[], uint32_t size,
                            uint32_t &offset)
{
	if ( !data || size <= offset || size - offset < TLV_HEADER_SIZE )
		return false;

	uint32_t tLen = *((const uint32_t *)(data + offset + TLV_HEADER_TYPE_SIZE));
	tLen = ntoh<uint32_t>(tLen);
	const uint32_t bufferSize = size - offset - TLV_HEADER_TYPE_SIZE;
	if ( bufferSize < tLen)
	{
		std::cerr << "RsAutoTlvItem::skipTlv(...) FAILED - not enough space. "
		          << "bufferSize: " << bufferSize << " tlvLenght: "
		          << tLen << std::endl;
		return false;
	}

	offset += TLV_HEADER_TYPE_SIZE;
	offset += tLen;
	return true;
}

static void print_wrong_usage()
{
	std::cerr << "You are using an RsTlvItem like it was an RsAutoTlvItem or "
	          << "vice versa. Reimplement your TLV item on top of RsAutoTlvItem"
	          << " instead of relying on deprecated, type unsafe code."
	          << std::endl;
	print_stacktrace();
}

void RsTlvItem::TlvClear() { print_wrong_usage(); }

std::ostream &RsTlvItem::printBase(std::ostream &out, std::string clsName,
                                   uint16_t indent) const
{
	printIndent(out, indent);
	out << "RsTlvItem: " << clsName << " Size: " << TlvSize()
	    << "  ***********************" << std::endl;
	return out;
}

std::ostream &RsTlvItem::printEnd(std::ostream &out, std::string clsName,
                                  uint16_t indent) const
{
	printIndent(out, indent);
	out << "***************** " << clsName << " ****************" << std::endl;
	return out;
}

std::ostream &printIndent(std::ostream &out, uint16_t indent)
{
	for(int i = 0; i < indent; i++) out << " ";
	return out;
}

bool RsTlvItem::deserialize(const uint8_t data[], uint32_t size,
                            uint32_t &offset)
{
	print_wrong_usage();
	return GetTlv((void *) data, size, &offset);
}

bool RsTlvItem::serialize(uint8_t data[], uint32_t size, uint32_t &offset) const
{
	print_wrong_usage();
	return SetTlv((void *)data, size, &offset);
}

uint32_t RsTlvItem::serialSize() const
{
	print_wrong_usage();
	return TlvSize();
}

uint32_t RsTlvItem::TlvSize() const
{
	print_wrong_usage();
	return 0;
}

std::ostream & RsTlvItem::print(std::ostream &out, uint16_t /*indent*/) const
{
	print_wrong_usage();
	return out;
}
