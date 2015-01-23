
#include <gtest/gtest.h>

#include "libretroshare/serialiser/support.h"
#include "libretroshare/gxs/common/data_support.h"
#include "rsdataservice_test.h"
#include "gxs/rsgds.h"
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
    std::map<RsNxsGrp*, RsGxsGrpMetaData*> grps, grps_copy;
    RsNxsGrp* grp;
    RsGxsGrpMetaData* grpMeta;
    for(int i = 0; i < nGrp; i++){
        std::pair<RsNxsGrp*, RsGxsGrpMetaData*> p;
       grp = new RsNxsGrp(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
       grpMeta = new RsGxsGrpMetaData();
       p.first = grp;
       p.second = grpMeta;
       init_item(*grp);
       init_item(grpMeta);
       grpMeta->mGroupId = grp->grpId;
       grps.insert(p);
       RsNxsGrp* grp_copy = new RsNxsGrp(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
       *grp_copy = *grp;
       RsGxsGrpMetaData* grpMeta_copy = new RsGxsGrpMetaData();
       *grpMeta_copy = *grpMeta;
       grps_copy.insert(std::make_pair(grp_copy, grpMeta_copy ));
       grpMeta = NULL;
       grp = NULL;
   }

    dStore->storeGroup(grps);

    //use copy, a grps are deleted in store
    grps.clear();
    grps = grps_copy;

    std::map<RsGxsGroupId, RsNxsGrp*> gR;
    std::map<RsGxsGroupId, RsGxsGrpMetaData*> grpMetaR;
    dStore->retrieveNxsGrps(gR, false, false);
    dStore->retrieveGxsGrpMetaData(grpMetaR);

    std::map<RsNxsGrp*, RsGxsGrpMetaData*>::iterator mit = grps.begin();

    bool grpMatch = true, grpMetaMatch = true;

    for(; mit != grps.end(); mit++)
    {
        const RsGxsGroupId grpId = mit->first->grpId;

        // check if it exists
        if(gR.find(grpId) == gR.end()) {
            grpMatch = false;
            break;
        }

        RsNxsGrp *l = mit->first,
        *r = gR[grpId];

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

        RsGxsGrpMetaData *l_Meta = mit->second,
        *r_Meta = grpMetaR[grpId];

        if(!(*l_Meta == *r_Meta))
        {
            grpMetaMatch = false;
            break;
        }

        /* release resources */
        delete l_Meta;
        delete r_Meta;
        delete l;
        delete r;

        remove(grpId.toStdString().c_str());
    }

    grpMetaR.clear();

    EXPECT_TRUE(grpMatch);
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

    std::map<RsNxsMsg*, RsGxsMsgMetaData*> msgs;
    std::map<RsNxsMsg*, RsGxsMsgMetaData*> msgs_copy;
    RsNxsMsg* msg = NULL;
    RsGxsMsgMetaData* msgMeta = NULL;
    int nMsgs = rand()%120;
    GxsMsgReq req;

    std::map<RsGxsMessageId, RsNxsMsg*> VergrpId0, VergrpId1;
    std::map<RsGxsMessageId, RsGxsMsgMetaData*> VerMetagrpId0, VerMetagrpId1;

    for(int i=0; i<nMsgs; i++)
    {
        msg = new RsNxsMsg(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
        msgMeta = new RsGxsMsgMetaData();
        init_item(*msg);
        init_item(msgMeta);
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

        RsNxsMsg* msg_copy = new RsNxsMsg(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
        RsGxsMsgMetaData* msgMeta_copy = new RsGxsMsgMetaData();

        *msg_copy = *msg;
        *msgMeta_copy = *msgMeta;

        // store msgs in map to use for verification
        std::pair<RsGxsMessageId, RsNxsMsg*> vP(msg->msgId, msg_copy);
        std::pair<RsGxsMessageId, RsGxsMsgMetaData*> vPmeta(msg->msgId, msgMeta_copy);

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



        msg = NULL;
        msgMeta = NULL;

        msgs.insert(p);
        msgs_copy.insert(std::make_pair(msg_copy, msgMeta_copy));
    }

    req[grpV[0]] = std::vector<RsGxsMessageId>(); // assign empty list for other

    dStore->storeMessage(msgs);
    msgs.clear();
    msgs = msgs_copy;

    // now retrieve msgs for comparison
    // first selective retrieval

    GxsMsgResult msgResult;
    GxsMsgMetaResult msgMetaResult;
    dStore->retrieveNxsMsgs(req, msgResult, false);

    dStore->retrieveGxsMsgMetaData(req, msgMetaResult);

    // now look at result for grpId 1
    std::vector<RsNxsMsg*>& result0 = msgResult[grpId0];
    std::vector<RsNxsMsg*>& result1 = msgResult[grpId1];
    std::vector<RsGxsMsgMetaData*>& resultMeta0 = msgMetaResult[grpId0];
    std::vector<RsGxsMsgMetaData*>& resultMeta1 = msgMetaResult[grpId1];



    bool msgGrpId0_Match = true, msgGrpId1_Match = true;
    bool msgMetaGrpId0_Match = true, msgMetaGrpId1_Match = true;

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


