/*******************************************************************************
 * libretroshare/src/dht: p3bitdht.cc                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2009-2010 by Robert Fernie <drbob@lunamutt.com>                   *
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

#include <list>

#include "dht/p3bitdht.h"

#include "bitdht/bdstddht.h"

#include "tcponudp/udprelay.h"
#ifdef RS_USE_DHT_STUNNER
#include "tcponudp/udpstunner.h"
#endif // RS_USE_DHT_STUNNER

#include "retroshare/rsbanlist.h"

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
                        uint32_t mode, uint32_t point, uint32_t param, uint32_t cbtype, uint32_t errcode)
{ 
	return mParent->ConnectCallback(srcId, proxyId, destId, mode, point, param, cbtype, errcode);
}  

virtual int dhtInfoCallback(const bdId *id, uint32_t type, uint32_t flags, std::string info)
{ 
	return mParent->InfoCallback(id, type, flags, info);
}  

	virtual int dhtIsBannedCallback(const sockaddr_in *addr, bool *isBanned)
	{
		// check whether ip filtering is enabled
		// if not return 0 to signal that no filter is available
		if(!rsBanList->ipFilteringEnabled())
			return 0;

		// now check the filter
		if(rsBanList->isAddressAccepted(
		            *reinterpret_cast<const sockaddr_storage*>(addr),
		            RSBANLIST_CHECKING_FLAGS_BLACKLIST ))
		{
			*isBanned = false;
		} else {
#ifdef DEBUG_BITDHT
			std::cerr << "p3BitDht dhtIsBannedCallback: peer is banned " << sockaddr_storage_tostring(*(const sockaddr_storage*)addr) << std::endl;
#endif
			*isBanned = true;
		}

		// return 1 to signal that a filter is available
		return 1;
	}

	private:

	p3BitDht *mParent;
};


p3BitDht::p3BitDht(const RsPeerId& id, pqiConnectCb *cb, p3NetMgr *nm, 
            UdpStack *udpstack, std::string bootstrapfile,const std::string& filteredipfile)
	:p3Config(), pqiNetAssistConnect(id, cb), mNetMgr(nm), dhtMtx("p3BitDht")
{
#ifdef RS_USE_DHT_STUNNER
	mDhtStunner = NULL;
	mProxyStunner = NULL;
#endif
	mRelay = NULL;

        mPeerSharer = NULL;

	mRelayHandler = NULL;

	std::string dhtVersion = "RS51"; // should come from elsewhere!
        mOwnRsId = id;

	mMinuteTS = 0; 

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
	storeTranslation_locked(id);
	lookupNodeId_locked(id, &mOwnDhtId);


#ifdef DEBUG_BITDHT
	std::cerr << "Own NodeId: ";
	bdStdPrintNodeId(std::cerr, &mOwnDhtId);
	std::cerr << std::endl;
#endif

	/* standard dht behaviour */
	//bdDhtFunctions *stdfns = new bdStdDht();
	mDhtFns = new bdModDht();

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht() startup ... creating UdpBitDht";
	std::cerr << std::endl;
#endif

	/* create dht */
    mUdpBitDht = new UdpBitDht(udpstack, &mOwnDhtId, dhtVersion, bootstrapfile, filteredipfile,mDhtFns);
	udpstack->addReceiver(mUdpBitDht);

	/* setup callback to here */
	p3BdCallback *bdcb = new p3BdCallback(this);
	mUdpBitDht->addCallback(bdcb);

#if 0
	/* enable all modes */
	/* Switched to only Proxy Mode - as Direct Connections can be unreliable - as they share the UDP with the DHT....
	 * We'll get these working properly and then if necessary get Direct further tested.
	 */
	mUdpBitDht->ConnectionOptions(
			// BITDHT_CONNECT_MODE_DIRECT | BITDHT_CONNECT_MODE_PROXY | BITDHT_CONNECT_MODE_RELAY,
			//BITDHT_CONNECT_MODE_DIRECT | BITDHT_CONNECT_MODE_PROXY,
			BITDHT_CONNECT_MODE_PROXY,
                        BITDHT_CONNECT_OPTION_AUTOPROXY);

