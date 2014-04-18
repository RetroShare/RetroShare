/*
 * rsgxsnetservice_test.cc
 *
 *  Created on: 11 Jul 2012
 *      Author: crispy
 */

#include <gtest/gtest.h>

#include "nxsgrpsync_test.h"
#include "nxstesthub.h"

TEST(libretroshare_gxs, gxs_grp_sync)
{
	rs_nxs_test::NxsTestScenario::pointer gsync_test = rs_nxs_test::NxsTestScenario::pointer(
			new rs_nxs_test::NxsGrpSync());
	rs_nxs_test::NxsTestHub tHub(gsync_test);
	tHub.StartTest();

	// wait for ten seconds
	rs_nxs_test::NxsTestHub::Wait(10);

	tHub.EndTest();

	ASSERT_TRUE(tHub.testsPassed());
}
