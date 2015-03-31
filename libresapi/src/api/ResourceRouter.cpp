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
    ResponseTask* eins(Request& req, Response& resp)
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

} // namespace resource_api
