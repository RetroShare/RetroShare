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
#include "retroshare/rsgrouter.h"

class RsGRouterGenericDataItem ;

static const uint32_t GROUTER_CLIENT_ID_MESSAGES     = 0x1001 ;

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
static const uint32_t GROUTER_ITEM_MAX_CACHE_KEEP_TIME     = 30*86400 ; // Items are kept in cache for 1 month, to allow sending to peers while not online.

static const uint32_t RS_GROUTER_ROUTING_STATE_UNKN = 0x0000 ;		// unknown. Unused.
static const uint32_t RS_GROUTER_ROUTING_STATE_PEND = 0x0001 ;		// item is pending. Should be sent asap. 
static const uint32_t RS_GROUTER_ROUTING_STATE_SENT = 0x0002 ;		// item is sent. Waiting for answer
static const uint32_t RS_GROUTER_ROUTING_STATE_ARVD = 0x0003 ;		// item is at destination. The cache only holds it to avoid duplication.
static const uint32_t RS_GROUTER_ROUTING_STATE_DEAD = 0x0004 ;		// item is at a dead end.

static const uint32_t RS_GROUTER_ACK_STATE_UNKN                = 0x0000 ;		// unknown destination key
static const uint32_t RS_GROUTER_ACK_STATE_RCVD                = 0x0001 ;		// data was received, directly
static const uint32_t RS_GROUTER_ACK_STATE_IRCV                = 0x0002 ;		// data was received indirectly
static const uint32_t RS_GROUTER_ACK_STATE_GVNP                = 0x0003 ;		// data was given up. No route.
static const uint32_t RS_GROUTER_ACK_STATE_NORO                = 0x0004 ;		// data was given up. No route.
static const uint32_t RS_GROUTER_ACK_STATE_TOOF                = 0x0005 ;		// dropped because of distance (too far)

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
		uint32_t status_flags ;									// pending, waiting, etc.
		RsPeerId  origin ;										// which friend sent us that item
		time_t received_time ;									// time at which the item was originally received
		time_t last_sent ;										// last time the item was sent to friends

		std::list<FriendTrialRecord> tried_friends ; 	// list of friends to which the item was sent ordered with time.
		GRouterKeyId destination_key ;						// ultimate destination for this key
		GRouterServiceId client_id ;							// service ID of the client. Only valid when origin==OwnId

		RsGRouterGenericDataItem *data_item ;
};

