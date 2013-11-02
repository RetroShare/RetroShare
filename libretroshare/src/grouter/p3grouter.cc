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

static const time_t RS_GROUTER_AUTOWASH_PERIOD           =       60 ; // Autowash every minute. Not a costly operation.
static const time_t RS_GROUTER_PUBLISH_CAMPAIGN_PERIOD   =    10*60 ; // Check for key advertising every 10 minutes
static const time_t RS_GROUTER_PUBLISH_KEY_TIME_INTERVAL = 24*60*60 ; // Advertise each key once a day at most.

p3GRouter::p3GRouter(p3LinkMgr *lm)
	: p3Service(RS_SERVICE_TYPE_GROUTER), p3Config(CONFIG_TYPE_GROUTER), mLinkMgr(lm), grMtx("GRouter")
{
	addSerialType(new RsGRouterSerialiser()) ;
}

int p3GRouter::tick()
{
	static time_t last_autowash_time = 0 ;
	static time_t last_publish_campaign_time = 0 ;

	time_t now = time(NULL) ;

	if(now > last_autowash_time + RS_GROUTER_AUTOWASH_PERIOD)
	{
		last_autowash_time = now ;
		autoWash() ;
	}
	// handle incoming items
	// 
	handleIncoming() ;
	
	// route pending objects
	//
	routeObjects() ;

	// advertise published keys
	//
	if(now > last_publish_campaign_time + RS_GROUTER_PUBLISH_CAMPAIGN_PERIOD)
	{
		last_publish_campaign_time = now ;
		publishKeys() ;
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

	std::cerr << "p3GRouter::autoWash() Unimplemented !!" << std::endl;

	// cleanup cache
}

void p3GRouter::routeObjects()
{
	// Go through list of published keys
	// broadcast a publishKeyItem for each of them.

	std::cerr << "p3GRouter::routeObjects() Unimplemented !!" << std::endl;
}
void p3GRouter::publishKeys()
{
	// Go through list of published keys
	// broadcast a publishKeyItem for each of them.

	time_t now = time(NULL) ;
	std::list<std::string> connected_peers ;
	mLinkMgr->getOnlineList(connected_peers) ;

	for(std::map<GRouterKeyId, GRouterPublishedKeyInfo>::iterator it(_owned_key_ids.begin());it!=_owned_key_ids.end();++it)
	{
		GRouterPublishedKeyInfo& info(it->second) ;

		if(now > info.last_published_time + RS_GROUTER_PUBLISH_KEY_TIME_INTERVAL)
		{
			// publish this key

			RsGRouterPublishKeyItem item ;
			item.diffusion_id = RSRandom::random_u32() ;
			item.published_key = it->first ;
			item.service_id = info.service_id ;
			item.randomized_distance = 0 ;
			item.description_string = info.description_string ;

			// get list of connected friends, and broadcast to all of them
			//
			for(std::list<std::string>::const_iterator it(connected_peers.begin());it!=connected_peers.end();++it)
			{
				RsGRouterPublishKeyItem *itm = new RsGRouterPublishKeyItem(item) ;
				itm->PeerId(*it) ;

				// we should randomise the depth

				sendItem(itm) ;
			}
		}
	}
}

bool p3GRouter::registerKey(const GRouterKeyId& key,const GRouterServiceId& client_id,const std::string& description) 
{
	RsStackMutex mtx(grMtx) ;

	GRouterPublishedKeyInfo info ;
	info.service_id = client_id ;
	info.description_string = description;
	info.validity_time = 0 ;			// not used yet.
	info.last_published_time = 0 ; 	// means never published, se it will be re-published soon.

	_owned_key_ids[key] = info ;

	return true ;
}

void p3GRouter::handleIncoming()
{
	RsItem *item ;

	while(NULL != (item = recvItem()))
	{
		switch(item->PacketSubType())
		{
			case RS_PKT_SUBTYPE_GROUTER_PUBLISH_KEY: handleRecvPublishKeyItem(dynamic_cast<RsGRouterPublishKeyItem*>(item)) ;
					   		  		    			 break ;
                             
			case RS_PKT_SUBTYPE_GROUTER_DATA:        handleRecvDataItem(dynamic_cast<RsGRouterGenericDataItem*>(item)) ;
					   		  		    			 break ;
                             
			case RS_PKT_SUBTYPE_GROUTER_ACK:			 handleRecvACKItem(dynamic_cast<RsGRouterACKItem*>(item)) ;
															 break ;
			default:
															 std::cerr << "(EE) " << __PRETTY_FUNCTION__ << ": Unhandled item type " << item->PacketSubType() << std::endl;
		}
		delete item ;
	}
}

void p3GRouter::handleRecvPublishKeyItem(RsGRouterPublishKeyItem *item)
{
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
	bool found = false ;

	if(it != _key_diffusion_time_stamps.end())	// found. We don't propagate further
		found = true ;

	_key_diffusion_time_stamps[item->diffusion_id] = time(NULL) ;	// always stamp

	if(found)
		return ;

	// Propagate the item to all other online friends. We don't do this right now, but push items in a queue.
	// Doing this we can control the amount of key propagation and avoid flooding.

	_key_diffusion_items.push(item) ;
}

void p3GRouter::handleRecvACKItem(RsGRouterACKItem *item)
{
	std::cerr << "Received data item item for key " << item->destination_key.toStdString() << std::endl;

	switch(item->state)
	{
		case RS_GROUTER_ACK_STATE_RECEIVED_INDIRECTLY:				
		case RS_GROUTER_ACK_STATE_RECEIVED:								// received: - do we update the routing matrix? 
			                                                      //           - and forward back
			break ;

		case RS_GROUTER_ACK_STATE_GIVEN_UP:				            // route is bad. We forward back and update the routing matrix.
			break ;
	}
}

void p3GRouter::handleRecvDataItem(RsGRouterGenericDataItem *item)
{
	std::cerr << "Received data item from key " << item->destination_key.toStdString() << std::endl;

	// update the local cache
	// 1 - do we have a sensible route for the item?
	//
	// 	1.1 - select the best guess, send the item
	// 	1.2 - keep track of origin and update list of tried directions
	//
	// 2 - no route. Keep the item for a while. Will be retried later.
}

void p3GRouter::sendData(const GRouterKeyId& destination, void *& item_data,uint32_t item_size) 
{
	std::cerr << "(WW) " << __PRETTY_FUNCTION__ << ": not implemented." << std::endl;
}

bool p3GRouter::loadList(std::list<RsItem*>& items) 
{
	std::cerr << "(WW) " << __PRETTY_FUNCTION__ << ": not implemented." << std::endl;
	return false ;
}
bool p3GRouter::saveList(bool&,std::list<RsItem*>& items) 
{
	std::cerr << "(WW) " << __PRETTY_FUNCTION__ << ": not implemented." << std::endl;
	return false ;
}


