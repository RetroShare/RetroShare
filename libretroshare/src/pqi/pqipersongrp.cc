/*******************************************************************************
 * libretroshare/src/pqi: pqipersongrp.cc                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2008  Robert Fernie <retroshare@lunamutt.com>            *
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
#include "pqi/pqipersongrp.h"
#include "pqi/p3linkmgr.h"
#include "util/rsdebug.h"
#include "util/rsprint.h"
#include "serialiser/rsserializer.h"

#include <stdio.h>

static struct RsLog::logInfo pqipersongrpzoneInfo = {RsLog::Default, "pqipersongrp"};
#define pqipersongrpzone &pqipersongrpzoneInfo

#ifdef WINDOWS_SYS
///////////////////////////////////////////////////////////
// hack for too many connections
#include "retroshare/rsinit.h"
static std::list<RsPeerId> waitingIds;
#define MAX_CONNECT_COUNT 3
///////////////////////////////////////////////////////////
#endif

/****
 *#define PGRP_DEBUG 1
 *#define PGRP_DEBUG_LOG 1
 ****/

#define DEFAULT_DOWNLOAD_KB_RATE	(200.0)
#define DEFAULT_UPLOAD_KB_RATE		(50.0)

/* MUTEX NOTES:
 * Functions like GetRsRawItem() lock itself (pqihandler) and
 * likewise ServiceServer and ConfigMgr mutex themselves.
 * This means the only things we need to worry about are:
 *  pqilistener and when accessing pqihandlers data.
 */


	// New speedy recv.
bool pqipersongrp::RecvRsRawItem(RsRawItem *item)
{
	std::cerr << "pqipersongrp::RecvRsRawItem()";
	std::cerr << std::endl;

	p3ServiceServer::recvItem(item);

	return true;
}



#ifdef TO_BE_REMOVED
// handle the tunnel services.
int pqipersongrp::tickServiceRecv()
{
	RsRawItem *pqi = NULL;
	int i = 0;

	pqioutput(PQL_DEBUG_ALL, pqipersongrpzone, "pqipersongrp::tickServiceRecv()");

	//p3ServiceServer::tick();

	while(NULL != (pqi = GetRsRawItem()))
	{
		static int ntimes=0 ;
		if(++ntimes < 20)
		{
		std::cerr << "pqipersongrp::tickServiceRecv() GetRsRawItem()";
        std::cerr << " should never happen anymore! item data=" << RsUtil::BinToHex((char*)pqi->getRawData(),pqi->getRawLength()) ;
        std::cerr << std::endl;
		}

		++i;
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, 
			"pqipersongrp::tickServiceRecv() Incoming TunnelItem");
		recvItem(pqi);
	}

	if (0 < i)
	{
		return 1;
	}
	return 0;
}
#endif

// handle the tunnel services.

// Improvements:
// This function is no longer necessary, and data is pushed directly to pqihandler.

#ifdef TO_BE_REMOVED
int pqipersongrp::tickServiceSend()
{
        RsRawItem *pqi = NULL;
	int i = 0;

	pqioutput(PQL_DEBUG_ALL, pqipersongrpzone, "pqipersongrp::tickServiceSend()");

	p3ServiceServer::tick();

	while(NULL != (pqi = outgoing())) /* outgoing has own locking */
	{
		++i;
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, 
			"pqipersongrp::tickTunnelServer() OutGoing RsItem");

		SendRsRawItem(pqi); /* Locked by pqihandler */
	}
	if (0 < i)
	{
		return 1;
	}
	return 0;
}

#endif


	// init
pqipersongrp::pqipersongrp(p3ServiceControl *ctrl, unsigned long flags)
    :pqihandler(), p3ServiceServer(this, ctrl), pqil(NULL), pqilMtx("pqipersongrp"), initFlags(flags)
{
}


