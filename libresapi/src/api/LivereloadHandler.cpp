#include "LivereloadHandler.h"

namespace resource_api
{
LivereloadHandler::LivereloadHandler(StateTokenServer *sts):
    mStateTokenServer(sts), mStateToken(sts->getNewToken())
{
    addResourceHandler("*", this, &LivereloadHandler::handleWildcard);
    addResourceHandler("trigger", this, &LivereloadHandler::handleTrigger);
}

void LivereloadHandler::handleWildcard(Request &/*req*/, Response &resp)
{
    resp.mStateToken = mStateToken;
    resp.setOk();
}

void LivereloadHandler::handleTrigger(Request &/*req*/, Response &resp)
{
    mStateTokenServer->replaceToken(mStateToken);
    resp.mStateToken = mStateToken;
    resp.setOk();
}

} // namespace resource_api
