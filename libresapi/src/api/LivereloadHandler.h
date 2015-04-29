#pragma once

#include "ResourceRouter.h"
#include "StateTokenServer.h"

namespace resource_api
{

// very simple livereload system, integrated into the existing state token system
// the response to / is only a statetoken
// if /trigger is called, then the state token is invalidaten and replaced wiht a new one
class LivereloadHandler: public ResourceRouter
{
public:
    LivereloadHandler(StateTokenServer* sts);

private:
    void handleWildcard(Request& req, Response& resp);
    void handleTrigger(Request& req, Response& resp);
    StateTokenServer* mStateTokenServer;
    StateToken mStateToken;
};
} // namespace resource_api
