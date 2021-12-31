/*******************************************************************************
 * libretroshare/src/retroshare: rsmsgs.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2007-2008  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2016-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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

#include <list>
#include <iostream>
#include <string>
#include <set>
#include <assert.h>

#include "retroshare/rstypes.h"
#include "retroshare/rsgxsifacetypes.h"
#include "retroshare/rsevents.h"
#include "util/rsdeprecate.h"
#include "util/rsmemory.h"

/********************** For Messages and Channels *****************/

#define RS_MSG_BOXMASK   0x000f   /* Mask for determining Box */

#define RS_MSG_OUTGOING        0x0001   /* !Inbox */
#define RS_MSG_PENDING         0x0002   /* OutBox */
#define RS_MSG_DRAFT           0x0004   /* Draft  */

/* ORs of above */
#define RS_MSG_INBOX           0x00     /* Inbox */
#define RS_MSG_SENTBOX         0x01     /* Sentbox */
#define RS_MSG_OUTBOX          0x03     /* Outbox */
#define RS_MSG_DRAFTBOX        0x05     /* Draftbox */

#define RS_MSG_NEW                   0x000010   /* New */
#define RS_MSG_TRASH                 0x000020   /* Trash */
#define RS_MSG_UNREAD_BY_USER        0x000040   /* Unread by user */
#define RS_MSG_REPLIED               0x000080   /* Message is replied */
#define RS_MSG_FORWARDED             0x000100   /* Message is forwarded */
#define RS_MSG_STAR                  0x000200   /* Message is marked with a star */
// system message
#define RS_MSG_USER_REQUEST          0x000400   /* user request */
#define RS_MSG_FRIEND_RECOMMENDATION 0x000800   /* friend recommendation */
#define RS_MSG_DISTANT               0x001000	/* message is distant */
#define RS_MSG_SIGNATURE_CHECKS      0x002000	/* message was signed, and signature checked */
#define RS_MSG_SIGNED                0x004000	/* message was signed and signature didn't check */
#define RS_MSG_LOAD_EMBEDDED_IMAGES  0x008000   /* load embedded images */
#define RS_MSG_PUBLISH_KEY           0x020000   /* publish key */
#define RS_MSG_SPAM                  0x040000   /* Message is marked as spam */

#define RS_MSG_SYSTEM                (RS_MSG_USER_REQUEST | RS_MSG_FRIEND_RECOMMENDATION | RS_MSG_PUBLISH_KEY)

#define RS_CHAT_LOBBY_EVENT_PEER_LEFT   				0x01
#define RS_CHAT_LOBBY_EVENT_PEER_STATUS 				0x02
#define RS_CHAT_LOBBY_EVENT_PEER_JOINED 				0x03
#define RS_CHAT_LOBBY_EVENT_PEER_CHANGE_NICKNAME 	0x04
#define RS_CHAT_LOBBY_EVENT_KEEP_ALIVE          	0x05

#define RS_MSGTAGTYPE_IMPORTANT  1
#define RS_MSGTAGTYPE_WORK       2
#define RS_MSGTAGTYPE_PERSONAL   3
#define RS_MSGTAGTYPE_TODO       4
#define RS_MSGTAGTYPE_LATER      5
#define RS_MSGTAGTYPE_USER       100

//#define RS_CHAT_LOBBY_PRIVACY_LEVEL_CHALLENGE  	0	/* Used to accept connection challenges only. */
//#define RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC  		1	/* lobby is visible by friends. Friends can connect.*/
//#define RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE 		2	/* lobby invisible by friends. Peers on invitation only .*/

#define RS_CHAT_TYPE_PUBLIC  1
#define RS_CHAT_TYPE_PRIVATE 2
#define RS_CHAT_TYPE_LOBBY   3
#define RS_CHAT_TYPE_DISTANT 4

const ChatLobbyFlags RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE( 0x00000001 ) ;
const ChatLobbyFlags RS_CHAT_LOBBY_FLAGS_deprecated    ( 0x00000002 ) ;
const ChatLobbyFlags RS_CHAT_LOBBY_FLAGS_PUBLIC        ( 0x00000004 ) ;
const ChatLobbyFlags RS_CHAT_LOBBY_FLAGS_CHALLENGE     ( 0x00000008 ) ;
const ChatLobbyFlags RS_CHAT_LOBBY_FLAGS_PGP_SIGNED    ( 0x00000010 ) ; // requires the signing ID to be PGP-linked. Avoids anonymous crap.

typedef uint64_t	ChatLobbyId ;
typedef uint64_t	ChatLobbyMsgId ;
typedef std::string ChatLobbyNickName ;
typedef std::string RsMailMessageId; // TODO: rebase on t_RsGenericIdType

/**
 * Used to return a tracker id so the API user can keep track of sent mail
 * status, it contains mail id, and recipient id
 */
struct RsMailIdRecipientIdPair : RsSerializable
{
	RsMailIdRecipientIdPair(RsMailMessageId mailId, RsGxsId recipientId):
	    mMailId(mailId), mRecipientId(recipientId) {}

