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
#include "retroshare/rsids.h"

typedef GRouterKeyIdType GRouterKeyId ;	// we use SSLIds, so that it's easier in the GUI to mix up peer ids with grouter ids.

class GRouterClientService ;
class RsGRouterGenericDataItem ;

class RsGRouter
{
	public:
		// This is the interface file for the global router service.
		//
		struct GRouterRoutingCacheInfo
		{
			GRouterMsgPropagationId mid ;
			RsPeerId                local_origin;
			GRouterKeyId            destination ;
			time_t                  time_stamp ;
			uint32_t                status ;
			uint32_t                data_size ;
		};

		struct GRouterPublishedKeyInfo
		{
			std::string  description_string ;
			uint32_t     service_id ;
		};

		struct GRouterRoutingMatrixInfo
		{
			// Probabilities of reaching a given key for each friend.
			// This concerns all known keys.
			//
			std::map<GRouterKeyId, std::vector<float> > per_friend_probabilities ;

			// List of friend ids in the same order. Should roughly correspond to the friends that are currently online.
			//
			std::vector<RsPeerId> friend_ids ;

			// List of own published keys, with associated service ID
			//
			std::map<GRouterKeyId,GRouterPublishedKeyInfo> published_keys ;
		};

		//===================================================//
		//                  Debugging info                   //
		//===================================================//

		virtual bool getRoutingCacheInfo(std::vector<GRouterRoutingCacheInfo>& infos) =0; 
		virtual bool getRoutingMatrixInfo(GRouterRoutingMatrixInfo& info) =0; 

		// retrieve the routing probabilities
		
		//===================================================//
		//         Communication to other services.          //
		//===================================================//

		virtual void sendData(const GRouterKeyId& destination, RsGRouterGenericDataItem *item) =0;
		virtual bool registerKey(const GRouterKeyId& key,const GRouterServiceId& client_id,const std::string& description_string) =0;
};

// To access the GRouter from anywhere
//
extern RsGRouter *rsGRouter ;
