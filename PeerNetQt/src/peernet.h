#ifndef PEER_NET_INTERFACE_H
#define PEER_NET_INTERFACE_H

/* top-level p2p overlay network interface */

#include <string>
#include <list>

#include "bitdht/bdiface.h"
#include "udp/udpstack.h"
#include "udp/udpbitdht.h"

#include "bitdht/bdstddht.h"

class DhtPeer
{
	public:
	std::string id;
};

class PeerStatus
{
	public:
	std::string mId;
	std::string mStatusMsg;
	uint32_t mState;
	time_t mUpdateTS;
};


class PeerNet: public BitDhtCallback
{
	public:
	PeerNet(std::string id, std::string bootstrapfile, uint16_t port);
	/* GUI interface */

	int getOwnId(bdNodeId *id);

	int add_peer(std::string id);
	int remove_peer(std::string id);


	std::string getPeerStatusString();
	std::string getDhtStatusString();
	int get_dht_peers(int lvl, bdBucket &peers);
	//int get_dht_peers(int lvl, std::list<DhtPeer> &peers);
	int get_net_peers(std::list<std::string> &peerIds);
	int get_peer_status(std::string peerId, PeerStatus &status);

	/* remember peers */
	int storePeers(std::string filepath);
	int loadPeers(std::string filepath);

	int storeConfig(std::string filepath);
	int loadConfig(std::string filepath);
	/* under the hood */



	/**** dht Callback ****/
                // dummy cos not needed for standard dht behaviour;
virtual int dhtNodeCallback(const bdId *id, uint32_t peerflags);

                // must be implemented.
virtual int dhtPeerCallback(const bdId *id, uint32_t status);
virtual int dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status);

                // connection callback. Not required for basic behaviour, but forced for initial development.
virtual int dhtConnectCallback(const bdId *srcId, const bdId *proxyId, const bdId *destId,
                                        uint32_t mode, uint32_t point, uint32_t cbtype); 


	private:

	UdpStack *mUdpStack;
	UdpBitDht *mUdpBitDht;

	std::map<std::string, PeerStatus> mPeers;

	bdNodeId mOwnId;

	std::string mPeersFile;
	std::string mConfigFile;

	uint16_t mPort;
};



#endif
