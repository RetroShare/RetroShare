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

typedef uint32_t GRouterServiceId ;
typedef uint32_t GRouterKeyPropagationId ;
typedef uint32_t GRouterMsgPropagationId ;

static const uint32_t RS_GROUTER_MATRIX_MAX_HIT_ENTRIES       = 5;
static const uint32_t RS_GROUTER_MATRIX_MIN_TIME_BETWEEN_HITS = 60;	// can be set to up to half the publish time interval. Prevents flooding routes.

static const time_t RS_GROUTER_DEBUG_OUTPUT_PERIOD       =       20 ; // Output everything
static const time_t RS_GROUTER_AUTOWASH_PERIOD           =       60 ; // Autowash every minute. Not a costly operation.
//static const time_t RS_GROUTER_PUBLISH_CAMPAIGN_PERIOD   =    10*60 ; // Check for key advertising every 10 minutes
//static const time_t RS_GROUTER_PUBLISH_KEY_TIME_INTERVAL = 24*60*60 ; // Advertise each key once a day at most.
static const time_t RS_GROUTER_PUBLISH_CAMPAIGN_PERIOD   =    1 *60 ; // Check for key advertising every 10 minutes
static const time_t RS_GROUTER_PUBLISH_KEY_TIME_INTERVAL =    2 *60 ; // Advertise each key once a day at most.

static const uint32_t RS_GROUTER_ROUTING_STATE_UNKN = 0x0000 ;		// unknown. Unused.
static const uint32_t RS_GROUTER_ROUTING_STATE_PEND = 0x0001 ;		// item is pending. Should be sent asap. 
static const uint32_t RS_GROUTER_ROUTING_STATE_SENT = 0x0002 ;		// item is sent. Waiting for answer
static const uint32_t RS_GROUTER_ROUTING_STATE_OWND = 0x0003 ;		// item is origined here. Needed for retry policy.
