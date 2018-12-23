/*******************************************************************************
 * libresapi/api/GxsResponseTask.h                                             *
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

#include "ApiTypes.h"
#include <retroshare/rsidentity.h>

namespace resource_api
{
// parent class for all responses that use the gxs backend
// this class implements the polling for gxs-tokens
// child classes pass gxs tokens to this class

// question: should this class be able to handle tokens from different services?
// then we would have to store a pointer to the token service for ever token
class GxsResponseTask: public ResponseTask
{
public:
    // token service is allowed to be null if no token functions are wanted
    GxsResponseTask(RsIdentity* id_service, RsTokenService* token_service = 0);
    virtual bool doWork(Request &req, Response& resp);

protected:
    // this method gets called when all the pending tokens have either status ok or fail
    // (= when the requests for these tokens are processed)
    // how will the child class find out if a request failed?
    // idea: don't call gxsDoWork() when a request failed, instead set the api response to fail
    // con: then the child class has no way to tell the outside world which request failed
    // pro: child class gets simpler, because no special error handling is required
    // implement this in a child class
    virtual void gxsDoWork(Request& req, Response& resp) = 0;

    // call this to wait for tokens before the next call to gxsDoWork()
    void addWaitingToken(uint32_t token);
    // call this to end the task
    void done();
    // request name for gxs id
    void requestGxsId(RsGxsId id);
    // call stream function in the next cycle, then the names are available
    void streamGxsId(RsGxsId id, StreamBase& stream);
    std::string getName(RsGxsId id);
private:
    RsIdentity* mIdService;
    RsTokenService* mTokenService;

    std::vector<uint32_t> mWaitingTokens;
    bool mDone;

    std::vector<RsGxsId> mIdentitiesToFetch;
    std::vector<RsIdentityDetails> mIdentityDetails;
};

} // namespace resource_api
