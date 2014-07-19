#ifndef GXSTESTSTATS_H
#define GXSTESTSTATS_H

#include "genexchangetester.h"

class GxsTestStats : public GenExchangeTest {
public:

    GxsTestStats(GenExchangeTestService* const testService,
            RsGeneralDataService* dataService);
    virtual ~GxsTestStats();

    void runTests();
private:

    void testGroupStatistics();
    void testServiceStatistics();
};

#endif // GXSTESTSTATS_H
