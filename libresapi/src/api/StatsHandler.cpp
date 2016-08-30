#include "StatsHandler.h"
#include "Operators.h"

#include <retroshare/rsconfig.h>
#include <retroshare/rspeers.h>
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

	// DHT/NAT info
	RsConfigNetStatus config;
	rsConfig->getConfigNetStatus(config);
	itemStream << makeKeyValue("dht_active",	config.DHTActive);
	itemStream << makeKeyValue("dht_ok",		config.netDhtOk);
	itemStream << makeKeyValue("dht_size_all",	config.netDhtNetSize);
	itemStream << makeKeyValue("dht_size_rs",	config.netDhtRsNetSize);
	uint32_t netState = rsConfig -> getNetState();
	itemStream << makeKeyValue("nat_state", netState);

	// ok
	resp.setOk();
}

} // namespace resource_api
