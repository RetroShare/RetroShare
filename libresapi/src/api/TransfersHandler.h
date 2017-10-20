#pragma once

#include "ResourceRouter.h"
#include "StateTokenServer.h"

#include <retroshare/rsfiles.h>
#include <retroshare/rspeers.h>

namespace resource_api
{

class TransfersHandler: public ResourceRouter, Tickable
{
public:
	TransfersHandler(StateTokenServer* sts, RsFiles* files, RsPeers *peers);
    virtual ~TransfersHandler();

    // from Tickable
    virtual void tick();
private:
    void handleWildcard(Request& req, Response& resp);
    void handleControlDownload(Request& req, Response& resp);
    void handleDownloads(Request& req, Response& resp);
	void handleUploads(Request& req, Response& resp);

    StateTokenServer* mStateTokenServer;
    RsFiles* mFiles;
	RsPeers* mRsPeers;

    StateToken mStateToken;
    time_t mLastUpdateTS;

    std::list<RsFileHash> mDownloadsAtLastCheck;
	std::list<RsFileHash> mUploadsAtLastCheck;
};

} // namespace resource_api
