#include "gxsteststats.h"

#define POLLING_TIME_OUT 5

GxsTestStats::GxsTestStats(GenExchangeTestService *const testService, RsGeneralDataService *dataService)
 : GenExchangeTest(testService, dataService, POLLING_TIME_OUT){
}

GxsTestStats::~GxsTestStats()
{
}

void GxsTestStats::runTests()
{
    testGroupStatistics();
}

void GxsTestStats::testGroupStatistics()
{

}

void GxsTestStats::testServiceStatistics()
{
}
