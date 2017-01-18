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


enum GxsMailSubtypes
{
	GXS_MAIL_SUBTYPE_MAIL = 1,
	GXS_MAIL_SUBTYPE_ACK,
	GXS_MAIL_SUBTYPE_GROUP
};

struct RsGxsMailBaseItem : RsGxsMsgItem
{
	RsGxsMailBaseItem(GxsMailSubtypes subtype) :
	    RsGxsMsgItem(RS_SERVICE_TYPE_GXS_MAIL, (uint8_t)subtype), flags(0) {}

	enum RsGxsMailFlags { READ = 0x1 };
	uint8_t flags;

	/**
	 * @brief recipient_hint used instead of plain recipient id, so sender can
	 *  decide the equilibrium between exposing the recipient and the cost of
	 *  completely anonymize it. So a bunch of luky non recipient can conclude
	 *  rapidly that they are not recipiend without trying to decrypt the
	 *  message.
	 *
	 * To be able to decide how much metadata we disclose sending a message we
	 * send an hint instead of the recipient id in clear, the hint cannot be
	 * false (the recipient would discard the mail) but may be arbitrarly
	 * obscure like 0xFF...FF so potentially everyone could be the recipient, or
	 * may expose the complete recipient id or be a middle ground.
	 * To calculate arbitrary precise hint one do a bitwise OR of the recipients
	 * keys and an arbytrary salting mask, the more recipients has the mail and
	 * the more 1 bits has the mask the less accurate is the hint.
	 * This way the sender is able to adjust the metadata privacy needed for the
	 * message, in the more private case (recipient_hint == 0xFF...FF) no one
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
	uint32_t recipient_hint;

	/**
	 * @brief maybe_recipient given an id and an hint check if they match
	 * @see recipient_hint
	 * @note this is not the final implementation as id and hint are not 32bit
	 *   integers it is just to not forget how to verify the hint/id matching
	 *   fastly with boolean ops
	 * @return true if the id may be recipient of the hint, false otherwise
	 */
	static bool maybe_recipient(uint32_t id, uint32_t hint)
	{ return (~id|hint) == 0xFFFFFFFF; }
};

struct RsGxsMailItem : RsGxsMailBaseItem
{
	RsGxsMailItem() : RsGxsMailBaseItem(GXS_MAIL_SUBTYPE_MAIL) {}

	RsTlvGxsIdSet recipients;

	/**
	 * @brief body of the email
	 * Should we ue MIME for compatibility with fido RS-email gateway?
	 * https://github.com/zeroreserve/fido
	 * https://github.com/RetroShare/fido
	 * https://en.wikipedia.org/wiki/MIME
	 */
	std::string body;

	void clear()
	{
		recipients.TlvClear();
		body.clear();
	}
	std::ostream &print(std::ostream &out, uint16_t indent = 0)
	{ return recipients.print(out, indent) << body; }
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
		uint32_t s = 8; // Header

		switch(item->PacketSubType())
		{
		case GXS_MAIL_SUBTYPE_MAIL:
		{
			RsGxsMailItem* i = dynamic_cast<RsGxsMailItem*>(item);
			if(i)
			{
				s += 4; // RsGxsMailBaseItem::recipient_hint
				s += i->recipients.TlvSize();
				s += getRawStringSize(i->body);
			}
			break;
		}
		case GXS_MAIL_SUBTYPE_ACK:
		{
			RsGxsMailAckItem* i = dynamic_cast<RsGxsMailAckItem*>(item);
			if(i)
			{
				s += 4; // RsGxsMailBaseItem::recipient_hint
				s += 1; // RsGxsMailAckItem::read
				s += i->recipient.serial_size();
			}
			break;
		}
		case GXS_MAIL_SUBTYPE_GROUP: break;
		default: return 0;
		}

		return s;
	}

	bool serialise(RsItem* item, void* data, uint32_t* size)
	{
		uint32_t tlvsize = RsGxsMailSerializer::size(item);
		uint32_t offset = 0;

		if(*size < tlvsize) return false;

		*size = tlvsize;

		bool ok = true;
		ok &= setRsItemHeader(data, tlvsize, item->PacketId(), tlvsize);

		/* skip the header */
		offset += 8;

		switch(item->PacketSubType())
		{
		case GXS_MAIL_SUBTYPE_MAIL:
		{
			RsGxsMailItem* i = dynamic_cast<RsGxsMailItem*>(item);
			if(i)
			{
				ok &= i->recipients.SetTlv(data, tlvsize, &offset);
				ok &= setRawString(data, tlvsize, &offset, i->body);
			}
			break;
		}
		case GXS_MAIL_SUBTYPE_ACK:
		{
			RsGxsMailAckItem* i = dynamic_cast<RsGxsMailAckItem*>(item);
			if(i)
			{
				ok &= i->recipient.serialise(data, tlvsize, offset);
				ok &= setRawUInt8(data, tlvsize, &offset, i->flags);
			}
			break;
		}
		case GXS_MAIL_SUBTYPE_GROUP: break;
		default: ok = false; break;
		}

		return ok;
	}

	RsItem* deserialise(void* data, uint32_t* size)
	{
		uint32_t rstype = getRsItemId(data);
		uint32_t rssize = getRsItemSize(data);
		uint32_t offset = 8;


		if ( (RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) ||
		     (RS_SERVICE_TYPE_GXS_MAIL != getRsItemService(rstype)) ||
		     (*size < rssize) )
			return NULL;

		*size = rssize;
		bool ok = true;
		RsItem* ret = NULL;

		switch (getRsItemSubType(rstype))
		{
		case GXS_MAIL_SUBTYPE_MAIL:
		{
			RsGxsMailItem* i = new RsGxsMailItem();
			ok &= i->recipients.GetTlv(data, *size, &offset);
			ok &= getRawString(data, *size, &offset, i->body);
			ret = i;
			break;
		}
		case GXS_MAIL_SUBTYPE_ACK:
		{
			RsGxsMailAckItem* i = new RsGxsMailAckItem();
			ok &= getRawUInt8(data, *size, &offset, &i->flags);
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

