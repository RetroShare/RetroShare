/*******************************************************************************
 * libretroshare/src/chat: rschatitems.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie.                                       *
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

#include "openssl/bn.h"
#include "retroshare/rstypes.h"
#include "serialiser/rsserializer.h"
#include "serialiser/rstlvkeys.h"
#include "rsitems/rsserviceids.h"
#include "rsitems/itempriorities.h"
#include "rsitems/rsitem.h"
#include "serialiser/rsserial.h"
#include "util/rsdeprecate.h"

#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvfileitem.h"
#include "retroshare/rsmsgs.h"

/* chat Flags */
const uint32_t RS_CHAT_FLAG_PRIVATE                    = 0x0001;
const uint32_t RS_CHAT_FLAG_REQUESTS_AVATAR            = 0x0002;
const uint32_t RS_CHAT_FLAG_CONTAINS_AVATAR            = 0x0004;
const uint32_t RS_CHAT_FLAG_AVATAR_AVAILABLE           = 0x0008;
const uint32_t RS_CHAT_FLAG_CUSTOM_STATE               = 0x0010;  // used for transmitting peer status string
const uint32_t RS_CHAT_FLAG_PUBLIC                     = 0x0020;
const uint32_t RS_CHAT_FLAG_REQUEST_CUSTOM_STATE       = 0x0040;
const uint32_t RS_CHAT_FLAG_CUSTOM_STATE_AVAILABLE     = 0x0080;
const uint32_t RS_CHAT_FLAG_PARTIAL_MESSAGE            = 0x0100;
const uint32_t RS_CHAT_FLAG_LOBBY                      = 0x0200;
const uint32_t RS_CHAT_FLAG_CLOSING_DISTANT_CONNECTION = 0x0400;
const uint32_t RS_CHAT_FLAG_ACK_DISTANT_CONNECTION     = 0x0800;
const uint32_t RS_CHAT_FLAG_KEEP_ALIVE                 = 0x1000;
const uint32_t RS_CHAT_FLAG_CONNEXION_REFUSED          = 0x2000;

const uint32_t RS_CHATMSG_CONFIGFLAG_INCOMING 		= 0x0001;

const uint8_t RS_PKT_SUBTYPE_CHAT_AVATAR           	  = 0x03 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_STATUS           	  = 0x04 ;	
const uint8_t RS_PKT_SUBTYPE_PRIVATECHATMSG_CONFIG 	  = 0x05 ;	
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_MSG_DEPRECATED    = 0x06 ;	// don't use ! Deprecated
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE_DEPREC     = 0x07 ;	// don't use ! Deprecated
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_ACCEPT     	  = 0x08 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_CHALLENGE  	  = 0x09 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_UNSUBSCRIBE	  = 0x0A ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_EVENT_DEPREC      = 0x0B ;	// don't use ! Deprecated
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_MSG        	  = 0x0C ;	// will be deprecated when only signed messages are accepted (02/2015)
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_REQUEST 	  = 0x0D ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_deprecated   = 0x0E ;	// to be removed
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE_deprecated = 0x0F ;	// to be removed
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_EVENT       	  = 0x10 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_deprecated2  = 0x11 ;	// to be removed (deprecated since 02 Dec. 2012)
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_deprecated3  = 0x12 ;
const uint8_t RS_PKT_SUBTYPE_DISTANT_INVITE_CONFIG   	  = 0x13 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_CONFIG      	  = 0x15 ;
const uint8_t RS_PKT_SUBTYPE_DISTANT_CHAT_DH_PUBLIC_KEY   = 0x16 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_SIGNED_MSG     	  = 0x17 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_SIGNED_EVENT      = 0x18 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_LIST              = 0x19 ;

RS_DEPRECATED_FOR(RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE) \
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE_DEPRECATED = 0x1A ;	// to be removed (deprecated since May 2017)

const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE            = 0x1B ;
const uint8_t RS_PKT_SUBTYPE_OUTGOING_MAP                 = 0x1C ;

const uint8_t RS_PKT_SUBTYPE_SUBSCRIBED_CHAT_LOBBY_CONFIG = 0x1D ;

