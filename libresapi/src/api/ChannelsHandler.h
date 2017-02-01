#pragma once

#include "ResourceRouter.h"

class RsGxsChannels;

namespace resource_api
{

class ChannelsHandler : public ResourceRouter
{
public:
    explicit ChannelsHandler(RsGxsChannels* channels);

private:
    ResponseTask* handleCreatePost(Request& req, Response& resp);

    RsGxsChannels* mChannels;
};

} // namespace resource_api
