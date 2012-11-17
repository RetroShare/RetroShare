

#include "genexchangetester.h"
#include "genexchangetestservice.h"
#include "util/utest.h"
#include "gxs/gxscoreserver.h"
#include "gxs/rsdataservice.h"
#include "rsdummyservices.h"

INITTEST();


int main()
{
    GenExchangeTester tester;

//    CHECK(tester.testMsgSubmissionRetrieval()); REPORT("testMsgSubmissionRetrieval()");
//    CHECK(tester.testSpecificMsgMetaRetrieval()); REPORT("testSpecificMsgMetaRetrieval()");
//    CHECK(tester.testMsgIdRetrieval()); REPORT("tester.testMsgIdRetrieval()");
//    CHECK(tester.testMsgIdRetrieval_OptParents()); REPORT("tester.testRelatedMsgIdRetrieval_Parents()");
//    CHECK(tester.testMsgIdRetrieval_OptOrigMsgId()); REPORT("tester.testRelatedMsgIdRetrieval_OrigMsgId()");
//    CHECK(tester.testMsgIdRetrieval_OptLatest()); REPORT("tester.testRelatedMsgIdRetrieval_Latest()");
//    CHECK(tester.testMsgMetaModRequest()); REPORT("tester.testMsgMetaModRequest()");
        //CHECK(tester.testMsgRelatedChildDataRetrieval()); REPORT("tester.testMsgRelatedChildDataRetrieval()");
//    CHECK(tester.testMsgAllVersions()); REPORT("tester.testMsgAllVersions()");

//    CHECK(tester.testGrpSubmissionRetrieval()); REPORT("tester.testGrpSubmissionRetrieval()");
 //   CHECK(tester.testGrpMetaRetrieval()); REPORT("tester.testGrpMetaRetrieval()");
//    CHECK(tester.testGrpIdRetrieval()); REPORT("tester.testGrpIdRetrieval()");
    CHECK(tester.testGrpMetaModRequest()); REPORT("tester.testGrpMetaModRequest()");

    FINALREPORT("RsGenExchangeTest");

    return 0;
}
