/*
 * libretroshare/src/dht: p3bitdht.h
 *
 * BitDht interface for RetroShare.
 *
 * Copyright 2009-2010 by Robert Fernie.
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


#include "dht/p3bitdht.h"

#include "bitdht/bdstddht.h"
#include "pqi/p3connmgr.h"

#include "tcponudp/udprelay.h"
#include "tcponudp/udpstunner.h"

#include <openssl/sha.h>


/* This is a conversion callback class between pqi interface
 * and the BitDht Interface.
 *
 */

/**** EXTERNAL INTERFACE DHT POINTER *****/
RsDht *rsDht = NULL;

class p3BdCallback: public BitDhtCallback
{
	public:

	p3BdCallback(p3BitDht *parent)
	:mParent(parent) { return; }

virtual int dhtNodeCallback(const bdId *id, uint32_t peerflags)
{
	return mParent->NodeCallback(id, peerflags);
}

virtual int dhtPeerCallback(const bdId *id, uint32_t status)
{
	return mParent->PeerCallback(id, status);
}

virtual int dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	return mParent->ValueCallback(id, key, status);
}

virtual int dhtConnectCallback(const bdId *srcId, const bdId *proxyId, const bdId *destId,
                        uint32_t mode, uint32_t point, uint32_t cbtype, uint32_t errcode)
{ 
	return 0; 
}  

	private:

	p3BitDht *mParent;
};


p3BitDht::p3BitDht(std::string id, pqiConnectCb *cb, UdpStack *udpstack, std::string bootstrapfile)
	:pqiNetAssistConnect(id, cb), dhtMtx("p3BitDht")
{
	mDhtStunner = NULL;
	mProxyStunner = NULL;
	mRelay = NULL;

	std::string dhtVersion = "RS51"; // should come from elsewhere!
	bdNodeId ownId;

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::p3BitDht()" << std::endl;
	std::cerr << "Using Id: " << id;
	std::cerr << std::endl;
	std::cerr << "Using Bootstrap File: " << bootstrapfile;
	std::cerr << std::endl;
	std::cerr << "Converting OwnId to bdNodeId....";
	std::cerr << std::endl;
#endif

	/* setup ownId */
	storeTranslation(id);
	lookupNodeId(id, &ownId);


#ifdef DEBUG_BITDHT
	std::cerr << "Own NodeId: ";
	bdStdPrintNodeId(std::cerr, &ownId);
	std::cerr << std::endl;
#endif

	/* standard dht behaviour */
	bdDhtFunctions *stdfns = new bdStdDht();

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht() startup ... creating UdpBitDht";
	std::cerr << std::endl;
#endif

	/* create dht */
	mUdpBitDht = new UdpBitDht(udpstack, &ownId, dhtVersion, bootstrapfile, stdfns);
	udpstack->addReceiver(mUdpBitDht);

	/* setup callback to here */
	p3BdCallback *bdcb = new p3BdCallback(this);
	mUdpBitDht->addCallback(bdcb);

}

p3BitDht::~p3BitDht()
{
	//udpstack->removeReceiver(mUdpBitDht);
	delete mUdpBitDht;
}


void    p3BitDht::setupConnectBits(UdpStunner *dhtStunner, UdpStunner *proxyStunner, UdpRelayReceiver  *relay)
{
	mDhtStunner = dhtStunner;
	mProxyStunner = proxyStunner;
	mRelay = relay;
}


void    p3BitDht::start()
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::start()";
	std::cerr << std::endl;
#endif

	mUdpBitDht->start(); /* starts up the bitdht thread */

	/* dht switched on by config later. */
}

	/* pqiNetAssist - external interface functions */
void    p3BitDht::enable(bool on)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::enable(" << on << ")";
	std::cerr << std::endl;
#endif

	if (on)
	{
		mUdpBitDht->startDht();
	}
	else
	{
		mUdpBitDht->stopDht();
	}
}
	
void    p3BitDht::shutdown() /* blocking call */
{
	mUdpBitDht->stopDht();
}


void	p3BitDht::restart()
{
	mUdpBitDht->stopDht();
	mUdpBitDht->startDht();
}

bool    p3BitDht::getEnabled()
{
	return (mUdpBitDht->stateDht() != 0);
}

bool    p3BitDht::getActive()
{
	return (mUdpBitDht->stateDht() >= BITDHT_MGR_STATE_ACTIVE);
}

