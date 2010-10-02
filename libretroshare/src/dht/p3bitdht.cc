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

#include <openssl/sha.h>


/* This is a conversion callback class between pqi interface
 * and the BitDht Interface.
 *
 */

class p3BdCallback: public BitDhtCallback
{
	public:

	p3BdCallback(p3BitDht *parent)
	:mParent(parent) { return; }

virtual int dhtNodeCallback(const bdId *id, uint32_t peerflags)
{
	return mParent->NodeCallback(id, peerflags);
}

virtual int dhtPeerCallback(const bdNodeId *id, uint32_t status)
{
	return mParent->PeerCallback(id, status);
}

virtual int dhtValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	return mParent->ValueCallback(id, key, status);
}

	private:

	p3BitDht *mParent;
};


p3BitDht::p3BitDht(std::string id, pqiConnectCb *cb, UdpStack *udpstack, std::string bootstrapfile)
	:pqiNetAssistConnect(id, cb)
{
	std::string dhtVersion = "RS51"; // should come from elsewhere!
	bdNodeId ownId;

	std::cerr << "p3BitDht::p3BitDht()" << std::endl;
	std::cerr << "Using Id: " << id;
	std::cerr << std::endl;
	std::cerr << "Using Bootstrap File: " << bootstrapfile;
	std::cerr << std::endl;
	std::cerr << "Converting OwnId to bdNodeId....";
	std::cerr << std::endl;

	/* setup ownId */
	storeTranslation(id);
	lookupNodeId(id, &ownId);


	std::cerr << "Own NodeId: ";
	bdStdPrintNodeId(std::cerr, &ownId);
	std::cerr << std::endl;


	/* standard dht behaviour */
	bdDhtFunctions *stdfns = new bdStdDht();

	std::cerr << "p3BitDht() startup ... creating UdpBitDht";
	std::cerr << std::endl;

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

void    p3BitDht::start()
{
	std::cerr << "p3BitDht::start()";
	std::cerr << std::endl;

	mUdpBitDht->start(); /* starts up the bitdht thread */
}

	/* pqiNetAssist - external interface functions */
void    p3BitDht::enable(bool on)
{
	//mUdpBitDht->enable(on);
}
	
void    p3BitDht::shutdown() /* blocking call */
{
	//mUdpBitDht->shutdown();
}


void	p3BitDht::restart()
{
	//mUdpBitDht->restart();
}

bool    p3BitDht::getEnabled()
{
	return false;
}

bool    p3BitDht::getActive()
{
	return false;
}

	/* pqiNetAssistConnect - external interface functions */
	/* add / remove peers */
bool 	p3BitDht::findPeer(std::string pid)
{
	std::cerr << "p3BitDht::findPeer(" << pid << ")";
	std::cerr << std::endl;

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

	std::cerr << "p3BitDht::findPeer() calling AddFindNode() with pid => NodeId: ";
	bdStdPrintNodeId(std::cerr, &nid);
	std::cerr << std::endl;

	/* add in peer */
	mUdpBitDht->addFindNode(&nid, BITDHT_QFLAGS_DO_IDLE);

	return true ;
}

bool 	p3BitDht::dropPeer(std::string pid)
{
	std::cerr << "p3BitDht::dropPeer(" << pid << ")";
	std::cerr << std::endl;

	/* convert id -> NodeId */
	bdNodeId nid;
	if (!lookupNodeId(pid, &nid))
	{
		std::cerr << "p3BitDht::dropPeer() Failed to lookup NodeId";
		std::cerr << std::endl;

		/* error */
		return false;
	}

	std::cerr << "p3BitDht::dropPeer() Translated to NodeId: ";
	bdStdPrintNodeId(std::cerr, &nid);
	std::cerr << std::endl;

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
	std::cerr << "p3BitDht::getPeerStatus(" << id << ")";
	std::cerr << std::endl;


	return false;
}

bool 	p3BitDht::getExternalInterface(struct sockaddr_in &raddr, 
					uint32_t &mode)
{

	std::cerr << "p3BitDht::getExternalInterface()";
	std::cerr << std::endl;


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
	std::cerr << "p3BitDht::calculateNodeId() " << pid;

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

	std::cerr << " => ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;

	return 1;
}

int p3BitDht::lookupNodeId(const std::string pid, bdNodeId *id)
{
	std::cerr << "p3BitDht::lookupNodeId() for : " << pid;
	std::cerr << std::endl;

	RsStackMutex stack(dhtMtx);

	std::map<std::string, bdNodeId>::iterator it;
	it = mTransToNodeId.find(pid);
	if (it == mTransToNodeId.end())
	{
		std::cerr << "p3BitDht::lookupNodeId() failed";
		std::cerr << std::endl;

		return 0;
	}
	*id = it->second;


	std::cerr << "p3BitDht::lookupNodeId() Found NodeId: ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;

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


	std::cerr << "p3BitDht::lookupRsId() Found Matching RsId: " << pid;
	std::cerr << std::endl;

	return 1;
}

int p3BitDht::storeTranslation(const std::string pid)
{
	std::cerr << "p3BitDht::storeTranslation(" << pid << ")";
	std::cerr << std::endl;

	bdNodeId nid;
	calculateNodeId(pid, &nid);

	RsStackMutex stack(dhtMtx);

	std::cerr << "p3BitDht::storeTranslation() Converts to NodeId: ";
	bdStdPrintNodeId(std::cerr, &(nid));
	std::cerr << std::endl;

	mTransToNodeId[pid] = nid;
	mTransToRsId[nid] = pid;

	std::cerr << "p3BitDht::storeTranslation() Success";
	std::cerr << std::endl;

	return 1;
}

int p3BitDht::removeTranslation(const std::string pid)
{
	RsStackMutex stack(dhtMtx);

	std::cerr << "p3BitDht::removeTranslation(" << pid << ")";
	std::cerr << std::endl;

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

	std::cerr << "p3BitDht::removeTranslation() Found Translation: NodeId: ";
	bdStdPrintNodeId(std::cerr, &(nid));
	std::cerr << std::endl;


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

	std::cerr << "p3BitDht::removeTranslation() SUCCESS";
	std::cerr << std::endl;

	return 1;
}


/********************** Callback Functions **************************/
int p3BitDht::NodeCallback(const bdId *id, uint32_t peerflags)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::NodeCallback()";
	std::cerr << std::endl;
#endif

	/* is it one that we are interested in? */
	std::string pid;
	/* check for translation */
	if (lookupRsId(&(id->id), pid))
	{
		/* we found it ... do callback to p3connmgr */
		//uint32_t cbflags = ONLINE | UNREACHABLE;

		std::cerr << "p3BitDht::NodeCallback() FOUND NODE!!!: ";
		bdStdPrintNodeId(std::cerr, &(id->id));
		std::cerr << "-> " << pid << " flags: " << peerflags;
		std::cerr << std::endl;

		/* add address to set */
		pqiIpAddrSet addrs;
		pqiIpAddress addr;

		addr.mAddr = id->addr;
		addr.mSeenTime = time(NULL);
		addr.mSrc = 0;

		addrs.updateExtAddrs(addr);
		int type = 0;

		/* callback to say they are online */
                mConnCb->peerStatus(pid, addrs, type, 0, RS_CB_DHT);

		return 1;
	}

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::NodeCallback() FAILED TO FIND NODE: ";
	bdStdPrintNodeId(std::cerr, &(id->id));
	std::cerr << std::endl;
#endif

	return 0;
}

