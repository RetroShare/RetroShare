/*******************************************************************************
 * libresapi/api/ApiPluginHandler.cpp                                          *
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
