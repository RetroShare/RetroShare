/*******************************************************************************
 * libretroshare/src/gxstrans: p3gxstrans.cc                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2016-2017  Gioacchino Mazzurco <gio@eigenlab.org>             *
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
#pragma once

#include <string>

#include "rsitems/rsgxsitems.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvidset.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsgxstrans.h"
#include "retroshare/rsgxscircles.h" // For: GXS_CIRCLE_TYPE_PUBLIC
#include "services/p3idservice.h"
#include "serialiser/rstypeserializer.h"

class RsNxsTransPresignedReceipt : public RsNxsMsg
{
public:
	RsNxsTransPresignedReceipt() : RsNxsMsg(RS_SERVICE_TYPE_GXS_TRANS) {}

    virtual ~RsNxsTransPresignedReceipt() {}
};

class RsGxsTransBaseMsgItem : public RsGxsMsgItem
{
public:
	explicit RsGxsTransBaseMsgItem(GxsTransItemsSubtypes subtype) :
	    RsGxsMsgItem( RS_SERVICE_TYPE_GXS_TRANS,
	                  static_cast<uint8_t>(subtype) ), mailId(0) {}

    virtual ~RsGxsTransBaseMsgItem() {}

	RsGxsTransId mailId;

	void inline clear()
	{
		mailId = 0;
		meta = RsMsgMetaData();
	}

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx )
	{ RS_SERIAL_PROCESS(mailId); }
};

class RsGxsTransPresignedReceipt : public RsGxsTransBaseMsgItem
{
public:
	RsGxsTransPresignedReceipt() : RsGxsTransBaseMsgItem(GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_RECEIPT) {}
    virtual ~RsGxsTransPresignedReceipt() {}
};

enum class RsGxsTransEncryptionMode : uint8_t
{
	CLEAR_TEXT                = 1,
	RSA                       = 2,
	UNDEFINED_ENCRYPTION      = 250
};

class RsGxsTransMailItem : public RsGxsTransBaseMsgItem
{
public:
	RsGxsTransMailItem() :
	    RsGxsTransBaseMsgItem(GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_MAIL),
	    cryptoType(RsGxsTransEncryptionMode::UNDEFINED_ENCRYPTION) {}

    virtual ~RsGxsTransMailItem() {}

	RsGxsTransEncryptionMode cryptoType;

	/**
	 * @brief recipientHint used instead of plain recipient id, so sender can
	 *  decide the equilibrium between exposing the recipient and the cost of
	 *  completely anonymize it. So a bunch of luky non recipient can conclude
	 *  rapidly that they are not the recipient without trying to decrypt the
	 *  message.
	 *
	 * To be able to decide how much metadata we disclose sending a message we
	 * send an hint instead of the recipient id in clear, the hint cannot be
	 * false (the recipient would discard the mail) but may be arbitrarly
	 * obscure like 0xFF...FF so potentially everyone could be the recipient, or
	 * may expose the complete recipient id or be a middle ground.
	 * To calculate arbitrary precise hint one do a bitwise OR of the recipients
	 * keys and an arbitrary salt, the more recipients has the mail and the more
	 * 1 bits has the salt the less accurate is the hint.
	 * This way the sender is able to adjust the metadata privacy needed for the
	 * message, in the more private case (recipientHint == 0xFFF...FFF) no one
	 * has a clue about who is the actual recipient, while this imply the cost
	 * that every potencial recipient has to try to decrypt it to know if it is
	 * for herself. This way a bunch of non recipients can rapidly discover that
	 * the message is not directed to them without attempting it's decryption.
	 *
	 * To check if one id may be the recipient of the mail or not one need to
	 * bitwise compare the hint with the id, if at least one bit of the hint is
	 * 0 while the corresponding bit in the id is 1 then the id cannot be the
	 * recipient of the mail.
	 *
	 * Note that by design one can prove that an id is not recipient of the mail
	 * but cannot prove it is.
	 * Also in the extreme case of using 0x00...00 as salt that is equivalent
	 * to not salting at all (aka the plain recipient id is used as hint) a
	 * malicious observer could not demostrate in a conclusive manner that the
	 * mail is directed to the actual recipient as the "apparently"
	 * corresponding hint may be fruit of a "luky" salting of another id.
	 */
	RsGxsId recipientHint;
	void inline saltRecipientHint(const RsGxsId& salt)
	{ recipientHint = recipientHint | salt; }

	/**
	 * @brief maybeRecipient given an id and an hint check if they match
	 * @see recipientHint
	 * @return true if the id may be recipient of the hint, false otherwise
	 */
	bool inline maybeRecipient(const RsGxsId& id) const
	{ return (~id|recipientHint) == allRecipientsHint; }

	const static RsGxsId allRecipientsHint;

	/** This should travel encrypted, unless EncryptionMode::CLEAR_TEXT
	 * is specified */
	std::vector<uint8_t> payload;

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx )
	{
		RsGxsTransBaseMsgItem::serial_process(j, ctx);
		RS_SERIAL_PROCESS(cryptoType);
		RS_SERIAL_PROCESS(recipientHint);
		RS_SERIAL_PROCESS(payload);
	}

	void clear()
	{
		RsGxsTransBaseMsgItem::clear();
		cryptoType = RsGxsTransEncryptionMode::UNDEFINED_ENCRYPTION;
		recipientHint.clear();
		payload.clear();
	}

	/// Maximum mail size in bytes 10 MiB is more than anything sane can need
	const static uint32_t MAX_SIZE = 10*8*1024*1024;
};

