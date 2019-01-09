/*******************************************************************************
 * libresapi/api/ChatHandler.cpp                                               *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2015 by electron128         <electron128@yahoo.com>               *
 * Copyright 2017 by Gioacchino Mazzurco <gio@eigenlab.org>                    *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "ChatHandler.h"
#include "Pagination.h"
#include "Operators.h"
#include "GxsResponseTask.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rshistory.h>

#include <algorithm>
#include <limits>
#include <sstream>
#include <time.h>

namespace resource_api
{

std::string id(const ChatHandler::Msg& m)
{
    std::stringstream ss;
    ss << m.id;
    return ss.str();
}

StreamBase& operator << (StreamBase& left, ChatHandler::Msg& m)
{
    left << makeKeyValueReference("incoming", m.incoming)
         << makeKeyValueReference("was_send", m.was_send)
	     << makeKeyValueReference("read", m.read)
         << makeKeyValueReference("author_id", m.author_id)
         << makeKeyValueReference("author_name", m.author_name)
         << makeKeyValueReference("msg", m.msg)
         << makeKeyValueReference("recv_time", m.recv_time)
         << makeKeyValueReference("send_time", m.send_time)
         << makeKeyValueReference("id", m.id);
    StreamBase& ls = left.getStreamToMember("links");
    ls.getStreamToMember();
    for(std::vector<ChatHandler::Triple>::iterator vit = m.links.begin(); vit != m.links.end(); ++vit)
    {
        ls.getStreamToMember() << makeKeyValueReference("first", vit->first)
                               << makeKeyValueReference("second", vit->second)
                               << makeKeyValueReference("third", vit->third);
    }
    return left;
}

bool compare_lobby_id(const ChatHandler::Lobby& l1, const ChatHandler::Lobby& l2)
{
    if(l1.auto_subscribe && !l2.auto_subscribe) return true;
    if(!l1.auto_subscribe && l2.auto_subscribe) return false;
    if(l1.is_private && !l2.is_private) return true;
    if(!l1.is_private && l2.is_private) return false;
    if(l1.subscribed && !l2.subscribed) return true;
    if(!l1.subscribed && l2.subscribed) return false;
    return l1.id < l2.id;
}

StreamBase& operator <<(StreamBase& left, KeyValueReference<ChatLobbyId> kv)
{
    if(left.serialise())
    {
        std::stringstream ss;
        ss << kv.value;
        left << makeKeyValue(kv.key, ss.str());
    }
    else
    {
        std::string val;
        left << makeKeyValueReference(kv.key, val);
        std::stringstream ss(val);
        ss >> kv.value;
    }
    return left;
}

StreamBase& operator << (StreamBase& left, ChatHandler::Lobby& l)
{
    ChatId chatId(l.id);
    if (l.is_broadcast)
        chatId = ChatId::makeBroadcastId();
    left << makeKeyValueReference("id", l.id)
         << makeKeyValue("chat_id", chatId.toStdString())
         << makeKeyValueReference("name",l.name)
         << makeKeyValueReference("topic", l.topic)
         << makeKeyValueReference("subscribed", l.subscribed)
         << makeKeyValueReference("auto_subscribe", l.auto_subscribe)
         << makeKeyValueReference("is_private", l.is_private)
         << makeKeyValueReference("is_broadcast", l.is_broadcast)
         << makeKeyValueReference("gxs_id", l.gxs_id);
    return left;
}

StreamBase& operator << (StreamBase& left, ChatHandler::ChatInfo& info)
{
    left << makeKeyValueReference("remote_author_id", info.remote_author_id)
         << makeKeyValueReference("remote_author_name", info.remote_author_name)
	     << makeKeyValueReference("own_author_id", info.own_author_id)
	     << makeKeyValueReference("own_author_name", info.own_author_name)
         << makeKeyValueReference("is_broadcast", info.is_broadcast)
         << makeKeyValueReference("is_distant_chat_id", info.is_distant_chat_id)
         << makeKeyValueReference("is_lobby", info.is_lobby)
         << makeKeyValueReference("is_peer", info.is_peer);
    return left;
}

class SendLobbyParticipantsTask: public GxsResponseTask
{
public:
    SendLobbyParticipantsTask(RsIdentity* idservice, ChatHandler::LobbyParticipantsInfo pi):
        GxsResponseTask(idservice, 0), mParticipantsInfo(pi)
    {
		const auto& map = mParticipantsInfo.participants;
		for(auto mit = map.begin(); mit != map.end(); ++mit)
        {
            requestGxsId(mit->first);
        }
    }
private:
    ChatHandler::LobbyParticipantsInfo mParticipantsInfo;
protected:
    virtual void gxsDoWork(Request &/*req*/, Response &resp)
    {
        resp.mDataStream.getStreamToMember();
		const auto& map = mParticipantsInfo.participants;
		for(auto mit = map.begin(); mit != map.end(); ++mit)
        {
            StreamBase& stream = resp.mDataStream.getStreamToMember();
            double last_active = mit->second;
            stream << makeKeyValueReference("last_active", last_active);
            streamGxsId(mit->first, stream.getStreamToMember("identity"));
        }
        resp.mStateToken = mParticipantsInfo.state_token;
        resp.setOk();
        done();
    }

};

