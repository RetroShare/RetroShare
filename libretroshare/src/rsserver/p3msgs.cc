
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
#include "chat/p3chatservice.h"

#include "pqi/authgpg.h"

/* external reference point */
RsMsgs *rsMsgs = NULL;

/****************************************/
/****************************************/

ChatId::ChatId():
    type(TYPE_NOT_SET),
    lobby_id(0)
{

}

ChatId::ChatId(RsPeerId id):
    lobby_id(0)
{
    type = TYPE_PRIVATE;
    peer_id = id;
}

ChatId::ChatId(RsGxsId id):
    lobby_id(0)
{
    type = TYPE_PRIVATE_DISTANT;
    gxs_id = id;
}

ChatId::ChatId(ChatLobbyId id):
    lobby_id(0)
{
    type = TYPE_LOBBY;
    lobby_id = id;
}

ChatId::ChatId(std::string str):
    lobby_id(0)
{
    type = TYPE_NOT_SET;
    if(str.empty())
        return;
    if(str[0] == 'P')
    {
        type = TYPE_PRIVATE;
        peer_id = RsPeerId(str.substr(1));
    }
    else if(str[0] == 'D')
    {
        type = TYPE_PRIVATE_DISTANT;
        gxs_id == GXSId(str.substr(1));
    }
    else if(str[0] == 'L')
    {
        if(sizeof(ChatLobbyId) != 8)
        {
            std::cerr << "ChatId::ChatId(std::string) Error: sizeof(ChatLobbyId) != 8. please report this" << std::endl;
            return;
        }
        str = str.substr(1);
        if(str.size() != 16)
            return;
        ChatLobbyId id = 0;
        for(int i = 0; i<16; i++)
        {
            uint8_t c = str[i];
            if(c <= '9')
                c -= '9';
            else
                c -= 'A';
            id = id << 4;
            id |= c;
        }
        type = TYPE_LOBBY;
        lobby_id = id;
    }
    else if(str[0] == 'B')
    {
        type = TYPE_BROADCAST;
    }
}

ChatId ChatId::makeBroadcastId()
{
    ChatId id;
    id.type = TYPE_BROADCAST;
    return id;
}

std::string ChatId::toStdString() const
{
    std::string str;
    if(type == TYPE_PRIVATE)
    {
        str += "P";
        str += peer_id.toStdString();
    }
    else if(type == TYPE_PRIVATE_DISTANT)
    {
        str += "D";
        str += gxs_id.toStdString();
    }
    else if(type == TYPE_LOBBY)
    {
        if(sizeof(ChatLobbyId) != 8)
        {
            std::cerr << "ChatId::toStdString() Error: sizeof(ChatLobbyId) != 8. please report this" << std::endl;
            return "";
        }
        ChatLobbyId id = lobby_id;
        for(int i = 0; i<16; i++)
        {
            uint8_t c = id >>(64-4);
            if(c > 9)
                c += 'A';
            else
                c += '1';
            str += c;
            id = id << 4;
        }
    }
    else if(type == TYPE_BROADCAST)
    {
        str += "B";
    }
    return str;
}

bool ChatId::operator <(const ChatId& other) const
{
    if(type != other.type)
        return type < other.type;
    else
    {
        switch(type)
        {
        case TYPE_NOT_SET:
            return false;
        case TYPE_PRIVATE:
            return peer_id < other.peer_id;
        case TYPE_PRIVATE_DISTANT:
            return gxs_id < other.gxs_id;
        case TYPE_LOBBY:
            return lobby_id < other.lobby_id;
        case TYPE_BROADCAST:
            return false;
        default:
            return false;
        }
    }
}

bool ChatId::isSameEndpoint(const ChatId &other) const
{
    if(type != other.type)
        return false;
    else
    {
        switch(type)
        {
        case TYPE_NOT_SET:
            return false;
        case TYPE_PRIVATE:
            return peer_id == other.peer_id;
        case TYPE_PRIVATE_DISTANT:
            return gxs_id == other.gxs_id;
        case TYPE_LOBBY:
            return lobby_id == other.lobby_id;
        case TYPE_BROADCAST:
            return true;
        default:
            return false;
        }
    }
}

