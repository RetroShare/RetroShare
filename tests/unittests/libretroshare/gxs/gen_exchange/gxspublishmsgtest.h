/*
 * gxspublishmsgtest.h
 *
 *  Created on: 27 Apr 2013
 *      Author: crispy
 */

#ifndef GXSPUBLISHMSGTEST_H_
#define GXSPUBLISHMSGTEST_H_

#include "genexchangetester.h"

class GxsPublishMsgTest: public GenExchangeTest {
public:
	GxsPublishMsgTest(GenExchangeTestService* const testService,
			RsGeneralDataService* dataService);
	virtual ~GxsPublishMsgTest();

	void runTests();

    // message tests
    bool testMsgSubmissionRetrieval();
//    bool testMsgIdRetrieval();
//    bool testMsgIdRetrieval_OptParents();
//    bool testMsgIdRetrieval_OptOrigMsgId();
//    bool testMsgIdRetrieval_OptLatest();
//    bool testSpecificMsgMetaRetrieval();


};

#endif /* GXSPUBLISHMSGTEST_H_ */