ChatHandler::ChatHandler(StateTokenServer *sts, RsNotify *notify, RsMsgs *msgs, RsPeers* peers, RsIdentity* identity, UnreadMsgNotify* unread):
    mStateTokenServer(sts), mNotify(notify), mRsMsgs(msgs), mRsPeers(peers), mRsIdentity(identity), mUnreadMsgNotify(unread), mMtx("ChatHandler::mMtx")
{
    mNotify->registerNotifyClient(this);
    mStateTokenServer->registerTickClient(this);

    mMsgStateToken = mStateTokenServer->getNewToken();
    mLobbiesStateToken = mStateTokenServer->getNewToken();
    mUnreadMsgsStateToken = mStateTokenServer->getNewToken();
	mInvitationsStateToken = mStateTokenServer->getNewToken();

    addResourceHandler("*", this, &ChatHandler::handleWildcard);
    addResourceHandler("lobbies", this, &ChatHandler::handleLobbies);
	addResourceHandler("create_lobby", this, &ChatHandler::handleCreateLobby);
    addResourceHandler("subscribe_lobby", this, &ChatHandler::handleSubscribeLobby);
    addResourceHandler("unsubscribe_lobby", this, &ChatHandler::handleUnsubscribeLobby);
	addResourceHandler("autosubscribe_lobby", this, &ChatHandler::handleAutoSubsribeLobby);
    addResourceHandler("clear_lobby", this, &ChatHandler::handleClearLobby);
	addResourceHandler("invite_to_lobby", this, &ChatHandler::handleInviteToLobby);
	addResourceHandler("get_invitations_to_lobby", this, &ChatHandler::handleGetInvitationsToLobby);
	addResourceHandler("answer_to_invitation", this, &ChatHandler::handleAnswerToInvitation);
    addResourceHandler("lobby_participants", this, &ChatHandler::handleLobbyParticipants);
	addResourceHandler("get_default_identity_for_chat_lobby", this, &ChatHandler::handleGetDefaultIdentityForChatLobby);
	addResourceHandler("set_default_identity_for_chat_lobby", this, &ChatHandler::handleSetDefaultIdentityForChatLobby);
	addResourceHandler("get_identity_for_chat_lobby", this, &ChatHandler::handleGetIdentityForChatLobby);
	addResourceHandler("set_identity_for_chat_lobby", this, &ChatHandler::handleSetIdentityForChatLobby);
    addResourceHandler("messages", this, &ChatHandler::handleMessages);
	addResourceHandler("send_message", this, &ChatHandler::handleSendMessage);
	addResourceHandler("mark_message_as_read", this, &ChatHandler::handleMarkMessageAsRead);
    addResourceHandler("mark_chat_as_read", this, &ChatHandler::handleMarkChatAsRead);
    addResourceHandler("info", this, &ChatHandler::handleInfo);
    addResourceHandler("receive_status", this, &ChatHandler::handleReceiveStatus);
    addResourceHandler("send_status", this, &ChatHandler::handleSendStatus);
    addResourceHandler("unread_msgs", this, &ChatHandler::handleUnreadMsgs);
	addResourceHandler("initiate_distant_chat", this, &ChatHandler::handleInitiateDistantChatConnexion);
	addResourceHandler("distant_chat_status", this, &ChatHandler::handleDistantChatStatus);
	addResourceHandler("close_distant_chat", this, &ChatHandler::handleCloseDistantChatConnexion);
}

ChatHandler::~ChatHandler()
{
    mNotify->unregisterNotifyClient(this);
    mStateTokenServer->unregisterTickClient(this);
}

void ChatHandler::notifyChatMessage(const ChatMessage &msg)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    mRawMsgs.push_back(msg);
}

void ChatHandler::notifyChatCleared(const ChatId &chat_id)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    //Remove processed messages
    std::list<Msg>& msgs = mMsgs[chat_id];
    msgs.clear();
    //Remove unprocessed messages
    for(std::list<ChatMessage>::iterator lit = mRawMsgs.begin(); lit != mRawMsgs.end();)
    {
        ChatMessage& msg = *lit;
        if (msg.chat_id == chat_id)
       {
            lit = mRawMsgs.erase(lit);
        } else {
            ++lit;
        }
    }
}

void ChatHandler::notifyChatStatus(const ChatId &chat_id, const std::string &status)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    locked_storeTypingInfo(chat_id, status);
}

void ChatHandler::notifyChatLobbyEvent(uint64_t lobby_id, uint32_t event_type,
                                       const RsGxsId &nickname, const std::string& any_string)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    if(event_type == RS_CHAT_LOBBY_EVENT_PEER_STATUS)
    {
        locked_storeTypingInfo(ChatId(lobby_id), any_string, nickname);
    }
}

void ChatHandler::notifyListChange(int list, int /*type*/)
{
	if(list == NOTIFY_LIST_CHAT_LOBBY_INVITATION)
	{
		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
		mStateTokenServer->replaceToken(mInvitationsStateToken);
	}
}

