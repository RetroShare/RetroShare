
#include "peernet.h"
#include <stdio.h>
#include <iostream>
#include <sstream>


#include "bitdht/bdstddht.h"
//#include "udp/udplayer.h"

#include "tcponudp/tou.h"
#include "util/rsnet.h"


PeerNet::PeerNet(std::string id, std::string configpath, uint16_t port)
{
	mDoUdpStackRestrictions = false;
        mLocalNetTesting = false;
	mMinuteTS = 0;

        std::cerr << "PeerNet::PeerNet()" << std::endl;
        std::cerr << "Using Id: " << id;
        std::cerr << std::endl;
        std::cerr << "Using Config Path: " << configpath;
        std::cerr << std::endl;
        std::cerr << "Converting OwnId to bdNodeId....";
        std::cerr << std::endl;

	std::cerr << "Loading Configuration";	
	std::cerr << std::endl;

	mConfigFile = configpath + "/peerconfig.txt";
	mPeersFile = configpath + "/peerlist.txt";
	mBootstrapFile = configpath + "/bdboot.txt";

	srand(time(NULL));

	mPort = 10240 + (rand() %  10240);
	if (!loadConfig(mConfigFile))
	{
		std::cerr << "Failed to loadConfig, Creating random Id/Port";
		std::cerr << std::endl;

		/* */
		bdStdRandomNodeId(&mOwnId);
	}

	if (!bdStdLoadNodeId(&mOwnId, id))
	{
		std::cerr << "Failed to load Id from FnParameters";	
		std::cerr << std::endl;
	}

	if (port > 1024)
	{
		mPort = port;
	}


        std::cerr << "Input Id: " << id;
        std::cerr << std::endl;

        std::cerr << "Own NodeId: ";
        bdStdPrintNodeId(std::cerr, &mOwnId);
        std::cerr << std::endl;

}

void PeerNet::setUdpStackRestrictions(std::list<std::pair<uint16_t, uint16_t> > &restrictions)
{
	mDoUdpStackRestrictions = true;
	mUdpStackRestrictions = restrictions;
}

void PeerNet::setProxyUdpStackRestrictions(std::list<std::pair<uint16_t, uint16_t> > &restrictions)
{
	mDoProxyUdpStackRestrictions = true;
	mProxyUdpStackRestrictions = restrictions;
}

void PeerNet::setLocalTesting()
{
	mLocalNetTesting = true;
}

void PeerNet::init()
{
        /* standard dht behaviour */
        std::string dhtVersion = "RS52"; // should come from elsewhere!
        bdDhtFunctions *stdfns = new bdStdDht();

        std::cerr << "PeerNet() startup ... creating UdpStack";
        std::cerr << std::endl;


        struct sockaddr_in tmpladdr;
        sockaddr_clear(&tmpladdr);
        tmpladdr.sin_port = htons(mPort);

	if (mDoUdpStackRestrictions)
	{
		mUdpStack = new UdpStack(UDP_TEST_RESTRICTED_LAYER, tmpladdr);
		RestrictedUdpLayer *url = (RestrictedUdpLayer *) mUdpStack->getUdpLayer();

		std::list<std::pair<uint16_t, uint16_t> >::iterator it;
		for(it = mUdpStackRestrictions.begin(); it != mUdpStackRestrictions.end(); it++)
		{
			url->addRestrictedPortRange(it->first, it->second);
		}
	}
	else
	{
        	mUdpStack = new UdpStack(tmpladdr);
	}

        std::cerr << "PeerNet() startup ... creating Advanced Stack";
        std::cerr << std::endl;

	/* ADVANCED STACK BUILDING! */

	/* construct the rest of the stack, important to build them in the correct order! */
	/* MOST OF THIS IS COMMENTED OUT UNTIL THE REST OF libretroshare IS READY FOR IT! */

#define PN_TOU_RECVER_DIRECT_IDX	0
#define PN_TOU_RECVER_PROXY_IDX		1
#define PN_TOU_RECVER_RELAY_IDX		2

#define PN_TOU_NUM_RECVERS		3

	UdpSubReceiver *udpReceivers[PN_TOU_NUM_RECVERS];
	int udpTypes[PN_TOU_NUM_RECVERS];

	
	std::cerr << "PeerNet() startup ... creating UdpStunner on UdpStack";
	std::cerr << std::endl;

	// STUNNER.
	mDhtStunner = new UdpStunner(mUdpStack);
	mUdpStack->addReceiver(mDhtStunner);
	//mDhtStunner->setTargetStunPeriod(0); /* passive */
	mDhtStunner->setTargetStunPeriod(300); /* very slow (300 = 5minutes) */
	
	std::cerr << "PeerNet() startup ... creating BitDHT on UdpStack";
	std::cerr << std::endl;

	// BITDHT.
	mUdpBitDht = new UdpBitDht(mUdpStack, &mOwnId, dhtVersion, mBootstrapFile, stdfns);
	mUdpStack->addReceiver(mUdpBitDht);

	/* setup callback to here */
	mUdpBitDht->addCallback(this);
	mUdpBitDht->ConnectionOptions(BITDHT_CONNECT_MODE_DIRECT |
		BITDHT_CONNECT_MODE_PROXY | BITDHT_CONNECT_MODE_RELAY, 0);
	
	std::cerr << "PeerNet() startup ... creating UdpRelayReceiver on UdpStack";
	std::cerr << std::endl;

	// NEXT THE RELAY (NEED to keep a reference for installing RELAYS)
	mRelayReceiver = new UdpRelayReceiver(mUdpStack); 
	udpReceivers[PN_TOU_RECVER_RELAY_IDX] = mRelayReceiver; /* RELAY Connections (DHT Port) */
	udpTypes[PN_TOU_RECVER_RELAY_IDX] = TOU_RECEIVER_TYPE_UDPRELAY;
	mUdpStack->addReceiver(udpReceivers[PN_TOU_RECVER_RELAY_IDX]);
	
	std::cerr << "PeerNet() startup ... creating UdpPeerReceiver on UdpStack";
	std::cerr << std::endl;
	
	// LAST ON THIS STACK IS STANDARD DIRECT TOU
	udpReceivers[PN_TOU_RECVER_DIRECT_IDX] = new UdpPeerReceiver(mUdpStack);  /* standard DIRECT Connections (DHT Port) */
	udpTypes[PN_TOU_RECVER_DIRECT_IDX] = TOU_RECEIVER_TYPE_UDPPEER;
	mUdpStack->addReceiver(udpReceivers[PN_TOU_RECVER_DIRECT_IDX]);

	std::cerr << "PeerNet() startup ... creating UdpProxyStack";
	std::cerr << std::endl;

	// NOW WE BUILD THE SECOND STACK.
	// Create the Second UdpStack... Port should be random (but openable!).
	struct sockaddr_in sndladdr;
	sockaddr_clear(&sndladdr);
	sndladdr.sin_port = htons(mPort + 11);
	//rsUdpStack *mUdpProxyStack = NULL;

	if (mDoProxyUdpStackRestrictions)
	{
		mUdpProxyStack = new UdpStack(UDP_TEST_RESTRICTED_LAYER, sndladdr);
		RestrictedUdpLayer *url = (RestrictedUdpLayer *) mUdpProxyStack->getUdpLayer();

		std::list<std::pair<uint16_t, uint16_t> >::iterator it;
		for(it = mProxyUdpStackRestrictions.begin(); it != mProxyUdpStackRestrictions.end(); it++)
		{
			url->addRestrictedPortRange(it->first, it->second);
		}
	}
	else
	{
        	mUdpProxyStack = new UdpStack(sndladdr);
	}

        std::cerr << "PeerNet() startup ... creating UdpStunner on UdpProxyStack";
        std::cerr << std::endl;
	
	// FIRSTLY THE PROXY STUNNER.
	mProxyStunner = new UdpStunner(mUdpProxyStack);
        mUdpProxyStack->addReceiver(mProxyStunner);
	//mProxyStunner->setTargetStunPeriod(0); /* passive */

        std::cerr << "PeerNet() startup ... creating UdpPeerReceiver(Proxy) on UdpProxyStack";
        std::cerr << std::endl;
	
	// FINALLY THE PROXY UDP CONNECTIONS
	udpReceivers[PN_TOU_RECVER_PROXY_IDX] = new UdpPeerReceiver(mUdpProxyStack); /* PROXY Connections (Alt UDP Port) */	
	udpTypes[PN_TOU_RECVER_PROXY_IDX] = TOU_RECEIVER_TYPE_UDPPEER;	
	mUdpProxyStack->addReceiver(udpReceivers[PN_TOU_RECVER_PROXY_IDX]);
	
	// NOW WE CAN PASS THE RECEIVERS TO TOU.
	tou_init((void **) udpReceivers, udpTypes,  PN_TOU_NUM_RECVERS);


	/* startup the Udp stuff! */
        mUdpBitDht->start();
	mUdpBitDht->startDht();

	// handle configuration.
	loadPeers(mPeersFile);
	storeConfig(mConfigFile);


	/* enable local net stuns (for testing) */
	if (mLocalNetTesting)
	{
		mProxyStunner->SetAcceptLocalNet();
		mDhtStunner->SetAcceptLocalNet();
	}


}

