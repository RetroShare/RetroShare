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

#include "retroshare/rstypes.h"
#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"

#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvfileitem.h"

#if 0
#include "serialiser/rstlvtypes.h"
#include "serialiser/rstlvfileitem.h"
#endif

/**************************************************************************/

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

const uint32_t RS_CHATMSG_CONFIGFLAG_INCOMING 		= 0x0001;

const uint8_t RS_PKT_SUBTYPE_CHAT_AVATAR           	  = 0x03 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_STATUS           	  = 0x04 ;	
const uint8_t RS_PKT_SUBTYPE_PRIVATECHATMSG_CONFIG 	  = 0x05 ;	
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_MSG_DEPRECATED  = 0x06 ;	// don't use ! Deprecated
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE_DEPREC   = 0x07 ;	// don't use ! Deprecated
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_ACCEPT     	  = 0x08 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_CHALLENGE  	  = 0x09 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_UNSUBSCRIBE	  = 0x0A ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_EVENT_DEPREC    = 0x0B ;	// don't use ! Deprecated
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_MSG        	  = 0x0C ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_REQUEST 	  = 0x0D ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_deprecated = 0x0E ;	// to be removed
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE       	  = 0x0F ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_EVENT       	  = 0x10 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_deprecated2	= 0x11 ;	// to be removed (deprecated since 02 Dec. 2012)
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_LIST         	  = 0x12 ;
const uint8_t RS_PKT_SUBTYPE_DISTANT_INVITE_CONFIG   	  = 0x13 ;
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_CONFIG      	  = 0x15 ;

// for defining tags themselves and msg tags
const uint8_t RS_PKT_SUBTYPE_MSG_TAG_TYPE 	= 0x03;
const uint8_t RS_PKT_SUBTYPE_MSG_TAGS 			= 0x04;
const uint8_t RS_PKT_SUBTYPE_MSG_SRC_TAG 		= 0x05;
const uint8_t RS_PKT_SUBTYPE_MSG_PARENT_TAG 	= 0x06;
const uint8_t RS_PKT_SUBTYPE_MSG_INVITE    	= 0x07;

typedef uint64_t 		ChatLobbyId ;
typedef uint64_t 		ChatLobbyMsgId ;
typedef std::string 	ChatLobbyNickName ;

class RsChatItem: public RsItem
{
	public:
		RsChatItem(uint8_t chat_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_CHAT,chat_subtype) 
		{
			setPriorityLevel(QOS_PRIORITY_RS_CHAT_ITEM) ;
		}

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
		RsChatMsgItem(uint8_t subtype) :RsChatItem(subtype) {}

		RsChatMsgItem(void *data,uint32_t size,uint8_t subtype = RS_PKT_SUBTYPE_DEFAULT) ; // deserialization

		virtual ~RsChatMsgItem() {}
		virtual void clear() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() ; 							// deserialise is handled using a constructor

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

		virtual RsChatLobbyBouncingObject *duplicate() const = 0 ;
		virtual uint32_t serial_size() ;
		virtual bool serialise(void *data,uint32_t tlvsize,uint32_t& offset) ;
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		bool deserialise(void *data,uint32_t rssize,uint32_t& offset) ;
};

class RsChatLobbyMsgItem: public RsChatMsgItem, public RsChatLobbyBouncingObject
{
	public:
		RsChatLobbyMsgItem() :RsChatMsgItem(RS_PKT_SUBTYPE_CHAT_LOBBY_MSG) {}

		RsChatLobbyMsgItem(void *data,uint32_t size) ; // deserialization /// TODO!!!

		virtual ~RsChatLobbyMsgItem() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);
		virtual RsChatLobbyBouncingObject *duplicate() const { return new RsChatLobbyMsgItem(*this) ; }

		virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() ;				 				// deserialise is handled using a constructor

		uint8_t subpacket_id ;											// this is for proper handling of split packets. 
		ChatLobbyMsgId parent_msg_id ;								// Used for threaded chat.
};

