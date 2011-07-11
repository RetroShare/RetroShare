/*
 * libretroshare/src/pqi: pqipersongrp.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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

#include "pqi/pqipersongrp.h"
#include "pqi/p3linkmgr.h"
#include "util/rsdebug.h"

#include <sstream>
#include <stdio.h>

const int pqipersongrpzone = 354;

#ifdef WINDOWS_SYS
///////////////////////////////////////////////////////////
// hack for too many connections
#include "retroshare/rsinit.h"
static std::list<std::string> waitingIds;
#define MAX_CONNECT_COUNT 5
///////////////////////////////////////////////////////////
#endif

/****
 *#define PGRP_DEBUG 1
 ****/
//#define PGRP_DEBUG 1

/* MUTEX NOTES:
 * Functions like GetRsRawItem() lock itself (pqihandler) and
 * likewise ServiceServer and ConfigMgr mutex themselves.
 * This means the only things we need to worry about are:
 *  pqilistener and when accessing pqihandlers data.
 */

// handle the tunnel services.
int pqipersongrp::tickServiceRecv()
{
        RsRawItem *pqi = NULL;
	int i = 0;
	{
		std::ostringstream out;
		out << "pqipersongrp::tickTunnelServer()";
		pqioutput(PQL_DEBUG_ALL, pqipersongrpzone, out.str());
	}

	//p3ServiceServer::tick();

	while(NULL != (pqi = GetRsRawItem()))
	{
		++i;
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, 
			"pqipersongrp::tickTunnelServer() Incoming TunnelItem");
		incoming(pqi);
	}

	if (0 < i)
	{
		return 1;
	}
	return 0;
}

// handle the tunnel services.
int pqipersongrp::tickServiceSend()
{
        RsRawItem *pqi = NULL;
	int i = 0;
	{
		std::ostringstream out;
		out << "pqipersongrp::tickServiceSend()";
		pqioutput(PQL_DEBUG_ALL, pqipersongrpzone, out.str());
	}

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


	// init
pqipersongrp::pqipersongrp(SecurityPolicy *glob, unsigned long flags)
	:pqihandler(glob), pqil(NULL), config(NULL), initFlags(flags)
{
}


int	pqipersongrp::tick()
{
	/* could limit the ticking of listener / tunnels to 1/sec...
	 * but not to important.
	 */

  { RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
	if (pqil)
	{
		pqil -> tick();
	}
  } /* UNLOCKED */

	int i = 0;

	if (tickServiceSend())
	{
		i = 1;
#ifdef PGRP_DEBUG
                std::cerr << "pqipersongrp::tick() moreToTick from tickServiceSend()" << std::endl;
#endif
	}

	if (pqihandler::tick()) /* does actual Send/Recv */
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

	return i;
}

int	pqipersongrp::status()
{
  { RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
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
		pqil = NULL;
	}
	else
	{
		/* extract details from 
		 */
		struct sockaddr_in laddr = mLinkMgr->getLocalAddress();
		
  		RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
		pqil = createListener(laddr);
	}
	return 1;
}

bool    pqipersongrp::resetListener(struct sockaddr_in &local)
{
        #ifdef PGRP_DEBUG
	std::cerr << "pqipersongrp::resetListener()" << std::endl;
        #endif

	// stop it, 
	// change the address.
	// restart.

 	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

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

/* NOT bothering to protect Config with a mutex.... it is not going to change
 * and has its own internal mutexs.
 */
int	pqipersongrp::setConfig(p3GeneralConfig *cfg)
{
	config = cfg;
	return 1;
}

static const std::string pqih_ftr("PQIH_FTR");

int     pqipersongrp::save_config()
{
	char line[512];
	sprintf(line, "%f %f", getMaxRate(true), getMaxRate(false));
	if (config)
	{
		config -> setSetting(pqih_ftr, std::string(line));
	}
	return 1;
}
	
int     pqipersongrp::load_config()
{
	std::string line;
	if (config)
	{
		line = config -> getSetting(pqih_ftr);
	}

	float mri, mro;

	if (2 == sscanf(line.c_str(), "%f %f", &mri, &mro))
	{
		setMaxRate(true, mri);
		setMaxRate(false, mro);
	}
	else
	{
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone,
			"pqipersongrp::load_config() Loading Default Rates!");

		setMaxRate(true,  500.0);
		setMaxRate(false, 500.0);
	}

	return 1;
}
	

void    pqipersongrp::statusChange(const std::list<pqipeer> &plist)
{

	/* iterate through, only worry about the friends */
	std::list<pqipeer>::const_iterator it;
	for(it = plist.begin(); it != plist.end(); it++)
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
		waitingIds.clear();
		return;
	}	

	/* check for active connections and start waiting id's */
	long connect_count = 0;

	{
		RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

		/* get address from p3connmgr */
		if (!mLinkMgr) {
			return;
		}

		/* check for active connections and start waiting id's */
		std::list<std::string> peers;
		mLinkMgr->getFriendList(peers);

		/* count connection attempts */
		std::list<std::string>::iterator peer;
		for (peer = peers.begin(); peer != peers.end(); peer++) {
			peerConnectState state;
			if (mLinkMgr->getFriendNetStatus(*peer, state) == false) {
				continue;
			}

			if (state.inConnAttempt) {
				connect_count++;
				if (connect_count >= MAX_CONNECT_COUNT) {
#ifdef PGRP_DEBUG
					std::cerr << "pqipersongrp::connectPeer() Too many connections due to windows limitations. There are " << waitingIds.size() << " waiting connections." << std::endl;
#endif
					return;
				}
			}
		}

	} /* UNLOCKED */

#ifdef PGRP_DEBUG
	std::cerr << "pqipersongrp::connectPeer() There are " << connect_count << " connection attempts and " << waitingIds.size() << " waiting connections. Can start " << (MAX_CONNECT_COUNT - connect_count) << " connection attempts." << std::endl;
#endif

	/* there is no need for a mutex for waitingIds */

	/* start some waiting id's */
	for (int i = connect_count; i < MAX_CONNECT_COUNT; i++) {
		if (waitingIds.empty()) {
			break;
		}
		std::string waitingId = waitingIds.front();
		waitingIds.pop_front();
		connectPeer(waitingId, true);
	}
}
///////////////////////////////////////////////////////////
#endif


