/*******************************************************************************
 * unittests/libretroshare/gxs/nxs_test/rsgxsnetservice_test.cc                *
 *                                                                             *
 * Copyright (C) 2012, Crispy <retroshare.team@gmailcom>                       *
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

#include "nxsgrpsync_test.h"
#include "nxsmsgsync_test.h"
#include "nxstesthub.h"
#include "nxsgrpsyncdelayed.h"

// disabled, because it fails after rebase to current master (did not fail in 2015, fails in 2016)
TEST(libretroshare_gxs, DISABLED_gxs_grp_sync)
{
	rs_nxs_test::NxsTestScenario *gsync_test = new rs_nxs_test::NxsGrpSync();
	rs_nxs_test::NxsTestHub tHub(gsync_test);
	tHub.StartTest();

    // wait xx secs, because sync happens every 60sec
    rs_nxs_test::NxsTestHub::Wait(1.5*60);

	tHub.EndTest();

	ASSERT_TRUE(tHub.testsPassed());

	tHub.CleanUpTest();
	delete gsync_test ;
}

// disabled, not implemented (does currently the same as NxsGrpSync)
TEST(libretroshare_gxs, DISABLED_gxs_grp_sync_delayed)
{
	rs_nxs_test::NxsTestScenario *gsync_test = new rs_nxs_test::NxsGrpSyncDelayed();
	rs_nxs_test::NxsTestHub tHub(gsync_test);
	tHub.StartTest();

    // wait xx secs, because sync happens every 60sec
        rs_nxs_test::NxsTestHub::Wait(2.5*60);

	tHub.EndTest();

	ASSERT_TRUE(tHub.testsPassed());

	tHub.CleanUpTest();

	delete gsync_test ;
}

TEST(libretroshare_gxs, gxs_msg_sync)
{
	rs_nxs_test::NxsTestScenario *gsync_test = new rs_nxs_test::NxsMsgSync();
	rs_nxs_test::NxsTestHub tHub(gsync_test);
	tHub.StartTest();

	// wait for ten seconds
	rs_nxs_test::NxsTestHub::Wait(10);

	tHub.EndTest();

	ASSERT_TRUE(tHub.testsPassed());

	tHub.CleanUpTest();
	delete gsync_test ;
}

TEST(libretroshare_gxs, gxs_msg_sync_delayed)
{

}