void ChatHandler::tick()
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********

    // first fetch lobbies
    std::vector<Lobby> lobbies;
    std::list<ChatLobbyId> subscribed_ids;
    mRsMsgs->getChatLobbyList(subscribed_ids);
    for(std::list<ChatLobbyId>::iterator lit = subscribed_ids.begin(); lit != subscribed_ids.end(); ++lit)
    {
        ChatLobbyInfo info;
        if(mRsMsgs->getChatLobbyInfo(*lit,info))
        {
            Lobby l;
            l.id = *lit;
            l.name = info.lobby_name;
            l.topic = info.lobby_topic;
            l.subscribed = true;
            l.auto_subscribe = info.lobby_flags & RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE;
            l.is_private = !(info.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC);
            l.is_broadcast = false;
            l.gxs_id = info.gxs_id;
            lobbies.push_back(l);

            // update the lobby participants list
            // maybe it causes to much traffic to do this in every tick,
            // because the client would get the whole list every time a message was received
            // we could reduce the checking frequency
            std::map<ChatLobbyId, LobbyParticipantsInfo>::iterator mit = mLobbyParticipantsInfos.find(*lit);
            if(mit == mLobbyParticipantsInfos.end())
            {
                mLobbyParticipantsInfos[*lit].participants = info.gxs_ids;
                mLobbyParticipantsInfos[*lit].state_token = mStateTokenServer->getNewToken();
            }
            else
            {
                LobbyParticipantsInfo& pi = mit->second;
				if(!std::equal(pi.participants.begin(), pi.participants.end(), info.gxs_ids.begin())
				        || pi.participants.size() != info.gxs_ids.size())
                {
                    pi.participants = info.gxs_ids;
                    mStateTokenServer->replaceToken(pi.state_token);
                }
            }
        }
    }

    // remove participants info of old lobbies
    std::vector<ChatLobbyId> participants_info_to_delete;
    for(std::map<ChatLobbyId, LobbyParticipantsInfo>::iterator mit = mLobbyParticipantsInfos.begin();
        mit != mLobbyParticipantsInfos.end(); ++mit)
    {
        if(std::find(subscribed_ids.begin(), subscribed_ids.end(), mit->first) == subscribed_ids.end())
        {
            participants_info_to_delete.push_back(mit->first);
        }
    }
    for(std::vector<ChatLobbyId>::iterator vit = participants_info_to_delete.begin(); vit != participants_info_to_delete.end(); ++vit)
    {
        LobbyParticipantsInfo& pi = mLobbyParticipantsInfos[*vit];
        mStateTokenServer->discardToken(pi.state_token);
        mLobbyParticipantsInfos.erase(*vit);
    }

    {
        Lobby l;
        l.name = "BroadCast";
        l.topic = "UnseenP2P broadcast chat: messages are sent to all connected friends.";
        l.subscribed = true;
        l.auto_subscribe = false;
        l.is_private = false;
        l.is_broadcast = true;
        lobbies.push_back(l);
    }

    std::vector<VisibleChatLobbyRecord> unsubscribed_lobbies;
    mRsMsgs->getListOfNearbyChatLobbies(unsubscribed_lobbies);
    for(std::vector<VisibleChatLobbyRecord>::iterator vit = unsubscribed_lobbies.begin(); vit != unsubscribed_lobbies.end(); ++vit)
    {
        const VisibleChatLobbyRecord& info = *vit;
        if(std::find(subscribed_ids.begin(), subscribed_ids.end(), info.lobby_id) == subscribed_ids.end())
        {
            Lobby l;
            l.id = info.lobby_id;
            l.name = info.lobby_name;
            l.topic = info.lobby_topic;
            l.subscribed = false;
            l.auto_subscribe = info.lobby_flags & RS_CHAT_LOBBY_FLAGS_AUTO_SUBSCRIBE;
            l.is_private = !(info.lobby_flags & RS_CHAT_LOBBY_FLAGS_PUBLIC);
            l.is_broadcast = false;
            l.gxs_id = RsGxsId();
            lobbies.push_back(l);
        }
    }

    // process new messages
    bool changed = false;
    bool lobby_unread_count_changed = false;
    std::vector<std::list<ChatMessage>::iterator> done;
    std::vector<RsPeerId> peers_changed;

    bool gxs_id_failed = false; // to prevent asking for multiple failing gxs ids in one tick, to not flush the cache

    for(std::list<ChatMessage>::iterator lit = mRawMsgs.begin(); lit != mRawMsgs.end(); ++lit)
    {
        ChatMessage& msg = *lit;
        std::string author_id;
        std::string author_name;
        if(msg.chat_id.isBroadcast())
        {
            author_id = msg.broadcast_peer_id.toStdString();
            author_name = mRsPeers->getPeerName(msg.broadcast_peer_id);
        }
        else if(msg.chat_id.isDistantChatId())
        {
            DistantChatPeerInfo dcpinfo ;
            
            if(!rsMsgs->getDistantChatStatus(msg.chat_id.toDistantChatId(),dcpinfo))
            {
                std::cerr << "(EE) cannot get info for distant chat peer " << msg.chat_id.toDistantChatId() << std::endl;
                continue ;
            }

            RsIdentityDetails details;
            if(!gxs_id_failed && mRsIdentity->getIdDetails(msg.incoming? dcpinfo.to_id: dcpinfo.own_id, details))
            {
                author_id = details.mId.toStdString();
                author_name = details.mNickname;
            }
            else
            {
                gxs_id_failed = true;
                continue;
            }
        }
        else if(msg.chat_id.isLobbyId())
        {
            author_id = msg.lobby_peer_gxs_id.toStdString();
            RsIdentityDetails details;
            if(!gxs_id_failed && mRsIdentity->getIdDetails(msg.lobby_peer_gxs_id, details))
            {
                author_name = details.mNickname;
                lobby_unread_count_changed = true;
            }
            else
            {
                gxs_id_failed = true;
                continue;
            }
        }
        else if(msg.chat_id.isPeerId())
        {
            RsPeerId id;
            if(msg.incoming)
                id = msg.chat_id.toPeerId();
            else
                id = mRsPeers->getOwnId();
            author_id = id.toStdString();
            author_name = mRsPeers->getPeerName(id);
            if(std::find(peers_changed.begin(), peers_changed.end(), msg.chat_id.toPeerId()) == peers_changed.end())
                peers_changed.push_back(msg.chat_id.toPeerId());
        }
        else
        {
            std::cerr << "Error in ChatHandler::tick(): unhandled chat_id=" << msg.chat_id.toStdString() << std::endl;
            // remove from queue, so msgs with wrong ids to not pile up
            done.push_back(lit);
            continue;
        }

        if(mChatInfo.find(msg.chat_id) == mChatInfo.end())
        {
            ChatInfo info;
            info.is_broadcast = msg.chat_id.isBroadcast();
            info.is_distant_chat_id = msg.chat_id.isDistantChatId();
            info.is_lobby = msg.chat_id.isLobbyId();
            info.is_peer = msg.chat_id.isPeerId();
            if(msg.chat_id.isLobbyId())
            {
                for(std::vector<Lobby>::iterator vit = mLobbies.begin(); vit != mLobbies.end(); ++vit)
                {
                    if(vit->id == msg.chat_id.toLobbyId())
                        info.remote_author_name = vit->name;
                }
            }
            else if(msg.chat_id.isDistantChatId())
			{
				RsIdentityDetails detailsRemoteIdentity;
				RsIdentityDetails detailsOwnIdentity;
				DistantChatPeerInfo dcpinfo;

				if( !gxs_id_failed &&
				        rsMsgs->getDistantChatStatus(
				            msg.chat_id.toDistantChatId(), dcpinfo )
				        && mRsIdentity->getIdDetails(dcpinfo.to_id, detailsRemoteIdentity)
				        && mRsIdentity->getIdDetails(dcpinfo.own_id, detailsOwnIdentity))
				{
					info.remote_author_id = detailsRemoteIdentity.mId.toStdString();
					info.remote_author_name = detailsRemoteIdentity.mNickname;

					info.own_author_id = detailsOwnIdentity.mId.toStdString();
					info.own_author_name = detailsOwnIdentity.mNickname;
				}
                else
                {
                    gxs_id_failed = true;
                    continue;
                }
            }
            else if(msg.chat_id.isPeerId())
            {
                info.remote_author_id = msg.chat_id.toPeerId().toStdString();
                info.remote_author_name = mRsPeers->getPeerName(msg.chat_id.toPeerId());
            }
            mChatInfo[msg.chat_id] = info;
        }

        Msg m;
        m.read = !msg.incoming;
        m.incoming = msg.incoming;
        m.was_send = msg.online;
        m.author_id = author_id;
        m.author_name = author_name;

        // remove html tags from chat message
        // extract links form href
        getPlainText(msg.msg, m.msg, m.links);
        m.recv_time = msg.recvTime;
        m.send_time = msg.sendTime;

        m.id = RSRandom::random_u32();

        mMsgs[msg.chat_id].push_back(m);
        done.push_back(lit);

        changed = true;
    }
    for(std::vector<std::list<ChatMessage>::iterator>::iterator vit = done.begin(); vit != done.end(); ++vit)
        mRawMsgs.erase(*vit);

    // send changes

    if(changed)
    {
        mStateTokenServer->replaceToken(mMsgStateToken);
        mStateTokenServer->replaceToken(mUnreadMsgsStateToken);
    }

    for(std::vector<RsPeerId>::iterator vit = peers_changed.begin(); vit != peers_changed.end(); ++vit)
    {
        const std::list<Msg>& msgs = mMsgs[ChatId(*vit)];
        uint32_t count = 0;
        for(std::list<Msg>::const_iterator lit = msgs.begin(); lit != msgs.end(); ++lit)
            if(!lit->read)
                count++;
        if(mUnreadMsgNotify)
            mUnreadMsgNotify->notifyUnreadMsgCountChanged(*vit, count);
    }

    std::sort(lobbies.begin(), lobbies.end(), &compare_lobby_id);
    if(lobby_unread_count_changed || mLobbies != lobbies)
    {
        mStateTokenServer->replaceToken(mLobbiesStateToken);
        mLobbies = lobbies;
    }
}