	RsMailMessageId mMailId;
	RsGxsId mRecipientId;

	/// @see RsSerializable
	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext &ctx ) override;

	bool operator<(const RsMailIdRecipientIdPair& other) const;
	bool operator==(const RsMailIdRecipientIdPair& other) const;

	RsMailIdRecipientIdPair() = default;
	~RsMailIdRecipientIdPair() override = default;
};

namespace Rs
{
namespace Msgs
{

class MsgAddress
{
	public:
		typedef enum { MSG_ADDRESS_TYPE_UNKNOWN  = 0x00,
							MSG_ADDRESS_TYPE_RSPEERID = 0x01, 
							MSG_ADDRESS_TYPE_RSGXSID  = 0x02, 
							MSG_ADDRESS_TYPE_EMAIL    = 0x03 } AddressType;

		typedef enum { MSG_ADDRESS_MODE_UNKNOWN = 0x00,
		               MSG_ADDRESS_MODE_TO      = 0x01,
		               MSG_ADDRESS_MODE_CC      = 0x02,
		               MSG_ADDRESS_MODE_BCC     = 0x03 } AddressMode;

		explicit MsgAddress(const RsGxsId&  gid, MsgAddress::AddressMode mmode)
			: _type(MSG_ADDRESS_TYPE_RSGXSID),  _mode(mmode), _addr_string(gid.toStdString()){}

		explicit MsgAddress(const RsPeerId& pid, MsgAddress::AddressMode mmode)
			: _type(MSG_ADDRESS_TYPE_RSPEERID), _mode(mmode), _addr_string(pid.toStdString()){}

		explicit MsgAddress(const std::string& email, MsgAddress::AddressMode mmode)
			: _type(MSG_ADDRESS_TYPE_EMAIL), _mode(mmode), _addr_string(email){}

		MsgAddress::AddressType type() { return _type ;}
		MsgAddress::AddressMode mode() { return _mode ;}

		RsGxsId toGxsId()     const { assert(_type==MSG_ADDRESS_TYPE_RSGXSID );return RsGxsId (_addr_string);}
		RsPeerId toRsPeerId() const { assert(_type==MSG_ADDRESS_TYPE_RSPEERID);return RsPeerId(_addr_string);}
		std::string toEmail() const { assert(_type==MSG_ADDRESS_TYPE_EMAIL   );return          _addr_string ;}

	private:
		AddressType _type ;
		AddressMode _mode ;
		std::string _addr_string ;
};

struct MessageInfo : RsSerializable
{
	MessageInfo(): msgflags(0), size(0), count(0), ts(0) {}

	std::string msgId;

    RsPeerId rspeerid_srcId;
    RsGxsId  rsgxsid_srcId;

    unsigned int msgflags;

	 // friend destinations
	 //
    std::set<RsPeerId> rspeerid_msgto;		// RsPeerId is used here for various purposes:
    std::set<RsPeerId> rspeerid_msgcc;		//    - real peer ids which are actual friend locations
    std::set<RsPeerId> rspeerid_msgbcc;		//    -

	 // distant peers
	 //
    std::set<RsGxsId> rsgxsid_msgto;		// RsPeerId is used here for various purposes:
    std::set<RsGxsId> rsgxsid_msgcc;		//    - real peer ids which are actual friend locations
    std::set<RsGxsId> rsgxsid_msgbcc;		//    -

    std::string title;
    std::string msg;

    std::string attach_title;
    std::string attach_comment;
    std::list<FileInfo> files;

    int size;  /* total of files */
    int count; /* file count     */

    int ts;

	// RsSerializable interface
	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext &ctx ) override
	{
		RS_SERIAL_PROCESS(msgId);

		RS_SERIAL_PROCESS(rspeerid_srcId);
		RS_SERIAL_PROCESS(rsgxsid_srcId);

		RS_SERIAL_PROCESS(msgflags);

		RS_SERIAL_PROCESS(rspeerid_msgto);
		RS_SERIAL_PROCESS(rspeerid_msgcc);
		RS_SERIAL_PROCESS(rspeerid_msgbcc);

		RS_SERIAL_PROCESS(rsgxsid_msgto);
		RS_SERIAL_PROCESS(rsgxsid_msgcc);
		RS_SERIAL_PROCESS(rsgxsid_msgcc);

		RS_SERIAL_PROCESS(title);
		RS_SERIAL_PROCESS(msg);

		RS_SERIAL_PROCESS(attach_title);
		RS_SERIAL_PROCESS(attach_comment);
		RS_SERIAL_PROCESS(files);

		RS_SERIAL_PROCESS(size);
		RS_SERIAL_PROCESS(count);

		RS_SERIAL_PROCESS(ts);
	}

	~MessageInfo() override;
};

struct MsgInfoSummary : RsSerializable
{
	MsgInfoSummary() : msgflags(0), count(0), ts(0) {}

	RsMailMessageId msgId;
	RsPeerId srcId;