typedef uint64_t 		ChatLobbyId ;
typedef uint64_t 		ChatLobbyMsgId ;
typedef std::string 	ChatLobbyNickName ;
typedef uint64_t		DistantChatDHSessionId ;

class RsChatItem: public RsItem
{
	public:
		RsChatItem(uint8_t chat_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_CHAT,chat_subtype) 
		{
			setPriorityLevel(QOS_PRIORITY_RS_CHAT_ITEM) ;
		}

		virtual ~RsChatItem() {}
        virtual void clear() {}
};

/*!
 * For sending chat msgs
 * @see p3ChatService
 */
class RsChatMsgItem: public RsChatItem
{
public:
    RsChatMsgItem() :RsChatItem(RS_PKT_SUBTYPE_DEFAULT) {}
    RsChatMsgItem(uint8_t subtype) :RsChatItem(subtype) {}

    //RsChatMsgItem() {}

    virtual ~RsChatMsgItem() {}

    // derived from RsItem

	void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
    virtual void clear() {}

    uint32_t chatFlags;
    uint32_t sendTime;
    std::string message;

    /* not serialised */
    uint32_t recvTime;
};

// This class contains the info to bounce an object throughout a lobby, while
// maintaining cache info to avoid duplicates.
//
class RsChatLobbyBouncingObject
{
public:
    ChatLobbyId lobby_id ;
    ChatLobbyMsgId msg_id ;
    ChatLobbyNickName nick ;	// Nickname of sender

    RsTlvKeySignature signature ;

    virtual RsChatLobbyBouncingObject *duplicate() const = 0 ;

protected:
    // The functions below handle the serialisation of data that is specific to the bouncing object level.
    // They are called by serial_size() and serialise() from children, but should not overload the serial_size() and
    // serialise() methods, otherwise the wrong method will be called when serialising from this top level class.

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    virtual uint32_t PacketId() const= 0;
};

class RsChatLobbyMsgItem: public RsChatMsgItem, public RsChatLobbyBouncingObject
{
public:
    RsChatLobbyMsgItem() :RsChatMsgItem(RS_PKT_SUBTYPE_CHAT_LOBBY_SIGNED_MSG) {}

    virtual ~RsChatLobbyMsgItem() {}
    virtual RsChatLobbyBouncingObject *duplicate() const { return new RsChatLobbyMsgItem(*this) ; }

	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    ChatLobbyMsgId parent_msg_id ;				// Used for threaded chat.

protected:
    virtual uint32_t PacketId() const { return RsChatMsgItem::PacketId() ; }
};

class RsChatLobbyEventItem: public RsChatItem, public RsChatLobbyBouncingObject
{
public:
	RsChatLobbyEventItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_SIGNED_EVENT) {}

	virtual ~RsChatLobbyEventItem() {}
	virtual RsChatLobbyBouncingObject *duplicate() const { return new RsChatLobbyEventItem(*this) ; }
	//
	virtual void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	// members.
	//
	uint8_t event_type ;		// used for defining the type of event.
	std::string string1;		// used for any string
	uint32_t sendTime;		// used to check for old looping messages

protected:
	virtual uint32_t PacketId() const { return RsChatItem::PacketId() ; }
};

class RsChatLobbyListRequestItem: public RsChatItem
{
	public:
		RsChatLobbyListRequestItem() : RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_REQUEST) {}
		virtual ~RsChatLobbyListRequestItem() {}

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);
};

struct VisibleChatLobbyInfo
{
    ChatLobbyId id ;
    std::string name ;
    std::string topic ;
    uint32_t    count ;
    ChatLobbyFlags flags ;
};

class RsChatLobbyListItem: public RsChatItem
{
	public:
		RsChatLobbyListItem() : RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_LIST) {}
		virtual ~RsChatLobbyListItem() {}

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

        std::vector<VisibleChatLobbyInfo> lobbies ;
};

class RsChatLobbyUnsubscribeItem: public RsChatItem
{
	public:
		RsChatLobbyUnsubscribeItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_UNSUBSCRIBE) {}

		virtual ~RsChatLobbyUnsubscribeItem() {} 

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		uint64_t lobby_id ;
};

class RsChatLobbyConnectChallengeItem: public RsChatItem
{
	public:
		RsChatLobbyConnectChallengeItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_CHALLENGE) {}

		virtual ~RsChatLobbyConnectChallengeItem() {} 

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		uint64_t challenge_code ;
};

