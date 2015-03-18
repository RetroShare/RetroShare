#include "FileSearchHandler.h"

#include <retroshare/rsexpr.h>
#include <sstream>

#include "Operators.h"

namespace resource_api
{

FileSearchHandler::FileSearchHandler(StateTokenServer *sts, RsNotify *notify, RsTurtle *turtle, RsFiles *files):
    mStateTokenServer(sts), mNotify(notify), mTurtle(turtle), mFiles(files),
    mMtx("FileSearchHandler")
{
    mNotify->registerNotifyClient(this);
    addResourceHandler("*", this, &FileSearchHandler::handleWildcard);
    addResourceHandler("create_search", this, &FileSearchHandler::handleCreateSearch);

    mSearchesStateToken = mStateTokenServer->getNewToken();
}

FileSearchHandler::~FileSearchHandler()
{
    mNotify->unregisterNotifyClient(this);
    mStateTokenServer->discardToken(mSearchesStateToken);
}

void FileSearchHandler::notifyTurtleSearchResult(uint32_t search_id, const std::list<TurtleFileInfo>& files)
{
    RsStackMutex stackMtx(mMtx); // ********** STACK LOCKED MTX **********
    std::map<uint32_t, Search>::iterator mit = mSearches.find(search_id);
    if(mit == mSearches.end())
        return;

    Search& search = mit->second;
    // set to a limit of 100 for now, can have more when we have pagination
    std::list<TurtleFileInfo>::const_iterator lit = files.begin();
    bool changed = false;
    while(search.mResults.size() < 100 && lit != files.end())
    {
        if(search.mHashes.find(lit->hash) == search.mHashes.end())
        {
            changed = true;
            FileDetail det ;
            det.rank = 0 ;
            det.age = 0 ;
            det.name = (*lit).name;
            det.hash = (*lit).hash;
            det.size = (*lit).size;
            det.id.clear();
            search.mResults.push_back(det);
            search.mHashes.insert(lit->hash);
        }
        lit++;
    }
    if(changed)
    {
        mStateTokenServer->discardToken(search.mStateToken);
        search.mStateToken = mStateTokenServer->getNewToken();
    }
}

// TODO: delete searches
void FileSearchHandler::handleWildcard(Request &req, Response &resp)
{
    if(!req.mPath.empty())
    {
        std::string str = req.mPath.top();
        req.mPath.pop();

        if(str.size() != 8)
        {
            resp.setFail("Error: id has wrong size, should be 8 characters");
            return;
        }
        uint32_t id = 0;
        // TODO fix this
        for(uint8_t i = 0; i < 8; i++)
        {
            id += (uint32_t(str[i]-'A')) << (i*4);
        }

        {
            RsStackMutex stackMtx(mMtx); // ********** STACK LOCKED MTX **********
            std::map<uint32_t, Search>::iterator mit = mSearches.find(id);
            if(mit == mSearches.end())
            {
                resp.setFail("Error: search id invalid");
                return;
            }

            Search& search = mit->second;
            resp.mStateToken = search.mStateToken;
            resp.mDataStream.getStreamToMember();
            for(std::list<FileDetail>::iterator lit = search.mResults.begin(); lit != search.mResults.end(); ++lit)
            {
                FileDetail& fd = *lit;
                double size = fd.size;
                resp.mDataStream.getStreamToMember()
                        << makeKeyValueReference("id", fd.hash)
                        << makeKeyValueReference("name", fd.name)
                        << makeKeyValueReference("hash", fd.hash)
                        << makeKeyValueReference("size", size)
                        << makeKeyValueReference("rank", fd.rank);
            }
        }
    }
    else
    {
        // list searches
        RsStackMutex stackMtx(mMtx); // ********** STACK LOCKED MTX **********
        resp.mDataStream.getStreamToMember();
        for(std::map<uint32_t, Search>::iterator mit = mSearches.begin(); mit != mSearches.end(); ++mit)
        {
            uint32_t id = mit->first;
            std::string idstr;
            // how many times do i have to write int to string conversation?
            // a library should do this
            for(uint8_t i = 0; i < 8; i++)
            {
                char c = ((id>>(i*4))&0xF)+'A';
                idstr += c;
            }
            resp.mDataStream.getStreamToMember()
                << makeKeyValueReference("id", idstr)
                << makeKeyValueReference("search_string", mit->second.mSearchString);
        }
        resp.mStateToken = mSearchesStateToken;
        resp.setOk();
    }
}

static bool dirDetailToFileDetail(const DirDetails& dir, FileDetail& fd)
{
    if (dir.type == DIR_TYPE_FILE)
    {
        fd.id	= dir.id;
        fd.name = dir.name;
        fd.hash = dir.hash;
        fd.path = dir.path;
        fd.size = dir.count;
        fd.age 	= dir.age;
        fd.rank = 0;
        return true;
    }
    else
        return false;
}

// see retroshare-gui/src/gui/Searchdialog.cpp
void FileSearchHandler::handleCreateSearch(Request &req, Response &resp)
{
    bool distant = false;// distant involves sending data, so have it off by default for privacy
    bool local = true;
    bool remote = true;
    std::string search_string;
    req.mStream << makeKeyValueReference("distant", distant)
                << makeKeyValueReference("local", local)
                << makeKeyValueReference("remote", remote)
                << makeKeyValueReference("search_string", search_string);

    std::istringstream iss(search_string);
    std::list<std::string> words;
    std::string s;
    while (std::getline(iss, s, ' ')) {
        std::cout << s << std::endl;
        words.push_back(s);
    }

    if(words.empty())
    {
        resp.setFail("Error: no search string given");
        return;
    }

    NameExpression exprs(ContainsAllStrings,words,true) ;
    LinearizedExpression lin_exp ;
    exprs.linearize(lin_exp) ;

    uint32_t search_id = RSRandom::random_u32();
    if(distant)
    {
        // i have no idea what the reasons for two different search modes are
        // rs-gui does it, so do we
        if(words.size() == 1)
            search_id = mTurtle->turtleSearch(words.front());
        else
            search_id = mTurtle->turtleSearch(lin_exp);
    }

    std::list<FileDetail> results;
    if(local)
    {
        std::list<DirDetails> local_results;
        rsFiles->SearchBoolExp(&exprs, local_results, RS_FILE_HINTS_LOCAL);

        for(std::list<DirDetails>::iterator lit = local_results.begin(); lit != local_results.end(); ++lit)
        {
            FileDetail fd;
            if(dirDetailToFileDetail(*lit, fd))
                results.push_back(fd);
        }
    }
    if(remote)
    {
        std::list<DirDetails> remote_results;
        rsFiles->SearchBoolExp(&exprs, remote_results, RS_FILE_HINTS_REMOTE);
        for(std::list<DirDetails>::iterator lit = remote_results.begin(); lit != remote_results.end(); ++lit)
        {
            FileDetail fd;
            if(dirDetailToFileDetail(*lit, fd))
                results.push_back(fd);
        }
    }

    {
        RsStackMutex stackMtx(mMtx); // ********** STACK LOCKED MTX **********

        Search& search = mSearches[search_id];
        search.mStateToken = mStateTokenServer->getNewToken();
        search.mSearchString = search_string;
        search.mResults.swap(results);

        mStateTokenServer->discardToken(mSearchesStateToken);
        mSearchesStateToken = mStateTokenServer->getNewToken();
    }

    std::string idstr;
    // how many times do i have to write int to string conversation?
    // a library should do this
    for(uint8_t i = 0; i < 8; i++)
    {
        char c = ((search_id>>(i*4))&0xF)+'A';
        idstr += c;
    }
    resp.mDataStream << makeKeyValueReference("search_id", idstr);
    resp.setOk();
}

} // namespace resource_api
