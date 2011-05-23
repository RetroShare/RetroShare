#ifndef RS_MSG_ITEMS_H
#define RS_MSG_ITEMS_H

/*
 * libretroshare/src/serialiser: rsmsgitems.h
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

#include <map>

#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"
#include "serialiser/rstlvtypes.h"

/**************************************************************************/

/* chat Flags */
const uint32_t RS_CHAT_FLAG_PRIVATE                = 0x0001;
const uint32_t RS_CHAT_FLAG_REQUESTS_AVATAR        = 0x0002;
const uint32_t RS_CHAT_FLAG_CONTAINS_AVATAR        = 0x0004;
const uint32_t RS_CHAT_FLAG_AVATAR_AVAILABLE       = 0x0008;
const uint32_t RS_CHAT_FLAG_CUSTOM_STATE           = 0x0010;  // used for transmitting peer status string
const uint32_t RS_CHAT_FLAG_PUBLIC                 = 0x0020;
const uint32_t RS_CHAT_FLAG_REQUEST_CUSTOM_STATE   = 0x0040;
const uint32_t RS_CHAT_FLAG_CUSTOM_STATE_AVAILABLE = 0x0080;
const uint32_t RS_CHAT_FLAG_PARTIAL_MESSAGE        = 0x0100;

const uint32_t RS_CHATMSG_CONFIGFLAG_INCOMING = 0x0001;

const uint8_t RS_PKT_SUBTYPE_CHAT_AVATAR           = 0x03 ;	// default is 0x01
const uint8_t RS_PKT_SUBTYPE_CHAT_STATUS           = 0x04 ;	// default is 0x01
const uint8_t RS_PKT_SUBTYPE_PRIVATECHATMSG_CONFIG = 0x05 ;	// default is 0x01 

// for defining tags themselves and msg tags
const uint8_t RS_PKT_SUBTYPE_MSG_TAG_TYPE = 0x03;
const uint8_t RS_PKT_SUBTYPE_MSG_TAGS = 0x04;
const uint8_t RS_PKT_SUBTYPE_MSG_SRC_TAG = 0x05;
const uint8_t RS_PKT_SUBTYPE_MSG_PARENT_TAG = 0x06;


class RsChatItem: public RsItem
{
	public:
		RsChatItem(uint8_t chat_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_CHAT,chat_subtype) {}

		virtual ~RsChatItem() {}
		virtual void clear() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) = 0 ;

		virtual bool serialise(void *data,uint32_t& size) = 0 ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() = 0 ; 							// deserialise is handled using a constructor
};

/*!
 * For sending chat msgs
 * @see p3ChatService
 */
class RsChatMsgItem: public RsChatItem
{
	public:
		RsChatMsgItem() :RsChatItem(RS_PKT_SUBTYPE_DEFAULT) {}
		RsChatMsgItem(void *data,uint32_t size) ; // deserialization

		virtual ~RsChatMsgItem() {}
		virtual void clear() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() ; 							// deserialise is handled using a constructor

		uint32_t chatFlags;
		uint32_t sendTime;
		std::wstring message;
		/* not serialised */
		uint32_t recvTime;
};

/*!
 * For saving incoming and outgoing chat msgs
 * @see p3ChatService
 */
class RsPrivateChatMsgConfigItem: public RsChatItem
{
	public:
		RsPrivateChatMsgConfigItem() :RsChatItem(RS_PKT_SUBTYPE_PRIVATECHATMSG_CONFIG) {}
		RsPrivateChatMsgConfigItem(void *data,uint32_t size) ; // deserialization

		virtual ~RsPrivateChatMsgConfigItem() {}
		virtual void clear() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() ; 							// deserialise is handled using a constructor

		/* set data from RsChatMsgItem to RsPrivateChatMsgConfigItem */
		void set(RsChatMsgItem *ci, const std::string &peerId, uint32_t confFlags);
		/* get data from RsPrivateChatMsgConfigItem to RsChatMsgItem */
		void get(RsChatMsgItem *ci);

		std::string configPeerId;
		uint32_t chatFlags;
		uint32_t configFlags;
		uint32_t sendTime;
		std::wstring message;
		uint32_t recvTime;
};

// This class contains activity info for the sending peer: active, idle, typing, etc.
//
class RsChatStatusItem: public RsChatItem
{
	public:
		RsChatStatusItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_STATUS) {}
		RsChatStatusItem(void *data,uint32_t size) ; // deserialization

		virtual ~RsChatStatusItem() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() ; 							// deserialise is handled using a constructor

		uint32_t flags ;
		std::string status_string;
};

// This class contains avatar images in Qt format.
//
class RsChatAvatarItem: public RsChatItem
{
	public:
		RsChatAvatarItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_AVATAR) {}
		RsChatAvatarItem(void *data,uint32_t size) ; // deserialization

		virtual ~RsChatAvatarItem() ;
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() ; 							// deserialise is handled using a constructor

		uint32_t image_size ;				// size of data in bytes
		unsigned char *image_data ;		// image
};


class RsChatSerialiser: public RsSerialType
{
	public:
		RsChatSerialiser() :RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_CHAT) {}

		virtual uint32_t 	size (RsItem *item) 
		{ 
			return static_cast<RsChatItem *>(item)->serial_size() ; 
		}
		virtual bool serialise(RsItem *item, void *data, uint32_t *size) 
		{ 
			return static_cast<RsChatItem *>(item)->serialise(data,*size) ; 
		}
		virtual RsItem *deserialise (void *data, uint32_t *size) ;
};

