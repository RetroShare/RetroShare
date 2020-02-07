/*******************************************************************************
 * libretroshare/src/rsserver: p3msgs.cc                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2006  Robert Fernie <retroshare@lunamutt.com>            *
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
#include <iostream>
#include <tuple>

#include "util/rsdir.h"
#include "util/rsdebug.h"
//const int p3facemsgzone = 11453;

#include <sys/time.h>
#include "util/rstime.h"

#include "retroshare/rstypes.h"
#include "rsserver/p3msgs.h"

#include "services/p3msgservice.h"
#include "chat/p3chatservice.h"

#include "pqi/authgpg.h"

using namespace Rs::Msgs;

/*extern*/ RsMsgs* rsMsgs = nullptr;

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

ChatId::ChatId(DistantChatPeerId id):
    lobby_id(0)
{
    type = TYPE_PRIVATE_DISTANT;
    distant_chat_id = id;
}

ChatId::ChatId(ChatLobbyId id):
    lobby_id(0)
{
    type = TYPE_LOBBY;
    lobby_id = id;
}

ChatId::ChatId(std::string str) : lobby_id(0)
{
	type = TYPE_NOT_SET;
	if(str.empty()) return;

    if(str[0] == 'P')
    {
        type = TYPE_PRIVATE;
        peer_id = RsPeerId(str.substr(1));
	}
	else if(str[0] == 'D')
	{
		type = TYPE_PRIVATE_DISTANT;
		distant_chat_id = DistantChatPeerId(str.substr(1));
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
                c -= '0';
            else
                c -= 'A' - 10;
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
        str += distant_chat_id.toStdString();
    }
    else if(type == TYPE_LOBBY)
    {
        if(sizeof(ChatLobbyId) != 8)
        {
            std::cerr << "ChatId::toStdString() Error: sizeof(ChatLobbyId) != 8. please report this" << std::endl;
            return "";
        }
        str += "L";

        ChatLobbyId id = lobby_id;
        for(int i = 0; i<16; i++)
        {
            uint8_t c = id >>(64-4);
            if(c > 9)
                c += 'A' - 10;
            else
                c += '0';
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
            return distant_chat_id < other.distant_chat_id;
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
            return distant_chat_id == other.distant_chat_id;
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
bool ChatId::isDistantChatId()  const
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
        std::cerr << "ChatId Warning: conversation to RsPeerId requested, but type is different. Current value=\"" << toStdString() << "\"" << std::endl;
        return RsPeerId();
    }
}

DistantChatPeerId     ChatId::toDistantChatId()   const
{
    if(type == TYPE_PRIVATE_DISTANT)
        return distant_chat_id;
    else
    {
        std::cerr << "ChatId Warning: conversation to DistantChatPeerId requested, but type is different. Current value=\"" << toStdString() << "\"" << std::endl;
        return DistantChatPeerId();
    }
}
ChatLobbyId ChatId::toLobbyId() const
{
    if(type == TYPE_LOBBY)
        return lobby_id;
    else
    {
        std::cerr << "ChatId Warning: conversation to ChatLobbyId requested, but type is different. Current value=\"" << toStdString() << "\"" << std::endl;
        return 0;
    }
}

bool p3Msgs::getMessageSummaries(std::list<MsgInfoSummary> &msgList)
{
	return mMsgSrv->getMessageSummaries(msgList);
}


uint32_t p3Msgs::getDistantMessagingPermissionFlags()
{
	return mMsgSrv->getDistantMessagingPermissionFlags();
}

void p3Msgs::setDistantMessagingPermissionFlags(uint32_t flags)
{
	return mMsgSrv->setDistantMessagingPermissionFlags(flags);
}


bool p3Msgs::getMessage(const std::string &mid, MessageInfo &msg)
{
	return mMsgSrv->getMessage(mid, msg);
}

void p3Msgs::getMessageCount(uint32_t &nInbox, uint32_t &nInboxNew, uint32_t &nOutbox, uint32_t &nDraftbox, uint32_t &nSentbox, uint32_t &nTrashbox)
{
	mMsgSrv->getMessageCount(nInbox, nInboxNew, nOutbox, nDraftbox, nSentbox, nTrashbox);
}

/****************************************/
/****************************************/
	/* Message Items */
bool p3Msgs::MessageSend(MessageInfo &info)
{
	return mMsgSrv->MessageSend(info);
}

uint32_t p3Msgs::sendMail(
        const RsGxsId from,
        const std::string& subject,
        const std::string& body,
        const std::set<RsGxsId>& to,
        const std::set<RsGxsId>& cc,
        const std::set<RsGxsId>& bcc,
        const std::vector<FileInfo>& attachments,
        std::set<RsMailIdRecipientIdPair>& trackingIds,
        std::string& errorMsg )
{
	return mMsgSrv->sendMail(
	            from, subject, body, to, cc, bcc, attachments,
	            trackingIds, errorMsg );
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

void p3Msgs::sendStatusString(const ChatId& id, const std::string& status_string)
{
    mChatSrv->sendStatusString(id, status_string);
}

void p3Msgs::clearChatLobby(const ChatId &id)
{
	mChatSrv->clearChatLobby(id);
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

bool p3Msgs::getChatLobbyInfo(const ChatLobbyId& id,ChatLobbyInfo& linfo)
{
    return mChatSrv->getChatLobbyInfo(id,linfo) ;
}
void p3Msgs::getChatLobbyList(std::list<ChatLobbyId>& lids)
{
    mChatSrv->getChatLobbyList(lids) ;
}
void p3Msgs::invitePeerToLobby(const ChatLobbyId& lobby_id, const RsPeerId& peer_id) 
{
	mChatSrv->invitePeerToLobby(lobby_id,peer_id) ;
}
void p3Msgs::sendLobbyStatusPeerLeaving(const ChatLobbyId& lobby_id)
{
	mChatSrv->sendLobbyStatusPeerLeaving(lobby_id) ;
}
void p3Msgs::unsubscribeChatLobby(const ChatLobbyId& lobby_id)
{
	mChatSrv->unsubscribeChatLobby(lobby_id) ;
}
bool p3Msgs::setDefaultIdentityForChatLobby(const RsGxsId& nick)
{
    return mChatSrv->setDefaultIdentityForChatLobby(nick) ;
}
void p3Msgs::getDefaultIdentityForChatLobby(RsGxsId& nick_name)
{
    mChatSrv->getDefaultIdentityForChatLobby(nick_name) ;
}

bool p3Msgs::setIdentityForChatLobby(const ChatLobbyId& lobby_id,const RsGxsId& nick)
{
    return mChatSrv->setIdentityForChatLobby(lobby_id,nick) ;
}
bool p3Msgs::getIdentityForChatLobby(const ChatLobbyId& lobby_id,RsGxsId& nick_name)
{
    return mChatSrv->getIdentityForChatLobby(lobby_id,nick_name) ;
}

bool p3Msgs::joinVisibleChatLobby(const ChatLobbyId& lobby_id,const RsGxsId& own_id)
{
    return mChatSrv->joinVisibleChatLobby(lobby_id,own_id) ;
}

void p3Msgs::getListOfNearbyChatLobbies(std::vector<VisibleChatLobbyRecord>& public_lobbies) 
{
	mChatSrv->getListOfNearbyChatLobbies(public_lobbies) ;
}

ChatLobbyId p3Msgs::createChatLobby(const std::string& lobby_name,const RsGxsId& lobby_identity,const std::string& lobby_topic,const std::set<RsPeerId>& invited_friends,ChatLobbyFlags privacy_type)
{
    return mChatSrv->createChatLobby(lobby_name,lobby_identity,lobby_topic,invited_friends,privacy_type) ;
}

void p3Msgs::setLobbyAutoSubscribe(const ChatLobbyId& lobby_id, const bool autoSubscribe)
{
    mChatSrv->setLobbyAutoSubscribe(lobby_id, autoSubscribe);
}

bool p3Msgs::getLobbyAutoSubscribe(const ChatLobbyId& lobby_id)
{
    return mChatSrv->getLobbyAutoSubscribe(lobby_id);
}


bool p3Msgs::acceptLobbyInvite(const ChatLobbyId& id,const RsGxsId& gxs_id)
{
    return mChatSrv->acceptLobbyInvite(id,gxs_id) ;
}
void p3Msgs::denyLobbyInvite(const ChatLobbyId& id) 
{
	mChatSrv->denyLobbyInvite(id) ;
}
void p3Msgs::getPendingChatLobbyInvites(std::list<ChatLobbyInvite>& invites) 
{
	mChatSrv->getPendingChatLobbyInvites(invites) ;
}
bool p3Msgs::initiateDistantChatConnexion(
        const RsGxsId& to_gxs_id, const RsGxsId& from_gxs_id,
        DistantChatPeerId& pid, uint32_t& error_code, bool notify )
{
	return mChatSrv->initiateDistantChatConnexion( to_gxs_id, from_gxs_id, pid, error_code, notify );
}
bool p3Msgs::getDistantChatStatus(const DistantChatPeerId& pid,DistantChatPeerInfo& info)
{
    return mChatSrv->getDistantChatStatus(pid,info) ;
}
bool p3Msgs::closeDistantChatConnexion(const DistantChatPeerId &pid)
{
	return mChatSrv->closeDistantChatConnexion(pid) ;
}
bool p3Msgs::setDistantChatPermissionFlags(uint32_t flags)
{
	return mChatSrv->setDistantChatPermissionFlags(flags) ;
}
uint32_t p3Msgs::getDistantChatPermissionFlags()
{
	return mChatSrv->getDistantChatPermissionFlags() ;
}

RsMsgs::~RsMsgs() = default;
Rs::Msgs::MessageInfo::~MessageInfo() = default;
MsgInfoSummary::~MsgInfoSummary() = default;
VisibleChatLobbyRecord::~VisibleChatLobbyRecord() = default;

void RsMailIdRecipientIdPair::serial_process(
        RsGenericSerializer::SerializeJob j,
        RsGenericSerializer::SerializeContext& ctx )
{
	RS_SERIAL_PROCESS(mMailId);
	RS_SERIAL_PROCESS(mRecipientId);
}

bool RsMailIdRecipientIdPair::operator<(const RsMailIdRecipientIdPair& o) const
{
	return std::tie(  mMailId,   mRecipientId) <
	       std::tie(o.mMailId, o.mRecipientId);
}

bool RsMailIdRecipientIdPair::operator==(const RsMailIdRecipientIdPair& o) const
{
	return std::tie(  mMailId,   mRecipientId) ==
	       std::tie(o.mMailId, o.mRecipientId);
}
