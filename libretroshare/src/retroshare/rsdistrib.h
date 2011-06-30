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


#define RS_DISTRIB_PRIVACY_MASK		0x0000000f   /* who can publish & view */
#define RS_DISTRIB_AUTHEN_MASK		0x000000f0   /* how to publish */
#define RS_DISTRIB_LISTEN_MASK		0x00000f00   /* distribution flags */
#define RS_DISTRIB_UPDATE_MASK 		0x0000f000   /* if sending a group info update */
#define RS_DISTRIB_MISC_MASK 		0x00ff0000   /* if sending a group info update */

#define RS_DISTRIB_PUBLIC		0x00000001   /* anyone can publish */
#define RS_DISTRIB_PRIVATE		0x00000002   /* anyone with key can publish */
#define RS_DISTRIB_ENCRYPTED		0x00000004   /* need publish key to view */

#define RS_DISTRIB_AUTHEN_REQ		0x00000010   /* you must sign messages */
#define RS_DISTRIB_AUTHEN_ANON		0x00000020   /* you can send anonymous messages */

#define RS_DISTRIB_ADMIN		0x00000100   
#define RS_DISTRIB_PUBLISH		0x00000200   
#define RS_DISTRIB_SUBSCRIBED		0x00000400   

#define RS_DISTRIB_UPDATE		0x00001000

/* don't know if this should go with the above flags, as it message specific, and not a group settings.
 * As it is currently not stored any configuration, it can be changed.
 */

#define RS_DISTRIB_MISSING_MSG		0x00010000


#endif
