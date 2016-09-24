#ifndef STATSHANDLER_H
#define STATSHANDLER_H

/*
 * simple class to output some basic stats about RS
 * like bandwidth, connected peers, ...
 */

#include "ResourceRouter.h"

namespace resource_api
{

class StatsHandler : public ResourceRouter
{
public:
	StatsHandler();

private:
	void handleStatsRequest(Request& req, Response& resp);
};

} // namespace resource_api

#endif // STATSHANDLER_H
