#pragma once

#include "ResourceRouter.h"
#include "StateTokenServer.h"
#include <retroshare/rsnotify.h>
#include <util/rsthreads.h>

class RsPeers;
class RsMsgs;

namespace resource_api
{

class PeersHandler: public ResourceRouter, NotifyClient, Tickable
{
public:
    PeersHandler(StateTokenServer* sts, RsNotify* notify, RsPeers* peers, RsMsgs* msgs);
    virtual ~PeersHandler();

    // from NotifyClient
    // note: this may get called from foreign threads
    virtual void notifyListChange(int list, int type); // friends list change

    // from Tickable
    virtual void tick();
private:
    void handleWildcard(Request& req, Response& resp);
    void handleExamineCert(Request& req, Response& resp);

    // a helper which ensures proper mutex locking
    StateToken getCurrentStateToken();

    StateTokenServer* mStateTokenServer;
    RsNotify* mNotify;
    RsPeers* mRsPeers;
    RsMsgs* mRsMsgs; // required for avatar data

    std::list<RsPeerId> mOnlinePeers;

    RsMutex mMtx;
    StateToken mStateToken; // mutex protected
};
} // namespace resource_api
