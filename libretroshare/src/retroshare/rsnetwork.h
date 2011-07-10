#ifndef RETROSHARE_NETWORK_GUI_INTERFACE_H
#define RETROSHARE_NETWORK_GUI_INTERFACE_H

/*
 * libretroshare/src/retroshare: rsnetwork.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2011-2011 by Robert Fernie.
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

#define RSNET_NETWORK_UNKNOWN		1
#define RSNET_NETWORK_RESTARTING	2
#define RSNET_NETWORK_OFFLINE		3
#define RSNET_NETWORK_LOCALNET		4
#define RSNET_NETWORK_BEHINDNAT		5
#define RSNET_NETWORK_EXTERNALIP	6

// WHAT TYPE OF FIREWALL?
#define RSNET_NATTYPE_NONE		1
#define RSNET_NATTYPE_UNKNOWN		2
#define RSNET_NATTYPE_SYMMETRIC 	3
#define RSNET_NATTYPE_RESTRICTED_CONE	4
#define RSNET_NATTYPE_FULL_CONE		5
#define RSNET_NATTYPE_OTHER		6

// WHAT TYPE OF HOLE?
#define RSNET_NATHOLE_UNKNOWN		0		
#define RSNET_NATHOLE_NONE		1		
#define RSNET_NATHOLE_UPNP		2	
#define RSNET_NATHOLE_NATPMP		3
#define RSNET_NATHOLE_FORWARDED		4

// Types of Connections.
#define RSNET_CONNECT_NONE		0x0000
#define RSNET_CONNECT_ACCEPT_TCP	0x0001
#define RSNET_CONNECT_OUTGOING_TCP	0x0002
#define RSNET_CONNECT_DIRECT_UDP	0x0100
#define RSNET_CONNECT_PROXY_UDP		0x0200
#define RSNET_CONNECT_RELAY_UDP		0x0400

// net state (good, okay, bad)
// BAD. (RED)
#define RSNET_NETSTATE_BAD_UNKNOWN	1
#define RSNET_NETSTATE_BAD_OFFLINE	2
#define RSNET_NETSTATE_BAD_NATSYM	3
#define RSNET_NETSTATE_BAD_NODHT_NAT	4

// CAUTION. (ORANGE)
#define RSNET_NETSTATE_WARNING_RESTART	5
#define RSNET_NETSTATE_WARNING_NATTED	6
#define RSNET_NETSTATE_WARNING_NODHT	7

// GOOD (GREEN)
// NAT with forwarded port, or EXT port.
#define RSNET_NETSTATE_GOOD		8

// ADVANCED MODE (BLUE)
// If the user knows what they are doing... we cannot confirm this.
#define RSNET_NETSTATE_ADV_FORWARD	9
#define RSNET_NETSTATE_ADV_DARK_FORWARD	10



#endif