int PeerNet::getOwnId(bdNodeId *id)
{
	*id = mOwnId;
	return 1;
}

int PeerNet::add_peer(std::string id)
{
	bdNodeId tmpId;

        if (!bdStdLoadNodeId(&tmpId, id))
        {
                std::cerr << "PeerNet::add_peer() Failed to load own Id";
                std::cerr << std::endl;
		return 0;
        }
	
	std::ostringstream str;
	bdStdPrintNodeId(str, &tmpId);
	std::string filteredId = str.str();

	std::cerr << "PeerNet::add_peer()";
	std::cerr << std::endl;

        std::cerr << "Input Id: " << id;
        std::cerr << std::endl;
        std::cerr << "Filtered Id: " << id;
        std::cerr << std::endl;
        std::cerr << "Final NodeId: ";
        bdStdPrintNodeId(std::cerr, &tmpId);
        std::cerr << std::endl;

	bool addedPeer = false;

	{
		bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	  if (mPeers.end() == mPeers.find(filteredId))
	  {
		std::cerr << "Adding New Peer: " << filteredId << std::endl;
		mPeers[filteredId] = PeerStatus();
		std::map<std::string, PeerStatus>::iterator it = mPeers.find(filteredId);

		//mUdpBitDht->addFindNode(&tmpId, BITDHT_QFLAGS_DO_IDLE);
		mUdpBitDht->addFindNode(&tmpId, BITDHT_QFLAGS_DO_IDLE | BITDHT_QFLAGS_UPDATES);

		it->second.mId = filteredId;
		bdsockaddr_clear(&(it->second.mDhtAddr));
		it->second.mDhtStatusMsg = "Just Added";	
		it->second.mDhtState = PN_DHT_STATE_SEARCHING;
		it->second.mDhtUpdateTS = time(NULL);

		// Initialise Everything.
		it->second.mConnectLogic.mPeerId = filteredId;

		it->second.mPeerReqStatusMsg = "Just Added";
		it->second.mPeerReqState = PN_PEER_REQ_STOPPED;
		  it->second.mPeerReqMode = 0;
		  //it->second.mPeerReqProxyId;
		it->second.mPeerReqTS = time(NULL);

		it->second.mPeerCbMsg = "No CB Yet";
		it->second.mPeerCbMode = 0;
		it->second.mPeerCbPoint = 0;
		  //it->second.mPeerCbProxyId = 0;
		  //it->second.mPeerCbDestId = 0;
		  it->second.mPeerCbTS = 0;

		it->second.mPeerConnectState = PN_PEER_CONN_DISCONNECTED;
		it->second.mPeerConnectMsg = "Disconnected";
		  it->second.mPeerConnectFd = 0;
		  it->second.mPeerConnectMode = 0;
		  //it->second.mPeerConnectProxyId;
		  it->second.mPeerConnectPoint = 0;
		  
		  it->second.mPeerConnectUdpTS = 0;
		  it->second.mPeerConnectTS = 0;
		  it->second.mPeerConnectClosedTS = 0;

		bdsockaddr_clear(&(it->second.mPeerConnectAddr));

		addedPeer = true;
	  }

		/* remove from FailedPeers */
	  std::map<std::string, PeerStatus>::iterator it = mFailedPeers.find(filteredId);
	  if (it != mFailedPeers.end())
	  {
		mFailedPeers.erase(it);
	  }
	}

	if (addedPeer)
	{
		// outside of mutex.
		storePeers(mPeersFile);
		return 1;
	}

	std::cerr << "Peer Already Exists, ignoring: " << filteredId << std::endl;

	return 0;
}

int PeerNet::remove_peer(std::string id)
{
	bdNodeId tmpId;

        if (!bdStdLoadNodeId(&tmpId, id))
        {
                std::cerr << "PeerNet::remove_peer() Failed to load own Id";
                std::cerr << std::endl;
		return 0;
        }

	std::ostringstream str;
	bdStdPrintNodeId(str, &tmpId);
	std::string filteredId = str.str();

	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	std::map<std::string, PeerStatus>::iterator it = mPeers.find(filteredId);

	if (mPeers.end() != it)
	{
		mUdpBitDht->removeFindNode(&tmpId);
		mPeers.erase(it);
		return 1;
	}
	return 0;
}

int PeerNet::get_dht_peers(int lvl, bdBucket &peers)
{
	/* must be able to access deep into bitdht, which isn't possible 
	 * at the moment! ... add this functionality now!
	 */

	return mUdpBitDht->getDhtBucket(lvl, peers);
}


std::string PeerNet::getDhtStatusString()
{
	std::ostringstream out;
	int state = mUdpBitDht->stateDht();

	switch(state)
	{
		default:
			out << "Unknown State: " << state << " ";
			break;
		case BITDHT_MGR_STATE_OFF:
			out << "BitDHT OFF ";
			break;
		case BITDHT_MGR_STATE_STARTUP:
			out << "BitDHT Startup ";
			break;
		case BITDHT_MGR_STATE_FINDSELF:
			out << "BitDHT FindSelf ";
			break;
		case BITDHT_MGR_STATE_ACTIVE:
			out << "BitDHT Active ";
			break;
		case BITDHT_MGR_STATE_REFRESH:
			out << "BitDHT Refresh ";
			break;
		case BITDHT_MGR_STATE_QUIET:
			out << "BitDHT Quiet ";
			break;
		case BITDHT_MGR_STATE_FAILED:
			out << "BitDHT Failed ";
			break;
	}


	out << " BitNetSize: " << mUdpBitDht->statsNetworkSize();
	out << " LocalNetSize: " << mUdpBitDht->statsBDVersionSize();

	return out.str();
}


std::string PeerNet::getPeerStatusString()
{
	std::ostringstream out;

	out << "OwnId: ";
	bdStdPrintNodeId(out, &mOwnId);	

	return out.str();
}

std::string PeerNet::getPeerAddressString()
{
	std::ostringstream out;

	out << " LocalPort: " << mPort;

	struct sockaddr_in extAddr;
	uint8_t extStable;
	if (mDhtStunner->externalAddr(extAddr, extStable))
	{
		out << " DhtExtAddr: " << inet_ntoa(extAddr.sin_addr);
                out << ":" << ntohs(extAddr.sin_port);

		if (extStable)
		{
			out << " (Stable) ";
		}
		else
		{
			out << " (Unstable) ";
		}
	}
	else
	{
		out << " DhtExtAddr: Unknown ";
	}
	if (mProxyStunner->externalAddr(extAddr, extStable))
	{
		out << " ProxyExtAddr: " << inet_ntoa(extAddr.sin_addr);
                out << ":" << ntohs(extAddr.sin_port);

		if (extStable)
		{
			out << " (Stable) ";
		}
		else
		{
			out << " (Unstable) ";
		}
	}
	else
	{
		out << " ProxyExtAddr: Unknown ";
	}

	return out.str();
}

uint32_t PeerNet::getNetStateNetworkMode()
{
	return mNetStateBox.getNetworkMode();
}

uint32_t PeerNet::getNetStateNatTypeMode()
{
	return mNetStateBox.getNatTypeMode();
}

uint32_t PeerNet::getNetStateNatHoleMode()
{
	return mNetStateBox.getNatHoleMode();
}

uint32_t PeerNet::getNetStateConnectModes()
{
	return mNetStateBox.getConnectModes();
}

uint32_t PeerNet::getNetStateNetStateMode()
{
	return mNetStateBox.getNetStateMode();
}


int PeerNet::get_net_peers(std::list<std::string> &peerIds)
{
	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	std::map<std::string, PeerStatus>::iterator it;
	for(it = mPeers.begin(); it != mPeers.end(); it++)
	{
		peerIds.push_back(it->first);
	}
	return 1;
}

int PeerNet::get_peer_status(std::string id, PeerStatus &status)
{
	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	std::map<std::string, PeerStatus>::iterator it = mPeers.find(id);
	if (it != mPeers.end())
	{
		status = it->second;
		return 1;
	}
	return 0;
}


int PeerNet::get_net_failedpeers(std::list<std::string> &peerIds)
{
	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	std::map<std::string, PeerStatus>::iterator it;
	for(it = mFailedPeers.begin(); it != mFailedPeers.end(); it++)
	{
		peerIds.push_back(it->first);
	}
	return 1;
}

int PeerNet::get_failedpeer_status(std::string id, PeerStatus &status)
{
	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	std::map<std::string, PeerStatus>::iterator it = mFailedPeers.find(id);
	if (it != mFailedPeers.end())
	{
		status = it->second;
		return 1;
	}
	return 0;
}

int PeerNet::get_relayends(std::list<UdpRelayEnd> &relayEnds)
{
	mRelayReceiver->getRelayEnds(relayEnds);
	return 1;
}

int PeerNet::get_relayproxies(std::list<UdpRelayProxy> &relayProxies)
{
	mRelayReceiver->getRelayProxies(relayProxies);
	return 1;
}


int PeerNet::get_dht_queries(std::map<bdNodeId, bdQueryStatus> &queries)
{
	return mUdpBitDht->getDhtQueries(queries);
}

int PeerNet::get_query_status(std::string id, bdQuerySummary &query)
{
	bdNodeId tmpId;
        if (!bdStdLoadNodeId(&tmpId, id))
	{
		return 0;
	}

	return mUdpBitDht->getDhtQueryStatus(&tmpId, query);
}

        /* remember peers */
