#pragma once

#include "ResourceRouter.h"
#include "StateTokenServer.h"
#include <retroshare/rsnotify.h>
#include <retroshare/rsmsgs.h>

class RsPeers;
class RsIdentity;

namespace resource_api
{

class UnreadMsgNotify{
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
        Lobby(): id(0), subscribed(false), auto_subscribe(false), is_private(false){}
        ChatLobbyId id;
        std::string name;
        std::string topic;
        bool subscribed;
        bool auto_subscribe;
        bool is_private;

        RsGxsId gxs_id;// for subscribed lobbies: the id we use to write messages

        bool operator==(const Lobby& l) const
        {
            return id == l.id
                    && name == l.name
                    && topic == l.topic
                    && subscribed == l.subscribed
                    && auto_subscribe == l.auto_subscribe
                    && is_private == l.is_private
                    && gxs_id == l.gxs_id;
        }
    };

    class ChatInfo{
    public:
        bool is_broadcast;
        bool is_gxs_id;
        bool is_lobby;
        bool is_peer;
        std::string remote_author_id;
        std::string remote_author_name;
    };

private:
    void handleWildcard(Request& req, Response& resp);
    void handleLobbies(Request& req, Response& resp);
    void handleSubscribeLobby(Request& req, Response& resp);
    void handleUnsubscribeLobby(Request& req, Response& resp);
    void handleMessages(Request& req, Response& resp);
    void handleSendMessage(Request& req, Response& resp);
    void handleMarkChatAsRead(Request& req, Response& resp);
    void handleInfo(Request& req, Response& resp);
    void handleTypingLabel(Request& req, Response& resp);
    void handleSendStatus(Request& req, Response& resp);
    void handleUnreadMsgs(Request& req, Response& resp);

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

    StateToken mLobbiesStateToken;
    std::vector<Lobby> mLobbies;

    StateToken mUnreadMsgsStateToken;

};
} // namespace resource_api
