#pragma once

#include "ResourceRouter.h"
#include <retroshare/rsplugin.h>

namespace resource_api
{

// forwards all incoming requests to retroshare plugins
class ApiPluginHandler: public ResourceRouter
{
public:
    ApiPluginHandler(StateTokenServer* statetokenserver, const RsPlugInInterfaces& ifaces);
    virtual ~ApiPluginHandler();

private:
    std::vector<ResourceRouter*> mChildren;
};

} // namespace resource_api