int PeerNet::storePeers(std::string filepath)
{
	std::cerr << "PeerNet::storePeers(" << filepath << ")";
	std::cerr << std::endl;

	FILE *fd = fopen(filepath.c_str(), "w");
	if (!fd)
	{
		std::cerr << "Failed to StorePeers, Issue Opening file: " << filepath;
		std::cerr << std::endl;
		return 0;
	}

	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	std::map<std::string, PeerStatus>::iterator it;
	for(it = mPeers.begin(); it != mPeers.end(); it++)
	{
		fprintf(fd, "%s\n", (it->first).c_str());
	}
	fclose(fd);
	return 1;
}
	
int PeerNet::loadPeers(std::string filepath)
{
	FILE *fd = fopen(filepath.c_str(), "r");
	if (!fd)
	{
		std::cerr << "Failed to loadPeers, Issue Opening file: " << filepath;
		std::cerr << std::endl;
		return 0;
	}

	char line[1024];
	bool firstline = true;

	std::list<std::string> peerIds;
	while(1 == fscanf(fd, "%[^\n]\n", line))
	{
		std::cerr << "Read Peer: " << line;
		std::cerr << std::endl;
		
		std::string id(line);
		peerIds.push_back(id);
	}
	fclose(fd);

	std::list<std::string>::iterator it;
	for(it = peerIds.begin(); it != peerIds.end(); it++)
	{
		add_peer(*it);
	}
	return 1;
}


        /* remember peers */
int PeerNet::storeConfig(std::string filepath)
{
	FILE *fd = fopen(filepath.c_str(), "w");
	if (!fd)
	{
		std::cerr << "Failed to StorePeers, Issue Opening file: " << filepath;
		std::cerr << std::endl;
		return 0;
	}

	/* store own hash */
	std::ostringstream ownidstr;
	bdStdPrintNodeId(ownidstr, &(mOwnId));
	fprintf(fd, "%s\n", ownidstr.str().c_str());

	fclose(fd);
	return 1;
}
	
int PeerNet::loadConfig(std::string filepath)
{
	FILE *fd = fopen(filepath.c_str(), "r");
	if (!fd)
	{
		std::cerr << "Failed to loadConfig, Issue Opening file: " << filepath;
		std::cerr << std::endl;
		return 0;
	}

	char line[1024];
	bool firstline = true;

	if (1 == fscanf(fd, "%[^\n]\n", line))
	{
		std::string id(line);
		std::cerr << "Read OwnId: " << line;
		std::cerr << std::endl;

		if (!bdStdLoadNodeId(&mOwnId, id))
		{
			std::cerr << "Failed to load own Id";	
			std::cerr << std::endl;
		}
	}
	fclose(fd);
	return 1;
}






        /**** dht Callback ****/
int PeerNet::dhtNodeCallback(const bdId *id, uint32_t peerflags)
{
	std::ostringstream str;
	bdStdPrintNodeId(str, &(id->id));
	std::string strId = str.str();

	std::map<std::string, PeerStatus>::iterator it = mPeers.find(strId);
	if (it != mPeers.end())
	{
#ifdef PEERNET_DEBUG
		std::cerr << "PeerNet::dhtNodeCallback() From KNOWN PEER: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << " Flags: " << peerflags;
		std::cerr << std::endl;
#endif
	}

	// These are the flags that will be returned by PeerNet & RS peers with the new DHT.
	// if (peerflags & BITDHT_PEER_STATUS_DHT_ENGINE_VERSION)  // Change to this later...
	if ((peerflags & BITDHT_PEER_STATUS_DHT_ENGINE) && 
		(peerflags & BITDHT_PEER_STATUS_DHT_APPL))
	{
#ifdef PEERNET_DEBUG
		std::cerr << "PeerNet::dhtNodeCallback() Passing Local Peer to DhtStunner: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;
#endif

		/* pass off to the Stunners
		 * but only if they need them.
		 * ideally don't pass to both peers... (XXX do later)
		 */
		if ((mProxyStunner) && (mProxyStunner->needStunPeers()))
		{
			mProxyStunner->addStunPeer(id->addr, strId.c_str());
		}
		/* else */ // removed else until we have lots of peers.

		if ((mDhtStunner) && (mDhtStunner->needStunPeers()))
		{
			mDhtStunner->addStunPeer(id->addr, strId.c_str());
		}
	}
	return 1;
}

int PeerNet::dhtPeerCallback(const bdId *id, uint32_t status)
{
	std::ostringstream str;
	bdStdPrintNodeId(str, &(id->id));
	std::string strId = str.str();

	//std::cerr << "PeerNet::dhtPeerCallback()";
	//std::cerr << std::endl;

	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	


	std::map<std::string, PeerStatus>::iterator it = mPeers.find(strId);
	if (it != mPeers.end())
	{
		switch(status)
		{
			default:
			{
				it->second.mDhtStatusMsg = "Unknown Dht State";
				it->second.mDhtState = PN_DHT_STATE_UNKNOWN;
			}
				break;

			case BITDHT_MGR_QUERY_FAILURE:
			{
				it->second.mDhtStatusMsg = "Search Failed (is DHT working?)";	
				it->second.mDhtState = PN_DHT_STATE_FAILURE;
			}
				break;
			case BITDHT_MGR_QUERY_PEER_OFFLINE:
			{
				it->second.mDhtStatusMsg = "Offline";	
				it->second.mDhtState = PN_DHT_STATE_OFFLINE;
			}
				break;
			case BITDHT_MGR_QUERY_PEER_UNREACHABLE:
			{
				it->second.mDhtStatusMsg = "Unreachable";	
				it->second.mDhtState = PN_DHT_STATE_UNREACHABLE;
				it->second.mDhtAddr = id->addr;

				UnreachablePeerCallback_locked(id, status, &(it->second));
			}
				break;
			case BITDHT_MGR_QUERY_PEER_ONLINE:
			{
				it->second.mDhtStatusMsg = "Online";	
				it->second.mDhtState = PN_DHT_STATE_ONLINE;
				it->second.mDhtAddr = id->addr;

				OnlinePeerCallback_locked(id, status, &(it->second));
			}
				break;
		}
		time_t now = time(NULL);
		it->second.mDhtUpdateTS = now;
		return 1;
	}
	else
	{
		std::cerr << "PeerNet::dhtPeerCallback() Unknown Peer: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << " status: " << status;
		std::cerr << std::endl;
	}

	return 1;
}



int PeerNet::OnlinePeerCallback_locked(const bdId *id, uint32_t status, PeerStatus *peerStatus)
{

	if (peerStatus->mPeerConnectState != PN_PEER_CONN_DISCONNECTED)
	{

		std::cerr << "dhtPeerCallback. WARNING Ignoring Callback. Peer Online, but connection already underway: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;

		return 1;
	}

	bool connectOk = true;

	/* work out network state */
	uint32_t connectFlags = peerStatus->mConnectLogic.connectCb(CSB_CONNECT_DIRECT, 
															   mNetStateBox.getNetworkMode(), mNetStateBox.getNatTypeMode());
	bool useProxyPort = (connectFlags & CSB_ACTION_PROXY_PORT);

	switch(connectFlags & CSB_ACTION_MASK_MODE)
	{
		default:
		case CSB_ACTION_WAIT:
		{
			connectOk = false;
		}
			break;
		case CSB_ACTION_DIRECT_CONN:
		{

			connectOk = true;
		}
			break;
		case CSB_ACTION_PROXY_CONN:
		{
			/* ERROR */
			std::cerr << "dhtPeerCallback: ERROR ConnectLogic returned PROXY";
			std::cerr << std::endl;
			connectOk = false;
		}
			break;
		case CSB_ACTION_RELAY_CONN:
		{
			/* ERROR */
			std::cerr << "dhtPeerCallback: ERROR ConnectLogic returned RELAY";
			std::cerr << std::endl;
			connectOk = false;
		}
			break;
	}

	if (connectOk)
	{
		std::cerr << "dhtPeerCallback. Peer Online, triggering Direct Connection for: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;

		/* Push Back PeerAction */
		PeerAction ca;
		ca.mType = PEERNET_ACTION_TYPE_CONNECT;
		ca.mMode = BITDHT_CONNECT_MODE_DIRECT;
		ca.mDestId = *id;
		ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;

		mActions.push_back(ca);
	}
	return 1;
}