	uint32_t msgflags;
	std::list<uint32_t> msgtags; /// that leaves 25 bits for user-defined tags.

	std::string title;
	int count; /** file count     */
	rstime_t ts;


	/// @see RsSerializable
	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext &ctx) override
	{
		RS_SERIAL_PROCESS(msgId);
		RS_SERIAL_PROCESS(srcId);

		RS_SERIAL_PROCESS(msgflags);
		RS_SERIAL_PROCESS(msgtags);

		RS_SERIAL_PROCESS(title);
		RS_SERIAL_PROCESS(count);
		RS_SERIAL_PROCESS(ts);
	}

	~MsgInfoSummary() override;
};

struct MsgTagInfo : RsSerializable
{
	virtual ~MsgTagInfo() = default;

	std::string msgId;
	std::list<uint32_t> tagIds;

	// RsSerializable interface
	void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
		RS_SERIAL_PROCESS(msgId);
		RS_SERIAL_PROCESS(tagIds);
	}
};

struct MsgTagType : RsSerializable
{
	virtual ~MsgTagType() = default;
	/* map containing tagId -> pair (text, rgb color) */
	std::map<uint32_t, std::pair<std::string, uint32_t> > types;

	// RsSerializable interface
	void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
		RS_SERIAL_PROCESS(types);
	}
};

} //namespace Rs
} //namespace Msgs

enum class RsMailStatusEventCode: uint8_t
{
	NEW_MESSAGE                     = 0x00,
	MESSAGE_REMOVED                 = 0x01,
	MESSAGE_SENT                    = 0x02,

	/// means the peer received the message
	MESSAGE_RECEIVED_ACK            = 0x03,

	/// An error occurred attempting to sign the message
	SIGNATURE_FAILED   = 0x04,

	MESSAGE_CHANGED                 = 0x05,
	TAG_CHANGED                     = 0x06,
};

struct RsMailStatusEvent : RsEvent
{
	RsMailStatusEvent() : RsEvent(RsEventType::MAIL_STATUS) {}

    RsMailStatusEventCode mMailStatusEventCode;
	std::set<RsMailMessageId> mChangedMsgIds;

	/// @see RsEvent
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RsEvent::serial_process(j, ctx);
		RS_SERIAL_PROCESS(mChangedMsgIds);
		RS_SERIAL_PROCESS(mMailStatusEventCode);
	}

	~RsMailStatusEvent() override = default;
};

enum class RsMailTagEventCode: uint8_t
{
	TAG_ADDED   = 0x00,
	TAG_CHANGED = 0x01,
	TAG_REMOVED = 0x02,
};

struct RsMailTagEvent : RsEvent
{
	RsMailTagEvent() : RsEvent(RsEventType::MAIL_TAG) {}

	RsMailTagEventCode mMailTagEventCode;
	std::set<std::string> mChangedMsgTagIds;

	/// @see RsEvent
	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RsEvent::serial_process(j, ctx);
		RS_SERIAL_PROCESS(mChangedMsgTagIds);
		RS_SERIAL_PROCESS(mMailTagEventCode);
	}

	~RsMailTagEvent() override = default;
};

#define RS_CHAT_PUBLIC 			0x0001
#define RS_CHAT_PRIVATE 		0x0002
#define RS_CHAT_AVATAR_AVAILABLE 	0x0004

#define RS_DISTANT_CHAT_STATUS_UNKNOWN			0x0000
#define RS_DISTANT_CHAT_STATUS_TUNNEL_DN   		0x0001
#define RS_DISTANT_CHAT_STATUS_CAN_TALK			0x0002
#define RS_DISTANT_CHAT_STATUS_REMOTELY_CLOSED 	0x0003

#define RS_DISTANT_CHAT_ERROR_NO_ERROR            0x0000 
#define RS_DISTANT_CHAT_ERROR_DECRYPTION_FAILED   0x0001 
#define RS_DISTANT_CHAT_ERROR_SIGNATURE_MISMATCH  0x0002 
#define RS_DISTANT_CHAT_ERROR_UNKNOWN_KEY         0x0003 
#define RS_DISTANT_CHAT_ERROR_UNKNOWN_HASH        0x0004 

#define RS_DISTANT_CHAT_FLAG_SIGNED               0x0001 
#define RS_DISTANT_CHAT_FLAG_SIGNATURE_OK         0x0002 

// flags to define who we accept to talk to. Each flag *removes* some people.

#define RS_DISTANT_MESSAGING_CONTACT_PERMISSION_FLAG_FILTER_NONE           0x0000 
#define RS_DISTANT_MESSAGING_CONTACT_PERMISSION_FLAG_FILTER_NON_CONTACTS   0x0001 
#define RS_DISTANT_MESSAGING_CONTACT_PERMISSION_FLAG_FILTER_EVERYBODY      0x0002 

#define RS_DISTANT_CHAT_CONTACT_PERMISSION_FLAG_FILTER_NONE           0x0000 
#define RS_DISTANT_CHAT_CONTACT_PERMISSION_FLAG_FILTER_NON_CONTACTS   0x0001 
#define RS_DISTANT_CHAT_CONTACT_PERMISSION_FLAG_FILTER_EVERYBODY      0x0002 

