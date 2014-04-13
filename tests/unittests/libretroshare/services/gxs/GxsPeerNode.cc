
// from libretroshare
#include "services/p3statusservice.h"
#include "serialiser/rsstatusitems.h"
#include "gxs/rsgixs.h"
#include "gxs/rsdataservice.h"
#include "gxs/rsgxsnetservice.h"
#include "util/rsdir.h"

// local
#include "GxsPeerNode.h"
#include "gxstestservice.h"

GxsPeerNode::GxsPeerNode(const RsPeerId &ownId, const std::list<RsPeerId> &friends, int testMode)
	:PeerNode(ownId, friends, false),
	mTestMode(testMode),
	mGxsDir("./gxs_unittest/" + ownId.toStdString() + "/"),
	mGxsIdService(NULL),
	mGxsCircles(NULL),
	mTestService(NULL), 
	mTestDs(NULL), 
	mTestNs(NULL)
{ 
	// extract bits we need.
	p3PeerMgr *peerMgr = getPeerMgr();
	p3LinkMgr *linkMgr = getLinkMgr();
	p3NetMgr  *netMgr = getNetMgr();
	RsNxsNetMgr *nxsMgr = getNxsNetMgr();
	p3ServiceControl *serviceCtrl = getServiceControl();	

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
                        mGxsIdService, mGxsCircles);

	AddService(mTestNs);

        //mConfigMgr->addConfiguration("posted.cfg", posted_ns);
	createThread(*mTestService);
        createThread(*mTestNs);

	//node->AddPqiServiceMonitor(status);
}


GxsPeerNode::~GxsPeerNode()
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
	


