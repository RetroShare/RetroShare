/*******************************************************************************
 * libretroshare/src/gxstrans: p3gxstrans.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2016-2018  Gioacchino Mazzurco <gio@eigenlab.org>             *
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

void OutgoingRecord_deprecated::serial_process(
        RsGenericSerializer::SerializeJob j,
        RsGenericSerializer::SerializeContext& ctx )
{
	RS_SERIAL_PROCESS(status);
	RS_SERIAL_PROCESS(recipient);
	RS_SERIAL_PROCESS(mailItem);
	RS_SERIAL_PROCESS(mailData);
	RS_SERIAL_PROCESS(clientService);
	RS_SERIAL_PROCESS(presignedReceipt);
}

void OutgoingRecord::serial_process(RsGenericSerializer::SerializeJob j,
                                    RsGenericSerializer::SerializeContext& ctx)
{
	RS_SERIAL_PROCESS(status);
	RS_SERIAL_PROCESS(recipient);
	RS_SERIAL_PROCESS(author);
	RS_SERIAL_PROCESS(group_id);
	RS_SERIAL_PROCESS(sent_ts);
	RS_SERIAL_PROCESS(mailItem);
	RS_SERIAL_PROCESS(mailData);
	RS_SERIAL_PROCESS(clientService);
	RS_SERIAL_PROCESS(presignedReceipt);
}
