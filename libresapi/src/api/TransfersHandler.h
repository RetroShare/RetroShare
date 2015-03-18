#pragma once

#pragma once
#include "ResourceRouter.h"
#include "StateTokenServer.h"

#include <retroshare/rsfiles.h>

namespace resource_api
{

class TransfersHandler: public ResourceRouter, Tickable
{
public:
    TransfersHandler(StateTokenServer* sts, RsFiles* files);
    virtual ~TransfersHandler();

    // from Tickable
    virtual void tick();
private:
    void handleWildcard(Request& req, Response& resp);
    void handleControlDownload(Request& req, Response& resp);
    void handleDownloads(Request& req, Response& resp);

    StateTokenServer* mStateTokenServer;
    RsFiles* mFiles;

    StateToken mStateToken;
    time_t mLastUpdateTS;

    std::list<RsFileHash> mDownloadsAtLastCheck;
};

} // namespace resource_api