struct DistantChatPeerInfo : RsSerializable
{
    DistantChatPeerInfo() : status(0),pending_items(0) {}

	RsGxsId to_id ;
	RsGxsId own_id ;
	DistantChatPeerId peer_id ;	// this is the tunnel id actually
	uint32_t status ;			// see the values in rsmsgs.h
    uint32_t pending_items;		// items not sent, waiting for a tunnel

	///* @see RsEvent @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext& ctx ) override
	{
		RS_SERIAL_PROCESS(to_id);
		RS_SERIAL_PROCESS(own_id);
		RS_SERIAL_PROCESS(peer_id);
		RS_SERIAL_PROCESS(status);
		RS_SERIAL_PROCESS(pending_items);
	}
};

// Identifier for an chat endpoint like
// neighbour peer, distant peer, chatlobby, broadcast
class ChatId : RsSerializable
{
public:
	ChatId();
	virtual ~ChatId() = default;

    explicit ChatId(RsPeerId     id);
    explicit ChatId(ChatLobbyId  id);
    explicit ChatId(DistantChatPeerId id);
    explicit ChatId(std::string str);
    static ChatId makeBroadcastId();

    std::string toStdString() const;
    bool operator<(const ChatId& other) const;
    bool isSameEndpoint(const ChatId& other) const;

    bool operator==(const ChatId& other) const { return isSameEndpoint(other) ; }

    bool isNotSet() const;
    bool isPeerId() const;
    bool isDistantChatId() const;
    bool isLobbyId() const;
    bool isBroadcast() const;

    RsPeerId    toPeerId()  const;
    ChatLobbyId toLobbyId() const;
    DistantChatPeerId toDistantChatId() const;

    // for the very specific case of transfering a status string
    // from the chatservice to the gui,
    // this defines from which peer the status string came from
    RsPeerId broadcast_status_peer_id;
private:
	enum Type : uint8_t
	{	TYPE_NOT_SET,
		TYPE_PRIVATE,            // private chat with directly connected friend, peer_id is valid
		TYPE_PRIVATE_DISTANT,    // private chat with distant peer, gxs_id is valid
		TYPE_LOBBY,              // chat lobby id, lobby_id is valid
		TYPE_BROADCAST           // message to/from all connected peers
	};

    Type type;
    RsPeerId peer_id;
    DistantChatPeerId distant_chat_id;
    ChatLobbyId lobby_id;

	// RsSerializable interface
public:
	void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
		RS_SERIAL_PROCESS(broadcast_status_peer_id);
		RS_SERIAL_PROCESS(type);
		RS_SERIAL_PROCESS(peer_id);
		RS_SERIAL_PROCESS(distant_chat_id);
		RS_SERIAL_PROCESS(lobby_id);
	}
};

struct ChatMessage : RsSerializable
{
    ChatId chat_id; // id of chat endpoint
    RsPeerId broadcast_peer_id; // only used for broadcast chat: source peer id
    RsGxsId lobby_peer_gxs_id; // only used for lobbys: nickname of message author
    std::string peer_alternate_nickname; // only used when key is unknown.

    unsigned int chatflags;
    uint32_t sendTime;
    uint32_t recvTime;
    std::string msg;
    bool incoming;
    bool online; // for outgoing messages: was this message send?
    //bool system_message;

	///* @see RsEvent @see RsSerializable
	void serial_process( RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext& ctx ) override
	{
		RS_SERIAL_PROCESS(chat_id);
		RS_SERIAL_PROCESS(broadcast_peer_id);
		RS_SERIAL_PROCESS(lobby_peer_gxs_id);
		RS_SERIAL_PROCESS(peer_alternate_nickname);

		RS_SERIAL_PROCESS(chatflags);
		RS_SERIAL_PROCESS(sendTime);
		RS_SERIAL_PROCESS(recvTime);
		RS_SERIAL_PROCESS(msg);
		RS_SERIAL_PROCESS(incoming);
		RS_SERIAL_PROCESS(online);
	}
};

class ChatLobbyInvite : RsSerializable
{
public:
	virtual ~ChatLobbyInvite() = default;

	ChatLobbyId lobby_id ;
	RsPeerId peer_id ;
	std::string lobby_name ;
	std::string lobby_topic ;
	ChatLobbyFlags lobby_flags ;

	// RsSerializable interface
public:
	void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
		RS_SERIAL_PROCESS(lobby_id);
		RS_SERIAL_PROCESS(peer_id);
		RS_SERIAL_PROCESS(lobby_name);
		RS_SERIAL_PROCESS(lobby_topic);
		RS_SERIAL_PROCESS(lobby_flags);
	}
};

struct VisibleChatLobbyRecord : RsSerializable
{
	VisibleChatLobbyRecord():
	    lobby_id(0), total_number_of_peers(0), last_report_time(0) {}

