#pragma once

#include <retroshare/rsplugin.h>

#include "ApiTypes.h"
#include "PeersHandler.h"
#include "IdentityHandler.h"
#ifdef FIXME
#include "WallHandler.h"
#endif
#include "ServiceControlHandler.h"
#include "StateTokenServer.h"
#include "FileSearchHandler.h"
#include "TransfersHandler.h"

namespace resource_api{

class ApiServerMainModules;
class ApiServerWallModule;

// main entry point for all resource api calls

// call chain is like this:
// Wt -> ApiServerWt -> ApiServer -> different handlers
// later i want to replace the parts with Wt with something else
// (Wt is a too large framework, a simple http server would be enough)
// maybe use libmicrohttpd
// the other use case for this api is a qt webkit view
// this works without html, the webkitview calls directly into our c++ code

// general part of the api server
// should work with any http library or a different transport protocol
class ApiServer
{
public:
    ApiServer();
    ~ApiServer();

    // it is currently hard to separate into http and non http stuff
    // mainly because the http path is used in the api
    // this has to change later
    // for now let the http part make the request object
    // and the general apiserver part makes the response
    std::string handleRequest(Request& request);

    // load the main api modules
    void loadMainModules(const RsPlugInInterfaces& ifaces);

    // only after rswall was started!
#ifdef FIXME
    void loadWallModule(const RsPlugInInterfaces& ifaces, RsWall::RsWall* wall);
#endif

    // allows to add more handlers
    // make sure the livetime of the handlers is longer than the api server
    template <class T>
    void addResourceHandler(std::string name, T* instance, ResponseTask* (T::*callback)(Request& req, Response& resp));
    template <class T>
    void addResourceHandler(std::string name, T* instance, void (T::*callback)(Request& req, Response& resp));

    StateTokenServer* getStateTokenServer(){ return &mStateTokenServer; }

private:
    StateTokenServer mStateTokenServer; // goes first, as others may depend on it
                                        // is always loaded, because it has no dependencies

    // only pointers here, to load/unload modules at runtime
    ApiServerMainModules* mMainModules; // loaded when RS is started
    ApiServerWallModule* mWallModule; // only loaded in rssocialnet plugin

    ResourceRouter mRouter;
};

// implementations
template <class T>
void ApiServer::addResourceHandler(std::string name, T* instance, ResponseTask* (T::*callback)(Request& req, Response& resp))
{
    mRouter.addResourceHandler(name, instance, callback);
}
template <class T>
void ApiServer::addResourceHandler(std::string name, T* instance, void (T::*callback)(Request& req, Response& resp))
{
    mRouter.addResourceHandler(name, instance, callback);
}

}
