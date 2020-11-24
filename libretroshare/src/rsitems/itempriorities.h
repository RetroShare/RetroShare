/*******************************************************************************
 * libretroshare/src/rsitems: rsitempriorities.h                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Cyril Soler <csoler@users.sourceforge.net>           *
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
#pragma once

#include <stdint.h>

// This file centralises QoS priorities for all transfer RsItems. 
//
const uint8_t QOS_PRIORITY_UNKNOWN                    = 0 ;
const uint8_t QOS_PRIORITY_DEFAULT                    = 3 ;
const uint8_t QOS_PRIORITY_TOP                        = 9 ;

// Turtle traffic
//
const uint8_t QOS_PRIORITY_RS_TURTLE_OPEN_TUNNEL      = 6 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_TUNNEL_OK        = 6 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_SEARCH_REQUEST   = 6 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_REQUEST     = 5 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_CRC_REQUEST = 5 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_CHUNK_CRC_REQUEST= 5 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_MAP_REQUEST = 5 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_SEARCH_RESULT    = 3 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_DATA        = 3 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_CRC         = 3 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_CHUNK_CRC        = 5 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_MAP         = 3 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_GENERIC_ITEM     = 3 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FORWARD_FILE_DATA= 3 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_GENERIC_DATA     = 5 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_GENERIC_FAST_DATA= 7 ;

// File transfer
//
const uint8_t QOS_PRIORITY_RS_FILE_REQUEST   			= 5 ;
const uint8_t QOS_PRIORITY_RS_FILE_CRC_REQUEST 			= 5 ;	
const uint8_t QOS_PRIORITY_RS_CHUNK_CRC_REQUEST			= 5 ;	
const uint8_t QOS_PRIORITY_RS_FILE_MAP_REQUEST 			= 5 ;
const uint8_t QOS_PRIORITY_RS_CACHE_REQUEST   			= 4 ;
const uint8_t QOS_PRIORITY_RS_FILE_DATA      			= 3 ;
const uint8_t QOS_PRIORITY_RS_FILE_CRC         			= 3 ;
const uint8_t QOS_PRIORITY_RS_CHUNK_CRC        			= 5 ;
const uint8_t QOS_PRIORITY_RS_FILE_MAP         			= 3 ;
const uint8_t QOS_PRIORITY_RS_CACHE_ITEM      			= 3 ;

// Discovery
//
const uint8_t QOS_PRIORITY_RS_DISC_HEART_BEAT 			= 8 ;
const uint8_t QOS_PRIORITY_RS_DISC_ASK_INFO       		= 2 ;
const uint8_t QOS_PRIORITY_RS_DISC_REPLY      			= 1 ;
const uint8_t QOS_PRIORITY_RS_DISC_VERSION    			= 1 ;

const uint8_t QOS_PRIORITY_RS_DISC_CONTACT    			= 2 ; // CONTACT and PGPLIST must have
const uint8_t QOS_PRIORITY_RS_DISC_PGP_LIST       		= 2 ; // same priority.
const uint8_t QOS_PRIORITY_RS_DISC_SERVICES    			= 2 ;
const uint8_t QOS_PRIORITY_RS_DISC_PGP_CERT    			= 1 ;

// File database
//
const uint8_t QOS_PRIORITY_RS_FAST_SYNC_REQUEST         = 7 ;
const uint8_t QOS_PRIORITY_RS_SLOW_SYNC_REQUEST         = 3 ;

// Heartbeat.
//
const uint8_t QOS_PRIORITY_RS_HEARTBEAT_PULSE 			= 8 ;

// Chat/Msgs
//
const uint8_t QOS_PRIORITY_RS_CHAT_ITEM       			= 7 ;
const uint8_t QOS_PRIORITY_RS_CHAT_AVATAR_ITEM       	= 2 ;
const uint8_t QOS_PRIORITY_RS_MSG_ITEM               	= 2 ;  // depreciated.
const uint8_t QOS_PRIORITY_RS_MAIL_ITEM               	= 2 ;  // new mail service
const uint8_t QOS_PRIORITY_RS_STATUS_ITEM     			= 2 ;

// RTT
//
const uint8_t QOS_PRIORITY_RS_RTT_PING               = 9 ;

// BanList
//
const uint8_t QOS_PRIORITY_RS_BANLIST_ITEM     			= 2 ;

// Bandwidth Control.
//
const uint8_t QOS_PRIORITY_RS_BWCTRL_ALLOWED_ITEM 		= 9 ;

// Dsdv Routing
//
const uint8_t QOS_PRIORITY_RS_DSDV_ROUTE     			= 4 ;
const uint8_t QOS_PRIORITY_RS_DSDV_DATA     			= 2 ;

// GXS
//
const uint8_t QOS_PRIORITY_RS_GXS_NET                   = 3 ;
// GXS Reputation.
const uint8_t QOS_PRIORITY_RS_GXSREPUTATION_ITEM		= 2;

// Service Info / Control.
const uint8_t QOS_PRIORITY_RS_SERVICE_INFO_ITEM		= 7;