/* Fn Was getting too big, so moved this specific callback here */
int PeerNet::UnreachablePeerCallback_locked(const bdId *id, uint32_t status, PeerStatus *peerStatus)
{

	if (peerStatus->mPeerConnectState != PN_PEER_CONN_DISCONNECTED)
	{
		std::cerr << "dhtPeerCallback. WARNING Ignoring Callback, Peer Unreachable, but connection already underway: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;

		return 1;
	}

	std::cerr << "dhtPeerCallback. Peer Unreachable, triggering Proxy | Relay Connection for: ";
	bdStdPrintId(std::cerr, id);
	std::cerr << std::endl;

	/*** At This point we need to be clever about re-connect attempts ....
	 * How do we store the historical attempts?
	 */

	bool proxyOk = false;
	bool connectOk = true;

	/* work out network state */
	uint32_t connectFlags = peerStatus->mConnectLogic.connectCb(CSB_CONNECT_UNREACHABLE, 
					mNetStateBox.getNetworkMode(), mNetStateBox.getNatTypeMode());
	bool useProxyPort = (connectFlags & CSB_ACTION_PROXY_PORT);

	switch(connectFlags & CSB_ACTION_MASK_MODE)
	{
		default:
		case CSB_ACTION_WAIT:
		{
			connectOk = false;
		}
			break;
		case CSB_ACTION_DIRECT_CONN:
		{
			/* ERROR */
			std::cerr << "dhtPeerCallback: ERROR ConnectLogic returned DIRECT";
			std::cerr << std::endl;

			connectOk = false;
		}
			break;
		case CSB_ACTION_PROXY_CONN:
		{
			proxyOk = true;
			connectOk = true;
		}
			break;
		case CSB_ACTION_RELAY_CONN:
		{
			proxyOk = false;
			connectOk = true;
		}
			break;
	}

	if (connectOk)
	{
		time_t now = time(NULL);
		/* Push Back PeerAction */
		PeerAction ca;
		ca.mType = PEERNET_ACTION_TYPE_CONNECT;
		ca.mDestId = *id;
		peerStatus->mConnectLogicTS = now;	
		peerStatus->mConnectLogicFlags = connectFlags;
		peerStatus->mConnectLogicProxyPort = useProxyPort;

		if (proxyOk)
		{
			ca.mMode = BITDHT_CONNECT_MODE_PROXY;
			std::cerr << "dhtPeerCallback. Trying Proxy Connection.";
			std::cerr << std::endl;
		}
		else
		{
			std::cerr << "dhtPeerCallback. Trying Relay Connection.";
			std::cerr << std::endl;
			ca.mMode = BITDHT_CONNECT_MODE_RELAY;
		}

		ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;
		mActions.push_back(ca);
	}
	else
	{
		std::cerr << "dhtPeerCallback. Cancelled Connection Attempt for";
		bdStdPrintId(std::cerr, id);
		std::cerr << std::endl;
	}

	return 1;
}



int PeerNet::dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	std::cerr << "PeerNet::dhtValueCallback()";
	std::cerr << std::endl;

	return 1;
}

PeerStatus *PeerNet::getPeerStatus_locked(const bdId *peerId)
{
	std::cerr << "PeerNet::getPeerStatus_locked() for: ";
	bdStdPrintId(std::cerr,peerId);
	std::cerr << std::endl;

	std::ostringstream str;
	bdStdPrintNodeId(str, &(peerId->id));
	std::string id = str.str();

	/* check if they are in our friend list */
	std::map<std::string, PeerStatus>::iterator it = mPeers.find(id);

	if (it == mPeers.end())
	{
		std::cerr << "PeerNet::getPeerStatus_locked() WARNING Failed to find PeerStatus for: ";
		bdStdPrintId(std::cerr,peerId);
		std::cerr << std::endl;

		return NULL;
	}
	return &(it->second);
}



