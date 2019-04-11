/*******************************************************************************
 * unittests/libretroshare/services/gxs/nxsbasic_test.cc                       *
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

#include "rsitems/rsnxsitems.h"

// local
#include "GxsIsolatedServiceTester.h"
#include "gxstestservice.h"

#define N_PEERS 10
#define MAX_COUNT 5

/* Here we want to test the most basic NXS interactions.
 * we have a GXS service, with no AuthorIDs or Circles.
 *
 */

/*
 * Test 1: nothing in Groups.... only sync packets.
 *
 * This test is rather slow - should speed it up.
 */

TEST(libretroshare_services, DISABLED_GXS_nxs_basic)
{
	RsPeerId ownId = RsPeerId::random();
	RsPeerId friendId = RsPeerId::random();

	std::list<RsPeerId> peers;
	peers.push_back(friendId);
	for(int i = 0; i < N_PEERS; i++)
	{
		peers.push_back(RsPeerId::random());
	}

	int testMode = 0;
	GxsIsolatedServiceTester tester(ownId, friendId, peers, testMode);

	int counter = 0;
	int syncGroupCount = 0;
	while((syncGroupCount < 2) && (counter < 30))
	{
		counter++;

		// Send in Lots of SyncGrp packets
		// Expect nothing back - as its empty.
		std::cerr << "Sending in SyncGrp";
		std::cerr << std::endl;

		RsNxsSyncGrpItem *syncGrp = new RsNxsSyncGrpItem(RS_SERVICE_GXS_TYPE_TEST);
		syncGrp->flag = 0; //RsNxsSyncGrp::FLAG_USE_SYNC_HASH;
		syncGrp->PeerId(friendId);
		tester.sendPacket(syncGrp);

		// Expect only RsNxsSyncGrp messages.
		if (tester.tickUntilPacket(1))
		{
			RsItem *item = tester.getPacket();
			ASSERT_TRUE(item != NULL);
			item->print(std::cerr);

			std::cerr << "Recved in SyncGrp";
			std::cerr << std::endl;

			RsNxsSyncGrpItem *grp = dynamic_cast<RsNxsSyncGrpItem *>(item);
			ASSERT_TRUE(grp);
			delete grp;

			syncGroupCount++;
		}
		sleep(1);
	}
	ASSERT_TRUE(syncGroupCount >= 2);
}



/*
 * Test 2: Create a Group ... should a response with Group List.
 * should test a full exchange.
 *
 */

TEST(libretroshare_services, DISABLED_GXS_nxs_basic2)
{
	RsPeerId ownId = RsPeerId::random();
	RsPeerId friendId = RsPeerId::random();

	std::list<RsPeerId> peers;
	peers.push_back(friendId);
	for(int i = 0; i < N_PEERS; i++)
	{
		peers.push_back(RsPeerId::random());
	}

	int testMode = 0;
	GxsIsolatedServiceTester tester(ownId, friendId, peers, testMode);

	/* create a couple of groups */
	GxsTestService *testService = tester.mTestService;
	RsTokenService *tokenService = testService->RsGenExchange::getTokenService();

	RsTestGroup grp1;
	grp1.mMeta.mGroupName = "grp1";
	grp1.mTestString = "test";
	uint32_t token1;

	RsTestGroup grp2;
	grp2.mMeta.mGroupName = "grp2";
	grp2.mTestString = "test2";
	uint32_t token2;

	ASSERT_TRUE(testService->submitTestGroup(token1, grp1));
	while(tokenService->requestStatus(token1) != RsTokenService::COMPLETE)
	{
		tester.tick();
		sleep(1);
	}

	ASSERT_TRUE(testService->submitTestGroup(token2, grp2));
	while(tokenService->requestStatus(token2) != RsTokenService::COMPLETE)
	{
		tester.tick();
		sleep(1);
	}

	std::cerr << "Created Groups.";
	std::cerr << std::endl;

	RsNxsSyncGrpItem *syncGrp= new RsNxsSyncGrpItem(RS_SERVICE_GXS_TYPE_TEST);
	syncGrp->flag = 0; //RsNxsSyncGrp::FLAG_USE_SYNC_HASH;
	syncGrp->PeerId(friendId);
	tester.sendPacket(syncGrp);


	// expect RsNxsTransac
	int counter = 0;
	bool recvedTransaction = false;

	while((!recvedTransaction) && (counter < MAX_COUNT))
	{
		counter++;

		// Expect only RsNxsSyncGrp messages.
		if (tester.tickUntilPacket(1))
		{
			RsItem *item = tester.getPacket();
			ASSERT_TRUE(item != NULL);

			std::cerr << "Recved in Packet";
			std::cerr << std::endl;
			item->print(std::cerr);

			// ignore NxsSyncGrp.
			RsNxsSyncGrpItem *grp = dynamic_cast<RsNxsSyncGrpItem *>(item);
			RsNxsTransacItem *trans = dynamic_cast<RsNxsTransacItem *>(item);
			if (grp)
			{
				std::cerr << "Recved in SyncGrp - ignoring";
				std::cerr << std::endl;
				delete grp;
			}
			else if (trans)
			{
				// Real expected packet.
				std::cerr << "Recved in RsNxsTransac";
				std::cerr << std::endl;
				recvedTransaction = true;

				// Generate Next Step.
			}
			else
			{
				// ERROR.
				ASSERT_TRUE(false);
			}
		}
		sleep(1);
	}
	ASSERT_TRUE(recvedTransaction);
}







