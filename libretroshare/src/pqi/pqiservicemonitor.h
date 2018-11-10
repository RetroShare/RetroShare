/*******************************************************************************
 * libretroshare/src/pqi: pqiservicemonitor.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2014 by Robert Fernie <drbob@lunamutt.com>                        *
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