bool ChatId::isNotSet() const
{
    return type == TYPE_NOT_SET;
}
bool ChatId::isPeerId() const
{
    return type == TYPE_PRIVATE;
}
bool ChatId::isGxsId()  const
{
    return type == TYPE_PRIVATE_DISTANT;
}
bool ChatId::isLobbyId() const
{
    return type == TYPE_LOBBY;
}
bool ChatId::isBroadcast() const
{
    return type == TYPE_BROADCAST;
}
RsPeerId    ChatId::toPeerId()  const
{
    if(type == TYPE_PRIVATE)
        return peer_id;
    else
    {
        std::cerr << "ChatId Warning: conversation to RsPeerId requested, but type is different." << std::endl;
        return RsPeerId();
    }
}
RsGxsId     ChatId::toGxsId()   const
{
    if(type == TYPE_PRIVATE_DISTANT)
        return gxs_id;
    else
    {
        std::cerr << "ChatId Warning: conversation to RsGxsId requested, but type is different." << std::endl;
        return RsGxsId();
    }
}
ChatLobbyId ChatId::toLobbyId() const
{
    if(type == TYPE_LOBBY)
        return lobby_id;
    else
    {
        std::cerr << "ChatId Warning: conversation to ChatLobbyId requested, but type is different." << std::endl;
        return 0;
    }
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
void p3Msgs::enableDistantMessaging(bool b)
{
	mMsgSrv->enableDistantMessaging(b);
}
bool p3Msgs::distantMessagingEnabled()
{
	return mMsgSrv->distantMessagingEnabled();
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

bool p3Msgs::MessageLoadEmbeddedImages(const std::string &mid, bool load)
{
	return mMsgSrv->setMsgFlag(mid, load ? RS_MSG_FLAGS_LOAD_EMBEDDED_IMAGES : 0, RS_MSG_FLAGS_LOAD_EMBEDDED_IMAGES);
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
bool p3Msgs::sendChat(ChatId destination, std::string msg)
{
    return mChatSrv->sendChat(destination, msg);
}

uint32_t p3Msgs::getMaxMessageSecuritySize(int type)
{
	return mChatSrv->getMaxMessageSecuritySize(type);
}

void p3Msgs::sendStatusString(const ChatId& peer_id, const std::string& status_string)
{
    mChatSrv->sendStatusString(peer_id, status_string);
}

void p3Msgs::getOwnAvatarData(unsigned char *& data,int& size)
{
	mChatSrv->getOwnAvatarJpegData(data,size) ;
}

void p3Msgs::setOwnAvatarData(const unsigned char *data,int size)
{
	mChatSrv->setOwnAvatarJpegData(data,size) ;
}

void p3Msgs::getAvatarData(const RsPeerId& pid,unsigned char *& data,int& size)
{
	mChatSrv->getAvatarJpegData(pid,data,size) ;
}

std::string p3Msgs::getCustomStateString(const RsPeerId& peer_id)
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

bool p3Msgs::getVirtualPeerId(const ChatLobbyId& id,RsPeerId& peer_id)
{
	return mChatSrv->getVirtualPeerId(id,peer_id) ;
}
bool p3Msgs::isLobbyId(const RsPeerId& peer_id,ChatLobbyId& id)
{
	return mChatSrv->isLobbyId(peer_id,id) ;
}

void p3Msgs::getChatLobbyList(std::list<ChatLobbyInfo>& linfos) 
{
	mChatSrv->getChatLobbyList(linfos) ;
}
void p3Msgs::invitePeerToLobby(const ChatLobbyId& lobby_id, const RsPeerId& peer_id) 
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

ChatLobbyId p3Msgs::createChatLobby(const std::string& lobby_name,const std::string& lobby_topic,const std::list<RsPeerId>& invited_friends,uint32_t privacy_type) 
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
bool p3Msgs::initiateDistantChatConnexion(const RsGxsId& to_gxs_id,const RsGxsId& from_gxs_id,uint32_t& error_code)
{
    return mChatSrv->initiateDistantChatConnexion(to_gxs_id,from_gxs_id,error_code) ;
}
bool p3Msgs::getDistantChatStatus(const RsGxsId &gxs_id,uint32_t &status)
{
    return mChatSrv->getDistantChatStatus(gxs_id,status) ;
}
bool p3Msgs::closeDistantChatConnexion(const RsGxsId& pid)
{
	return mChatSrv->closeDistantChatConnexion(pid) ;
}

