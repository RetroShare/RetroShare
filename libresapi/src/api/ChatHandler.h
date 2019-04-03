/*******************************************************************************
 * libresapi/api/ChatHandler.h                                                 *
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
#pragma once

#include "ResourceRouter.h"
#include "StateTokenServer.h"
#include <retroshare/rsnotify.h>
#include <retroshare/rsmsgs.h>
#include "util/rstime.h"

class RsPeers;
struct RsIdentity;

namespace resource_api
{

class UnreadMsgNotify
{
public:
	virtual void notifyUnreadMsgCountChanged(const RsPeerId& peer, uint32_t count) = 0;
};

class ChatHandler: public ResourceRouter, NotifyClient, Tickable
{
public:
    ChatHandler(StateTokenServer* sts, RsNotify* notify, RsMsgs* msgs, RsPeers* peers, RsIdentity *identity, UnreadMsgNotify* unread);
    virtual ~ChatHandler();

    // from NotifyClient
    // note: this may get called from the own and from foreign threads
    virtual void notifyChatMessage(const ChatMessage& msg);
    virtual void notifyChatCleared(const ChatId& chat_id);

    // typing label for peer, broadcast and distant chat
    virtual void notifyChatStatus     (const ChatId&      /* chat_id  */, const std::string& /* status_string */);

    //typing label for lobby chat, peer join and leave messages
    virtual void notifyChatLobbyEvent (uint64_t           /* lobby id */, uint32_t           /* event type    */ ,
                                       const RsGxsId& /* nickname */,const std::string& /* any string */);

	virtual void notifyListChange(int list, int type);

    // from tickable
    virtual void tick();

public:
    class Triple
    {
    public:
        Triple(): first(-1), second(-1), third(-1){}
        double first;
        double second;
        double third;
    };

    class Msg{
    public:
        bool read;
        bool incoming;
        bool was_send;
        //std::string chat_type;
        std::string author_id; // peer or gxs id or "system" for system messages
        std::string author_name;
        std::string msg; // plain text only!
        std::vector<Triple> links;
        uint32_t recv_time;
        uint32_t send_time;

        uint32_t id;
    };

    class Lobby{
    public:
        Lobby(): id(0), subscribed(false), auto_subscribe(false), is_private(false), is_broadcast(false){}
        ChatLobbyId id;
        std::string name;
        std::string topic;
        bool subscribed;
        bool auto_subscribe;
        bool is_private;
        bool is_broadcast;

        RsGxsId gxs_id;// for subscribed lobbies: the id we use to write messages

        bool operator==(const Lobby& l) const
        {
            return id == l.id
                    && name == l.name
                    && topic == l.topic
                    && subscribed == l.subscribed
                    && auto_subscribe == l.auto_subscribe
                    && is_private == l.is_private
                    && is_broadcast == l.is_broadcast
                    && gxs_id == l.gxs_id;
        }
    };

    class LobbyParticipantsInfo{
    public:
        StateToken state_token;
		std::map<RsGxsId, rstime_t> participants;
    };

    class ChatInfo{
    public:
        bool is_broadcast;
        bool is_distant_chat_id;
        bool is_lobby;
        bool is_peer;
		std::string own_author_id;
		std::string own_author_name;
        std::string remote_author_id;
        std::string remote_author_name;
    };

    class TypingLabelInfo{
    public:
        time_t timestamp;
        std::string status;
        StateToken state_token;
        // only for lobbies
        RsGxsId author_id;
    };

private:
    void handleWildcard(Request& req, Response& resp);
    void handleLobbies(Request& req, Response& resp);
	void handleCreateLobby(Request& req, Response& resp);
    void handleSubscribeLobby(Request& req, Response& resp);
    void handleUnsubscribeLobby(Request& req, Response& resp);
	void handleAutoSubsribeLobby(Request& req, Response& resp);
	void handleInviteToLobby(Request& req, Response& resp);
	void handleGetInvitationsToLobby(Request& req, Response& resp);
	void handleAnswerToInvitation(Request& req, Response& resp);
    void handleClearLobby(Request& req, Response& resp);
    ResponseTask* handleLobbyParticipants(Request& req, Response& resp);
	void handleGetDefaultIdentityForChatLobby(Request& req, Response& resp);
	void handleSetDefaultIdentityForChatLobby(Request& req, Response& resp);
	void handleGetIdentityForChatLobby(Request& req, Response& resp);
	void handleSetIdentityForChatLobby(Request& req, Response& resp);
    void handleMessages(Request& req, Response& resp);
	void handleSendMessage(Request& req, Response& resp);
	void handleMarkMessageAsRead(Request& req, Response& resp);
    void handleMarkChatAsRead(Request& req, Response& resp);
    void handleInfo(Request& req, Response& resp);
    ResponseTask *handleReceiveStatus(Request& req, Response& resp);
    void handleSendStatus(Request& req, Response& resp);
    void handleUnreadMsgs(Request& req, Response& resp);
	void handleInitiateDistantChatConnexion(Request& req, Response& resp);
	void handleDistantChatStatus(Request& req, Response& resp);
	void handleCloseDistantChatConnexion(Request& req, Response& resp);

    void getPlainText(const std::string& in, std::string &out, std::vector<Triple> &links);
    // last parameter is only used for lobbies!
    void locked_storeTypingInfo(const ChatId& chat_id, std::string status, RsGxsId lobby_gxs_id = RsGxsId());

    StateTokenServer* mStateTokenServer;
    RsNotify* mNotify;
    RsMsgs* mRsMsgs;
    RsPeers* mRsPeers;
    RsIdentity* mRsIdentity;
    UnreadMsgNotify* mUnreadMsgNotify;

    RsMutex mMtx;

    StateToken mMsgStateToken;
    std::list<ChatMessage> mRawMsgs;
    std::map<ChatId, std::list<Msg> > mMsgs;

    std::map<ChatId, ChatInfo> mChatInfo;

    std::map<ChatId, TypingLabelInfo> mTypingLabelInfo;

    StateToken mLobbiesStateToken;
    std::vector<Lobby> mLobbies;

    std::map<ChatLobbyId, LobbyParticipantsInfo> mLobbyParticipantsInfos;

    StateToken mUnreadMsgsStateToken;
	StateToken mInvitationsStateToken;

};
} // namespace resource_api
