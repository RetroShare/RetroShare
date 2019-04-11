

/***** Test for the new DHT system *****/


#include "pqi/pqinetwork.h"

#include "util/rsnet.h"
#include "util/rsthreads.h"
#include "util/rsprint.h"

#include "pqi/p3dhtmgr.h"

#include <iostream>
#include <sstream>


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

std::string ownId = "AAAA";
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
	inet_aton("10.0.0.19", &(raddr.sin_addr));
	raddr.sin_port = htons(7812);
	raddr.sin_family = AF_INET;

	if (doNotify)
	{
		std::cerr << "Responding to Notify" << std::endl;
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

int main()
{
	rstime_t startTime = time(NULL);
	bool haveOwnAddress = false;
	/* setup system */
	initTestData();

	pqiConnectCbDummy cbTester;
	DhtMgrTester  dhtTester(ownId, &cbTester);

	/* now add in some peers */

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
	dhtTester.enable(true);

	std::cerr << "Adding a List of Peers" << std::endl;
	for(int i = 0; i < NumOfPeers; i++)
	{
		dhtTester.findPeer(peerIds[i]);
	}
		

	/* wait loop */
	while(1)
	{
		std::cerr << "Main waiting..." << std::endl;
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
		sleep(3);
#else
		Sleep(3000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/


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

		if (!haveOwnAddress)
		{
			if (time(NULL) - startTime > 20)
			{
				std::cerr << "Setting Own Address!" << std::endl;
				haveOwnAddress = true;

				uint32_t type = DHT_ADDR_UDP;

				struct sockaddr_in laddr;
				inet_aton("10.0.0.111", &(laddr.sin_addr));
				laddr.sin_port = htons(7812);
				laddr.sin_family = AF_INET;

				struct sockaddr_in raddr;
				inet_aton("10.0.0.11", &(raddr.sin_addr));
				raddr.sin_port = htons(7812);
				raddr.sin_family = AF_INET;

				dhtTester.setExternalInterface(laddr, raddr, type);
			}
		}

	}
};