int PeerNet::dhtConnectCallback(const bdId *srcId, const bdId *proxyId, const bdId *destId,
							uint32_t mode, uint32_t point, uint32_t cbtype, uint32_t errcode)
{
	std::cerr << "PeerNet::dhtConnectCallback()";
	std::cerr << std::endl;
	std::cerr << "srcId: ";
	bdStdPrintId(std::cerr, srcId);
	std::cerr << std::endl;
	std::cerr << "proxyId: ";
	bdStdPrintId(std::cerr, proxyId);
	std::cerr << std::endl;
	std::cerr << "destId: ";
	bdStdPrintId(std::cerr, destId);
	std::cerr << std::endl;

	std::cerr << "mode: " << mode;
	std::cerr << " point: " << point;
	std::cerr << " cbtype: " << cbtype;
	std::cerr << std::endl;


	/* we handle MID and START/END points differently... this is biggest difference.
	 * so handle first.
	 */

	bdId peerId;
	time_t now = time(NULL);

	switch(point)
	{
		default:
		case BD_PROXY_CONNECTION_UNKNOWN_POINT:
		{
			std::cerr << "PeerNet::dhtConnectCallback() UNKNOWN point, ignoring Callback";
			std::cerr << std::endl;
			return 0;
		}
		case BD_PROXY_CONNECTION_START_POINT:
			peerId = *destId;
			break;
		case BD_PROXY_CONNECTION_END_POINT:
			peerId = *srcId;
			break;
		case BD_PROXY_CONNECTION_MID_POINT:
		{
			/* AS a mid point, we can receive.... AUTH,PENDING,PROXY,FAILED */

			switch(cbtype)
			{
				case BITDHT_CONNECT_CB_AUTH:
				{
					std::cerr << "dhtConnectionCallback() Proxy Connection Requested Between:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					std::cerr << std::endl;

					int connectionAllowed = BITDHT_CONNECT_ERROR_GENERIC;
					if (checkProxyAllowed(srcId, destId, mode))
					{
						connectionAllowed = BITDHT_CONNECT_ANSWER_OKAY;
						std::cerr << "dhtConnectionCallback() Connection Allowed";
						std::cerr << std::endl;
					}
					else
					{
						connectionAllowed = BITDHT_CONNECT_ERROR_AUTH_DENIED;
						std::cerr << "dhtConnectionCallback() Connection Denied";
						std::cerr << std::endl;
					}
		
					bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

					/* Push Back PeerAction */
					PeerAction ca;
					ca.mType = PEERNET_ACTION_TYPE_AUTHORISE;
					ca.mMode = mode;
					ca.mProxyId = *proxyId;
					ca.mSrcId = *srcId;
					ca.mDestId = *destId;
					ca.mPoint = point;
					ca.mAnswer = connectionAllowed;
		
					mActions.push_back(ca);
				}
				break;
				case BITDHT_CONNECT_CB_PENDING:
				{
					std::cerr << "dhtConnectionCallback() Proxy Connection Pending:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					std::cerr << std::endl;

					if (mode == BITDHT_CONNECT_MODE_RELAY)
					{
						//Installed at Request Now.
						//installRelayConnection(srcId, destId, mode);
					}
				}
				break;
				case BITDHT_CONNECT_CB_PROXY:
				{
					std::cerr << "dhtConnectionCallback() Proxy Connection Starting:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					std::cerr << std::endl;
				}
				break;
				case BITDHT_CONNECT_CB_FAILED:
				{
					std::cerr << "dhtConnectionCallback() Proxy Connection Failed:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					
					std::cerr << " ErrorCode: " << errcode;
					int errsrc = errcode & BITDHT_CONNECT_ERROR_MASK_SOURCE;
					int errtype = errcode & BITDHT_CONNECT_ERROR_MASK_TYPE;
					
					std::cerr << " ErrorSrc: " << errsrc;
					std::cerr << " ErrorType: " << errtype;
					
					std::cerr << std::endl;

					if (mode == BITDHT_CONNECT_MODE_RELAY)
					{
						removeRelayConnection(srcId, destId);
					}
				}
				break;
				default:
				case BITDHT_CONNECT_CB_START:
				{
					std::cerr << "dhtConnectionCallback() ERROR unexpected Proxy ConnectionCallback:";
					std::cerr << std::endl;
					bdStdPrintId(std::cerr, srcId);
					std::cerr << " and ";
					bdStdPrintId(std::cerr, destId);
					std::cerr << std::endl;
				}
				break;
			}
			/* End the MID Point stuff */
			return 1;
		}
	}

	/* if we get here, we are an endpoint (peer specified in peerId) */

	switch(cbtype)
	{
		case BITDHT_CONNECT_CB_AUTH:
		{
			std::cerr << "dhtConnectionCallback() Connection Requested By: ";
			bdStdPrintId(std::cerr, &(peerId));
			std::cerr << std::endl;

			int connectionAllowed = BITDHT_CONNECT_ERROR_GENERIC;
			if (checkConnectionAllowed(&(peerId), mode))
			{
				connectionAllowed = BITDHT_CONNECT_ANSWER_OKAY;
				std::cerr << "dhtConnectionCallback() Connection Allowed";
				std::cerr << std::endl;
			}
			else
			{
				connectionAllowed = BITDHT_CONNECT_ERROR_AUTH_DENIED;
				std::cerr << "dhtConnectionCallback() Connection Denied";
				std::cerr << std::endl;
			}
	
			bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

			/* Push Back PeerAction */
			PeerAction ca;
			ca.mType = PEERNET_ACTION_TYPE_AUTHORISE;
			ca.mProxyId = *proxyId;
			ca.mSrcId = *srcId;
			ca.mDestId = *destId;
	
			/* Check Proxy ExtAddress Status (but only if connection is Allowed) */	
			if ((connectionAllowed == BITDHT_CONNECT_ANSWER_OKAY) &&
			 		 (mode == BITDHT_CONNECT_MODE_PROXY))
			{
				std::cerr << "dhtConnectionCallback() Checking Address for Proxy";
				std::cerr << std::endl;

				struct sockaddr_in extaddr;
				uint8_t extStable = 0;
				sockaddr_clear(&extaddr);

				bool connectOk = false;
				bool proxyPort = false; 
				std::cerr << "dhtConnectionCallback():  Proxy... deciding which port to use.";
				std::cerr << std::endl;
	
				PeerStatus *ps = getPeerStatus_locked(&(peerId));
				if (ps)
				{
					proxyPort = ps->mConnectLogic.shouldUseProxyPort(
						mNetStateBox.getNetworkMode(), mNetStateBox.getNatTypeMode());

					ps->mConnectLogicTS = now;
					ps->mConnectLogicFlags = 0;
					ps->mConnectLogicProxyPort = proxyPort;

					std::cerr << "dhtConnectionCallback: Setting ProxyPort: ";
					std::cerr << " UseProxyPort? " << ps->mConnectLogicProxyPort;
					std::cerr << std::endl;

					connectOk = true;
				}
				else
				{
					std::cerr << "PeerAction: Connect Proxy: ERROR Cannot find PeerStatus";
					std::cerr << std::endl;
				}

				UdpStunner *stunner = mProxyStunner;
				if (!proxyPort)
				{
					stunner = mDhtStunner;
				}
				
				if ((connectOk) && (stunner->externalAddr(extaddr, extStable)))
				{
					if (extStable)
					{
						std::cerr << "dhtConnectionCallback() Proxy Connection Attempt to: ";
						bdStdPrintId(std::cerr, &(peerId));
						std::cerr << " is OkGo as we have Stable Own External Proxy Address";
						std::cerr << std::endl;

						if (point == BD_PROXY_CONNECTION_END_POINT)
						{
							ca.mDestId.addr = extaddr;
						}
						else
						{
							ca.mSrcId.addr = extaddr;
							std::cerr << "dhtConnectionCallback() ERROR Proxy Auth as SrcId";
							std::cerr << std::endl;
						}
						
					}
					else
					{
						connectionAllowed = BITDHT_CONNECT_ERROR_UNREACHABLE;
						std::cerr << "dhtConnectionCallback() Proxy Connection";
						std::cerr << " is Discarded, as Own External Proxy Address is Not Stable!";
						std::cerr << std::endl;
					}
				}
				else
				{
					connectionAllowed = BITDHT_CONNECT_ERROR_TEMPUNAVAIL;
					std::cerr << "dhtConnectionCallback() ERROR Proxy Connection ";
					std::cerr << " is Discarded, as Failed to get Own External Proxy Address.";
					std::cerr << std::endl;
				}
			}

			ca.mMode = mode;
			ca.mPoint = point;
			ca.mAnswer = connectionAllowed;

			mActions.push_back(ca);
		}
		break;

		case BITDHT_CONNECT_CB_START:
		{
			std::cerr << "dhtConnectionCallback() Connection Starting with: ";
			bdStdPrintId(std::cerr, &(peerId));
			std::cerr << std::endl;

			bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

			/* Push Back PeerAction */
			PeerAction ca;
			ca.mType = PEERNET_ACTION_TYPE_START;
			ca.mMode = mode;
			ca.mProxyId = *proxyId;
			ca.mSrcId = *srcId;
			ca.mDestId = *destId;
			ca.mPoint = point;
			ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;
		
			mActions.push_back(ca);

		}
		break;

		case BITDHT_CONNECT_CB_FAILED:
		{
			std::cerr << "dhtConnectionCallback() Connection Attempt Failed with:";
			bdStdPrintId(std::cerr, &(peerId));
			std::cerr << std::endl;
			
			std::cerr << "dhtConnectionCallback() Proxy:";			
			bdStdPrintId(std::cerr, proxyId);
			std::cerr << std::endl;
				
			std::cerr << "dhtConnectionCallback() ";			
			std::cerr << " ErrorCode: " << errcode;
			int errsrc = errcode & BITDHT_CONNECT_ERROR_MASK_SOURCE;
			int errtype = errcode & BITDHT_CONNECT_ERROR_MASK_TYPE;
				
			std::cerr << " ErrorSrc: " << errsrc;
			std::cerr << " ErrorType: " << errtype;
			std::cerr << std::endl;
			
			bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

			PeerStatus *ps = getPeerStatus_locked(&peerId);
			if (ps)
			{
				ps->mPeerCbMsg = "ERROR : ";
				ps->mPeerCbMsg += decodeConnectionError(errcode);
				ps->mPeerCbMode = mode;
				ps->mPeerCbPoint = point;
				ps->mPeerCbProxyId = *proxyId;
				ps->mPeerCbDestId = peerId;
				ps->mPeerCbTS = now;
			}
			else
			{
				std::cerr << "dhtConnectionCallback() ";			
				std::cerr << "ERROR Unknown Peer";
				std::cerr << std::endl;
			}
		}
		break;

		case BITDHT_CONNECT_CB_REQUEST: 
		{
			std::cerr << "dhtConnectionCallback() Local Connection Request Feedback:";
			bdStdPrintId(std::cerr, &(peerId));
			std::cerr << std::endl;
			
			std::cerr << "dhtConnectionCallback() Proxy:";			
			bdStdPrintId(std::cerr, proxyId);
			std::cerr << std::endl;

			if (point != BD_PROXY_CONNECTION_START_POINT)
			{
				std::cerr << "dhtConnectionCallback() ERROR Cannot find PeerStatus";
				std::cerr << std::endl;
				return 0;
			}

			bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

			PeerStatus *ps = getPeerStatus_locked(&peerId);
			if (ps)
			{
				if (errcode)
				{
					ps->mPeerReqStatusMsg = "STOPPED: ";
					ps->mPeerReqStatusMsg += decodeConnectionError(errcode);
					ps->mPeerReqState = PN_PEER_REQ_STOPPED;
					ps->mPeerReqTS = now;

					int updatecode = CSB_UPDATE_FAILED_ATTEMPT;
					int errtype = errcode & BITDHT_CONNECT_ERROR_MASK_TYPE;
					switch(errtype)
					{
						/* fatal errors */
						case BITDHT_CONNECT_ERROR_AUTH_DENIED:
						{
							updatecode = CSB_UPDATE_AUTH_DENIED;
						}
							break;

						/* move on errors */
						case BITDHT_CONNECT_ERROR_UNREACHABLE: // sym NAT.
						case BITDHT_CONNECT_ERROR_UNSUPPORTED: // mode unavailable.
						{
							updatecode = CSB_UPDATE_MODE_UNAVAILABLE;
						}
							break;

						/* standard failed attempts */
						case BITDHT_CONNECT_ERROR_GENERIC:
						case BITDHT_CONNECT_ERROR_PROTOCOL:
						case BITDHT_CONNECT_ERROR_TIMEOUT:
						case BITDHT_CONNECT_ERROR_TEMPUNAVAIL:
						case BITDHT_CONNECT_ERROR_NOADDRESS:
						case BITDHT_CONNECT_ERROR_OVERLOADED:
						case BITDHT_CONNECT_ERROR_DUPLICATE:
						/* CB_REQUEST errors */
						case BITDHT_CONNECT_ERROR_TOOMANYRETRY:
						case BITDHT_CONNECT_ERROR_OUTOFPROXY:
						{
							updatecode = CSB_UPDATE_FAILED_ATTEMPT;
						}
							break;

						/* user cancelled ... no update */
						case BITDHT_CONNECT_ERROR_USER:
						{
							updatecode = CSB_UPDATE_NONE;
						}
							break;
					}
					if (updatecode)
					{
						ps->mConnectLogic.updateCb(updatecode);
					}
				}
				else // a new connection attempt.
				{
					ps->mPeerReqStatusMsg = "Connect Attempt";
					ps->mPeerReqState = PN_PEER_REQ_RUNNING;
					ps->mPeerReqMode = mode;
					ps->mPeerReqProxyId = *proxyId;
					ps->mPeerReqTS = now;

					// This also is flagged into the instant Cb info.
					ps->mPeerCbMsg = "Local Connect Attempt";
					ps->mPeerCbMode = mode;
					ps->mPeerCbPoint = point;
					ps->mPeerCbProxyId = *proxyId;
					ps->mPeerCbDestId = peerId;
					ps->mPeerCbTS = now;
				}
			}
			else
			{
				std::cerr << "dhtConnectionCallback() ERROR Cannot find PeerStatus";
				std::cerr << std::endl;
			}
		}
		break;

		default:
		case BITDHT_CONNECT_CB_PENDING:
		case BITDHT_CONNECT_CB_PROXY:
		{
			std::cerr << "dhtConnectionCallback() ERROR unexpected ConnectionCallback:";
			std::cerr << std::endl;
			bdStdPrintId(std::cerr, srcId);
			std::cerr << " and ";
			bdStdPrintId(std::cerr, destId);
			std::cerr << std::endl;
		}
		break;
	}
	return 1;
}


/* tick stuff that isn't in its own thread */

int PeerNet::tick()
{
	if (mDhtStunner)
		mDhtStunner->tick();

	if (mProxyStunner)
		mProxyStunner->tick();

	doActions();
	monitorConnections();

	minuteTick();

	keepaliveConnections();

	time_t now = time(NULL);

	std::cerr << "PeerNet::tick() TIME: " << ctime(&now) << std::endl;
	std::cerr.flush();

	return 1;
}

#define MINUTE_IN_SECS	60

