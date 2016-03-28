#pragma once

#include "ResourceRouter.h"

class RsGxsChannels;

namespace resource_api
{

class ChannelsHandler : public ResourceRouter
{
public:
    ChannelsHandler(RsGxsChannels* channels);

private:
    ResponseTask* handleCreatePost(Request& req, Response& resp);

    RsGxsChannels* mChannels;
};

} // namespace resource_api
