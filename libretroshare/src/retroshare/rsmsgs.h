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

#include "rstypes.h"

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

#define RS_MSG_NEW                   0x0010   /* New */
#define RS_MSG_TRASH                 0x0020   /* Trash */
#define RS_MSG_UNREAD_BY_USER        0x0040   /* Unread by user */
#define RS_MSG_REPLIED               0x0080   /* Message is replied */
#define RS_MSG_FORWARDED             0x0100   /* Message is forwarded */
#define RS_MSG_STAR                  0x0200   /* Message is marked with a star */
// system message
#define RS_MSG_USER_REQUEST          0x0400   /* user request */
#define RS_MSG_FRIEND_RECOMMENDATION 0x0800   /* friend recommendation */
#define RS_MSG_SYSTEM                (RS_MSG_USER_REQUEST | RS_MSG_FRIEND_RECOMMENDATION)
#define RS_MSG_ENCRYPTED             0x1000	 /* message is encrypted */

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

#define RS_CHAT_LOBBY_PRIVACY_LEVEL_CHALLENGE  	0	/* Used to accept connexion challenges only. */
#define RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC  		1	/* lobby is visible by friends. Friends can connect.*/
#define RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE 		2	/* lobby invisible by friends. Peers on invitation only .*/

typedef uint64_t 		ChatLobbyId ;
typedef uint64_t 		ChatLobbyMsgId ;
typedef std::string 	ChatLobbyNickName ;

class MessageInfo 
{
	public:
	MessageInfo() {}
	std::string msgId;
	std::string srcId;

	unsigned int msgflags;

	std::list<std::string> msgto;
	std::list<std::string> msgcc;
	std::list<std::string> msgbcc;

	std::wstring title;
	std::wstring msg;

	std::wstring attach_title;
	std::wstring attach_comment;
	std::list<FileInfo> files;
	std::map<std::string,std::string> encryption_keys ; // for concerned ids only the public pgp key id to encrypt the message with.
	int size;  /* total of files */
	int count; /* file count     */

	int ts;
};

class MsgInfoSummary 
{
	public:
	MsgInfoSummary() {}

	std::string msgId;
	std::string srcId;

	uint32_t msgflags;

	std::wstring title;
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

#define RS_CHAT_PUBLIC 				0x0001
#define RS_CHAT_PRIVATE 			0x0002
#define RS_CHAT_AVATAR_AVAILABLE 	0x0004

#define RS_DISTANT_CHAT_STATUS_UNKNOWN			0x0000
#define RS_DISTANT_CHAT_STATUS_TUNNEL_DN		0x0001
#define RS_DISTANT_CHAT_STATUS_TUNNEL_OK		0x0002
#define RS_DISTANT_CHAT_STATUS_CAN_TALK		0x0003

#define RS_DISTANT_CHAT_ERROR_NO_ERROR            0x0000 
#define RS_DISTANT_CHAT_ERROR_DECRYPTION_FAILED   0x0001 
#define RS_DISTANT_CHAT_ERROR_SIGNATURE_MISMATCH  0x0002 
#define RS_DISTANT_CHAT_ERROR_UNKNOWN_KEY         0x0003 

class ChatInfo
{
	public:
	std::string rsid;
	std::string peer_nickname;
	unsigned int chatflags;
	uint32_t sendTime;
	uint32_t recvTime;
	std::wstring msg;
};

class ChatLobbyInvite
{
	public:
		ChatLobbyId lobby_id ;
		std::string peer_id ;
		std::string lobby_name ;
		std::string lobby_topic ;
		uint32_t lobby_privacy_level ;						
};

class VisibleChatLobbyRecord
{
	public:
		VisibleChatLobbyRecord() { total_number_of_peers = 0 ; }

		ChatLobbyId lobby_id ;									// unique id of the lobby
		std::string lobby_name ;								// name to use for this lobby
		std::string lobby_topic ;								// topic to use for this lobby
		std::set<std::string> participating_friends ;	// list of direct friend who participate. 

