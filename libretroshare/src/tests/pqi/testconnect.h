#ifndef TEST_PQICONNECT_H
#define TEST_PQICONNECT_H

/*
 * libretroshare/src/test/pqi testconnect.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2010 by Robert Fernie.
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

#include "pqi/pqiperson.h"
//#include "pqi/pqibin.h"
#include "util/rsnet.h"

/*******************************************************
 *
 * Test structure
 *****************/


#define NUM_CONN_MAX  10

#define TST_STATE_CONNECT	0x0001
#define TST_STATE_LISTEN	0x0002

/* static test variables */
extern uint32_t testCounter;
extern pqiconnect *testConnectPointer[NUM_CONN_MAX];
extern uint32_t testConnectState[NUM_CONN_MAX];
extern std::list<uint32_t> testConnectTimes[NUM_CONN_MAX];
extern std::list<struct sockaddr_in> testConnectAddrs[NUM_CONN_MAX];

extern std::list<uint32_t> testResetState[NUM_CONN_MAX];
extern std::list<uint32_t> testResetTimes[NUM_CONN_MAX];

uint32_t findWhichConnect(pqiconnect *conn);
void addTestConnect(pqiconnect *pqi);
void resetTestConnects();

class testConnect: public pqiconnect
{
	public:

	testConnect(RsSerialiser *rss, NetBinInterface *ni_in);
	virtual ~testConnect();
	
		/* dummyConnect */
	virtual int     connect(struct sockaddr_in raddr);
	virtual int     listen();
	virtual int     stoplistening();
	virtual int     reset();
	virtual bool    connect_parameter(uint32_t type, uint32_t value);
	virtual int     getConnectAddress(struct sockaddr_in &raddr);
}; // end of testConnect.


pqiperson *createTestPerson(std::string id, pqipersongrp *ppg);


#endif

