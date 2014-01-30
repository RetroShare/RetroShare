/*
 * libretroshare/src/services: p3grouter.cc
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

#include "util/rsrandom.h"
#include "pqi/p3linkmgr.h"
#include "serialiser/rsconfigitems.h"

#include "p3grouter.h"
#include "grouteritems.h"
#include "groutertypes.h"
#include "grouterclientservice.h"

p3GRouter::p3GRouter(p3LinkMgr *lm)
	: p3Service(RS_SERVICE_TYPE_GROUTER), p3Config(CONFIG_TYPE_GROUTER), mLinkMgr(lm), grMtx("GRouter")
{
	addSerialType(new RsGRouterSerialiser()) ;

	_changed = false ;
}

int p3GRouter::tick()
{
	static time_t last_autowash_time = 0 ;
	static time_t last_publish_campaign_time = 0 ;
	static time_t last_debug_output_time = 0 ;
	static time_t last_config_changed = 0 ;

	time_t now = time(NULL) ;

	if(now > last_autowash_time + RS_GROUTER_AUTOWASH_PERIOD)
	{
		// route pending objects
		//
		routePendingObjects() ;

		last_autowash_time = now ;
		autoWash() ;
	}
	// Handle incoming items
	// 
	handleIncoming() ;
	
	// Advertise published keys
	//
	if(now > last_publish_campaign_time + RS_GROUTER_PUBLISH_CAMPAIGN_PERIOD)
	{
		last_publish_campaign_time = now ;

		publishKeys() ;
		_routing_matrix.updateRoutingProbabilities() ;
	}

	// Debug dump everything
	//
	if(now > last_debug_output_time + RS_GROUTER_DEBUG_OUTPUT_PERIOD)
	{
		last_debug_output_time = now ;
		debugDump() ;
	}

	// If content has changed, save config, at most every RS_GROUTER_MIN_CONFIG_SAVE_PERIOD seconds appart
	// Otherwise, always save at least every RS_GROUTER_MAX_CONFIG_SAVE_PERIOD seconds
	//
	if(_changed && now > last_config_changed + RS_GROUTER_MIN_CONFIG_SAVE_PERIOD)
	{
		std::cerr << "p3GRouter::tick(): triggering config save." << std::endl;

		_changed = false ;
		last_config_changed = now ;
		IndicateConfigChanged() ;
	}

	return 0 ;
}

RsSerialiser *p3GRouter::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser ;

	rss->addSerialType(new RsGRouterSerialiser) ;
	rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss ;
}

void p3GRouter::autoWash()
{
	RsStackMutex mtx(grMtx) ;

	std::cerr << "p3GRouter::autoWash(): cleaning old entried." << std::endl;

	// cleanup cache

	time_t now = time(NULL) ;

	for(std::map<GRouterKeyPropagationId,time_t>::iterator it(_key_diffusion_time_stamps.begin());it!=_key_diffusion_time_stamps.end();)
		if(it->second + RS_GROUTER_KEY_DIFFUSION_MAX_KEEP < now)
		{
			std::cerr << "  Removing key diffusion time stamp " << it->second << " for diffusion id " << std::hex << it->first << std::dec << std::endl;
			std::map<GRouterKeyPropagationId,time_t>::iterator tmp(it) ;
			++tmp ;
			_key_diffusion_time_stamps.erase(it) ;
			it = tmp ;
		}
		else
			++it ;

	// look into pending items.
	
	std::cerr << "  Pending key diffusion items: " << _key_diffusion_items.size() << std::endl;
	std::cerr << "  Pending messages to route  : " << _pending_messages.size() << std::endl;
}

void p3GRouter::routePendingObjects()
{
	RsStackMutex mtx(grMtx) ;

	// Go through list of published keys
	// broadcast a publishKeyItem for each of them.
	//
	// The routing rules are the following:
	//
	//	Go through list of cached routing objects. For each object:
	//   	if(Last try is old)
	//   		put the object in pending list
	//
	//  (This loop is costly (could handle lots of items), so it should be done less often.)
	//
	//		Add peer to the list of tried routes with time stamp
	//		Keep the list of tried friends
	//
	//		Go through list of pendign objects. For each object:
	//			Select one route direction
	//				- according to current probabilities from the routing matrix
	//				- according to list of previous attempts
	//
	//				if(route found)
	//					forward item, update state and time stamp
	//				else
	//					if(I am not the sender)
	//						send back ACK(given up)						// if I am the sender, I will keep trying.
	//    	
	//    Item has received an ACK
	//
	//    	ACK: given up  =>  change route
	//    	ACK: received =>  Remove item from routed items
	//    	
	//    The list in _pending_messages is necessarily short and most of the time empty. Once
	//    treated, objects are stored in _routing_cache, where they wait for an answer.

	time_t now = time(NULL) ;

	std::cerr << "p3GRouter::routeObjects() triage phase:" << std::endl;
	std::cerr << "Cached Items : " << _pending_messages.size() << std::endl;

	std::list<std::string> lst_tmp ;
	std::list<SSLIdType> lst ;
	mLinkMgr->getOnlineList(lst_tmp) ;
	SSLIdType own_id( mLinkMgr->getOwnId() );

	for(std::list<std::string>::const_iterator it(lst_tmp.begin());it!=lst_tmp.end();++it)
		lst.push_back(SSLIdType(*it)) ;

	for(std::map<GRouterMsgPropagationId, GRouterRoutingInfo>::iterator it(_pending_messages.begin());it!=_pending_messages.end();)
		if((it->second.status_flags & RS_GROUTER_ROUTING_STATE_PEND) || (it->second.status_flags == RS_GROUTER_ROUTING_STATE_SENT && it->second.tried_friends.front().time_stamp+RS_GROUTER_ROUTING_WAITING_TIME < now))
		{
			std::cerr << "  Msg id: " << std::hex << it->first << std::dec << std::endl;
			std::cerr << "  Origin: " << it->second.origin.toStdString() << std::endl;
			std::cerr << "  Last  : " << it->second.tried_friends.front().friend_id.toStdString() << std::endl;
			std::cerr << "  Time  : " << it->second.tried_friends.front().time_stamp << std::endl;
			std::cerr << "  Flags : " << it->second.status_flags << std::endl;
			std::cerr << "  Probabilities: " << std::endl;

			std::map<SSLIdType,float> probas ;		// friends probabilities for online friend list.
			SSLIdType routed_friend ;					// friend chosen for the next hop
			float best_proba = 0.0f;					// temp variable used to select the best proba
			bool should_remove = false ;				// should we remove this from the map?

			// retrieve probabilities for this key.
			//
			if(! _routing_matrix.computeRoutingProbabilities(it->second.data_item->destination_key, lst, probas))
			{
				// key does not exist in routing matrix => send back an ACK(unknown)

				std::cerr << "    [Cannot compute. Unknown destination key!!] " << std::endl;

				if(it->second.origin != own_id)
				{
					std::cerr << "      removing item and notifying the sender (" << it->second.origin.toStdString() << ")" << std::endl;

					sendACK(it->second.origin,it->first,RS_GROUTER_ACK_STATE_UNKNOWN) ;

					// remove item from cache
					//
					should_remove = true ;
				}
				std::cerr << "      item is ours. Keeping it until a route is known." << std::endl;

				// else, select a routing friend at random, or just wait? Wait is probably better.
			}

			bool friend_found = false ;

			for(std::map<SSLIdType,float>::const_iterator it2(probas.begin());it2!=probas.end();++it2)
			{
				std::cerr << "     " << it2->first.toStdString() << " : " << it2->second << std::endl;

				// select the peer with highest probability that hasn't yet been tried.

				if(it2->second > best_proba && !(it2->first == it->second.tried_friends.front().friend_id))
				{
					routed_friend = it2->first ;
					best_proba = it2->second ;
					friend_found = true ;
				}
			}

			std::cerr << "  Best route: " << routed_friend.toStdString() << ", with probability " << best_proba << std::endl;

			// now, send the item.

			if(friend_found)
			{
				// make a deep copy of the item
				RsGRouterGenericDataItem *new_item = it->second.data_item->duplicate() ;

				// update cache entry
				FriendTrialRecord ftr ;
				ftr.time_stamp = now ;
				ftr.friend_id = routed_friend ;
				it->second.tried_friends.push_front(ftr) ;
				it->second.status_flags = RS_GROUTER_ROUTING_STATE_SENT ;

				std::cerr << "  Sending..." << std::endl;
				// send
				new_item->PeerId(routed_friend.toStdString()) ;
				sendItem(new_item) ;
			}
			else if(it->second.origin.toStdString() != mLinkMgr->getOwnId() || std::find(lst.begin(),lst.end(),it->second.origin) != lst.end())
			{
				// There's no correct friend to send this item to. We keep it for a while. If it's too old,
				// we discard it. For now, the procedure is to send back an ACK.

				std::cerr << "  Item has no route candidate. It's too old. " << std::endl;
				std::cerr << "  sending ACK(no route) to peer " << it->second.origin.toStdString() << std::endl;

				sendACK(it->second.origin,it->first,RS_GROUTER_ACK_STATE_NO_ROUTE) ;

				should_remove = true ;
			}

			if(should_remove)
			{
				// We remove from the map. That means the RsItem* has been transfered to somewhere else.
				//
				std::cerr << "  Removing item from pending items" << std::endl;

				std::map<GRouterMsgPropagationId, GRouterRoutingInfo>::iterator tmp(it) ;
				delete it->second.data_item ;
				++tmp ;
				_pending_messages.erase(it) ;
				it = tmp ;
			}
			else
				++it ;
		}
		else
			std::cerr << "Skipping " << std::hex << it->first << std::dec << ", dest=" << it->second.data_item->destination_key.toStdString() << ", state = " << it->second.status_flags << ", stamp=" << it->second.tried_friends.front().time_stamp << " - " << it->second.tried_friends.front().friend_id.toStdString() << std::endl;
}

void p3GRouter::publishKeys()
{
	RsStackMutex mtx(grMtx) ;

	// Go through list of published keys
	// broadcast a publishKeyItem for each of them.

	time_t now = time(NULL) ;

	for(std::map<GRouterKeyId, GRouterPublishedKeyInfo>::iterator it(_owned_key_ids.begin());it!=_owned_key_ids.end();++it)
	{
		GRouterPublishedKeyInfo& info(it->second) ;

		if(now > info.last_published_time + RS_GROUTER_PUBLISH_KEY_TIME_INTERVAL)
		{
			// publish this key

			std::cerr << "Publishing this key: " << std::endl;
			std::cerr << "   Key id          : " << it->first.toStdString() << std::endl;
			std::cerr << "   Service id      : " << std::hex << info.service_id << std::dec << std::endl;
			std::cerr << "   Description     : " << info.description_string << std::endl;
			std::cerr << "   Fingerprint     : " << info.fpr.toStdString() << std::endl;

			RsGRouterPublishKeyItem item ;
			item.diffusion_id = RSRandom::random_u32() ;
			item.published_key = it->first ;
			item.service_id = info.service_id ;
			item.randomized_distance = drand48() ;
			item.fingerprint = info.fpr;
			item.description_string = info.description_string ;
			item.PeerId("") ;	// no peer id => key is forwarded to all friends.

			locked_forwardKey(item) ;


			info.last_published_time = now ;
		}
	}
}

void p3GRouter::locked_forwardKey(const RsGRouterPublishKeyItem& item) 
{
	std::list<std::string> connected_peers ;
	mLinkMgr->getOnlineList(connected_peers) ;

	std::cerr << "  Forwarding key item to all available friends..." << std::endl;

	// get list of connected friends, and broadcast to all of them
	//
	for(std::list<std::string>::const_iterator it(connected_peers.begin());it!=connected_peers.end();++it)
		if(item.PeerId() != *it)
		{
			std::cerr << "    sending to " << (*it) << std::endl; 

			RsGRouterPublishKeyItem *itm = new RsGRouterPublishKeyItem(item) ;
			itm->PeerId(*it) ;

			// we should randomise the depth

			sendItem(itm) ;
		}
		else
			std::cerr << "    Not forwarding to source id " << item.PeerId() << std::endl;
}
bool p3GRouter::registerKey(const GRouterKeyId& key,const PGPFingerprintType& fps,const GRouterServiceId& client_id,const std::string& description) 
{
	RsStackMutex mtx(grMtx) ;

	GRouterPublishedKeyInfo info ;
	info.service_id = client_id ;
	info.fpr = fps ;
	info.description_string = description.substr(0,20);
	info.validity_time = 0 ;			// not used yet.
	info.last_published_time = 0 ; 	// means never published, se it will be re-published soon.

	_owned_key_ids[key] = info ;

	std::cerr << "Registered the following key: " << std::endl;
	std::cerr << "   Key id      : " << key.toStdString() << std::endl;
	std::cerr << "   Client id   : " << std::hex << client_id << std::dec << std::endl;
	std::cerr << "   Description : " << info.description_string << std::endl;

	return true ;
}
bool p3GRouter::unregisterKey(const GRouterKeyId& key)
{
	RsStackMutex mtx(grMtx) ;

	std::map<GRouterKeyId,GRouterPublishedKeyInfo>::iterator it = _owned_key_ids.find(key) ;

	if(it == _owned_key_ids.end())
	{
		std::cerr << "p3GRouter::unregisterKey(): key " << key.toStdString() << " not found." << std::endl;
		return false ;
	}

	std::cerr << "p3GRouter::unregistered the following key: " << std::endl;
	std::cerr << "   Key id      : " << key.toStdString() << std::endl;
	std::cerr << "   Client id   : " << std::hex << it->second.service_id << std::dec << std::endl;
	std::cerr << "   Description : " << it->second.description_string << std::endl;

	_owned_key_ids.erase(it) ;

	return true ;
}

void p3GRouter::handleIncoming()
{
	RsItem *item ;

	while(NULL != (item = recvItem()))
	{
		switch(item->PacketSubType())
		{
			case RS_PKT_SUBTYPE_GROUTER_PUBLISH_KEY: 	handleRecvPublishKeyItem(dynamic_cast<RsGRouterPublishKeyItem*>(item)) ;
																	break ;
                             
			case RS_PKT_SUBTYPE_GROUTER_DATA:        	handleRecvDataItem(dynamic_cast<RsGRouterGenericDataItem*>(item)) ;
																	break ;
                             
			case RS_PKT_SUBTYPE_GROUTER_ACK:			 	handleRecvACKItem(dynamic_cast<RsGRouterACKItem*>(item)) ;
																	break ;
			default:
															 std::cerr << "(EE) " << __PRETTY_FUNCTION__ << ": Unhandled item type " << item->PacketSubType() << std::endl;
		}
		delete item ;
	}
}

void p3GRouter::handleRecvPublishKeyItem(RsGRouterPublishKeyItem *item)
{
	RsStackMutex mtx(grMtx) ;

	std::cerr << "Received key publish item for key :" << std::endl ;
	std::cerr << "     diffusion  = " << std::hex << item->diffusion_id << std::dec << std::endl ;
	std::cerr << "     key id     = " << item->published_key.toStdString() << std::endl ;
	std::cerr << "     service id = " << std::hex << item->service_id << std::dec << std::endl;
	std::cerr << "     distance   = " << item->randomized_distance << std::endl;
	std::cerr << "     description= " << item->description_string << std::endl;

	// update the route matrix

	_routing_matrix.addRoutingClue(item->published_key,item->service_id,item->randomized_distance,item->description_string,SSLIdType(item->PeerId())) ;

	// forward the key to other peers according to key forwarding cache
	
	std::map<GRouterKeyPropagationId,time_t>::iterator it = _key_diffusion_time_stamps.find(item->diffusion_id) ;

	bool found = (it != _key_diffusion_time_stamps.end());	// found. We don't propagate further

	_key_diffusion_time_stamps[item->diffusion_id] = time(NULL) ;	// always stamp

	if(found)
	{
		std::cerr << "  Key diffusion item already in cache. Not forwardign further." << std::endl;
		return ;
	}

	// Propagate the item to all other online friends. We don't do this right now, but push items in a queue.
	// Doing this we can control the amount of key propagation and avoid flooding.

	locked_forwardKey(*item) ;

	_changed = true ;
}

void p3GRouter::handleRecvACKItem(RsGRouterACKItem *item)
{
	RsStackMutex mtx(grMtx) ;
	std::cerr << "Received ACK item, mid=" << std::hex << item->mid << std::dec << std::endl;

	// find the item in the pendign list, 
	// 	- if not found, drop.
	//
	// ...and act appropriately:
	// 	- item was 
	// 	- if we're origin 
	// 			notify the client service
	// 		else
	// 		 	remove item
	//
	std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator it(_pending_messages.find(item->mid)) ;

	if(it == _pending_messages.end())
	{
		std::cerr << "p3GRouter::handleRecvACKItem(): cannot find entry for message id " << std::hex << item->mid << std::dec << ". Dropping it." << std::endl;
		return ;
	}
	switch(item->state)
	{
		case RS_GROUTER_ACK_STATE_RECEIVED_INDIRECTLY:				
			// Do nothing. It was indirectly received: fine. We don't need to notify the origin
			// otherwise lots of ACKs will flow to the origin.
			break ;
		case RS_GROUTER_ACK_STATE_RECEIVED:								
			// Notify the origin. This is the main route and it was successful.
																				
			std::cerr << "  forwarding ACK to origin: " << it->second.origin.toStdString() << std::endl;

			sendACK(it->second.origin,item->mid,RS_GROUTER_ACK_STATE_RECEIVED) ;
			break ;

		case RS_GROUTER_ACK_STATE_GIVEN_UP:				            // route is bad. We forward back and update the routing matrix.
			break ;
	}

	if(it->second.origin.toStdString() == mLinkMgr->getOwnId())
	{
		// find the client service and notify it.
		std::cerr << "  We're owner: should notify client id" << std::endl;
	}

	// Always remove the item.
	//
	delete it->second.data_item ;
	_pending_messages.erase(it) ;
}

void p3GRouter::handleRecvDataItem(RsGRouterGenericDataItem *item)
{
	RsStackMutex mtx(grMtx) ;
	std::cerr << "Received data item for key " << item->destination_key.toStdString() << std::endl;

	// Do we have this item in the cache already?
	//   - if not, add in the pending items
	//   - if yet. Ignore, or send ACK for shorter route.
	
	std::map<GRouterKeyId,GRouterPublishedKeyInfo>::const_iterator it = _owned_key_ids.find(item->destination_key) ;
	std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::const_iterator itr = _pending_messages.find(item->routing_id) ;
	RsGRouterGenericDataItem *item_copy = NULL;

	if(itr != _pending_messages.end())
	{
		std::cerr << "  Item is already there. Nothing to do. Should we update the cache?" << std::endl;

		item_copy = itr->second.data_item ;
	}
	else		// item is now known. Store it into pending msgs. We make a copy, since the item will be deleted otherwise.
	{
		std::cerr << "  Item is new. Storing in cache as pending messages." << std::endl;

		GRouterRoutingInfo info ;

		info.data_item = item->duplicate() ;
		item_copy = info.data_item ;

		if(it != _owned_key_ids.end())
			info.status_flags = RS_GROUTER_ROUTING_STATE_ARVD ;
		else
			info.status_flags = RS_GROUTER_ROUTING_STATE_PEND ;

		info.origin = SSLIdType(item->PeerId()) ;
		info.received_time = time(NULL) ;

		_pending_messages[item->routing_id] = info ;
	}

	// Is the item for us? If so, find the client service and send the item back.

	if(it != _owned_key_ids.end())
		if(time(NULL) < it->second.validity_time) 
		{
			// test validity time. If too old, we don't forward.

			std::map<GRouterServiceId,GRouterClientService*>::const_iterator its = _registered_services.find(it->second.service_id) ;

			if(its != _registered_services.end())
			{
				std::cerr << "  Key is owned by us. Notifying service for this item." << std::endl;
				its->second->receiveGRouterData(item_copy,it->first) ;
			}
			else
				std::cerr << "  (EE) weird situation. No service registered for a key that we own. Key id = " << item->destination_key.toStdString() << ", service id = " << it->second.service_id << std::endl;
		}
		else
			std::cerr << "  (WW) key is outdated. Dropping this item." << std::endl;
	else
		std::cerr << "  Item is not for us. Leaving in pending msgs to be routed later." << std::endl;

	_changed = true ;
}

bool p3GRouter::registerClientService(const GRouterServiceId& id,GRouterClientService *service)
{
	RsStackMutex mtx(grMtx) ;
	_registered_services[id] = service ;
	return true ;
}

void p3GRouter::sendData(const GRouterKeyId& destination, RsGRouterGenericDataItem *item)
{
	RsStackMutex mtx(grMtx) ;
	// push the item into pending messages.
	//
	GRouterRoutingInfo info ;

	info.data_item = item ;
	info.status_flags = RS_GROUTER_ROUTING_STATE_PEND ;
	info.origin = SSLIdType(mLinkMgr->getOwnId()) ;
	info.received_time = time(NULL) ;
	
	// Make sure we have a unique id (at least locally).
	//
	GRouterMsgPropagationId propagation_id ;
	do { propagation_id = RSRandom::random_u32(); } while(_pending_messages.find(propagation_id) != _pending_messages.end()) ;

	std::cerr << "p3GRouter::sendGRouterData(): pushing the followign item in the msg pending list:" << std::endl;
	std::cerr << "  data_item.size = " << info.data_item->data_size << std::endl;
	std::cerr << "  data_item.byte = " << info.data_item->data_bytes << std::endl;
	std::cerr << "  status         = " << info.status_flags << std::endl;
	std::cerr << "  origin         = " << info.origin.toStdString() << std::endl;
	std::cerr << "  Recv time      = " << info.received_time << std::endl;

	_pending_messages[propagation_id] = info ;
}

void p3GRouter::sendACK(const SSLIdType& peer, GRouterMsgPropagationId mid, uint32_t ack_flags)
{
	RsGRouterACKItem *item = new RsGRouterACKItem ;

	item->state = ack_flags ;
	item->mid = mid ;
	item->PeerId(peer.toStdString()) ;

	sendItem(item) ;
}

bool p3GRouter::loadList(std::list<RsItem*>& items) 
{
	RsStackMutex mtx(grMtx) ;

	std::cerr << "p3GRouter::loadList() : " << std::endl;

	_routing_matrix.loadList(items) ;

	// remove all existing objects.
	//
	std::cerr << "  removing all existing items (" << _pending_messages.size() << " items to delete)." << std::endl;

	for(std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator it(_pending_messages.begin());it!=_pending_messages.end();++it)
		delete it->second.data_item ;
	_pending_messages.clear() ;

	for(std::list<RsItem*>::const_iterator it(items.begin());it!=items.end();++it)
	{
		RsGRouterRoutingInfoItem *itm1 = NULL ;

		if(NULL != (itm1 = dynamic_cast<RsGRouterRoutingInfoItem*>(*it)))
		{
			_pending_messages[itm1->data_item->routing_id] = *itm1 ;	
			_pending_messages[itm1->data_item->routing_id].data_item = itm1->data_item ;	// avoids duplication.

			itm1->data_item = NULL ;	// prevents deletion.
		}

		delete *it ;
	}
	return true ;
}
bool p3GRouter::saveList(bool& cleanup,std::list<RsItem*>& items) 
{
	// We save
	// 	- the routing clues
	// 	- the pending items

	cleanup = true ; // the client should delete the items.

	std::cerr << "p3GRouter::saveList()..." << std::endl;
	std::cerr << "  saving routing clues." << std::endl;

	_routing_matrix.saveList(items) ;

	std::cerr << "  saving pending items." << std::endl;

	for(std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::const_iterator it(_pending_messages.begin());it!=_pending_messages.end();++it)
	{
		RsGRouterRoutingInfoItem *item = new RsGRouterRoutingInfoItem ;
		
		*(GRouterRoutingInfo*)item = it->second ;	// copy all members

		item->data_item = it->second.data_item->duplicate() ;	// deep copy, because we call delete on the object, and the item might be removed before we handle it in the client.

		items.push_back(item) ;
	}

	return true ;
}

// Dump everything
//
void p3GRouter::debugDump()
{
	RsStackMutex mtx(grMtx) ;

	time_t now = time(NULL) ;

	std::cerr << "Full dump of Global Router state: " << std::endl; 
	std::cerr << "  Owned keys : " << std::endl;

	for(std::map<GRouterKeyId, GRouterPublishedKeyInfo>::const_iterator it(_owned_key_ids.begin());it!=_owned_key_ids.end();++it)
	{
		std::cerr << "    Key id          : " << it->first.toStdString() << std::endl;
		std::cerr << "      Service id    : " << std::hex << it->second.service_id << std::dec << std::endl;
		std::cerr << "      Description   : " << it->second.description_string << std::endl;
		std::cerr << "      Last published: " << now - it->second.last_published_time << " secs ago" << std::endl;
	}

	std::cerr << "  Registered services: " << std::endl;

	for(std::map<GRouterServiceId,GRouterClientService *>::const_iterator it(_registered_services.begin() );it!=_registered_services.end();++it)
		std::cerr << "    " << std::hex << it->first << "   " << std::dec << (void*)it->second << std::endl;

	std::cerr << "  Key diffusion cache: " << std::endl;

	for(std::map<GRouterKeyPropagationId,time_t>::const_iterator it(_key_diffusion_time_stamps.begin() );it!=_key_diffusion_time_stamps.end();++it)
		std::cerr << "    " << std::hex << it->first << "   " << std::dec << now - it->second << " secs ago" << std::endl;

	std::cerr << "  Key diffusion items: " << std::endl;
	std::cerr << "    [Not shown yet]    " << std::endl;

	std::cerr << "  Data items: " << std::endl;
	std::cerr << "    [Not shown yet]    " << std::endl;

	std::cerr << "  Routing matrix: " << std::endl;

	_routing_matrix.debugDump() ;
}




