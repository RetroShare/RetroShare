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


#include "ppg_harness.h"
#include "conn_harness.h"

#include "pqi/p3connmgr.h"
#include "pqi/authssltest.h"
#include "pqi/authgpgtest.h"

#include "pqi/p3dhtmgr.h"
#include "upnp/upnphandler.h"

#include "util/rsnet.h"
#include <iostream>
#include <sstream>
#include "util/utest.h"

INITTEST();

/* generic startup test */
#define MAX_TIME_SHORT_TEST 10
#define MAX_TIME_BASIC_TEST 60

int run_connect_test(rstime_t timeout);

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


/* ACTUAL TEST */
int main(int argc, char **argv)
{
	/* test p3connmgr net stuff */
	std::cerr << "p3connmgr_net_connect_test" << std::endl;

	// setup test authssl.
	setAuthGPG(new AuthGPGtest());
	setAuthSSL(new AuthSSLtest());

	createP3ConnMgr("abcd", 0);
	pqiNetStatus status;

	setupPqiPersonGrpTH();

	/* install ppg as a monitor */

	mConnMgr->addMonitor(mPqiPersonGrpTH);


	std::string peer1_gpgid = "PEERID0001_GPG__";
	std::string peer1_sslid = "PEERID0001_SSL__";
	
	/* first test, add a single peer (no Addresses) */
	mConnMgr->addFriend(peer1_sslid, peer1_gpgid);

	run_connect_test(MAX_TIME_SHORT_TEST);

	mConnMgr->getNetStatus(status);

	CHECK(status.mLocalAddrOk == false);
	CHECK(status.mExtAddrOk == false);

	// Check that there have been... 1 reset / 0 connects / 1 listen
	// per connect.

	REPORT("p3connmgr_connect_test() SINGLE PEER - NO Addresses");
	/* second test, add addresses for peer */

	rstime_t now = time(NULL);

	pqiIpAddrSet peer1_tst2_addrs;
	pqiIpAddress paddr;
	inet_aton("192.168.1.1", &(paddr.mAddr.sin_addr));
	paddr.mAddr.sin_port = htons(6411);
	paddr.mSeenTime = now - 10;
	peer1_tst2_addrs.mLocal.updateIpAddressList(paddr);

	inet_aton("192.168.1.2", &(paddr.mAddr.sin_addr));
	paddr.mAddr.sin_port = htons(6422);
	paddr.mSeenTime = now - 20;
	peer1_tst2_addrs.mLocal.updateIpAddressList(paddr);

	inet_aton("23.56.25.1", &(paddr.mAddr.sin_addr));
	paddr.mAddr.sin_port = htons(6511);
	paddr.mSeenTime = now - 30;
	peer1_tst2_addrs.mExt.updateIpAddressList(paddr);

	inet_aton("23.56.25.2", &(paddr.mAddr.sin_addr));
	paddr.mAddr.sin_port = htons(6522);
	paddr.mSeenTime = now - 40;
	peer1_tst2_addrs.mExt.updateIpAddressList(paddr);
	
	mConnMgr->updateAddressList(peer1_sslid, peer1_tst2_addrs);
	mConnMgr->retryConnect(peer1_sslid);

	run_connect_test(MAX_TIME_BASIC_TEST);

	// Check that there have been:
	// 0 Listens / 0 resets.
	// 0 connect attempts on UDP
	// 4 connect attempts on TCP
	// 0 connect attempts on TUNNEL
	REPORT("p3connmgr_connect_test() SINGLE PEER - 4 Addresses");

	/* third test, add an second peer with addresses */

	/* fourth test, setup default action as connect */
	setDefaultTestConnectAction(TST_ACTION_SUCCESS);
}

/* Generic restart test */
int run_connect_test(rstime_t timeout)
{
	/* tick */
	rstime_t start = time(NULL);
	bool extAddr = false;

	while ((start > time(NULL) - timeout) && (!extAddr))
	{
		mConnMgr->tick();
		tickPqiPersonGrpTH();
	
		sleep(1);
	}
	return 1;
}