	ChatLobbyId lobby_id ;						// unique id of the lobby
	std::string lobby_name ;					// name to use for this lobby
	std::string lobby_topic ;					// topic to use for this lobby
	std::set<RsPeerId> participating_friends ;	// list of direct friend who participate.

	uint32_t total_number_of_peers ;			// total number of particpating peers. Might not be
	rstime_t last_report_time ; 					// last time the lobby was reported.
	ChatLobbyFlags lobby_flags ;				// see RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC / RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE

	/// @see RsSerializable
	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext &ctx) override
	{
		RS_SERIAL_PROCESS(lobby_id);
		RS_SERIAL_PROCESS(lobby_name);
		RS_SERIAL_PROCESS(lobby_topic);
		RS_SERIAL_PROCESS(participating_friends);

		RS_SERIAL_PROCESS(total_number_of_peers);
		RS_SERIAL_PROCESS(last_report_time);
		RS_SERIAL_PROCESS(lobby_flags);
	}

	~VisibleChatLobbyRecord() override;
};

class ChatLobbyInfo : RsSerializable
{
public:
	virtual ~ChatLobbyInfo() = default;

	ChatLobbyId lobby_id ;						// unique id of the lobby
	std::string lobby_name ;					// name to use for this lobby
	std::string lobby_topic ;					// topic to use for this lobby
	std::set<RsPeerId> participating_friends ;	// list of direct friend who participate. Used to broadcast sent messages.
	RsGxsId gxs_id ;							// ID to sign messages

	ChatLobbyFlags lobby_flags ;				// see RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC / RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE
	std::map<RsGxsId, rstime_t> gxs_ids ;			// list of non direct friend who participate. Used to display only.
	rstime_t last_activity ;						// last recorded activity. Useful for removing dead lobbies.

    virtual void clear() { gxs_ids.clear(); lobby_id = 0; lobby_name.clear(); lobby_topic.clear(); participating_friends.clear(); }

	// RsSerializable interface
public:
	void serial_process(RsGenericSerializer::SerializeJob j, RsGenericSerializer::SerializeContext &ctx) {
		RS_SERIAL_PROCESS(lobby_id);
		RS_SERIAL_PROCESS(lobby_name);
		RS_SERIAL_PROCESS(lobby_topic);
		RS_SERIAL_PROCESS(participating_friends);
		RS_SERIAL_PROCESS(gxs_id);

		RS_SERIAL_PROCESS(lobby_flags);
		RS_SERIAL_PROCESS(gxs_ids);
		RS_SERIAL_PROCESS(last_activity);
	}
};


class RsMsgs;
/**
 * @brief Pointer to retroshare's message service
 * @jsonapi{development}
 */
extern RsMsgs* rsMsgs;

class RsMsgs 
{
public:

	/**
	 * @brief getMessageSummaries
	 * @jsonapi{development}
	 * @param[out] msgList
	 * @return always true
	 */
	virtual bool getMessageSummaries(std::list<Rs::Msgs::MsgInfoSummary> &msgList) = 0;

	/**
	 * @brief getMessage
	 * @jsonapi{development}
	 * @param[in] msgId message ID to lookup
	 * @param[out] msg
	 * @return true on success
	 */
	virtual bool getMessage(const std::string &msgId, Rs::Msgs::MessageInfo &msg)  = 0;

	/**
	 * @brief sendMail
	 * @jsonapi{development}
	 * @param[in] from GXS id of the author
	 * @param[in] subject Mail subject
	 * @param[in] mailBody Mail body
	 * @param[in] to list of To: recipients
	 * @param[in] cc list of CC: recipients
	 * @param[in] bcc list of BCC: recipients
	 * @param[in] attachments list of suggested files
	 * @param[out] trackingIds storage for tracking ids for each sent mail
	 * @param[out] errorMsg error message if errors occurred, empty otherwise
	 * @return number of successfully sent mails
	 */
	virtual uint32_t sendMail(
	        const RsGxsId from,
	        const std::string& subject,
	        const std::string& mailBody,
	        const std::set<RsGxsId>& to = std::set<RsGxsId>(),
	        const std::set<RsGxsId>& cc = std::set<RsGxsId>(),
	        const std::set<RsGxsId>& bcc = std::set<RsGxsId>(),
	        const std::vector<FileInfo>& attachments = std::vector<FileInfo>(),
	        std::set<RsMailIdRecipientIdPair>& trackingIds =
	            RS_DEFAULT_STORAGE_PARAM(std::set<RsMailIdRecipientIdPair>),
	        std::string& errorMsg =
	            RS_DEFAULT_STORAGE_PARAM(std::string) ) = 0;

	/**
	 * @brief getMessageCount
	 * @jsonapi{development}
	 * @param[out] nInbox
	 * @param[out] nInboxNew
	 * @param[out] nOutbox
	 * @param[out] nDraftbox
	 * @param[out] nSentbox
	 * @param[out] nTrashbox
	 */
	virtual void getMessageCount(uint32_t &nInbox, uint32_t &nInboxNew, uint32_t &nOutbox, uint32_t &nDraftbox, uint32_t &nSentbox, uint32_t &nTrashbox) = 0;

