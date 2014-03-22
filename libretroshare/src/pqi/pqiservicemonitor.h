/*
 * libretroshare/src/pqi: pqiservicemonitor.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2014 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#ifndef PQI_SERVICE_MONITOR_H
#define PQI_SERVICE_MONITOR_H

#include <inttypes.h>
#include <string>
#include <list>
#include <retroshare/rstypes.h>

/* ACTIONS */
const uint32_t RS_SERVICE_PEER_NEW              = 0x0001;    /* new Peer */
const uint32_t RS_SERVICE_PEER_CONNECTED        = 0x0002;    /* service connected */
const uint32_t RS_SERVICE_PEER_DISCONNECTED     = 0x0010;
const uint32_t RS_SERVICE_PEER_REMOVED		= 0x0020;

class pqiServicePeer
{
        public:
	RsPeerId    id;
	uint32_t    actions;
};


class pqiServiceMonitor
{
	public:
	pqiServiceMonitor()  { return; }
virtual ~pqiServiceMonitor() { return; }

	/*!
	 * this serves as a call back function for server which has
	 * a handle on the subclass and updates this subclass on the
	 * action of peer's of the client (state and action information)
	 *
	 *@param plist contains list of states and actions of the client's peers
	 */
virtual void	statusChange(const std::list<pqiServicePeer> &plist) = 0;
};

#endif // PQI_SERVICE_MONITOR_H
