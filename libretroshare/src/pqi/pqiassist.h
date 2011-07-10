/*
 * libretroshare/src/pqi: pqiassist.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2007 by Robert Fernie.
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


#ifndef MRK_PQI_ASSIST_H
#define MRK_PQI_ASSIST_H

#include <string>
#include <map>
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

};

class pqiNetAssistFirewall: public pqiNetAssist
{
	public:

virtual	~pqiNetAssistFirewall() { return; }

		/* the address that the listening port is on */
virtual void    setInternalPort(unsigned short iport_in) = 0;
virtual void    setExternalPort(unsigned short eport_in) = 0;
 
	 	/* as determined by uPnP */
virtual bool    getInternalAddress(struct sockaddr_in &addr) = 0;
virtual bool    getExternalAddress(struct sockaddr_in &addr) = 0;

};


/* this is for the Stunners 
 *
 *
 */

class pqiAddrAssist
{
	public:

	pqiAddrAssist() { return; }
virtual	~pqiAddrAssist() { return; }

virtual bool    getExternalAddr(struct sockaddr_in &remote, uint8_t &stable) = 0;

};



class pqiNetAssistConnect: public pqiNetAssist
{
	/* 
	 */
	public:
	pqiNetAssistConnect(std::string id, pqiConnectCb *cb)
	:mPeerId(id), mConnCb(cb) { return; }

	/********** External DHT Interface ************************
	 * These Functions are the external interface
	 * for the DHT, and must be non-blocking and return quickly
	 */

virtual int	tick() = 0; /* for internal accounting */

	/* add / remove peers */
virtual bool 	findPeer(std::string id) = 0;
virtual bool 	dropPeer(std::string id) = 0;

	/* extract current peer status */
virtual bool 	getPeerStatus(std::string id, 
			struct sockaddr_in &laddr, struct sockaddr_in &raddr, 
			uint32_t &type, uint32_t &mode) = 0;

//virtual bool 	getExternalInterface(struct sockaddr_in &raddr, 
//					uint32_t &mode) = 0;

	/***** Stats for Network / DHT *****/
virtual bool    getNetworkStats(uint32_t &netsize, uint32_t &localnetsize) = 0;

	protected:
	std::string  mPeerId;
	pqiConnectCb *mConnCb;
};


#endif /* MRK_PQI_ASSIST_H */

