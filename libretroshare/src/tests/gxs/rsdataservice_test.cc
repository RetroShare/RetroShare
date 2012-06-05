
#include "support.h"
#include "data_support.h"
#include "rsdataservice_test.h"
#include "gxs/rsdataservice.h"

#define DATA_BASE_NAME "msg_grp_Store"

INITTEST();
RsGeneralDataService* dStore = NULL;

void setUp();
void tearDown();

int main()
{

    std::cerr << "RsDataService Tests" << std::endl;

    test_groupStoreAndRetrieve(); REPORT("test_groupStoreAndRetrieve");

    test_messageStoresAndRetrieve(); REPORT("test_messageStoresAndRetrieve");

    test_messageVersionRetrieve(); REPORT("test_messageVersionRetrieve");

    test_groupVersionRetrieve(); REPORT("test_groupVersionRetrieve");

    FINALREPORT("RsDataService Tests");

    return TESTRESULT();

}




void test_groupStoreAndRetrieve(){

    setUp();

    int nGrp = rand()%32;
    std::set<RsNxsGrp*> s;
    RsNxsGrp* grp;
    for(int i = 0; i < nGrp; i++){
       grp = new RsNxsGrp(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
       init_item(grp);
       s.insert(grp);
   }

    dStore->storeGroup(s);

    std::map<std::string, RsNxsGrp*> gm;
    dStore->retrieveGrps(gm, false);

    // now match grps together

    // simple check,are they the same size
    CHECK(gm.size() == s.size());

    std::set<RsNxsGrp*>::iterator sit = s.begin();
    std::map<std::string, RsNxsGrp*>::iterator mit;
    bool matched = true;

    for(; sit != s.end(); sit++){
        RsNxsGrp* g1 = *sit;
        mit = gm.find(g1->grpId);

        if(mit == gm.end()){
            matched = false;
            continue;
        }

        RsNxsGrp* g2 = gm[g1->grpId];

        if(! (*g1 == *g2) )
            matched = false;


        // remove grp file
        if(g1)
            remove(g1->grpId.c_str());
    }

    CHECK(matched);

    tearDown();
}



void test_messageStoresAndRetrieve(){

    setUp();

    int nMsgs = rand()%32;
    std::set<RsNxsMsg*> s;
    RsNxsMsg* msg;
    std::string grpId;
    randString(SHORT_STR, grpId);
    for(int i = 0; i < nMsgs; i++){
       msg = new RsNxsMsg(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
       init_item(msg);
       msg->grpId = grpId;
       s.insert(msg);
   }

    dStore->storeMessage(s);

    std::map<std::string, RsNxsMsg*> msgs;
    dStore->retrieveMsgs(grpId, msgs, false);

    CHECK(msgs.size() == s.size());

    std::set<RsNxsMsg*>::iterator sit = s.begin();
    std::map<std::string, RsNxsMsg*>::iterator mit;
    bool matched = true;

    for(; sit != s.end(); sit++){
        RsNxsMsg* m1 = *sit;
        mit = msgs.find(m1->msgId);

        if(mit == msgs.end()){
            matched = false;
            continue;
        }

        RsNxsMsg* m2 = msgs[m1->msgId];

        if(! (*m1 == *m2) )
            matched = false;
    }

    CHECK(matched);

    std::string msgFile = grpId + "-msgs";
    remove(msgFile.c_str());

    tearDown();
}

void test_messageVersionRetrieve(){

    setUp();

    // place two messages in store and attempt to retrieve them
    std::set<RsNxsMsg*> s;
    RsNxsMsg* msg1 = new RsNxsMsg(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);;
    RsNxsMsg* msg2 = new RsNxsMsg(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);;
    RsNxsMsg* msg3 = new RsNxsMsg(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);;
    std::string grpId;
    randString(SHORT_STR, grpId);
    msg1->grpId = grpId;
    msg2->grpId = grpId;
    msg3->grpId = grpId;
    init_item(msg1);
    init_item(msg2);
    init_item(msg3);
    s.insert(msg1); s.insert(msg2); s.insert(msg3);

    dStore->storeMessage(s);

    RsGxsMsgId msgId;
    msgId.grpId = msg2->grpId;
    msgId.idSign = msg2->idSign;
    msgId.msgId = msg2->msgId;
    RsNxsMsg* msg2_r = dStore->retrieveMsgVersion(msgId);

    CHECK(msg2_r != NULL);

    if(msg2_r)
        CHECK(*msg2 == *msg2_r);

    delete msg1;
    delete msg2;
    delete msg3;
    delete msg2_r;

    std::string msgFile = grpId + "-msgs";
    remove(msgFile.c_str());

    tearDown();
}

void test_groupVersionRetrieve(){

    setUp();

    std::set<RsNxsGrp*> grps;
    RsNxsGrp* group1 = new RsNxsGrp(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
    RsNxsGrp* group2 = new RsNxsGrp(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);;
    RsNxsGrp* group3 = new RsNxsGrp(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);;
    RsNxsGrp* group2_r = NULL;

    init_item(group1);
    init_item(group2);
    init_item(group3);

    grps.insert(group1); grps.insert(group2); grps.insert(group3);

    RsGxsGrpId grpId;
    grpId.grpId = group2->grpId;
    grpId.adminSign = group2->adminSign;

    dStore->storeGroup(grps);
    group2_r = dStore->retrieveGrpVersion(grpId);


    CHECK(group2_r != NULL);

    if(group2_r)
        CHECK(*group2 == *group2_r);


    delete group1;
    delete group2;
    delete group3;
    delete group2_r;

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
