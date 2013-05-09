

#include "genexchangetester.h"
#include "gxspublishgrouptest.h"
#include "gxspublishmsgtest.h"
#include "util/utest.h"
#include "gxs/rsdataservice.h"
#include "rsdummyservices.h"


/*!
 * It always hard to say exactly what coverage of a test would
 * be ahead of time. Partly because its difficult to create the
 * actual conditions of a test or the permutations of different request
 * options to a module is extremely large (and there are probably ways to deal with this)
 * In so far as the genexchange test is concerned we are primarily interested that it
 * retrieves and stores data correctly
 * The auxillary (and important) requirement is authentication and ensuring the authentication
 * rules are respected. This auxillary requirement is of the "hard" situation to create as
 * genexchange depends on an external module (rsidentity) for satisfying a significant sum
 * of its authentication. This difficulty is solved with a dummy identity service.
 * Which passes all authentications (In this respect authentication) is reserved for "online"
 * testing and is relatively straight forward.
 *
 */

INITTEST();


int main()
{

    RsGeneralDataService* dataStore = new RsDataService("./", "testServiceDb", RS_SERVICE_TYPE_DUMMY, NULL);

    // we want to use default authentication which is NO authentication :)
    GenExchangeTestService testService(dataStore, NULL, NULL);

    //GxsPublishGroupTest testGrpPublishing(&testService, dataStore);
    //testGrpPublishing.runTests();

    GxsPublishMsgTest testMsgPublishing(&testService, dataStore);
    testMsgPublishing.runTests();

    FINALREPORT("RsGenExchangeTest");

    return 0;
}
