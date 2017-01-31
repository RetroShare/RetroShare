#pragma once
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

#include <string>

#include "rsgxsitems.h"
#include "serialiser/rsbaseserial.h"
#include "serialiser/rstlvidset.h"
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsgxscircles.h" // For: GXS_CIRCLE_TYPE_PUBLIC
#include "services/p3idservice.h"

/// Values must fit into uint8_t
enum GxsMailItemsSubtypes
{
	GXS_MAIL_SUBTYPE_MAIL    = 1,
	GXS_MAIL_SUBTYPE_ACK     = 2,
	GXS_MAIL_SUBTYPE_GROUP   = 3
};

struct RsGxsMailBaseItem : RsGxsMsgItem
{
	RsGxsMailBaseItem(GxsMailItemsSubtypes subtype) :
	    RsGxsMsgItem( RS_SERVICE_TYPE_GXS_MAIL,
	                  static_cast<uint8_t>(subtype) ),
	    cryptoType(UNDEFINED_ENCRYPTION) {}

	/// Values must fit into uint8_t
	enum EncryptionMode
	{
		CLEAR_TEXT                = 1,
		RSA                       = 2,
		UNDEFINED_ENCRYPTION      = 250
	};
	EncryptionMode cryptoType;

	/**
	 * @brief recipientsHint used instead of plain recipient id, so sender can
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
	 * message, in the more private case (recipientsHint == 0xFFF...FFF) no one
	 * has a clue about who is the actual recipient, while this imply the cost
	 * that every potencial recipient has to try to decrypt it to know if it is
	 * for herself. This way a bunch of non recipients can rapidly discover that
	 * the message is not directed to them without attempting it's decryption.
	 *
	 * To check if one id may be the recipient of the mail or not one need to
	 * bitwise compare the hint with the id, if at least one bit of the hint is
	 * 0 while the corrisponding bit in the id is 1 then the id cannot be the
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
	RsGxsId recipientsHint;

	void static inline saltRecipientHint(RsGxsId& hint, const RsGxsId& salt)
	{ hint = hint | salt; }
	void inline saltRecipientHint(const RsGxsId& salt)
	{ saltRecipientHint(recipientsHint, salt); }

	/**
	 * @brief maybeRecipient given an id and an hint check if they match
	 * @see recipientHint
	 * @note this is not the final implementation as id and hint are not 32bit
	 *   integers it is just to not forget how to verify the hint/id matching
	 *   fastly with boolean ops
	 * @return true if the id may be recipient of the hint, false otherwise
	 */
	bool static inline maybeRecipient(const RsGxsId& hint, const RsGxsId& id)
	{ return (~id|hint) == allRecipientsHint; }
	bool inline maybeRecipient(const RsGxsId& id) const
	{ return maybeRecipient(recipientsHint, id); }

	const static RsGxsId allRecipientsHint;

	void inline clear()
	{
		cryptoType = UNDEFINED_ENCRYPTION;
		recipientsHint.clear();
		meta = RsMsgMetaData();
	}

	static uint32_t inline size()
	{
		return  8 + // Header
		        1 + // cryptoType
		        RsGxsId::serial_size(); // recipientsHint
	}
	bool serialize(uint8_t* data, uint32_t size, uint32_t& offset) const;
	bool deserialize(const uint8_t* data, uint32_t& size, uint32_t& offset);
	std::ostream &print(std::ostream &out, uint16_t /*indent = 0*/);
};

struct RsGxsMailItem : RsGxsMailBaseItem
{
	RsGxsMailItem(GxsMailItemsSubtypes subtype) :
	    RsGxsMailBaseItem(subtype) {}
	RsGxsMailItem() :
	    RsGxsMailBaseItem(GXS_MAIL_SUBTYPE_MAIL) {}

	/** This should travel encrypted, unless EncryptionMode::CLEAR_TEXT
	 * is specified */
	std::vector<uint8_t> payload;

	uint32_t size() const { return RsGxsMailBaseItem::size() + payload.size(); }
	bool serialize(uint8_t* data, uint32_t size, uint32_t& offset) const
	{
		bool ok = size < MAX_SIZE;
		ok = ok && RsGxsMailBaseItem::serialize(data, size, offset);
		uint32_t psz = payload.size();
		ok = ok && memcpy(data+offset, &payload[0], psz);
		offset += psz;
		return ok;
	}
	bool deserialize(const uint8_t* data, uint32_t& size, uint32_t& offset)
	{
		uint32_t bsz = RsGxsMailBaseItem::size();
		uint32_t psz = size - bsz;
		return size < MAX_SIZE && size >= bsz
		        && RsGxsMailBaseItem::deserialize(data, size, offset)
		        && (payload.resize(psz), memcpy(&payload[0], data+offset, psz));
	}
	void clear() { RsGxsMailBaseItem::clear(); payload.clear(); }

