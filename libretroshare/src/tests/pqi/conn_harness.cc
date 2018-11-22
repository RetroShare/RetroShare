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


#include "conn_harness.h"
#include "pqi/pqibin.h"
#include <iostream>
#include <sstream>

/*******************************************************
 *
 * Test structure
 *****************/


/* static test variables */
uint32_t testCounter = 0;
pqiconnect *testConnectPointer[NUM_CONN_MAX];
PQInterface *testConnectParent[NUM_CONN_MAX];
uint32_t testConnectState[NUM_CONN_MAX];
std::list<uint32_t> testConnectTimes[NUM_CONN_MAX];
std::list<struct sockaddr_in> testConnectAddrs[NUM_CONN_MAX];

std::list<uint32_t> testResetState[NUM_CONN_MAX];
std::list<uint32_t> testResetTimes[NUM_CONN_MAX];

uint32_t testConnectAction[NUM_CONN_MAX] = { 0 };
rstime_t   testConnectPeriod[NUM_CONN_MAX] = { 0 };

uint32_t defTestConnectAction = TST_ACTION_FAILED;


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

void addTestConnect(pqiconnect *pqi, PQInterface *parent)
{
	testConnectPointer[testCounter] = pqi;
	testConnectParent[testCounter] = parent;
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

/* setup callback actions */
void setDefaultTestConnectAction(uint32_t action)
{
	defTestConnectAction = action;
}

void forceConnect(uint32_t idx)
{
	/* flag as con */
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
	testConnectAction[cidx] = defTestConnectAction;

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

	/* sum up PERIOD and DELAY */
	if (type == NET_PARAM_CONNECT_DELAY)
	{
		std::cerr << "PARAMETER: DELAY: " << value;
		std::cerr << std::endl;

		//testConnectPeriod[cidx] = value;
	}
	else if (type == NET_PARAM_CONNECT_PERIOD)
	{
		std::cerr << "PARAMETER: PERIOD: " << value;
		std::cerr << std::endl;

		//testConnectPeriod[cidx] += value;
	}
	else if (type == NET_PARAM_CONNECT_TIMEOUT)
	{
		std::cerr << "PARAMETER: TIMEOUT: " << value;
		std::cerr << std::endl;

		testConnectPeriod[cidx] = value;
		//testConnectPeriod[cidx] += value;
	}
	std::cerr << "total testConnectPeriod: " << testConnectPeriod[cidx];
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


int    testConnect::tick()
{
	int cidx = findWhichConnect(this);

	std::cerr << "testConnect[" << cidx << "].tick()";
	std::cerr << std::endl;

	if (testConnectState[cidx] & TST_STATE_CONNECT)
	{
		rstime_t now = time(NULL);
		rstime_t cTime = testConnectTimes[cidx].back();
		if (now - cTime > testConnectPeriod[cidx])
		{

			std::cerr << "testConnect[" << cidx << "].tick() reached ConnectPeriod";
			std::cerr << " age: " << now - cTime << " period: " << testConnectPeriod[cidx];
			std::cerr << std::endl;

			PQInterface *connParent = testConnectParent[cidx];

			testConnectState[cidx] &= ~TST_STATE_CONNECT;
			/* do callback now */
			if (testConnectAction[cidx] == TST_ACTION_SUCCESS)
			{
				std::cerr << "testConnect[" << cidx << "].tick() CONNECTED callback";
				std::cerr << std::endl;

				testConnectState[cidx] &= ~TST_STATE_CONNECTED;
				/* do callback */
		                connParent -> notifyEvent(this->ni, NET_CONNECT_SUCCESS);
	
			}
			else if (testConnectAction[cidx] == TST_ACTION_FAILED)
			{
				std::cerr << "testConnect[" << cidx << "].tick() FAILED callback";
				std::cerr << std::endl;

				/* do callback */
		                connParent -> notifyEvent(this->ni, NET_CONNECT_FAILED);

			}
			else
			{
				std::cerr << "testConnect[" << cidx << "].tick() no callback (BAD)";
				std::cerr << std::endl;
			}

		}
	}
	return 0;
}





