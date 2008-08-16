/*
 * libretroshare/src/pqi: p3upnpmgr.h
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
virtual bool    getInternalAddress(struct sockaddr_in &addr) = 0;
virtual bool    getExternalAddress(struct sockaddr_in &addr) = 0;

};

#endif /* MRK_P3_UPNP_MANAGER_H */

