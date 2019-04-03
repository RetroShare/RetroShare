/*******************************************************************************
 * libretroshare/src/rsitems: rsserviceids.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef RS_SERVICE_IDS_H
#define RS_SERVICE_IDS_H

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
const uint16_t RS_SERVICE_TYPE_FILE_INDEX     = 0x0001;

/* These are Services only */
const uint16_t RS_SERVICE_TYPE_DISC           = 0x0011;
const uint16_t RS_SERVICE_TYPE_CHAT           = 0x0012;
const uint16_t RS_SERVICE_TYPE_MSG            = 0x0013;
const uint16_t RS_SERVICE_TYPE_TURTLE         = 0x0014;
const uint16_t RS_SERVICE_TYPE_TUNNEL         = 0x0015;
const uint16_t RS_SERVICE_TYPE_HEARTBEAT      = 0x0016;
const uint16_t RS_SERVICE_TYPE_FILE_TRANSFER  = 0x0017;
const uint16_t RS_SERVICE_TYPE_GROUTER        = 0x0018;
const uint16_t RS_SERVICE_TYPE_FILE_DATABASE  = 0x0019;

const uint16_t RS_SERVICE_TYPE_SERVICEINFO    = 0x0020;
/* Bandwidth Control */
const uint16_t RS_SERVICE_TYPE_BWCTRL         = 0x0021;
// New Mail Service (replace old Msg Service) 
const uint16_t RS_SERVICE_TYPE_MAIL           = 0x0022;
const uint16_t RS_SERVICE_TYPE_DIRECT_MAIL    = 0x0023;
const uint16_t RS_SERVICE_TYPE_DISTANT_MAIL   = 0x0024;
const uint16_t RS_SERVICE_TYPE_GWEMAIL_MAIL   = 0x0025;
const uint16_t RS_SERVICE_TYPE_SERVICE_CONTROL= 0x0026;
const uint16_t RS_SERVICE_TYPE_DISTANT_CHAT   = 0x0027;
const uint16_t RS_SERVICE_TYPE_GXS_TUNNEL     = 0x0028;

// Non essential services.
const uint16_t RS_SERVICE_TYPE_BANLIST        = 0x0101;
const uint16_t RS_SERVICE_TYPE_STATUS         = 0x0102;

 /* New Cache Services  */
/* Rs Network Exchange Service */
const uint16_t RS_SERVICE_TYPE_NXS 	       = 0x0200;

const uint16_t RS_SERVICE_GXS_TYPE_GXSID       = 0x0211;
const uint16_t RS_SERVICE_GXS_TYPE_PHOTO       = 0x0212;
const uint16_t RS_SERVICE_GXS_TYPE_WIKI        = 0x0213;
const uint16_t RS_SERVICE_GXS_TYPE_WIRE        = 0x0214;
const uint16_t RS_SERVICE_GXS_TYPE_FORUMS      = 0x0215;
const uint16_t RS_SERVICE_GXS_TYPE_POSTED      = 0x0216;
const uint16_t RS_SERVICE_GXS_TYPE_CHANNELS    = 0x0217;
const uint16_t RS_SERVICE_GXS_TYPE_GXSCIRCLE   = 0x0218;
// not gxs, but used with identities.
const uint16_t RS_SERVICE_GXS_TYPE_REPUTATION  = 0x0219;
const uint16_t RS_SERVICE_TYPE_GXS_RECOGN      = 0x0220;
const uint16_t RS_SERVICE_TYPE_GXS_TRANS       = 0x0230;
const uint16_t RS_SERVICE_TYPE_JSONAPI         = 0x0240;

const uint16_t RS_SERVICE_GXS_TYPE_FORUMS_CONFIG   = 0x0315;
const uint16_t RS_SERVICE_GXS_TYPE_CHANNELS_CONFIG = 0x0317;

// Experimental Services.
/* DSDV Testing at the moment - Service Only */
const uint16_t RS_SERVICE_TYPE_DSDV        = 0x1010;
/* Latency RTT Measurements */
const uint16_t RS_SERVICE_TYPE_RTT         = 0x1011;


/***************** IDS ALLOCATED FOR PLUGINS ******************/
// 2000+

const uint16_t RS_SERVICE_TYPE_PLUGIN_ARADO_ID    = 0x2001;
const uint16_t RS_SERVICE_TYPE_PLUGIN_QCHESS_ID   = 0x2002;
const uint16_t RS_SERVICE_TYPE_PLUGIN_FEEDREADER  = 0x2003;

// Reserved for packet slicing probes.
const uint16_t RS_SERVICE_TYPE_PACKET_SLICING_PROBE = 0xAABB;

// Nabu's services.
const uint16_t RS_SERVICE_TYPE_PLUGIN_FIDO_GW     = 0xF1D0;
const uint16_t RS_SERVICE_TYPE_PLUGIN_ZERORESERVE = 0xBEEF;

/****************** BELOW ARE ONLY THEORETICAL (CAN BE CHANGED) *****/
/*
 * If you are planning on making a new service....
 * Test it out with an ID in the range from 0xf000 - 0xffff
 * And Change the ID everytime you make significant changes to the
 * data structures... 
 *
 * eg.
 * const uint16_t RS_SERVICE_TYPE_DISTRIB_O1  = 0xf110;  // First  Revision.
 * const uint16_t RS_SERVICE_TYPE_DISTRIB_O2  = 0xf111;  // Second Revision.
 * const uint16_t RS_SERVICE_TYPE_DISTRIB     = 0xf112;  // Final  Revision.
 *
 * This minimises the chances of your new serialisers messing with
 * other existing, older, or proposed services.
 *
 * ONLY MOVE your Id once the service has been finalised.
 *
 */

 /*! for Qblog service (Cache Only) */
//const uint16_t RS_SERVICE_TYPE_QBLOG	   = 0xf010;

/* TEST VOIP - Service only */
// NOT SURE WHATS HAPPENING WITH THIS ONE? 
// SHOULD BE DEFINED IN PLUGIN SECTION!
//const uint16_t RS_SERVICE_TYPE_VOIP        = 0xf011;

 /* Proxy - Service only */
//const uint16_t RS_SERVICE_TYPE_PROXY       = 0xf030;

/* Games/External Apps - Service Only */
//const uint16_t RS_SERVICE_TYPE_GAME_LAUNCHER = 0xf200;
//const uint16_t RS_SERVICE_TYPE_PORT          = 0xf201;

/* Example Games (NOT USED YET!) */
/* Board Games */
//const uint16_t RS_SERVICE_TYPE_GAME_QTCHESS  = 0xf211;
//const uint16_t RS_SERVICE_TYPE_GAME_QGO      = 0xf212;

/* Card Games */
//const uint16_t RS_SERVICE_TYPE_GAME_BIGTWO   = 0xf213;
//const uint16_t RS_SERVICE_TYPE_GAME_POKER    = 0xf214;

/***************** IDS ALLOCATED FOR PLUGINS ******************/

//const uint16_t RS_SERVICE_TYPE_PLUGIN_ARADO_TEST_ID1   = 0xf401;
//const uint16_t RS_SERVICE_TYPE_PLUGIN_QCHESS_TEST_ID1   = 0xf402;

// test
//const uint16_t RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM = 0xf403;


#endif /* RS_SERVICE_IDS_H */


