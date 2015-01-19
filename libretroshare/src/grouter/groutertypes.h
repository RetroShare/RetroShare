/*
 * libretroshare/src/services: groutermatrix.h
 *
 * Services for RetroShare.
 *
 * Copyright 2013 by Cyril Soler
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
 * Please report all bugs and problems to "csoler@users.sourceforge.net".
 *
 */

#pragma once

#include <stdint.h>
#include <time.h>
#include <list>
#include "pgp/rscertificate.h"
#include "turtle/p3turtle.h"
#include "retroshare/rsgrouter.h"

class RsGRouterGenericDataItem ;
class RsGRouterSignedReceiptItem ;

static const uint16_t GROUTER_CLIENT_ID_MESSAGES     = 0x1001 ;

static const uint32_t RS_GROUTER_MATRIX_MAX_HIT_ENTRIES       = 10;	// max number of clues to store
static const uint32_t RS_GROUTER_MATRIX_MIN_TIME_BETWEEN_HITS = 60;	// can be set to up to half the publish time interval. Prevents flooding routes.
static const uint32_t RS_GROUTER_MIN_CONFIG_SAVE_PERIOD =  5;	// at most save config every 5 seconds

static const float RS_GROUTER_BASE_WEIGHT_ROUTED_MSG  = 1.0f ; 		// base contribution of routed message clue to routing matrix
static const float RS_GROUTER_BASE_WEIGHT_GXS_PACKET  = 0.1f ; 		// base contribution of GXS message to routing matrix

static const time_t RS_GROUTER_DEBUG_OUTPUT_PERIOD         =       10 ; // Output everything
static const time_t RS_GROUTER_AUTOWASH_PERIOD             =       10 ; // Autowash every minute. Not a costly operation.
static const time_t RS_GROUTER_MATRIX_UPDATE_PERIOD        =    1 *10 ; // Check for key advertising every 10 minutes
static const time_t RS_GROUTER_ROUTING_WAITING_TIME        =    2 *60 ; // time between two trial of sending a given message
//atic const time_t RS_GROUTER_ROUTING_WAITING_TIME        =     3600 ; // time between two trial of sending a given message
static const time_t RS_GROUTER_MEAN_EXPECTED_RTT           =       30 ; // reference RTT time for a message. 

static const uint32_t GROUTER_ITEM_DISTANCE_UNIT           =      256 ; // One unit of distance between two peers
static const uint32_t GROUTER_ITEM_MAX_TRAVEL_DISTANCE     =    6*256 ; // 6 distance units. That is a lot.
static const uint32_t GROUTER_ITEM_MAX_CACHE_KEEP_TIME     =    86400 ; // Cached items are kept for 24 hours at most.
static const uint32_t GROUTER_ITEM_MAX_CACHE_KEEP_TIME_DEAD=     3600 ; // DEAD Items are kept in cache for only 1 hour to favor re-exploring dead routes.

static const uint32_t RS_GROUTER_DATA_STATUS_UNKNOWN       = 0x0000 ;	// unknown. Unused.
static const uint32_t RS_GROUTER_DATA_STATUS_PENDING       = 0x0001 ;	// item is pending. Should be sent asap.
static const uint32_t RS_GROUTER_DATA_STATUS_SENT          = 0x0002 ;	// item is sent. Waiting for answer
static const uint32_t RS_GROUTER_DATA_STATUS_RECEIPT_OK    = 0x0003 ;	// item is at destination.

static const uint32_t RS_GROUTER_TUNNEL_STATUS_UNMANAGED   = 0x0000 ; // no tunnel requested atm
static const uint32_t RS_GROUTER_TUNNEL_STATUS_PENDING     = 0x0001 ; // tunnel requested to turtle
static const uint32_t RS_GROUTER_TUNNEL_STATUS_READY       = 0x0002 ; // tunnel is ready but we're still waiting for various confirmations
static const uint32_t RS_GROUTER_TUNNEL_STATUS_CAN_SEND    = 0x0003 ; // tunnel is ready and data can be sent

class FriendTrialRecord
{
	public:
		RsPeerId  friend_id ;			// id of the friend
		time_t    time_stamp ;			// time of the last tried
		float     probability ;			// probability at which the item was selected
		uint32_t  nb_friends ;			// number of friends at the time of sending the item
	
		bool serialise(void *data,uint32_t& offset,uint32_t size) const ;
		bool deserialise(void *data,uint32_t& offset,uint32_t size) ;
};

class GRouterRoutingInfo
{
public:
    uint32_t data_status ;			// pending, waiting, etc.
    uint32_t tunnel_status ;		// status of tunnel handling.
    time_t received_time_TS ;		// time at which the item was originally received
    time_t last_sent_TS ;			// last time the item was sent to friends
    time_t last_tunnel_request_TS ;		// last time tunnels have been asked for this item.
    uint32_t sending_attempts ;		// number of times tunnels have been asked for this peer without success

    RsGxsId destination_key ;		// ultimate destination for this key
    GRouterServiceId client_id ;		// service ID of the client. Only valid when origin==OwnId
    TurtleFileHash tunnel_hash ;		// tunnel hash to be used for this item

    RsGRouterGenericDataItem *data_item ;
    RsGRouterSignedReceiptItem *receipt_item ;
};

