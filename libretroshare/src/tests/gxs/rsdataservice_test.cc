
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

    //test_messageStoresAndRetrieve(); REPORT("test_messageStoresAndRetrieve");

    FINALREPORT("RsDataService Tests");

    return TESTRESULT();

}




void test_groupStoreAndRetrieve(){

    setUp();

    int nGrp = rand()%32;
    std::map<RsNxsGrp*, RsGxsGrpMetaData*> grps;
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
       grps.insert(p);
   }

    dStore->storeGroup(grps);

    std::map<std::string, RsNxsGrp*> gR;
    std::map<std::string, RsGxsGrpMetaData*> grpMetaR;
    dStore->retrieveNxsGrps(gR, false);
    dStore->retrieveGxsGrpMetaData(grpMetaR);

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
