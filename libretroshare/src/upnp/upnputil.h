//this file uses miniupnp

#ifndef MINIUPNP_UTIL_H_
#define MINIUPNP_UTIL_H_

/* $Id: upnpc.c,v 1.50 2007/04/26 19:00:10 nanard Exp $ */
/* Project : miniupnp
 * Author : Thomas Bernard
 * Copyright (c) 2005 Thomas Bernard
 * This software is subject to the conditions detailed in the
 * LICENCE file provided in this distribution.
 * */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#include <winsock2.h>
#define snprintf _snprintf
#endif
#include <miniwget.h>
#include <miniupnpc.h>
#include <upnpcommands.h>

/* protofix() checks if protocol is "UDP" or "TCP" 
 * returns NULL if not */
const char * protofix(const char * proto);
void DisplayInfos(struct UPNPUrls * urls,
                  struct IGDdatas * data);

void GetConnectionStatus(struct UPNPUrls * urls,
                         struct IGDdatas * data);

void ListRedirections(struct UPNPUrls * urls,
                      struct IGDdatas * data);

bool SetRedirectAndTest(struct UPNPUrls * urls,
                               struct IGDdatas * data,
				const char * iaddr,
				const char * iport,
				const char * eport,
                       		const char * proto);

bool TestRedirect(struct UPNPUrls * urls,
                               struct IGDdatas * data,
				const char * iaddr,
				const char * iport,
				const char * eport,
                       		const char * proto);

bool RemoveRedirect(struct UPNPUrls * urls,
                    struct IGDdatas * data,
			   const char * eport,
			   const char * proto);

/* EOF */
#endif