static void writeUTF8 ( std::ostream & Out, unsigned int Ch )
/* writes Ch in UTF-8 encoding to Out. Note this version only deals
   with characters up to 16 bits.
   From: http://www.codecodex.com/wiki/Unescape_HTML_special_characters_from_a_String*/
{
    if (Ch >= 0x800)
    {
        Out.put(0xE0 | ((Ch >> 12) & 0x0F));
        Out.put(0x80 | ((Ch >>  6) & 0x3F));
        Out.put(0x80 | ((Ch      ) & 0x3F));
    }
    else if (Ch >= 0x80)
    {
        Out.put(0xC0 | ((Ch >>  6) & 0x1F));
        Out.put(0x80 | ((Ch      ) & 0x3F));
    }
    else
    {
        Out.put(Ch);
    }
}

static unsigned int stringToDecUInt ( std::string str)
{
    unsigned int out = 0;
    unsigned int max = std::numeric_limits<int>::max() / 10;
    int lenght = str.length();
    for (int curs = 0; curs < lenght; ++curs) {
        char c = str[curs];
        if ( (c >= '0')
             && (c <= '9')
             && (out < max)
             ) {
            out *= 10;
            out += (int)( c - '0');
        } else return 0;
    }
    return out;
}

void ChatHandler::getPlainText(const std::string& in, std::string &out, std::vector<Triple> &links)
{
    if (in.size() == 0)
        return;

    if (in[0] != '<' || in[in.size() - 1] != '>')
    {
        // It's a plain text message without HTML
        out = in;
        return;
    }
    bool ignore = false;

    bool keep_link = false;
    std::string last_six_chars;
    unsigned int tag_start_index = 0;
    Triple current_link;
    bool onEscapeChar = false;
    unsigned int escapeCharIndexStart = -1;

    for(unsigned int i = 0; i < in.size(); ++i)
    {
        if(keep_link && in[i] == '"')
        {
            keep_link = false;
            current_link.second = out.size();
        }
        if(last_six_chars == "href=\"")
        {
            keep_link = true;
            current_link.first = out.size();
        }

        // "rising edge" sets mode to ignore
        if(in[i] == '<')
        {
            tag_start_index = i;
            ignore = true;
        }
        if(!ignore || keep_link)
            out += in[i];
        // "falling edge" resets mode to keep
        if(in[i] == '>' && ignore) {
            // leave ignore mode on, if it's a style tag
            if (tag_start_index == 0 || tag_start_index + 6 > i || in.substr(tag_start_index, 6) != "<style") {
                ignore = false;
                tag_start_index = 0;
            }
        }

        last_six_chars += in[i];
        if(last_six_chars.size() > 6)
            last_six_chars = last_six_chars.substr(1);
        std::string a = "</a>";
        if(   current_link.first != -1
              && last_six_chars.size() >= a.size()
              && last_six_chars.substr(last_six_chars.size()-a.size()) == a)
        {
            // only allow these protocols
            // we don't want for example javascript:alert(0)
            std::string http = "http://";
            std::string https = "https://";
            std::string retroshare = "unseenp2p://";
            if(    out.substr(current_link.first, http.size()) == http
                   || out.substr(current_link.first, https.size()) == https
                   || out.substr(current_link.first, retroshare.size()) == retroshare)
            {
                current_link.third = out.size();
                links.push_back(current_link);
            }
            current_link = Triple();
        }
        std::string br = "<br/>";
        if(last_six_chars.size() >= br.size()
           && last_six_chars.substr(last_six_chars.size()-br.size()) == br)
        {
            out += "\n";
        }
        if (in[i] == '&') {
            onEscapeChar = true;
            escapeCharIndexStart = out.length();
        }
        if (ignore || keep_link) {
            onEscapeChar = false;
            escapeCharIndexStart = -1;
        }
        if ((in[i] == ';') && onEscapeChar) {
            onEscapeChar = false;
            bool escapeFound = true;
            std::string escapeReplace = "";
            //Keep only escape value to replace it
            std::string escapeCharValue = out.substr(escapeCharIndexStart,
                                                     out.length() - escapeCharIndexStart - 1);
            if (escapeCharValue[0] == '#') {
                int escapedCharUTF8 = stringToDecUInt(escapeCharValue.substr(1));
                std::ostringstream escapedSStream;
                writeUTF8( escapedSStream, escapedCharUTF8);
                escapeReplace = escapedSStream.str();
            } else if (escapeCharValue == "euro") {
                escapeReplace = "€";
            } else if (escapeCharValue == "nbsp") {
                escapeReplace = " ";
            } else if (escapeCharValue == "quot") {
                escapeReplace = "\"";
            } else if (escapeCharValue == "amp") {
                escapeReplace = "&";
            } else if (escapeCharValue == "lt") {
                escapeReplace = "<";
            } else if (escapeCharValue == "gt") {
                escapeReplace = ">";
            } else if (escapeCharValue == "iexcl") {
                escapeReplace = "¡";
            } else if (escapeCharValue == "cent") {
                escapeReplace = "¢";
            } else if (escapeCharValue == "pound") {
                escapeReplace = "£";
            } else if (escapeCharValue == "curren") {
                escapeReplace = "¤";
            } else if (escapeCharValue == "yen") {
                escapeReplace = "¥";
            } else if (escapeCharValue == "brvbar") {
                escapeReplace = "¦";
            } else if (escapeCharValue == "sect") {
                escapeReplace = "§";
            } else if (escapeCharValue == "uml") {
                escapeReplace = "¨";
            } else if (escapeCharValue == "copy") {
                escapeReplace = "©";
            } else if (escapeCharValue == "ordf") {
                escapeReplace = "ª";
            } else if (escapeCharValue == "not") {
                escapeReplace = "¬";
            } else if (escapeCharValue == "shy") {
                escapeReplace = " ";//?
            } else if (escapeCharValue == "reg") {
                escapeReplace = "®";
            } else if (escapeCharValue == "macr") {
                escapeReplace = "¯";
            } else if (escapeCharValue == "deg") {
                escapeReplace = "°";
            } else if (escapeCharValue == "plusmn") {
                escapeReplace = "±";
            } else if (escapeCharValue == "sup2") {
                escapeReplace = "²";
            } else if (escapeCharValue == "sup3") {
                escapeReplace = "³";
            } else if (escapeCharValue == "acute") {
                escapeReplace = "´";
            } else if (escapeCharValue == "micro") {
                escapeReplace = "µ";
            } else if (escapeCharValue == "para") {
                escapeReplace = "¶";
            } else if (escapeCharValue == "middot") {
                escapeReplace = "·";
            } else if (escapeCharValue == "cedil") {
                escapeReplace = "¸";
            } else if (escapeCharValue == "sup1") {
                escapeReplace = "¹";
            } else if (escapeCharValue == "ordm") {
                escapeReplace = "º";
            } else if (escapeCharValue == "raquo") {
                escapeReplace = "»";
            } else if (escapeCharValue == "frac14") {
                escapeReplace = "¼";
            } else if (escapeCharValue == "frac12") {
                escapeReplace = "½";
            } else if (escapeCharValue == "frac34") {
                escapeReplace = "¾";
            } else if (escapeCharValue == "iquest") {
                escapeReplace = "¿";
            } else if (escapeCharValue == "Agrave") {
                escapeReplace = "À";
            } else if (escapeCharValue == "Aacute") {
                escapeReplace = "Á";
            } else if (escapeCharValue == "Acirc") {
                escapeReplace = "Â";
            } else if (escapeCharValue == "Atilde") {
                escapeReplace = "Ã";
            } else if (escapeCharValue == "Auml") {
                escapeReplace = "Ä";
            } else if (escapeCharValue == "Aring") {
                escapeReplace = "Å";
            } else if (escapeCharValue == "AElig") {
                escapeReplace = "Æ";
            } else if (escapeCharValue == "Ccedil") {
                escapeReplace = "Ç";
            } else if (escapeCharValue == "Egrave") {
                escapeReplace = "È";
            } else if (escapeCharValue == "Eacute") {
                escapeReplace = "É";
            } else if (escapeCharValue == "Ecirc") {
                escapeReplace = "Ê";
            } else if (escapeCharValue == "Euml") {
                escapeReplace = "Ë";
            } else if (escapeCharValue == "Igrave") {
                escapeReplace = "Ì";
            } else if (escapeCharValue == "Iacute") {
                escapeReplace = "Í";
            } else if (escapeCharValue == "Icirc") {
                escapeReplace = "Î";
            } else if (escapeCharValue == "Iuml") {
                escapeReplace = "Ï";
            } else if (escapeCharValue == "ETH") {
                escapeReplace = "Ð";
            } else if (escapeCharValue == "Ntilde") {
                escapeReplace = "Ñ";
            } else if (escapeCharValue == "Ograve") {
                escapeReplace = "Ò";
            } else if (escapeCharValue == "Oacute") {
                escapeReplace = "Ó";
            } else if (escapeCharValue == "Ocirc") {
                escapeReplace = "Ô";
            } else if (escapeCharValue == "Otilde") {
                escapeReplace = "Õ";
            } else if (escapeCharValue == "Ouml") {
                escapeReplace = "Ö";
            } else if (escapeCharValue == "times") {
                escapeReplace = "×";
            } else if (escapeCharValue == "Oslash") {
                escapeReplace = "Ø";
            } else if (escapeCharValue == "Ugrave") {
                escapeReplace = "Ù";
            } else if (escapeCharValue == "Uacute") {
                escapeReplace = "Ú";
            } else if (escapeCharValue == "Ucirc") {
                escapeReplace = "Û";
            } else if (escapeCharValue == "Uuml") {
                escapeReplace = "Ü";
            } else if (escapeCharValue == "Yacute") {
                escapeReplace = "Ý";
            } else if (escapeCharValue == "THORN") {
                escapeReplace = "Þ";
            } else if (escapeCharValue == "szlig") {
                escapeReplace = "ß";
            } else if (escapeCharValue == "agrave") {
                escapeReplace = "à";
            } else if (escapeCharValue == "aacute") {
                escapeReplace = "á";
            } else if (escapeCharValue == "acirc") {
                escapeReplace = "â";
            } else if (escapeCharValue == "atilde") {
                escapeReplace = "ã";
            } else if (escapeCharValue == "auml") {
                escapeReplace = "ä";
            } else if (escapeCharValue == "aring") {
                escapeReplace = "å";
            } else if (escapeCharValue == "aelig") {
                escapeReplace = "æ";
            } else if (escapeCharValue == "ccedil") {
                escapeReplace = "ç";
            } else if (escapeCharValue == "egrave") {
                escapeReplace = "è";
            } else if (escapeCharValue == "eacute") {
                escapeReplace = "é";
            } else if (escapeCharValue == "ecirc") {
                escapeReplace = "ê";
            } else if (escapeCharValue == "euml") {
                escapeReplace = "ë";
            } else if (escapeCharValue == "igrave") {
                escapeReplace = "ì";
            } else if (escapeCharValue == "iacute") {
                escapeReplace = "í";
            } else if (escapeCharValue == "icirc") {
                escapeReplace = "î";
            } else if (escapeCharValue == "iuml") {
                escapeReplace = "ï";
            } else if (escapeCharValue == "eth") {
                escapeReplace = "ð";
            } else if (escapeCharValue == "ntilde") {
                escapeReplace = "ñ";
            } else if (escapeCharValue == "ograve") {
                escapeReplace = "ò";
            } else if (escapeCharValue == "oacute") {
                escapeReplace = "ó";
            } else if (escapeCharValue == "ocirc") {
                escapeReplace = "ô";
            } else if (escapeCharValue == "otilde") {
                escapeReplace = "õ";
            } else if (escapeCharValue == "ouml") {
                escapeReplace = "ö";
            } else if (escapeCharValue == "divide") {
                escapeReplace = "÷";
            } else if (escapeCharValue == "oslash") {
                escapeReplace = "ø";
            } else if (escapeCharValue == "ugrave") {
                escapeReplace = "ù";
            } else if (escapeCharValue == "uacute") {
                escapeReplace = "ú";
            } else if (escapeCharValue == "ucirc") {
                escapeReplace = "û";
            } else if (escapeCharValue == "uuml") {
                escapeReplace = "ü";
            } else if (escapeCharValue == "yacute") {
                escapeReplace = "ý";
            } else if (escapeCharValue == "thorn") {
                escapeReplace = "þ";
            } else {
                escapeFound = false;
            }
            if (escapeFound) {
                out = out.substr(0, escapeCharIndexStart-1);
                out += escapeReplace;
            }
        }
    }
}