bool    p3BitDht::getNetworkStats(uint32_t &netsize, uint32_t &localnetsize)
{
	netsize = mUdpBitDht->statsNetworkSize();
	localnetsize = mUdpBitDht->statsBDVersionSize();
	return true;
}

	/* pqiNetAssistConnect - external interface functions */
	/* add / remove peers */
bool 	p3BitDht::findPeer(std::string pid)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::findPeer(" << pid << ")";
	std::cerr << std::endl;
#endif

	/* convert id -> NodeId */
	if (!storeTranslation(pid))
	{
		std::cerr << "p3BitDht::findPeer() Failed to storeTranslation";
		std::cerr << std::endl;

		/* error */
		return false;
	}

	bdNodeId nid;
	if (!lookupNodeId(pid, &nid))
	{
		std::cerr << "p3BitDht::findPeer() Failed to lookupNodeId";
		std::cerr << std::endl;

		/* error */
		return false;
	}

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::findPeer() calling AddFindNode() with pid => NodeId: ";
	bdStdPrintNodeId(std::cerr, &nid);
	std::cerr << std::endl;
#endif

	/* add in peer */
	mUdpBitDht->addFindNode(&nid, BITDHT_QFLAGS_DO_IDLE);

	return true ;
}

bool 	p3BitDht::dropPeer(std::string pid)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::dropPeer(" << pid << ")";
	std::cerr << std::endl;
#endif

	/* convert id -> NodeId */
	bdNodeId nid;
	if (!lookupNodeId(pid, &nid))
	{
		std::cerr << "p3BitDht::dropPeer() Failed to lookup NodeId";
		std::cerr << std::endl;

		/* error */
		return false;
	}

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::dropPeer() Translated to NodeId: ";
	bdStdPrintNodeId(std::cerr, &nid);
	std::cerr << std::endl;
#endif

	/* remove in peer */
	mUdpBitDht->removeFindNode(&nid);

	/* remove from translation */
	if (!removeTranslation(pid))
	{
		std::cerr << "p3BitDht::dropPeer() Failed to removeTranslation";
		std::cerr << std::endl;

		/* error */
		return false;
	}

	return true ;
}

	/* extract current peer status */
bool 	p3BitDht::getPeerStatus(std::string id, 
				struct sockaddr_in &laddr, struct sockaddr_in &raddr, 
				uint32_t &type, uint32_t &mode)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::getPeerStatus(" << id << ")";
	std::cerr << std::endl;
#endif


	return false;
}

bool 	p3BitDht::getExternalInterface(struct sockaddr_in &raddr, 
					uint32_t &mode)
{

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::getExternalInterface()";
	std::cerr << std::endl;
#endif


	return false;
}


/* Adding a little bit of fixed test...
 * This allows us to ensure that only compatible peers will find each other
 */

const	uint8_t RS_DHT_VERSION_LEN = 17;
const	uint8_t rs_dht_version_data[RS_DHT_VERSION_LEN] = "RS_VERSION_0.5.1";

/******************** Conversion Functions **************************/
int p3BitDht::calculateNodeId(const std::string pid, bdNodeId *id)
{
	/* generate node id from pid */
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::calculateNodeId() " << pid;
#endif

	/* use a hash to make it impossible to reverse */

        uint8_t sha_hash[SHA_DIGEST_LENGTH];
        memset(sha_hash,0,SHA_DIGEST_LENGTH*sizeof(uint8_t)) ;
        SHA_CTX *sha_ctx = new SHA_CTX;
        SHA1_Init(sha_ctx);

        SHA1_Update(sha_ctx, rs_dht_version_data, RS_DHT_VERSION_LEN);
        SHA1_Update(sha_ctx, pid.c_str(), pid.length());
        SHA1_Final(sha_hash, sha_ctx);

        for(int i = 0; i < SHA_DIGEST_LENGTH && (i < BITDHT_KEY_LEN); i++)
        {
		id->data[i] = sha_hash[i];
        }
	delete sha_ctx;

#ifdef DEBUG_BITDHT
	std::cerr << " => ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif

	return 1;
}

int p3BitDht::lookupNodeId(const std::string pid, bdNodeId *id)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::lookupNodeId() for : " << pid;
	std::cerr << std::endl;
#endif

	RsStackMutex stack(dhtMtx);

	std::map<std::string, bdNodeId>::iterator it;
	it = mTransToNodeId.find(pid);
	if (it == mTransToNodeId.end())
	{
#ifdef DEBUG_BITDHT
		std::cerr << "p3BitDht::lookupNodeId() failed";
		std::cerr << std::endl;
#endif

		return 0;
	}
	*id = it->second;


