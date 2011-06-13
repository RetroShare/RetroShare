#ifndef PEER_NET_INTERFACE_H
#define PEER_NET_INTERFACE_H

/* top-level p2p overlay network interface */

#include <string>
#include <list>

#include "bitdht/bdiface.h"
#include "udp/udpstack.h"
#include "udp/udpbitdht.h"

#include "bitdht/bdstddht.h"

#include "tcponudp/udpstunner.h"

#define PN_DHT_STATE_UNKNOWN           0
#define PN_DHT_STATE_SEARCHING         1
#define PN_DHT_STATE_FAILURE           2
#define PN_DHT_STATE_OFFLINE           3
#define PN_DHT_STATE_UNREACHABLE       4
#define PN_DHT_STATE_ONLINE            5

#define PN_PEER_STATE_DISCONNECTED   		1
#define PN_PEER_STATE_DENIED_NOT_FRIEND   	2
#define PN_PEER_STATE_DENIED_UNAVAILABLE_MODE   3
#define PN_PEER_STATE_DENIED_OVERLOADED_MODE   	4
#define PN_PEER_STATE_UDP_FAILED      		5
#define PN_PEER_STATE_UDP_CLOSED      		6

#define PN_PEER_STATE_CONNECTION_INITIATED  	7
#define PN_PEER_STATE_CONNECTION_AUTHORISED  	8
#define PN_PEER_STATE_UDP_STARTED      		9
#define PN_PEER_STATE_CONNECTED        		10

class DhtPeer
{
	public:
	std::string id;
};

class PeerStatus
{
	public:
	std::string mId;

	/* DHT Status */
	std::string        mDhtStatusMsg;
	uint32_t           mDhtState;
	struct sockaddr_in mDhtAddr;
	time_t             mDhtUpdateTS;

	/* Connection Status */
	std::string        mPeerStatusMsg;
	uint32_t           mPeerState;
	struct sockaddr_in mPeerAddr;
	time_t             mPeerUpdateTS;
	
	int		   mPeerFd;
	time_t             mPeerConnTS;
	std::string	   mPeerIncoming;
};


#define PEERNET_ACTION_TYPE_CONNECT     1
#define PEERNET_ACTION_TYPE_AUTHORISE   2
#define PEERNET_ACTION_TYPE_START	3

class PeerAction
{
	public:

	uint32_t mType;
	bdId mSrcId;
	bdId mProxyId;
	bdId mDestId;
	uint32_t mMode;
	uint32_t mPoint;
	uint32_t mAnswer;
};


class PeerNet: public BitDhtCallback
{
	public:
	PeerNet(std::string id, std::string bootstrapfile, uint16_t port);

	void setUdpStackRestrictions(std::list<std::pair<uint16_t, uint16_t> > &restrictions);
	void init();

	int getOwnId(bdNodeId *id);

	int add_peer(std::string id);
	int remove_peer(std::string id);

	std::string getPeerStatusString();
	std::string getDhtStatusString();
	int get_dht_peers(int lvl, bdBucket &peers);
	//int get_dht_peers(int lvl, std::list<DhtPeer> &peers);
	int get_net_peers(std::list<std::string> &peerIds);
	int get_peer_status(std::string peerId, PeerStatus &status);

	int get_net_failedpeers(std::list<std::string> &peerIds);
	int get_failedpeer_status(std::string peerId, PeerStatus &status);

	/* remember peers */
	int storePeers(std::string filepath);
	int loadPeers(std::string filepath);

	int storeConfig(std::string filepath);
	int loadConfig(std::string filepath);
	/* under the hood */

	int tick();
	int doActions();

	void sendMessage(std::string msg);
	int  getMessage(std::string id, std::string &msg);

	/**** dht Callback ****/
                // dummy cos not needed for standard dht behaviour;
virtual int dhtNodeCallback(const bdId *id, uint32_t peerflags);

                // must be implemented.
virtual int dhtPeerCallback(const bdId *id, uint32_t status);
virtual int dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status);

                // connection callback. Not required for basic behaviour, but forced for initial development.
virtual int dhtConnectCallback(const bdId *srcId, const bdId *proxyId, const bdId *destId,
                                        uint32_t mode, uint32_t point, uint32_t cbtype); 


	/**** Connection Handling ******/
	void monitorConnections();

	void removeRelayConnection(const bdId *srcId, const bdId *destId, int mode);
	void installRelayConnection(const bdId *srcId, const bdId *destId, int mode);
	void initiateConnection(const bdId *srcId, const bdId *proxyId, const bdId *destId, 
			uint32_t mode, uint32_t loc, uint32_t answer);

	int checkConnectionAllowed(const bdId *peerId, int mode);
	int checkProxyAllowed(const bdId *srcId, const bdId *destId, int mode);


	private:

	UdpStack *mUdpStack;
	UdpBitDht *mUdpBitDht;

	UdpStunner *mDhtStunner;
	//UdpStunner *mSyncStunner;


	bdNodeId mOwnId;

	std::string mPeersFile;
	std::string mConfigFile;
	std::string mBootstrapFile;

	uint16_t mPort;

	/* port restrictions */
	bool mDoUdpStackRestrictions;
	std::list<std::pair<uint16_t, uint16_t> > mUdpStackRestrictions;


	/* below here must be mutex protected */
	bdMutex mPeerMutex;

	std::map<std::string, PeerStatus> mPeers;
	std::map<std::string, PeerStatus> mFailedPeers; /* peers that have tried to connect to us */

	/* Connection Action Queue */
	std::list<PeerAction> mActions;
};



#endif