void ChatHandler::locked_storeTypingInfo(const ChatId &chat_id, std::string status, RsGxsId lobby_gxs_id)
{
    TypingLabelInfo& info = mTypingLabelInfo[chat_id];
    info.timestamp = time(0);
    info.status = status;
    mStateTokenServer->replaceToken(info.state_token);
    info.author_id = lobby_gxs_id;
}

void ChatHandler::handleWildcard(Request &/*req*/, Response &resp)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    resp.mDataStream.getStreamToMember();
    for(std::map<ChatId, std::list<Msg> >::iterator mit = mMsgs.begin(); mit != mMsgs.end(); ++mit)
    {
        resp.mDataStream.getStreamToMember() << makeValue(mit->first.toStdString());
    }
    resp.setOk();
}

void ChatHandler::handleLobbies(Request &/*req*/, Response &resp)
{
    tick();

	{
		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    resp.mDataStream.getStreamToMember();
    for(std::vector<Lobby>::iterator vit = mLobbies.begin(); vit != mLobbies.end(); ++vit)
    {
        uint32_t unread_msgs = 0;
        ChatId chat_id(vit->id);
        std::map<ChatId, std::list<Msg> >::iterator mit = mMsgs.find(chat_id);
        if(mit != mMsgs.end())
        {
            std::list<Msg>& msgs = mit->second;
            for(std::list<Msg>::iterator lit = msgs.begin(); lit != msgs.end(); ++lit)
                if(!lit->read)
                    unread_msgs++;
        }
        resp.mDataStream.getStreamToMember() << *vit << makeKeyValueReference("unread_msg_count", unread_msgs);
    }
    resp.mStateToken = mLobbiesStateToken;
	}
    resp.setOk();
}

