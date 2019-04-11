/*******************************************************************************
 * libretroshare/src/pqi: pqiassist.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2007 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef MRK_PQI_ASSIST_H
#define MRK_PQI_ASSIST_H

#include <string>
#include <map>
#include "retroshare/rstypes.h"
#include "pqi/pqinetwork.h"
#include "pqi/pqimonitor.h"

/**********
 * This header file provides two interfaces for assisting 
 * the connections to friends.
 *
 * pqiNetAssistFirewall - which will provides interfaces
 * to functionality like upnp and apple's equivalent.
 *
 * pqiNetAssistConnect - which will provides interfaces
 * to other networks (DHT) etc that can provide information.
 * These classes would be expected to use the pqiMonitor
 * callback system to notify the connectionMgr.
 *
 ***/

class pqiNetAssist
{
	public:

virtual	~pqiNetAssist() { return; }

		/* External Interface */
virtual void    enable(bool on) = 0;  
virtual void    shutdown() = 0; /* blocking call */
virtual void	restart() = 0;

virtual bool    getEnabled() = 0;
virtual bool    getActive() = 0;

virtual int	tick() { return 0; } /* for internal accounting */

};


#define PFP_TYPE_UDP	0x0001
#define PFP_TYPE_TCP	0x0002

class PortForwardParams
{
	public:
	uint32_t fwdId;
	uint32_t status;
	uint32_t typeFlags;
	struct sockaddr_storage intAddr;
	struct sockaddr_storage extaddr;
};

class pqiNetAssistFirewall: public pqiNetAssist
{
	public:

virtual	~pqiNetAssistFirewall() { return; }

		/* the address that the listening port is on */
virtual void    setInternalPort(unsigned short iport_in) = 0;
virtual void    setExternalPort(unsigned short eport_in) = 0;
 
	 	/* as determined by uPnP */
virtual bool    getInternalAddress(struct sockaddr_storage &addr) = 0;
virtual bool    getExternalAddress(struct sockaddr_storage &addr) = 0;


		/* New Port Forward interface to support as many ports as necessary */
virtual bool    requestPortForward(const PortForwardParams &params) = 0;
virtual bool    statusPortForward(const uint32_t fwdId, PortForwardParams &params) = 0;


};


#define PNASS_TYPE_BADPEER	0x0001
#define PNASS_REASON_UNKNOWN	0x0001

class pqiNetAssistPeerShare
{
	public:

	/* share Addresses for various reasons (bad peers, etc) */
virtual void 	updatePeer(const RsPeerId& id, const struct sockaddr_storage &addr, int type, int reason, int age) = 0;

};


#ifdef RS_USE_DHT_STUNNER
/* this is for the Stunners 
 *
 *
 */

class pqiAddrAssist
{
	public:

	pqiAddrAssist() { return; }
virtual	~pqiAddrAssist() { return; }

virtual bool    getExternalAddr(struct sockaddr_storage &remote, uint8_t &stable) = 0;
virtual void    setRefreshPeriod(int32_t period) = 0;
virtual int	tick() = 0; /* for internal accounting */

};
#endif // RS_USE_DHT_STUNNER

#define NETASSIST_KNOWN_PEER_OFFLINE        0x0001
#define NETASSIST_KNOWN_PEER_ONLINE         0x0002

#define NETASSIST_KNOWN_PEER_WHITELIST      0x0100
#define NETASSIST_KNOWN_PEER_FRIEND         0x0200
#define NETASSIST_KNOWN_PEER_FOF            0x0400
#define NETASSIST_KNOWN_PEER_RELAY          0x0800
#define NETASSIST_KNOWN_PEER_SELF           0x1000

#define NETASSIST_KNOWN_PEER_TYPE_MASK      0xff00

class pqiNetAssistConnect: public pqiNetAssist
{
	/* 
	 */
	public:
	pqiNetAssistConnect(const RsPeerId& id, pqiConnectCb *cb)
	:mPeerId(id), mConnCb(cb) { return; }

	/********** External DHT Interface ************************
	 * These Functions are the external interface
	 * for the DHT, and must be non-blocking and return quickly
	 */


	/* add / remove peers */
virtual bool 	findPeer(const RsPeerId& id) = 0;
virtual bool 	dropPeer(const RsPeerId& id) = 0;

	/* add non-active peers (can still toggle active/non-active via above) */
virtual int addBadPeer(const struct sockaddr_storage &addr, uint32_t reason, uint32_t flags, uint32_t age) = 0;
virtual int addKnownPeer(const RsPeerId &pid, const struct sockaddr_storage &addr, uint32_t flags) = 0;

virtual void ConnectionFeedback(const RsPeerId& pid, int mode) = 0;

	/* extract current peer status */
virtual bool 	getPeerStatus(const RsPeerId& id, 
			struct sockaddr_storage &laddr, struct sockaddr_storage &raddr, 
			uint32_t &type, uint32_t &mode) = 0; // DEPRECIATE.

virtual bool    setAttachMode(bool on) = 0;		// FIXUP.

	/***** Stats for Network / DHT *****/
virtual bool    getNetworkStats(uint32_t &netsize, uint32_t &localnetsize) = 0; // DEPRECIATE.

	protected:
	RsPeerId  mPeerId;
	pqiConnectCb *mConnCb;
};


#endif /* MRK_PQI_ASSIST_H */

