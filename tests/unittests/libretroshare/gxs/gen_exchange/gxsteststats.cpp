/*******************************************************************************
 * unittests/libretroshare/gxs/gen_exchange/gxsteststats.cpp                   *
 *                                                                             *
 * Copyright (C) 2013, Crispy <retroshare.team@gmailcom>                       *
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

#include "gxsteststats.h"
#include "libretroshare/serialiser/support.h"

#define POLLING_TIME_OUT 5

GxsTestStats::GxsTestStats(GenExchangeTestService *const testService, RsGeneralDataService *dataService)
 : GenExchangeTest(testService, dataService, POLLING_TIME_OUT){
}

GxsTestStats::~GxsTestStats()
{
}

void GxsTestStats::runTests()
{
    //testGroupStatistics();
    testServiceStatistics();
}

void GxsTestStats::testGroupStatistics()
{
    setUp();

    GenExchangeTestService* testService = getTestService();
    RsTokenService* tokenService = getTokenService();

    // create some random grps to allow msg testing

    RsDummyGrp* dgrp1 = new RsDummyGrp();

    init(*dgrp1);
    RsTokReqOptions opts;
    opts.mReqType = 45000;
    uint32_t token;
    RsGxsGroupId grpId;

    testService->publishDummyGrp(token, dgrp1);
    pollForGrpAcknowledgement(token, grpId);

    RsDummyMsg* msg1 = new RsDummyMsg();
    init(*msg1);

    RsDummyMsg* msg2 = new RsDummyMsg();
    init(*msg2);

    msg1->meta.mGroupId = grpId;
    getTestService()->publishDummyMsg(token, msg1);

    RsGxsGrpMsgIdPair msgId;
    pollForMsgAcknowledgement(token, msgId);

    msg2->meta.mGroupId = grpId;
    getTestService()->publishDummyMsg(token, msg2);

    pollForMsgAcknowledgement(token, msgId);

    tokenService->requestGroupStatistic(token, grpId);

    opts.mReqType = GXS_REQUEST_TYPE_GROUP_STATS;
    pollForToken(token, opts);

    GxsGroupStatistic stats;
    ASSERT_TRUE(getGroupStatistic(token, stats));

    ASSERT_TRUE(stats.mNumMsgs == 2);
    breakDown();


}

void GxsTestStats::testServiceStatistics()
{
    setUp();

    GenExchangeTestService* testService = getTestService();
    RsTokenService* tokenService = getTokenService();

    // create some random grps to allow msg testing

    RsDummyGrp* dgrp1 = new RsDummyGrp();
    RsDummyGrp* dgrp2 = new RsDummyGrp();

    init(*dgrp1);
    init(*dgrp2);
    RsTokReqOptions opts;
    opts.mReqType = 45000;
    uint32_t token;
    RsGxsGroupId grpId;

    testService->publishDummyGrp(token, dgrp1);
    pollForGrpAcknowledgement(token, grpId);

    testService->publishDummyGrp(token, dgrp2);
    pollForGrpAcknowledgement(token, grpId);

    RsDummyMsg* msg1 = new RsDummyMsg();
    init(*msg1);

    RsDummyMsg* msg2 = new RsDummyMsg();
    init(*msg2);

    msg1->meta.mGroupId = grpId;
    getTestService()->publishDummyMsg(token, msg1);

    RsGxsGrpMsgIdPair msgId;
    pollForMsgAcknowledgement(token, msgId);

    msg2->meta.mGroupId = grpId;
    getTestService()->publishDummyMsg(token, msg2);

    pollForMsgAcknowledgement(token, msgId);

    tokenService->requestServiceStatistic(token);

    opts.mReqType = GXS_REQUEST_TYPE_SERVICE_STATS;
    pollForToken(token, opts);

    GxsServiceStatistic stats;
    ASSERT_TRUE(getServiceStatistic(token, stats));

    ASSERT_TRUE(stats.mNumMsgs == 2);
    ASSERT_TRUE(stats.mNumGrps == 2);
    ASSERT_TRUE(stats.mNumGrpsSubscribed == 2);

    breakDown();
}
