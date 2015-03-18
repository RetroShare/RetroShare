#pragma once

#include "ApiTypes.h"
#include "ResourceRouter.h"

class RsServiceControl;

namespace resource_api
{

class ServiceControlHandler: public ResourceRouter
{
public:
    ServiceControlHandler(RsServiceControl* control);

private:
    RsServiceControl* mRsServiceControl;
    void handleWildcard(Request& req, Response& resp);
};
} // namespace resource_api
