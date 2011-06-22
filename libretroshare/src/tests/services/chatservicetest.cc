/*
 * chatservicetest.cpp
 *
 *  Created on: 17 Jun 2011
 *      Author: greyfox
 */

#include "chatservicetest.h"

chatServiceTest::chatServiceTest()
 : ServiceTest() {

	mChat = new p3ChatService(mConnMgr);
}

chatServiceTest::~chatServiceTest() {
	delete mChat;
}

void chatServiceTest::runTests(){

}

void chatServiceTest::insertChatItems(){

}