int PeerNet::minuteTick()
{
	/* should be Mutex protected? Only one thread should get here for now */

	time_t now = time(NULL);
	if (now - mMinuteTS > MINUTE_IN_SECS)
	{
		mMinuteTS = now;
		netStateTick();
		mRelayReceiver->checkRelays();
	}
	return 1;
}

#define DHT_PEERS_ACTIVE	2

int PeerNet::netStateTick()
{
	bool dhtOn = true;
	bool dhtActive = (mUdpBitDht->statsNetworkSize() > DHT_PEERS_ACTIVE);
	mNetStateBox.setDhtState(dhtOn, dhtActive);

	struct sockaddr_in extAddr;
	uint8_t extStable;
	if (mDhtStunner->externalAddr(extAddr, extStable))
	{
		mNetStateBox.setAddressStunDht(&extAddr, extStable != 0);
	}

	if (mProxyStunner->externalAddr(extAddr, extStable))
	{
		mNetStateBox.setAddressStunProxy(&extAddr, extStable != 0);
	}
	
	return 1;
}


int PeerNet::doActions()
{
#ifdef PEERNET_DEBUG
	std::cerr << "PeerNet::doActions()" << std::endl;
#endif

	time_t now = time(NULL);

	while(mActions.size() > 0)
	{
		PeerAction action;

		{
			bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

			if (mActions.size() < 1)
			{
				break;
			}

			action = mActions.front();
			mActions.pop_front();
		}

		switch(action.mType)
		{
			case PEERNET_ACTION_TYPE_CONNECT:
			{
				/* connect attempt */
				std::cerr << "PeerAction. Connection Attempt to: ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;

				bool connectionRequested = false;

				if ((action.mMode == BITDHT_CONNECT_MODE_DIRECT) ||
						(action.mMode == BITDHT_CONNECT_MODE_RELAY))
				{
					struct sockaddr_in laddr; // We zero this address. The DHT layer should be able to handle this!
					sockaddr_clear(&laddr);
					uint32_t start = 1;
					mUdpBitDht->ConnectionRequest(&laddr, &(action.mDestId.id), action.mMode, start);
					connectionRequested = true;
				}
				else if (action.mMode == BITDHT_CONNECT_MODE_PROXY)
				{
					struct sockaddr_in extaddr;
					uint8_t extStable = 0;
					sockaddr_clear(&extaddr);
					bool proxyPort = true;
					bool connectOk = false;

					std::cerr << "PeerAction:  Proxy... deciding which port to use.";
					std::cerr << std::endl;
					{
						bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	
	
						PeerStatus *ps = getPeerStatus_locked(&(action.mDestId));
						if (ps)
						{
							std::cerr << "PeerAction: Proxy: Using ConnectLogic Info from: ";
							std::cerr << now-ps->mConnectLogicTS << " ago. Flags: " << ps->mConnectLogicFlags;
							std::cerr << " UseProxyPort? " << ps->mConnectLogicProxyPort;
							std::cerr << std::endl;

							connectOk = true;
							proxyPort = ps->mConnectLogicProxyPort;

						}
						else
						{
							std::cerr << "PeerAction: Connect Proxy: ERROR Cannot find PeerStatus";
							std::cerr << std::endl;
						}
					}
					UdpStunner *stunner = mProxyStunner;
					if (!proxyPort)
					{
						stunner = mDhtStunner;
					}
					
					if ((connectOk) && (stunner->externalAddr(extaddr, extStable)))
					{
						if (extStable)
						{
							std::cerr << "PeerAction: Proxy Connection Attempt to: ";
							bdStdPrintId(std::cerr, &(action.mDestId));
							std::cerr << " is OkGo as we have Stable Own External Proxy Address";
							std::cerr << std::endl;

							int start = 1;
							mUdpBitDht->ConnectionRequest(&extaddr, &(action.mDestId.id), action.mMode, start);
							connectionRequested = true;
						}
						else
						{
							std::cerr << "PeerAction: ERROR Proxy Connection Attempt to: ";
							bdStdPrintId(std::cerr, &(action.mDestId));
							std::cerr << " is Discarded, as Own External Proxy Address is Not Stable!";
							std::cerr << std::endl;

							bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	
	
							PeerStatus *ps = getPeerStatus_locked(&(action.mDestId));
							if (ps)
							{
								ps->mConnectLogic.updateCb(CSB_UPDATE_MODE_UNAVAILABLE);
							}
						}
					}
					else
					{
						std::cerr << "PeerAction: ERROR Proxy Connection Attempt to: ";
						bdStdPrintId(std::cerr, &(action.mDestId));
						std::cerr << " is Discarded, as Failed to get Own External Proxy Address.";
						std::cerr << std::endl;

						bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	
	
						PeerStatus *ps = getPeerStatus_locked(&(action.mDestId));
						if (ps)
						{
							ps->mConnectLogic.updateCb(CSB_UPDATE_FAILED_ATTEMPT);
						}
					}
				}

				if (connectionRequested)
				{
					bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

					PeerStatus *ps = getPeerStatus_locked(&(action.mDestId));
					if (ps)
					{
						ps->mPeerReqStatusMsg = "Connect Request";
						ps->mPeerReqState = PN_PEER_REQ_RUNNING;
						ps->mPeerReqMode = action.mMode;
						ps->mPeerReqTS = now;
					}
					else
					{
						std::cerr << "PeerAction: Connect ERROR Cannot find PeerStatus";
						std::cerr << std::endl;
					}
				}

			}
			break;

			case PEERNET_ACTION_TYPE_AUTHORISE:
			{
				/* connect attempt */
				std::cerr << "PeerAction. Authorise Connection between: ";
				bdStdPrintId(std::cerr, &(action.mSrcId));
				std::cerr << " and ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;

				mUdpBitDht->ConnectionAuth(&(action.mSrcId), &(action.mProxyId), &(action.mDestId), 
					action.mMode, action.mPoint, action.mAnswer);

				// Only feedback to the gui if we are at END.
				if (action.mPoint == BD_PROXY_CONNECTION_END_POINT)
				{
					bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

					PeerStatus *ps = getPeerStatus_locked(&(action.mSrcId));
					if (ps)
					{	
						if (action.mAnswer)
						{
							ps->mPeerCbMsg = "WE DENIED AUTH: ERROR : ";
							ps->mPeerCbMsg += decodeConnectionError(action.mAnswer);
						}
						else
						{
							ps->mPeerCbMsg = "We AUTHED";
						}
						ps->mPeerCbMode = action.mMode;
						ps->mPeerCbPoint = action.mPoint;
						ps->mPeerCbProxyId = action.mProxyId;
						ps->mPeerCbDestId = action.mSrcId;
						ps->mPeerCbTS = now;
					}
					// Not an error if AUTH_DENIED - cos we don't know them! (so won't be in peerList).
					else if (action.mAnswer | BITDHT_CONNECT_ERROR_AUTH_DENIED)
					{
						std::cerr << "PeerAction Authorise Connection ";			
						std::cerr << "Denied Unknown Peer";
						std::cerr << std::endl;
					}
					else 
					{
						std::cerr << "PeerAction Authorise Connection ";			
						std::cerr << "ERROR Unknown Peer & !DENIED ???";
						std::cerr << std::endl;
					}
				}
			}
			break;

			case PEERNET_ACTION_TYPE_START:
			{
				/* connect attempt */
				std::cerr << "PeerAction. Start Connection between: ";
				bdStdPrintId(std::cerr, &(action.mSrcId));
				std::cerr << " and ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;

				initiateConnection(&(action.mSrcId), &(action.mProxyId), &(action.mDestId), 
					action.mMode, action.mPoint, action.mAnswer);
			}
			break;

			case PEERNET_ACTION_TYPE_RESTARTREQ:
			{
				/* connect attempt */
				std::cerr << "PeerAction. Restart Connection Attempt to: ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;

				struct sockaddr_in laddr; 
				sockaddr_clear(&laddr);
				uint32_t start = 1;
				mUdpBitDht->ConnectionRequest(&laddr, &(action.mDestId.id), action.mMode, start);
			}
			break;

			case PEERNET_ACTION_TYPE_KILLREQ:
			{
				/* connect attempt */
				std::cerr << "PeerAction. Kill Connection Attempt to: ";
				bdStdPrintId(std::cerr, &(action.mDestId));
				std::cerr << " mode: " << action.mMode;
				std::cerr << std::endl;

				struct sockaddr_in laddr; 
				sockaddr_clear(&laddr);
				uint32_t start = 0;
				mUdpBitDht->ConnectionRequest(&laddr, &(action.mDestId.id), action.mMode, start);
			}
			break;

		}
	}
	return 1;
}





/****************************** Connection Logic ***************************/

/* Proxies!.
 *
 * We can allow all PROXY connections, as it is just a couple of messages,
 * however, we should be smart about how many RELAY connections are allowed.
 * 
 * e.g. Allow 20 Relay connections.
 * 15 for friends, 5 for randoms.
 *
 * Can also validate addresses with own secure connections.
 */