#endif

	setupRelayDefaults();

	clearDataRates();
}

p3BitDht::~p3BitDht()
{
	//udpstack->removeReceiver(mUdpBitDht);
	delete mUdpBitDht;
}


bool 	p3BitDht::getOwnDhtId(std::string &ownDhtId)
{
        bdStdPrintNodeId(ownDhtId, &(mOwnDhtId), false);
	return true;
}

#ifdef RS_USE_DHT_STUNNER
void    p3BitDht::setupConnectBits(UdpStunner *dhtStunner, UdpStunner *proxyStunner, UdpRelayReceiver  *relay)
{
	mDhtStunner = dhtStunner;
	mProxyStunner = proxyStunner;
	mRelay = relay;
}
#else // RS_USE_DHT_STUNNER
void    p3BitDht::setupConnectBits(UdpRelayReceiver  *relay)
{
	mRelay = relay;
}
#endif //RS_USE_DHT_STUNNER

void    p3BitDht::setupPeerSharer(pqiNetAssistPeerShare *sharer)
{
	mPeerSharer = sharer;
}

	/* Tweak the DHT Parameters */
void 	p3BitDht::modifyNodesPerBucket(uint16_t count)
{
	bdModDht *modFns = (bdModDht *) mDhtFns;
	modFns->setNodesPerBucket(count);
}

/* Support for Outsourced Relay Handling */

void    p3BitDht::installRelayHandler(p3BitDhtRelayHandler *handler)
{
	/* The Handler is mutex protected, as its installation can occur when the dht is already running */
	RsStackMutex stack(dhtMtx);    /********* LOCKED *********/

	mRelayHandler = handler;
}

UdpRelayReceiver *p3BitDht::getRelayReceiver()
{
	return mRelay;
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

#if 0
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

#endif


	/* extract current peer status */
bool 	p3BitDht::getPeerStatus(const RsPeerId& id, 
				struct sockaddr_storage &/*laddr*/, struct sockaddr_storage &/*raddr*/,
				uint32_t &/*type*/, uint32_t &/*mode*/)
{
	/* remove unused parameter warnings */
	(void) id;

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::getPeerStatus(" << id << ")";
	std::cerr << std::endl;
#endif

	return false;
}

bool 	p3BitDht::getExternalInterface(struct sockaddr_storage &/*raddr*/,
					uint32_t &/*mode*/)
{

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::getExternalInterface()";
	std::cerr << std::endl;
#endif


    return false;
}

bool p3BitDht::isAddressBanned(const sockaddr_storage &raddr)
{
    if(raddr.ss_family == AF_INET6)	// the DHT does not handle INET6 addresses yet.
        return false ;

    if(raddr.ss_family == AF_INET)
        return mUdpBitDht->isAddressBanned((sockaddr_in&)raddr) ;

    return false ;
}

void p3BitDht::getListOfBannedIps(std::list<RsDhtFilteredPeer>& ips)
{
    std::list<bdFilteredPeer> lst ;

    mUdpBitDht->getListOfBannedIps(lst) ;

    for(std::list<bdFilteredPeer>::const_iterator it(lst.begin());it!=lst.end();++it)
    {
        RsDhtFilteredPeer fp ;
        fp.mAddr = (*it).mAddr ;
        fp.mFilterFlags = (*it).mFilterFlags ;
        fp.mFilterTS = (*it).mFilterTS ;
        fp.mLastSeen = (*it).mLastSeen ;
        ips.push_back(fp) ;
    }
}

bool 	p3BitDht::setAttachMode(bool on)
{

#ifdef DEBUG_BITDHT
	std::cerr << "p3BitDht::setAttachMode(" << on << ")";
	std::cerr << std::endl;
#endif

	return mUdpBitDht->setAttachMode(on);
}