/// @deprecated since May 2017, to be removed
class RsChatLobbyInviteItem_Deprecated : public RsChatItem
{
	public:
		RsChatLobbyInviteItem_Deprecated() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE_DEPRECATED) {}
		virtual ~RsChatLobbyInviteItem_Deprecated() {}

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		ChatLobbyId lobby_id ;
		std::string lobby_name ;
		std::string lobby_topic ;
		ChatLobbyFlags lobby_flags ;
};

class RsChatLobbyInviteItem: public RsChatItem
{
	public:
		RsChatLobbyInviteItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE) {}
		virtual ~RsChatLobbyInviteItem() {}

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		ChatLobbyId lobby_id ;
		std::string lobby_name ;
		std::string lobby_topic ;
		ChatLobbyFlags lobby_flags ;
};

/*!
 * For saving incoming and outgoing chat msgs
 * @see p3ChatService
 */
struct RsPrivateChatMsgConfigItem : RsChatItem
{
	RsPrivateChatMsgConfigItem() :
	    RsChatItem(RS_PKT_SUBTYPE_PRIVATECHATMSG_CONFIG) {}

	virtual ~RsPrivateChatMsgConfigItem() {}
	virtual void clear() {}

	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx );

	/** set data from RsChatMsgItem to RsPrivateChatMsgConfigItem */
	void set(RsChatMsgItem *ci, const RsPeerId &peerId, uint32_t confFlags);
	/** get data from RsPrivateChatMsgConfigItem to RsChatMsgItem */
	void get(RsChatMsgItem *ci);

	RsPeerId configPeerId;
	uint32_t chatFlags;
	uint32_t configFlags;
	uint32_t sendTime;
	std::string message;
	uint32_t recvTime;
};

class RsSubscribedChatLobbyConfigItem: public RsChatItem
{
public:
    RsSubscribedChatLobbyConfigItem() :RsChatItem(RS_PKT_SUBTYPE_SUBSCRIBED_CHAT_LOBBY_CONFIG) {}
	virtual ~RsSubscribedChatLobbyConfigItem() {}

    virtual void clear() { RsChatItem::clear(); info.clear(); }

	void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

    ChatLobbyInfo info;
};

class RsChatLobbyConfigItem: public RsChatItem
{
public:
	RsChatLobbyConfigItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_CONFIG) { lobby_Id = 0; }

	virtual ~RsChatLobbyConfigItem() {}

	virtual void clear() { lobby_Id = 0; }

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

	uint64_t lobby_Id;
	uint32_t flags ;
};

// This class contains activity info for the sending peer: active, idle, typing, etc.
//
class RsChatStatusItem: public RsChatItem
{
	public:
		RsChatStatusItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_STATUS) {}

		virtual ~RsChatStatusItem() {}

		void serial_process(RsGenericSerializer::SerializeJob j,RsGenericSerializer::SerializeContext& ctx);

		uint32_t flags ;
		std::string status_string;
};

// This class contains avatar images in Qt format.
//
class RsChatAvatarItem: public RsChatItem
{
public:
	RsChatAvatarItem():
	    RsChatItem(RS_PKT_SUBTYPE_CHAT_AVATAR),
	    image_size(0), image_data(nullptr)
	{ setPriorityLevel(QOS_PRIORITY_RS_CHAT_AVATAR_ITEM); }

	~RsChatAvatarItem() override { free(image_data); }

	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext& ctx) override;

	uint32_t image_size; /// size of data in bytes
	unsigned char* image_data ; /// image data
};


struct PrivateOugoingMapItem : RsChatItem
{
	PrivateOugoingMapItem() : RsChatItem(RS_PKT_SUBTYPE_OUTGOING_MAP) {}

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx );

	std::map<uint64_t, RsChatMsgItem> store;
};

struct RsChatSerialiser : RsServiceSerializer
{
	RsChatSerialiser(RsSerializationFlags flags = RsSerializationFlags::NONE):
	    RsServiceSerializer(RS_SERVICE_TYPE_CHAT, flags) {}

	virtual RsItem *create_item(uint16_t service_id,uint8_t item_sub_id) const;
};

