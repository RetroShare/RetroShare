/*
 * libretroshare/src/services: p3grouter.h
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

#include <map>
#include <queue>

#include "rsgrouter.h"
#include "services/p3service.h"
#include "pqi/p3cfgmgr.h"
#include "retroshare/rstypes.h"

#include "groutertypes.h"
#include "groutermatrix.h"
#include "groutercache.h"
#include "grouteritems.h"

// To be put in pqi/p3cfgmgr.h
//
static const uint32_t CONFIG_TYPE_GROUTER = 0x0016 ;

class p3LinkMgr ;
class RsGRouterPublishKeyItem ;
class RsGRouterACKItem ;

class p3GRouter: public RsGRouter, public p3Service, public p3Config
{
	public:
		p3GRouter(p3LinkMgr *lm) ;

		//===================================================//
		//         Router clients business                   //
		//===================================================//

		// This method allows to associate client ids (that are saved to disk) to client objects deriving 
		// from GRouterClientService. The various services are responsible for regstering themselves to the
		// global router, with consistent ids. The services are stored in a map, and arriving objects are
		// passed on the correct service depending on the client id of the key they are reaching.
		//
		bool registerClientService(const GRouterServiceId& id,GRouterClientService *service) ;

		// Use this method to register/unregister a key that the global router will
		// forward in the network, so that is can be a possible destination for
		// global messages.
		//
		// 	key		: 		The key that is published
		// 	fingerp  :     Fingerprint of the key to encrypt the data.
		// 	desc_str :     Any fixed length string (< 20 characters) to descript the address in words.
		// 	client_id:		id of the client service to send the traffic to.
		// 	               To obtain a client id, the service must register using the previous method.
		//
		// Unregistering a key might not have an instantaneous effect, so the client is responsible for 
		// discarding traffic that might later come for this key.
		//
		bool registerKey(const GRouterKeyId& key,const PGPFingerprintType& pgp_fingerprint,
								const GRouterServiceId& client_id,const std::string& description_string) ;

		bool unregisterKey(const GRouterKeyId& key) ;

		//===================================================//
		//         Client/server request services            //
		//===================================================//

		// Sends an item to the given destination.  The router takes ownership of
		// the memory. That means item_data will be erase on return.
		//
		void sendData(const GRouterKeyId& destination, RsGRouterGenericDataItem *item) ;

		// Sends an ACK to the origin of the msg. This is used to notify for 
		// unfound route, or message correctly received, depending on the particular situation.
		//
		void sendACK(const RsPeerId& peer,GRouterMsgPropagationId mid, uint32_t flags) ;

		//===================================================//
		//                  Interface with RsGRouter         //
		//===================================================//

		// debug info from routing matrix
		// 	- list of known key ids
		// 	- list of clues/time_stamp for each key.
		// 	- real time routing probabilities
		//
		virtual bool getRoutingMatrixInfo(RoutingMatrixInfo& info) { return false ;}

		// debug info from routing cache
		// 	- Cache Items
		// 		* which message ids
		// 		* directions
		// 		* timestamp
		// 		* message type
		// 	- Cache state (memory size, etc)
		//
		virtual bool getRoutingCacheInfo(RoutingCacheInfo& info) { return false ;} 

	protected:
		//===================================================//
		//         Routing method handling                   //
		//===================================================//

		// Calls 
		// 	- autoWash()
		// 	- packet handling methods 
		// 	- matrix updates
		//
		virtual int tick() ; 

	private:
		void autoWash() ;
		void routePendingObjects() ;
		void handleIncoming() ;
		void publishKeys() ;
		void debugDump() ;
		void locked_forwardKey(const RsGRouterPublishKeyItem&) ;

		//===================================================//
		//                  p3Config methods                 //
		//===================================================//

		// Load/save the routing info, the pending items in transit, and the config variables.
		//
		virtual bool loadList(std::list<RsItem*>& items) ;
		virtual bool saveList(bool& cleanup,std::list<RsItem*>& items) ;

		virtual RsSerialiser *setupSerialiser() ;

		//===================================================//
		//                  Debug methods                    //
		//===================================================//

		// Prints the internal state of the router, for debug purpose.
		//
		void debug_dump() ;

		// Stores the routing info
		// 	- list of known key ids
		// 	- list of clues/time_stamp for each key.
		// 	- real time routing probabilities
		//
		GRouterMatrix _routing_matrix ;

		// Stores the routing events.
		// 	- ongoing requests, waiting for return ACK
		// 	- pending items
		// Both a stored in 2 different lists, to allow a more efficient handling.
		//
		std::map<GRouterMsgPropagationId, GRouterRoutingInfo> _pending_messages;// pending messages

		// Stores the keys which identify the router's node. For each key, a structure holds:
		// 	- the client service
		// 	- flags
		// 	- usage time stamps
		//
		std::map<GRouterKeyId, GRouterPublishedKeyInfo> _owned_key_ids ;

		// Key publish cache and buffers
		//   Handles key publish items routes and forwarding info.
		//
		//  1 - timestamps of diffused keys received stored by diffusion id. 
		std::map<GRouterKeyPropagationId,time_t> _key_diffusion_time_stamps ;

		//  2 - list of key diffusion items to be routed. These are stored in a priority structure
		//      where the priority is based on key distance, so that:
		//      	- long distance keys get propagated less easily
		//      	- when the list exceeds the maximum allowed size, items with the largest distance get dropped.
		//
		std::priority_queue<RsGRouterPublishKeyItem *> _key_diffusion_items ;

		// Registered services. These are known to the different peers with a common id,
		// so it's important to keep consistency here. This map is volatile, and re-created at each startup of
		// the software, when newly created services register themselves.

		std::map<GRouterServiceId,GRouterClientService *> _registered_services ;

		// Data handling ethods
		//
		void handleRecvPublishKeyItem(RsGRouterPublishKeyItem *item) ;
		void handleRecvDataItem(RsGRouterGenericDataItem *item);
		void handleRecvACKItem(RsGRouterACKItem *item);

		// Pointers to other RS objects
		//
		p3LinkMgr *mLinkMgr ;

		// Multi-thread protection mutex.
		//
		RsMutex grMtx ;

		// config update/save variables
		bool _changed ;
};


