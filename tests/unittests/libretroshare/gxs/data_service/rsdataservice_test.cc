/*******************************************************************************
 * unittests/libretroshare/gxs/data_service/rsdataservice_test.cc              *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
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

#include "libretroshare/serialiser/support.h"
#include "libretroshare/gxs/common/data_support.h"
#include "rsdataservice_test.h"
#include "gxs/rsgds.h"
#include "gxs/rsgxsutil.h"
#include "gxs/rsdataservice.h"

#define DATA_BASE_NAME "msg_grp_Store"


RsGeneralDataService* dStore = NULL;

void setUp();
void tearDown();

TEST(libretroshare_gxs, RsDataService)
{

    std::cerr << "RsDataService Tests" << std::endl;

    test_groupStoreAndRetrieve();
    test_messageStoresAndRetrieve();
}



/*!
 * All memory is disposed off, good for looking
 * for memory leaks
 */
void test_groupStoreAndRetrieve(){

    setUp();

    int nGrp = rand()%32;
	RsNxsGrpDataTemporaryList grps, grps_copy;
    RsNxsGrp* grp;
    RsGxsGrpMetaData* grpMeta;

    for(int i = 0; i < nGrp; i++)
	{
		std::pair<RsNxsGrp*, RsGxsGrpMetaData*> p;
		grp = new RsNxsGrp(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
		grpMeta = new RsGxsGrpMetaData();

		init_item(*grp);
		init_item(grpMeta);

		grpMeta->mGroupId = grp->grpId;
		grp->metaData = grpMeta ;

		grps.push_back(grp);
	}

    dStore->storeGroup(grps);

    RsNxsGrpDataTemporaryMap gR;
    RsGxsGrpMetaTemporaryMap grpMetaR;

    dStore->retrieveNxsGrps(gR, false, false);
    dStore->retrieveGxsGrpMetaData(grpMetaR);

    bool grpMatch = true, grpMetaMatch = true;

    for( std::list<RsNxsGrp*>::iterator mit = grps.begin(); mit != grps.end(); mit++)
    {
        const RsGxsGroupId grpId = (*mit)->metaData->mGroupId;

        // check if it exists
        if(gR.find(grpId) == gR.end()) {
            grpMatch = false;
            break;
        }

        RsNxsGrp *l = *mit;
        RsNxsGrp *r = gR[grpId];

        // assign transaction number
        // to right to as tn is not stored
        // in db
        r->transactionNumber = l->transactionNumber;

        // then do a comparison
        if(!( *l == *r)) {
            grpMatch = false;
            break;
        }

        // now do a comparison of grp meta types

        if(grpMetaR.find(grpId) == grpMetaR.end())
        {
            grpMetaMatch = false;
            break;
        }

        RsGxsGrpMetaData *l_Meta = (*mit)->metaData,
        *r_Meta = const_cast<RsGxsGrpMetaData*>(grpMetaR[grpId]);

        // assign signSet and mGrpSize
        // to right as these values are not stored in db
        r_Meta->signSet = l_Meta->signSet;
        r_Meta->mGrpSize = l_Meta->mGrpSize;

        if(!(*l_Meta == *r_Meta))
        {
            grpMetaMatch = false;
            break;
        }

        remove(grpId.toStdString().c_str());
    }

    grpMetaR.clear();

    EXPECT_TRUE(grpMatch && grpMetaMatch);
    tearDown();
}

/*!
 * Test for both selective and
 * bulk msg retrieval
 */
void test_messageStoresAndRetrieve()
{
    setUp();

    // first create a grpId
    RsGxsGroupId grpId0, grpId1;

    grpId0 = RsGxsGroupId::random();
    grpId1 = RsGxsGroupId::random();
    std::vector<RsGxsGroupId> grpV; // stores grpIds of all msgs stored and retrieved
    grpV.push_back(grpId0);
    grpV.push_back(grpId1);

	RsNxsMsgDataTemporaryList msgs;
    RsNxsMsg* msg = NULL;
    RsGxsMsgMetaData* msgMeta = NULL;
    int nMsgs = rand()%120;
    GxsMsgReq req;

	// These ones are not in auto-delete structures because the data is deleted as part of the RsNxsMsg struct in the msgs list.
	std::map<RsGxsMessageId,RsNxsMsg*> VergrpId0 ;
	std::map<RsGxsMessageId,RsNxsMsg*> VergrpId1 ;

    std::map<RsGxsMessageId, RsGxsMsgMetaData*> VerMetagrpId0;
    std::map<RsGxsMessageId, RsGxsMsgMetaData*> VerMetagrpId1;

    for(int i=0; i<nMsgs; i++)
    {
        msg = new RsNxsMsg(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
        msgMeta = new RsGxsMsgMetaData();
        init_item(*msg);
        init_item(msgMeta);

		msg->metaData = msgMeta ;

        std::pair<RsNxsMsg*, RsGxsMsgMetaData*> p(msg, msgMeta);
        int chosen = 0;
        if(rand()%50 > 24){
            chosen = 1;

        }

        const RsGxsGroupId& grpId = grpV[chosen];

        if(chosen)
            req[grpId].push_back(msg->msgId);

        msgMeta->mMsgId = msg->msgId;
        msgMeta->mGroupId = msg->grpId = grpId;

        // store msgs in map to use for verification
        std::pair<RsGxsMessageId, RsNxsMsg*> vP(msg->msgId, msg);
        std::pair<RsGxsMessageId, RsGxsMsgMetaData*> vPmeta(msg->msgId, msgMeta);

        if(!chosen)
        {
            VergrpId0.insert(vP);
            VerMetagrpId0.insert(vPmeta);
        }
        else
        {
            VergrpId1.insert(vP);
            VerMetagrpId0.insert(vPmeta);
        }


        msgs.push_back(msg);
    }

    req[grpV[0]] = std::vector<RsGxsMessageId>(); // assign empty list for other

    dStore->storeMessage(msgs);

    // now retrieve msgs for comparison
    // first selective retrieval

	t_RsGxsGenericDataTemporaryMapVector<RsNxsMsg> 	       msgResult ; //GxsMsgResult msgResult;. The temporary version cleans up itself.
	t_RsGxsGenericDataTemporaryMapVector<RsGxsMsgMetaData> msgMetaResult ;

    dStore->retrieveNxsMsgs(req, msgResult, false);
    dStore->retrieveGxsMsgMetaData(req, msgMetaResult);

    // now look at result for grpId 1
    std::vector<RsNxsMsg*>& result0 = msgResult[grpId0];
    std::vector<RsNxsMsg*>& result1 = msgResult[grpId1];
    std::vector<RsGxsMsgMetaData*>& resultMeta0 = msgMetaResult[grpId0];
    //std::vector<RsGxsMsgMetaData*>& resultMeta1 = msgMetaResult[grpId1];



    bool msgGrpId0_Match = true, msgGrpId1_Match = true;
    bool msgMetaGrpId0_Match = true/*, msgMetaGrpId1_Match = true*/;

    // MSG test, selective retrieval
    for(std::vector<RsNxsMsg*>::size_type i = 0; i < result0.size(); i++)
    {
        RsNxsMsg* l = result0[i] ;

        if(VergrpId0.find(l->msgId) == VergrpId0.end())
        {
            msgGrpId0_Match = false;
            break;
        }

        RsNxsMsg* r = VergrpId0[l->msgId];
        r->transactionNumber = l->transactionNumber;

        if(!(*l == *r))
        {
            msgGrpId0_Match = false;
            break;
        }
    }

    EXPECT_TRUE(msgGrpId0_Match);

    // META test
    for(std::vector<RsGxsMsgMetaData*>::size_type i = 0; i < resultMeta0.size(); i++)
    {
        RsGxsMsgMetaData* l = resultMeta0[i] ;

        if(VerMetagrpId0.find(l->mMsgId) == VerMetagrpId0.end())
        {
            msgMetaGrpId0_Match = false;
            break;
        }

        RsGxsMsgMetaData* r = VerMetagrpId0[l->mMsgId];

        if(!(*l == *r))
        {
            msgMetaGrpId0_Match = false;
            break;
        }
    }

    EXPECT_TRUE(msgMetaGrpId0_Match);

    // MSG test, bulk retrieval
    for(std::vector<RsNxsMsg*>::size_type i = 0; i < result1.size(); i++)
    {
        RsNxsMsg* l = result1[i] ;

        if(VergrpId1.find(l->msgId) == VergrpId1.end())
        {
            msgGrpId1_Match = false;
            break;
        }

        RsNxsMsg* r = VergrpId1[l->msgId];

        r->transactionNumber = l->transactionNumber;

        if(!(*l == *r))
        {
            msgGrpId1_Match = false;
            break;
        }
    }

    EXPECT_TRUE(msgGrpId1_Match);

    //dStore->retrieveGxsMsgMetaData();
    std::string msgFile = grpId0.toStdString() + "-msgs";
    remove(msgFile.c_str());
    msgFile = grpId1.toStdString() + "-msgs";
    remove(msgFile.c_str());
    tearDown();
}



void setUp(){
    dStore = new RsDataService(".", DATA_BASE_NAME, RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
}

void tearDown(){

    dStore->resetDataStore(); // reset to clean up store files except db
    delete dStore;
    dStore = NULL;
    int rc = remove(DATA_BASE_NAME);

    if(rc == 0){
        std::cerr << "Successful tear down" << std::endl;
    }
    else{
        std::cerr << "Tear down failed" << std::endl;
        perror("Error: ");
    }

}


