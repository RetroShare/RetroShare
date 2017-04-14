#pragma once

#include <retroshare/rsnotify.h>
#include <util/rsthreads.h>

#include "ResourceRouter.h"
#include "StateTokenServer.h"

class RsIdentity;

namespace resource_api
{

class IdentityHandler: public ResourceRouter, NotifyClient
{
public:
    IdentityHandler(StateTokenServer* sts, RsNotify* notify, RsIdentity* identity);
    virtual ~IdentityHandler();

    // from NotifyClient
    // note: this may get called from foreign threads
    virtual void notifyGxsChange(const RsGxsChanges &changes);

private:
    void handleWildcard(Request& req, Response& resp);
	void handleNotOwnIdsRequest(Request& req, Response& resp);
	void handleOwnIdsRequest(Request& req, Response& resp);

    ResponseTask *handleOwn(Request& req, Response& resp);
    ResponseTask *handleCreateIdentity(Request& req, Response& resp);
	ResponseTask *handleDeleteIdentity(Request& req, Response& resp);

    StateTokenServer* mStateTokenServer;
    RsNotify* mNotify;
    RsIdentity* mRsIdentity;

    RsMutex mMtx;
    StateToken mStateToken; // mutex protected
};
} // namespace resource_api
