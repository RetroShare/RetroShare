/*
 * gxspublishgrouptest.h
 *
 *  Created on: 27 Apr 2013
 *      Author: crispy
 */

#ifndef GXSPUBLISHGROUPTEST_H_
#define GXSPUBLISHGROUPTEST_H_

#include "genexchangetester.h"

class GxsPublishGroupTest : public GenExchangeTest {
public:

	GxsPublishGroupTest(GenExchangeTestService* const testService,
			RsGeneralDataService* dataService);
	virtual ~GxsPublishGroupTest();

    void runTests();

private:

    // group tests
//UNUSED    bool testGrpSubmissionRetrieval();
//UNUSED    bool testSpecificGrpRetrieval();
//UNUSED    bool testGrpIdRetrieval();
//UNUSED    bool testGrpMetaRetrieval();
    bool testUpdateGroup();


private:

};

#endif /* GXSPUBLISHGROUPTEST_H_ */
