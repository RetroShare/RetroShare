/*******************************************************************************
 * libretroshare/src/dht: p3bitdht_peernet.cc                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2009-2010 by Robert Fernie. <drbob@lunamutt.com>                  *
 * Copyright (C) 2015-2018  Gioacchino Mazzurco <gio@eigenlab.org>             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include "dht/p3bitdht.h"

#include "bitdht/bdstddht.h"

#include "tcponudp/udprelay.h"
#include "tcponudp/udpstunner.h"

#include <openssl/sha.h>

/***
 *
 * #define DEBUG_BITDHT	1
 * #define DEBUG_BITDHT_TRANSLATE	1
 *
 **/

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


bool 	p3BitDht::findPeer(const RsPeerId& pid)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::findPeer(" << pid << ")";
	std::cerr << std::endl;
#endif
	bdNodeId nid;

	{
		RsStackMutex stack(dhtMtx);    /********* LOCKED *********/
	
		DhtPeerDetails *dpd = findInternalRsPeer_locked(pid);
		if (!dpd)
		{
			dpd = addInternalPeer_locked(pid, RsDhtPeerType::FRIEND);
			if (!dpd)
			{
				/* ERROR */
#ifdef DEBUG_BITDHT
				std::cerr << "p3BitDht::findPeer() ERROR installing InternalPeer";
				std::cerr << std::endl;
#endif
				return false;
			}
	
			/* new entry... what do we need to set? */
			dpd->mDhtState = RsDhtPeerDht::SEARCHING;
	
#ifdef DEBUG_BITDHT
			std::cerr << "p3BitDht::findPeer() Installed new DhtPeer with pid => NodeId: ";
			bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
			std::cerr << std::endl;
#endif
		}
		else
		{
			/* old entry */
#ifdef DEBUG_BITDHT
			std::cerr << "p3BitDht::findPeer() Reactivating DhtPeer with pid => NodeId: ";
			bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
			std::cerr << std::endl;
#endif
	
			if (dpd->mDhtState != RsDhtPeerDht::NOT_ACTIVE)
			{
#ifdef DEBUG_BITDHT
				std::cerr << "p3BitDht::findPeer() WARNING DhtState is Already Active!";
				bdStdPrintNodeId(std::cerr, &(dpd->mDhtId.id));
				std::cerr << std::endl;
#endif
			}
			else
			{
				/* flag as searching */
				dpd->mDhtState = RsDhtPeerDht::SEARCHING;
#ifdef DEBUG_BITDHT
				std::cerr << "p3BitDht::findPeer() Marking Old Peer as SEARCHING";
				std::cerr << std::endl;
#endif
			}
	
		}
	
		nid = dpd->mDhtId.id;

#ifdef DEBUG_BITDHT
		std::cerr << "p3BitDht::findPeer() calling AddFindNode() with pid => NodeId: ";
		bdStdPrintNodeId(std::cerr, &nid);
		std::cerr << std::endl;
#endif

	}

	/* add in peer */
	mUdpBitDht->addFindNode(&nid, BITDHT_QFLAGS_DO_IDLE | BITDHT_QFLAGS_UPDATES);

	return true ;
}

bool 	p3BitDht::dropPeer(const RsPeerId& pid)
{
#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::dropPeer(" << pid << ")";
	std::cerr << std::endl;
#endif

	bdNodeId nid;

	{
		RsStackMutex stack(dhtMtx);    /********* LOCKED *********/
	
		DhtPeerDetails *dpd = findInternalRsPeer_locked(pid);
		if (!dpd)
		{
#ifdef DEBUG_BITDHT
			std::cerr << "p3BitDht::dropPeer(" << pid << ") HACK TO INCLUDE FRIEND AS NON-ACTIVE PEER";
			std::cerr << std::endl;
#endif
	
			//addFriend(pid);
			dpd = addInternalPeer_locked(pid, RsDhtPeerType::FOF);
			
			return false;
		}
	
		/* flag as searching */
		dpd->mDhtState = RsDhtPeerDht::NOT_ACTIVE;
	
		nid = dpd->mDhtId.id;
	
#ifdef DEBUG_BITDHT
		std::cerr << "p3BitDht::dropPeer() calling removeFindNode() with pid => NodeId: ";
		bdStdPrintNodeId(std::cerr, &nid);
		std::cerr << std::endl;
#endif
	}
	
	/* remove in peer */
	mUdpBitDht->removeFindNode(&nid);

	/* not removing from translation */

	return true ;
}


/******************************************************************************************
 ********************************* Basic Peer Details *************************************
 ******************************************************************************************/

