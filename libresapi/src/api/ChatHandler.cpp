#include "ChatHandler.h"
#include "Pagination.h"
#include "Operators.h"
#include "GxsResponseTask.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#include <sstream>
#include <algorithm>

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
    left << makeKeyValueReference("id", l.id)
         << makeKeyValue("chat_id", ChatId(l.id).toStdString())
         << makeKeyValueReference("name",l.name)
         << makeKeyValueReference("topic", l.topic)
         << makeKeyValueReference("subscribed", l.subscribed)
         << makeKeyValueReference("auto_subscribe", l.auto_subscribe)
         << makeKeyValueReference("is_private", l.is_private)
         << makeKeyValueReference("gxs_id", l.gxs_id);
    return left;
}

StreamBase& operator << (StreamBase& left, ChatHandler::ChatInfo& info)
{
    left << makeKeyValueReference("remote_author_id", info.remote_author_id)
         << makeKeyValueReference("remote_author_name", info.remote_author_name)
         << makeKeyValueReference("is_broadcast", info.is_broadcast)
         << makeKeyValueReference("is_gxs_id", info.is_gxs_id)
         << makeKeyValueReference("is_lobby", info.is_lobby)
         << makeKeyValueReference("is_peer", info.is_peer);
    return left;
}

ChatHandler::ChatHandler(StateTokenServer *sts, RsNotify *notify, RsMsgs *msgs, RsPeers* peers, RsIdentity* identity, UnreadMsgNotify* unread):
    mStateTokenServer(sts), mNotify(notify), mRsMsgs(msgs), mRsPeers(peers), mRsIdentity(identity), mUnreadMsgNotify(unread), mMtx("ChatHandler::mMtx")
{
    mNotify->registerNotifyClient(this);
    mStateTokenServer->registerTickClient(this);

    mMsgStateToken = mStateTokenServer->getNewToken();
    mLobbiesStateToken = mStateTokenServer->getNewToken();
    mUnreadMsgsStateToken = mStateTokenServer->getNewToken();

    addResourceHandler("*", this, &ChatHandler::handleWildcard);
    addResourceHandler("lobbies", this, &ChatHandler::handleLobbies);
    addResourceHandler("subscribe_lobby", this, &ChatHandler::handleSubscribeLobby);
    addResourceHandler("unsubscribe_lobby", this, &ChatHandler::handleUnsubscribeLobby);
    addResourceHandler("messages", this, &ChatHandler::handleMessages);
    addResourceHandler("send_message", this, &ChatHandler::handleSendMessage);
    addResourceHandler("mark_chat_as_read", this, &ChatHandler::handleMarkChatAsRead);
    addResourceHandler("info", this, &ChatHandler::handleInfo);
    addResourceHandler("typing_label", this, &ChatHandler::handleTypingLabel);
    addResourceHandler("send_status", this, &ChatHandler::handleSendStatus);
    addResourceHandler("unread_msgs", this, &ChatHandler::handleUnreadMsgs);
}

ChatHandler::~ChatHandler()
{
    mNotify->unregisterNotifyClient(this);
    mStateTokenServer->unregisterTickClient(this);
}

void ChatHandler::notifyChatMessage(const ChatMessage &msg)
{
    RS_STACK_MUTEX(mMtx); /********** LOCKED **********/
    mRawMsgs.push_back(msg);
}

// to be removed
/*
ChatHandler::Lobby ChatHandler::getLobbyInfo(ChatLobbyId id)
{
    tick();

    RS_STACK_MUTEX(mMtx); // ********* LOCKED **********
    for(std::vector<Lobby>::iterator vit = mLobbies.begin(); vit != mLobbies.end(); ++vit)
        if(vit->id == id)
            return *vit;
    std::cerr << "ChatHandler::getLobbyInfo Error: Lobby not found" << std::endl;
    return Lobby();
}
*/

