

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

    CHECK(tester.testMsgSubmissionRetrieval()); REPORT("testMsgSubmissionRetrieval()");
    CHECK(tester.testSpecificMsgMetaRetrieval()); REPORT("testSpecificMsgMetaRetrieval()");
    CHECK(tester.testMsgIdRetrieval()); REPORT("tester.testMsgIdRetrieval()");
    CHECK(tester.testRelatedMsgIdRetrieval_Parents()); REPORT("tester.testRelatedMsgIdRetrieval_Parents()");
    CHECK(tester.testRelatedMsgIdRetrieval_OrigMsgId()); REPORT("tester.testRelatedMsgIdRetrieval_OrigMsgId()");
    CHECK(tester.testRelatedMsgIdRetrieval_Latest()); REPORT("tester.testRelatedMsgIdRetrieval_Latest()");

    CHECK(tester.testGrpSubmissionRetrieval()); REPORT("tester.testGrpSubmissionRetrieval()");
    CHECK(tester.testGrpMetaRetrieval()); REPORT("tester.testGrpMetaRetrieval()");
    CHECK(tester.testGrpIdRetrieval()); REPORT("tester.testGrpIdRetrieval()");
    CHECK(tester.testGrpMetaModRequest()); REPORT("tester.testGrpMetaModRequest()");

    FINALREPORT("RsGenExchangeTest");

    return 0;
}
