
/*
 * "$Id: p3face-msgs.cc,v 1.7 2007-05-05 16:10:06 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#include <iostream>

#include "util/rsdir.h"
#include "util/rsdebug.h"
const int p3facemsgzone = 11453;

#include <sys/time.h>
#include <time.h>

#include "retroshare/rstypes.h"
#include "rsserver/p3msgs.h"

#include "services/p3msgservice.h"
#include "services/p3chatservice.h"

#include "pqi/authgpg.h"

/* external reference point */
RsMsgs *rsMsgs = NULL;

/****************************************/
/****************************************/

std::ostream &operator<<(std::ostream &out, const ChatInfo &info)
{
	out << "ChatInfo: rsid: " << info.rsid << std::endl;
	out << "chatflags: " << info.chatflags << std::endl;
	out << "sendTime: " << info.sendTime << std::endl;
	out << "recvTime: " << info.recvTime << std::endl;
	std::string message;
	message.assign(info.msg.begin(), info.msg.end());
	out << "msg: " << message;
	return out;
}

bool operator==(const ChatInfo& info1, const ChatInfo& info2)
{
	return info1.rsid == info2.rsid &&
		   info1.chatflags == info2.chatflags &&
		   info1.sendTime == info2.sendTime &&
		   info1.recvTime == info2.recvTime &&
		   info1.msg == info2.msg;

}

bool p3Msgs::getMessageSummaries(std::list<MsgInfoSummary> &msgList)
{
	return mMsgSrv->getMessageSummaries(msgList);
}



bool p3Msgs::getMessage(const std::string &mid, MessageInfo &msg)
{
	return mMsgSrv->getMessage(mid, msg);
}

void p3Msgs::getMessageCount(unsigned int *pnInbox, unsigned int *pnInboxNew, unsigned int *pnOutbox, unsigned int *pnDraftbox, unsigned int *pnSentbox, unsigned int *pnTrashbox)
{
	mMsgSrv->getMessageCount(pnInbox, pnInboxNew, pnOutbox, pnDraftbox, pnSentbox, pnTrashbox);
}

/****************************************/
/****************************************/
	/* Message Items */
bool p3Msgs::MessageSend(MessageInfo &info)
{
	return mMsgSrv->MessageSend(info);
}

bool p3Msgs::decryptMessage(const std::string& mId)
{
	return mMsgSrv->decryptMessage(mId);
}
bool p3Msgs::createDistantOfflineMessengingInvite(time_t ts, std::string& hash) 
{
	return mMsgSrv->createDistantOfflineMessengingInvite(ts,hash) ;
}
bool p3Msgs::getDistantOfflineMessengingInvites(std::vector<DistantOfflineMessengingInvite>& invites)
{
	return mMsgSrv->getDistantOfflineMessengingInvites(invites);
}
void p3Msgs::enableDistantMessaging(bool b)
{
	mMsgSrv->enableDistantMessaging(b);
}
bool p3Msgs::distantMessagingEnabled()
{
	return mMsgSrv->distantMessagingEnabled();
}
bool p3Msgs::getDistantMessageHash(const std::string& pgp_id,std::string& hash)
{
	return mMsgSrv->getDistantMessageHash(pgp_id,hash);
}

bool p3Msgs::SystemMessage(const std::string &title, const std::string &message, uint32_t systemFlag)
{
	return mMsgSrv->SystemMessage(title, message, systemFlag);
}

bool p3Msgs::MessageToDraft(MessageInfo &info, const std::string &msgParentId)
{
	return mMsgSrv->MessageToDraft(info, msgParentId);
}

bool p3Msgs::MessageToTrash(const std::string &mid, bool bTrash)
{
	return mMsgSrv->MessageToTrash(mid, bTrash);
}

bool p3Msgs::getMsgParentId(const std::string &msgId, std::string &msgParentId)
{
	return mMsgSrv->getMsgParentId(msgId, msgParentId);
}

/****************************************/
/****************************************/
bool p3Msgs::MessageDelete(const std::string &mid)
{
	//std::cerr << "p3Msgs::MessageDelete() ";
	//std::cerr << "mid: " << mid << std::endl;

	return mMsgSrv -> removeMsgId(mid);
}

bool p3Msgs::MessageRead(const std::string &mid, bool unreadByUser)
{
	//std::cerr << "p3Msgs::MessageRead() ";
	//std::cerr << "mid: " << mid << std::endl;

	return mMsgSrv -> markMsgIdRead(mid, unreadByUser);
}

