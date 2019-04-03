/*******************************************************************************
 * libretroshare/src/upnp: upnphandler_miniupnp.h                              *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2018 Retroshare Team <retroshare.project@gmail.com>          *
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
//windows/osx (miniupnpc) implementation
#ifndef _RS_UPNP_IFACE_H
#define _RS_UPNP_IFACE_H

#include <string.h>

#include <string>
#include <map>

/* platform independent networking... */
#include "pqi/pqinetwork.h"
#include "pqi/pqiassist.h"

#include "util/rsthreads.h"

class upnpentry
{
	public:
	std::string name;
	std::string id;
	struct sockaddr_in addr;
	unsigned int flags;
	int status;
	int lastTs;
};

class upnpforward
{
	public:
	std::string name;
	unsigned int flags;
	struct sockaddr_in iaddr;
	struct sockaddr_in eaddr;
	int status;
	int lastTs;
};


#define RS_UPNP_S_UNINITIALISED  0
#define RS_UPNP_S_UNAVAILABLE    1
#define RS_UPNP_S_READY          2
#define RS_UPNP_S_TCP_FAILED     3
#define RS_UPNP_S_UDP_FAILED     4
#define RS_UPNP_S_ACTIVE         5

class uPnPConfigData;

class upnphandler: public pqiNetAssistFirewall
{
	public:

	upnphandler();
virtual	~upnphandler();

	/* External Interface (pqiNetAssistFirewall) */
virtual void    enable(bool active);
virtual void    shutdown();
virtual void 	restart();

virtual bool    getEnabled();
virtual bool    getActive();

virtual void    setInternalPort(unsigned short iport_in);
virtual void    setExternalPort(unsigned short eport_in);
virtual bool    getInternalAddress(struct sockaddr_storage &addr);
virtual bool    getExternalAddress(struct sockaddr_storage &addr);

	/* TO IMPLEMENT: New Port Forward interface to support as many ports as necessary */
virtual bool    requestPortForward(const PortForwardParams &/*params*/) { return false; }
virtual bool    statusPortForward(const uint32_t /*fwdId*/, PortForwardParams &/*params*/) { return false; }

/* Public functions - for background thread operation,
 * but effectively private from rest of RS, as in derived class
 */

bool 	start_upnp();
bool	shutdown_upnp();

bool initUPnPState();
bool printUPnPState();

	private:

bool background_setup_upnp(bool, bool);
bool checkUPnPActive();

	/* Mutex for data below */
	RsMutex dataMtx;

	bool toEnable;   /* overall on/off switch */
	bool toStart;  /* if set start forwarding */
	bool toStop;   /* if set stop  forwarding */

	unsigned short iport;
	unsigned short eport;       /* config            */
	unsigned short eport_curr;  /* current forwarded */

	/* info from upnp */
	unsigned int upnpState;
	uPnPConfigData *upnpConfig;

	struct sockaddr_in upnp_iaddr;
	struct sockaddr_in upnp_eaddr;

	/* active port forwarding */
	std::list<upnpforward> activeForwards;

};

#endif /* _RS_UPNP_IFACE_H */
