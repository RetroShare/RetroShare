/*******************************************************************************
 * libresapi/api/FileSearchHandler.h                                           *
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

#include <retroshare/rsnotify.h>
#include <retroshare/rsturtle.h>
#include <retroshare/rsfiles.h>

namespace resource_api
{

class FileSearchHandler: public ResourceRouter, NotifyClient
{
public:
    FileSearchHandler(StateTokenServer* sts, RsNotify* notify, RsTurtle* turtle, RsFiles* files);
    virtual ~FileSearchHandler();

    // from NotifyClient
    virtual void notifyTurtleSearchResult(const RsPeerId &pid, uint32_t search_id, const std::list<TurtleFileInfo>& files);
private:
    void handleWildcard(Request& req, Response& resp);
	void handleCreateSearch(Request& req, Response& resp);
	void handleGetSearchResult(Request& req, Response& resp);

    StateTokenServer* mStateTokenServer;
    RsNotify* mNotify;
	//RsTurtle* mTurtle;
	RsFiles* mRsFiles;

    class Search{
    public:
        StateToken mStateToken;
        std::string mSearchString; // extra service: store the search string
        std::list<FileDetail> mResults;
        // a set for fast deduplication lookup
        std::set<RsFileHash> mHashes;
    };

    RsMutex mMtx;
    StateToken mSearchesStateToken;
    std::map<uint32_t, Search> mSearches; // mutex protected
};

} // namespace resource_api