int	pqipersongrp::tick()
{
	/* could limit the ticking of listener / tunnels to 1/sec...
	 * but not to important.
	 */

	{ 
		RsStackMutex stack(pqilMtx); /******* LOCKED MUTEX **********/
		if (pqil)
		{
			pqil -> tick();
		}
	} /* UNLOCKED */

	int i = 0;

#ifdef TO_BE_REMOVED
    if (tickServiceSend())
	{
		i = 1;
#ifdef PGRP_DEBUG
                std::cerr << "pqipersongrp::tick() moreToTick from tickServiceSend()" << std::endl;
#endif
	}
	if (pqihandler::tick()) /* does Send/Recv */
	{
		i = 1;
#ifdef PGRP_DEBUG
                std::cerr << "pqipersongrp::tick() moreToTick from pqihandler::tick()" << std::endl;
#endif
	}
	if (tickServiceRecv())
	{
		i = 1;
#ifdef PGRP_DEBUG
                std::cerr << "pqipersongrp::tick() moreToTick from tickServiceRecv()" << std::endl;
#endif
    }
#endif

	if(pqihandler::tick())
		i=1;

    p3ServiceServer::tick();

	return i;
}

int	pqipersongrp::status()
{
	{ 
		RsStackMutex stack(pqilMtx); /******* LOCKED MUTEX **********/
		if (pqil)
		{
			pqil -> status();
		}
	} /* UNLOCKED */

	return pqihandler::status();
}


/* Initialise pqilistener */
int	pqipersongrp::init_listener()
{
	/* extract our information from the p3ConnectMgr */
	if (initFlags & PQIPERSON_NO_LISTENER)
	{
		RsStackMutex stack(pqilMtx); /******* LOCKED MUTEX **********/
		pqil = NULL;
	}
	else
	{
		/* extract details from 
		 */
		struct sockaddr_storage laddr;
		mLinkMgr->getLocalAddress(laddr);
		
		RsStackMutex stack(pqilMtx); /******* LOCKED MUTEX **********/
		pqil = locked_createListener(laddr);
	}
	return 1;
}

bool    pqipersongrp::resetListener(const struct sockaddr_storage &local)
{
        #ifdef PGRP_DEBUG
	std::cerr << "pqipersongrp::resetListener()" << std::endl;
        #endif

	// stop it, 
	// change the address.
	// restart.

	RsStackMutex stack(pqilMtx); /******* LOCKED MUTEX **********/

	if (pqil != NULL)
	{
                #ifdef PGRP_DEBUG
		std::cerr << "pqipersongrp::resetListener() haveListener" << std::endl;
                #endif

		pqil -> resetlisten();
		pqil -> setListenAddr(local);
		pqil -> setuplisten();

                #ifdef PGRP_DEBUG
		std::cerr << "pqipersongrp::resetListener() done!" << std::endl;
                #endif

	}
	return 1;
}

void    pqipersongrp::statusChange(const std::list<pqipeer> &plist)
{

	/* iterate through, only worry about the friends */
	std::list<pqipeer>::const_iterator it;
	for(it = plist.begin(); it != plist.end(); ++it)
	{
	  if (it->state & RS_PEER_S_FRIEND)
	  {
	  	/* now handle add/remove */
		if ((it->actions & RS_PEER_NEW) 
		   || (it->actions & RS_PEER_MOVED))
		{
			addPeer(it->id);
		}

		if (it->actions & RS_PEER_CONNECT_REQ)
		{
			connectPeer(it->id);
		}
	  }
	  else /* Not Friend */
	  {
		if (it->actions & RS_PEER_MOVED)
		{
			removePeer(it->id);
		}
	  }
	}
}

