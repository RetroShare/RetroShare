/*
 * libretroshare/src/test/pqi p3connmgr_test.cc
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
 * p3connmgr test module.
 *
 * create a p3connmgr and run the following tests.
 * 1) UDP test
 * 2) UPNP test
 * 3) ExtAddr test.
 * 4) full reset in between.
 * 
 */


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>




//#include "pqi/p3connmgr.h"
#include "pqi/authssltest.h"
#include "pqi/authgpgtest.h"

#include "pqi/p3dhtmgr.h"
#include "upnp/upnphandler_linux.h"

#include "util/rsnet.h"
#include <iostream>
#include <sstream>
#include "util/utest.h"

INITTEST();

/* generic startup test */
#define MAX_TIME_BASIC_TEST 40
#define MAX_TIME_UPNP_TEST  700 /* seems to take a while */

#define RESTART_EXPECT_NO_EXT_ADDR		1
#define RESTART_EXPECT_EXTFINDER_ADDR		2
#define RESTART_EXPECT_UPNP_ADDR		3
#define RESTART_EXPECT_DHT_ADDR			4

int test_p3connmgr_restart_test(uint32_t expectState, rstime_t timeout);


#define RESET_VIA_LOCAL_ADDR		1
#define RESET_VIA_REMOTE_ADDR		2


int force_reset(uint32_t method);


/*******************************************************
 *
 * Test structure
 *****************/

#define FLAG_UPNP	1
#define FLAG_UDP	2
#define FLAG_DHT	4
#define FLAG_EXT	8

p3ConnectMgr *mConnMgr;
pqiNetAssistFirewall *mUpnpMgr = NULL;
p3DhtMgr *mDhtMgr = NULL;

void createP3ConnMgr(std::string id, uint32_t testFlags)
{
	/* now add test children */
	{
		std::cerr << "createP3ConnMgr()";
		std::cerr << std::endl;
	}

	mConnMgr = new p3ConnectMgr();

	mDhtMgr = NULL; //new p3DhtMgr();

	/* setup status */
	//mConnMgr->setStatus(UPNP);
}

void add_upnp()
{
	pqiNetAssistFirewall *mUpnpMgr = new upnphandler();
	mDhtMgr = NULL; //new p3DhtMgr();

        //mConnMgr->addNetAssistConnect(1, mDhtMgr);
        mConnMgr->addNetAssistFirewall(1, mUpnpMgr);
}

void disableUpnp()
{
	//mConnMgr->
}


void enableUpnp()
{



}

void disableExtFinder()
{
	//mConnMgr->
	mConnMgr->setIPServersEnabled(false);
}


void enableExtFinder()
{
	mConnMgr->setIPServersEnabled(true);
}


/* ACTUAL TEST */
int main(int argc, char **argv)
{
	/* test p3connmgr net stuff */
	std::cerr << "p3connmgr_net_restart_test" << std::endl;

	// setup test authssl.
	setAuthGPG(new AuthGPGtest());
	setAuthSSL(new AuthSSLtest());

	createP3ConnMgr("abcd", 0);
	pqiNetStatus status;


	/* first test, no Upnp / ExtFinder -> expect no Ext Address */
	mConnMgr->setNetworkMode(AuthSSL::getAuthSSL()->OwnId(), RS_NET_MODE_UDP);
	disableExtFinder();

	mConnMgr->getNetStatus(status);

	CHECK(status.mLocalAddrOk == false);
	CHECK(status.mExtAddrOk == false);
	test_p3connmgr_restart_test(RESTART_EXPECT_NO_EXT_ADDR, MAX_TIME_BASIC_TEST);

	REPORT("test_p3connmgr_restart_test()");

	/* second test, add ExtFinder -> expect Ext Address */
	enableExtFinder();
	mConnMgr->setNetworkMode(AuthSSL::getAuthSSL()->OwnId(), RS_NET_MODE_UDP);
	force_reset(RESET_VIA_LOCAL_ADDR);

	CHECK(status.mLocalAddrOk == false);
	CHECK(status.mExtAddrOk == false);
	test_p3connmgr_restart_test(RESTART_EXPECT_EXTFINDER_ADDR, MAX_TIME_BASIC_TEST);

	/* third test. disable ExtFinder again -> expect No Ext Address */
	disableExtFinder();
	mConnMgr->setNetworkMode(AuthSSL::getAuthSSL()->OwnId(), RS_NET_MODE_UDP);
	force_reset(RESET_VIA_LOCAL_ADDR);
	CHECK(status.mLocalAddrOk == false);
	CHECK(status.mExtAddrOk == false);
	test_p3connmgr_restart_test(RESTART_EXPECT_NO_EXT_ADDR, MAX_TIME_BASIC_TEST);


	/* fourth test. enable Upnp -> expect Upnp Ext Address */
	add_upnp();
	enableUpnp();
	mConnMgr->setNetworkMode(AuthSSL::getAuthSSL()->OwnId(), RS_NET_MODE_UPNP);
	force_reset(RESET_VIA_LOCAL_ADDR);
	CHECK(status.mLocalAddrOk == false);
	CHECK(status.mExtAddrOk == false);


	test_p3connmgr_restart_test(RESTART_EXPECT_UPNP_ADDR, MAX_TIME_UPNP_TEST);
	/* fifth test. disable Upnp -> expect No Ext Address */
	disableUpnp();
	mConnMgr->setNetworkMode(AuthSSL::getAuthSSL()->OwnId(), RS_NET_MODE_UDP);
	force_reset(RESET_VIA_LOCAL_ADDR);
	CHECK(status.mLocalAddrOk == false);
	CHECK(status.mExtAddrOk == false);

	test_p3connmgr_restart_test(RESTART_EXPECT_NO_EXT_ADDR, MAX_TIME_BASIC_TEST);
	/* sixth test. enable both Ext and Upnp -> expect UpnP Ext Address (prefered) */	
	enableExtFinder();
	enableUpnp();
	mConnMgr->setNetworkMode(AuthSSL::getAuthSSL()->OwnId(), RS_NET_MODE_UPNP);
	force_reset(RESET_VIA_LOCAL_ADDR);
	CHECK(status.mLocalAddrOk == false);
	CHECK(status.mExtAddrOk == false);

	test_p3connmgr_restart_test(RESTART_EXPECT_UPNP_ADDR, MAX_TIME_UPNP_TEST);
	REPORT("test_p3connmgr_restart_test()");

	FINALREPORT("p3connmgr_net_restart_test");

	return TESTRESULT();
}

