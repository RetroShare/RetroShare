#include "ApiPluginHandler.h"

namespace resource_api
{

ApiPluginHandler::ApiPluginHandler(StateTokenServer* statetokenserver, const RsPlugInInterfaces& ifaces)
{
    for(int i = 0; i < ifaces.mPluginHandler->nbPlugins(); i++)
    {
        RsPlugin* plugin = ifaces.mPluginHandler->plugin(i);
        // if plugin is not loaded, pointer is null
        if(plugin == 0)
            continue;
        std::string entrypoint;
        ResourceRouter* child = plugin->new_resource_api_handler(ifaces, statetokenserver, entrypoint);
        if(child != 0)
        {
            mChildren.push_back(child);
            if(isNameUsed(entrypoint))
            {
                std::cerr << "Cannot add plugin api entry point with name=" << entrypoint << ", becaus ethis name is already in use!" << std::endl;
            }
            else
            {
                std::cerr << "Added libresapi plugin with entrypoint " << entrypoint << std::endl;
                addResourceHandler(entrypoint, child, &ResourceRouter::handleRequest);
            }
        }
    }
}

ApiPluginHandler::~ApiPluginHandler()
{
    for(std::vector<ResourceRouter*>::iterator vit = mChildren.begin(); vit != mChildren.end(); ++vit)
    {
        delete *vit;
    }
    mChildren.clear();
}

} // namespace resource_api
