
// local
#include "GxsPairServiceTester.h"
#include "GxsPeerNode.h"
#include "gxstestservice.h"

// libretroshare
#include "serialiser/rsnxsitems.h"

GxsPairServiceTester::GxsPairServiceTester(const RsPeerId &peerId1, const RsPeerId &peerId2, int testMode)
	:SetServiceTester()
{ 
	// setup stuff.
	addSerialType(new RsNxsSerialiser(RS_SERVICE_GXS_TYPE_TEST));

	std::list<RsPeerId> friendList1, friendList2;
	friendList1.push_back(peerId2);
	friendList2.push_back(peerId1);
	
	GxsPeerNode *n1 = new GxsPeerNode(peerId1, friendList1, testMode);
	GxsPeerNode *n2 = new GxsPeerNode(peerId2, friendList2, testMode);

	addNode(peerId1, n1);
	addNode(peerId2, n2);

	startup();
	tick();

	bringOnline(peerId1, friendList1);
	bringOnline(peerId2, friendList2);
}


GxsPairServiceTester::~GxsPairServiceTester()
{
	return;
}
	

GxsPeerNode *GxsPairServiceTester::getGxsPeerNode(const RsPeerId &id)
{
	return (GxsPeerNode *) getPeerNode(id);
}


void GxsPairServiceTester::createGroup(const RsPeerId &id, const std::string &name)
{
        /* create a couple of groups */
        GxsTestService *testService = getGxsPeerNode(id)->mTestService;
        RsTokenService *tokenService = testService->RsGenExchange::getTokenService();

        RsTestGroup grp1;
        grp1.mMeta.mGroupName = name;
        grp1.mTestString = "testString";
        uint32_t token1;
                
        testService->submitTestGroup(token1, grp1);
        while(tokenService->requestStatus(token1) != RsTokenService::GXS_REQUEST_V2_STATUS_COMPLETE)
        {
                tick();
                sleep(1);
        }
}