/**************************************************************************/

const uint32_t RS_MSG_FLAGS_OUTGOING       = 0x0001;
const uint32_t RS_MSG_FLAGS_PENDING        = 0x0002;
const uint32_t RS_MSG_FLAGS_DRAFT          = 0x0004;
const uint32_t RS_MSG_FLAGS_NEW            = 0x0010;
const uint32_t RS_MSG_FLAGS_TRASH          = 0x0020;
const uint32_t RS_MSG_FLAGS_UNREAD_BY_USER = 0x0040;
const uint32_t RS_MSG_FLAGS_REPLIED        = 0x0080;
const uint32_t RS_MSG_FLAGS_FORWARDED      = 0x0100;
const uint32_t RS_MSG_FLAGS_STAR           = 0x0200;

class RsMsgItem: public RsItem
{
	public:
	RsMsgItem() 
	:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG, 
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
	
	RsMsgItem(uint16_t type) 
	:RsItem(RS_PKT_VERSION_SERVICE, type, 
		RS_PKT_SUBTYPE_DEFAULT)
	{ return; }
	
virtual ~RsMsgItem(); 
virtual void clear();
std::ostream &print(std::ostream &out, uint16_t indent = 0);

	uint32_t msgFlags;
	uint32_t msgId;

	uint32_t sendTime;
	uint32_t recvTime;

	std::wstring subject;
	std::wstring message;

	RsTlvPeerIdSet msgto;
	RsTlvPeerIdSet msgcc;
	RsTlvPeerIdSet msgbcc;

	RsTlvFileSet attachment;
};

class RsMsgTagType : public RsItem
{
public:
	RsMsgTagType()
		:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG,
			RS_PKT_SUBTYPE_MSG_TAG_TYPE)
		{ return; }

	std::ostream &print(std::ostream &out, uint16_t indent = 0);


	virtual ~RsMsgTagType();
	virtual void clear();

	std::string text;
	uint32_t rgb_color;
	uint32_t tagId;

};

class RsMsgTags : public RsItem
{
public:
	RsMsgTags()
		:RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG,
			RS_PKT_SUBTYPE_MSG_TAGS)
		{ return; }

	std::ostream &print(std::ostream &out, uint16_t indent = 0);

	virtual ~RsMsgTags();
	virtual void clear();

	uint32_t msgId;
	std::list<uint32_t> tagIds;
};

class RsMsgSrcId : public RsItem
{

public:
	RsMsgSrcId()
	: RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG,
				RS_PKT_SUBTYPE_MSG_SRC_TAG)
	{ return;}

	std::ostream &print(std::ostream &out, uint16_t indent = 0);

	virtual ~RsMsgSrcId();
	virtual void clear();


	uint32_t msgId;
	std::string srcId;

};

class RsMsgParentId : public RsItem
{

public:
	RsMsgParentId()
	: RsItem(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG,
				RS_PKT_SUBTYPE_MSG_PARENT_TAG)
	{ return;}

	std::ostream &print(std::ostream &out, uint16_t indent = 0);

	virtual ~RsMsgParentId();
	virtual void clear();


	uint32_t msgId;
	uint32_t msgParentId;

};

class RsMsgSerialiser: public RsSerialType
{
	public:
	RsMsgSerialiser(bool bConfiguration = false)
	:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG), m_bConfiguration (bConfiguration)
	{ return; }
	
	RsMsgSerialiser(uint16_t type)
	:RsSerialType(RS_PKT_VERSION_SERVICE, type), m_bConfiguration (false)
	{ return; }
	
virtual     ~RsMsgSerialiser() { return; }
	
virtual	uint32_t    size(RsItem *);
virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
virtual	RsItem *    deserialise(void *data, uint32_t *size);


	private:

virtual	uint32_t    sizeMsgItem(RsMsgItem *);
virtual	bool        serialiseMsgItem  (RsMsgItem *item, void *data, uint32_t *size);
virtual	RsMsgItem *deserialiseMsgItem(void *data, uint32_t *size);

virtual	uint32_t    sizeTagItem(RsMsgTagType *);
virtual	bool        serialiseTagItem  (RsMsgTagType *item, void *data, uint32_t *size);
virtual	RsMsgTagType *deserialiseTagItem(void *data, uint32_t *size);

virtual	uint32_t    sizeMsgTagItem(RsMsgTags *);
virtual	bool        serialiseMsgTagItem  (RsMsgTags *item, void *data, uint32_t *size);
virtual	RsMsgTags *deserialiseMsgTagItem(void *data, uint32_t *size);

virtual	uint32_t    sizeMsgSrcIdItem(RsMsgSrcId *);
virtual	bool        serialiseMsgSrcIdItem  (RsMsgSrcId *item, void *data, uint32_t *size);
virtual	RsMsgSrcId *deserialiseMsgSrcIdItem(void *data, uint32_t *size);

virtual	uint32_t    sizeMsgParentIdItem(RsMsgParentId *);
virtual	bool        serialiseMsgParentIdItem  (RsMsgParentId *item, void *data, uint32_t *size);
virtual	RsMsgParentId *deserialiseMsgParentIdItem(void *data, uint32_t *size);


	bool m_bConfiguration; // is set to true for saving configuration (enables serialising msgId)
};

/**************************************************************************/

#endif /* RS_MSG_ITEMS_H */


