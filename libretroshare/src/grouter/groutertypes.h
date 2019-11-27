/*******************************************************************************
 * libretroshare/src/grouter: groutertypes.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013 by Cyril Soler <csoler@users.sourceforge.net>                *
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

#include <cstdint>
#include <list>

#include "util/rstime.h"
#include "pgp/rscertificate.h"
#include "turtle/p3turtle.h"
#include "retroshare/rsgrouter.h"

class RsGRouterGenericDataItem;
class RsGRouterSignedReceiptItem ;

#ifndef V07_NON_BACKWARD_COMPATIBLE_CHANGE_GROUTER_SERVICE_ID
/// Pay special attention fixing this, as may break retrocompatibility!
RS_DEPRECATED_FOR(RsServiceType::MSG)
static const uint16_t GROUTER_CLIENT_ID_MESSAGES     = 0x1001;
#endif // V07_NON_BACKWARD_COMPATIBLE_CHANGE_GROUTER_SERVICE_ID

static const uint32_t RS_GROUTER_MATRIX_MAX_HIT_ENTRIES       =        10 ; // max number of clues to store
static const uint32_t RS_GROUTER_MATRIX_MIN_TIME_BETWEEN_HITS =        60 ; // can be set to up to half the publish time interval. Prevents flooding routes.
static const uint32_t RS_GROUTER_MIN_CONFIG_SAVE_PERIOD       =        61 ; // at most save config every 10 seconds
static const uint32_t RS_GROUTER_MAX_KEEP_TRACKING_CLUES      =  86400*10 ; // max time for which we keep record of tracking info: 10 days.

static const float RS_GROUTER_BASE_WEIGHT_ROUTED_MSG          = 1.0f ;	// base contribution of routed message clue to routing matrix
static const float RS_GROUTER_BASE_WEIGHT_GXS_PACKET          = 0.1f ;	// base contribution of GXS message to routing matrix
static const float RS_GROUTER_PROBABILITY_THRESHOLD_FOR_RANDOM_ROUTING = 0.01f ;	// routing probability under which the routage is performed randomly
static const float RS_GROUTER_PROBABILITY_THRESHOLD_BEST_PEERS_SELECT  = 0.5f ;	// min ratio of forward proba with respect to best peer.

static const uint32_t MAX_TUNNEL_WAIT_TIME                 = 60          ; // wait for 60 seconds at most for a tunnel response.
static const uint32_t MAX_TUNNEL_UNMANAGED_TIME            = 600         ; // min time before retry tunnels for that msg.
static const uint32_t MAX_DELAY_FOR_RESEND                 = 2*86400+300 ; // re-send if held for more than 2 days (cache store period) plus security delay.
static const uint32_t MAX_DESTINATION_KEEP_TIME            = 20*86400    ; // keep for 20 days in destination cache to avoid re-
static const uint32_t MAX_RECEIPT_KEEP_TIME                = 20*86400    ; // keep for 20 days in destination cache to avoid re-
static const uint32_t TUNNEL_OK_WAIT_TIME                  = 2           ; // wait for 2 seconds after last tunnel ok, so that we have a complete set of tunnels.
static const uint32_t MAX_GROUTER_DATA_SIZE                = 2*1024*1024 ; // 2MB size limit. This is of course arbitrary.
static const uint32_t MAX_TRANSACTION_ACK_WAITING_TIME     = 60          ; // wait for at most 60 secs for a ACK. If not restart the transaction.
static const uint32_t DIRECT_FRIEND_TRY_DELAY              = 20          ; // wait for 20 secs if no friends available, then try tunnels.
static const uint32_t MAX_INACTIVE_DATA_PIPE_DELAY         = 300         ; // clean inactive data pipes for more than 5 mins
static const uint32_t GROUTER_MAX_DUPLICATION_FACTOR       = 10          ; // max number of duplicates for a given message to keep in the network
static const uint32_t GROUTER_MAX_BRANCHING_FACTOR         = 3           ; // max number of branches, for locally forwarding items

static const rstime_t   RS_GROUTER_DEBUG_OUTPUT_PERIOD       =      10 ; // Output everything
static const rstime_t   RS_GROUTER_AUTOWASH_PERIOD           =      10 ; // Autowash every minute. Not a costly operation.
static const rstime_t   RS_GROUTER_MATRIX_UPDATE_PERIOD      =   60*10 ; // Check for key advertising every 10 minutes
static const uint32_t GROUTER_ITEM_MAX_CACHE_KEEP_TIME     = 2*86400 ; // Cached items are kept for 48 hours at most.

static const uint32_t RS_GROUTER_DATA_STATUS_UNKNOWN       = 0x0000 ;	// unknown. Unused.
static const uint32_t RS_GROUTER_DATA_STATUS_PENDING       = 0x0001 ;	// item is pending. Should be sent asap.
static const uint32_t RS_GROUTER_DATA_STATUS_SENT          = 0x0002 ;	// item is sent to tunnel or friend. No need to keep sending.
static const uint32_t RS_GROUTER_DATA_STATUS_RECEIPT_OK    = 0x0003 ;	// item is at destination.
static const uint32_t RS_GROUTER_DATA_STATUS_ONGOING       = 0x0004 ;	// transaction is ongoing.
static const uint32_t RS_GROUTER_DATA_STATUS_DONE          = 0x0005 ;	// receipt item has been forward to all routes. We can remove the item.

static const uint32_t RS_GROUTER_SENDING_STATUS_TUNNEL     = 0x0001 ;	// item was sent in a tunnel
static const uint32_t RS_GROUTER_SENDING_STATUS_FRIEND     = 0x0002 ;	// item was sent to a friend

static const uint32_t RS_GROUTER_TUNNEL_STATUS_UNMANAGED   = 0x0000 ; 	// no tunnel requested atm
static const uint32_t RS_GROUTER_TUNNEL_STATUS_PENDING     = 0x0001 ; 	// tunnel requested to turtle
static const uint32_t RS_GROUTER_TUNNEL_STATUS_READY       = 0x0002 ; 	// tunnel is ready but we're still waiting for various confirmations

class FriendTrialRecord
{
	public:
		RsPeerId  friend_id ;			// id of the friend
		rstime_t    time_stamp ;			// time of the last tried
		float     probability ;			// probability at which the item was selected
		uint32_t  nb_friends ;			// number of friends at the time of sending the item
	
		bool serialise(void *data,uint32_t& offset,uint32_t size) const ;
		bool deserialise(void *data,uint32_t& offset,uint32_t size) ;
};

class GRouterRoutingInfo
{
    // There's no destructor to this class, because the memory is managed elsewhere, which
    // ovoids lots of duplications if the class is copied.
public:
    GRouterRoutingInfo()
      : data_status(0)
      , tunnel_status(0)
      , received_time_TS(0)
      , last_sent_TS(0)
      , last_tunnel_request_TS(0)
      , sending_attempts(0)
      , routing_flags(0)
      , data_item(NULL)
      , receipt_item(NULL)
      , data_transaction_TS(0) // this is not serialised.
    {}

    uint32_t data_status ;		// pending, waiting, etc.
    uint32_t tunnel_status ;		// status of tunnel handling.

    rstime_t received_time_TS ;		// time at which the item was originally received
    rstime_t last_sent_TS ;		// last time the item was sent
    rstime_t last_tunnel_request_TS ;	// last time tunnels have been asked for this item.
    uint32_t sending_attempts ;		// number of times tunnels have been asked for this peer without success

    GRouterServiceId client_id ;	// service ID of the client. Only valid when origin==OwnId
    TurtleFileHash tunnel_hash ;	// tunnel hash to be used for this item
    uint32_t routing_flags ;

    RsGRouterGenericDataItem *data_item ;
    RsGRouterSignedReceiptItem *receipt_item ;

    RsTlvPeerIdSet incoming_routes ;
    Sha1CheckSum item_hash ;		// used to check re-existance, consistency, etc.

    // non serialised data

    rstime_t data_transaction_TS ;

    static const uint32_t ROUTING_FLAGS_ALLOW_TUNNELS  = 0x0001;
    static const uint32_t ROUTING_FLAGS_ALLOW_FRIENDS  = 0x0002;
    static const uint32_t ROUTING_FLAGS_IS_ORIGIN      = 0x0004;
    static const uint32_t ROUTING_FLAGS_IS_DESTINATION = 0x0008;
};

