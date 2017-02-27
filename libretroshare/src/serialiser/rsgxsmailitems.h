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

/// Subservices identifiers (like port for TCP)
enum class GxsMailSubServices : uint16_t
{
	UNKNOWN         = 0,
	TEST_SERVICE    = 1,
	P3_MSG_SERVICE  = 2,
	P3_CHAT_SERVICE = 3
};

/// Values must fit into uint8_t
enum class GxsMailItemsSubtypes : uint8_t
{
	GXS_MAIL_SUBTYPE_MAIL    = 1,
	GXS_MAIL_SUBTYPE_RECEIPT = 2,
	GXS_MAIL_SUBTYPE_GROUP   = 3,
	OUTGOING_RECORD_ITEM     = 4
};

typedef uint64_t RsGxsMailId;

struct RsNxsMailPresignedReceipt : RsNxsMsg
{
	RsNxsMailPresignedReceipt() : RsNxsMsg(RS_SERVICE_TYPE_GXS_MAIL) {}
};

struct RsGxsMailBaseItem : RsGxsMsgItem
{
	RsGxsMailBaseItem(GxsMailItemsSubtypes subtype) :
	    RsGxsMsgItem( RS_SERVICE_TYPE_GXS_MAIL,
	                  static_cast<uint8_t>(subtype) ), mailId(0) {}

	RsGxsMailId mailId;

	void inline clear()
	{
		mailId = 0;
		meta = RsMsgMetaData();
	}

	static uint32_t inline size()
	{
		return  8 + // Header
		        8;  // mailId
	}
	bool serialize(uint8_t* data, uint32_t size, uint32_t& offset) const;
	bool deserialize(const uint8_t* data, uint32_t& size, uint32_t& offset);
	std::ostream &print(std::ostream &out, uint16_t /*indent = 0*/);
};

struct RsGxsMailPresignedReceipt : RsGxsMailBaseItem
{
	RsGxsMailPresignedReceipt() :
	    RsGxsMailBaseItem(GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_RECEIPT) {}
};

enum class RsGxsMailEncryptionMode : uint8_t
{
	CLEAR_TEXT                = 1,
	RSA                       = 2,
	UNDEFINED_ENCRYPTION      = 250
};

struct RsGxsMailItem : RsGxsMailBaseItem
{
	RsGxsMailItem() :
	    RsGxsMailBaseItem(GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_MAIL),
	    cryptoType(RsGxsMailEncryptionMode::UNDEFINED_ENCRYPTION) {}

	RsGxsMailEncryptionMode cryptoType;

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
	void inline saltRecipientHint(const RsGxsId& salt)
	{ recipientsHint = recipientsHint | salt; }

	/**
	 * @brief maybeRecipient given an id and an hint check if they match
	 * @see recipientHint
	 * @return true if the id may be recipient of the hint, false otherwise
	 */
	bool inline maybeRecipient(const RsGxsId& id) const
	{ return (~id|recipientsHint) == allRecipientsHint; }

	const static RsGxsId allRecipientsHint;

	/** This should travel encrypted, unless EncryptionMode::CLEAR_TEXT
	 * is specified */
	std::vector<uint8_t> payload;

	uint32_t size() const
	{
		return RsGxsMailBaseItem::size() +
		        1 + // cryptoType
		        recipientsHint.serial_size() +
		        payload.size();
	}
	bool serialize(uint8_t* data, uint32_t size, uint32_t& offset) const
	{
		bool ok = size < MAX_SIZE;
		ok = ok && RsGxsMailBaseItem::serialize(data, size, offset);
		ok = ok && setRawUInt8( data, size, &offset,
		                        static_cast<uint8_t>(cryptoType) );
		ok = ok && recipientsHint.serialise(data, size, offset);
		uint32_t psz = payload.size();
		ok = ok && memcpy(data+offset, &payload[0], psz);
		offset += psz;
		return ok;
	}
	bool deserialize(const uint8_t* data, uint32_t& size, uint32_t& offset)
	{
		void* sizePtr = const_cast<uint8_t*>(data+offset);
		uint32_t rssize = getRsItemSize(sizePtr);

		uint32_t roffset = offset;
		bool ok = rssize <= size && size < MAX_SIZE;
		ok = ok && RsGxsMailBaseItem::deserialize(data, rssize, roffset);

		void* dataPtr = const_cast<uint8_t*>(data);
		uint8_t crType;
		ok = ok && getRawUInt8(dataPtr, rssize, &roffset, &crType);
		cryptoType = static_cast<RsGxsMailEncryptionMode>(crType);
		ok = ok && recipientsHint.deserialise(dataPtr, rssize, roffset);
		uint32_t psz = rssize - roffset;
		ok = ok && (payload.resize(psz), memcpy(&payload[0], data+roffset, psz));
		ok = ok && (roffset += psz);
		if(ok) { size = rssize; offset = roffset; }
		else size = 0;
		return ok;
	}
	void clear()
	{
		RsGxsMailBaseItem::clear();
		cryptoType = RsGxsMailEncryptionMode::UNDEFINED_ENCRYPTION;
		recipientsHint.clear();
		payload.clear();
	}