#ifdef WINDOWS_SYS
///////////////////////////////////////////////////////////
// hack for too many connections
void pqipersongrp::statusChanged()
{
#warning "Windows connection limited hacked together - please fix"

	if (RsInit::isWindowsXP() == false) {
		/* the problem only exist in Windows XP */
		RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
		waitingIds.clear();
		return;
	}	

	{
		RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
		if (waitingIds.empty()) 
		{
			/* nothing to do */
			return;
		}
	}

	/* check for active connections and start waiting id's */
	long connect_count = 0;
	std::list<RsPeerId> toConnect;

	{
		RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

		/* get address from p3connmgr */
		if (!mLinkMgr) {
			return;
		}

		/* check for active connections and start waiting id's */
		std::list<RsPeerId> peers;
		mLinkMgr->getFriendList(peers);

		/* count connection attempts */
		std::list<RsPeerId>::iterator peer;
		for (peer = peers.begin(); peer != peers.end(); ++peer) {
			peerConnectState state;
			if (mLinkMgr->getFriendNetStatus(*peer, state) == false) {
				continue;
			}

			if (state.inConnAttempt) {
				connect_count++;
				if (connect_count >= MAX_CONNECT_COUNT) {
#ifdef PGRP_DEBUG
					std::cerr << "pqipersongrp::statusChanged() Too many connections due to windows limitations. There are " << waitingIds.size() << " waiting connections." << std::endl;
#endif
					return;
				}
			}
		}

#ifdef PGRP_DEBUG
		std::cerr << "pqipersongrp::statusChanged() There are ";
		std::cerr << connect_count << " connection attempts and " << waitingIds.size();
		std::cerr << " waiting connections. Can start ";
		std::cerr << (MAX_CONNECT_COUNT - connect_count) << " connection attempts.";
		std::cerr << std::endl;
#endif

		/* start some waiting id's */
		for (int i = connect_count; i < MAX_CONNECT_COUNT; i++) 
		{
			if (waitingIds.empty()) {
				break;
			}
			RsPeerId waitingId = waitingIds.front();
			waitingIds.pop_front();
	
#ifdef PGRP_DEBUG
			std::cerr << " pqipersongrp::statusChanged() id: " << waitingId << " connect peer";
			std::cerr << std::endl;
#endif

			toConnect.push_back(waitingId);
		}
	} /* UNLOCKED */

	std::list<RsPeerId>::iterator cit;
	for(cit = toConnect.begin(); cit != toConnect.end(); ++cit)
	{
		connectPeer(*cit, true);
	}
}
///////////////////////////////////////////////////////////
#endif

bool pqipersongrp::getCryptoParams(const RsPeerId& id,RsPeerCryptoParams& params)
{
	RsStackMutex stack(coreMtx); /******* LOCKED MUTEX **********/

	std::map<RsPeerId, SearchModule *>::iterator it = mods.find(id) ;

	if(it == mods.end())
		return false ;

	return it->second->pqi->getCryptoParams(params) ;

	//return locked_getCryptoParams(id,params) ;
}

int     pqipersongrp::addPeer(const RsPeerId& id)
{
	pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, "pqipersongrp::addPeer() PeerId: " + id.toStdString());

	std::cerr << "pqipersongrp::addPeer() id: " << id;
	std::cerr << std::endl;
#ifdef PGRP_DEBUG
#endif

	SearchModule *sm = NULL;

	{ 
		// The Mutex is required here as pqiListener is not thread-safe.
		RsStackMutex stack(pqilMtx); /******* LOCKED MUTEX **********/
		pqiperson *pqip = locked_createPerson(id, pqil);
	
		// attach to pqihandler
		sm = new SearchModule();
		sm -> peerid = id;
		sm -> pqi = pqip;
	
		// reset it to start it working.
#ifdef PGRP_DEBUG_LOG
		pqioutput(PQL_WARNING, pqipersongrpzone, "pqipersongrp::addPeer() => reset() called to initialise new person");
#endif
		pqip -> reset();
		pqip -> listen();

	} /* UNLOCKED */

	return AddSearchModule(sm);
}


int     pqipersongrp::removePeer(const RsPeerId& id)
{
	std::map<RsPeerId, SearchModule *>::iterator it;

#ifdef PGRP_DEBUG
	std::cerr << "pqipersongrp::removePeer() id: " << id;
	std::cerr << std::endl;
#endif

  	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	it = mods.find(id);
	if (it != mods.end())
	{
		SearchModule *mod = it->second;
		pqiperson *p = (pqiperson *) mod -> pqi;
		p -> stoplistening();
		pqioutput(PQL_WARNING, pqipersongrpzone, "pqipersongrp::removePeer() => reset() called before deleting person");
		p -> reset();
		p -> fullstopthreads();
		delete p;
		mods.erase(it);
	}
	else
	{
		std::cerr << " pqipersongrp::removePeer() ERROR doesn't exist! id: " << id;
		std::cerr << std::endl;
	}
	return 1;
}