void ChatHandler::tick()
{
    RS_STACK_MUTEX(mMtx); /********** LOCKED **********/

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
            l.gxs_id = info.gxs_id;
            lobbies.push_back(l);
        }
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
        else if(msg.chat_id.isGxsId())
        {
            author_id = msg.chat_id.toGxsId().toStdString();
            RsIdentityDetails details;
            if(!gxs_id_failed && mRsIdentity->getIdDetails(msg.chat_id.toGxsId(), details))
            {
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
            info.is_gxs_id = msg.chat_id.isGxsId();
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
            else if(msg.chat_id.isGxsId())
            {
                RsIdentityDetails details;
                if(!gxs_id_failed && mRsIdentity->getIdDetails(msg.chat_id.toGxsId(), details))
                {
                    info.remote_author_id = msg.chat_id.toGxsId().toStdString();
                    info.remote_author_name = details.mNickname;
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
        const std::string& in = msg.msg;
        std::string out;
        bool ignore = false;
        bool keep_link = false;
        std::string last_six_chars;
        Triple current_link;
        std::vector<Triple> links;
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
                ignore = true;
            }
            std::string a = "</a>";
            if(   current_link.first != -1
               && last_six_chars.size() >= a.size()
               && last_six_chars.substr(last_six_chars.size()-a.size()) == a)
            {
                current_link.third = out.size();
                links.push_back(current_link);
                current_link = Triple();
            }
            if(!ignore || keep_link)
                out += in[i];
            // "falling edge" resets mode to keep
            if(in[i] == '>')
                ignore = false;

            last_six_chars += in[i];
            if(last_six_chars.size() > 6)
                last_six_chars = last_six_chars.substr(1);
        }
        m.msg = out;
        m.links = links;
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

void ChatHandler::handleWildcard(Request &req, Response &resp)
{
    RS_STACK_MUTEX(mMtx); /********** LOCKED **********/
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

    RS_STACK_MUTEX(mMtx); /********** LOCKED **********/
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
}

void ChatHandler::handleMessages(Request &req, Response &resp)
{
    RS_STACK_MUTEX(mMtx); /********** LOCKED **********/
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
        resp.mStateToken = mMsgStateToken; // even set state token, if not found yet, maybe later messages arrive and then the chat id will be found
        resp.setFail("chat with id=\""+req.mPath.top()+"\" not found");
        return;
    }
    resp.mStateToken = mMsgStateToken;
    handlePaginationRequest(req, resp, mit->second);
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

void ChatHandler::handleMarkChatAsRead(Request &req, Response &resp)
{
    RS_STACK_MUTEX(mMtx); /********** LOCKED **********/
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

    mStateTokenServer->replaceToken(mUnreadMsgsStateToken);
}

// to be removed
// we do now cache chat info, to be able to include it in new message notify easily
/*
class InfoResponseTask: public GxsResponseTask
{
public:
    InfoResponseTask(ChatHandler* ch, RsPeers* peers, RsIdentity* identity): GxsResponseTask(identity, 0), mChatHandler(ch), mRsPeers(peers), mState(BEGIN){}

    enum State {BEGIN, WAITING};
    ChatHandler* mChatHandler;
    RsPeers* mRsPeers;
    State mState;
    bool is_broadcast;
    bool is_gxs_id;
    bool is_lobby;
    bool is_peer;
    std::string remote_author_id;
    std::string remote_author_name;
    virtual void gxsDoWork(Request& req, Response& resp)
    {
        ChatId id(req.mPath.top());
        if(id.isNotSet())
        {
            resp.setFail("not a valid chat id");
            done();
            return;
        }
        if(mState == BEGIN)
        {
            is_broadcast = false;
            is_gxs_id = false;
            is_lobby = false;
            is_peer = false;
            if(id.isBroadcast())
            {
                is_broadcast = true;
            }
            else if(id.isGxsId())
            {
                is_gxs_id = true;
                remote_author_id = id.toGxsId().toStdString();
                requestGxsId(id.toGxsId());
            }
            else if(id.isLobbyId())
            {
                is_lobby = true;
                remote_author_id = "";
                remote_author_name = mChatHandler->getLobbyInfo(id.toLobbyId()).name;
            }
            else if(id.isPeerId())
            {
                is_peer = true;
                remote_author_id = id.toPeerId().toStdString();
                remote_author_name = mRsPeers->getPeerName(id.toPeerId());
            }
            else
            {
                std::cerr << "Error in InfoResponseTask::gxsDoWork(): unhandled chat_id=" << id.toStdString() << std::endl;
            }
            mState = WAITING;
        }
        else
        {
            if(is_gxs_id)
                remote_author_name = getName(id.toGxsId());
            resp.mDataStream << makeKeyValueReference("remote_author_id", remote_author_id)
                             << makeKeyValueReference("remote_author_name", remote_author_name)
                             << makeKeyValueReference("is_broadcast", is_broadcast)
                             << makeKeyValueReference("is_gxs_id", is_gxs_id)
                             << makeKeyValueReference("is_lobby", is_lobby)
                             << makeKeyValueReference("is_peer", is_peer);
            resp.setOk();
            done();
        }
    }
};

ResponseTask *ChatHandler::handleInfo(Request &req, Response &resp)
{
    return new InfoResponseTask(this, mRsPeers, mRsIdentity);
}
*/

void ChatHandler::handleInfo(Request &req, Response &resp)
{
    RS_STACK_MUTEX(mMtx); /********** LOCKED **********/
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

void ChatHandler::handleTypingLabel(Request &req, Response &resp)
{

}

void ChatHandler::handleSendStatus(Request &req, Response &resp)
{

}

void ChatHandler::handleUnreadMsgs(Request &req, Response &resp)
{
    RS_STACK_MUTEX(mMtx); /********** LOCKED **********/

    resp.mDataStream.getStreamToMember();
    for(std::map<ChatId, std::list<Msg> >::const_iterator mit = mMsgs.begin(); mit != mMsgs.end(); ++mit)
    {
        uint32_t count = 0;
        for(std::list<Msg>::const_iterator lit = mit->second.begin(); lit != mit->second.end(); ++lit)
            if(!lit->read)
                count++;
        std::map<ChatId, ChatInfo>::iterator mit2 = mChatInfo.find(mit->first);
        if(mit2 == mChatInfo.end())
            std::cerr << "Error in ChatHandler::handleUnreadMsgs(): ChatInfo not found. It is weird if this happens. Normally it should not happen." << std::endl;
        if(count && (mit2 != mChatInfo.end()))
        {
            resp.mDataStream.getStreamToMember()
                    << makeKeyValue("id", mit->first.toStdString())
                    << makeKeyValueReference("unread_count", count)
                    << mit2->second;
        }
    }
    resp.mStateToken = mUnreadMsgsStateToken;
}

} // namespace resource_api