void ChatHandler::handleSubscribeLobby(Request &req, Response &resp)
{
    ChatLobbyId id = 0;
    RsGxsId gxs_id;
    req.mStream << makeKeyValueReference("id", id) << makeKeyValueReference("gxs_id", gxs_id);

    if(id == 0)
    {
        resp.setFail("Error: id must not be null");
        return;
    }
    if(gxs_id.isNull())
    {
        resp.setFail("Error: gxs_id must not be null");
        return;
    }
    if(mRsMsgs->joinVisibleChatLobby(id, gxs_id))
        resp.setOk();
    else
        resp.setFail("lobby join failed. (See console for more info)");
}

void ChatHandler::handleUnsubscribeLobby(Request &req, Response &resp)
{
    ChatLobbyId id = 0;
    req.mStream << makeKeyValueReference("id", id);
    mRsMsgs->unsubscribeChatLobby(id);
    resp.setOk();
}

void ChatHandler::handleAutoSubsribeLobby(Request& req, Response& resp)
{
	ChatLobbyId chatId = 0;
	bool autosubsribe;
	req.mStream << makeKeyValueReference("chatid", chatId) << makeKeyValueReference("autosubsribe", autosubsribe);
	mRsMsgs->setLobbyAutoSubscribe(chatId, autosubsribe);
	resp.setOk();
}

void ChatHandler::handleClearLobby(Request &req, Response &resp)
{
    ChatLobbyId id = 0;
    req.mStream << makeKeyValueReference("id", id);
    if (id !=0) {
        notifyChatCleared(ChatId(id));
    } else {
        //Is BroadCast
        notifyChatCleared(ChatId("B"));
    }
    resp.setOk();
}

void ChatHandler::handleInviteToLobby(Request& req, Response& resp)
{
	std::string chat_id;
	std::string pgp_id;
	req.mStream << makeKeyValueReference("chat_id", chat_id);
	req.mStream << makeKeyValueReference("pgp_id", pgp_id);

	ChatId chatId(chat_id);
	RsPgpId pgpId(pgp_id);

	std::list<RsPeerId> peerIds;
	mRsPeers->getAssociatedSSLIds(pgpId, peerIds);

	for(std::list<RsPeerId>::iterator it = peerIds.begin(); it != peerIds.end(); it++)
		mRsMsgs->invitePeerToLobby(chatId.toLobbyId(), (*it));

	resp.setOk();
}

void ChatHandler::handleGetInvitationsToLobby(Request& /*req*/, Response& resp)
{
	std::list<ChatLobbyInvite> invites;
	mRsMsgs->getPendingChatLobbyInvites(invites);

	resp.mDataStream.getStreamToMember();
	for(std::list<ChatLobbyInvite>::const_iterator it = invites.begin(); it != invites.end(); ++it)
	{
		resp.mDataStream.getStreamToMember()
		        << makeKeyValue("peer_id", (*it).peer_id.toStdString())
		        << makeKeyValue("lobby_id", (*it).lobby_id)
		        << makeKeyValue("lobby_name", (*it).lobby_name)
		        << makeKeyValue("lobby_topic", (*it).lobby_topic);
	}

	resp.mStateToken = mInvitationsStateToken;
	resp.setOk();
}

void ChatHandler::handleAnswerToInvitation(Request& req, Response& resp)
{
	ChatLobbyId lobbyId = 0;
	req.mStream << makeKeyValueReference("lobby_id", lobbyId);

	bool join;
	req.mStream << makeKeyValueReference("join", join);

	std::string gxs_id;
	req.mStream << makeKeyValueReference("gxs_id", gxs_id);
	RsGxsId gxsId(gxs_id);

	if(join)
	{
		if(rsMsgs->acceptLobbyInvite(lobbyId, gxsId))
			resp.setOk();
		else
			resp.setFail();
	}
	else
	{
		rsMsgs->denyLobbyInvite(lobbyId);
		resp.setOk();
	}
}

ResponseTask* ChatHandler::handleLobbyParticipants(Request &req, Response &resp)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********

    ChatId id(req.mPath.top());
    if(!id.isLobbyId())
    {
        resp.setFail("Path element \""+req.mPath.top()+"\" is not a ChatLobbyId.");
        return 0;
    }
    std::map<ChatLobbyId, LobbyParticipantsInfo>::const_iterator mit = mLobbyParticipantsInfos.find(id.toLobbyId());
    if(mit == mLobbyParticipantsInfos.end())
    {
        resp.setFail("lobby not found");
        return 0;
    }
    return new SendLobbyParticipantsTask(mRsIdentity, mit->second);
}

void ChatHandler::handleGetDefaultIdentityForChatLobby(Request& /*req*/,
                                                       Response& resp)
{
	RsGxsId gxsId;
	mRsMsgs->getDefaultIdentityForChatLobby(gxsId);
	resp.mDataStream << makeKeyValue("gxs_id", gxsId.toStdString());
	resp.setOk();
}

void ChatHandler::handleSetDefaultIdentityForChatLobby(Request& req, Response& resp)
{
	std::string gxs_id;
	req.mStream << makeKeyValueReference("gxs_id", gxs_id);
	RsGxsId gxsId(gxs_id);

	if(gxsId.isNull())
	{
		resp.setFail("Error: gxs_id must not be null");
		return;
	}

	if(mRsMsgs->setDefaultIdentityForChatLobby(gxsId))
		resp.setOk();
	else
		resp.setFail("Failure to change default identity for chat lobby");
}

void ChatHandler::handleGetIdentityForChatLobby(Request& req, Response& resp)
{
	RsGxsId gxsId;
	std::string chat_id;
	req.mStream << makeKeyValueReference("chat_id", chat_id);
	ChatId chatId(chat_id);

	if(chatId.isNotSet())
	{
		resp.setFail("Error: chat_id must not be null");
		return;
	}

	if(mRsMsgs->getIdentityForChatLobby(chatId.toLobbyId(), gxsId))
	{
		resp.mDataStream << makeKeyValue("gxs_id", gxsId.toStdString());
		resp.setOk();
	}
	else
		resp.setFail();
}

