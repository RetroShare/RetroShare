#pragma once

// from librssimulator
#include "testing/IsolatedServiceTester.h"

class	RsGxsIdExchange;
class	RsGxsCircleExchange;
class	GxsTestService;
class 	RsGeneralDataService;
class   RsGxsNetService;

class GxsIsolatedServiceTester: public IsolatedServiceTester
{
public:

	GxsIsolatedServiceTester(const RsPeerId &ownId, const RsPeerId &friendId, std::list<RsPeerId> peers, int testMode);
	~GxsIsolatedServiceTester();

	uint32_t mTestMode;
	std::string mGxsDir;

	// Id and Circle Interfaces. (NULL for now).
	RsGxsIdExchange *mGxsIdService;
	RsGxsCircleExchange *mGxsCircles;

	GxsTestService *mTestService;
        RsGeneralDataService* mTestDs;
        RsGxsNetService* mTestNs;
};