class RsChatLobbyEventItem: public RsChatItem, public RsChatLobbyBouncingObject
{
	public:
		RsChatLobbyEventItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_EVENT) {}
		RsChatLobbyEventItem(void *data,uint32_t size) ; // deserialization /// TODO!!!

		virtual ~RsChatLobbyEventItem() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);
		virtual RsChatLobbyBouncingObject *duplicate() const { return new RsChatLobbyEventItem(*this) ; }
		// 
		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ;				 			

		// members.
		//
		uint8_t event_type ;		// used for defining the type of event.
		std::string string1;		// used for any string
		uint32_t sendTime;		// used to check for old looping messages
};

class RsChatLobbyListRequestItem: public RsChatItem
{
	public:
		RsChatLobbyListRequestItem() : RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_REQUEST) {}
		RsChatLobbyListRequestItem(void *data,uint32_t size) ; 
		virtual ~RsChatLobbyListRequestItem() {}

		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ;				 			

		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);
};
class RsChatLobbyListItem_deprecated: public RsChatItem
{
	public:
		RsChatLobbyListItem_deprecated() : RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_deprecated) {}
		RsChatLobbyListItem_deprecated(void *data,uint32_t size) ; 
		virtual ~RsChatLobbyListItem_deprecated() {}

		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ;				 			

		std::vector<ChatLobbyId> lobby_ids ;
		std::vector<std::string> lobby_names ;
		std::vector<uint32_t>    lobby_counts ;
};
class RsChatLobbyListItem_deprecated2: public RsChatItem
{
	public:
		RsChatLobbyListItem_deprecated2() : RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_LIST_deprecated2) {}
		RsChatLobbyListItem_deprecated2(void *data,uint32_t size) ; 
		virtual ~RsChatLobbyListItem_deprecated2() {}

		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ;				 			

		std::vector<ChatLobbyId> lobby_ids ;
		std::vector<std::string> lobby_names ;
		std::vector<std::string> lobby_topics ;
		std::vector<uint32_t>    lobby_counts ;
};
class RsChatLobbyListItem: public RsChatItem
{
	public:
		RsChatLobbyListItem() : RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_LIST) {}
		RsChatLobbyListItem(void *data,uint32_t size) ; 
		virtual ~RsChatLobbyListItem() {}

		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ;				 			

		std::vector<ChatLobbyId> lobby_ids ;
		std::vector<std::string> lobby_names ;
		std::vector<std::string> lobby_topics ;
		std::vector<uint32_t>    lobby_counts ;
		std::vector<uint32_t>    lobby_privacy_levels ;
};
class RsChatLobbyUnsubscribeItem: public RsChatItem
{
	public:
		RsChatLobbyUnsubscribeItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_UNSUBSCRIBE) {}
		RsChatLobbyUnsubscribeItem(void *data,uint32_t size) ; // deserialization 

		virtual ~RsChatLobbyUnsubscribeItem() {} 
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint64_t lobby_id ;

		virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() ; 							// deserialise is handled using a constructor
};


class RsChatLobbyConnectChallengeItem: public RsChatItem
{
	public:
		RsChatLobbyConnectChallengeItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_CHALLENGE) {}
		RsChatLobbyConnectChallengeItem(void *data,uint32_t size) ; // deserialization 

		virtual ~RsChatLobbyConnectChallengeItem() {} 
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		uint64_t challenge_code ;

		virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() ; 							// deserialise is handled using a constructor
};

class RsChatLobbyInviteItem: public RsChatItem
{
	public:
		RsChatLobbyInviteItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE) {}
		RsChatLobbyInviteItem(void *data,uint32_t size) ; // deserialization 

		virtual ~RsChatLobbyInviteItem() {} 
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		ChatLobbyId lobby_id ;
		std::string lobby_name ;
		std::string lobby_topic ;
		uint32_t lobby_privacy_level ;

		virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() ; 							// deserialise is handled using a constructor
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
		void set(RsChatMsgItem *ci, const RsPeerId &peerId, uint32_t confFlags);
		/* get data from RsPrivateChatMsgConfigItem to RsChatMsgItem */
		void get(RsChatMsgItem *ci);

		RsPeerId configPeerId;
		uint32_t chatFlags;
		uint32_t configFlags;
		uint32_t sendTime;
		std::string message;
		uint32_t recvTime;
};
class RsPrivateChatDistantInviteConfigItem: public RsChatItem
{
	public:
		RsPrivateChatDistantInviteConfigItem() :RsChatItem(RS_PKT_SUBTYPE_DISTANT_INVITE_CONFIG) {}
		RsPrivateChatDistantInviteConfigItem(void *data,uint32_t size) ; // deserialization

