/*******************************************************************************
 * libresapi/api/ResourceRouter.h                                              *
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
#include <iostream>

namespace resource_api
{
// a base class for routing requests to handler methods
// nothing is thread safe here
class ResourceRouter
{
public:
    virtual ~ResourceRouter();

    // can return NULL, if the request was processed
    // if the Response can not be created immediately,
    // then return a object which implements the ResponseTask interface
    ResponseTask* handleRequest(Request& req, Response& resp);

    template <class T>
    void addResourceHandler(std::string name, T* instance, ResponseTask* (T::*callback)(Request& req, Response& resp));
    template <class T>
    void addResourceHandler(std::string name, T* instance, void (T::*callback)(Request& req, Response& resp));

    bool isNameUsed(std::string name);
private:
    class HandlerBase
    {
    public:
        virtual ~HandlerBase(){}
        virtual ResponseTask* handleRequest(Request& req, Response& resp) = 0;
    };
    template <class T>
    class Handler: public HandlerBase
    {
    public:
        virtual ResponseTask* handleRequest(Request &req, Response &resp)
        {
            return (instance->*method)(req, resp);
        }
        T* instance;
        ResponseTask* (T::*method)(Request& req, Response& resp);
    };
    template <class T>
    class InstantResponseHandler: public HandlerBase
    {
    public:
        virtual ResponseTask* handleRequest(Request &req, Response &resp)
        {
            (instance->*method)(req, resp);
            return 0;
        }
        T* instance;
        void (T::*method)(Request& req, Response& resp);
    };

    std::vector<std::pair<std::string, HandlerBase*> > mHandlers;
};

// the advantage of this approach is:
// the method name is arbitrary, one class can have many different handler methods
// with raw objects the name of the handler method would be fixed, and we would need one class for every handler
// the downside is complicated template magic
template <class T>
void ResourceRouter::addResourceHandler(std::string name, T* instance, ResponseTask* (T::*callback)(Request& req, Response& resp))
{
    if(isNameUsed(name))
    {
        std::cerr << "ResourceRouter::addResourceHandler ERROR: name=" << name << " alerady in use. Not adding new Handler!" << std::endl;
    }
    Handler<T>* handler = new Handler<T>();
    handler->instance = instance;
    handler->method = callback;
    mHandlers.push_back(std::make_pair(name, handler));
}
template <class T>
void ResourceRouter::addResourceHandler(std::string name, T* instance, void (T::*callback)(Request& req, Response& resp))
{
    if(isNameUsed(name))
    {
        std::cerr << "ResourceRouter::addResourceHandler ERROR: name=" << name << " alerady in use. Not adding new Handler!" << std::endl;
    }
    InstantResponseHandler<T>* handler = new InstantResponseHandler<T>();
    handler->instance = instance;
    handler->method = callback;
    mHandlers.push_back(std::make_pair(name, handler));
}

} // namespace resource_api
