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
	NxsGrpSync* gsync_test = new NxsGrpSync();
	rs_nxs_test::NxsTestHub tHub(gsync_test);
	tHub.StartTest();
	rs_nxs_test::NxsTestHub::wait(10);
	tHub.EndTest();

	ASSERT_TRUE(tHub.testsPassed());
}
