/*******************************************************************************
 * libresapi/api/ApiServer.h                                                   *
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

#include <retroshare/rsplugin.h>

#include "ApiTypes.h"
#include "PeersHandler.h"
#include "IdentityHandler.h"
#include "ForumHandler.h"
#include "ServiceControlHandler.h"
#include "StateTokenServer.h"
#include "FileSearchHandler.h"
#include "FileSharingHandler.h"
#include "TransfersHandler.h"
#include "LivereloadHandler.h"
#include "TmpBlobStore.h"
#include "ChatHandler.h"

namespace resource_api{

class ApiServerMainModules;

// main entry point for all resource_api calls
// general part of the api server
// should work with any http library or a different transport protocol (e.g. SSH)

// call chain is like this:
// HTTP server -> ApiServer -> different handlers
// or
// GUI -> ApiServer -> different handlers
// multiple clients can use the same ApiServer instance at the same time

// ALL public methods in this class are thread safe
// this allows differen threads to send requests
class ApiServer
{
public:
    ApiServer();
    ~ApiServer();

    class RequestId{
    public:
        RequestId(): done(false), task(0), request(0), response(0){}
        bool operator ==(const RequestId& r){
            const RequestId& l = *this;
            return (l.done==r.done)&&(l.task==r.task)&&(l.request==r.request)&&(l.response&&r.response);
        }
    private:
        friend class ApiServer;
        bool done; // this flag will be set to true, to signal the task id is valid and the task is done
                   // (in case there was no ResponseTask and task was zero)
        ResponseTask* task; // null when the task id is invalid or when there was no task
        Request* request;
        Response* response;
    };

    // process the requestgiven by request and return the response as json string
    // blocks until the request was processed
    std::string handleRequest(Request& request);

    // request and response must stay valid until isRequestDone returns true
    // this method may do some work but it does not block
    RequestId handleRequest(Request& request, Response& response);

    // ticks the request
    // returns true if the request is done or the id is invalid
    // this method may do some work but it does not block
    bool isRequestDone(RequestId id);

    // load the main api modules
    void loadMainModules(const RsPlugInInterfaces& ifaces);

    // allows to add more handlers
    // make sure the livetime of the handlers is longer than the api server
    template <class T>
    void addResourceHandler(std::string name, T* instance, ResponseTask* (T::*callback)(Request& req, Response& resp));
    template <class T>
    void addResourceHandler(std::string name, T* instance, void (T::*callback)(Request& req, Response& resp));

    StateTokenServer* getStateTokenServer(){ return &mStateTokenServer; }
    TmpBlobStore* getTmpBlobStore(){ return &mTmpBlobStore; }

private:
    RsMutex mMtx;
    StateTokenServer mStateTokenServer; // goes first, as others may depend on it
                                        // is always loaded, because it has no dependencies
    LivereloadHandler mLivereloadhandler;
    TmpBlobStore mTmpBlobStore;

    // only pointers here, to load/unload modules at runtime
    ApiServerMainModules* mMainModules; // loaded when RS is started

    ResourceRouter mRouter;

    std::vector<RequestId> mRequests;
};

// implementations
template <class T>
void ApiServer::addResourceHandler(std::string name, T* instance, ResponseTask* (T::*callback)(Request& req, Response& resp))
{
    RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    mRouter.addResourceHandler(name, instance, callback);
}
template <class T>
void ApiServer::addResourceHandler(std::string name, T* instance, void (T::*callback)(Request& req, Response& resp))
{
    RS_STACK_MUTEX(mMtx); // ********** LOCKED **********
    mRouter.addResourceHandler(name, instance, callback);
}

}
