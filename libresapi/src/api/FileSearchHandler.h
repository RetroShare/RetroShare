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
    virtual void notifyTurtleSearchResult(uint32_t search_id, const std::list<TurtleFileInfo>& files);
private:
    void handleWildcard(Request& req, Response& resp);
    void handleCreateSearch(Request& req, Response& resp);

    StateTokenServer* mStateTokenServer;
    RsNotify* mNotify;
    RsTurtle* mTurtle;
    RsFiles* mFiles;

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
