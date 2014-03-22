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
const uint16_t RS_SERVICE_TYPE_FILE_INDEX    = 0x0001;

/* These are Services only */
const uint16_t RS_SERVICE_TYPE_DISC          = 0x0011;
const uint16_t RS_SERVICE_TYPE_CHAT          = 0x0012;
const uint16_t RS_SERVICE_TYPE_MSG           = 0x0013;
const uint16_t RS_SERVICE_TYPE_TURTLE        = 0x0014;
const uint16_t RS_SERVICE_TYPE_TUNNEL        = 0x0015;
const uint16_t RS_SERVICE_TYPE_HEARTBEAT     = 0x0016;
const uint16_t RS_SERVICE_TYPE_FILE_TRANSFER = 0x0017;

/* BanList Still Testing at the moment - Service Only */
const uint16_t RS_SERVICE_TYPE_BANLIST     = 0x0101;

const uint16_t RS_SERVICE_TYPE_SERVICEINFO   = 0x0201;

/* Caches based on p3distrib (Cache Only)
 * Unfortunately, noone changed the DUMMY IDS... so we are stuck with them!
 */
const uint16_t RS_SERVICE_TYPE_DISTRIB     = 0xf110;
const uint16_t RS_SERVICE_TYPE_FORUM       = 0xf120;
const uint16_t RS_SERVICE_TYPE_CHANNEL     = 0xf130;
const uint16_t RS_SERVICE_TYPE_CHANNEL_SOCKET = 0xf140;

 /* Status - Service only */
const uint16_t RS_SERVICE_TYPE_STATUS      = 0xf020;

/***************** IDS ALLOCATED FOR PLUGINS ******************/

const uint16_t RS_SERVICE_TYPE_PLUGIN_ARADO_ID   = 0x0401;
const uint16_t RS_SERVICE_TYPE_PLUGIN_QCHESS_ID  = 0x0402;
const uint16_t RS_SERVICE_TYPE_PLUGIN_FEEDREADER = 0x0403;


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
const uint16_t RS_SERVICE_TYPE_QBLOG	   = 0xf010;

/* TEST VOIP - Service only */
// NOT SURE WHATS HAPPENING WITH THIS ONE? 
// SHOULD BE DEFINED IN PLUGIN SECTION!
//const uint16_t RS_SERVICE_TYPE_VOIP        = 0xf011;

 /* Status - Service only */
//const uint16_t RS_SERVICE_TYPE_STATUS      = 0xf020;

 /* Proxy - Service only */
const uint16_t RS_SERVICE_TYPE_PROXY       = 0xf030;

/* DSDV Testing at the moment - Service Only */
const uint16_t RS_SERVICE_TYPE_DSDV        = 0xf050;
/* Latency RTT Measurements */
const uint16_t RS_SERVICE_TYPE_RTT         = 0xf051;

/* Bandwidth Testing at the moment - Service Only */
const uint16_t RS_SERVICE_TYPE_BWCTRL      = 0xf060;




//const uint16_t RS_SERVICE_TYPE_DISTRIB     = 0xf110;
//const uint16_t RS_SERVICE_TYPE_FORUM       = 0xf120;
//const uint16_t RS_SERVICE_TYPE_CHANNEL     = 0xf130;
//const uint16_t RS_SERVICE_TYPE_CHANNEL_SOCKET = 0xf140;

/* Games/External Apps - Service Only */
const uint16_t RS_SERVICE_TYPE_GAME_LAUNCHER = 0xf200;
const uint16_t RS_SERVICE_TYPE_PORT          = 0xf201;

/* Example Games (NOT USED YET!) */
/* Board Games */
const uint16_t RS_SERVICE_TYPE_GAME_QTCHESS  = 0xf211;
const uint16_t RS_SERVICE_TYPE_GAME_QGO      = 0xf212;

/* Card Games */
const uint16_t RS_SERVICE_TYPE_GAME_BIGTWO   = 0xf213;
const uint16_t RS_SERVICE_TYPE_GAME_POKER    = 0xf214;

 /* New Cache Services  */
/* Rs Network Exchange Service */
const uint16_t RS_SERVICE_TYPE_NXS 	   = 0xf300;

const uint16_t RS_SERVICE_GXSV2_TYPE_GXSID       = 0xf311;
const uint16_t RS_SERVICE_GXSV2_TYPE_GXSCIRCLE   = 0xf312;
const uint16_t RS_SERVICE_GXSV2_TYPE_PHOTO       = 0xf313;
const uint16_t RS_SERVICE_GXSV2_TYPE_WIKI        = 0xf314;
const uint16_t RS_SERVICE_GXSV2_TYPE_WIRE        = 0xf315;
const uint16_t RS_SERVICE_GXSV2_TYPE_FORUMS      = 0xf316;
const uint16_t RS_SERVICE_GXSV2_TYPE_POSTED      = 0xf317;
const uint16_t RS_SERVICE_GXSV2_TYPE_CHANNELS    = 0xf318;
const uint16_t RS_SERVICE_GXSV2_TYPE_REPUTATION  = 0xf319;

const uint16_t RS_SERVICE_GXSV3_TYPE_GXSID       = 0xf321;
const uint16_t RS_SERVICE_GXSV3_TYPE_PHOTO       = 0xf322;
const uint16_t RS_SERVICE_GXSV3_TYPE_WIKI        = 0xf323;
const uint16_t RS_SERVICE_GXSV3_TYPE_WIRE        = 0xf324;
const uint16_t RS_SERVICE_GXSV3_TYPE_FORUMS      = 0xf325;
const uint16_t RS_SERVICE_GXSV3_TYPE_POSTED      = 0xf326;
const uint16_t RS_SERVICE_GXSV3_TYPE_CHANNELS    = 0xf327;
const uint16_t RS_SERVICE_GXSV3_TYPE_GXSCIRCLE   = 0xf328;
const uint16_t RS_SERVICE_GXSV3_TYPE_REPUTATION  = 0xf329;

const uint16_t RS_SERVICE_TYPE_GXS_RECOGN        = 0xf331;

/***************** IDS ALLOCATED FOR PLUGINS ******************/

const uint16_t RS_SERVICE_TYPE_PLUGIN_ARADO_TEST_ID1   = 0xf401;
const uint16_t RS_SERVICE_TYPE_PLUGIN_QCHESS_TEST_ID1   = 0xf402;

// test
const uint16_t RS_SERVICE_TYPE_PLUGIN_SIMPLE_FORUM = 0xf403;



#endif /* RS_SERVICE_IDS_H */