bool p3Msgs::MessageReplied(const std::string &mid, bool replied)
{
	return mMsgSrv->setMsgFlag(mid, replied ? RS_MSG_FLAGS_REPLIED : 0, RS_MSG_FLAGS_REPLIED);
}

bool p3Msgs::MessageForwarded(const std::string &mid, bool forwarded)
{
	return mMsgSrv->setMsgFlag(mid, forwarded ? RS_MSG_FLAGS_FORWARDED : 0, RS_MSG_FLAGS_FORWARDED);
}

bool 	p3Msgs::getMessageTagTypes(MsgTagType& tags)
{
	return mMsgSrv->getMessageTagTypes(tags);
}

bool p3Msgs::MessageStar(const std::string &mid, bool star)

{
	return mMsgSrv->setMsgFlag(mid, star ? RS_MSG_FLAGS_STAR : 0, RS_MSG_FLAGS_STAR);
}

bool  p3Msgs::setMessageTagType(uint32_t tagId, std::string& text, uint32_t rgb_color)
{
	return mMsgSrv->setMessageTagType(tagId, text, rgb_color);
}

bool    p3Msgs::removeMessageTagType(uint32_t tagId)
{
	return mMsgSrv->removeMessageTagType(tagId);
}

bool 	p3Msgs::getMessageTag(const std::string &msgId, MsgTagInfo& info)
{
	return mMsgSrv->getMessageTag(msgId, info);
}

bool 	p3Msgs::setMessageTag(const std::string &msgId, uint32_t tagId, bool set)
{
	return mMsgSrv->setMessageTag(msgId, tagId, set);
}

bool    p3Msgs::resetMessageStandardTagTypes(MsgTagType& tags)
{
	return mMsgSrv->resetMessageStandardTagTypes(tags);
}

/****************************************/
/****************************************/
bool 	p3Msgs::sendPublicChat(const std::string& msg)
{
	/* send a message to all for now */
	return mChatSrv -> sendPublicChat(msg);
}

bool 	p3Msgs::sendPrivateChat(const std::string& id, const std::string& msg)
{
	/* send a message to peer */
	return mChatSrv -> sendPrivateChat(id, msg);
}

void p3Msgs::sendGroupChatStatusString(const std::string& status_string) 
{
	mChatSrv->sendGroupChatStatusString(status_string);
}

void p3Msgs::sendStatusString(const std::string& peer_id, const std::string& status_string)
{
	mChatSrv->sendStatusString(peer_id, status_string);
}

int	p3Msgs::getPublicChatQueueCount()
{
	return mChatSrv->getPublicChatQueueCount();
}

bool	p3Msgs::getPublicChatQueue(std::list<ChatInfo> &chats)
{
	return mChatSrv->getPublicChatQueue(chats);
}

int	p3Msgs::getPrivateChatQueueCount(bool incoming)
{
	return mChatSrv->getPrivateChatQueueCount(incoming);
}

bool   p3Msgs::getPrivateChatQueueIds(bool incoming, std::list<std::string> &ids)
{
	return mChatSrv->getPrivateChatQueueIds(incoming, ids);
}

bool	p3Msgs::getPrivateChatQueue(bool incoming, const std::string& id, std::list<ChatInfo> &chats)
{
	return mChatSrv->getPrivateChatQueue(incoming, id, chats);
}

bool	p3Msgs::clearPrivateChatQueue(bool incoming, const std::string& id)
{
	return mChatSrv->clearPrivateChatQueue(incoming, id);
}

void p3Msgs::getOwnAvatarData(unsigned char *& data,int& size)
{
	mChatSrv->getOwnAvatarJpegData(data,size) ;
}

void p3Msgs::setOwnAvatarData(const unsigned char *data,int size)
{
	mChatSrv->setOwnAvatarJpegData(data,size) ;
}

void p3Msgs::getAvatarData(const std::string& pid,unsigned char *& data,int& size)
{
	mChatSrv->getAvatarJpegData(pid,data,size) ;
}

std::string p3Msgs::getCustomStateString(const std::string& peer_id)
{
	return mChatSrv->getCustomStateString(peer_id) ;
}

std::string p3Msgs::getCustomStateString()
{
	return mChatSrv->getOwnCustomStateString() ;
}

void p3Msgs::setCustomStateString(const std::string& state_string)
{
	mChatSrv->setOwnCustomStateString(state_string) ;
}

