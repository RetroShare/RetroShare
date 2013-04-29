/*
 * gxspublishgrouptest.cc
 *
 *  Created on: 27 Apr 2013
 *      Author: crispy
 */

#include "gxspublishgrouptest.h"
#include "util/utest.h"

#define POLLING_TIME_OUT 5

GxsPublishGroupTest::GxsPublishGroupTest(GenExchangeTestService* const testService,
		RsGeneralDataService* dataService)
 : GenExchangeTest(testService, dataService, POLLING_TIME_OUT)
{

}

GxsPublishGroupTest::~GxsPublishGroupTest()
{
}

bool GxsPublishGroupTest::testGrpSubmissionRetrieval()
{

	setUp();

	GenExchangeTestService* testService = getTestService();
	RsTokenService* tokenService = getTokenService();

    // create some random grps to allow msg testing

	RsDummyGrp* dgrp1 = new RsDummyGrp();
	RsDummyGrp* dgrp2 = new RsDummyGrp();
	RsDummyGrp* dgrp3 = new RsDummyGrp();

	RsDummyGrp* dgrp1_copy = new RsDummyGrp();
	RsDummyGrp* dgrp2_copy = new RsDummyGrp();
	RsDummyGrp* dgrp3_copy = new RsDummyGrp();

	init(*dgrp1);
	init(*dgrp2);
	init(*dgrp3);

	RsTokReqOptions opts;
	opts.mReqType = 45000;
	uint32_t token;
	RsGxsGroupId grpId;

	std::list<RsDummyGrp*> groupsPublished;

	*dgrp1_copy = *dgrp1;
	testService->publishDummyGrp(token, dgrp1);
	pollForGrpAcknowledgement(token, grpId);
	dgrp1_copy->meta.mGroupId = grpId;
	groupsPublished.push_back(dgrp1_copy);

	*dgrp2_copy = *dgrp2;
	testService->publishDummyGrp(token, dgrp2);
	pollForGrpAcknowledgement(token, grpId);
	dgrp1_copy->meta.mGroupId = grpId;
	groupsPublished.push_back(dgrp2_copy);

	*dgrp3_copy = *dgrp3;
	testService->publishDummyGrp(token, dgrp3);
	pollForGrpAcknowledgement(token, grpId);
	dgrp1_copy->meta.mGroupId = grpId;
	groupsPublished.push_back(dgrp3_copy);


	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	std::list<RsGxsGroupId> grpIds;
	tokenService->requestGroupInfo(token, 0, opts);

	pollForToken(token, opts, true);

	bool ok = compareGrpData();

	breakDown();

	return ok;
}

//bool GxsPublishGroupTest::testSpecificGrpRetrieval()
//{
//
//	return false;
//}
//
//bool GxsPublishGroupTest::testGrpIdRetrieval()
//{
//    setUp();
//    setUpLargeGrps(30); // create a large amount of grps
//
//
//    RsTokReqOptions opts;
//    opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;
//    uint32_t token;
//    std::list<RsGxsGroupId> grpIds;
//    mTokenService->requestGroupInfo(token, 0, opts);
//
//    pollForToken(token, opts);
//
//    std::list<RsGxsGroupId>::iterator lit_out = mGrpIdsOut.begin();
//
//    bool ok = true;
//
//    for(; lit_out != mGrpIdsOut.end(); lit_out++)
//    {
//        std::list<RsGxsGroupId>::iterator lit_in = mGrpIdsIn.begin();
//
//        bool found = true;
//        for(; lit_in != mGrpIdsIn.end(); lit_in++)
//        {
//
//             if(*lit_out == *lit_in)
//             {
//                 found = true;
//             }
//        }
//
//        if(!found)
//        {
//            ok = false;
//            break;
//        }
//    }
//
//    breakDown();
//    return ok;
//}
//bool GxsPublishGroupTest::testGrpMetaRetrieval()
//{
//    setUp();
//
//    // create some random grps to allow msg testing
//
//   RsDummyGrp* dgrp1 = new RsDummyGrp();
//   RsDummyGrp* dgrp2 = new RsDummyGrp();
//   RsDummyGrp* dgrp3 = new RsDummyGrp();
//
//   init(dgrp1);
//   init(dgrp2);
//   init(dgrp3);
//
//   RsTokReqOptions opts;
//   opts.mReqType = 45000;
//   uint32_t token;
//   RsGxsGroupId grpId;
//
//   RsGroupMetaData tempMeta;
//
//   tempMeta = dgrp1->meta;
//   mTestService->publishDummyGrp(token, dgrp1);
//   pollForToken(token, opts);
//   mTestService->acknowledgeTokenGrp(token, grpId);
//   tempMeta.mGroupId = grpId;
//   mGrpMetaDataOut.push_back(tempMeta);
//
//
//   tempMeta = dgrp2->meta;
//   mTestService->publishDummyGrp(token, dgrp2);
//   pollForToken(token, opts);
//   mTestService->acknowledgeTokenGrp(token, grpId);
//   tempMeta.mGroupId = grpId;
//   mGrpMetaDataOut.push_back(tempMeta);
//
//   tempMeta = dgrp3->meta;
//   mTestService->publishDummyGrp(token, dgrp3);
//   pollForToken(token, opts);
//   mTestService->acknowledgeTokenGrp(token, grpId);
//   tempMeta.mGroupId = grpId;
//   mGrpMetaDataOut.push_back(tempMeta);
//
//
//   opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
//
//   mTokenService->requestGroupInfo(token, 0, opts);
//
//   pollForToken(token, opts);
//
//   std::list<RsGroupMetaData>::iterator lit_out = mGrpMetaDataOut.begin();
//
//   bool ok = true;
//
//   for(; lit_out != mGrpMetaDataOut.end(); lit_out++)
//   {
//       const RsGroupMetaData& grpMetaOut = *lit_out;
//
//       std::list<RsGroupMetaData>::iterator lit_in = mGrpMetaDataIn.begin();
//
//       bool found = true;
//       for(; lit_in != mGrpMetaDataIn.end(); lit_in++)
//       {
//            const RsGroupMetaData& grpMetaIn = *lit_in;
//
//            if(grpMetaOut.mGroupId == grpMetaIn.mGroupId)
//            {
//                found = true;
//                ok &= grpMetaIn == grpMetaOut;
//            }
//       }
//
//       if(!found)
//       {
//           ok = false;
//           break;
//       }
//   }
//
//   breakDown();
//   return ok;
//}
void GxsPublishGroupTest::runTests()
{
	CHECK(testGrpSubmissionRetrieval());
}