int pqipersongrp::tagHeartbeatRecvd(const RsPeerId& id)
{
        std::map<RsPeerId, SearchModule *>::iterator it;

#ifdef PGRP_DEBUG
        std::cerr << " pqipersongrp::tagHeartbeatRecvd() id: " << id;
        std::cerr << std::endl;
#endif

        RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

        it = mods.find(id);
        if (it != mods.end())
        {
                SearchModule *mod = it->second;
                pqiperson *p = (pqiperson *) mod -> pqi;
		p->receiveHeartbeat();
		return 1;
        }
        return 0;
}






int     pqipersongrp::connectPeer(const RsPeerId& id
#ifdef WINDOWS_SYS
///////////////////////////////////////////////////////////
// hack for too many connections
								  , bool bConnect /*= false*/
///////////////////////////////////////////////////////////
#endif
								  )
{
	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	if (!mLinkMgr)
		return 0;

	if (id == mLinkMgr->getOwnId())
	{
#ifdef PGRP_DEBUG
		std::cerr << "pqipersongrp::connectPeer() ERROR Failed, connecting to own id." << std::endl;
#endif
		return 0;
	}
	std::map<RsPeerId, SearchModule *>::iterator it;
	it = mods.find(id);
	if (it == mods.end())
	{
		return 0;
	}
	/* get the connect attempt details from the p3connmgr... */
	SearchModule *mod = it->second;
	pqiperson *p = (pqiperson *) mod -> pqi;

#ifdef WINDOWS_SYS
	///////////////////////////////////////////////////////////
	// hack for too many connections

	if (RsInit::isWindowsXP()) {
		/* the problem only exist in Windows XP */
		if (bConnect == false) {
			/* check for id is waiting */
			if (std::find(waitingIds.begin(), waitingIds.end(), id) != waitingIds.end()) {
				/* id is waiting for a connection */
#ifdef PGRP_DEBUG
				std::cerr << " pqipersongrp::connectPeer() id: " << id << " is already waiting ";
				std::cerr << std::endl;
#endif
				return 0;
			}

			/* check for connection type of the next connect attempt */
			peerConnectState state;
			if (mLinkMgr->getFriendNetStatus(id, state) == false) {
#ifdef PGRP_DEBUG
				std::cerr << " pqipersongrp::connectPeer() id: " << id << " No friend net status";
				std::cerr << std::endl;
#endif
				return 0;
			}
			if (state.connAddrs.size() < 1) {
#ifdef PGRP_DEBUG
				std::cerr << " pqipersongrp::connectPeer() id: " << id << " No existing connect addresses";
				std::cerr << std::endl;
#endif
				return 0;
			}
			const peerConnectAddress &currentConnAddrAttempt = state.connAddrs.front();
			if (currentConnAddrAttempt.type & RS_NET_CONN_TCP_ALL) {
#ifdef PGRP_DEBUG
				std::cerr << " pqipersongrp::connectPeer() id: " << id << " added to the waiting list";
				std::cerr << std::endl;
#endif
				/* TCP connect, add id to waiting */
				waitingIds.push_back(id);

				/* wait for call to connectPeer with empty id */
				return 0;
			}

			/* try all other types of connect directly */

#ifdef PGRP_DEBUG
			std::cerr << " pqipersongrp::connectPeer() id: " << id << " connect directly without wait";
			std::cerr << std::endl;
#endif
		}

		/* remove id from waiting */
		waitingIds.remove(id);
	}

	///////////////////////////////////////////////////////////
#endif

	struct sockaddr_storage addr;
	uint32_t delay;
	uint32_t period;
	uint32_t timeout;
	uint32_t type;
	uint32_t flags = 0 ;

	struct sockaddr_storage proxyaddr;
	struct sockaddr_storage srcaddr;
	uint32_t bandwidth;
	std::string domain_addr;
	uint16_t domain_port;
	  	  
	if (!mLinkMgr->connectAttempt(id, addr, proxyaddr, srcaddr, delay, period, type, flags, bandwidth, domain_addr, domain_port))
	{
#ifdef PGRP_DEBUG
		std::cerr << " pqipersongrp::connectPeer() No Net Address";
		std::cerr << std::endl;
#endif
		return 0;
	}

#ifdef PGRP_DEBUG
	std::cerr << " pqipersongrp::connectPeer() connectAttempt data id: " << id;
	std::cerr << " addr: " << sockaddr_storage_tostring(addr);
	std::cerr << " delay: " << delay;
	std::cerr << " period: " << period;
	std::cerr << " type: " << type;
	std::cerr << " flags: " << flags;
	std::cerr << " domain_addr: " << domain_addr;
	std::cerr << " domain_port: " << domain_port;
	std::cerr << std::endl;
#endif


	uint32_t ptype;
	if (type & RS_NET_CONN_TCP_ALL)
	{
		switch (type) {
		case RS_NET_CONN_TCP_HIDDEN_TOR:
			ptype = PQI_CONNECT_HIDDEN_TOR_TCP;
			timeout = RS_TCP_HIDDEN_TIMEOUT_PERIOD;
			break;
		case RS_NET_CONN_TCP_HIDDEN_I2P:
			ptype = PQI_CONNECT_HIDDEN_I2P_TCP;
			timeout = RS_TCP_HIDDEN_TIMEOUT_PERIOD;
			break;
		default:
			ptype = PQI_CONNECT_TCP;
			timeout = RS_TCP_STD_TIMEOUT_PERIOD;
			break;
		}
#ifdef PGRP_DEBUG
		std::cerr << " pqipersongrp::connectPeer() connecting with TCP: Timeout :" << timeout;
		std::cerr << std::endl;
#endif
	}
	else if (type & RS_NET_CONN_UDP_ALL)
	{
		ptype = PQI_CONNECT_UDP;
		timeout = period + RS_UDP_STD_TIMEOUT_PERIOD; // Split of UNCERTAINTY + TIME FOR TTL to RISE to Connection.
#ifdef PGRP_DEBUG
		std::cerr << " pqipersongrp::connectPeer() connecting with UDP: Timeout :" << timeout;
		std::cerr << std::endl;
#endif
	}
	else
	{
#ifdef PGRP_DEBUG
		std::cerr << " pqipersongrp::connectPeer() Ignoring Unknown Type:" << type;
		std::cerr << std::endl;
#endif
		return 0;
	}

	p->connect(ptype, addr, proxyaddr, srcaddr, delay, period, timeout, flags, bandwidth, domain_addr, domain_port);

	return 1;
}

