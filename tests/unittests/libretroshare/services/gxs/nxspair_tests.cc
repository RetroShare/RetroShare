/*******************************************************************************
 * unittests/libretroshare/services/gxs/nxspair_test.cc                        *
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

// from libretroshare
#include "rsitems/rsnxsitems.h"

// local
#include "GxsPairServiceTester.h"
#include "GxsPeerNode.h"
#include "gxstestservice.h"

/* Here we want to test the most basic NXS interactions.
 * we have a GXS service, with no AuthorIDs or Circles.
 *
 */

/*
 * Test 1: nothing in Groups.... only sync packets.
 *
 * This test is rather slow - should speed it up.
 */

//TEST(libretroshare_services, GxsNxsPairExchange1)
TEST(libretroshare_services, DISABLED_GxsNxsPairExchange1)
{
	RsPeerId p1 = RsPeerId::random();
	RsPeerId p2 = RsPeerId::random();
	int testMode = 0;

	GxsPairServiceTester tester(p1, p2, testMode, false);
	GxsPeerNode *peerNode1 = tester.getGxsPeerNode(p1);
	GxsPeerNode *peerNode2 = tester.getGxsPeerNode(p2);

	// we only care about the transaction going one way ...
	// so drop SyncGrp packets from p2 -> p1.

	SetFilter &dropFilter = tester.getDropFilter();
	dropFilter.setFilterMode(SetFilter::FILTER_PARAMS);
	dropFilter.setUseSource(true);
	dropFilter.addSource(p2);
	{
		RsNxsSyncGrpItem *syncGrp = new RsNxsSyncGrpItem(RS_SERVICE_GXS_TYPE_TEST);
		dropFilter.setUseFullTypes(true);
		dropFilter.addFullType(syncGrp->PacketId());
	}

	// these are currently slow operations.

	RsGxsGroupId p2GroupId1, p2GroupId2;
	RsGxsCircleId nullCircleId;
	RsGxsId	      nullAuthorId;
	EXPECT_TRUE(peerNode2->createGroup("group1", GXS_CIRCLE_TYPE_PUBLIC, nullCircleId, nullAuthorId, p2GroupId1));
	EXPECT_TRUE(peerNode2->createGroup("group2", GXS_CIRCLE_TYPE_PUBLIC, nullCircleId, nullAuthorId, p2GroupId2));
	std::cerr << "p2->group1 id: " << p2GroupId1;
	std::cerr << std::endl;
	std::cerr << "p2->group2 id: " << p2GroupId2;
	std::cerr << std::endl;

	int counter = 0;
	while((counter < 30))
	{
		counter++;
		tester.tick();
		sleep(1);
	}

	std::list<RsGxsGroupId> p1GroupList;
	std::list<RsGxsGroupId> p2GroupList;
	EXPECT_TRUE(peerNode1->getGroupList(p1GroupList));
	EXPECT_TRUE(peerNode2->getGroupList(p2GroupList));
	EXPECT_TRUE(p1GroupList.size() == 2);
	EXPECT_TRUE(p2GroupList.size() == 2);
	EXPECT_TRUE(p1GroupList.end() != std::find(p1GroupList.begin(), p1GroupList.end(), p2GroupId1));
	EXPECT_TRUE(p1GroupList.end() != std::find(p1GroupList.begin(), p1GroupList.end(), p2GroupId2));
	EXPECT_TRUE(p2GroupList.end() != std::find(p2GroupList.begin(), p2GroupList.end(), p2GroupId1));
	EXPECT_TRUE(p2GroupList.end() != std::find(p2GroupList.begin(), p2GroupList.end(), p2GroupId2));

	std::vector<RsTestGroup> p1Groups;
	std::vector<RsTestGroup> p2Groups;
	EXPECT_TRUE(peerNode1->getGroups(p1Groups));
	EXPECT_TRUE(peerNode2->getGroups(p2Groups));
	EXPECT_TRUE(p1Groups.size() == 2);
	EXPECT_TRUE(p2Groups.size() == 2);

	tester.PrintCapturedPackets();
}


/***
 * Test 2 includes ID & Circle Services.
 **/

TEST(libretroshare_services, DISABLED_GxsNxsPairExchange2)
//TEST(libretroshare_services, GxsNxsPairExchange2)
{
	RsPeerId p1 = RsPeerId::random();
	RsPeerId p2 = RsPeerId::random();
	int testMode = 0;

	GxsPairServiceTester tester(p1, p2, testMode, true);
	GxsPeerNode *peerNode1 = tester.getGxsPeerNode(p1);
	GxsPeerNode *peerNode2 = tester.getGxsPeerNode(p2);

	// we only care about the transaction going one way ...
	// so drop SyncGrp packets from p2 -> p1.

	SetFilter &dropFilter = tester.getDropFilter();
	dropFilter.setFilterMode(SetFilter::FILTER_PARAMS);
	dropFilter.setUseSource(true);
	dropFilter.addSource(p2);
	{
		RsNxsSyncGrpItem *syncGrp = new RsNxsSyncGrpItem(RS_SERVICE_GXS_TYPE_TEST);
		dropFilter.setUseFullTypes(true);
		dropFilter.addFullType(syncGrp->PacketId());
	}

	// these are currently slow operations.
	RsGxsGroupId p2GroupId1, p2GroupId2;
	RsGxsCircleId nullCircleId;
	RsGxsId	      nullAuthorId;
	EXPECT_TRUE(peerNode2->createGroup("group1", GXS_CIRCLE_TYPE_PUBLIC, nullCircleId, nullAuthorId, p2GroupId1));
	EXPECT_TRUE(peerNode2->createGroup("group2", GXS_CIRCLE_TYPE_PUBLIC, nullCircleId, nullAuthorId, p2GroupId2));
	std::cerr << "p2->group1 id: " << p2GroupId1;
	std::cerr << std::endl;
	std::cerr << "p2->group2 id: " << p2GroupId2;
	std::cerr << std::endl;


	int counter = 0;
	while((counter < 60))
	{
		counter++;
		tester.tick();
		sleep(1);
	}

	std::list<RsGxsGroupId> p1GroupList;
	std::list<RsGxsGroupId> p2GroupList;
	EXPECT_TRUE(peerNode1->getGroupList(p1GroupList));
	EXPECT_TRUE(peerNode2->getGroupList(p2GroupList));
	EXPECT_TRUE(p1GroupList.size() == 2);
	EXPECT_TRUE(p2GroupList.size() == 2);
	EXPECT_TRUE(p1GroupList.end() != std::find(p1GroupList.begin(), p1GroupList.end(), p2GroupId1));
	EXPECT_TRUE(p1GroupList.end() != std::find(p1GroupList.begin(), p1GroupList.end(), p2GroupId2));
	EXPECT_TRUE(p2GroupList.end() != std::find(p2GroupList.begin(), p2GroupList.end(), p2GroupId1));
	EXPECT_TRUE(p2GroupList.end() != std::find(p2GroupList.begin(), p2GroupList.end(), p2GroupId2));

	tester.PrintCapturedPackets();
}