		uint32_t total_number_of_peers ;						// total number of particpating peers. Might not be
		time_t last_report_time ; 								// last time the lobby was reported.
		uint32_t lobby_privacy_level ;						// see RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC / RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE
};


class ChatLobbyInfo
{
	public:
		ChatLobbyId lobby_id ;									// unique id of the lobby
		std::string lobby_name ;								// name to use for this lobby
		std::string lobby_topic ;								// topic to use for this lobby
		std::set<std::string> participating_friends ;	// list of direct friend who participate. Used to broadcast sent messages.
		std::string nick_name ;									// nickname to use for this lobby

		uint32_t lobby_privacy_level ;						// see RS_CHAT_LOBBY_PRIVACY_LEVEL_PUBLIC / RS_CHAT_LOBBY_PRIVACY_LEVEL_PRIVATE
		std::map<std::string,time_t> nick_names ;			// list of non direct friend who participate. Used to display only.
		time_t last_activity ;									// last recorded activity. Useful for removing dead lobbies.
};

struct DistantChatInviteInfo
{
	std::string hash ;										// hash to contact the invite and refer to it.
	std::string encrypted_radix64_string ;				// encrypted radix string used to for the chat link
	std::string destination_pgp_id ;						// pgp is of the destination of the chat link
	time_t 		time_of_validity ;   					// time when te invite becomes unusable
};

std::ostream &operator<<(std::ostream &out, const MessageInfo &info);
std::ostream &operator<<(std::ostream &out, const ChatInfo &info);

bool operator==(const ChatInfo&, const ChatInfo&);

class RsMsgs;
extern RsMsgs   *rsMsgs;

struct DistantOfflineMessengingInvite
{
	std::string issuer_pgp_id ;
	std::string hash ;
	time_t time_of_validity ;
};


class RsMsgs 
{
	public:

