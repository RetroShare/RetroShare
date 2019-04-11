/*******************************************************************************
 * unittests/libretroshare/services/status/status_test.cc                      *
 *                                                                             *
 * Copyright 2012      by Robert Fernie    <retroshare.project@gmail.com>      *
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


#include <gtest/gtest.h>

// from librssimulator
#include "testing/IsolatedServiceTester.h"
#include "peer/PeerNode.h"

// from libretroshare
#include "services/p3statusservice.h"
#include "rsitems/rsstatusitems.h"

#define N_PEERS 10

TEST(libretroshare_services, Status_test1)
{
	RsPeerId ownId = RsPeerId::random();
	RsPeerId friendId = RsPeerId::random();

	std::list<RsPeerId> peers;
	peers.push_back(friendId);
	for(int i = 0; i < N_PEERS; i++)
	{
		peers.push_back(RsPeerId::random());
	}

	IsolatedServiceTester tester(ownId, peers);

	// extract bits we need.
	PeerNode *node = tester.getPeerNode();
	//p3PeerMgr *peerMgr = node->getPeerMgr();
	//p3LinkMgr *linkMgr = node->getLinkMgr();
	//p3NetMgr  *netMgr = node->getNetMgr();
	p3ServiceControl *serviceCtrl = node->getServiceControl();	

	// add in service.
	p3StatusService *status = new p3StatusService(serviceCtrl);
	node->AddService(status);
	node->AddPqiServiceMonitor(status);

	tester.addSerialType(new RsStatusSerialiser());

	/**************** Start Test ****************/
	// setup.
	tester.startup(); 
	tester.tick();

	// Expect no packets - as noone is online.
	EXPECT_FALSE(tester.tickUntilPacket(20));

	/*************** Connect Peers **************/
	// bring people online.	
	std::list<RsPeerId> onlineList;
	onlineList.push_back(friendId);
	node->bringOnline(onlineList);

	EXPECT_TRUE(tester.tickUntilPacket(20));
	/*************** First Packet ***************/

	RsItem *item = tester.getPacket();
	EXPECT_TRUE(item != NULL);

	// expecting Discovery 
	RsStatusItem *statusitem = dynamic_cast<RsStatusItem *>(item);
	EXPECT_TRUE(statusitem != NULL);
	EXPECT_TRUE(statusitem->PeerId() == friendId);
	if (statusitem)
	{
		delete statusitem;
	}

	/*************** Reply Packet ***************/
	statusitem = new RsStatusItem();
	statusitem->PeerId(friendId);
	tester.sendPacket(statusitem);

	// expect...
	//EXPECT_TRUE(test.tickUntilPacket(20));

}