		virtual ~RsPrivateChatDistantInviteConfigItem() {}
		virtual void clear() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() ; 							// deserialise is handled using a constructor

		unsigned char aes_key[16] ;
        RsFileHash hash ;
		std::string encrypted_radix64_string ;
		RsPgpId destination_pgp_id ;
		uint32_t time_of_validity ;
		uint32_t last_hit_time ;
		uint32_t flags ;
};
class RsChatLobbyConfigItem: public RsChatItem
{
public:
    RsChatLobbyConfigItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_CONFIG) { lobby_Id = 0; }
    RsChatLobbyConfigItem(void *data,uint32_t size) ; // deserialization

    virtual ~RsChatLobbyConfigItem() {}

    virtual void clear() { lobby_Id = 0; }
    virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

    virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
    virtual uint32_t serial_size() ; 							// deserialise is handled using a constructor

    uint64_t lobby_Id;
	 uint32_t flags ;
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
		RsChatAvatarItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_AVATAR) {setPriorityLevel(QOS_PRIORITY_RS_CHAT_AVATAR_ITEM) ;}
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

const uint32_t RS_MSG_FLAGS_OUTGOING              = 0x00000001;
const uint32_t RS_MSG_FLAGS_PENDING               = 0x00000002;
const uint32_t RS_MSG_FLAGS_DRAFT                 = 0x00000004;
const uint32_t RS_MSG_FLAGS_NEW                   = 0x00000010;
const uint32_t RS_MSG_FLAGS_TRASH                 = 0x00000020;
const uint32_t RS_MSG_FLAGS_UNREAD_BY_USER        = 0x00000040;
const uint32_t RS_MSG_FLAGS_REPLIED               = 0x00000080;
const uint32_t RS_MSG_FLAGS_FORWARDED             = 0x00000100;
const uint32_t RS_MSG_FLAGS_STAR                  = 0x00000200;
const uint32_t RS_MSG_FLAGS_PARTIAL               = 0x00000400;
// system message
const uint32_t RS_MSG_FLAGS_USER_REQUEST          = 0x00000800;
const uint32_t RS_MSG_FLAGS_FRIEND_RECOMMENDATION = 0x00001000;
const uint32_t RS_MSG_FLAGS_SYSTEM                = RS_MSG_FLAGS_USER_REQUEST | RS_MSG_FLAGS_FRIEND_RECOMMENDATION;
const uint32_t RS_MSG_FLAGS_RETURN_RECEPT         = 0x00002000;
const uint32_t RS_MSG_FLAGS_ENCRYPTED             = 0x00004000;
const uint32_t RS_MSG_FLAGS_DISTANT               = 0x00008000;
const uint32_t RS_MSG_FLAGS_SIGNATURE_CHECKS      = 0x00010000;
const uint32_t RS_MSG_FLAGS_SIGNED                = 0x00020000;
const uint32_t RS_MSG_FLAGS_LOAD_EMBEDDED_IMAGES  = 0x00040000;
const uint32_t RS_MSG_FLAGS_DECRYPTED             = 0x00080000;

class RsMessageItem: public RsItem
{
	public:
		RsMessageItem(uint8_t msg_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_MSG,msg_subtype) 
		{
			setPriorityLevel(QOS_PRIORITY_RS_MSG_ITEM) ;
		}

		virtual ~RsMessageItem() {}
		virtual void clear() {}
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0) = 0 ;

		virtual bool serialise(void *data,uint32_t& size,bool config) = 0 ;	
		virtual uint32_t serial_size(bool config) = 0 ; 						
};


class RsMsgItem: public RsMessageItem
{
	public:
		RsMsgItem() :RsMessageItem(RS_PKT_SUBTYPE_DEFAULT) {}

		virtual ~RsMsgItem() {}
		virtual void clear();

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

		// ----------- Specific fields ------------- //

		uint32_t msgFlags;
		uint32_t msgId;

		uint32_t sendTime;
		uint32_t recvTime;

		std::string subject;
		std::string message;