int p3BitDht::PeerCallback(const bdNodeId *id, uint32_t status)
{
	std::cerr << "p3BitDht::PeerCallback() NodeId: ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;

	/* is it one that we are interested in? */
	std::string pid;
	/* check for translation */
	if (lookupRsId(id, pid))
	{
		std::cerr << "p3BitDht::PeerCallback() => RsId: ";
		std::cerr << pid << " status: " << status;
		std::cerr << " NOOP for NOW";
		std::cerr << std::endl;


		/* we found it ... do callback to p3connmgr */
		//uint32_t cbflags = ONLINE | REACHABLE;

		/* callback to say they are online */
                //mConnCb->peerStatus(ent.id, addrs, ent.type, 0, RS_CB_DHT);
        	//mConnCb->peerConnectRequest(peer.id, peer.laddr, RS_CB_DHT);

		return 1;
	}
	else
	{
		std::cerr << "p3BitDht::PeerCallback()";
		std::cerr << " FAILED TO TRANSLATE ID ";
		std::cerr << " status: " << status;
		std::cerr << " NOOP for NOW";
		std::cerr << std::endl;
	}

	return 0;
}

int p3BitDht::ValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	std::cerr << "p3BitDht::ValueCallback() NOOP for NOW";
	std::cerr << std::endl;

	/* ignore for now */
	return 0;
}