void ChatHandler::handleSetIdentityForChatLobby(Request& req, Response& resp)
{
	std::string chat_id;
	req.mStream << makeKeyValueReference("chat_id", chat_id);
	ChatId chatId(chat_id);

	if(chatId.isNotSet())
	{
		resp.setFail("Error: chat_id must not be null");
		return;
	}

	std::string gxs_id;
	req.mStream << makeKeyValueReference("gxs_id", gxs_id);
	RsGxsId gxsId(gxs_id);

	if(gxsId.isNull())
	{
		resp.setFail("Error: gxs_id must not be null");
		return;
	}

	if(mRsMsgs->setIdentityForChatLobby(chatId.toLobbyId(), gxsId))
		resp.setOk();
	else
		resp.setFail();
}

void ChatHandler::handleMessages(Request &req, Response &resp)
{
	/* G10h4ck: Whithout this the request processing won't happen, copied from
	 * ChatHandler::handleLobbies, is this a work around or is the right way of
	 * doing it? */
	tick();

	{
		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	ChatId id(req.mPath.top());

    // make response a list
    resp.mDataStream.getStreamToMember();
    if(id.isNotSet())
    {
		resp.setFail("\""+req.mPath.top()+"\" is not a valid chat id");
		return;
    }
    std::map<ChatId, std::list<Msg> >::iterator mit = mMsgs.find(id);
	if(mit == mMsgs.end())
	{
		/* set state token, even if not found yet, maybe later messages arrive
		 * and then the chat id will be found */
		resp.mStateToken = mMsgStateToken;
		resp.setFail("chat with id=\""+req.mPath.top()+"\" not found");
		return;
	}
    resp.mStateToken = mMsgStateToken;
    handlePaginationRequest(req, resp, mit->second);
	}
}

void ChatHandler::handleSendMessage(Request &req, Response &resp)
{
    std::string chat_id;
    std::string msg;
    req.mStream << makeKeyValueReference("chat_id", chat_id)
                << makeKeyValueReference("msg", msg);
    ChatId id(chat_id);
    if(id.isNotSet())
    {
        resp.setFail("chat_id is invalid");
        return;
    }
    if(mRsMsgs->sendChat(id, msg))
        resp.setOk();
    else
        resp.setFail("failed to send message");
}

void ChatHandler::handleMarkMessageAsRead(Request &req, Response &resp)
{
	std::string chat_id_string;
	std::string msg_id;
	req.mStream << makeKeyValueReference("chat_id", chat_id_string)
	            << makeKeyValueReference("msg_id", msg_id);

	ChatId chatId(chat_id_string);
	if(chatId.isNotSet())
	{
		resp.setFail(chat_id_string + " is not a valid chat id");
		return;
	}

	std::map<ChatId, std::list<Msg> >::iterator mit = mMsgs.find(chatId);
	if(mit == mMsgs.end())
	{
		resp.setFail("chat not found. Maybe this chat does not have messages yet?");
		return;
	}

	std::list<Msg>& msgs = mit->second;
	for(std::list<Msg>::iterator lit = msgs.begin(); lit != msgs.end(); ++lit)
	{
		if(id((*lit)) == msg_id)
			(*lit).read = true;
	}

	// lobby list contains unread msgs, so update it
	if(chatId.isLobbyId())
		mStateTokenServer->replaceToken(mLobbiesStateToken);
	if(chatId.isPeerId() && mUnreadMsgNotify)
		mUnreadMsgNotify->notifyUnreadMsgCountChanged(chatId.toPeerId(), 0);

	mStateTokenServer->replaceToken(mMsgStateToken);
	mStateTokenServer->replaceToken(mUnreadMsgsStateToken);
}

void ChatHandler::handleMarkChatAsRead(Request &req, Response &resp)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
	ChatId id(req.mPath.top());

    if(id.isNotSet())
    {
		resp.setFail("\""+req.mPath.top()+"\" is not a valid chat id");
        return;
    }
    std::map<ChatId, std::list<Msg> >::iterator mit = mMsgs.find(id);
    if(mit == mMsgs.end())
    {
        resp.setFail("chat not found. Maybe this chat does not have messages yet?");
        return;
    }
    std::list<Msg>& msgs = mit->second;
    for(std::list<Msg>::iterator lit = msgs.begin(); lit != msgs.end(); ++lit)
    {
        lit->read = true;
    }
    // lobby list contains unread msgs, so update it
    if(id.isLobbyId())
        mStateTokenServer->replaceToken(mLobbiesStateToken);
    if(id.isPeerId() && mUnreadMsgNotify)
        mUnreadMsgNotify->notifyUnreadMsgCountChanged(id.toPeerId(), 0);

	mStateTokenServer->replaceToken(mMsgStateToken);
    mStateTokenServer->replaceToken(mUnreadMsgsStateToken);
}

void ChatHandler::handleInfo(Request &req, Response &resp)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    ChatId id(req.mPath.top());
    if(id.isNotSet())
    {
        resp.setFail("\""+req.mPath.top()+"\" is not a valid chat id");
        return;
    }
    std::map<ChatId, ChatInfo>::iterator mit = mChatInfo.find(id);
    if(mit == mChatInfo.end())
    {
        resp.setFail("chat not found.");
        return;
    }
    resp.mDataStream << mit->second;
    resp.setOk();
}

class SendTypingLabelInfo: public GxsResponseTask
{
public:
    SendTypingLabelInfo(RsIdentity* identity, RsPeers* peers, ChatId id, const ChatHandler::TypingLabelInfo& info):
        GxsResponseTask(identity), mState(BEGIN), mPeers(peers),mId(id), mInfo(info) {}
private:
    enum State {BEGIN, WAITING_ID};
    State mState;
    RsPeers* mPeers;
    ChatId mId;
    ChatHandler::TypingLabelInfo mInfo;
protected:
    void gxsDoWork(Request& /*req*/, Response& resp)
    {
        if(mState == BEGIN)
        {
            // lobby and distant require to fetch a gxs_id
            if(mId.isLobbyId())
            {
				if(!mInfo.author_id.isNull())
					requestGxsId(mInfo.author_id);
            }
            else if(mId.isDistantChatId())
            {
                DistantChatPeerInfo dcpinfo ;
                rsMsgs->getDistantChatStatus(mId.toDistantChatId(), dcpinfo);
                requestGxsId(dcpinfo.to_id);
            }
            mState = WAITING_ID;
        }
        else
        {
            std::string name = "BUG: case not handled in SendTypingLabelInfo";
			RsGxsId author_id = mInfo.author_id;
            if(mId.isPeerId())
            {
                name = mPeers->getPeerName(mId.toPeerId());
            }
            else if(mId.isDistantChatId())
            {
                DistantChatPeerInfo dcpinfo ;
                rsMsgs->getDistantChatStatus(mId.toDistantChatId(), dcpinfo);
                name = getName(dcpinfo.to_id);
				author_id = dcpinfo.to_id;
            }
            else if(mId.isLobbyId())
            {
                name = getName(mInfo.author_id);
            }
            else if(mId.isBroadcast())
            {
                name = mPeers->getPeerName(mId.broadcast_status_peer_id);
            }
            uint32_t ts = mInfo.timestamp;
            resp.mDataStream << makeKeyValueReference("author_name", name)
			                 << makeKeyValue("author_id", author_id.toStdString())
                             << makeKeyValueReference("timestamp", ts)
                             << makeKeyValueReference("status_string", mInfo.status);
            resp.mStateToken = mInfo.state_token;
            resp.setOk();
            done();
        }
    }
};

