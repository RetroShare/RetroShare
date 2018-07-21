/*******************************************************************************
 * libretroshare/src/pqi: p3upnpmgr.h                                          *
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
#ifndef MRK_P3_UPNP_MANAGER_H
#define MRK_P3_UPNP_MANAGER_H

/* platform independent networking... */
#include "util/rsthreads.h"
#include "pqi/pqinetwork.h"

class p3UpnpMgr: public pqiNetAssistFirewall
{
	public:

virtual	~p3UpnpMgr() { return; }

		/* External Interface */
virtual void    enable(bool on) = 0;  /* launches thread to start it up */
virtual void    shutdown() = 0;       /* blocking shutdown call */
virtual void	restart() = 0;   	  /* must be called if ports change */

virtual bool    getEnabled() = 0;
virtual bool    getActive() = 0;

		/* the address that the listening port is on */
virtual void    setInternalPort(unsigned short iport_in) = 0;
virtual void    setExternalPort(unsigned short eport_in) = 0;
 
	 	/* as determined by uPnP */
virtual bool    getInternalAddress(struct sockaddr_storage &addr) = 0;
virtual bool    getExternalAddress(struct sockaddr_storage &addr) = 0;

};

#endif /* MRK_P3_UPNP_MANAGER_H */

