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

#include "gxstrans/p3gxstransitems.h"

const RsGxsId RsGxsTransMailItem::allRecipientsHint("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");


bool RsGxsTransBaseItem::serialize(uint8_t* data, uint32_t size,
                                  uint32_t& offset) const
{
	bool ok = setRsItemHeader(data+offset, size, PacketId(), size);
	ok = ok && (offset += 8); // Take header in account
	ok = ok && setRawUInt64(data, size, &offset, mailId);
	return ok;
}

bool RsGxsTransBaseItem::deserialize( const uint8_t* data, uint32_t& size,
                                      uint32_t& offset)
{
	void* hdrPtr = const_cast<uint8_t*>(data+offset);
	uint32_t rssize = getRsItemSize(hdrPtr);
	uint32_t roffset = offset + 8; // Take header in account

	void* dataPtr = const_cast<uint8_t*>(data);
	bool ok = rssize <= size;
	ok = ok && getRawUInt64(dataPtr, rssize, &roffset, &mailId);
	if(ok) { size = rssize; offset = roffset; }
	else size = 0;
	return ok;
}

std::ostream& RsGxsTransBaseItem::print(std::ostream &out, uint16_t)
{ return out << __PRETTY_FUNCTION__ << " mailId: " << mailId; }

bool RsGxsTransSerializer::serialise(RsItem* item, void* data, uint32_t* size)
{
	uint32_t itemSize = RsGxsTransSerializer::size(item);
	if(*size < itemSize)
	{
		std::cout << __PRETTY_FUNCTION__ << " failed due to wrong size: "
		          << size << " < " << itemSize << std::endl;
		return false;
	}

	uint8_t* dataPtr = reinterpret_cast<uint8_t*>(data);
	bool ok = false;
	switch(static_cast<GxsTransItemsSubtypes>(item->PacketSubType()))
	{
	case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_MAIL:
	{
		uint32_t offset = 0;
		RsGxsTransMailItem* i = dynamic_cast<RsGxsTransMailItem*>(item);
		ok = i && i->serialize(dataPtr, itemSize, offset);
		break;
	}
	case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_RECEIPT:
	{
		RsGxsTransPresignedReceipt* i =
		        dynamic_cast<RsGxsTransPresignedReceipt*>(item);
		uint32_t offset = 0;
		ok = i && i->serialize(dataPtr, itemSize, offset);
		break;
	}
	case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_GROUP:
		ok = setRsItemHeader(data, itemSize, item->PacketId(), itemSize);
		break;
	case GxsTransItemsSubtypes::OUTGOING_RECORD_ITEM:
	{
		uint32_t offset = 0;
		OutgoingRecord* i = dynamic_cast<OutgoingRecord*>(item);
		ok = i && i->serialize(dataPtr, itemSize, offset);
		break;
	}
	default: ok = false; break;
	}

	if(ok)
	{
		*size = itemSize;
		return true;
	}

	std::cout << __PRETTY_FUNCTION__ << " failed!" << std::endl;
	return false;
}

OutgoingRecord::OutgoingRecord( RsGxsId rec, GxsTransSubServices cs,
                                const uint8_t* data, uint32_t size ) :
    RsItem( RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_GXS_TRANS,
            static_cast<uint8_t>(GxsTransItemsSubtypes::OUTGOING_RECORD_ITEM) ),
    status(GxsTransSendStatus::PENDING_PROCESSING), recipient(rec),
    clientService(cs)
{
	mailData.resize(size);
	memcpy(&mailData[0], data, size);
}

void OutgoingRecord::clear()
{
	status = GxsTransSendStatus::UNKNOWN;
	recipient.clear();
	mailItem.clear();
	mailData.clear();
	clientService = GxsTransSubServices::UNKNOWN;
	presignedReceipt.clear();
}

std::ostream& OutgoingRecord::print(std::ostream& out, uint16_t)
{ return out << "TODO: OutgoingRecordItem::print(...)"; }

OutgoingRecord::OutgoingRecord() :
    RsItem( RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_GXS_TRANS,
            static_cast<uint8_t>(GxsTransItemsSubtypes::OUTGOING_RECORD_ITEM) )
{ clear();}

uint32_t OutgoingRecord::size() const
{
	return 8 + // Header
	        1 + // status
	        recipient.serial_size() +
	        mailItem.serial_size() +
	        4 + // sizeof(mailData.size())
	        mailData.size() +
	        2 + // clientService
	        presignedReceipt.serial_size();
}

bool OutgoingRecord::serialize( uint8_t* data, uint32_t size,
                                            uint32_t& offset) const
{
	bool ok = true;

	ok = ok && setRsItemHeader(data+offset, size, PacketId(), size)
	        && (offset += 8); // Take header in account

	ok = ok && setRawUInt8(data, size, &offset, static_cast<uint8_t>(status));

	ok = ok && recipient.serialise(data, size, offset);

	uint32_t tmpOffset = 0;
	uint32_t tmpSize = mailItem.serial_size();
	ok = ok && mailItem.serialize(data+offset, tmpSize, tmpOffset)
	        && (offset += tmpOffset);

	uint32_t dSize = mailData.size();
	ok = ok && setRawUInt32(data, size, &offset, dSize)
	        && memcpy(data+offset, &mailData[0], dSize) && (offset += dSize);

	ok = ok && setRawUInt16( data, size, &offset,
	                         static_cast<uint16_t>(clientService) );

	dSize = presignedReceipt.serial_size();
	ok = ok && presignedReceipt.serialise(data+offset, dSize)
	        && (offset += dSize);

	return ok;
}

bool OutgoingRecord::deserialize(
        const uint8_t* data, uint32_t& size, uint32_t& offset)
{
	bool ok = true;

	void* dataPtr = const_cast<uint8_t*>(data);
	offset += 8; // Header

	uint8_t tmpStatus = 0;
	ok = ok && getRawUInt8(dataPtr, size, &offset, &tmpStatus);
	status = static_cast<GxsTransSendStatus>(tmpStatus);

	uint32_t tmpSize = size;
	ok = ok && recipient.deserialise(dataPtr, tmpSize, offset);

	void* hdrPtr = const_cast<uint8_t*>(data+offset);
	tmpSize = getRsItemSize(hdrPtr);

	uint32_t tmpOffset = 0;
	ok = ok && mailItem.deserialize(static_cast<uint8_t*>(hdrPtr), tmpSize, tmpOffset);
	ok = ok && (offset += tmpOffset);

	tmpSize = size;
	ok = getRawUInt32(dataPtr, tmpSize, &offset, &tmpSize);
	ok = ok && (tmpSize+offset < size);
	ok = ok && (mailData.resize(tmpSize), memcpy(&mailData[0], data, tmpSize));
	ok = ok && (offset += tmpSize);

	uint16_t cs = 0;
	ok = ok && getRawUInt16(dataPtr, offset+2, &offset, &cs);
	clientService = static_cast<GxsTransSubServices>(cs);

	tmpSize = size;
	ok = ok && presignedReceipt.deserialize(data, tmpSize, offset);

	return ok;
}

