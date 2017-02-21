/*
 * GXS Mailing Service
 * Copyright (C) 2016-2017  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "serialiser/rsgxsmailitems.h"

const RsGxsId RsGxsMailItem::allRecipientsHint("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");


bool RsGxsMailBaseItem::serialize(uint8_t* data, uint32_t size,
                                  uint32_t& offset) const
{
	bool ok = setRsItemHeader(data, size, PacketId(), size);
	ok = ok && (offset += 8); // Take header in account
	ok = ok && setRawUInt64(data, size, &offset, mailId);
	return ok;
}

bool RsGxsMailBaseItem::deserialize(const uint8_t* data, uint32_t& size,
                                    uint32_t& offset)
{
	void* dataPtr = reinterpret_cast<void*>(const_cast<uint8_t*>(data));
	uint32_t rssize = getRsItemSize(dataPtr);
	uint32_t roffset = offset + 8; // Take header in account
	bool ok = rssize <= size;
	ok = ok && getRawUInt64(dataPtr, rssize, &roffset, &mailId);
	if(ok) { size = rssize; offset = roffset; }
	else size = 0;
	return ok;
}

std::ostream& RsGxsMailBaseItem::print(std::ostream &out, uint16_t)
{ return out << " RsGxsMailBaseItem::mailId: " << mailId; }

bool RsGxsMailSerializer::serialise(RsItem* item, void* data, uint32_t* size)
{
	uint32_t itemSize = RsGxsMailSerializer::size(item);
	if(*size < itemSize)
	{
		std::cout << "RsGxsMailSerializer::serialise(...) failed due to wrong size: "
		          << size << " < " << itemSize << std::endl;
		return false;
	}

	uint8_t* dataPtr = reinterpret_cast<uint8_t*>(data);
	bool ok = true;
	switch(item->PacketSubType())
	{
	case GXS_MAIL_SUBTYPE_MAIL:
	{
		uint32_t offset = 0;
		RsGxsMailItem* i = dynamic_cast<RsGxsMailItem*>(item);
		ok = i->serialize(dataPtr, itemSize, offset);
		break;
	}
	case GXS_MAIL_SUBTYPE_RECEIPT:
	{
		RsGxsMailPresignedReceipt* i =
		        dynamic_cast<RsGxsMailPresignedReceipt*>(item);
		uint32_t offset = 0;
		ok = i->serialize(dataPtr, itemSize, offset);
		break;
	}
	case GXS_MAIL_SUBTYPE_GROUP:
		ok = setRsItemHeader(data, itemSize, item->PacketId(), itemSize);
		break;
	default: ok = false; break;
	}

	if(ok)
	{
		*size = itemSize;
		return true;
	}

	std::cout << "RsGxsMailSerializer::serialise(...) failed!" << std::endl;
	return false;
}

