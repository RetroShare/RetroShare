/*******************************************************************************
 * libresapi/api/TransfersHandler.h                                            *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
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
	void handleSetFileDestinationDirectory(Request& req, Response& resp);
	void handleSetFileDestinationName(Request& req, Response& resp);
	void handleSetFileChunkStrategy(Request& req, Response& resp);

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
