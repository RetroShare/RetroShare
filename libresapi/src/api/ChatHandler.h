#pragma once

#include "ResourceRouter.h"
#include "StateTokenServer.h"
#include <retroshare/rsnotify.h>

class RsMsgs;

namespace resource_api
{

class ChatHandler: public ResourceRouter, NotifyClient
{
public:
    ChatHandler(StateTokenServer* sts, RsNotify* notify, RsMsgs* msgs);
    virtual ~ChatHandler();

    // from NotifyClient
    // note: this may get called from the own and from foreign threads
    virtual void notifyChatMessage();

private:
    void handleWildcard(Request& req, Response& resp);

    StateTokenServer* mStateTokenServer;
    RsNotify* mNotify;
    RsMsgs* mRsMsgs;

    RsMutex mMtx;
    StateToken mStateToken; // mutex protected

    // msgs flow like this:
    // chatservice -> rawMsgs -> processedMsgs -> deletion

    std::map<ChatId, std::list<ChatMessage> > mRawMsgs;

    class Msg{
    public:
        bool incoming;
        bool was_send;
        //std::string chat_type;
        std::string author_id; // peer or gxs id or "system" for system messages
        std::string author_name;
        std::string msg; // plain text only!
    };

    std::map<ChatId, std::list<Msg> > mProcessedMsgs;

};
} // namespace resource_api