int PeerNet::checkProxyAllowed(const bdId *srcId, const bdId *destId, int mode)
{
	std::cerr << "PeerNet::checkProxyAllowed()";
	std::cerr << std::endl;

	// Dont think that a mutex is required here! But might be so just lock to ensure that it is possible.
	{
		bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	}

	if (mode == BITDHT_CONNECT_MODE_PROXY) 
	{
		std::cerr << "PeerNet::checkProxyAllowed() Allowing all PROXY connections, OKAY";
		std::cerr << std::endl;

		return 1;
		//return CONNECTION_OKAY;
	}

	if (mode != BITDHT_CONNECT_MODE_RELAY)
	{
		std::cerr << "PeerNet::checkProxyAllowed() unknown Connect Mode DENIED";
		std::cerr << std::endl;
		return 0;
	}

	/* will install the Relay Here... so that we reserve the Relay Space for later. */
	if (installRelayConnection(srcId, destId))
	{
		std::cerr << "PeerNet::checkProxyAllowed() Successfully added Relay, Connection OKAY";
		std::cerr << std::endl;

		return 1;
		// CONNECT_OKAY.
	}
	else
	{
		std::cerr << "PeerNet::checkProxyAllowed() Failed to install Relay, Connection DENIED";
		std::cerr << std::endl;

		return 0;
		//return CONNECT_MODE_OVERLOADED;
	}

	return 0;
	//return CONNECT_MODE_NOTAVAILABLE;
	//return CONNECT_MODE_OVERLOADED;
}


int PeerNet::checkConnectionAllowed(const bdId *peerId, int mode)
{
	std::cerr << "PeerNet::checkConnectionAllowed() to: ";
	bdStdPrintId(std::cerr,peerId);
	std::cerr << " mode: " << mode;
	std::cerr << std::endl;

	std::ostringstream str;
	bdStdPrintNodeId(str, &(peerId->id));
	std::string id = str.str();

	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	time_t now = time(NULL);

	/* check if they are in our friend list */
	std::map<std::string, PeerStatus>::iterator it = mPeers.find(id);

	if (it == mPeers.end())
	{
		std::cerr << "PeerNet::checkConnectionAllowed() Peer Not Friend, DENIED";
		std::cerr << std::endl;

		/* store as failed connection attempt */
		it = mFailedPeers.find(id);
		if (it == mFailedPeers.end())
		{
			mFailedPeers[id] = PeerStatus();
			it = mFailedPeers.find(id);
		}

		/* flag as failed */
		it->second.mId = id;

		it->second.mDhtStatusMsg = "Unknown";
		it->second.mDhtState = PN_DHT_STATE_UNKNOWN;
		it->second.mDhtUpdateTS = now;

		it->second.mPeerReqStatusMsg = "Denied Non-Friend";
		it->second.mPeerReqState = PN_PEER_REQ_STOPPED;
		it->second.mPeerReqTS = now;
		it->second.mPeerReqMode = 0;
		//it->second.mPeerProxyId;
		it->second.mPeerReqTS = now;

		it->second.mPeerCbMsg = "Denied Non-Friend";

		it->second.mPeerConnectMsg = "Denied Non-Friend";
		it->second.mPeerConnectState = PN_PEER_CONN_DISCONNECTED;

		
		return 0;
		//return NOT_FRIEND;
	}

	/* are a friend */

	if (it->second.mPeerConnectState == PN_PEER_CONN_CONNECTED)
	{
		std::cerr << "PeerNet::checkConnectionAllowed() ERROR Peer Already Connected, DENIED";
		std::cerr << std::endl;

		// STATUS UPDATE DONE IN ACTION.
		//it->second.mPeerStatusMsg = "2nd Connection Attempt!";	
		//it->second.mPeerUpdateTS = now;
		return 0;
		//return ALREADY_CONNECTED;
	}

#if 0
	/* are we capable of making this type of connection? */
	if (mode == BITDHT_CONNECT_MODE_RELAY) 
	{
		std::cerr << "PeerNet::checkConnectionAllowed() RELAY connection not possible, DENIED";
		std::cerr << std::endl;

		it->second.mPeerStatusMsg = "Attempt with Unavailable Mode";	
		it->second.mPeerState = PN_PEER_STATE_DENIED_UNAVAILABLE_MODE;
		it->second.mPeerUpdateTS = now;
		return 0;
		//return NOT_CAPABLE;
	}
#endif

	return 1;
	//return CONNECTION_OKAY;		
}





void PeerNet::initiateConnection(const bdId *srcId, const bdId *proxyId, const bdId *destId, uint32_t mode, uint32_t loc, uint32_t answer)
{
	std::cerr << "PeerNet::initiateConnection()";
	std::cerr << " mode: " << mode;
	std::cerr << std::endl;

	bdId peerConnectId;

	/* determine who the actual destination is.
	 * as we always specify the remote address, this is all we need. 
	 */
	if (loc == BD_PROXY_CONNECTION_START_POINT)
	{
		peerConnectId = *destId;
	}
	else if (loc == BD_PROXY_CONNECTION_END_POINT)
	{
		peerConnectId = *srcId;
	}
	else
	{
		std::cerr << "PeerNet::initiateConnection() ERROR, NOT either START or END";
		std::cerr << std::endl;
		/* ERROR */
		return;
	}

	std::ostringstream str;
	bdStdPrintNodeId(str, &(peerConnectId.id));
	std::string peerId = str.str();

	std::cerr << "PeerNet::initiateConnection() Connecting to ";
	bdStdPrintId(std::cerr, &peerConnectId);
	std::cerr << std::endl;

	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	/* grab a socket */
	std::map<std::string, PeerStatus>::iterator it = mPeers.find(peerId);
	if (it == mPeers.end())
	{
		std::cerr << "PeerNet::initiateConnection() ERROR Peer not found";
		std::cerr << std::endl;
		return;
	}

	if (it->second.mPeerConnectState != PN_PEER_CONN_DISCONNECTED)
	{
		std::cerr << "PeerNet::initiateConnection() ERROR Peer is not Disconnected";
		std::cerr << std::endl;
		return;
	}

	int fd = 0;

	/* start the connection */
	/* These Socket Modes must match the TOU Stack - or it breaks. */
	switch(mode)
	{
		default:
		case BITDHT_CONNECT_MODE_DIRECT:
			fd = tou_socket(PN_TOU_RECVER_DIRECT_IDX, TOU_RECEIVER_TYPE_UDPPEER, 0);
			break;

		case BITDHT_CONNECT_MODE_PROXY:
		{
			time_t now = time(NULL);
			std::cerr << "PeerNet::initiateConnection() Peer Mode Proxy... deciding which port to use.";
			std::cerr << std::endl;
			std::cerr << "PeerNet::initiateConnection() Using ConnectLogic Info from: ";
			std::cerr << now-it->second.mConnectLogicTS << " ago. Flags: " << it->second.mConnectLogicFlags;
			std::cerr << " UseProxyPort? " << it->second.mConnectLogicProxyPort;
			std::cerr << std::endl;

			if (it->second.mConnectLogicProxyPort)
			{
				fd = tou_socket(PN_TOU_RECVER_PROXY_IDX, TOU_RECEIVER_TYPE_UDPPEER, 0);
			}
			else
			{
				fd = tou_socket(PN_TOU_RECVER_DIRECT_IDX, TOU_RECEIVER_TYPE_UDPPEER, 0);
			}

		}
			break;
	
		case BITDHT_CONNECT_MODE_RELAY:
			fd = tou_socket(PN_TOU_RECVER_RELAY_IDX, TOU_RECEIVER_TYPE_UDPRELAY, 0);
			break;
	}

	if (fd < 0)
	{
		std::cerr << "PeerNet::initiateConnection()";
		std::cerr << " ERROR Open TOU Socket FAILED";
		std::cerr << std::endl;
		return;
	}

	it->second.mPeerConnectFd = fd;
	it->second.mPeerConnectProxyId = *proxyId;
	it->second.mPeerConnectPeerId = peerConnectId;
	
#define PEERNET_DIRECT_CONN_PERIOD	5
#define PEERNET_PROXY_CONN_PERIOD	30

#define PEERNET_CONNECT_TIMEOUT		(60)

	int connPeriod = PEERNET_PROXY_CONN_PERIOD;

	switch(mode)
	{
		default:
		case BITDHT_CONNECT_MODE_DIRECT:
			connPeriod = PEERNET_DIRECT_CONN_PERIOD; // can be much smaller as we are already talking to the peer.
			// Fall through.
		case BITDHT_CONNECT_MODE_PROXY:
			tou_connect(fd, (const struct sockaddr *)  (&(peerConnectId.addr)), sizeof(peerConnectId.addr), connPeriod);
			break;
	
		case BITDHT_CONNECT_MODE_RELAY:

			if (loc == BD_PROXY_CONNECTION_START_POINT)
			{
				/* standard order connection call */
				tou_connect_via_relay(fd, &(srcId->addr), &(proxyId->addr), &(destId->addr));
			}
			else // END_POINT
			{
				/* reverse order connection call */
				tou_connect_via_relay(fd, &(destId->addr), &(proxyId->addr), &(srcId->addr));
			}
			break;
	}

	/* store results in Status */
	it->second.mPeerConnectMsg = "UDP started";
	it->second.mPeerConnectState = PN_PEER_CONN_UDP_STARTED;
	it->second.mPeerConnectUdpTS = time(NULL);
	it->second.mPeerConnectMode = mode;
	it->second.mPeerConnectPoint = loc;

}


