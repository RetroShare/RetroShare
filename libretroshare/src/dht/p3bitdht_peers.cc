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

#include "tcponudp/udprelay.h"
#include "tcponudp/udpstunner.h"

#include <openssl/sha.h>

#define DEBUG_BITDHT	1

/******************************************************************************************
 ********************************* Existing Interface *************************************
 ******************************************************************************************/
	/* pqiNetAssistConnect - external interface functions */
	/* add / remove peers */
/*****
 * At the moment, findPeer, dropPeer are the only way that peer info enters the dht.
 * This will obviously change, and we will have a list of Friends and FoF,
 * but for now we need to expect that this function will add unknown pids.
 *
 */
#define USE_OLD_DHT_INTERFACE	1

bool 	p3BitDht::findPeer(std::string pid)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::findPeer(" << pid << ")";
	std::cerr << std::endl;
#endif

	DhtPeerDetails *dpd = findInternalRsPeer_locked(pid);
	if (!dpd)
	{
		dpd = addInternalPeer_locked(pid, RSDHT_PEERTYPE_FRIEND);
		if (!dpd)
		{
			/* ERROR */
			return false;
		}
	}

	/* flag as searching */
	dpd->mDhtState = RSDHT_PEERDHT_SEARCHING;

	bdNodeId nid = dpd->mDhtId.id;

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


	DhtPeerDetails *dpd = findInternalRsPeer_locked(pid);
	if (!dpd)
	{
		/* ERROR */
		return false;
	}

	/* flag as searching */
	dpd->mDhtState = RSDHT_PEERDHT_NOT_ACTIVE;

	bdNodeId nid = dpd->mDhtId.id;

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::dropPeer() calling removeFindNode() with pid => NodeId: ";
	bdStdPrintNodeId(std::cerr, &nid);
	std::cerr << std::endl;
#endif

	/* remove in peer */
	mUdpBitDht->removeFindNode(&nid);

	/* not removing from translation */

	return true ;
}


/******************************************************************************************
 ********************************* Basic Peer Details *************************************
 ******************************************************************************************/

int p3BitDht::addFriend(const std::string pid)
{
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	return (NULL != addInternalPeer_locked(pid, RSDHT_PEERTYPE_FRIEND));
}


int p3BitDht::addFriendOfFriend(const std::string pid)
{
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	return (NULL != addInternalPeer_locked(pid, RSDHT_PEERTYPE_FOF));
}


int p3BitDht::addOther(const std::string pid)
{
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	return (NULL != addInternalPeer_locked(pid, RSDHT_PEERTYPE_OTHER));
}


int p3BitDht::removePeer(const std::string pid)
{
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	return removeInternalPeer_locked(pid);
}


/******************************************************************************************
 ********************************* Basic Peer Details *************************************
 ******************************************************************************************/

DhtPeerDetails *p3BitDht::addInternalPeer_locked(const std::string pid, int type)
{
	/* create the data structure */
	if (!havePeerTranslation_locked(pid))
	{
		storeTranslation_locked(pid);
	}

	bdNodeId id;
	if (!lookupNodeId_locked(pid, &id))
	{
		return 0;
	}

	DhtPeerDetails *dpd = findInternalDhtPeer_locked(&id, RSDHT_PEERTYPE_ANY);
	if (!dpd)
	{
		DhtPeerDetails newdpd;
		mPeers[id] = newdpd;
		dpd = findInternalDhtPeer_locked(&id, RSDHT_PEERTYPE_ANY);
	}

	/* what do we need to reset? */
	dpd->mPeerType = type;
	dpd->mDhtId.id = id;
	dpd->mRsId = pid;

	return dpd;
}


int p3BitDht::removeInternalPeer_locked(const std::string pid)
{
	bdNodeId id;
	if (!lookupNodeId_locked(pid, &id))
	{
		return 0;
	}

	std::map<bdNodeId, DhtPeerDetails>::iterator it = mPeers.find(id);
	if (it == mPeers.end())
	{
		return 0;
	}

	mPeers.erase(it);

	// remove the translation?
	removeTranslation_locked(pid);

	return 1;	
}


/* indexed by bdNodeId, as this is the more used call interface */
DhtPeerDetails *p3BitDht::findInternalDhtPeer_locked(const bdNodeId *id, int type)
{
	std::map<bdNodeId, DhtPeerDetails>::iterator it = mPeers.find(*id);
	if (it == mPeers.end())
	{
		return NULL;
	}
	if (type)
	{
		if (it->second.mPeerType != type)
		{
			return NULL;
		}
	}
	return &(it->second);
}