	/// Maximum mail size in bytes 10 MiB is more than anything sane can need
	const static uint32_t MAX_SIZE = 10*8*1024*1024;
};

struct RsGxsMailAckItem : RsGxsMailBaseItem
{
	RsGxsMailAckItem() : RsGxsMailBaseItem(GXS_MAIL_SUBTYPE_ACK) {}
	RsGxsId recipient;

	void clear() { recipient.clear(); }
	std::ostream &print(std::ostream &out, uint16_t /*indent = 0*/)
	{ return out << recipient.toStdString(); }
};

struct RsGxsMailGroupItem : RsGxsGrpItem
{
	RsGxsMailGroupItem() :
	    RsGxsGrpItem(RS_SERVICE_TYPE_GXS_MAIL, GXS_MAIL_SUBTYPE_GROUP)
	{
		meta.mGroupFlags = GXS_SERV::FLAG_PRIVACY_PUBLIC;
		meta.mGroupName = "Mail";
		meta.mCircleType = GXS_CIRCLE_TYPE_PUBLIC;
	}

	void clear() {}
	std::ostream &print(std::ostream &out, uint16_t /*indent = 0*/)
	{ return out; }
};

struct RsGxsMailSerializer : RsSerialType
{
	RsGxsMailSerializer() : RsSerialType( RS_PKT_VERSION_SERVICE,
	                                      RS_SERVICE_TYPE_GXS_MAIL ) {}
	~RsGxsMailSerializer() {}

	uint32_t size(RsItem* item)
	{
		uint32_t sz = 0;
		switch(item->PacketSubType())
		{
		case GXS_MAIL_SUBTYPE_MAIL:
		{
			RsGxsMailItem* i = dynamic_cast<RsGxsMailItem*>(item);
			if(i) sz = i->size();
			break;
		}
		case GXS_MAIL_SUBTYPE_ACK:
		{
			RsGxsMailAckItem* i = dynamic_cast<RsGxsMailAckItem*>(item);
			if(i)
			{
				sz = 8; // Header
				sz += 4; // RsGxsMailBaseItem::recipient_hint
				sz += 1; // RsGxsMailAckItem::read
				sz += i->recipient.serial_size();
			}
			break;
		}
		case GXS_MAIL_SUBTYPE_GROUP: sz = 8; break;
		default: break;
		}

		return sz;
	}

	bool serialise(RsItem* item, void* data, uint32_t* size);

	RsItem* deserialise(void* data, uint32_t* size)
	{
		uint32_t rstype = getRsItemId(data);
		uint32_t rssize = getRsItemSize(data);
		uint8_t pktv = getRsItemVersion(rstype);
		uint16_t srvc = getRsItemService(rstype);

		if ( (RS_PKT_VERSION_SERVICE != pktv) || // 0x02
		     (RS_SERVICE_TYPE_GXS_MAIL != srvc) || // 0x0230 = 560
		     (*size < rssize) )
		{
			print_stacktrace();
			return NULL;
		}

		*size = rssize;
		bool ok = true;
		RsItem* ret = NULL;

		switch (getRsItemSubType(rstype))
		{
		case GXS_MAIL_SUBTYPE_MAIL:
		{
			RsGxsMailItem* i = new RsGxsMailItem();
			uint32_t offset = 0;
			const uint8_t* dataPtr = reinterpret_cast<uint8_t*>(data);
			ok = ok && i->deserialize(dataPtr, *size, offset);
			ret = i;
			break;
		}
		case GXS_MAIL_SUBTYPE_ACK:
		{
			RsGxsMailAckItem* i = new RsGxsMailAckItem();
			uint32_t offset = 0;
			ok &= i->recipient.deserialise(data, *size, offset);
			ret = i;
			break;
		}
		case GXS_MAIL_SUBTYPE_GROUP:
		{
			ret = new RsGxsMailGroupItem();
			break;
		}
		default:
			ok = false;
			break;
		}

		if(ok) return ret;

		delete ret;
		return NULL;
	}
};

