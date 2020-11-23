/*******************************************************************************
 * libretroshare/src/pqi: pqimonitor.h                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie.                                       *
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
#include "retroshare/rstypes.h"

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


#define RS_CB_DHT 		0x0001   /* from dht */
#define RS_CB_DISC 		0x0002   /* from peers */
#define RS_CB_PERSON 		0x0003   /* from connection */
#define RS_CB_PROXY 		0x0004   /* via proxy */


#define RS_CB_FLAG_MASK_MODE		0x00ff
#define RS_CB_FLAG_MASK_ORDER		0xff00

#define RS_CB_FLAG_MODE_TCP		0x0001
#define RS_CB_FLAG_MODE_UDP_DIRECT	0x0002
#define RS_CB_FLAG_MODE_UDP_PROXY	0x0004
#define RS_CB_FLAG_MODE_UDP_RELAY	0x0008

#define RS_CB_FLAG_ORDER_UNSPEC		0x0100
#define RS_CB_FLAG_ORDER_PASSIVE	0x0200
#define RS_CB_FLAG_ORDER_ACTIVE		0x0400

#define RSUDP_NUM_TOU_RECVERS		3

#define RSUDP_TOU_RECVER_DIRECT_IDX        0
#define RSUDP_TOU_RECVER_PROXY_IDX         1
#define RSUDP_TOU_RECVER_RELAY_IDX         2


class pqipeer
{
	public:
		RsPeerId id;
		std::string name;
		uint32_t    state;
		uint32_t    actions;
};

class p3LinkMgr;

/*!
 * This class should be implemented
 * to use the information that is passed to it via
 * from the p3ConnectMngr
 * Useful information is sent via ticks such as a peer's
 * state and a peer's action
 */
class pqiMonitor
{
	public:
	pqiMonitor() :mLinkMgr(NULL) { return; }
virtual ~pqiMonitor() { return; }

	/*!
	 * passes a handle the retroshare connection manager
	 */
	void setLinkMgr(p3LinkMgr *lm) { mLinkMgr = lm; }

	/*!
	 * this serves as a call back function for server which has
	 * a handle on the subclass and updates this subclass on the
	 * action of peer's of the client (state and action information)
	 *
	 *@param plist contains list of states and actions of the client's peers
	 */
virtual void	statusChange(const std::list<pqipeer> &plist) = 0;

    // This is used to force disconnection of a peer, if e.g. something suspicious happenned.

    virtual void disconnectPeer(const RsPeerId& peer) ;

#ifdef WINDOWS_SYS
///////////////////////////////////////////////////////////
// hack for too many connections
virtual void	statusChanged() {};
///////////////////////////////////////////////////////////
#endif

//virtual void	ownStatusChange(pqipeer &) { return; } // SIGNAL reset or similar.
//virtual void	peerStatus(std::string id, uint32_t mode) = 0;

	protected:
	p3LinkMgr *mLinkMgr;
};




class pqiConnectCb
{
	public:
virtual ~pqiConnectCb() { return; }
virtual void	peerStatus(const RsPeerId& id, const pqiIpAddrSet &addrs,
			uint32_t type, uint32_t flags, uint32_t source) = 0;

virtual void    peerConnectRequest(const RsPeerId& id, const struct sockaddr_storage &raddr,  
			const struct sockaddr_storage &proxyaddr,  const struct sockaddr_storage &srcaddr,  
                        uint32_t source, uint32_t flags, uint32_t delay, uint32_t bandwidth) = 0;


//virtual void	stunStatus(std::string id, const struct sockaddr_storage &raddr, uint32_t type, uint32_t flags) = 0;
};


/**** DUMMY CB FOR TESTING (just prints) ****/
class pqiConnectCbDummy: public pqiConnectCb
{
	public:
	pqiConnectCbDummy();
virtual ~pqiConnectCbDummy();
virtual void	peerStatus(const RsPeerId& id, const pqiIpAddrSet &addrs,
			uint32_t type, uint32_t mode, uint32_t source);

	virtual void peerConnectRequest(const RsPeerId& id, const struct sockaddr_storage &raddr,
	                                   const struct sockaddr_storage &proxyaddr,  const struct sockaddr_storage &srcaddr,
	                                   uint32_t source, uint32_t flags, uint32_t delay, uint32_t bandwidth);

//virtual void	stunStatus(std::string id, const struct sockaddr_storage &raddr, uint32_t type, uint32_t flags);
};


/* network listener interface - used to reset network addresses */
class pqiNetListener
{
	public:
virtual bool resetListener(const struct sockaddr_storage &local) = 0;

};


#endif // PQI_MONITOR_H