/******************************************************
 *
 */

int force_reset(uint32_t method)
{
	/* force reset network */
	struct sockaddr_in tst_addr;
	inet_aton("123.45.2.2", &(tst_addr.sin_addr));
	tst_addr.sin_port = ntohs(8461);

	mConnMgr->setLocalAddress(AuthSSL::getAuthSSL()->OwnId(), tst_addr);

	return 1;
}

/* Generic restart test */
int test_p3connmgr_restart_test(uint32_t expectState, rstime_t timeout)
{
	/* force reset network */
	struct sockaddr_in tst_addr;
	inet_aton("123.45.2.2", &(tst_addr.sin_addr));
	tst_addr.sin_port = ntohs(8461);

	mConnMgr->setLocalAddress(AuthSSL::getAuthSSL()->OwnId(), tst_addr);

	/* tick */
	rstime_t start = time(NULL);
	bool extAddr = false;

	while ((start > time(NULL) - timeout) && (!extAddr))
	{
		mConnMgr->tick();

		pqiNetStatus status;
		mConnMgr->getNetStatus(status);
		std::cerr << "test_p3connmgr_restart_test() Age: " << time(NULL) - start;
		std::cerr << " netStatus:";
		std::cerr << std::endl;
		status.print(std::cerr);

		if (status.mExtAddrOk)
		{
			std::cerr << "test_p3connmgr_restart_test() Got ExtAddr. Finished Restart.";
			std::cerr << std::endl;
			extAddr = true;
		}
		sleep(1);
	}

	std::cerr << "test_p3connmgr_restart_test() Test Mode: " << expectState << " Complete";
	std::cerr << std::endl;

	pqiNetStatus status;
	mConnMgr->getNetStatus(status);
	status.print(std::cerr);
	if (status.mExtAddrOk)
	{
		CHECK(isValidNet(&(status.mExtAddr.sin_addr)));
	}
	CHECK(isValidNet(&(status.mLocalAddr.sin_addr)));

	/* check expectState */
	switch(expectState)
	{
		default:
		case RESTART_EXPECT_NO_EXT_ADDR:
			CHECK(status.mLocalAddrOk == true);
			CHECK(status.mExtAddrOk == false);
			CHECK(status.mExtAddrStableOk == false);
			CHECK(status.mUpnpOk == false);
			CHECK(status.mDhtOk == false);
		break;

		case RESTART_EXPECT_EXTFINDER_ADDR:
			CHECK(status.mLocalAddrOk == true);
			CHECK(status.mExtAddrOk == true);
			CHECK(status.mExtAddrStableOk == false);
			CHECK(status.mUpnpOk == false);
		break;

		case RESTART_EXPECT_UPNP_ADDR:
			CHECK(status.mLocalAddrOk == true);
			CHECK(status.mExtAddrOk == true);
			CHECK(status.mExtAddrStableOk == true);
			CHECK(status.mUpnpOk == true);
		break;

		case RESTART_EXPECT_DHT_ADDR:
			CHECK(status.mLocalAddrOk == true);
			CHECK(status.mExtAddrOk == false);
			CHECK(status.mUpnpOk == false);
			CHECK(status.mDhtOk == true);
		break;
	}

	return 1;
}