	/// Maximum mail size in bytes 10 MiB is more than anything sane can need
	const static uint32_t MAX_SIZE = 10*8*1024*1024;
};

struct RsGxsMailGroupItem : RsGxsGrpItem
{
	RsGxsMailGroupItem() :
	    RsGxsGrpItem( RS_SERVICE_TYPE_GXS_MAIL,
	                  static_cast<uint8_t>(
	                      GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_GROUP) )
	{
		meta.mGroupFlags = GXS_SERV::FLAG_PRIVACY_PUBLIC;
		meta.mGroupName = "Mail";
		meta.mCircleType = GXS_CIRCLE_TYPE_PUBLIC;
	}

	void clear() {}
	std::ostream &print(std::ostream &out, uint16_t /*indent = 0*/)
	{ return out; }
};

enum class GxsMailStatus : uint8_t
{
	UNKNOWN = 0,
	PENDING_PROCESSING,
	PENDING_PREFERRED_GROUP,
	PENDING_RECEIPT_CREATE,
	PENDING_RECEIPT_SIGNATURE,
	PENDING_SERIALIZATION,
	PENDING_PAYLOAD_CREATE,
	PENDING_PAYLOAD_ENCRYPT,
	PENDING_PUBLISH,
	/** This will be useful so the user can know if the mail reached at least
	 * some friend node, in case of internet connection interruption */
	//PENDING_TRANSFER,
	PENDING_RECEIPT_RECEIVE,
	/// Records with status >= RECEIPT_RECEIVED get deleted
	RECEIPT_RECEIVED,
	FAILED_RECEIPT_SIGNATURE = 240,
	FAILED_ENCRYPTION
};

class RsGxsMailSerializer;
struct OutgoingRecord : RsItem
{
	OutgoingRecord( RsGxsId rec, GxsMailSubServices cs,
	                const uint8_t* data, uint32_t size );

	GxsMailStatus status;
	RsGxsId recipient;
	/// Don't use a pointer would be invalid after publish
	RsGxsMailItem mailItem;
	std::vector<uint8_t> mailData;
	GxsMailSubServices clientService;
	RsNxsMailPresignedReceipt presignedReceipt;

	uint32_t size() const;
	bool serialize(uint8_t* data, uint32_t size, uint32_t& offset) const;
	bool deserialize(const uint8_t* data, uint32_t& size, uint32_t& offset);

	virtual void clear();
	virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

private:
	friend class RsGxsMailSerializer;
	OutgoingRecord();
};


struct RsGxsMailSerializer : RsSerialType
{
	RsGxsMailSerializer() : RsSerialType( RS_PKT_VERSION_SERVICE,
	                                      RS_SERVICE_TYPE_GXS_MAIL ) {}
	~RsGxsMailSerializer() {}

	uint32_t size(RsItem* item)
	{
		uint32_t sz = 0;
		switch(static_cast<GxsMailItemsSubtypes>(item->PacketSubType()))
		{
		case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_MAIL:
		{
			RsGxsMailItem* i = dynamic_cast<RsGxsMailItem*>(item);
			if(i) sz = i->size();
			break;
		}
		case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_RECEIPT:
			sz = RsGxsMailPresignedReceipt::size(); break;
		case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_GROUP: sz = 8; break;
		case GxsMailItemsSubtypes::OUTGOING_RECORD_ITEM:
		{
			OutgoingRecord* ci = dynamic_cast<OutgoingRecord*>(item);
			if(ci) sz = ci->size();
			break;
		}
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
		const uint8_t* dataPtr = reinterpret_cast<uint8_t*>(data);

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

		switch (static_cast<GxsMailItemsSubtypes>(getRsItemSubType(rstype)))
		{
		case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_MAIL:
		{
			RsGxsMailItem* i = new RsGxsMailItem();
			uint32_t offset = 0;
			ok = ok && i->deserialize(dataPtr, *size, offset);
			ret = i;
			break;
		}
		case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_RECEIPT:
		{
			RsGxsMailPresignedReceipt* i = new RsGxsMailPresignedReceipt();
			uint32_t offset = 0;
			ok &= i->deserialize(dataPtr, *size, offset);
			ret = i;
			break;
		}
		case GxsMailItemsSubtypes::GXS_MAIL_SUBTYPE_GROUP:
		{
			ret = new RsGxsMailGroupItem();
			break;
		}
		case GxsMailItemsSubtypes::OUTGOING_RECORD_ITEM:
		{
			OutgoingRecord* i = new OutgoingRecord();
			uint32_t offset = 0;
			ok = ok && i->deserialize(dataPtr, *size, offset);
			ret = i;
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