	/**
	 * @brief SystemMessage
	 * @jsonapi{development}
	 * @param[in] title
	 * @param[in] message
	 * @param[in] systemFlag
	 * @return true on success
	 */
	virtual bool SystemMessage(const std::string &title, const std::string &message, uint32_t systemFlag) = 0;

	/**
	 * @brief MessageToDraft
	 * @jsonapi{development}
	 * @param[in] info
	 * @param[in] msgParentId
	 * @return true on success
	 */
	virtual bool MessageToDraft(Rs::Msgs::MessageInfo &info, const std::string &msgParentId) = 0;

	/**
	 * @brief MessageToTrash
	 * @jsonapi{development}
	 * @param[in] msgId        Id of the message to mode to trash box
	 * @param[in] bTrash       Move to trash if true, otherwise remove from trash
	 * @return true on success
	 */
	virtual bool MessageToTrash(const std::string &msgId, bool bTrash)   = 0;

	/**
	 * @brief getMsgParentId
	 * @jsonapi{development}
	 * @param[in] msgId
	 * @param[out] msgParentId
	 * @return true on success
	 */
	virtual bool getMsgParentId(const std::string &msgId, std::string &msgParentId) = 0;

	/**
	 * @brief MessageDelete
	 * @jsonapi{development}
	 * @param[in] msgId
	 * @return true on success
	 */
	virtual bool MessageDelete(const std::string &msgId)                 = 0;

	/**
	 * @brief MessageRead
	 * @jsonapi{development}
	 * @param[in] msgId
	 * @param[in] unreadByUser
	 * @return true on success
	 */
	virtual bool MessageRead(const std::string &msgId, bool unreadByUser) = 0;

	/**
	 * @brief MessageReplied
	 * @jsonapi{development}
	 * @param[in] msgId
	 * @param[in] replied
	 * @return true on success
	 */
	virtual bool MessageReplied(const std::string &msgId, bool replied) = 0;

	/**
	 * @brief MessageForwarded
	 * @jsonapi{development}
	 * @param[in] msgId
	 * @param[in] forwarded
	 * @return true on success
	 */
	virtual bool MessageForwarded(const std::string &msgId, bool forwarded) = 0;

	/**
	 * @brief MessageStar
	 * @jsonapi{development}
	 * @param[in] msgId
	 * @param[in] mark
	 * @return true on success
	 */
	virtual bool MessageStar(const std::string &msgId, bool mark) = 0;

	/**
	 * @brief MessageJunk
	 * @jsonapi{development}
	 * @param[in] msgId
	 * @param[in] mark
	 * @return true on success
	 */
	virtual bool MessageJunk(const std::string &msgId, bool mark) = 0;

	/**
	 * @brief MessageLoadEmbeddedImages
	 * @jsonapi{development}
	 * @param[in] msgId
	 * @param[in] load
	 * @return true on success
	 */
	virtual bool MessageLoadEmbeddedImages(const std::string &msgId, bool load) = 0;

	/* message tagging */
	/**
	 * @brief getMessageTagTypes
	 * @jsonapi{development}
	 * @param[out] tags
	 * @return always true
	 */
	virtual bool getMessageTagTypes(Rs::Msgs::MsgTagType& tags) = 0;