int p3BitDht::addBadPeer( const sockaddr_storage &addr, uint32_t /*reason*/,
                          uint32_t /*flags*/, uint32_t /*age*/ )
{
	//mUdpBitDht->updateKnownPeer(&id, 0, bdflags);

	sockaddr_in addrv4;
	sockaddr_storage tmpaddr;
	sockaddr_storage_copy(addr, tmpaddr);
	if(!sockaddr_storage_ipv6_to_ipv4(tmpaddr))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error: got non IPv4 address!"
		          << std::endl;
		sockaddr_storage_dump(addr);
		print_stacktrace();
		return -EINVAL;
	}

	struct sockaddr_in *ap = (struct sockaddr_in *) &tmpaddr;

	// convert.
	addrv4.sin_family = ap->sin_family;
	addrv4.sin_addr = ap->sin_addr;
	addrv4.sin_port = ap->sin_port;	
	
#ifdef RS_USE_DHT_STUNNER
	if (mDhtStunner)
	{
		mDhtStunner->dropStunPeer(addrv4);
	}
	if (mProxyStunner)
	{
		mProxyStunner->dropStunPeer(addrv4);
	}
#endif // RS_USE_DHT_STUNNER
	return 1;
}


int p3BitDht::addKnownPeer( const RsPeerId &pid,
                            const sockaddr_storage &addr, uint32_t flags )
{
	sockaddr_in addrv4;
	sockaddr_clear(&addrv4);

	sockaddr_storage tmpaddr;
	sockaddr_storage_copy(addr, tmpaddr);
	if( !sockaddr_storage_isnull(addr) &&
	         !sockaddr_storage_ipv6_to_ipv4(tmpaddr) )
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error: got non IPv4 address!"
		          << std::endl;
		sockaddr_storage_dump(addr);
		print_stacktrace();
		return -EINVAL;
	}

	// convert.
	struct sockaddr_in *ap = (struct sockaddr_in *) &tmpaddr;
	addrv4.sin_family = ap->sin_family;
	addrv4.sin_addr = ap->sin_addr;
	addrv4.sin_port = ap->sin_port;

	RsDhtPeerType p3type = RsDhtPeerType::ANY;
	int bdflags = 0;
	bdId id;
	bool isOwnId = false;

	switch(flags & NETASSIST_KNOWN_PEER_TYPE_MASK)
	{
		default:
			return 0;
			break;
		case NETASSIST_KNOWN_PEER_WHITELIST:
		    p3type = RsDhtPeerType::OTHER;
			bdflags = BITDHT_PEER_STATUS_DHT_WHITELIST;

			break;
		case NETASSIST_KNOWN_PEER_FOF:
		    p3type = RsDhtPeerType::FOF;
			bdflags = BITDHT_PEER_STATUS_DHT_FOF;

			break;
		case NETASSIST_KNOWN_PEER_FRIEND:
		    p3type = RsDhtPeerType::FRIEND;
			bdflags = BITDHT_PEER_STATUS_DHT_FRIEND;

			break;
		case NETASSIST_KNOWN_PEER_RELAY:
		    p3type = RsDhtPeerType::OTHER;
			bdflags = BITDHT_PEER_STATUS_DHT_RELAY_SERVER;

			break;
		case NETASSIST_KNOWN_PEER_SELF:
		    p3type = RsDhtPeerType::OTHER;
			bdflags = BITDHT_PEER_STATUS_DHT_SELF;
			isOwnId = true;


			break;
	}

	if (flags & NETASSIST_KNOWN_PEER_ONLINE)
	{
		bdflags |= BD_FRIEND_ENTRY_ONLINE;
	}

	if (!isOwnId)
	{
		RS_STACK_MUTEX(dhtMtx);
		DhtPeerDetails *dpd = addInternalPeer_locked(pid, p3type);
	
	
		if (bdflags & BD_FRIEND_ENTRY_ONLINE)
		{
			/* can we update the address? */
			//dpd->mDhtId.addr = addr;
		}
	
	
		id.id = dpd->mDhtId.id;
		id.addr = addrv4;
	}
	else
	{
		// shouldn't use own id without mutex - but it is static!
		id.id = mOwnDhtId;
		id.addr = addrv4;
	}

	mUdpBitDht->updateKnownPeer(&id, 0, bdflags);

	return 1;
}