int PeerNet::installRelayConnection(const bdId *srcId, const bdId *destId)
{
	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	/* work out if either srcId or DestId is a friend */
	int relayClass = UDP_RELAY_CLASS_GENERAL;

	std::ostringstream str;
	bdStdPrintNodeId(str, &(srcId->id));
	std::string strId1 = str.str();

	str.clear();
	bdStdPrintNodeId(str, &(destId->id));
	std::string strId2 = str.str();

        /* grab a socket */
        std::map<std::string, PeerStatus>::iterator it;
	it = mPeers.find(strId1);
        if (it != mPeers.end())
        {
		relayClass = UDP_RELAY_CLASS_FRIENDS;
        }

	it = mPeers.find(strId2);
        if (it != mPeers.end())
        {
		relayClass = UDP_RELAY_CLASS_FRIENDS;
        }

	/* will install the Relay Here... so that we reserve the Relay Space for later. */
	UdpRelayAddrSet relayAddrs(&(srcId->addr), &(destId->addr));
	if (mRelayReceiver->addUdpRelay(&relayAddrs, relayClass))
	{
		std::cerr << "PeerNet::installRelayConnection() Successfully added Relay, Connection OKAY";
		std::cerr << std::endl;

		return 1;
		// CONNECT_OKAY.
	}
	else
	{
		std::cerr << "PeerNet::installRelayConnection() Failed to install Relay, Connection DENIED";
		std::cerr << std::endl;

		return 0;
		//return CONNECT_MODE_OVERLOADED;
	}
	return 0;
}


int PeerNet::removeRelayConnection(const bdId *srcId, const bdId *destId)
{
	//bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	UdpRelayAddrSet relayAddrs(&(srcId->addr), &(destId->addr));
	if (mRelayReceiver->removeUdpRelay(&relayAddrs))
	{
		std::cerr << "PeerNet::removeRelayConnection() Successfully removed Relay";
		std::cerr << std::endl;

		return 1;
	}
	else
	{
		std::cerr << "PeerNet::removeRelayConnection() ERROR Failed to remove Relay";
		std::cerr << std::endl;

		return 0;
	}
}

/***************************************************** UDP Connections *****************************/

void PeerNet::monitorConnections()
{
	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	
	std::map<std::string, PeerStatus>::iterator it;
	time_t now = time(NULL);
	for(it = mPeers.begin(); it != mPeers.end(); it++)
	{
		if (it->second.mPeerConnectState == PN_PEER_CONN_UDP_STARTED)
		{
			std::cerr << "PeerNet::monitorConnections() Connection in progress to: " << it->second.mId;
			std::cerr << std::endl;

			int fd = it->second.mPeerConnectFd;
			if (tou_connected(fd))
			{
				std::cerr << "PeerNet::monitorConnections() InProgress Connection Now Active: " << it->second.mId;
				std::cerr << std::endl;

				/* switch state! */
				it->second.mPeerConnectState = PN_PEER_CONN_CONNECTED;
				it->second.mPeerConnectTS = time(NULL);

				it->second.mConnectLogic.updateCb(CSB_UPDATE_CONNECTED);

				std::ostringstream msg;
				msg << "Connected in " << it->second.mPeerConnectTS - it->second.mPeerConnectUdpTS;
				msg << " secs";
				it->second.mPeerConnectMsg = msg.str();

				// Remove the Connection Request.
				if (it->second.mPeerReqState == PN_PEER_REQ_RUNNING)
				{
					std::cerr << "PeerNet::monitorConnections() Request Active, Stopping Request";
					std::cerr << std::endl;
				
					/* Push Back PeerAction */
					PeerAction ca;
					ca.mType = PEERNET_ACTION_TYPE_KILLREQ;
					ca.mMode = it->second.mPeerConnectMode;
					//ca.mProxyId = *proxyId;
					//ca.mSrcId = *srcId;
					ca.mDestId = it->second.mPeerConnectPeerId;
					//ca.mPoint = point;
					ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;
		
					mActions.push_back(ca);
			
					// What the Action should do...	
					//struct sockaddr_in tmpaddr;
					//bdsockaddr_clear(&tmpaddr);
					//int start = 0;
					//mUdpBitDht->ConnectionRequest(&tmpaddr, &(it->second.mPeerConnectPeerId.id), it->second.mPeerConnectMode, start);
				}
				// only an error if we initiated the connection.
				else if (it->second.mPeerConnectPoint == BD_PROXY_CONNECTION_START_POINT)
				{
					std::cerr << "PeerNet::monitorConnections() ERROR Request not active, can't stop";
					std::cerr << std::endl;										
				}

			}
			else if (now - it->second.mPeerConnectUdpTS > PEERNET_CONNECT_TIMEOUT)
			{
				std::cerr << "PeerNet::monitorConnections() ERROR InProgress Connection Failed: " << it->second.mId;
				std::cerr << std::endl;

				/* shut id down */
				it->second.mPeerConnectState = PN_PEER_CONN_DISCONNECTED;
				it->second.mPeerConnectMsg = "UDP Failed";
				tou_close(fd);
				
				if (it->second.mPeerReqState == PN_PEER_REQ_RUNNING)
				{
					std::cerr << "PeerNet::monitorConnections() Request Active (Paused)... restarting";
					std::cerr << std::endl;					

					/* Push Back PeerAction */
					PeerAction ca;
					ca.mType = PEERNET_ACTION_TYPE_RESTARTREQ;
					ca.mMode = it->second.mPeerConnectMode;
					//ca.mProxyId = *proxyId;
					//ca.mSrcId = *srcId;
					ca.mDestId = it->second.mPeerConnectPeerId;
					//ca.mPoint = point;
					ca.mAnswer = BITDHT_CONNECT_ERROR_NONE;
		
					mActions.push_back(ca);
		
					// What the Action should do!	
					// tell it to keep going.
					//struct sockaddr_in tmpaddr;
					//bdsockaddr_clear(&tmpaddr);
					//int start = 1;
					//mUdpBitDht->ConnectionRequest(&tmpaddr, &(it->second.mPeerConnectPeerId.id), it->second.mPeerConnectMode, start);
				}
				// only an error if we initiated the connection.
				else if (it->second.mPeerConnectPoint == BD_PROXY_CONNECTION_START_POINT)
				{
					std::cerr << "PeerNet::monitorConnections() ERROR Request not active, can't stop";
					std::cerr << std::endl;										
				}

			}
		}

		if (it->second.mPeerConnectState == PN_PEER_CONN_CONNECTED)
		{
			/* fd should be valid, check it */
			int fd = it->second.mPeerConnectFd;
			if (tou_connected(fd))
			{
				/* check for traffic */
				char buf[10240];
				memset(buf, 0, 10240); /* install \0 everywhere */
				int read = tou_read(fd, buf, 10240);
				if (read > 0)
				{
					std::string msg(buf);
					for(int i = 0; i < msg.size(); )
					{
						if (msg[i] == '^')
							msg.erase(i,1);
						else
							i++;
					}

					if (msg.size() > 0)
					{
						it->second.mPeerIncoming += msg;
					}
				}
			}
			else
			{
				std::cerr << "PeerNet::monitorConnections() Active Connection Closed: " << it->second.mId;
				std::cerr << std::endl;

				it->second.mConnectLogic.updateCb(CSB_UPDATE_DISCONNECTED);

				it->second.mPeerConnectState = PN_PEER_CONN_DISCONNECTED;
				it->second.mPeerConnectClosedTS = time(NULL);
				std::ostringstream msg;
				msg << "Closed, Alive for: " << it->second.mPeerConnectClosedTS - it->second.mPeerConnectTS;
				msg << " secs";
				it->second.mPeerConnectMsg = msg.str();
				tou_close(fd);
			}
		}
	}
}


void PeerNet::sendMessage(std::string msg)
{
	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	//std::cerr << "PeerNet::sendMessage() : " << msg;
	//std::cerr << std::endl;


	std::map<std::string, PeerStatus>::iterator it;
	for(it = mPeers.begin(); it != mPeers.end(); it++)
	{
		if (it->second.mPeerConnectState == PN_PEER_CONN_CONNECTED)
		{
			/* fd should be valid, check it */
			int fd = it->second.mPeerConnectFd;
			if (tou_connected(fd))
			{
				int written = tou_write(fd, msg.c_str(), msg.size());
				if (written != msg.size())
				{
					/* */
					std::cerr << "PeerNet::sendMessage() ERROR Sending to " << it->second.mId;
					std::cerr << std::endl;
				}	
				else
				{
					//std::cerr << "PeerNet::sendMessage() Sent to " << it->second.mId;
					//std::cerr << std::endl;
				}
			}
		}
	}
}

int  PeerNet::getMessage(std::string id, std::string &msg)
{
	bdStackMutex stack(mPeerMutex); /********** LOCKED MUTEX ***************/	

	std::map<std::string, PeerStatus>::iterator it = mPeers.find(id);
	if (it != mPeers.end())
	{
		if (it->second.mPeerIncoming.size() > 0)
		{
			msg = it->second.mPeerIncoming;
			it->second.mPeerIncoming.clear();

			std::cerr << "PeerNet::getMessage() : " << msg;
			std::cerr << std::endl;

			return 1;
		}
	}
	return 0;
}




void PeerNet::keepaliveConnections()
{
	std::string msg("^");
	sendMessage(msg);
}


