/*
 * libretroshare/src/services: rsgrouter.h
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

#include "util/rsdir.h"
#include "grouter/groutertypes.h"

typedef Sha1CheckSum GRouterKeyId ;	// we use sha1. Gives sufficient entropy.
class GRouterClientService ;
class RsGRouterGenericDataItem ;

// This is the interface file for the global router service.
//
struct RoutingCacheInfo
{
	// what do we want to show here?
	// 	- recently routed items
	// 	- ongoing routing info
	// 		- pending items, waiting for an answer
	// 		- 
};

struct RoutingMatrixInfo
{
	// Probabilities of reaching a given key for each friend.
	// This concerns all known keys.
	//
	std::map<GRouterKeyId, std::vector<float> > per_friend_probabilities ;

	// List of own published keys, with associated service ID
	//
	std::map<GRouterKeyId, GRouterClientService *> published_keys ;
};

class RsGRouter
{
	public:
		//===================================================//
		//                  Debugging info                   //
		//===================================================//

		virtual bool getRoutingCacheInfo(RoutingCacheInfo& info) =0; 
		virtual bool getRoutingMatrixInfo(RoutingMatrixInfo& info) =0; 

		// retrieve the routing probabilities
		
		//===================================================//
		//         Communication to other services.          //
		//===================================================//

		virtual void sendData(const GRouterKeyId& destination, RsGRouterGenericDataItem *item) =0;
		virtual bool registerKey(const GRouterKeyId& key,const PGPFingerprintType& fps,const GRouterServiceId& client_id,const std::string& description_string) =0;

};

// To access the GRouter from anywhere
//
extern RsGRouter *rsGRouter ;
