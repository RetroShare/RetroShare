/*******************************************************************************
 * unittests/libretroshare/gxs/gen_exchange/gxspublishmsgtest.cc               *
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

#include <gtest/gtest.h>

#include "gxspublishmsgtest.h"

#define POLLING_TIME_OUT 5

GxsPublishMsgTest::GxsPublishMsgTest(GenExchangeTestService* const testService,
		RsGeneralDataService* dataService)
 : GenExchangeTest(testService, dataService, POLLING_TIME_OUT)
{
}

GxsPublishMsgTest::~GxsPublishMsgTest()
{
}

void GxsPublishMsgTest::runTests()
{
	EXPECT_TRUE(testMsgSubmissionRetrieval());
}

bool GxsPublishMsgTest::testMsgSubmissionRetrieval()
{
    // start up
    setUp();
    std::list<RsGxsGroupId> grpIds;
    createGrps(4, grpIds);

    /********************/

    RsDummyMsg* msg = new RsDummyMsg();
    init(*msg);

    msg->meta.mGroupId = grpIds.front();
    uint32_t token;
    RsDummyMsg* msgOut = new RsDummyMsg();
    *msgOut = *msg;
    getTestService()->publishDummyMsg(token, msg);


    RsGxsGrpMsgIdPair msgId;
    pollForMsgAcknowledgement(token, msgId);
    msgOut->meta.mMsgId = msgId.second;

    DummyMsgMap msgMap;
    std::vector<RsDummyMsg*> msgV;
    msgV.push_back(msgOut);
    msgMap[msgOut->meta.mGroupId] = msgV;
    storeToMsgDataOutMaps(msgMap);


    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

    getTokenService()->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds);

    // poll again
    pollForToken(token, opts, true);

    bool ok = compareMsgDataMaps();

    // complete
    breakDown();

    return ok;
}



