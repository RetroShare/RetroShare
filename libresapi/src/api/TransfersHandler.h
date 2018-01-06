#pragma once

#include "ResourceRouter.h"
#include "StateTokenServer.h"

#include <retroshare/rsfiles.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsnotify.h>

namespace resource_api
{

class TransfersHandler: public ResourceRouter, Tickable, NotifyClient
{
public:
	TransfersHandler(StateTokenServer* sts, RsFiles* files, RsPeers *peers, RsNotify& notify);
    virtual ~TransfersHandler();

	/**
	 Derived from NotifyClient
	 This function may be called from foreign thread
	*/
	virtual void notifyListChange(int list, int type);

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
	RsNotify& mNotify;

	/**
	 Protects mStateToken that may be changed in foreign thread
	 @see TransfersHandler::notifyListChange(...)
	*/
	RsMutex mMtx;

    StateToken mStateToken;
    time_t mLastUpdateTS;

    std::list<RsFileHash> mDownloadsAtLastCheck;
	std::list<RsFileHash> mUploadsAtLastCheck;
};

} // namespace resource_api
