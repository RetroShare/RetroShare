/*
 * libretroshare/src/serialiser: itempriorities.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2011-2011 by Cyril Soler
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net"
 *
 */

// This file centralises QoS priorities for all transfer RsItems. 
//
const uint8_t QOS_PRIORITY_UNKNOWN                    = 0 ;
const uint8_t QOS_PRIORITY_DEFAULT                    = 3 ;
const uint8_t QOS_PRIORITY_TOP                        = 9 ;

// Turtle traffic
//
const uint8_t QOS_PRIORITY_RS_TURTLE_OPEN_TUNNEL    	= 6 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_TUNNEL_OK      	= 6 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_SEARCH_REQUEST	= 5 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_REQUEST   	= 5 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_CRC_REQUEST = 5 ;	
const uint8_t QOS_PRIORITY_RS_TURTLE_CHUNK_CRC_REQUEST= 5 ;	
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_MAP_REQUEST = 5 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_SEARCH_RESULT  	= 3 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_DATA      	= 3 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_CRC         = 3 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_CHUNK_CRC        = 5 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FILE_MAP         = 3 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_GENERIC_ITEM     = 3 ;
const uint8_t QOS_PRIORITY_RS_TURTLE_FORWARD_FILE_DATA= 2 ;

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

// Chat/Msgs
//
const uint8_t QOS_PRIORITY_RS_CHAT_ITEM       			= 7 ;
const uint8_t QOS_PRIORITY_RS_CHAT_AVATAR_ITEM       	= 2 ;
const uint8_t QOS_PRIORITY_RS_MSG_ITEM               	= 2 ;
const uint8_t QOS_PRIORITY_RS_STATUS_ITEM     			= 2 ;

// VOIP
//
const uint8_t QOS_PRIORITY_RS_VOIP_PING               = 9 ;

// BanList
//
const uint8_t QOS_PRIORITY_RS_BANLIST_ITEM     			= 2 ;

// Dsdv Routing
//
const uint8_t QOS_PRIORITY_RS_DSDV_ROUTE     			= 4 ;
const uint8_t QOS_PRIORITY_RS_DSDV_DATA     			= 2 ;

