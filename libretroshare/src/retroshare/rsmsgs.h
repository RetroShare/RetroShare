#ifndef RS_MSG_GUI_INTERFACE_H
#define RS_MSG_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsmsgs.h
 *
 * RetroShare C++ Interface.
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


#include <list>
#include <iostream>
#include <string>
#include <set>
#include <assert.h>

#include "rstypes.h"
#include "rsgxsifacetypes.h"

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
typedef std::string 	ChatLobbyNickName ;

typedef uint64_t     MessageId ;


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

class MessageInfo_v2
{
	public:
		//MessageInfo_v2() {}

		unsigned int msgflags;

		//RsMessageId msgId;
		MsgAddress from ;

		std::list<MsgAddress> rcpt ;

		// Headers
		//
		std::string subject;
		std::string msg;
		time_t time_stamp ;

		//std::list<MessageHeader> headers ;

		std::string attach_title;
		std::string attach_comment;
		std::list<FileInfo> files;

		int size;  /* total of files */
		int count; /* file count     */
};

class MessageInfo
{
public:
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
};


class MsgInfoSummary 
{
	public:
    MsgInfoSummary(): msgflags(0), count(0), ts(0) {}

	std::string msgId;
	RsPeerId srcId;

	uint32_t msgflags;

	std::string title;
	int count; /* file count     */
	time_t ts;

};

class MsgTagInfo
{
public:
	MsgTagInfo() {}

	std::string msgId;
	std::list<uint32_t> tagIds;
};

class MsgTagType
{
public:
	MsgTagType() {}

	/* map containing tagId -> pair (text, rgb color) */
	std::map<uint32_t, std::pair<std::string, uint32_t> > types;

};

} //namespace Rs
} //namespace Msgs


#define RS_CHAT_PUBLIC 			0x0001
#define RS_CHAT_PRIVATE 		0x0002
#define RS_CHAT_AVATAR_AVAILABLE 	0x0004

#define RS_DISTANT_CHAT_STATUS_UNKNOWN			0x0000
#define RS_DISTANT_CHAT_STATUS_TUNNEL_DN   		0x0001
#define RS_DISTANT_CHAT_STATUS_CAN_TALK		0x0002
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

struct DistantChatPeerInfo
{
	RsGxsId to_id ;
	RsGxsId own_id ;
	DistantChatPeerId peer_id ;	// this is the tunnel id actually
	uint32_t status ;		// see the values in rsmsgs.h
};

// Identifier for an chat endpoint like
// neighbour peer, distant peer, chatlobby, broadcast
class ChatId
{
public:
    ChatId();
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
    RsGxsId     toGxsId()   const;
    ChatLobbyId toLobbyId() const;
    DistantChatPeerId toDistantChatId() const;

    // for the very specific case of transfering a status string
    // from the chatservice to the gui,
    // this defines from which peer the status string came from
    RsPeerId broadcast_status_peer_id;
private:
    enum Type { TYPE_NOT_SET,
                TYPE_PRIVATE,            // private chat with directly connected friend, peer_id is valid
                TYPE_PRIVATE_DISTANT,    // private chat with distant peer, gxs_id is valid
                TYPE_LOBBY,              // chat lobby id, lobby_id is valid
                TYPE_BROADCAST           // message to/from all connected peers
              };

    Type type;
    RsPeerId peer_id;
    DistantChatPeerId distant_chat_id;
    ChatLobbyId lobby_id;
};

class ChatMessage
{
public:
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
};

class ChatLobbyInvite
{
	public:
		ChatLobbyId lobby_id ;
		RsPeerId peer_id ;
		std::string lobby_name ;
		std::string lobby_topic ;
        ChatLobbyFlags lobby_flags ;
};

class VisibleChatLobbyRecord
{
public:
    VisibleChatLobbyRecord(): lobby_id(0), total_number_of_peers(0), last_report_time(0){}

    ChatLobbyId lobby_id ;									// unique id of the lobby
    std::string lobby_name ;								// name to use for this lobby
    std::string lobby_topic ;								// topic to use for this lobby
    std::set<RsPeerId> participating_friends ;	// list of direct friend who participate.

    uint32_t total_number_of_peers ;						// total number of particpating peers. Might not be
    time_t last_report_time ; 								// last time the lobby was reported.
    ChatLobbyFlags lobby_flags ;									// see RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC / RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE
};


class ChatLobbyInfo
{
	public:
        ChatLobbyId lobby_id ;						// unique id of the lobby
        std::string lobby_name ;					// name to use for this lobby
        std::string lobby_topic ;					// topic to use for this lobby
        std::set<RsPeerId> participating_friends ;			// list of direct friend who participate. Used to broadcast sent messages.
        RsGxsId gxs_id ;						// ID to sign messages

        ChatLobbyFlags lobby_flags ;					// see RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC / RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE
        std::map<RsGxsId,time_t> gxs_ids ;				// list of non direct friend who participate. Used to display only.
        time_t last_activity ;						// last recorded activity. Useful for removing dead lobbies.
};

std::ostream &operator<<(std::ostream &out, const Rs::Msgs::MessageInfo &info);

class RsMsgs;
extern RsMsgs   *rsMsgs;

class RsMsgs 
{
	public:

