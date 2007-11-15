/*
 * "$Id: pqipacket.h,v 1.6 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2006 by Robert Fernie.
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



#ifndef MRK_PQI_PACKET_HEADER
#define MRK_PQI_PACKET_HEADER


/* This is a set of functions for packaging 
 * the search queries/replies/data into packets
 *
 * Currently it makes very simple 1kb packets.
 *
 */

// The standard data types and the search interface.
#include "pqi/pqi.h"

// defs for streams.
#include <iostream>

typedef void  pqipkt;

// Handle Incoming Pkts......

// constants 
int	pqipkt_basesize();
int	pqipkt_maxsize();

// determine properties. (possible from incomplete pkt).
int	pqipkt_rawlen(pqipkt *);
bool    pqipkt_check(pqipkt *pkt, int size);

PQItem *pqipkt_create(pqipkt *block);

// Outgoing Pkts.... (These are raw packets....(a chunk of data))
pqipkt *pqipkt_makepkt(PQItem *pqi);

// delete the raw packet (allocated using above).
void 	pqipkt_delete(pqipkt *);

// print packet.
// 
void    pqipkt_print(pqipkt *pkt);

// Type Extension methods for PQTunnel.
class PQTunnel;
class PQTunnelInit;

int registerTunnelType(int subtype, PQTunnel *(*fn)(void *d, int n));
PQTunnel *createTunnelType(int subtype, void *d, int n);

int registerTunnelInitType(int subtype, PQTunnelInit *(*fn)(void *d, int n));
PQTunnelInit *createTunnelInitType(int subtype, void *d, int n);

#endif //MRK_PQI_PACKET_HEADER
