

#include "pqi/p3connmgr.h"


/***** Test for the new DHT system *****/


#include "util/rsnet.h"
#include "util/rsthreads.h"
#include "util/rsprint.h"
#include "pqi/p3dhtmgr.h"
#include "pqi/p3connmgr.h"
#error secpolicy was removed, also remove it from test to fix compile
#include "pqi/pqisecurity.h"
#include "pqi/pqipersongrp.h"

#include <iostream>
#include <sstream>

#include "tcponudp/udpsorter.h"

/***** Test Framework  *****/

const int NumOfPeers = 10;
std::string peerIds[NumOfPeers] = 
 	{"PEER01",   
 	 "PEER02",   /* Always online, no notify */
 	 "PEER03",   /* notify/online at 20sec */
 	 "PEER04",   /* Always online, notify at 30 sec */
 	 "PEER05",
 	 "PEER06",   /* notify/online at 50sec */
 	 "PEER07",
 	 "PEER08",
 	 "PEER09",   /* notify/online at 80sec */
 	 "PEER10"};

#define STUN_PORT 7777

std::string ownId = "OWNID-AAAA";
rstime_t      ownPublishTs; 

RsMutex frmMtx;
std::list<std::string> searchIds;
std::list<uint32_t>    searchModes;

std::map<std::string, bool> onlineMap;
std::map<uint32_t, std::string> notifyMap;

void initTestData()
{
	ownPublishTs = 0;
	/* setup Peers that are online always */
	bool online;
	uint32_t ts;
	for(int i = 0; i < NumOfPeers; i++)
	{
		online = false;
		if ((i == 1) || (i == 3))
		{
			online = true;
		}
		onlineMap[peerIds[i]] = online;

		if ((i == 2) || (i == 3) || 
			(i == 5) || (i == 8))
		{
			ts = i * 10;
			notifyMap[ts] = peerIds[i];
		}
	}
}

void respondPublish()
{
	frmMtx.lock();   /*   LOCK TEST FRAMEWORK MUTEX */
	if (!ownPublishTs)
	{
		std::cerr << "Own ID first published!" << std::endl;
		ownPublishTs = time(NULL);
	}
	frmMtx.unlock(); /* UNLOCK TEST FRAMEWORK MUTEX */
}

void respondSearch(p3DhtMgr *mgr, std::string id, uint32_t mode)
{
	std::cerr << "Checking for Search Results" << std::endl;
	rstime_t now = time(NULL);
	bool doNotify = false;
	bool doOnline = false;
	std::string notifyId;

	frmMtx.lock();   /*   LOCK TEST FRAMEWORK MUTEX */
	if ((mode == DHT_MODE_NOTIFY) && (ownPublishTs))
	{
		/* */
		std::map<uint32_t, std::string>::iterator it;
		uint32_t delta_t = now - ownPublishTs;
		it = notifyMap.begin();
		if (it != notifyMap.end())
		{
			if (it->first <= delta_t)
			{
				notifyId = it->second;
				onlineMap[notifyId] = true;
				notifyMap.erase(it);
				doNotify = true;
			}
		}
	}
	else if (mode == DHT_MODE_SEARCH)
	{
		
		/* translate */
		std::map<std::string, bool>::iterator mit;
		for(mit = onlineMap.begin(); (mit != onlineMap.end()) &&
					(RsUtil::HashId(mit->first, false) != id); mit++);
		
		if (mit != onlineMap.end())
		{
			doOnline = mit->second;
		}
	}

	frmMtx.unlock(); /* UNLOCK TEST FRAMEWORK MUTEX */

	uint32_t type = 0;
	
	struct sockaddr_in laddr;
	inet_aton("10.0.0.129", &(laddr.sin_addr));
	laddr.sin_port = htons(7812);
	laddr.sin_family = AF_INET;
	
	struct sockaddr_in raddr;
	inet_aton("127.0.0.1", &(raddr.sin_addr));
	raddr.sin_port = htons(STUN_PORT);
	raddr.sin_family = AF_INET;
	
	if (doNotify)
	{
		std::cerr << "Responding to Notify: id:" << notifyId << std::endl;
		mgr->dhtResultNotify(RsUtil::HashId(notifyId, true));
	}
	
	if (doOnline)
	{
		std::cerr << "Responding to Search" << std::endl;
		mgr->dhtResultSearch(id, laddr, raddr, type, "");
	}

}

		
/***** Test Framework  *****/

class DhtMgrTester: public p3DhtMgr
{

	/* Implementation */
	public:

	DhtMgrTester(std::string id, pqiConnectCb *cb)
	:p3DhtMgr(id, cb)
	{
		return;
	}


	
	
	
	/* Blocking calls (only from thread) */
virtual bool dhtPublish(std::string id,
		struct sockaddr_in &laddr, struct sockaddr_in &raddr,
		uint32_t type, std::string sign)
{
	std::cerr << "DhtMgrTester::dhtPublish() id: " << RsUtil::BinToHex(id);
	std::cerr << " laddr: " << rs_inet_ntoa(laddr.sin_addr) << " lport: " << ntohs(laddr.sin_port);
	std::cerr << " raddr: " << rs_inet_ntoa(raddr.sin_addr) << " rport: " << ntohs(raddr.sin_port);
	std::cerr << " type: " << type << " sign: " << sign;
	std::cerr << std::endl;
	
	respondPublish();
	
	return true;
}
	
virtual bool dhtNotify(std::string peerid, std::string ownid, std::string sign)
{
	std::cerr << "DhtMgrTester::dhtNotify() id: " << RsUtil::BinToHex(peerid) << ", ownId: " << RsUtil::BinToHex(ownId);
	std::cerr << " sign: " << sign;
	std::cerr << std::endl;
	
	return true;
}
	
virtual bool dhtSearch(std::string id, uint32_t mode)
{
	std::cerr << "DhtMgrTester::dhtSearch(id: " << RsUtil::BinToHex(id) << ", mode: " << mode << ")" << std::endl;
	
	frmMtx.lock();   /* LOCK TEST FRAMEWORK MUTEX */
	searchIds.push_back(id);
	searchModes.push_back(mode);
	frmMtx.unlock(); /* LOCK TEST FRAMEWORK MUTEX */
	
	return true;
}
	
};


/* OVERLOAD THE ConnMgr - to insert peers */
class p3TestConnMgr: public p3ConnectMgr
{
	public:
	p3TestConnMgr(int mode)
	:p3ConnectMgr(new p3DummyAuthMgr()), mTestMode(mode) { return; }

	protected:
        /* must be virtual for testing */
virtual void    loadConfiguration()
{

	/* setup own address */
	ownState.id = ownId;
	ownState.name = "SELF NAME";
	ownState.localaddr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &(ownState.localaddr.sin_addr));
	ownState.localaddr.sin_port = htons(7812);
	ownState.netMode = RS_NET_MODE_UDP;
	ownState.visState = RS_VIS_STATE_STD;

	/* others not important */
	//ownState.state = 0;
	//ownState.actions = 0;


	if (mTestMode == 1) /* Add to Stun List */
	{
		for(int i = 0; i < NumOfPeers; i++)
		{
			mStunList.push_back(peerIds[i]);
		}
	}
	else if (mTestMode == 2) /* add to peers */
	{
		/* add in as peers */
		//addPeer();
		for(int i = 0; i < NumOfPeers; i++)
		{
			if (i < 5)
			{
				mStunList.push_back(RsUtil::HashId(peerIds[i]));
			}
			else
			{
				addFriend(peerIds[i]);
			}
		}
	}
}

	protected:

	uint32_t mTestMode;
};


int main()
{
	rstime_t startTime = time(NULL);
	/* setup system */
	initTestData();

	/* setup a Stunner to respond to ConnMgr */

	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &(saddr.sin_addr));
	saddr.sin_port = htons(STUN_PORT);
	UdpSorter stunner(saddr); /* starts a receiving thread */

	p3TestConnMgr connMgr(2);
	DhtMgrTester  dhtTester(ownId, &connMgr);

	/* now add in some peers */
	connMgr.setDhtMgr(&dhtTester);
	connMgr.setUpnpMgr(NULL);

	/************ ADD pqipersongrp as pqimonitor *****************/
#error secpolicy was removed, should remove it from tests too
	SecurityPolicy *pol = secpolicy_create();
	unsigned long flags = 0;
	pqipersongrp *pqipg = new pqipersongrpDummy(pol, flags);

	connMgr.addMonitor(pqipg);

	/************ ADD pqipersongrp as pqimonitor *****************/


	/* startup dht */
	std::cerr << "Starting up DhtTester()" << std::endl;
	dhtTester.start();

	/* wait for a little before switching on */
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
	sleep(1);
#else
	Sleep(1000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

	std::cerr << "Switching on DhtTester()" << std::endl;
	dhtTester.setDhtOn(true);

	/* wait loop */
	while(1)
	{
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
		sleep(1);
#else
		Sleep(1000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

		connMgr.tick();
		pqipg->tick();

		/* handle async search */
		frmMtx.lock();   /*   LOCK TEST FRAMEWORK MUTEX */

		std::string id;
		uint32_t  mode;
		bool doRespond = false;
		if (searchIds.size() > 0)
		{
			id   = searchIds.front();
			mode = searchModes.front();
			doRespond = true;
			searchIds.pop_front();
			searchModes.pop_front();
		}

		frmMtx.unlock(); /* UNLOCK TEST FRAMEWORK MUTEX */

		if (doRespond)
		{
			respondSearch(&dhtTester, id, mode);
		}
	}
};