#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::lookupNodeId() Found NodeId: ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif

	return 1;
}


int p3BitDht::lookupRsId(const bdNodeId *id, std::string &pid)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::lookupRsId() for : ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif

	RsStackMutex stack(dhtMtx);

	std::map<bdNodeId, std::string>::iterator nit;
	nit = mTransToRsId.find(*id);
	if (nit == mTransToRsId.end())
	{
#ifdef DEBUG_BITDHT
		std::cerr << "p3BitDht::lookupRsId() failed";
		std::cerr << std::endl;
#endif

		return 0;
	}
	pid = nit->second;


#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::lookupRsId() Found Matching RsId: " << pid;
	std::cerr << std::endl;
#endif

	return 1;
}

int p3BitDht::storeTranslation(const std::string pid)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::storeTranslation(" << pid << ")";
	std::cerr << std::endl;
#endif

	bdNodeId nid;
	calculateNodeId(pid, &nid);

	RsStackMutex stack(dhtMtx);

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::storeTranslation() Converts to NodeId: ";
	bdStdPrintNodeId(std::cerr, &(nid));
	std::cerr << std::endl;
#endif

	mTransToNodeId[pid] = nid;
	mTransToRsId[nid] = pid;

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::storeTranslation() Success";
	std::cerr << std::endl;
#endif

	return 1;
}

int p3BitDht::removeTranslation(const std::string pid)
{
	RsStackMutex stack(dhtMtx);

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::removeTranslation(" << pid << ")";
	std::cerr << std::endl;
#endif

	std::map<std::string, bdNodeId>::iterator it = mTransToNodeId.find(pid);
	it = mTransToNodeId.find(pid);
	if (it == mTransToNodeId.end())
	{
		std::cerr << "p3BitDht::removeTranslation() ERROR MISSING TransToNodeId";
		std::cerr << std::endl;
		/* missing */
		return 0;
	}

	bdNodeId nid = it->second;

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::removeTranslation() Found Translation: NodeId: ";
	bdStdPrintNodeId(std::cerr, &(nid));
	std::cerr << std::endl;
#endif


	std::map<bdNodeId, std::string>::iterator nit;
	nit = mTransToRsId.find(nid);
	if (nit == mTransToRsId.end())
	{
		std::cerr << "p3BitDht::removeTranslation() ERROR MISSING TransToRsId";
		std::cerr << std::endl;
		/* inconsistent!!! */
		return 0;
	}

	mTransToNodeId.erase(it);
	mTransToRsId.erase(nit);

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::removeTranslation() SUCCESS";
	std::cerr << std::endl;
#endif

	return 1;
}


/********************** Callback Functions **************************/
uint32_t translatebdcbflgs(uint32_t peerflags);

int p3BitDht::NodeCallback(const bdId *id, uint32_t peerflags)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::NodeCallback()";
	std::cerr << std::endl;
#endif

	// XXX THIS IS A BAD HACK TO PREVENT connection attempt to MEDIASENTRY (dht spies)
	// peers... These peers appear to masquerade as your OwnNodeId!!!!
	// which means peers could attempt to connect too?? not sure about this?
	// Anyway they don't appear to REPLY to FIND_NODE requests...
	// So if we ignore these peers, we'll only get the true RS peers!

	// This should be fixed by a collaborative IP filter system,
	// EACH peer identifies dodgey IP (ones spoofing yourself), and shares them
	// This are distributed around RS via a service, and the UDP ignores 
	// all comms from these sources. (AT a low level).

	if (peerflags != BITDHT_PEER_STATUS_RECV_NODES)
	{

#ifdef DEBUG_BITDHT
		std::cerr << "p3BitDht::NodeCallback() Ignoring !FIND_NODE callback from:";
		bdStdPrintNodeId(std::cerr, &(id->id));
		std::cerr << " flags: " << peerflags;
		std::cerr << std::endl;
#endif

		return 0;
	}

	/* is it one that we are interested in? */
	std::string pid;
	/* check for translation */
	if (lookupRsId(&(id->id), pid))
	{
#ifdef DEBUG_BITDHT
		/* we found it ... do callback to p3connmgr */
		std::cerr << "p3BitDht::NodeCallback() FOUND NODE!!!: ";
		bdStdPrintNodeId(std::cerr, &(id->id));
		std::cerr << "-> " << pid << " flags: " << peerflags;
		std::cerr << std::endl;
#endif
			/* send status info to p3connmgr */



		pqiIpAddress dhtpeer;
		dhtpeer.mSrc = RS_CB_DHT;
		dhtpeer.mSeenTime = time(NULL);
		dhtpeer.mAddr = id->addr;

		pqiIpAddrSet addrs;
		addrs.updateExtAddrs(dhtpeer);

                uint32_t type = RS_NET_CONN_UDP_DHT_SYNC;
		uint32_t flags = translatebdcbflgs(peerflags);
		uint32_t source = RS_CB_DHT;

		mConnCb->peerStatus(pid, addrs, type, flags, source);

		return 1;
	}

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::NodeCallback() FAILED TO FIND NODE: ";
	bdStdPrintNodeId(std::cerr, &(id->id));
	std::cerr << std::endl;