class RsGxsTransGroupItem : public RsGxsGrpItem
{
public:
	RsGxsTransGroupItem() :
	    RsGxsGrpItem( RS_SERVICE_TYPE_GXS_TRANS,
	                  static_cast<uint8_t>(
	                      GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_GROUP) )
	{
		meta.mGroupFlags = GXS_SERV::FLAG_PRIVACY_PUBLIC;
		meta.mGroupName = "Mail";
		meta.mCircleType = GXS_CIRCLE_TYPE_PUBLIC;
	}
    virtual ~RsGxsTransGroupItem() {}

	// TODO: Talk with Cyril why there is no RsGxsGrpItem::serial_process
	virtual void serial_process(RsGenericSerializer::SerializeJob /*j*/,
	                            RsGenericSerializer::SerializeContext& /*ctx*/)
	{}

	void clear() {}
	std::ostream &print(std::ostream &out, uint16_t /*indent = 0*/)
	{ return out; }
};

class RsGxsTransSerializer;

class OutgoingRecord_deprecated : public RsItem
{
public:
	OutgoingRecord_deprecated( RsGxsId rec, GxsTransSubServices cs, const uint8_t* data, uint32_t size );

    virtual ~OutgoingRecord_deprecated() {}

	GxsTransSendStatus status;
	RsGxsId recipient;
	/// Don't use a pointer would be invalid after publish
	RsGxsTransMailItem mailItem;

	std::vector<uint8_t> mailData;
	GxsTransSubServices clientService;

	RsNxsTransPresignedReceipt presignedReceipt;

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx );

	void clear() {}

private:
	friend class RsGxsTransSerializer;
	OutgoingRecord_deprecated();
};

class OutgoingRecord : public RsItem
{
public:
	OutgoingRecord( RsGxsId rec, GxsTransSubServices cs,
	                const uint8_t* data, uint32_t size );

    virtual ~OutgoingRecord() {}

	GxsTransSendStatus       status;
	RsGxsId                  recipient;

	RsGxsId                  author;		// These 3 fields cannot be stored in mailItem.meta, which is not serialised.
	RsGxsGroupId             group_id ;
	uint32_t                 sent_ts ;

	/// Don't use a pointer would be invalid after publish
	RsGxsTransMailItem mailItem;

	std::vector<uint8_t> mailData;
	GxsTransSubServices clientService;

	RsNxsTransPresignedReceipt presignedReceipt;

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx );

	void clear() {}

private:
	friend class RsGxsTransSerializer;
	OutgoingRecord();
};


class RsGxsTransSerializer : public RsServiceSerializer
{
public:
	RsGxsTransSerializer() : RsServiceSerializer(RS_SERVICE_TYPE_GXS_TRANS) {}
	virtual ~RsGxsTransSerializer() {}

	RsItem* create_item(uint16_t service_id, uint8_t item_sub_id) const
	{
		if(service_id != RS_SERVICE_TYPE_GXS_TRANS) return NULL;

		switch(static_cast<GxsTransItemsSubtypes>(item_sub_id))
		{
		case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_MAIL: return new RsGxsTransMailItem();
		case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_RECEIPT: return new RsGxsTransPresignedReceipt();
		case GxsTransItemsSubtypes::GXS_TRANS_SUBTYPE_GROUP: return new RsGxsTransGroupItem();
		case GxsTransItemsSubtypes::OUTGOING_RECORD_ITEM_deprecated: return new OutgoingRecord_deprecated();
		case GxsTransItemsSubtypes::OUTGOING_RECORD_ITEM: return new OutgoingRecord();
		default: return NULL;
		}
	}
};

