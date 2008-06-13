#ifndef RS_DISTRIB_GUI_INTERFACE_H
#define RS_DISTRIB_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rsdistrib.h
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2007-2008 by Robert Fernie.
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


#define RS_DISTRIB_PRIVACY_MASK		0x000f   /* who can publish & view */
#define RS_DISTRIB_AUTHEN_MASK		0x00f0   /* how to publish */
#define RS_DISTRIB_LISTEN_MASK		0x0f00   /* distribution flags */

#define RS_DISTRIB_PUBLIC		0x0001   /* anyone can publish */
#define RS_DISTRIB_PRIVATE		0x0002   /* anyone with key can publish */
#define RS_DISTRIB_ENCRYPTED		0x0004   /* need publish key to view */

#define RS_DISTRIB_AUTHEN_REQ		0x0010   /* you must sign messages */
#define RS_DISTRIB_AUTHEN_ANON		0x0020   /* you can send anonymous messages */

#define RS_DISTRIB_ADMIN		0x0100   
#define RS_DISTRIB_PUBLISH		0x0200   
#define RS_DISTRIB_SUBSCRIBED		0x0400   


#endif