/* Total duplicate of above function - can't be helped, should merge somehow */
int p3BitDht::addKnownNode(const bdId *id, uint32_t flags) 
{
	int bdflags = 0;

	switch(flags & NETASSIST_KNOWN_PEER_TYPE_MASK)
	{
		default:
			return 0;
			break;
		case NETASSIST_KNOWN_PEER_WHITELIST:
			bdflags = BITDHT_PEER_STATUS_DHT_WHITELIST;

			break;
		case NETASSIST_KNOWN_PEER_FOF:
			bdflags = BITDHT_PEER_STATUS_DHT_FOF;

			break;
		case NETASSIST_KNOWN_PEER_FRIEND:
			bdflags = BITDHT_PEER_STATUS_DHT_FRIEND;

			break;
		case NETASSIST_KNOWN_PEER_RELAY:
			bdflags = BITDHT_PEER_STATUS_DHT_RELAY_SERVER;

			break;
		case NETASSIST_KNOWN_PEER_SELF:
			bdflags = BITDHT_PEER_STATUS_DHT_SELF;

			break;
	}

	if (flags & NETASSIST_KNOWN_PEER_ONLINE)
	{
		bdflags |= BD_FRIEND_ENTRY_ONLINE;
	}

	mUdpBitDht->updateKnownPeer(id, 0, bdflags);

	return 1;
}

	

#if 0
int p3BitDht::addFriend(const std::string pid)
{
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	return (NULL != addInternalPeer_locked(pid, RsDhtPeerType::FRIEND));
}


int p3BitDht::addFriendOfFriend(const std::string pid)
{
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	return (NULL != addInternalPeer_locked(pid, RsDhtPeerType::FOF));
}


int p3BitDht::addOther(const std::string pid)
{
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	return (NULL != addInternalPeer_locked(pid, RsDhtPeerType::OTHER));
}
#endif


int p3BitDht::removePeer(const RsPeerId& pid)
{
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	return removeInternalPeer_locked(pid);
}


/******************************************************************************************
 ********************************* Basic Peer Details *************************************
 ******************************************************************************************/

DhtPeerDetails *p3BitDht::addInternalPeer_locked(const RsPeerId& pid, RsDhtPeerType type)
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

	DhtPeerDetails *dpd = findInternalDhtPeer_locked(&id, RsDhtPeerType::ANY);
	if (!dpd)
	{
        DhtPeerDetails newdpd;

        newdpd.mDhtId.id = id;
        newdpd.mRsId = pid;
		    newdpd.mDhtState = RsDhtPeerDht::NOT_ACTIVE;
			newdpd.mPeerType = RsDhtPeerType::ANY;

        mPeers[id] = newdpd;
		dpd = findInternalDhtPeer_locked(&id, RsDhtPeerType::ANY);

        if(dpd == NULL)
        {
            std::cerr << "(EE) inconsistency error in p3BitDht::addInternalPeer_locked() Cannot find peer that was just added." << std::endl;
            return NULL;
        }
	}

	/* what do we need to reset? */
	dpd->mPeerType = type;

	return dpd;
}


int p3BitDht::removeInternalPeer_locked(const RsPeerId& pid)
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
DhtPeerDetails *p3BitDht::findInternalDhtPeer_locked(const bdNodeId *id, RsDhtPeerType type)
{
	std::map<bdNodeId, DhtPeerDetails>::iterator it = mPeers.find(*id);
	if (it == mPeers.end())
	{
		return NULL;
	}
	if (type != RsDhtPeerType::ANY)
	{
		if (it->second.mPeerType != type)
		{
			return NULL;
		}
	}
	return &(it->second);
}


/* interface to get with alt id */
DhtPeerDetails *p3BitDht::findInternalRsPeer_locked(const RsPeerId &pid)
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

	DhtPeerDetails *dpd = findInternalDhtPeer_locked(&id,RsDhtPeerType::ANY);

	return dpd;
}


/******************************************************************************************
 *************************** Fundamental Node Translation *********************************
 ******************************************************************************************/

bool p3BitDht::havePeerTranslation_locked(const RsPeerId &pid)
{
#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::havePeerTranslation_locked() for : " << pid;
	std::cerr << std::endl;
#endif

	std::map<RsPeerId, bdNodeId>::iterator it;
	it = mTransToNodeId.find(pid);
	if (it == mTransToNodeId.end())
	{
#ifdef DEBUG_BITDHT_TRANSLATE
		std::cerr << "p3BitDht::havePeerTranslation_locked() failed Missing translation";
		std::cerr << std::endl;
#endif

		return false;
	}

#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::havePeerTranslation_locked() Found NodeId: ";
	bdStdPrintNodeId(std::cerr, &(it->second));
	std::cerr << std::endl;
#endif

	return true;
}


int p3BitDht::lookupNodeId_locked(const RsPeerId& pid, bdNodeId *id)
{
#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::lookupNodeId_locked() for : " << pid;
	std::cerr << std::endl;
#endif

	std::map<RsPeerId, bdNodeId>::iterator it;
	it = mTransToNodeId.find(pid);
	if (it == mTransToNodeId.end())
	{
#ifdef DEBUG_BITDHT_TRANSLATE
		std::cerr << "p3BitDht::lookupNodeId_locked() failed";
		std::cerr << std::endl;
#endif

		return 0;
	}
	*id = it->second;


#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::lookupNodeId_locked() Found NodeId: ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif

	return 1;
}


