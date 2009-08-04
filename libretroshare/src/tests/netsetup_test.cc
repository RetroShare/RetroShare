
#include "pqi/p3connmgr.h"
#include "pqi/p3authmgr.h"
#include "util/utest.h"

#include "upnp/upnphandler.h" 
#include "dht/opendhtmgr.h" 
#include "tcponudp/tou.h"


INITTEST();

int end_test()
{
        FINALREPORT("net_test1");

	exit(TESTRESULT());
}


void printNetworkStatus(p3ConnectMgr *connMgr)
{
	std::cerr << "network status for : " << connMgr->getOwnId() << std::endl;
	std::cerr << "Net Ok:" << connMgr->getNetStatusOk() << std::endl;
	std::cerr << "Upnp Ok:" << connMgr->getNetStatusUpnpOk() << std::endl;
	std::cerr << "DHT Ok:" << connMgr->getNetStatusDhtOk() << std::endl;
	std::cerr << "Ext Ok:" << connMgr->getNetStatusExtOk() << std::endl;
	std::cerr << "Udp Ok:" << connMgr->getNetStatusUdpOk() << std::endl;
	std::cerr << "Tcp Ok:" << connMgr->getNetStatusTcpOk() << std::endl;
	std::cerr << "network status for : " << connMgr->getOwnId() << std::endl;

	peerConnectState state;
	if (connMgr->getOwnNetStatus(state))
	{
		std::string txt = textPeerConnectState(state);
		std::cerr << "State: " << txt << std::endl;
	}
	else
	{
		std::cerr << "No Net Status" << std::endl;
	}
}


void setupTest(int i, p3ConnectMgr *cMgr)
{
	switch(i)
	{
		case 1:
		{
			/* Test One */

		}
		break;

		case 10:
		{
			/* Test One */

		}
		break;

		case 15:
		{
			/* Test One */

		}
		break;

		case 20:
		{
			/* Test One */

		}
		break;

		case 13:
		{
			/* Test One */

		}
		break;

		default:
			std::cerr << "setupTest(" << i << ") no test here" << std::endl;
	}
}

void checkResults(int i, p3ConnectMgr *cMgr)
{
	switch(i)
	{
		/* Test One: Setup - without any support */
		case 1:
		{
			/* Expect UDP ports to be established by now */

			//CHECK(isExternalNet(&loopback_addr)==false);
		}
		break;

		case 10:
		{
			/* Expect Local IP Address to be known */

		}
		break;

		case 15:
		{

		        REPORT("Basic Networking Setup");
		}
		break;

		/* Test Two: DHT Running */
		case 111:
		{
			/* Expect UDP ports to be established by now */

			        //CHECK(isExternalNet(&loopback_addr)==false);
		}
		break;

		case 110:
		{
			/* Expect Local IP Address to be known */

		}
		break;

		/* Test 3:  */
		case 145:
		{

		}
		break;

		case 100:
		{
			/* Test One */

		}
		break;

		case 5000:
		{
			/* End of Tests */
			end_test();
		}
		break;

		default:
		{
			std::cerr << "checkResults(" << i << ") no test here" << std::endl;
			printNetworkStatus(cMgr);

		}
	}
}

class TestMonitor: public pqiMonitor
{
        public:
virtual void    statusChange(const std::list<pqipeer> &plist)
{
	std::cerr << "TestMonitor::statusChange()";
	std::cerr << std::endl;
	std::list<pqipeer>::const_iterator it;
	for(it = plist.begin(); it != plist.end(); it++)
	{
		std::cerr << "Event!";
		std::cerr << std::endl;
	}
}
		
};

int main(int argc, char **argv)
{
	/* options */
	bool enable_upnp = true;
	bool enable_dht  = true;
	bool enable_forward = true;

	/* handle options */
	
	int testtype = 1;
	switch(testtype)
	{
		case 1:
			/* udp test */
			enable_upnp = false;
			enable_forward = false;
			enable_dht = true;
			break;

	}


	std::string ownId = "OWNID";

	/* create a dummy auth mgr */
	p3AuthMgr *authMgr = new p3DummyAuthMgr();
	p3ConnectMgr *connMgr = new p3ConnectMgr(authMgr);

        /* Setup Notify Early - So we can use it. */
        //rsNotify = new p3Notify();

        pqiNetAssistFirewall *upnpMgr = NULL;
        p3DhtMgr  *dhtMgr  = NULL;

	if (enable_upnp)
	{
        	std::cerr << "Switching on UPnP" << std::endl;
        	upnpMgr = new upnphandler();
        	connMgr->addNetAssistFirewall(1, upnpMgr);
	}

	if (enable_dht)
	{
        	p3DhtMgr  *dhtMgr  = new OpenDHTMgr(ownId, connMgr, "./");
        	connMgr->addNetAssistConnect(1, dhtMgr);

        	dhtMgr->start();
        	std::cerr << "Switching on DHT" << std::endl;
        	dhtMgr->enable(true);
	}


        /**************************************************************************/
        /* need to Monitor too! */

	TestMonitor *testmonitor = new TestMonitor();
        connMgr->addMonitor(testmonitor);

	connMgr->checkNetAddress();
	int i;
	for(i=0; 1; i++)
	{
		connMgr->tick();

		setupTest(i, connMgr);

		sleep(1);
		connMgr->tick();

		checkResults(i, connMgr);

		tou_tick_stunkeepalive();
	}
}

