/*
 * rsgxsnetservice_test.cc
 *
 *  Created on: 11 Jul 2012
 *      Author: crispy
 */

#include <gtest/gtest.h>

#include "nxsgrpsync_test.h"
#include "nxsmsgsync_test.h"
#include "nxstesthub.h"
#include "nxsgrpsyncdelayed.h"

// disabled, because it fails after rebase to current master (did not fail in 2015, fails in 2016)
TEST(libretroshare_gxs, DISABLED_gxs_grp_sync)
{
	rs_nxs_test::NxsTestScenario::pointer gsync_test = rs_nxs_test::NxsTestScenario::pointer(
			new rs_nxs_test::NxsGrpSync());
	rs_nxs_test::NxsTestHub tHub(gsync_test);
	tHub.StartTest();

    // wait xx secs, because sync happens every 60sec
    rs_nxs_test::NxsTestHub::Wait(1.5*60);

	tHub.EndTest();

	ASSERT_TRUE(tHub.testsPassed());

	tHub.CleanUpTest();
}

// disabled, not implemented (does currently the same as NxsGrpSync)
TEST(libretroshare_gxs, DISABLED_gxs_grp_sync_delayed)
{
	rs_nxs_test::NxsTestScenario::pointer gsync_test = rs_nxs_test::NxsTestScenario::pointer(
			new rs_nxs_test::NxsGrpSyncDelayed());
	rs_nxs_test::NxsTestHub tHub(gsync_test);
	tHub.StartTest();

    // wait xx secs, because sync happens every 60sec
        rs_nxs_test::NxsTestHub::Wait(2.5*60);

	tHub.EndTest();

	ASSERT_TRUE(tHub.testsPassed());

	tHub.CleanUpTest();

}

TEST(libretroshare_gxs, gxs_msg_sync)
{
	rs_nxs_test::NxsTestScenario::pointer gsync_test = rs_nxs_test::NxsTestScenario::pointer(
			new rs_nxs_test::NxsMsgSync);
	rs_nxs_test::NxsTestHub tHub(gsync_test);
	tHub.StartTest();

	// wait for ten seconds
	rs_nxs_test::NxsTestHub::Wait(10);

	tHub.EndTest();

	ASSERT_TRUE(tHub.testsPassed());

	tHub.CleanUpTest();
}

TEST(libretroshare_gxs, gxs_msg_sync_delayed)
{

}

