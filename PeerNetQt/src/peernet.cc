
#include "peernet.h"
#include <iostream>
#include <sstream>


#include "bitdht/bdstddht.h"
#include "tcponudp/tou.h"
#include "util/rsnet.h"


PeerNet::PeerNet(std::string id, std::string configpath, uint16_t port)
{
        std::string dhtVersion = "RS52"; // should come from elsewhere!

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
	std::string bootstrapfile = configpath + "/bdboot.txt";

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

        /* standard dht behaviour */
        bdDhtFunctions *stdfns = new bdStdDht();

        std::cerr << "PeerNet() startup ... creating UdpStack";
        std::cerr << std::endl;


        struct sockaddr_in tmpladdr;
        sockaddr_clear(&tmpladdr);
        tmpladdr.sin_port = htons(mPort);
        mUdpStack = new UdpStack(tmpladdr);

        std::cerr << "PeerNet() startup ... creating UdpBitDht";
        std::cerr << std::endl;

        /* create dht */
        mUdpBitDht = new UdpBitDht(mUdpStack, &mOwnId, dhtVersion, bootstrapfile, stdfns);
        mUdpStack->addReceiver(mUdpBitDht);

        /* setup callback to here */
        mUdpBitDht->addCallback(this);

	/* setup TOU part */
	tou_init(mUdpStack);

	/* startup the Udp stuff! */
        mUdpBitDht->start();
	mUdpBitDht->startDht();

	// handle configuration.
	loadPeers(mPeersFile);
	storeConfig(mConfigFile);

}

int PeerNet::getOwnId(bdNodeId *id)
{
	*id = mOwnId;
	return 1;
}

#define PN_PEER_STATE_UNKNOWN		0
#define PN_PEER_STATE_SEARCHING		1
#define PN_PEER_STATE_FAILURE		2
#define PN_PEER_STATE_OFFLINE		3
#define PN_PEER_STATE_UNREACHABLE	4
#define PN_PEER_STATE_ONLINE		5

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

	if (mPeers.end() == mPeers.find(filteredId))
	{
		std::cerr << "Adding New Peer: " << filteredId << std::endl;
		mPeers[filteredId] = PeerStatus();
		std::map<std::string, PeerStatus>::iterator it = mPeers.find(filteredId);

		mUdpBitDht->addFindNode(&tmpId, BITDHT_QFLAGS_DO_IDLE);

		it->second.mId = filteredId;
		it->second.mStatusMsg = "Just Added";	
		it->second.mState = PN_PEER_STATE_SEARCHING;
		it->second.mUpdateTS = time(NULL);

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

	out << " LocalPort: " << mPort;

	return out.str();
}





int PeerNet::get_net_peers(std::list<std::string> &peerIds)
{
	std::map<std::string, PeerStatus>::iterator it;
	for(it = mPeers.begin(); it != mPeers.end(); it++)
	{
		peerIds.push_back(it->first);
	}
	return 1;
}

int PeerNet::get_peer_status(std::string id, PeerStatus &status)
{
	std::map<std::string, PeerStatus>::iterator it = mPeers.find(id);
	if (it != mPeers.end())
	{
		status = it->second;
		return 1;
	}
	return 0;
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
		std::cerr << "PeerNet::dhtNodeCallback() From KNOWN PEER: ";
		bdStdPrintId(std::cerr, id);
		std::cerr << " Flags: " << peerflags;
		std::cerr << std::endl;
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

	std::map<std::string, PeerStatus>::iterator it = mPeers.find(strId);
	if (it != mPeers.end())
	{
		switch(status)
		{
			default:
			{
				it->second.mStatusMsg = "Unknown Peer State";
				it->second.mState = PN_PEER_STATE_UNKNOWN;
			}
				break;

			case BITDHT_MGR_QUERY_FAILURE:
			{
				it->second.mStatusMsg = "Search Failed (is DHT working?)";	
				it->second.mState = PN_PEER_STATE_FAILURE;
			}
				break;
			case BITDHT_MGR_QUERY_PEER_OFFLINE:
			{
				it->second.mStatusMsg = "Peer Offline";	
				it->second.mState = PN_PEER_STATE_OFFLINE;
			}
				break;
			case BITDHT_MGR_QUERY_PEER_UNREACHABLE:
			{
				it->second.mStatusMsg = "Peer Unreachable";	
				it->second.mState = PN_PEER_STATE_UNREACHABLE;
			}
				break;
			case BITDHT_MGR_QUERY_PEER_ONLINE:
			{
				it->second.mStatusMsg = "Peer Online";	
				it->second.mState = PN_PEER_STATE_ONLINE;
			}
				break;
		}
		time_t now = time(NULL);
		it->second.mUpdateTS = now;
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


int PeerNet::dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	std::cerr << "PeerNet::dhtValueCallback()";
	std::cerr << std::endl;

	return 1;
}

int PeerNet::dhtConnectCallback(const bdId *srcId, const bdId *proxyId, const bdId *destId,
                                        uint32_t mode, uint32_t point, uint32_t cbtype)
{
	std::cerr << "PeerNet::dhtConnectCallback()";
	std::cerr << std::endl;

	switch(cbtype)
	{
		case BITDHT_CONNECT_CB_AUTH:
		{


		}
		break;
		case BITDHT_CONNECT_CB_PENDING:
		{

		}
		break;
		case BITDHT_CONNECT_CB_START:
		{

		}
		break;
		case BITDHT_CONNECT_CB_PROXY:
		{

		}
		break;
		default:
		case BITDHT_CONNECT_CB_FAILED:
		{

		}
		break;
	}
	return 1;
}




