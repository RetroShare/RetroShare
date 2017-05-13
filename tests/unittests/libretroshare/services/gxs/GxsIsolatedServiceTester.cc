
// from librssimulator
#include "testing/IsolatedServiceTester.h"
#include "peer/PeerNode.h"

// from libretroshare
#include "services/p3statusservice.h"
#include "rsitems/rsstatusitems.h"
#include "gxs/rsgixs.h"
#include "gxs/rsdataservice.h"
#include "gxs/rsgxsnetservice.h"
#include "util/rsdir.h"

// local
#include "GxsIsolatedServiceTester.h"
#include "gxstestservice.h"

GxsIsolatedServiceTester::GxsIsolatedServiceTester(const RsPeerId &ownId, const RsPeerId &friendId, 
				std::list<RsPeerId> peers, int testMode)
	:IsolatedServiceTester(ownId, peers),
	mTestMode(testMode),
	mGxsDir("./gxs_unittest/"),
	mGxsIdService(NULL),
	mGxsCircles(NULL),
	mTestService(NULL), 
	mTestDs(NULL), 
	mTestNs(NULL)
{ 
	// extract bits we need.
	PeerNode *node = getPeerNode();
	//p3PeerMgr *peerMgr = node->getPeerMgr();
	//p3LinkMgr *linkMgr = node->getLinkMgr();
	//p3NetMgr  *netMgr = node->getNetMgr();
	RsNxsNetMgr *nxsMgr = node->getNxsNetMgr();
	//p3ServiceControl *serviceCtrl = node->getServiceControl();

	// Create Service for Testing.
	// Specific Testing service here.
        RsDirUtil::checkCreateDirectory(mGxsDir);

        std::set<std::string> filesToKeep;
        RsDirUtil::cleanupDirectory(mGxsDir, filesToKeep);

        mTestDs = new RsDataService(mGxsDir, "test_db",
                        RS_SERVICE_GXS_TYPE_TEST,
                        NULL, "testPasswd");

        mTestService = new GxsTestService(mTestDs, NULL, mGxsIdService, testMode);

        mTestNs = new RsGxsNetService(
                        RS_SERVICE_GXS_TYPE_TEST, mTestDs, nxsMgr,
                        mTestService, mTestService->getServiceInfo(),
                        NULL, mGxsCircles);

	node->AddService(mTestNs);

        //mConfigMgr->addConfiguration("posted.cfg", posted_ns);
    mTestService->start();
        mTestNs->start();

	//node->AddPqiServiceMonitor(status);
	addSerialType(new RsNxsSerialiser(RS_SERVICE_GXS_TYPE_TEST));

	startup();
	tick();

	std::list<RsPeerId> onlineList;
	onlineList.push_back(friendId);
	node->bringOnline(onlineList);
}


GxsIsolatedServiceTester::~GxsIsolatedServiceTester()
{
	mTestService->join();
        mTestNs->join();

	delete mTestNs;
	delete mTestService;
	// this is deleted somewhere else?
	//delete mTestDs;

        std::set<std::string> filesToKeep;
        RsDirUtil::cleanupDirectory(mGxsDir, filesToKeep);
}
	