int     pqipersongrp::addPeer(std::string id)
{
	{
		std::ostringstream out;
		out << "pqipersongrp::addPeer() PeerId: " << id;
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, out.str());
	}

	std::cerr << "pqipersongrp::addPeer() id: " << id;
	std::cerr << std::endl;
#ifdef PGRP_DEBUG
#endif

	SearchModule *sm = NULL;
  { RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
	std::map<std::string, SearchModule *>::iterator it;
	it = mods.find(id);
	if (it != mods.end())
	{
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, 
			"pqipersongrp::addPeer() Peer already in Use!");

		std::cerr << " pqipersongrp::addPeer() ERROR Peer already in use! id: " << id;
		std::cerr << std::endl;

		return -1;
	}

	pqiperson *pqip = createPerson(id, pqil);

	// attach to pqihandler
	sm = new SearchModule();
	sm -> peerid = id;
	sm -> pqi = pqip;
	sm -> sp = secpolicy_create();

	// reset it to start it working.
	pqip -> reset();
	pqip -> listen();
  } /* UNLOCKED */

	return AddSearchModule(sm);
}


int     pqipersongrp::removePeer(std::string id)
{
	std::map<std::string, SearchModule *>::iterator it;

#ifdef PGRP_DEBUG
#endif
	std::cerr << "pqipersongrp::removePeer() id: " << id;
	std::cerr << std::endl;

  	RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/

	it = mods.find(id);
	if (it != mods.end())
	{
		SearchModule *mod = it->second;
		// Don't duplicate remove!!!
		//RemoveSearchModule(mod);
		secpolicy_delete(mod -> sp);
		pqiperson *p = (pqiperson *) mod -> pqi;
		p -> stoplistening();
		p -> reset();
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

int pqipersongrp::tagHeartbeatRecvd(std::string id)
{
        std::map<std::string, SearchModule *>::iterator it;

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






int     pqipersongrp::connectPeer(std::string id
#ifdef WINDOWS_SYS
///////////////////////////////////////////////////////////
// hack for too many connections
								  , bool bConnect /*= false*/
///////////////////////////////////////////////////////////
#endif
								  )
{
	/* get status from p3connectMgr */
#ifdef PGRP_DEBUG
	std::cerr << " pqipersongrp::connectPeer() id: " << id << " does nothing yet! ";
	std::cerr << std::endl;
#endif

  { RsStackMutex stack(coreMtx); /**************** LOCKED MUTEX ****************/
        if (id == mLinkMgr->getOwnId()) {
            #ifdef PGRP_DEBUG
            std::cerr << "pqipersongrp::connectPeer() Failed, connecting to own id." << std::endl;
            #endif
            return 0;
        }
        std::map<std::string, SearchModule *>::iterator it;
	it = mods.find(id);
	if (it == mods.end())
	{
		return 0;
	}
	/* get the connect attempt details from the p3connmgr... */
	SearchModule *mod = it->second;
	pqiperson *p = (pqiperson *) mod -> pqi;


	/* get address from p3connmgr */
	if (!mLinkMgr)
		return 0;

#ifdef WINDOWS_SYS
	///////////////////////////////////////////////////////////
	// hack for too many connections

	if (RsInit::isWindowsXP()) {
		/* the problem only exist in Windows XP */
		if (bConnect == false) {
			/* check for id is waiting */
			if (std::find(waitingIds.begin(), waitingIds.end(), id) != waitingIds.end()) {
				/* id is waiting for a connection */
				return 0;
			}

			/* add id to waiting */
			waitingIds.push_back(id);

			/* wait for call to connectPeer with empty id */
			return 0;
		}

		/* remove id from waiting */
		waitingIds.remove(id);
	}

	///////////////////////////////////////////////////////////
#endif

	struct sockaddr_in addr;
	uint32_t delay;
	uint32_t period;
	uint32_t timeout;
	uint32_t type;

	if (!mLinkMgr->connectAttempt(id, addr, delay, period, type))
	{
#ifdef PGRP_DEBUG
		std::cerr << " pqipersongrp::connectPeer() No Net Address";
		std::cerr << std::endl;
#endif
		return 0;
	}

#ifdef PGRP_DEBUG
	std::cerr << " pqipersongrp::connectPeer() connectAttempt data id: " << id;
	std::cerr << " addr: " << rs_inet_ntoa(addr.sin_addr) << ":" << ntohs(addr.sin_port);
	std::cerr << " delay: " << delay;
	std::cerr << " period: " << period;
	std::cerr << " type: " << type;
	std::cerr << std::endl;
#endif


	uint32_t ptype;
	if (type & RS_NET_CONN_TCP_ALL)
	{
		ptype = PQI_CONNECT_TCP;
		timeout = RS_TCP_STD_TIMEOUT_PERIOD; 
#ifdef PGRP_DEBUG
		std::cerr << " pqipersongrp::connectPeer() connecting with TCP: Timeout :" << timeout;
		std::cerr << std::endl;
#endif
	}
	else if (type & RS_NET_CONN_UDP_ALL)
	{
		ptype = PQI_CONNECT_UDP;
		timeout = period * 2;
#ifdef PGRP_DEBUG
		std::cerr << " pqipersongrp::connectPeer() connecting with UDP: Timeout :" << timeout;
		std::cerr << std::endl;
#endif
	}
	else if (type & RS_NET_CONN_TUNNEL)
	{
		ptype = PQI_CONNECT_TUNNEL;
		timeout = period * 2;
#ifdef PGRP_DEBUG
		std::cerr << " pqipersongrp::connectPeer() connecting with Tunnel: Timeout :" << timeout;
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

	p->connect(ptype, addr, delay, period, timeout);

  } /* UNLOCKED */


	/* */
	return 1;
}

bool    pqipersongrp::notifyConnect(std::string id, uint32_t ptype, bool success, struct sockaddr_in raddr)
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
	else if (ptype == PQI_CONNECT_TUNNEL)
	{
		type = RS_NET_CONN_TUNNEL;
	}

	
	if (mLinkMgr)
		mLinkMgr->connectResult(id, success, type, raddr);
	
	return (NULL != mLinkMgr);
}

/******************************** DUMMY Specific features ***************************/

#include "pqi/pqibin.h"

pqilistener * pqipersongrpDummy::createListener(struct sockaddr_in laddr)
{
	pqilistener *listener = new pqilistener();
	return listener;
}


pqiperson * pqipersongrpDummy::createPerson(std::string id, pqilistener *listener)
{
	{
		std::ostringstream out;
		out << "pqipersongrpDummy::createPerson() PeerId: " << id;
		pqioutput(PQL_DEBUG_BASIC, pqipersongrpzone, out.str());
	}

	pqiperson *pqip = new pqiperson(id, this);

	// TCP
	NetBinDummy *d1 = new NetBinDummy(pqip, id, PQI_CONNECT_TCP);

	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsFileItemSerialiser());
	rss->addSerialType(new RsCacheItemSerialiser());
	rss->addSerialType(new RsServiceSerialiser());

	pqiconnect *pqic = new pqiconnect(rss, d1);

	pqip -> addChildInterface(PQI_CONNECT_TCP, pqic);

	// UDP.
	NetBinDummy *d2 = new NetBinDummy(pqip, id, PQI_CONNECT_UDP);

	RsSerialiser *rss2 = new RsSerialiser();
	rss2->addSerialType(new RsFileItemSerialiser());
	rss2->addSerialType(new RsCacheItemSerialiser());
	rss2->addSerialType(new RsServiceSerialiser());

	pqiconnect *pqic2 	= new pqiconnect(rss2, d2);

	pqip -> addChildInterface(PQI_CONNECT_UDP, pqic2);

	return pqip;
}

/******************************** DUMMY Specific features ***************************/