#endif

	return 0;
}
uint32_t translatebdcbflgs(uint32_t peerflags)
{
	uint32_t outflags = 0;
	outflags |= RS_NET_FLAGS_ONLINE;
	return outflags;
#if 0

	// The input flags.
#define         BITDHT_PEER_STATUS_RECV_PONG            0x00000001
#define         BITDHT_PEER_STATUS_RECV_NODES           0x00000002
#define         BITDHT_PEER_STATUS_RECV_HASHES          0x00000004

#define         BITDHT_PEER_STATUS_DHT_ENGINE           0x00000100
#define         BITDHT_PEER_STATUS_DHT_APPL             0x00000200
#define         BITDHT_PEER_STATUS_DHT_VERSION          0x00000400

	if (peerflags & ONLINE)
	{
		outflags |= RS_NET_FLAGS_ONLINE;
	}
	if (peerflags & ONLINE)
	{
		outflags |= RS_NET_FLAGS_EXTERNAL_ADDR | RS_NET_FLAGS_STABLE_UDP;
	}
#endif
}


int p3BitDht::PeerCallback(const bdId *id, uint32_t status)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::PeerCallback() bdId: ";
	bdStdPrintId(std::cerr, id);
	std::cerr << std::endl;
#endif

	/* is it one that we are interested in? */
	std::string pid;
	/* check for translation */
	if (lookupRsId(&(id->id), pid))
	{
#ifdef DEBUG_BITDHT
		std::cerr << "p3BitDht::PeerCallback() => RsId: ";
		std::cerr << pid << " status: " << status;
		std::cerr << " NOOP for NOW";
		std::cerr << std::endl;
#endif

		bool connect = false;
		switch(status)
		{
  			case BITDHT_MGR_QUERY_FAILURE:
				/* do nothing */
#ifdef DEBUG_BITDHT
				std::cerr << "p3BitDht::PeerCallback() QUERY FAILURE ... do nothin ";
				std::cerr << std::endl;
#endif

			break;

			case BITDHT_MGR_QUERY_PEER_OFFLINE:
				/* do nothing */

#ifdef DEBUG_BITDHT
				std::cerr << "p3BitDht::PeerCallback() QUERY PEER OFFLINE ... do nothin ";
				std::cerr << std::endl;
#endif

			break;

			case BITDHT_MGR_QUERY_PEER_UNREACHABLE:
				/* do nothing */

#ifdef DEBUG_BITDHT
				std::cerr << "p3BitDht::PeerCallback() QUERY PEER UNREACHABLE ... flag? / do nothin ";
				std::cerr << std::endl;
#endif


			break;

			case BITDHT_MGR_QUERY_PEER_ONLINE:
				/* do something */

#ifdef DEBUG_BITDHT
				std::cerr << "p3BitDht::PeerCallback() QUERY PEER ONLINE ... try udp connection";
				std::cerr << std::endl;
#endif

				connect = true;
			break;
		}

		if (connect)
		{
#ifdef DEBUG_BITDHT
			std::cerr << "p3BitDht::PeerCallback() mConnCb->peerConnectRequest()";
			std::cerr << std::endl;
#endif

			mConnCb->peerConnectRequest(pid, id->addr, RS_CB_DHT);
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
#ifdef DEBUG_BITDHT
		std::cerr << "p3BitDht::PeerCallback()";
		std::cerr << " FAILED TO TRANSLATE ID ";
		std::cerr << " status: " << status;
		std::cerr << " NOOP for NOW";
		std::cerr << std::endl;
#endif
	}

	return 0;
}

int p3BitDht::ValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::ValueCallback() NOOP for NOW";
	std::cerr << std::endl;
#endif

	/* ignore for now */
	return 0;
}

