/*
 * libretroshare/src/test/pqi pqiperson_test.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

/******
 * pqiperson acts as the wrapper for all connection methods to a single peer.
 * 
 * This test creates, a pqiperson and simulates connections, disconnections.
 * packets passing through.
 * 
 */






#include "pqi/pqiperson.h"
#include "pqi/pqibin.h"

#include "testconnect.h"

#include "util/rsnet.h"
#include <iostream>
#include <sstream>
#include "util/utest.h"

INITTEST();

int test_person_basic_reset();
int test_person_basic_listen();


int main(int argc, char **argv)
{
	std::cerr << "pqiperson_test" << std::endl;

	test_person_basic_reset();
	test_person_basic_listen();

	FINALREPORT("pqiperson_test");

	return TESTRESULT();
}

/******************************************************
 *
 * tests()
 *
 **** ensure that functions call children 
 * pqiperson::reset()
 * pqiperson::listen()
 * pqiperson::stoplistening()
 *
 * 
 * pqiperson::connect(...)
 *   -> check that gets through to children.
 *   -> check that cannot have simultaneous connections.
 *   -> 
 *
 */


/*******************************************************
 *
 * Test structure
 *****************/

#define NUM_CHILDREN  3

pqiperson *createTestPerson(std::string id, pqipersongrp *ppg);

class pqipersongrpTestHarness: public pqipersongrp
{
        public:
        pqipersongrpTestHarness(SecurityPolicy *pol, unsigned long flags)
        :pqipersongrp(pol, flags) 
	{ 
		return; 
	}

        protected:

        /********* FUNCTIONS to OVERLOAD for specialisation ********/
virtual pqilistener *createListener(struct sockaddr_in laddr)
{
	return new pqilistener(laddr);	
}

virtual pqiperson   *createPerson(std::string id, pqilistener *listener)
{
	return createTestPerson(id, this);
}

        /********* FUNCTIONS to OVERLOAD for specialisation ********/
};




pqiperson *createTestPerson(std::string id, pqipersongrp *ppg)
{
	/* now add test children */
	{
		std::cerr << "createTestPerson()";
		std::cerr << std::endl;
	}

	pqiperson *pqip = new pqiperson(id, ppg);

	/* construct the serialiser ....
	 * Needs:
	 * * FileItem
	 * * FileData
	 * * ServiceGeneric
	 */

	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsFileItemSerialiser());
	rss->addSerialType(new RsCacheItemSerialiser());
	rss->addSerialType(new RsServiceSerialiser());

        NetBinDummy *dummy1 = new NetBinDummy(pqip, id, PQI_CONNECT_TUNNEL);
	pqiconnect *pqisc = new testConnect(rss, dummy1);
	addTestConnect(pqisc);
	pqip -> addChildInterface(PQI_CONNECT_TCP, pqisc);

	RsSerialiser *rss2 = new RsSerialiser();
	rss2->addSerialType(new RsFileItemSerialiser());
	rss2->addSerialType(new RsCacheItemSerialiser());
	rss2->addSerialType(new RsServiceSerialiser());

        NetBinDummy *dummy2 = new NetBinDummy(pqip, id, PQI_CONNECT_TUNNEL);
	pqiconnect *pqicontun 	= new testConnect(rss2, dummy2);
	addTestConnect(pqicontun);
	pqip -> addChildInterface(PQI_CONNECT_TUNNEL, pqicontun);


        RsSerialiser *rss3 = new RsSerialiser();
        rss3->addSerialType(new RsFileItemSerialiser());
        rss3->addSerialType(new RsCacheItemSerialiser());
        rss3->addSerialType(new RsServiceSerialiser());

        NetBinDummy *dummy3 = new NetBinDummy(pqip, id, PQI_CONNECT_UDP);
        pqiconnect *pqiusc 	= new testConnect(rss3, dummy3);
	addTestConnect(pqiusc);
        pqip -> addChildInterface(PQI_CONNECT_UDP, pqiusc);

	return pqip;
}


/* First Sets of Tests are very basic:
 *   reset()
 *   connect()
 *   listen() / stoplistening()
 *   disconnect()
 */

int test_person_basic_reset()
{
	/* reset test */
	resetTestConnects();

	/* create test person */
	std::string id = "12345678901234567890123456789012";

	pqipersongrp *ppg = pqipersongrpTestHarness(NULL, 0);

	/* setup fake other bits */


	/* add peers - and watch it all happen! */



	pqiperson *person = createTestPerson(id, ppg);

	/* reset person */
	person->reset();

	/* count number of resets */
	CHECK(NUM_CHILDREN == testCounter);

	int i;
	for(i = 0; i < NUM_CHILDREN; i++)
	{
		CHECK(testResetTimes[i].size() == 1);
		CHECK(testConnectState[i] == 0);
		CHECK(testConnectTimes[i].size() == 0);
	}

	/* clean up */
	delete person;

	REPORT("test_person_basic_reset()");

	return 1;
}


int test_person_basic_listen()
{
	/* reset test */
	resetTestConnects();

	/* create test person */
	std::string id = "12345678901234567890123456789012";
	pqipersongrp *ppg = NULL;
	pqiperson *person = createTestPerson(id, ppg);

	/* reset person */
	person->listen();

	/* count number of resets */
	CHECK(NUM_CHILDREN == testCounter);

	int i;
	for(i = 0; i < NUM_CHILDREN; i++)
	{
		CHECK(testResetTimes[i].size() == 0);
		CHECK(testConnectTimes[i].size() == 0);
		CHECK(testConnectState[i] == TST_STATE_LISTEN);
	}

	person->stoplistening();

	for(i = 0; i < NUM_CHILDREN; i++)
	{
		CHECK(testResetTimes[i].size() == 0);
		CHECK(testConnectTimes[i].size() == 0);
		CHECK(testConnectState[i] == 0);
	}

	/* clean up */
	delete person;

	REPORT("test_person_basic_listen()");

	return 1;
}




