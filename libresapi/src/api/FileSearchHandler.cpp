/*******************************************************************************
 * libresapi/api/FileSearchHandler.cpp                                         *
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
#include <time.h>

#include "FileSearchHandler.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsexpr.h>
#include <sstream>

#include "Operators.h"

namespace resource_api
{

FileSearchHandler::FileSearchHandler(StateTokenServer *sts, RsNotify *notify, RsTurtle */*turtle*/, RsFiles *files):
    mStateTokenServer(sts), mNotify(notify)/*, mTurtle(turtle)*/, mRsFiles(files),
    mMtx("FileSearchHandler")
{
    mNotify->registerNotifyClient(this);
    addResourceHandler("*", this, &FileSearchHandler::handleWildcard);
	addResourceHandler("create_search", this, &FileSearchHandler::handleCreateSearch);
	addResourceHandler("get_search_result", this, &FileSearchHandler::handleGetSearchResult);

    mSearchesStateToken = mStateTokenServer->getNewToken();
}

FileSearchHandler::~FileSearchHandler()
{
    mNotify->unregisterNotifyClient(this);
    mStateTokenServer->discardToken(mSearchesStateToken);
}

void FileSearchHandler::notifyTurtleSearchResult(const RsPeerId& /*pid*/,uint32_t search_id, const std::list<TurtleFileInfo>& files)
{
	RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
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
			RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
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
				        << makeKeyValueReference("path", fd.path)
				        << makeKeyValue("peer_id", fd.id.toStdString())
                        << makeKeyValueReference("size", size)
				        << makeKeyValueReference("rank", fd.rank)
				        << makeKeyValueReference("age", fd.age);
            }
        }
    }
    else
    {
        // list searches
		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
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
        fd.age 	= time(NULL) - dir.mtime;
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

    RsRegularExpression::NameExpression exprs(RsRegularExpression::ContainsAllStrings,words,true) ;
    RsRegularExpression::LinearizedExpression lin_exp ;
    exprs.linearize(lin_exp) ;

    uint32_t search_id = RSRandom::random_u32();
    if(distant)
    {
        // i have no idea what the reasons for two different search modes are
        // rs-gui does it, so do we
        if(words.size() == 1)
            search_id = rsFiles->turtleSearch(words.front());
        else
            search_id = rsFiles->turtleSearch(lin_exp);
    }

    std::list<FileDetail> results;
    if(local)
    {
        std::list<DirDetails> local_results;
		mRsFiles->SearchBoolExp(&exprs, local_results, RS_FILE_HINTS_LOCAL);

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
		mRsFiles->SearchBoolExp(&exprs, remote_results, RS_FILE_HINTS_REMOTE);
        for(std::list<DirDetails>::iterator lit = remote_results.begin(); lit != remote_results.end(); ++lit)
        {
            FileDetail fd;
            if(dirDetailToFileDetail(*lit, fd))
                results.push_back(fd);
        }
    }

    {
		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********

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

void FileSearchHandler::handleGetSearchResult(Request& req, Response& resp)
{
	std::string search_id;
	req.mStream << makeKeyValueReference("search_id", search_id);

	if(search_id.size() != 8)
	{
		resp.setFail("Error: id has wrong size, should be 8 characters");
		return;
	}

	uint32_t id = 0;
	for(uint8_t i = 0; i < 8; i++)
	{
		id += (uint32_t(search_id[i]-'A')) << (i*4);
	}

	{
		RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
		std::map<uint32_t, Search>::iterator mit = mSearches.find(id);
		if(mit == mSearches.end())
		{
			resp.setFail("Error: search id invalid");
			return;
		}

		Search& search = mit->second;
		resp.mStateToken = search.mStateToken;
		resp.mDataStream.getStreamToMember();

		RsPgpId ownId = rsPeers->getGPGOwnId();
		for(std::list<FileDetail>::iterator lit = search.mResults.begin(); lit != search.mResults.end(); ++lit)
		{
			FileDetail& fd = *lit;
			bool isFriend = rsPeers->isFriend(fd.id);
			bool isOwn = false;
			if(ownId == rsPeers->getGPGId(fd.id))
				isOwn = true;

			double size = fd.size;
			resp.mDataStream.getStreamToMember()
			        << makeKeyValueReference("id", fd.hash)
			        << makeKeyValueReference("name", fd.name)
			        << makeKeyValueReference("hash", fd.hash)
			        << makeKeyValueReference("path", fd.path)
			        << makeKeyValue("peer_id", fd.id.toStdString())
			        << makeKeyValueReference("is_friends", isFriend)
			        << makeKeyValueReference("is_own", isOwn)
			        << makeKeyValueReference("size", size)
			        << makeKeyValueReference("rank", fd.rank)
			        << makeKeyValueReference("age", fd.age);
		}
	}
	resp.setOk();
}

} // namespace resource_api
