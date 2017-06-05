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
#include "serialiser/rstypeserializer.h"

const RsGxsId RsGxsTransMailItem::allRecipientsHint("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");

OutgoingRecord_deprecated::OutgoingRecord_deprecated()
    : RsItem( RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_GXS_TRANS, static_cast<uint8_t>(GxsTransItemsSubtypes::OUTGOING_RECORD_ITEM_deprecated) ) { clear();}

OutgoingRecord::OutgoingRecord()
    : RsItem( RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_GXS_TRANS, static_cast<uint8_t>(GxsTransItemsSubtypes::OUTGOING_RECORD_ITEM) ) { clear();}

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


RS_REGISTER_ITEM_TYPE(RsGxsTransMailItem)         // for mailItem
RS_REGISTER_ITEM_TYPE(RsNxsTransPresignedReceipt) // for presignedReceipt

void OutgoingRecord_deprecated::serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext& ctx)
{
	RS_REGISTER_SERIAL_MEMBER_TYPED(status, uint8_t);
	RS_REGISTER_SERIAL_MEMBER(recipient);
	RS_REGISTER_SERIAL_MEMBER(mailItem);
	RS_REGISTER_SERIAL_MEMBER(mailData);
	RS_REGISTER_SERIAL_MEMBER_TYPED(clientService, uint16_t);
	RS_REGISTER_SERIAL_MEMBER(presignedReceipt);
}

void OutgoingRecord::serial_process(RsGenericSerializer::SerializeJob j,
                                    RsGenericSerializer::SerializeContext& ctx)
{
	RS_REGISTER_SERIAL_MEMBER_TYPED(status, uint8_t);
	RS_REGISTER_SERIAL_MEMBER(recipient);
	RS_REGISTER_SERIAL_MEMBER(author);
	RS_REGISTER_SERIAL_MEMBER(group_id);
	RS_REGISTER_SERIAL_MEMBER(sent_ts);
	RS_REGISTER_SERIAL_MEMBER(mailItem);
	RS_REGISTER_SERIAL_MEMBER(mailData);
	RS_REGISTER_SERIAL_MEMBER_TYPED(clientService, uint16_t);
	RS_REGISTER_SERIAL_MEMBER(presignedReceipt);
}
