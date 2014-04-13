#pragma once

// from librssimulator
#include "peer/PeerNode.h"

class	RsGxsIdExchange;
class	RsGxsCircleExchange;
class	GxsTestService;
class 	RsGeneralDataService;
class   RsGxsNetService;

class GxsPeerNode: public PeerNode
{
public:

	GxsPeerNode(const RsPeerId &ownId, const std::list<RsPeerId> &peers, int testMode);
	~GxsPeerNode();

	uint32_t mTestMode;
	std::string mGxsDir;

	// Id and Circle Interfaces. (NULL for now).
	RsGxsIdExchange *mGxsIdService;
	RsGxsCircleExchange *mGxsCircles;

	GxsTestService *mTestService;
        RsGeneralDataService* mTestDs;
        RsGxsNetService* mTestNs;
};