/* interface to get with alt id */
DhtPeerDetails *p3BitDht::findInternalRsPeer_locked(const std::string &pid)
{
	/* create the data structure */
	if (!havePeerTranslation_locked(pid))
	{
		return NULL;
	}

	bdNodeId id;
	if (!lookupNodeId_locked(pid, &id))
	{
		return NULL;
	}

	DhtPeerDetails *dpd = findInternalDhtPeer_locked(&id,RSDHT_PEERTYPE_ANY);

	return dpd;
}


/******************************************************************************************
 *************************** Fundamental Node Translation *********************************
 ******************************************************************************************/

bool p3BitDht::havePeerTranslation_locked(const std::string &pid)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::havePeerTranslation_locked() for : " << pid;
	std::cerr << std::endl;
#endif

	std::map<std::string, bdNodeId>::iterator it;
	it = mTransToNodeId.find(pid);
	if (it == mTransToNodeId.end())
	{
#ifdef DEBUG_BITDHT
		std::cerr << "p3BitDht::havePeerTranslation_locked() failed Missing translation";
		std::cerr << std::endl;
#endif

		return false;
	}

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::havePeerTranslation_locked() Found NodeId: ";
	bdStdPrintNodeId(std::cerr, &(it->second));
	std::cerr << std::endl;
#endif

	return true;
}


int p3BitDht::lookupNodeId_locked(const std::string pid, bdNodeId *id)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::lookupNodeId_locked() for : " << pid;
	std::cerr << std::endl;
#endif

	std::map<std::string, bdNodeId>::iterator it;
	it = mTransToNodeId.find(pid);
	if (it == mTransToNodeId.end())
	{
#ifdef DEBUG_BITDHT
		std::cerr << "p3BitDht::lookupNodeId_locked() failed";
		std::cerr << std::endl;
#endif

		return 0;
	}
	*id = it->second;


#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::lookupNodeId_locked() Found NodeId: ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif

	return 1;
}


int p3BitDht::lookupRsId_locked(const bdNodeId *id, std::string &pid)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::lookupRsId_locked() for : ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif

	std::map<bdNodeId, std::string>::iterator nit;
	nit = mTransToRsId.find(*id);
	if (nit == mTransToRsId.end())
	{
#ifdef DEBUG_BITDHT
		std::cerr << "p3BitDht::lookupRsId_locked() failed";
		std::cerr << std::endl;
#endif

		return 0;
	}
	pid = nit->second;


#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::lookupRsId_locked() Found Matching RsId: " << pid;
	std::cerr << std::endl;
#endif

	return 1;
}

int p3BitDht::storeTranslation_locked(const std::string pid)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::storeTranslation_locked(" << pid << ")";
	std::cerr << std::endl;
#endif

	bdNodeId nid;
	calculateNodeId(pid, &nid);

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::storeTranslation_locked() Converts to NodeId: ";
	bdStdPrintNodeId(std::cerr, &(nid));
	std::cerr << std::endl;
#endif

	mTransToNodeId[pid] = nid;
	mTransToRsId[nid] = pid;

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::storeTranslation_locked() Success";
	std::cerr << std::endl;
#endif

	return 1;
}

int p3BitDht::removeTranslation_locked(const std::string pid)
{

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::removeTranslation_locked(" << pid << ")";
	std::cerr << std::endl;
#endif

	std::map<std::string, bdNodeId>::iterator it = mTransToNodeId.find(pid);
	it = mTransToNodeId.find(pid);
	if (it == mTransToNodeId.end())
	{
		std::cerr << "p3BitDht::removeTranslation_locked() ERROR MISSING TransToNodeId";
		std::cerr << std::endl;
		/* missing */
		return 0;
	}

	bdNodeId nid = it->second;

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::removeTranslation_locked() Found Translation: NodeId: ";
	bdStdPrintNodeId(std::cerr, &(nid));
	std::cerr << std::endl;
#endif


	std::map<bdNodeId, std::string>::iterator nit;
	nit = mTransToRsId.find(nid);
	if (nit == mTransToRsId.end())
	{
		std::cerr << "p3BitDht::removeTranslation_locked() ERROR MISSING TransToRsId";
		std::cerr << std::endl;
		/* inconsistent!!! */
		return 0;
	}

	mTransToNodeId.erase(it);
	mTransToRsId.erase(nit);

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::removeTranslation_locked() SUCCESS";
	std::cerr << std::endl;
#endif

	return 1;
}


/* Adding a little bit of fixed text...
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
