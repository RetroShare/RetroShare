

#include "genexchangetester.h"
#include "genexchangetestservice.h"
#include "util/utest.h"
#include "gxs/gxscoreserver.h"
#include "gxs/rsdataservice.h"
#include "rsdummyservices.h"

INITTEST();


int main()
{


    GxsCoreServer gxsCore;

    // create data service and dummy net service
    RsDummyNetService dummyNet;
    RsDataService dataStore("./", "testServiceDb", 0, NULL);
    GenExchangeTestService testService;
    gxsCore.addService(&testService);
    createThread(gxsCore);



}