	RsMsgs() { return; }
virtual ~RsMsgs() { return; }

/****************************************/
/*             Message Items            */
/****************************************/

virtual bool getMessageSummaries(std::list<Rs::Msgs::MsgInfoSummary> &msgList) = 0;
virtual bool getMessage(const std::string &mId, Rs::Msgs::MessageInfo &msg)  = 0;
virtual void getMessageCount(unsigned int *pnInbox, unsigned int *pnInboxNew, unsigned int *pnOutbox, unsigned int *pnDraftbox, unsigned int *pnSentbox, unsigned int *pnTrashbox) = 0;

virtual bool MessageSend(Rs::Msgs::MessageInfo &info) = 0;
virtual bool SystemMessage(const std::string &title, const std::string &message, uint32_t systemFlag) = 0;
virtual bool MessageToDraft(Rs::Msgs::MessageInfo &info, const std::string &msgParentId) = 0;
virtual bool MessageToTrash(const std::string &mid, bool bTrash)   = 0;
virtual bool getMsgParentId(const std::string &msgId, std::string &msgParentId) = 0;

virtual bool MessageDelete(const std::string &mid)                 = 0;
virtual bool MessageRead(const std::string &mid, bool unreadByUser) = 0;
virtual bool MessageReplied(const std::string &mid, bool replied) = 0;
virtual bool MessageForwarded(const std::string &mid, bool forwarded) = 0;
virtual bool MessageStar(const std::string &mid, bool mark) = 0;
virtual bool MessageLoadEmbeddedImages(const std::string &mid, bool load) = 0;

/* message tagging */

virtual bool getMessageTagTypes(Rs::Msgs::MsgTagType& tags) = 0;
/* set == false && tagId == 0 --> remove all */
virtual bool setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color) = 0;
virtual bool removeMessageTagType(uint32_t tagId) = 0;

virtual bool getMessageTag(const std::string &msgId, Rs::Msgs::MsgTagInfo& info) = 0;
virtual bool setMessageTag(const std::string &msgId, uint32_t tagId, bool set) = 0;

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
virtual	bool     sendChat(ChatId id, std::string msg) = 0;
virtual uint32_t getMaxMessageSecuritySize(int type)  = 0;

virtual void   sendStatusString(const ChatId& id,const std::string& status_string) = 0 ;

virtual void   setCustomStateString(const std::string&  status_string) = 0 ;
virtual std::string getCustomStateString() = 0 ;
virtual std::string getCustomStateString(const RsPeerId& peer_id) = 0 ;

// get avatar data for peer pid
virtual void getAvatarData(const RsPeerId& pid,unsigned char *& data,int& size) = 0 ;
// set own avatar data 
virtual void setOwnAvatarData(const unsigned char *data,int size) = 0 ;
virtual void getOwnAvatarData(unsigned char *& data,int& size) = 0 ;

/****************************************/
/*            Chat lobbies              */
/****************************************/

virtual bool joinVisibleChatLobby(const ChatLobbyId& lobby_id,const RsGxsId& own_id) = 0 ;
/// get ids of subscribed lobbies
virtual void getChatLobbyList(std::list<ChatLobbyId>& cl_list) = 0;
/// get lobby info of a subscribed chat lobby. Returns true if lobby id is valid.
virtual bool getChatLobbyInfo(const ChatLobbyId& id,ChatLobbyInfo& info) = 0 ;
/// get info about all lobbies, subscribed and unsubscribed
virtual void getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord>& public_lobbies) = 0 ;
virtual void invitePeerToLobby(const ChatLobbyId& lobby_id,const RsPeerId& peer_id) = 0;
virtual bool acceptLobbyInvite(const ChatLobbyId& id,const RsGxsId& identity) = 0 ;
virtual void denyLobbyInvite(const ChatLobbyId& id) = 0 ;
virtual void getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites) = 0;
virtual void unsubscribeChatLobby(const ChatLobbyId& lobby_id) = 0;
virtual bool setIdentityForChatLobby(const ChatLobbyId& lobby_id,const RsGxsId& nick) = 0;
virtual bool getIdentityForChatLobby(const ChatLobbyId& lobby_id,RsGxsId& nick) = 0 ;
virtual bool setDefaultIdentityForChatLobby(const RsGxsId& nick) = 0;
virtual void getDefaultIdentityForChatLobby(RsGxsId& id) = 0 ;
    virtual void setLobbyAutoSubscribe(const ChatLobbyId& lobby_id, const bool autoSubscribe) = 0 ;
    virtual bool getLobbyAutoSubscribe(const ChatLobbyId& lobby_id) = 0 ;
virtual ChatLobbyId createChatLobby(const std::string& lobby_name,const RsGxsId& lobby_identity,const std::string& lobby_topic,const std::set<RsPeerId>& invited_friends,ChatLobbyFlags lobby_privacy_type) = 0 ;

/****************************************/
/*            Distant chat              */
/****************************************/

    virtual uint32_t getDistantChatPermissionFlags()=0 ;
    virtual bool setDistantChatPermissionFlags(uint32_t flags)=0 ;
    
virtual bool initiateDistantChatConnexion(const RsGxsId& to_pid,const RsGxsId& from_pid,DistantChatPeerId& pid,uint32_t& error_code) = 0;
virtual bool getDistantChatStatus(const DistantChatPeerId& pid,DistantChatPeerInfo& info)=0;
virtual bool closeDistantChatConnexion(const DistantChatPeerId& pid)=0;

};


#endif

