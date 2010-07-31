/*
 * libretroshare/src/pqi: pqimonitor.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#ifndef PQI_MONITOR_H
#define PQI_MONITOR_H

/**** Rough sketch of a Monitor class 
 * expect it to change significantly
 *
 */

#include <inttypes.h>
#include <string>
#include <list>
#include "pqi/pqiipset.h"

/************** Define Type/Mode/Source ***************/


/* STATE MASK */
const uint32_t RS_PEER_STATE_MASK       = 0x00ff;
const uint32_t RS_PEER_ACTION_MASK      = 0xff00;

/* STATE */
const uint32_t RS_PEER_S_FRIEND		= 0x0001;
const uint32_t RS_PEER_S_ONLINE    	= 0x0002;    /* heard from recently..*/
const uint32_t RS_PEER_S_CONNECTED   	= 0x0004;  
const uint32_t RS_PEER_S_UNREACHABLE    = 0x0008;  

/* ACTIONS */
const uint32_t RS_PEER_NEW              = 0x0001;    /* new Peer */
const uint32_t RS_PEER_ONLINE 		= 0x0002;
const uint32_t RS_PEER_CONNECTED        = 0x0004;
const uint32_t RS_PEER_MOVED            = 0x0008;    /* moved from F->O or O->F */
const uint32_t RS_PEER_DISCONNECTED     = 0x0010;
const uint32_t RS_PEER_CONNECT_REQ      = 0x0020;

/* Stun Status Flags */
//const uint32_t RS_STUN_SRC_DHT		= 0x0001;
//const uint32_t RS_STUN_SRC_PEER 	= 0x0002;
const uint32_t RS_STUN_ONLINE 		= 0x0010;
const uint32_t RS_STUN_FRIEND 		= 0x0020;
const uint32_t RS_STUN_FRIEND_OF_FRIEND	= 0x0040;



#define RS_CONNECT_PASSIVE 	1
#define RS_CONNECT_ACTIVE 	2

#define RS_CB_DHT 		1   /* from dht */
#define RS_CB_DISC 		2   /* from peers */
#define RS_CB_PERSON 		3   /* from connection */
#define RS_CB_PROXY 		4   /* via proxy */


class pqipeer
{
        public:
std::string id;
std::string name;
uint32_t    state;
uint32_t    actions;
};

class p3ConnectMgr;

class pqiMonitor
{
	public:
	pqiMonitor() :mConnMgr(NULL) { return; }
virtual ~pqiMonitor() { return; }

	void setConnectionMgr(p3ConnectMgr *cm) { mConnMgr = cm; }
virtual void	statusChange(const std::list<pqipeer> &plist) = 0;
//virtual void	ownStatusChange(pqipeer &) { return; } // SIGNAL reset or similar.
//virtual void	peerStatus(std::string id, uint32_t mode) = 0;

	protected:
	p3ConnectMgr *mConnMgr;
};




class pqiConnectCb
{
	public:
virtual ~pqiConnectCb() { return; }
virtual void	peerStatus(std::string id, const pqiIpAddrSet &addrs,
			uint32_t type, uint32_t flags, uint32_t source) = 0;

virtual void    peerConnectRequest(std::string id,              
                        struct sockaddr_in raddr, uint32_t source) = 0;

//virtual void	stunStatus(std::string id, struct sockaddr_in raddr, uint32_t type, uint32_t flags) = 0;
};


/**** DUMMY CB FOR TESTING (just prints) ****/
class pqiConnectCbDummy: public pqiConnectCb
{
	public:
	pqiConnectCbDummy();
virtual ~pqiConnectCbDummy();
virtual void	peerStatus(std::string id, const pqiIpAddrSet &addrs,
			uint32_t type, uint32_t mode, uint32_t source);

virtual void    peerConnectRequest(std::string id,              
                        struct sockaddr_in raddr, uint32_t source);

//virtual void	stunStatus(std::string id, struct sockaddr_in raddr, uint32_t type, uint32_t flags);
};


/* network listener interface - used to reset network addresses */
class pqiNetListener
{
	public:
virtual bool resetListener(struct sockaddr_in &local) = 0;

};


#endif // PQI_MONITOR_H

