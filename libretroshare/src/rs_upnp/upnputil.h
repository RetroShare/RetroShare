/*******************************************************************************
 * libretroshare/src/upnp: upnputil.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 * From MiniUPnPc, re-licensed with permission                                 *
 *                                                                             *
 * Copyright (c) 2005-2016, Thomas BERNARD                                     *
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

//this file uses miniupnp

#ifndef MINIUPNP_UTIL_H_
#define MINIUPNP_UTIL_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#define snprintf _snprintf
#endif
#include <miniupnpc/miniwget.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>

//Define this variable follow the date of used MiniUPnP Library
//#define MINIUPNPC_API_VERSION	-3
#ifndef MINIUPNPC_API_VERSION
#error MINIUPNPC_API_VERSION is not defined. You may define one follow miniupnpc library version
	//2006/09/04 to 2007/10/01 => -7//Start return struct UPNPDev * for upnpDiscover
	//2007/10/16 => -6 upnpDiscover
	//2007/12/19 => -5 upnpDiscover
	//2008/09/25 => -4 upnpDiscover
	//2009/04/17 => -3 UPNP_AddPortMapping
	//2010/12/09 => -2 //struct IGDdatas_service CIF;
	//2011/02/15 => -1 UPNP_AddPortMapping
	//2011/03/14 => 6 //Start of MINIUPNPC_API_VERSION
#endif//>=-7

/* Ensure linking names are okay on OSX platform. (C interface) */

#ifdef  __cplusplus
extern "C" {
#endif


/* protofix() checks if protocol is "UDP" or "TCP" 
 * returns NULL if not */
const char * protofix(const char * proto);
void DisplayInfos(struct UPNPUrls * urls,
                  struct IGDdatas * data);

void GetConnectionStatus(struct UPNPUrls * urls,
                         struct IGDdatas * data);

void ListRedirections(struct UPNPUrls * urls,
                      struct IGDdatas * data);

int SetRedirectAndTest(struct UPNPUrls * urls,
                       struct IGDdatas * data,
                       const char * iaddr,
                       const char * iport,
                       const char * eport,
                       const char * proto,
                       const char *leaseDuration,
                       const char *description,
                       int addAny);

int TestRedirect(struct UPNPUrls * urls,
                               struct IGDdatas * data,
				const char * iaddr,
				const char * iport,
				const char * eport,
                       		const char * proto);

int RemoveRedirect(struct UPNPUrls * urls,
                    struct IGDdatas * data,
			   const char * eport,
			   const char * proto);

#ifdef  __cplusplus
}
#endif

/* EOF */
#endif
