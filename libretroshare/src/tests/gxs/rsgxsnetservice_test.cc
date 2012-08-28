/*
 * rsgxsnetservice_test.cc
 *
 *  Created on: 11 Jul 2012
 *      Author: crispy
 */

#include "util/utest.h"
#include "nxstesthub.h"
#include "nxstestscenario.h"

INITTEST();


int main()
{

	// first setup
	NxsMessageTest msgTest(RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM);
        std::set<std::string> peers;
        peers.insert("PeerA");
        peers.insert("PeerB");
        NxsTestHub hub(&msgTest, peers);

	// now get things started
	createThread(hub);

        double timeDelta = 30;

	// put this thread to sleep for 10 secs
        // make thread sleep for a bit
#ifndef WINDOWS_SYS
        usleep((int) (timeDelta * 1000000));
#else
        Sleep((int) (timeDelta * 1000));
#endif

	hub.join();
	CHECK(hub.testsPassed());

	hub.cleanUp();

    FINALREPORT("RsGxsNetService Tests");

    return TESTRESULT();
}