bool    pqipersongrp::notifyConnect(const RsPeerId& id, uint32_t ptype, bool success, bool isIncomingConnection, const struct sockaddr_storage &raddr)
{
	uint32_t type = 0;
	if (ptype == PQI_CONNECT_TCP)
	{
		type = RS_NET_CONN_TCP_ALL;
	}
	else if (ptype == PQI_CONNECT_UDP)
	{
		type = RS_NET_CONN_UDP_ALL;
	}
	
	if (mLinkMgr)
		mLinkMgr->connectResult(id, success, isIncomingConnection, type, raddr);
	
	return (NULL != mLinkMgr);
}

/******************************** DUMMY Specific features ***************************/

#include "pqi/pqibin.h"

pqilistener * pqipersongrpDummy::locked_createListener(const struct sockaddr_storage & /*laddr*/)
{
	pqilistener *listener = new pqilistener();
	return listener;
}


pqiperson * pqipersongrpDummy::locked_createPerson(const RsPeerId& id, pqilistener * /*listener*/)
{
	pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, "pqipersongrpDummy::createPerson() PeerId: " + id.toStdString());

	pqiperson *pqip = new pqiperson(id, this);

	// TCP
	NetBinDummy *d1 = new NetBinDummy(pqip, id, PQI_CONNECT_TCP);

	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsRawSerialiser());

	pqiconnect *pqic = new pqiconnect(pqip, rss, d1);

	pqip -> addChildInterface(PQI_CONNECT_TCP, pqic);

	// UDP.
	NetBinDummy *d2 = new NetBinDummy(pqip, id, PQI_CONNECT_UDP);

	RsSerialiser *rss2 = new RsSerialiser();
	rss2->addSerialType(new RsRawSerialiser());

	pqiconnect *pqic2 	= new pqiconnect(pqip, rss2, d2);

	pqip -> addChildInterface(PQI_CONNECT_UDP, pqic2);

	return pqip;
}

/******************************** DUMMY Specific features ***************************/

