/*******************************************************************************
 * libresapi/api/ResourceRouter.cpp                                            *
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
#include "ResourceRouter.h"



namespace resource_api
{

class TestResource: public ResourceRouter
{
public:
    TestResource()
    {
        addResourceHandler("eins", this, &TestResource::eins);
    }
    ResponseTask* eins(Request& /* req */, Response& /* resp */)
    {
        return 0;
    }
};

ResourceRouter::~ResourceRouter()
{
    std::vector<std::pair<std::string, HandlerBase*> >::iterator vit;
    for(vit = mHandlers.begin(); vit != mHandlers.end(); vit++)
    {
        delete vit->second;
    }
}

ResponseTask* ResourceRouter::handleRequest(Request& req, Response& resp)
{
    std::vector<std::pair<std::string, HandlerBase*> >::iterator vit;
    if(!req.mPath.empty())
    {
        for(vit = mHandlers.begin(); vit != mHandlers.end(); vit++)
        {
            if(vit->first == req.mPath.top())
            {
                req.mPath.pop();

				//Just for GUI benefit
				std::string callbackName;
				req.mStream << makeKeyValueReference("callback_name", callbackName);
				resp.mCallbackName = callbackName;
				//

                return vit->second->handleRequest(req, resp);
            }
        }
    }
    // not found, search for wildcard handler
    for(vit = mHandlers.begin(); vit != mHandlers.end(); vit++)
    {
        if(vit->first == "*")
        {
            // don't pop the path component, because it may contain usefull info for the wildcard handler
            //req.mPath.pop();
            return vit->second->handleRequest(req, resp);
        }
    }
    resp.setFail("ResourceRouter::handleRequest() Error: no handler for this path.");
    return 0;
}

bool ResourceRouter::isNameUsed(std::string name)
{
    std::vector<std::pair<std::string, HandlerBase*> >::iterator vit;
    for(vit = mHandlers.begin(); vit != mHandlers.end(); vit++)
    {
        if(vit->first == name)
        {
            return true;
        }
    }
    return false;
}

} // namespace resource_api
