/*
 * gxsmsgrelatedtest.h
 *
 *  Created on: 27 Apr 2013
 *      Author: crispy
 */

#ifndef GXSMSGRELATEDTEST_H_
#define GXSMSGRELATEDTEST_H_

#include "genexchangetester.h"

class GxsMsgRelatedTest: public GenExchangeTest {
public:
	GxsMsgRelatedTest();
	virtual ~GxsMsgRelatedTest();

    // request msg related tests
    bool testMsgRelatedChildIdRetrieval();
    bool testMsgRelatedChildDataRetrieval();
    bool testMsgRelatedChildDataRetrieval_Multi();
    bool testMsgAllVersions();
};

#endif /* GXSMSGRELATEDTEST_H_ */
