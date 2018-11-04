/*******************************************************************************
 * unittests/libretroshare/gxs/gen_exchange/gxspublishgrouptest.cc             *
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

#include "gxspublishgrouptest.h"
#include "libretroshare/serialiser/support.h"

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

	std::vector<RsDummyGrp*> groupsPublished;

	*dgrp1_copy = *dgrp1;
	testService->publishDummyGrp(token, dgrp1);
	pollForGrpAcknowledgement(token, grpId);
	dgrp1_copy->meta.mGroupId = grpId;
	groupsPublished.push_back(dgrp1_copy);

	*dgrp2_copy = *dgrp2;
	testService->publishDummyGrp(token, dgrp2);
	pollForGrpAcknowledgement(token, grpId);
	dgrp2_copy->meta.mGroupId = grpId;
	groupsPublished.push_back(dgrp2_copy);

	*dgrp3_copy = *dgrp3;
	testService->publishDummyGrp(token, dgrp3);
	pollForGrpAcknowledgement(token, grpId);
	dgrp3_copy->meta.mGroupId = grpId;
	groupsPublished.push_back(dgrp3_copy);


	storeToGrpDataOutList(groupsPublished);

	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;
	std::list<RsGxsGroupId> grpIds;
	tokenService->requestGroupInfo(token, 0, opts);

	pollForToken(token, opts, true);


	bool ok = compareGrpData();

	breakDown();

	return ok;
}

bool GxsPublishGroupTest::testSpecificGrpRetrieval()
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

	init(*dgrp1);
	init(*dgrp2);
	init(*dgrp3);

	RsTokReqOptions opts;
	opts.mReqType = 45000;
	uint32_t token;
	RsGxsGroupId grpId;

	std::vector<RsDummyGrp*> groupsPublished;
	std::list<RsGxsGroupId> grpIds;

	*dgrp1_copy = *dgrp1;
	testService->publishDummyGrp(token, dgrp1);
	pollForGrpAcknowledgement(token, grpId);
	dgrp1_copy->meta.mGroupId = grpId;
	groupsPublished.push_back(dgrp1_copy);
	grpIds.push_back(grpId);

	*dgrp2_copy = *dgrp2;
	testService->publishDummyGrp(token, dgrp2);
	pollForGrpAcknowledgement(token, grpId);
	dgrp2_copy->meta.mGroupId = grpId;
	groupsPublished.push_back(dgrp2_copy);
	grpIds.push_back(grpId);


	testService->publishDummyGrp(token, dgrp3);
	pollForGrpAcknowledgement(token, grpId);


	storeToGrpDataOutList(groupsPublished);

	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	tokenService->requestGroupInfo(token, 0, opts, grpIds);

	pollForToken(token, opts, true);


	bool ok = compareGrpData();

	breakDown();

	return ok;
}

bool GxsPublishGroupTest::testGrpIdRetrieval()
{
    setUp();

    std::list<RsGxsGroupId> grpIds;
    createGrps(5, grpIds);
    storeToGrpIdsOutList(grpIds);
    RsTokReqOptions opts;
    opts.mReqType = GXS_REQUEST_TYPE_GROUP_IDS;
    uint32_t token;

    getTokenService()->requestGroupInfo(token, 0, opts);

    pollForToken(token, opts, true);

    bool ok = compareGrpIds();

    breakDown();

    return ok;
}

bool GxsPublishGroupTest::testUpdateGroup()
{
    setUp();

    GenExchangeTestService* testService = getTestService();
    RsTokenService* tokenService = getTokenService();

// create some random grps to allow msg testing

    RsDummyGrp* dgrp1 = new RsDummyGrp();
    RsDummyGrp* dgrp2 = new RsDummyGrp();

    RsDummyGrp* dgrp2_copy = new RsDummyGrp();

    init(*dgrp1);
    init(*dgrp2);

    RsTokReqOptions opts;
    opts.mReqType = 45000;
    uint32_t token;
    RsGxsGroupId grpId;

    std::vector<RsDummyGrp*> groupsPublished;
    std::list<RsGxsGroupId> grpIds;

    std::string name = dgrp1->meta.mGroupName;
    *dgrp2 = *dgrp1;
    testService->publishDummyGrp(token, dgrp1);
    bool ok = pollForGrpAcknowledgement(token, grpId);

    grpIds.push_back(grpId);
    RsGxsGroupUpdateMeta updateMeta(grpId);

    updateMeta.setMetaUpdate(RsGxsGroupUpdateMeta::NAME, name);
    randString(SHORT_STR, dgrp2->grpData);
    dgrp2->meta.mGroupId = grpId;
    *dgrp2_copy = *dgrp2;
    dgrp2->grpData ="ojfosfjsofjsof";
    testService->updateDummyGrp(token, updateMeta, dgrp2);
    ok &= pollForGrpAcknowledgement(token, grpId);

    groupsPublished.push_back(dgrp2_copy);

    opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

    tokenService->requestGroupInfo(token, 0, opts, grpIds);

    pollForToken(token, opts, true);


    ok &= compareGrpData();

    breakDown();

    return ok;

}

bool GxsPublishGroupTest::testGrpMetaRetrieval()
{

	setUp();

	GenExchangeTestService* testService = getTestService();

    // create some random grps to allow msg testing

	RsDummyGrp* dgrp1 = new RsDummyGrp();
	RsDummyGrp* dgrp2 = new RsDummyGrp();
	RsDummyGrp* dgrp3 = new RsDummyGrp();

	init(*dgrp1);
	init(*dgrp2);
	init(*dgrp3);

	RsTokReqOptions opts;
	opts.mReqType = 45000;
	uint32_t token;
	RsGxsGroupId grpId;

	RsGroupMetaData meta1(dgrp1->meta);
	RsGroupMetaData meta2(dgrp2->meta);
	RsGroupMetaData meta3(dgrp3->meta);

	std::list<RsGroupMetaData> groupsMetaPublished;

	testService->publishDummyGrp(token, dgrp1);
	pollForGrpAcknowledgement(token, grpId);
	meta1.mGroupId = grpId;
	groupsMetaPublished.push_back(meta1);

	testService->publishDummyGrp(token, dgrp2);
	pollForGrpAcknowledgement(token, grpId);
	meta2.mGroupId = grpId;
	groupsMetaPublished.push_back(meta2);

	testService->publishDummyGrp(token, dgrp3);
	pollForGrpAcknowledgement(token, grpId);
	meta3.mGroupId = grpId;
	groupsMetaPublished.push_back(meta3);

	storeToGrpMetaOutList(groupsMetaPublished);

	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;
	std::list<RsGxsGroupId> grpIds;

	getTokenService()->requestGroupInfo(token, 0, opts);

	pollForToken(token, opts, true);

	bool ok = compareGrpMeta();

	breakDown();

   return ok;
}

void GxsPublishGroupTest::runTests()
{
//        CHECK(testGrpSubmissionRetrieval());
//        CHECK(testGrpIdRetrieval());
//        CHECK(testGrpMetaRetrieval());
    //   CHECK(testSpecificGrpRetrieval());
        EXPECT_TRUE(testUpdateGroup());
}




