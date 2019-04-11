/*******************************************************************************
 * libresapi/api/GxsResponseTask.cpp                                           *
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
#include "GxsResponseTask.h"

#include "Operators.h"

namespace resource_api
{

GxsResponseTask::GxsResponseTask(RsIdentity *id_service, RsTokenService *token_service):
    mIdService(id_service), mTokenService(token_service),
    mDone(false)
{

}

bool GxsResponseTask::doWork(Request &req, Response &resp)
{
    bool ready = true;
    // check if gxs requests are ready
    if(mTokenService && !mWaitingTokens.empty())
    {
        for(std::vector<uint32_t>::iterator vit = mWaitingTokens.begin(); vit != mWaitingTokens.end(); ++vit)
        {
            uint8_t status = mTokenService->requestStatus(*vit);
            if(status != RsTokenService::COMPLETE)
            {
                ready = false;
            }
            if(status == RsTokenService::FAILED)
            {
                std::cerr   << "GxsResponseTask::doWork() Error: token failed. aborting." << std::endl;
                resp.setFail("GxsResponseTask::doWork() Error: token failed.");
                return false; // don't continue
            }
        }
    }

    if(mIdService == 0)
    {
        std::cerr << "GxsResponseTask::doWork() ERROR: constucted with idservice = 0. Fix your code or report this bug." << std::endl;
        resp.setFail("GxsResponseTask::doWork() ERROR: constucted with idservice = 0. Fix your code or report this bug.");
        return false; // don't continue
    }

    // check if we have identities to fetch
    bool more = true;
    while(!mIdentitiesToFetch.empty() && more)
    {
        // there are two methods to fetch identity data:
        // - request gxs group, get token, get group
        // - the direct way where we may have to wait, but identities will cache the result
        // if we need to get many identuties, then we may flush the cache
        // but if we reaquest the groups, no caching is done on the rs side (OS will cache the identities file)
        // it has to be measured what is better
        RsGxsId id = mIdentitiesToFetch.back();
        RsIdentityDetails details;
        if(mIdService->getIdDetails(id, details))
        {
            mIdentitiesToFetch.pop_back();
            mIdentityDetails.push_back(details);
        }
        else
        {
            more = false; // pause when an id failed, to give the service time tim fetch the data
            ready = false;
            // TODO: remove identities which failed many times from list, to avoid blocking when ids fail
        }
    }
    if(!ready)
        return true; // want to continue later

    mWaitingTokens.clear();
    mIdentitiesToFetch.clear();
    gxsDoWork(req, resp);

    if(mDone) return false;
    else return true;
}

void GxsResponseTask::addWaitingToken(uint32_t token)
{
    if(mTokenService)
        mWaitingTokens.push_back(token);
    else
        std::cerr << "GxsResponseTask::addWaitingToken() ERROR: constructed with tokenservice=0. Unable to handle token processing. Fix your code or report this bug." << std::endl;
}

void GxsResponseTask::done()
{
    mDone = true;
}

void GxsResponseTask::requestGxsId(RsGxsId id)
{
    mIdentitiesToFetch.push_back(id);
}

void GxsResponseTask::streamGxsId(RsGxsId id, StreamBase &stream)
{
    // will see if this works or if we have to use an index
    for(std::vector<RsIdentityDetails>::iterator vit = mIdentityDetails.begin();
        vit != mIdentityDetails.end(); ++vit)
    {
        if(vit->mId == id)
        {
            stream << makeKeyValueReference("id", id)
                   << makeKeyValueReference("gxs_id", id)
                   << makeKeyValueReference("flags", vit->mFlags)
                   << makeKeyValueReference("name", vit->mNickname);
            return;
        }
    }
}

std::string GxsResponseTask::getName(RsGxsId id)
{
    for(std::vector<RsIdentityDetails>::iterator vit = mIdentityDetails.begin();
        vit != mIdentityDetails.end(); ++vit)
    {
        if(vit->mId == id)
            return vit->mNickname;
    }
    std::cerr << "Warning: identity not found in GxsResponseTask::getName(). This is probably a bug. You must call GxsResponseTask::requestGxsId() before you can get the name." << std::endl;
    return "";
}


} // namespace resource_api