		RsTlvPeerIdSet rspeerid_msgto;
		RsTlvPeerIdSet rspeerid_msgcc;
		RsTlvPeerIdSet rspeerid_msgbcc;

		RsTlvGxsIdSet rsgxsid_msgto;
		RsTlvGxsIdSet rsgxsid_msgcc;
		RsTlvGxsIdSet rsgxsid_msgbcc;

		RsTlvFileSet attachment;
};

class RsMsgTagType : public RsMessageItem
{
	public:
		RsMsgTagType() :RsMessageItem(RS_PKT_SUBTYPE_MSG_TAG_TYPE) {}

		virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

		virtual ~RsMsgTagType() {}
		virtual void clear();

		// ----------- Specific fields ------------- //
		//
		std::string text;
		uint32_t rgb_color;
		uint32_t tagId;
};

class RsMsgTags : public RsMessageItem
{
public:
	RsMsgTags()
		:RsMessageItem(RS_PKT_SUBTYPE_MSG_TAGS) {}

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

	virtual std::ostream &print(std::ostream &out, uint16_t indent = 0);

	virtual ~RsMsgTags() {}
	virtual void clear();

		// ----------- Specific fields ------------- //
		//
	uint32_t msgId;
	std::list<uint32_t> tagIds;
};

class RsMsgSrcId : public RsMessageItem
{
	public:
		RsMsgSrcId() : RsMessageItem(RS_PKT_SUBTYPE_MSG_SRC_TAG) {}

		std::ostream &print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

		virtual ~RsMsgSrcId() {}
		virtual void clear();

		// ----------- Specific fields ------------- //
		//

		uint32_t msgId;
		RsPeerId srcId;
};
class RsPublicMsgInviteConfigItem : public RsMessageItem
{
	public:
		RsPublicMsgInviteConfigItem() : RsMessageItem(RS_PKT_SUBTYPE_MSG_INVITE) {}

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

		std::ostream &print(std::ostream &out, uint16_t indent = 0);

		virtual ~RsPublicMsgInviteConfigItem() {}
		virtual void clear();

		// ----------- Specific fields ------------- //
		//
		std::string hash ;
		time_t time_stamp ;
};


class RsMsgParentId : public RsMessageItem
{
	public:
		RsMsgParentId() : RsMessageItem(RS_PKT_SUBTYPE_MSG_PARENT_TAG) {}

		std::ostream &print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size,bool config) ;	
		virtual uint32_t serial_size(bool config) ; 						

		virtual ~RsMsgParentId() {}
		virtual void clear();

		// ----------- Specific fields ------------- //
		//
		uint32_t msgId;
		uint32_t msgParentId;
};

class RsMsgSerialiser: public RsSerialType
{
	public:
		RsMsgSerialiser(bool bConfiguration = false)
			:RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_MSG), m_bConfiguration (bConfiguration) {}

		RsMsgSerialiser(uint16_t type)
			:RsSerialType(RS_PKT_VERSION_SERVICE, type), m_bConfiguration (false) {}

		virtual     ~RsMsgSerialiser() {}

		virtual	uint32_t    size(RsItem *item) 
		{ 
			return dynamic_cast<RsMessageItem*>(item)->serial_size(m_bConfiguration) ; 
		}
		virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size)
		{
			return dynamic_cast<RsMessageItem*>(item)->serialise(data,*size,m_bConfiguration) ; 
		}
		virtual	RsItem *    deserialise(void *data, uint32_t *size);

	private:

		virtual	RsMsgItem                   *deserialiseMsgItem(void *data, uint32_t *size);
		virtual	RsMsgTagType                *deserialiseTagItem(void *data, uint32_t *size);
		virtual	RsMsgTags                   *deserialiseMsgTagItem(void *data, uint32_t *size);
		virtual	RsMsgSrcId                  *deserialiseMsgSrcIdItem(void *data, uint32_t *size);
		virtual	RsMsgParentId               *deserialiseMsgParentIdItem(void *data, uint32_t *size);
		virtual	RsPublicMsgInviteConfigItem *deserialisePublicMsgInviteConfigItem(void *data, uint32_t *size);

		bool m_bConfiguration; // is set to true for saving configuration (enables serialising msgId)
};

/**************************************************************************/

#endif /* RS_MSG_ITEMS_H */