int p3BitDht::lookupRsId_locked(const bdNodeId *id, RsPeerId&pid)
{
#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::lookupRsId_locked() for : ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif

	std::map<bdNodeId, RsPeerId>::iterator nit;
	nit = mTransToRsId.find(*id);
	if (nit == mTransToRsId.end())
	{
#ifdef DEBUG_BITDHT_TRANSLATE
		std::cerr << "p3BitDht::lookupRsId_locked() failed";
		std::cerr << std::endl;
#endif

		return 0;
	}
	pid = nit->second;


#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::lookupRsId_locked() Found Matching RsId: " << pid;
	std::cerr << std::endl;
#endif

	return 1;
}

int p3BitDht::storeTranslation_locked(const RsPeerId& pid)
{
#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::storeTranslation_locked(" << pid << ")";
	std::cerr << std::endl;
#endif

	bdNodeId nid;
	calculateNodeId(pid, &nid);

#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::storeTranslation_locked() Converts to NodeId: ";
	bdStdPrintNodeId(std::cerr, &(nid));
	std::cerr << std::endl;
#endif

	mTransToNodeId[pid] = nid;
	mTransToRsId[nid] = pid;

#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::storeTranslation_locked() Success";
	std::cerr << std::endl;
#endif

	return 1;
}

int p3BitDht::removeTranslation_locked(const RsPeerId& pid)
{

#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::removeTranslation_locked(" << pid << ")";
	std::cerr << std::endl;
#endif

	std::map<RsPeerId, bdNodeId>::iterator it = mTransToNodeId.find(pid);
	it = mTransToNodeId.find(pid);
	if (it == mTransToNodeId.end())
	{
		std::cerr << "p3BitDht::removeTranslation_locked() ERROR MISSING TransToNodeId";
		std::cerr << std::endl;
		/* missing */
		return 0;
	}

	bdNodeId nid = it->second;

#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::removeTranslation_locked() Found Translation: NodeId: ";
	bdStdPrintNodeId(std::cerr, &(nid));
	std::cerr << std::endl;
#endif


	std::map<bdNodeId, RsPeerId>::iterator nit;
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

#ifdef DEBUG_BITDHT_TRANSLATE
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
int p3BitDht::calculateNodeId(const RsPeerId& pid, bdNodeId *id)
{
	/* generate node id from pid */
#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << "p3BitDht::calculateNodeId() " << pid;
#endif

	/* use a hash to make it impossible to reverse */

        uint8_t sha_hash[SHA_DIGEST_LENGTH];
        memset(sha_hash,0,SHA_DIGEST_LENGTH*sizeof(uint8_t)) ;
        SHA_CTX *sha_ctx = new SHA_CTX;
        SHA1_Init(sha_ctx);

        SHA1_Update(sha_ctx, rs_dht_version_data, RS_DHT_VERSION_LEN);
        SHA1_Update(sha_ctx, pid.toByteArray(), RsPeerId::SIZE_IN_BYTES);
        SHA1_Final(sha_hash, sha_ctx);

        for(int i = 0; i < SHA_DIGEST_LENGTH && (i < BITDHT_KEY_LEN); i++)
        {
		id->data[i] = sha_hash[i];
        }
	delete sha_ctx;

#ifdef DEBUG_BITDHT_TRANSLATE
	std::cerr << " => ";
	bdStdPrintNodeId(std::cerr, id);
	std::cerr << std::endl;
#endif

	return 1;
}


/******************** Conversion Functions **************************/

DhtPeerDetails::DhtPeerDetails()
{
	mDhtState = RsDhtPeerDht::NOT_ACTIVE;

	mDhtState = RsDhtPeerDht::SEARCHING;
	mDhtUpdateTS = time(NULL);
		
	mPeerReqStatusMsg = "Just Added";
	mPeerReqState = RsDhtPeerRequest::STOPPED;
	mPeerReqMode = 0;
	//mPeerReqProxyId;
	mPeerReqTS = time(NULL);

 	mExclusiveProxyLock = false;
		
	mPeerCbMsg = "No CB Yet";
	mPeerCbMode = 0;
	mPeerCbPoint = 0;
	//mPeerCbProxyId = 0;
	//mPeerCbDestId = 0;
	mPeerCbTS = 0;
		
	mPeerConnectState = RsDhtPeerConnectState::DISCONNECTED;
	mPeerConnectMsg = "Disconnected";
	mPeerConnectMode = 0;
	//dpd->mPeerConnectProxyId;
	mPeerConnectPoint = 0;
		
	mPeerConnectUdpTS = 0;
	mPeerConnectTS = 0;
	mPeerConnectClosedTS = 0;
		
	bdsockaddr_clear(&(mPeerConnectAddr));
}

