/*
 * gxspublishmsgtest.cpp
 *
 *  Created on: 27 Apr 2013
 *      Author: crispy
 */

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