bool p3Msgs::getVirtualPeerId(const ChatLobbyId& id,std::string& peer_id)
{
	return mChatSrv->getVirtualPeerId(id,peer_id) ;
}
bool p3Msgs::isLobbyId(const std::string& peer_id,ChatLobbyId& id)
{
	return mChatSrv->isLobbyId(peer_id,id) ;
}

void p3Msgs::getChatLobbyList(std::list<ChatLobbyInfo>& linfos) 
{
	mChatSrv->getChatLobbyList(linfos) ;
}
void p3Msgs::invitePeerToLobby(const ChatLobbyId& lobby_id, const std::string& peer_id) 
{
	mChatSrv->invitePeerToLobby(lobby_id,peer_id) ;
}
void p3Msgs::unsubscribeChatLobby(const ChatLobbyId& lobby_id)
{
	mChatSrv->unsubscribeChatLobby(lobby_id) ;
}
bool p3Msgs::setDefaultNickNameForChatLobby(const std::string& nick)
{
	return mChatSrv->setDefaultNickNameForChatLobby(nick) ;
}
bool p3Msgs::getDefaultNickNameForChatLobby(std::string& nick_name)
{
	return mChatSrv->getDefaultNickNameForChatLobby(nick_name) ;
}

bool p3Msgs::setNickNameForChatLobby(const ChatLobbyId& lobby_id,const std::string& nick)
{
	return mChatSrv->setNickNameForChatLobby(lobby_id,nick) ;
}
bool p3Msgs::getNickNameForChatLobby(const ChatLobbyId& lobby_id,std::string& nick_name)
{
	return mChatSrv->getNickNameForChatLobby(lobby_id,nick_name) ;
}

bool p3Msgs::joinVisibleChatLobby(const ChatLobbyId& lobby_id) 
{
	return mChatSrv->joinVisibleChatLobby(lobby_id) ;
}

void p3Msgs::getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord>& public_lobbies) 
{
	mChatSrv->getListOfNearbyChatLobbies(public_lobbies) ;
}

ChatLobbyId p3Msgs::createChatLobby(const std::string& lobby_name,const std::string& lobby_topic,const std::list<std::string>& invited_friends,uint32_t privacy_type) 
{
	return mChatSrv->createChatLobby(lobby_name,lobby_topic,invited_friends,privacy_type) ;
}

void p3Msgs::setLobbyAutoSubscribe(const ChatLobbyId& lobby_id, const bool autoSubscribe)
{
    mChatSrv->setLobbyAutoSubscribe(lobby_id, autoSubscribe);
}

bool p3Msgs::getLobbyAutoSubscribe(const ChatLobbyId& lobby_id)
{
    return mChatSrv->getLobbyAutoSubscribe(lobby_id);
}


bool p3Msgs::acceptLobbyInvite(const ChatLobbyId& id) 
{
	return mChatSrv->acceptLobbyInvite(id) ;
}
void p3Msgs::denyLobbyInvite(const ChatLobbyId& id) 
{
	mChatSrv->denyLobbyInvite(id) ;
}
void p3Msgs::getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites) 
{
	mChatSrv->getPendingChatLobbyInvites(invites) ;
}
bool p3Msgs::createDistantChatInvite(const std::string& pgp_id,time_t time_of_validity,std::string& encrypted_string)
{
	return mChatSrv->createDistantChatInvite(pgp_id,time_of_validity,encrypted_string) ;
}
bool p3Msgs::getDistantChatInviteList(std::vector<DistantChatInviteInfo>& invites)
{
	return mChatSrv->getDistantChatInviteList(invites) ;
}
bool p3Msgs::initiateDistantChatConnexion(const std::string& encrypted_str,time_t validity_time,std::string& hash,uint32_t& error_code)
{
	return mChatSrv->initiateDistantChatConnexion(encrypted_str,validity_time,hash,error_code) ;
}
bool p3Msgs::initiateDistantChatConnexion(const std::string& hash,uint32_t& error_code)
{
	return mChatSrv->initiateDistantChatConnexion(hash,error_code) ;
}
bool p3Msgs::getDistantChatStatus(const std::string& hash,uint32_t& status,std::string& pgp_id) 
{
	return mChatSrv->getDistantChatStatus(hash,status,pgp_id) ;
}
bool p3Msgs::closeDistantChatConnexion(const std::string& hash)
{
	return mChatSrv->closeDistantChatConnexion(hash) ;
}
bool p3Msgs::removeDistantChatInvite(const std::string& hash)
{
	return mChatSrv->removeDistantChatInvite(hash) ;
}