ResponseTask* ChatHandler::handleReceiveStatus(Request &req, Response &resp)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    ChatId id(req.mPath.top());
    if(id.isNotSet())
    {
        resp.setFail("\""+req.mPath.top()+"\" is not a valid chat id");
        return 0;
    }
    std::map<ChatId, TypingLabelInfo>::iterator mit = mTypingLabelInfo.find(id);
    if(mit == mTypingLabelInfo.end())
    {
        locked_storeTypingInfo(id, "");
        mit = mTypingLabelInfo.find(id);
    }

    return new SendTypingLabelInfo(mRsIdentity, mRsPeers, id, mit->second);
}

void ChatHandler::handleSendStatus(Request &req, Response &resp)
{
    std::string chat_id;
    std::string status;
    req.mStream << makeKeyValueReference("chat_id", chat_id)
                << makeKeyValueReference("status", status);
    ChatId id(chat_id);
    if(id.isNotSet())
    {
        resp.setFail("chat_id is invalid");
        return;
    }
    mRsMsgs->sendStatusString(id, status);
    resp.setOk();
}

void ChatHandler::handleUnreadMsgs(Request &/*req*/, Response &resp)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********

    resp.mDataStream.getStreamToMember();
	for( std::map<ChatId, std::list<Msg> >::const_iterator mit = mMsgs.begin();
	     mit != mMsgs.end(); ++mit )
    {
        uint32_t count = 0;
		for( std::list<Msg>::const_iterator lit = mit->second.begin();
		     lit != mit->second.end(); ++lit ) if(!lit->read) ++count;
        std::map<ChatId, ChatInfo>::iterator mit2 = mChatInfo.find(mit->first);
        if(mit2 == mChatInfo.end())
            std::cerr << "Error in ChatHandler::handleUnreadMsgs(): ChatInfo not found. It is weird if this happens. Normally it should not happen." << std::endl;
        if(count && (mit2 != mChatInfo.end()))
        {
            resp.mDataStream.getStreamToMember()
#warning Gioacchino Mazzurco 2017-03-24: @deprecated using "id" as key can cause problems in some JS based \
	        languages like Qml @see chat_id instead
			        << makeKeyValue("id", mit->first.toStdString())
			        << makeKeyValue("chat_id", mit->first.toStdString())
                    << makeKeyValueReference("unread_count", count)
                    << mit2->second;
        }
    }
    resp.mStateToken = mUnreadMsgsStateToken;
	resp.setOk();
}

void ChatHandler::handleInitiateDistantChatConnexion(Request& req, Response& resp)
{
	std::string own_gxs_hex, remote_gxs_hex;

	req.mStream << makeKeyValueReference("own_gxs_hex", own_gxs_hex)
	            << makeKeyValueReference("remote_gxs_hex", remote_gxs_hex);

	RsGxsId sender_id(own_gxs_hex);
	if(sender_id.isNull())
	{
		resp.setFail("own_gxs_hex is invalid");
		return;
	}

	RsGxsId receiver_id(remote_gxs_hex);
	if(receiver_id.isNull())
	{
		resp.setFail("remote_gxs_hex is invalid");
		return;
	}

	DistantChatPeerId distant_chat_id;
	uint32_t error_code;

	if(mRsMsgs->initiateDistantChatConnexion( receiver_id, sender_id,
	                                          distant_chat_id, error_code,
	                                          false )) resp.setOk();
	else resp.setFail("Failed to initiate distant chat");

	ChatId chat_id(distant_chat_id);
	resp.mDataStream << makeKeyValue("chat_id", chat_id.toStdString())
	                 << makeKeyValueReference("error_code", error_code);
}

void ChatHandler::handleDistantChatStatus(Request& req, Response& resp)
{
	std::string distant_chat_hex;
	req.mStream << makeKeyValueReference("chat_id", distant_chat_hex);

	ChatId id(distant_chat_hex);
	DistantChatPeerInfo info;
	if(mRsMsgs->getDistantChatStatus(id.toDistantChatId(), info)) resp.setOk();
	else resp.setFail("Failed to get status for distant chat");

	resp.mDataStream << makeKeyValue("own_gxs_hex", info.own_id.toStdString())
	                 << makeKeyValue("remote_gxs_hex", info.to_id.toStdString())
	                 << makeKeyValue("chat_id", info.peer_id.toStdString())
	                 << makeKeyValue("status", info.status);
}

void ChatHandler::handleCloseDistantChatConnexion(Request& req, Response& resp)
{
	std::string distant_chat_hex;
	req.mStream << makeKeyValueReference("distant_chat_hex", distant_chat_hex);
	ChatId chatId(distant_chat_hex);

	if (mRsMsgs->closeDistantChatConnexion(chatId.toDistantChatId()))
		resp.setOk();
	else
		resp.setFail("Failed to close distant chat");
}

void ChatHandler::handleCreateLobby(Request& req, Response& resp)
{
	std::set<RsPeerId> invited_identites;
	std::string lobby_name;
	std::string lobby_topic;
	std::string gxs_id;

	req.mStream << makeKeyValueReference("lobby_name", lobby_name);
	req.mStream << makeKeyValueReference("lobby_topic", lobby_topic);
	req.mStream << makeKeyValueReference("gxs_id", gxs_id);

	RsGxsId gxsId(gxs_id);

	bool lobby_public;
	bool pgp_signed;

	req.mStream << makeKeyValueReference("lobby_public", lobby_public);
	req.mStream << makeKeyValueReference("pgp_signed", pgp_signed);

	ChatLobbyFlags lobby_flags;

	if(lobby_public)
		lobby_flags |= RS_CHAT_LOBBY_FLAGS_PUBLIC;

	if(pgp_signed)
		lobby_flags |= RS_CHAT_LOBBY_FLAGS_PGP_SIGNED;

	mRsMsgs->createChatLobby(lobby_name, gxsId, lobby_topic, invited_identites, lobby_flags);

	tick();
	resp.setOk();
}

} // namespace resource_api
