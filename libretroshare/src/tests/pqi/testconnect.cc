/*
 * libretroshare/src/test/pqi testconnect.cc
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


#include "testconnect.h"
#include "pqi/pqibin.h"
#include <iostream>
#include <sstream>

/*******************************************************
 *
 * Test structure
 *****************/


#define NUM_CONN_MAX  10
#define NUM_CHILDREN  3

#define TST_STATE_CONNECT	0x0001
#define TST_STATE_LISTEN	0x0002

/* static test variables */
uint32_t testCounter = 0;
pqiconnect *testConnectPointer[NUM_CONN_MAX];
uint32_t testConnectState[NUM_CONN_MAX];
std::list<uint32_t> testConnectTimes[NUM_CONN_MAX];
std::list<struct sockaddr_in> testConnectAddrs[NUM_CONN_MAX];

std::list<uint32_t> testResetState[NUM_CONN_MAX];
std::list<uint32_t> testResetTimes[NUM_CONN_MAX];

uint32_t findWhichConnect(pqiconnect *conn)
{
	int i;
	for(i = 0; i < NUM_CONN_MAX; i++)
	{
		if (testConnectPointer[i] == conn)
		{
			return i;
		}
	}
	// error.
	std::cerr << "Connect Missing" << std::endl;
	exit(1);
	return 0;
}

void addTestConnect(pqiconnect *pqi)
{
	testConnectPointer[testCounter] = pqi;
	testConnectState[testCounter] = 0;
	testCounter++;
	if (testCounter > NUM_CONN_MAX)
	{
		std::cerr << "Too Many Connects" << std::endl;
		exit(1);
	}
}

void resetTestConnects()
{
	testCounter = 0;
	int i;
	for(i = 0; i < NUM_CONN_MAX; i++)
	{
		testConnectAddrs[i].clear();
		testConnectTimes[i].clear();
		testResetState[i].clear();
		testResetTimes[i].clear();
	}
}


testConnect::testConnect(RsSerialiser *rss, NetBinInterface *ni_in)
	:pqiconnect(rss, ni_in)
{

}
	
testConnect::~testConnect() 
{ 
	return;
}

	/* dummyConnect */
int     testConnect::connect(struct sockaddr_in raddr)
{
	int cidx = findWhichConnect(this);
	rstime_t now = time(NULL);

	testConnectState[cidx] |= TST_STATE_CONNECT;
	testConnectTimes[cidx].push_back(now);
	testConnectAddrs[cidx].push_back(raddr);

	std::cerr << "testConnect[" << cidx << "].connect() called";
	std::cerr << std::endl;
	return 0;
}

int     testConnect::listen()
{
	int cidx = findWhichConnect(this);
	testConnectState[cidx] |= TST_STATE_LISTEN;
	std::cerr << "testConnect[" << cidx << "].listen() called";
	std::cerr << std::endl;
	return 1;
}
	
int     testConnect::stoplistening() 
{
	int cidx = findWhichConnect(this);
	testConnectState[cidx] &= ~TST_STATE_LISTEN;
	std::cerr << "testConnect[" << cidx << "].stoplistening() called";
	std::cerr << std::endl;
	return 1;
}
	
int     testConnect::reset()
{
	int cidx = findWhichConnect(this);
	rstime_t now = time(NULL);

	// reset everything except listening.
	testResetState[cidx].push_back(testConnectState[cidx]);
	testResetTimes[cidx].push_back(now);
	testConnectState[cidx] &= TST_STATE_LISTEN; 

	std::cerr << "testConnect[" << cidx << "].reset() called";
	std::cerr << std::endl;
	return 1;
}

// leave this as is 	virtual int     disconnect()            { return ni -> reset(); }
	
bool    testConnect::connect_parameter(uint32_t type, uint32_t value) 
{
	int cidx = findWhichConnect(this);
	//testConnectState[cidx] |= TST_STATE_LISTEN;
	std::cerr << "testConnect[" << cidx << "].connect_parameter() called";
	std::cerr << std::endl;
	return true;
}
	
int     testConnect::getConnectAddress(struct sockaddr_in &raddr)
{
	int cidx = findWhichConnect(this);
	if (testConnectAddrs[cidx].size() > 0)
	{
		raddr = testConnectAddrs[cidx].back();
		return 1;
	}
	std::cerr << "testConnect[" << cidx << "].getConnectAddress() called";
	std::cerr << std::endl;
	return 0;
}

