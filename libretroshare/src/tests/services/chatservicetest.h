/*
 * chatservicetest.h
 *
 *  Created on: 17 Jun 2011
 *      Author: greyfox
 */

#ifndef CHATSERVICETEST_H_
#define CHATSERVICETEST_H_

#include "servicetest.h"
#include "services/p3chatservice.h"

class chatServiceTest: public ServiceTest {

public:
	chatServiceTest();
	virtual ~chatServiceTest();

public:
	void runTests();

private:

	p3ChatService* mChat;

};

#endif /* CHATSERVICETEST_H_ */