	/**
	 * @brief setMessageTagType
	 * @jsonapi{development}
	 * @param[in] tagId
	 * @param[in] text
	 * @param[in] rgb_color
	 * @return true on success
	 */
	virtual bool setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color) = 0;

	/**
	 * @brief removeMessageTagType
	 * @jsonapi{development}
	 * @param[in] tagId
	 * @return true on success
	 */
	virtual bool removeMessageTagType(uint32_t tagId) = 0;

	/**
	 * @brief getMessageTag
	 * @jsonapi{development}
	 * @param[in] msgId
	 * @param[out] info
	 * @return true on success
	 */
	virtual bool getMessageTag(const std::string &msgId, Rs::Msgs::MsgTagInfo& info) = 0;

	/**
	 * @brief setMessageTag
	 * set == false && tagId == 0 --> remove all
	 * @jsonapi{development}
	 * @param[in] msgId
	 * @param[in] tagId
	 * @param[in] set
	 * @return true on success
	 */
	virtual bool setMessageTag(const std::string &msgId, uint32_t tagId, bool set) = 0;

	/**
	 * @brief resetMessageStandardTagTypes
	 * @jsonapi{development}
	 * @param[out] tags
	 * @return always true
	 */
	virtual bool resetMessageStandardTagTypes(Rs::Msgs::MsgTagType& tags) = 0;

	/****************************************/
	/*        Private distant messages      */
	/****************************************/

    virtual uint32_t getDistantMessagingPermissionFlags()=0 ;
    virtual void setDistantMessagingPermissionFlags(uint32_t flags)=0 ;
    
	/****************************************/
	/*                 Chat                 */
	/****************************************/
	// sendChat for broadcast, private, lobby and private distant chat
	// note: for lobby chat, you first have to subscribe to a lobby
	//       for private distant chat, it is reqired to have an active distant chat session

	/**
	 * @brief sendChat send a chat message to a given id
	 * @jsonapi{development}
	 * @param[in] id id to send the message
	 * @param[in] msg message to send
	 * @return true on success
	 */
	virtual bool sendChat(ChatId id, std::string msg) = 0;

	/**
	 * @brief getMaxMessageSecuritySize get the maximum size of a chta message
	 * @jsonapi{development}
	 * @param[in] type chat type
	 * @return maximum size or zero for infinite
	 */
	virtual uint32_t getMaxMessageSecuritySize(int type) = 0;

	/**
	 * @brief sendStatusString send a status string
	 * @jsonapi{development}
	 * @param[in] id chat id to send the status string to
	 * @param[in] status_string status string
	 */
	virtual void sendStatusString(const ChatId &id, const std::string &status_string) = 0;

	/**
	 * @brief clearChatLobby clear a chat lobby
	 * @jsonapi{development}
	 * @param[in] id chat lobby id to clear
	 */
	virtual void clearChatLobby(const ChatId &id) = 0;

	/**
	 * @brief setCustomStateString set your custom status message
	 * @jsonapi{development}
	 * @param[in] status_string status message
	 */
	virtual void setCustomStateString(const std::string &status_string) = 0;

	/**
	 * @brief getCustomStateString get your custom status message
	 * @return status message
	 */
	virtual std::string getCustomStateString() = 0;

	/**
	 * @brief getCustomStateString get the custom status message from a peer
	 * @jsonapi{development}
	 * @param[in] peer_id peer id to the peer you want to get the status message from
	 * @return status message
	 */
	virtual std::string getCustomStateString(const RsPeerId &peer_id) = 0;

// get avatar data for peer pid
virtual void getAvatarData(const RsPeerId& pid,unsigned char *& data,int& size) = 0 ;
// set own avatar data 
virtual void setOwnAvatarData(const unsigned char *data,int size) = 0 ;
virtual void getOwnAvatarData(unsigned char *& data,int& size) = 0 ;

	/****************************************/
	/*            Chat lobbies              */
	/****************************************/
	/**
	 * @brief joinVisibleChatLobby join a lobby that is visible
	 * @jsonapi{development}
	 * @param[in] lobby_id lobby to join to
	 * @param[in] own_id chat id to use
	 * @return true on success
	 */
	virtual bool joinVisibleChatLobby(const ChatLobbyId &lobby_id, const RsGxsId &own_id) = 0 ;

	/**
	 * @brief getChatLobbyList get ids of subscribed lobbies
	 * @jsonapi{development}
	 * @param[out] cl_list lobby list
	 */
	virtual void getChatLobbyList(std::list<ChatLobbyId> &cl_list) = 0;

	/**
	 * @brief getChatLobbyInfo get lobby info of a subscribed chat lobby. Returns true if lobby id is valid.
	 * @jsonapi{development}
	 * @param[in] id id to get infos from
	 * @param[out] info lobby infos
	 * @return true on success
	 */
	virtual bool getChatLobbyInfo(const ChatLobbyId &id, ChatLobbyInfo &info) = 0 ;

	/**
	 * @brief getListOfNearbyChatLobbies get info about all lobbies, subscribed and unsubscribed
	 * @jsonapi{development}
	 * @param[out] public_lobbies list of all visible lobbies
	 */
	virtual void getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord> &public_lobbies) = 0 ;

	/**
	 * @brief invitePeerToLobby invite a peer to join a lobby
	 * @jsonapi{development}
	 * @param[in] lobby_id lobby it to invite into
	 * @param[in] peer_id peer to invite
	 */
	virtual void invitePeerToLobby(const ChatLobbyId &lobby_id, const RsPeerId &peer_id) = 0;

	/**
	 * @brief acceptLobbyInvite accept a chat invite
	 * @jsonapi{development}
	 * @param[in] id chat lobby id you were invited into and you want to join
	 * @param[in] identity chat identity to use
	 * @return true on success
	 */
	virtual bool acceptLobbyInvite(const ChatLobbyId &id, const RsGxsId &identity) = 0 ;

	/**
	 * @brief denyLobbyInvite deny a chat lobby invite
	 * @jsonapi{development}
	 * @param[in] id chat lobby id you were invited into
     * @return true on success
     */
    virtual bool denyLobbyInvite(const ChatLobbyId &id) = 0 ;

	/**
	 * @brief getPendingChatLobbyInvites get a list of all pending chat lobby invites
	 * @jsonapi{development}
	 * @param[out] invites list of all pending chat lobby invites
	 */
	virtual void getPendingChatLobbyInvites(std::list<ChatLobbyInvite> &invites) = 0;

	/**
	 * @brief unsubscribeChatLobby leave a chat lobby
	 * @jsonapi{development}
	 * @param[in] lobby_id lobby to leave
	 */
	virtual void unsubscribeChatLobby(const ChatLobbyId &lobby_id) = 0;

	/**
	 * @brief sendLobbyStatusPeerLeaving notify friend nodes that we're leaving a subscribed lobby
	 * @jsonapi{development}
	 * @param[in] lobby_id lobby to leave
	 */
	virtual void sendLobbyStatusPeerLeaving(const ChatLobbyId& lobby_id) = 0;

	/**
	 * @brief setIdentityForChatLobby set the chat identit
	 * @jsonapi{development}
	 * @param[in] lobby_id lobby to change the chat idnetity for
	 * @param[in] nick new chat identity
	 * @return true on success
	 */
	virtual bool setIdentityForChatLobby(const ChatLobbyId &lobby_id, const RsGxsId &nick) = 0;

	/**
	 * @brief getIdentityForChatLobby
	 * @jsonapi{development}
	 * @param[in] lobby_id	lobby to get the chat id from
	 * @param[out] nick	chat identity
	 * @return true on success
	 */
	virtual bool getIdentityForChatLobby(const ChatLobbyId &lobby_id, RsGxsId &nick) = 0 ;

	/**
	 * @brief setDefaultIdentityForChatLobby set the default identity used for chat lobbies
	 * @jsonapi{development}
	 * @param[in] nick chat identitiy to use
	 * @return true on success
	 */
	virtual bool setDefaultIdentityForChatLobby(const RsGxsId &nick) = 0;

	/**
	 * @brief getDefaultIdentityForChatLobby get the default identity used for chat lobbies
	 * @jsonapi{development}
	 * @param[out] id chat identitiy to use
	 */
	virtual void getDefaultIdentityForChatLobby(RsGxsId &id) = 0 ;

	/**
	 * @brief setLobbyAutoSubscribe enable or disable auto subscribe for a chat lobby
	 * @jsonapi{development}
	 * @param[in] lobby_id lobby to auto (un)subscribe
	 * @param[in] autoSubscribe set value for auto subscribe
	 */
	virtual void setLobbyAutoSubscribe(const ChatLobbyId &lobby_id, const bool autoSubscribe) = 0 ;

	/**
	 * @brief getLobbyAutoSubscribe get current value of auto subscribe
	 * @jsonapi{development}
	 * @param[in] lobby_id lobby to get value from
	 * @return wether lobby has auto subscribe enabled or disabled
	 */
	virtual bool getLobbyAutoSubscribe(const ChatLobbyId &lobby_id) = 0 ;

	/**
	 * @brief createChatLobby create a new chat lobby
	 * @jsonapi{development}
	 * @param[in] lobby_name lobby name
	 * @param[in] lobby_identity chat id to use for new lobby
	 * @param[in] lobby_topic lobby toppic
	 * @param[in] invited_friends list of friends to invite
	 * @param[in] lobby_privacy_type flag for new chat lobby
	 * @return chat id of new lobby
	 */
	virtual ChatLobbyId createChatLobby(const std::string &lobby_name, const RsGxsId &lobby_identity, const std::string &lobby_topic, const std::set<RsPeerId> &invited_friends, ChatLobbyFlags lobby_privacy_type) = 0 ;

