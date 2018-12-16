/*******************************************************************************
 * unittests/libretroshare/services/gxs/GxsIsolatedServiceTester.cc            *
 *                                                                             *
 * Copyright 2014      by Robert Fernie    <retroshare.project@gmail.com>      *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 ******************************************************************************/

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
	


