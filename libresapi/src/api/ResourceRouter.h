#pragma once

#include "ApiTypes.h"

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
    Handler<T>* handler = new Handler<T>();
    handler->instance = instance;
    handler->method = callback;
    mHandlers.push_back(std::make_pair(name, handler));
}
template <class T>
void ResourceRouter::addResourceHandler(std::string name, T* instance, void (T::*callback)(Request& req, Response& resp))
{
    InstantResponseHandler<T>* handler = new InstantResponseHandler<T>();
    handler->instance = instance;
    handler->method = callback;
    mHandlers.push_back(std::make_pair(name, handler));
}

} // namespace resource_api