/****************************************/
/*            Distant chat              */
/****************************************/

    virtual uint32_t getDistantChatPermissionFlags()=0 ;
    virtual bool setDistantChatPermissionFlags(uint32_t flags)=0 ;
	
    	/**
	 * @brief initiateDistantChatConnexion initiate a connexion for a distant chat
	 * @jsonapi{development}
	 * @param[in] to_pid RsGxsId to start the connection
	 * @param[in] from_pid owned RsGxsId who start the connection
	 * @param[out] pid distant chat id
	 * @param[out] error_code if the connection can't be stablished
	 * @param[in] notify notify remote that the connection is stablished
	 * @return true on success. If you try to initate a connection already started it will return the pid of it. 
	 */
	virtual bool initiateDistantChatConnexion(
	        const RsGxsId& to_pid, const RsGxsId& from_pid,
	        DistantChatPeerId& pid, uint32_t& error_code,
	        bool notify = true ) = 0;

	/**
	 * @brief getDistantChatStatus receives distant chat info to a given distant chat id
	 * @jsonapi{development}
	 * @param[in] pid distant chat id
	 * @param[out] info distant chat info
	 * @return true on success
	 */
	virtual bool getDistantChatStatus(const DistantChatPeerId& pid, DistantChatPeerInfo& info)=0;
	
	/**
	 * @brief closeDistantChatConnexion 
	 * @jsonapi{development}
	 * @param[in] pid distant chat id to close the connection
	 * @return true on success
	 */
	virtual bool closeDistantChatConnexion(const DistantChatPeerId& pid)=0;

	/**
	 * @brief MessageSend
	 * @jsonapi{development}
	 * @param[in] info
	 * @return always true
	 */
	RS_DEPRECATED_FOR(sendMail)
	virtual bool MessageSend(Rs::Msgs::MessageInfo &info) = 0;

	virtual ~RsMsgs();
};
