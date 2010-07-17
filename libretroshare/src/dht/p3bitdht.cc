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

	/* setup ownId */
	storeTranslation(id);
	lookupNodeId(id, &ownId);

	/* standard dht behaviour */
	bdDhtFunctions *stdfns = new bdStdDht();

	/* create dht */
	mUdpBitDht = new UdpBitDht(udpstack, &ownId, dhtVersion, bootstrapfile, stdfns);

	/* setup callback to here */
	p3BdCallback *bdcb = new p3BdCallback(this);
	mUdpBitDht->addCallback(bdcb);

}

p3BitDht::~p3BitDht()
{
	delete mUdpBitDht;
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
	/* convert id -> NodeId */
	if (!storeTranslation(pid))
	{
		/* error */
		return false;
	}

	bdNodeId nid;
	if (!lookupNodeId(pid, &nid))
	{
		/* error */
		return false;
	}

	/* add in peer */
	mUdpBitDht->addFindNode(&nid, 0);

}

bool 	p3BitDht::dropPeer(std::string pid)
{
	/* convert id -> NodeId */
	bdNodeId nid;
	if (!lookupNodeId(pid, &nid))
	{
		/* error */
		return false;
	}

	/* remove in peer */
	mUdpBitDht->removeFindNode(&nid);

	/* remove from translation */
	if (!removeTranslation(pid))
	{
		/* error */
		return false;
	}
}

	/* extract current peer status */
bool 	p3BitDht::getPeerStatus(std::string id, 
			struct sockaddr_in &raddr, uint32_t &mode)
{


	return false;
}

bool 	p3BitDht::getExternalInterface(struct sockaddr_in &raddr, 
					uint32_t &mode)
{


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
	return 1;
}

int p3BitDht::lookupNodeId(const std::string pid, bdNodeId *id)
{
	RsStackMutex stack(dhtMtx);

	std::map<std::string, bdNodeId>::iterator it;
	it = mTransToNodeId.find(pid);
	if (it == mTransToNodeId.end())
	{
		return 0;
	}
	*id = it->second;

	return 1;
}


int p3BitDht::lookupRsId(const bdNodeId *id, std::string &pid)
{
	RsStackMutex stack(dhtMtx);

	std::map<bdNodeId, std::string>::iterator nit;
	nit = mTransToRsId.find(*id);
	if (nit == mTransToRsId.end())
	{
		return 0;
	}
	pid = nit->second;

	return 1;
}

int p3BitDht::storeTranslation(const std::string pid)
{
	bdNodeId nid;
	calculateNodeId(pid, &nid);

	RsStackMutex stack(dhtMtx);

	mTransToNodeId[pid] = nid;
	mTransToRsId[nid] = pid;

	return 1;
}

int p3BitDht::removeTranslation(const std::string pid)
{
	RsStackMutex stack(dhtMtx);

	std::map<std::string, bdNodeId>::iterator it = mTransToNodeId.find(pid);
	it = mTransToNodeId.find(pid);
	if (it == mTransToNodeId.end())
	{
		/* missing */
		return 0;
	}

	bdNodeId nid = it->second;
	std::map<bdNodeId, std::string>::iterator nit;
	nit = mTransToRsId.find(nid);
	if (nit == mTransToRsId.end())
	{
		/* inconsistent!!! */
		return 0;
	}

	mTransToNodeId.erase(it);
	mTransToRsId.erase(nit);

	return 1;
}


/********************** Callback Functions **************************/
int p3BitDht::NodeCallback(const bdId *id, uint32_t peerflags)
{
	/* is it one that we are interested in? */
	std::string pid;
	/* check for translation */
	if (lookupRsId(&(id->id), pid))
	{
		/* we found it ... do callback to p3connmgr */
		//uint32_t cbflags = ONLINE | UNREACHABLE;

		return 1;
	}

	return 0;
}

int p3BitDht::PeerCallback(const bdNodeId *id, uint32_t status)
{
	/* is it one that we are interested in? */
	std::string pid;
	/* check for translation */
	if (lookupRsId(id, pid))
	{
		/* we found it ... do callback to p3connmgr */
		//uint32_t cbflags = ONLINE | REACHABLE;

		return 1;
	}

	return 0;
}

int p3BitDht::ValueCallback(const bdNodeId *id, std::string key, uint32_t status)
{
	/* ignore for now */
	return 0;
}

