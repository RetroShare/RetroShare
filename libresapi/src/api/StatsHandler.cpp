#include "StatsHandler.h"
#include "Operators.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsconfig.h>
#include <pqi/authssl.h>

namespace resource_api
{

StatsHandler::StatsHandler()
{
	addResourceHandler("*", this, &StatsHandler::handleStatsRequest);
}

void StatsHandler::handleStatsRequest(Request &/*req*/, Response &resp)
{
	StreamBase& itemStream = resp.mDataStream.getStreamToMember();

	// location info
	itemStream << makeKeyValue("name", rsPeers->getGPGName(rsPeers->getGPGOwnId()));
	itemStream << makeKeyValue("location", AuthSSL::getAuthSSL()->getOwnLocation());

	// peer info
	unsigned int all, online;
	rsPeers->getPeerCount(&all, &online, false);
	itemStream << makeKeyValue("peers_all", all);
	itemStream << makeKeyValue("peers_connected", online);

	// bandwidth info
	float downKb, upKb;
	rsConfig->GetCurrentDataRates(downKb, upKb);
	itemStream << makeKeyValue("bandwidth_up_kb", (double)upKb);
	itemStream << makeKeyValue("bandwidth_down_kb", (double)downKb);

	resp.setOk();
}

} // namespace resource_api