	RsMsgs() { return; }
virtual ~RsMsgs() { return; }

/****************************************/
/*             Message Items            */
/****************************************/

virtual bool getMessageSummaries(std::list<MsgInfoSummary> &msgList) = 0;
virtual bool getMessage(const std::string &mId, MessageInfo &msg)  = 0;
virtual void getMessageCount(unsigned int *pnInbox, unsigned int *pnInboxNew, unsigned int *pnOutbox, unsigned int *pnDraftbox, unsigned int *pnSentbox, unsigned int *pnTrashbox) = 0;
virtual bool decryptMessage(const std::string& mId) = 0 ;

virtual bool MessageSend(MessageInfo &info) = 0;
virtual bool SystemMessage(const std::wstring &title, const std::wstring &message, uint32_t systemFlag) = 0;
virtual bool MessageToDraft(MessageInfo &info, const std::string &msgParentId) = 0;
virtual bool MessageToTrash(const std::string &mid, bool bTrash)   = 0;
virtual bool getMsgParentId(const std::string &msgId, std::string &msgParentId) = 0;

virtual bool MessageDelete(const std::string &mid)                 = 0;
virtual bool MessageRead(const std::string &mid, bool unreadByUser) = 0;
virtual bool MessageReplied(const std::string &mid, bool replied) = 0;
virtual bool MessageForwarded(const std::string &mid, bool forwarded) = 0;
virtual bool MessageStar(const std::string &mid, bool mark) = 0;

/* message tagging */

virtual bool getMessageTagTypes(MsgTagType& tags) = 0;
/* set == false && tagId == 0 --> remove all */
virtual bool setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color) = 0;
virtual bool removeMessageTagType(uint32_t tagId) = 0;

virtual bool getMessageTag(const std::string &msgId, MsgTagInfo& info) = 0;
virtual bool setMessageTag(const std::string &msgId, uint32_t tagId, bool set) = 0;

virtual bool resetMessageStandardTagTypes(MsgTagType& tags) = 0;

/* private distant messages */

virtual bool createDistantOfflineMessengingInvite(time_t validity_time_stamp, std::string& hash)=0 ;
virtual bool getDistantOfflineMessengingInvites(std::vector<DistantOfflineMessengingInvite>& invites) = 0 ;

/****************************************/
/*                 Chat                 */
/****************************************/
virtual	bool   sendPublicChat(const std::wstring& msg) = 0;
virtual	bool   sendPrivateChat(const std::string& id, const std::wstring& msg) = 0;
virtual int     getPublicChatQueueCount() = 0;
virtual	bool   getPublicChatQueue(std::list<ChatInfo> &chats) = 0;
virtual int     getPrivateChatQueueCount(bool incoming) = 0;
virtual	bool   getPrivateChatQueueIds(bool incoming, std::list<std::string> &ids) = 0;
virtual	bool   getPrivateChatQueue(bool incoming, const std::string& id, std::list<ChatInfo> &chats) = 0;
virtual	bool   clearPrivateChatQueue(bool incoming, const std::string& id) = 0;

virtual void   sendStatusString(const std::string& id,const std::string& status_string) = 0 ;
virtual void   sendGroupChatStatusString(const std::string& status_string) = 0 ;

virtual void   setCustomStateString(const std::string&  status_string) = 0 ;
virtual std::string getCustomStateString() = 0 ;
virtual std::string getCustomStateString(const std::string& peer_id) = 0 ;

// get avatar data for peer pid
virtual void getAvatarData(const std::string& pid,unsigned char *& data,int& size) = 0 ;
// set own avatar data 
virtual void setOwnAvatarData(const unsigned char *data,int size) = 0 ;
virtual void getOwnAvatarData(unsigned char *& data,int& size) = 0 ;

/****************************************/
/*            Chat lobbies              */
/****************************************/

virtual bool joinVisibleChatLobby(const ChatLobbyId& lobby_id) = 0 ;
virtual bool isLobbyId(const std::string& virtual_peer_id,ChatLobbyId& lobby_id) = 0;
virtual bool getVirtualPeerId(const ChatLobbyId& lobby_id,std::string& vpid) = 0;
virtual void getChatLobbyList(std::list<ChatLobbyInfo>& cl_info) = 0;
virtual void getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord>& public_lobbies) = 0 ;
virtual void invitePeerToLobby(const ChatLobbyId& lobby_id,const std::string& peer_id) = 0;
virtual bool acceptLobbyInvite(const ChatLobbyId& id) = 0 ;
virtual void denyLobbyInvite(const ChatLobbyId& id) = 0 ;
virtual void getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites) = 0;
virtual void unsubscribeChatLobby(const ChatLobbyId& lobby_id) = 0;
virtual bool setNickNameForChatLobby(const ChatLobbyId& lobby_id,const std::string& nick) = 0;
virtual bool getNickNameForChatLobby(const ChatLobbyId& lobby_id,std::string& nick) = 0 ;
virtual bool setDefaultNickNameForChatLobby(const std::string& nick) = 0;
virtual bool getDefaultNickNameForChatLobby(std::string& nick) = 0 ;
virtual ChatLobbyId createChatLobby(const std::string& lobby_name,const std::string& lobby_topic,const std::list<std::string>& invited_friends,uint32_t lobby_privacy_type) = 0 ;

/****************************************/
/*            Distant chat              */
/****************************************/

virtual bool createDistantChatInvite(const std::string& pgp_id,time_t time_of_validity,std::string& encrypted_string) = 0 ;
virtual bool getDistantChatInviteList(std::vector<DistantChatInviteInfo>& invites) = 0;
virtual bool initiateDistantChatConnexion(const std::string& encrypted_string,std::string& hash,uint32_t& error_code) = 0;
virtual bool getDistantChatStatus(const std::string& hash,uint32_t& status,std::string& pgp_id) = 0;
virtual bool closeDistantChatConnexion(const std::string& hash) = 0;

};


#endif

