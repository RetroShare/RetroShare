#ifndef RS_SERVICE_IDS_H
#define RS_SERVICE_IDS_H

/*
 * libretroshare/src/serialiser: rsserviceids.h
 *
 * RetroShare Serialiser.
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

#include <inttypes.h>

/* Single place for Cache/Service Ids (uint16_t)
 * to 64K of them....
 *
 * Some Services/Caches are only msgs or caches...
 * but they need to be combined so that the messages/caches
 * can easily be aligned.
 *
 */

/* These are Cache Only */
const uint16_t RS_SERVICE_TYPE_FILE_INDEX  = 0x0001;
const uint16_t RS_SERVICE_TYPE_RANK        = 0x0002;

/* These are Services only */
const uint16_t RS_SERVICE_TYPE_DISC        = 0x0011;
const uint16_t RS_SERVICE_TYPE_CHAT        = 0x0012;
const uint16_t RS_SERVICE_TYPE_MSG         = 0x0013;


/****************** BELOW ARE ONLY THEORETICAL (CAN BE CHANGED) *****/

const uint16_t RS_SERVICE_TYPE_STATUS      = 0x0012;


const uint16_t RS_SERVICE_TYPE_CHANNEL_MSG = 0x0015;
const uint16_t RS_SERVICE_TYPE_PROXY_MSG   = 0x0016;


const uint16_t RS_SERVICE_TYPE_GAME_LAUNCHER = 0x1000;

/* Example Games */
/* Board Games */
const uint16_t RS_SERVICE_TYPE_GAME_QTCHESS  = 0x1001;
const uint16_t RS_SERVICE_TYPE_GAME_QGO      = 0x1002;

/* Card Games */
const uint16_t RS_SERVICE_TYPE_GAME_BIGTWO   = 0x1003;
const uint16_t RS_SERVICE_TYPE_GAME_POKER    = 0x1004;



/* Combined Cache/Service ids */



#endif /* RS_SERVICE_IDS_H */


