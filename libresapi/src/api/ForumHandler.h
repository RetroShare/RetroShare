#ifndef FORUMHANDLER_H
#define FORUMHANDLER_H

#include "ResourceRouter.h"

class RsGxsForums;

namespace resource_api
{

class ForumHandler : public ResourceRouter
{
public:
    ForumHandler(RsGxsForums* forums);
private:
    RsGxsForums* mRsGxsForums;
    void handleWildcard(Request& req, Response& resp);
};
} // namespace resource_api
#endif // FORUMHANDLER_H
