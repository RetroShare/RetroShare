/*
 * libretroshare/src/serialiser: rschatitems.h
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

#pragma once

#include "openssl/bn.h"
#include "retroshare/rstypes.h"
#include "serialiser/rstlvkeys.h"
#include "serialiser/rsserviceids.h"
#include "serialiser/rsserial.h"

#include "serialiser/rstlvidset.h"
#include "serialiser/rstlvfileitem.h"

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
const uint8_t RS_PKT_SUBTYPE_CHAT_LOBBY_INVITE       	  = 0x1A ;

typedef uint64_t 		ChatLobbyId ;
typedef uint64_t 		ChatLobbyMsgId ;
typedef std::string 		ChatLobbyNickName ;
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

    RsTlvKeySignature signature ;

    virtual RsChatLobbyBouncingObject *duplicate() const = 0 ;
    virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

    // returns the size in bytes of the data chunk to sign.

    virtual uint32_t signed_serial_size() =0;
    virtual bool serialise_signed_part(void *data,uint32_t& size) = 0;

protected:
    // The functions below handle the serialisation of data that is specific to the bouncing object level.
    // They are called by serial_size() and serialise() from children, but should not overload the serial_size() and
    // serialise() methods, otherwise the wrong method will be called when serialising from this top level class.

    uint32_t serialized_size(bool include_signature) ;
    bool serialise_to_memory(void *data,uint32_t tlvsize,uint32_t& offset,bool include_signature) ;
    bool deserialise_from_memory(void *data,uint32_t rssize,uint32_t& offset) ;
};

class RsChatLobbyMsgItem: public RsChatMsgItem, public RsChatLobbyBouncingObject
{
public:
    RsChatLobbyMsgItem() :RsChatMsgItem(RS_PKT_SUBTYPE_CHAT_LOBBY_SIGNED_MSG) {}

    RsChatLobbyMsgItem(void *data,uint32_t size) ; // deserialization /// TODO!!!

    virtual ~RsChatLobbyMsgItem() {}
    virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);
    virtual RsChatLobbyBouncingObject *duplicate() const { return new RsChatLobbyMsgItem(*this) ; }

    virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
    virtual uint32_t serial_size() ;			// deserialise is handled using a constructor

    virtual uint32_t signed_serial_size() ;
    virtual bool serialise_signed_part(void *data,uint32_t& size) ;// Isn't it better that items can serialize themselves ?

    ChatLobbyMsgId parent_msg_id ;				// Used for threaded chat.
};

class RsChatLobbyEventItem: public RsChatItem, public RsChatLobbyBouncingObject
{
    public:
        RsChatLobbyEventItem() :RsChatItem(RS_PKT_SUBTYPE_CHAT_LOBBY_SIGNED_EVENT) {}
        RsChatLobbyEventItem(void *data,uint32_t size) ; // deserialization /// TODO!!!

        virtual ~RsChatLobbyEventItem() {}
        virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);
        virtual RsChatLobbyBouncingObject *duplicate() const { return new RsChatLobbyEventItem(*this) ; }
        //
        virtual bool serialise(void *data,uint32_t& size) ;
        virtual uint32_t serial_size() ;

        virtual uint32_t signed_serial_size() ;
    virtual bool serialise_signed_part(void *data,uint32_t& size) ;

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
		RsChatLobbyListItem(void *data,uint32_t size) ; 
		virtual ~RsChatLobbyListItem() {}

		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ;				 			

        std::vector<VisibleChatLobbyInfo> lobbies ;
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
        ChatLobbyFlags lobby_flags ;

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

// This class contains the public Diffie-Hellman parameters to be sent
// when performing a DH agreement over a distant chat tunnel.
//
class RsChatDHPublicKeyItem: public RsChatItem
{
	public:
		RsChatDHPublicKeyItem() :RsChatItem(RS_PKT_SUBTYPE_DISTANT_CHAT_DH_PUBLIC_KEY) {setPriorityLevel(QOS_PRIORITY_RS_CHAT_ITEM) ;}
		RsChatDHPublicKeyItem(void *data,uint32_t size) ; // deserialization

		virtual ~RsChatDHPublicKeyItem() { BN_free(public_key) ; } 
		virtual std::ostream& print(std::ostream &out, uint16_t indent = 0);

		virtual bool serialise(void *data,uint32_t& size) ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() ; 							// deserialise is handled using a constructor

		// Private data to DH public key item
		//
		BIGNUM *public_key ;

		RsTlvKeySignature signature ;	// signs the public key in a row.
		RsTlvSecurityKey_deprecated  gxs_key ;	// public key of the signer

	private:
		RsChatDHPublicKeyItem(const RsChatDHPublicKeyItem&) : RsChatItem(RS_PKT_SUBTYPE_DISTANT_CHAT_DH_PUBLIC_KEY) {}						// make the object non copy-able
		const RsChatDHPublicKeyItem& operator=(const RsChatDHPublicKeyItem&) { return *this ;}
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

