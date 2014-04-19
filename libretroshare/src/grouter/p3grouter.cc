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

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Decentralized routing
// =====================
// 
// Use cases:
//    - Peer A asks for B's key, for which he has the signature, or the ID.
//    - Peer A wants to send a private msg to peer C, for which he has the public key
//    - Peer A wants to contact a channel's owner, a group owner, a forum owner, etc.
//    - Peer C needs to route msg/key requests from unknown peer, to unknown peer so that the information
//       eventually reach their destination.
// 
// Main idea: Each peer holds a local routing table, a matrix with probabilities that each friend
// is a correct path for a given key ID.
// 
// The routing tables are updated as messages go back and forth. Successful
// interactions feed the routing table with information of where to route the
// packets.
// 
// The routing is kept probabilistic, meaning that the optimal route is not
// always chosen, but the randomness helps updating the routing probabilities.
// 
// Services that might use the router (All services really...)
//     - Identity manager (p3Identity)
//        - asks identities i.e. RSA public keys (i.e. sends dentity requests through router)
//     - Messenger
//        - sends/receives messages to distant peers
//     - Channels, forums, posted, etc.
//        - send messages to the origin of the channel/forum/posted
// 
// GUI
//    - a debug panel should show the routing info: probabilities for all known IDs
//    - routing probabilities for a given ID accordign to who's connected
// 
// Decentralized routing algorithm:
//    - tick() method
//       * calls autoWash(), send() and receive()
// 
//    - message passing
//       - upward: 
//          * Forward msg to friends according to probabilities.
//          * If all equal, send to all friends (or a rando subset of them).
//          * keep the local routing info in a cache that is saved (Which peer issued the msg)
//             - which probability was used to chose this friend (will be useful
//               to compute the routing contribution if the msg is ACK-ed)
//
//          Two probabilities are computed:
//          	- routing probabilities among connected friends
//          		* this is computed by the routing matrix
//          	- branching factor N
//          		* depends on the depth of the items
//          		* depends on the distribution of probabilities (min and max)
//
//          Once computed, 
//          	- the item is forwarded randomly to N peers drawn from the list of connected peers with the given probabilities.
//          	- the depth of the item is incremented randomly
// 
//       - downward: look into routing cache. If info not present, drop the item.
//         Forward item into stored direction.
// 
//       - routing probability computation: count number of times a reliable info is obtained from
//         which direction for which identity
//          * the count is a floating point number, since weights can be assigned to each info
//            (especially for importance sampling)
//          * init: all friends have equal count of 0 (or 1, well, we'll have to make this right).
//          * We use importance sampling, meaning that when peer relays a msg from ID:
//                 count[ID, peer] += 1.0 / importance
// 
//              ... where importance was the probability of chosing peer for the
//              route upward.
// 
//          * probability of forward is proportional to count.
// 
//    - routing cache
//       * this cache stores messages IDs (like turtle router) but is saved on disk
//       * it is used to remember where to send back responses to messages, and
//         with what probability the route was chosen.
//       * cache items have a TTL and the cache is cleaned regularly.
// 
//    - routing matrix
//       * the structure is fed by other services, when they receive key IDs.
//       * stores for each identity the count of how many times each peer gave reliable info for that ID.
//        That information should be enough to route packets in the correct direction. 
//       * saved to disk.
//       * all contributions should have a time stamp. Regularly, the oldest contributions are removed.
// 
//          struct RoutingMatrixHitEntry
//          {
//             float weight ;
//             time_t time_stamp ;
//          }
//          typedef std::map<std::string,std::list<RoutingMatrixHitEntry> > RSAKeyRoutingMap ;
// 
//          class RoutingMatrix
//          {
//             public:
//                // Computes the routing probabilities for this id  for the given list of friends.
//                // the computation accounts for the time at which the info was received and the
//                // weight of each routing hit record.
//                //
//                bool computeRoutingProbabilities(RSAKeyIDType id, const std::vector<SSLIdType>& friends,
//                                                 std::vector<float>& probas) const ;
// 
//                // remove oldest entries.
//                bool autoWash() ;
// 
//                // Record one routing clue. The events can possibly be merged in time buckets.
//                //
//                bool addRoutingEvent(RSAKeyIDType id,const SSLIdType& which friend) ;
// 
//             private:
//                std::map<RSAKeyIDType, RSAKeyRoutingMap> _known_keys ;
//          };
// 
//    - Routed packets: we use a common packet type for all services:
// 
//       We need two abstract item types:
// 
//          * Data packet
//             - packet unique ID (sha1, or uint64_t)
//             - destination ID (for Dn packets, the destination is the source!)
//             - packet type: Id request, Message, etc.
//             - packet service ID (Can be messenging, channels, etc).
//             - packet data (void* + size_t)
//             - flags (such as ACK or response required, and packet direction)
//             - routed directions and probabilities
//          * ACK packet.
//             - packet unique ID (the id of the corresponding data)
//             - flags (reason for ACK. Could be data delivered, or error, too far, etc)
// 
//    - Data storage packets
//       * We need storage packets for the matrix states.
//       * General routing options info?
// 
//    - estimated memory cost
//       For each identity, the matrix needs
//          - hits for each friend peer with time stamps. That means 8 bytes per hit.
//             That is for 1000 identities, having at most 100 hits each (We keep
//             the hits below a maximum. 100 seems ok.), that is 1000*100*8 < 1MB. Not much.
// 
//    - Main difficulties:
//       * have a good re-try strategy if a msg does not arrive.
//       * handle peer availability. In forward mode: easy. In backward mode:
//         difficult. We should wait, and send back the packet if possible.
//       * robustness
//       * security: avoid flooding, and message alteration.
// 
// 	- Questions to be solved
// 		* how do we talk to other services?
// 			- keep a list of services?
// 
// 			- in practice, services will need to send requests, and expect responses.
// 				* gxs (p3identity) asks for a key, gxs (p3identity) should get the key.
// 				* msg service wants to send a distant msg, or msg receives a distant msg.
// 
// 				=> we need abstract packets and service ids.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <math.h>

#include "util/rsrandom.h"
#include "pqi/p3linkmgr.h"
#include "serialiser/rsconfigitems.h"

#include "p3grouter.h"
#include "grouteritems.h"
#include "groutertypes.h"
#include "grouterclientservice.h"

/**********************/
#define GROUTER_DEBUG
/**********************/

const std::string p3GRouter::SERVICE_INFO_APP_NAME = "Global Router" ;

p3GRouter::p3GRouter(p3ServiceControl *sc,p3LinkMgr *lm)
	: p3Service(), p3Config(), mServiceControl(sc), mLinkMgr(lm), grMtx("GRouter")
{
	addSerialType(new RsGRouterSerialiser()) ;

	_last_autowash_time = 0 ;
	_last_debug_output_time = 0 ;
	_last_config_changed = 0 ;
	_last_matrix_update_time = 0 ;

	_random_salt = RSRandom::random_u64() ;

	_changed = false ;
}

int p3GRouter::tick()
{
	time_t now = time(NULL) ;

	if(now > _last_autowash_time + RS_GROUTER_AUTOWASH_PERIOD)
	{
		// route pending objects
		//
		routePendingObjects() ;

		_last_autowash_time = now ;
		autoWash() ;
	}
	// Handle incoming items
	// 
	handleIncoming() ;
	
	// Update routing matrix
	//
	if(now > _last_matrix_update_time + RS_GROUTER_MATRIX_UPDATE_PERIOD)
	{
		RsStackMutex mtx(grMtx) ;

		_last_matrix_update_time = now ;
		_routing_matrix.updateRoutingProbabilities() ;		// This should be locked.
	}

#ifdef GROUTER_DEBUG
	// Debug dump everything
	//
	if(now > _last_debug_output_time + RS_GROUTER_DEBUG_OUTPUT_PERIOD)
	{
		_last_debug_output_time = now ;
		if(_debug_enabled)
			debugDump() ;
	}
#endif

	// If content has changed, save config, at most every RS_GROUTER_MIN_CONFIG_SAVE_PERIOD seconds appart
	// Otherwise, always save at least every RS_GROUTER_MAX_CONFIG_SAVE_PERIOD seconds
	//
	if(_changed && now > _last_config_changed + RS_GROUTER_MIN_CONFIG_SAVE_PERIOD)
	{
#ifdef GROUTER_DEBUG
		grouter_debug() << "p3GRouter::tick(): triggering config save." << std::endl;
#endif

		_changed = false ;
		_last_config_changed = now ;
		IndicateConfigChanged() ;
	}

	return 0 ;
}

time_t p3GRouter::computeNextTimeDelay(time_t stored_time)
{
	// Computes the time to wait before re-sending the object, based on how long it has been stored already.
	
	if(stored_time <       2*60  )  return      10 ;	// re-schedule every 10 secs for items not older than 2 mins. This ensures a rapid spread when peers are online.
	if(stored_time <      40*60  )  return 10 * 60 ;	// then, try every 10 mins for 40 mins
	if(stored_time <       4*3600)  return    3600 ;	// then, try every hour for 4 hours
	if(stored_time <   10*24*3600)  return 12*3600 ;	// then, try every 12 hours for 10 days
	if(stored_time < 6*30*24*3600)  return 5*86400 ;	// then, try every 5 days for 6 months

	return 6*30*86400 ;											// default: try every 5 months
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

#ifdef GROUTER_DEBUG
	grouter_debug() << "p3GRouter::autoWash(): cleaning old entried." << std::endl;
#endif

	// cleanup cache

	time_t now = time(NULL) ;

	for(std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator it(_pending_messages.begin());it!=_pending_messages.end();)
		if(it->second.received_time + GROUTER_ITEM_MAX_CACHE_KEEP_TIME < now)	// is the item too old for cache
		{
#ifdef GROUTER_DEBUG
			grouter_debug() << "  Removing cache item " << std::hex << it->first << std::dec << std::endl;
#endif
			delete it->second.data_item ;
			std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator tmp(it) ;
			++tmp ;
			_pending_messages.erase(it) ;
			it = tmp ;
		}
		else if(it->second.data_item != NULL && it->second.status_flags == RS_GROUTER_ROUTING_STATE_SENT && computeNextTimeDelay(it->second.last_sent - it->second.received_time) + it->second.last_sent < now)
		{
			it->second.status_flags = RS_GROUTER_ROUTING_STATE_PEND ;
#ifdef GROUTER_DEBUG
			grouter_debug() << "  Scheduling the item " << std::hex << it->first << std::dec << " for sending again." << std::endl;
#endif
			++it ;
		}
		else
			++it ;

	// look into pending items.
	
#ifdef GROUTER_DEBUG
	grouter_debug() << "  Pending messages to route  : " << _pending_messages.size() << std::endl;
#endif
}

void p3GRouter::routePendingObjects()
{
	RsStackMutex mtx(grMtx) ;

	time_t now = time(NULL) ;

#ifdef GROUTER_DEBUG
	grouter_debug() << "p3GRouter::routeObjects() triage phase:" << std::endl;
	grouter_debug() << "Cached Items : " << _pending_messages.size() << std::endl;
#endif

	std::set<RsPeerId> lst ;
	mServiceControl->getPeersConnected(getServiceInfo().mServiceType,lst) ;
	RsPeerId own_id( mServiceControl->getOwnId() );

	// The policy is the following:
	//
	// - all pending messages should be handled.	A msg is pending when it is waiting for routage.
	//   A pending message should always have a non NULL data item attached.
	//
	for(std::map<GRouterMsgPropagationId, GRouterRoutingInfo>::iterator it(_pending_messages.begin());it!=_pending_messages.end();)
		if(it->second.status_flags == RS_GROUTER_ROUTING_STATE_PEND)
		{
			// make sure we have data to send.
			//
			if(it->second.data_item == NULL)
			{
				grouter_debug() << "  (EE) Pending item has no data!!" << std::endl;
				++it ;
				continue ;
			}
#ifdef GROUTER_DEBUG
			grouter_debug() << "  Msg id: " << std::hex << it->first << std::dec << std::endl;
			grouter_debug() << "  Origin: " << it->second.origin.toStdString() << std::endl;
			if(!it->second.tried_friends.empty())
			{
				grouter_debug() << "    Last  : " << it->second.tried_friends.front().friend_id.toStdString() << std::endl;
				grouter_debug() << "    S Time: " << it->second.tried_friends.front().time_stamp << std::endl;
			}
			grouter_debug() << "  Recvd : " << now - it->second.received_time << " secs ago." << std::endl;
			grouter_debug() << "  Sent  : " << now - it->second.last_sent << " secs ago." << std::endl;
			grouter_debug() << "  Flags : " << it->second.status_flags << std::endl;
			grouter_debug() << "  Dist  : " << it->second.data_item->randomized_distance<< std::endl;
			grouter_debug() << "  Probabilities: " << std::endl;
#endif
			std::vector<RsPeerId> pids ;
			for(std::set<RsPeerId>::const_iterator its(lst.begin());its!=lst.end();++its)
				if(*its != it->second.origin)
					pids.push_back(*its) ;

			if(pids.empty())	// no friends to send to!! Send back a give up signal.
			{
				sendACK(it->second.origin,it->first,RS_GROUTER_ACK_STATE_GVNP) ;
				it->second.status_flags = RS_GROUTER_ROUTING_STATE_DEAD ;
				delete it->second.data_item ;
				it->second.data_item = NULL ;
				++it ;
				continue ;
			}
			std::vector<float> probas ;		// friends probabilities for online friend list.
			RsPeerId routed_friend ;			// friend chosen for the next hop

			// Retrieve probabilities for this key. This call always succeeds. If no route is known, all probabilities become equal.
			//
			_routing_matrix.computeRoutingProbabilities(it->second.destination_key, pids, probas) ;

			// Compute the maximum branching factor.

			int N = computeBranchingFactor(pids,it->second.data_item->randomized_distance) ;

			// Now use this to select N random peers according to the given probabilities

			std::set<uint32_t> routing_friend_indices = computeRoutingFriends(pids,probas,N) ;

#ifdef GROUTER_DEBUG
			grouter_debug() << "  Routing statistics: " << std::endl;
#endif

			// Actually send the item.

			for(std::set<uint32_t>::const_iterator its(routing_friend_indices.begin());its!=routing_friend_indices.end();++its)
			{
#ifdef GROUTER_DEBUG
				grouter_debug() << "    Friend : " << (*its) << std::endl;
#endif

				// make a deep copy of the item
				RsGRouterGenericDataItem *new_item = it->second.data_item->duplicate() ;

				// update cache entry
				FriendTrialRecord ftr ;
				ftr.time_stamp = now ;
				ftr.friend_id = pids[*its];
				ftr.probability = probas[*its] ;
				ftr.nb_friends = probas.size() ;

				it->second.tried_friends.push_front(ftr) ;

#ifdef GROUTER_DEBUG
				grouter_debug() << "    Routing probability: " << ftr.probability << std::endl;
				grouter_debug() << "    Sending..." << std::endl;
#endif

				// send
				new_item->PeerId(pids[*its]) ;
				new_item->randomized_distance += computeRandomDistanceIncrement(pids[*its],new_item->destination_key) ;

				sendItem(new_item) ;
			}

			it->second.status_flags = RS_GROUTER_ROUTING_STATE_SENT ;
			it->second.last_sent = now ;
		}
		else
		{
#ifdef GROUTER_DEBUG
			grouter_debug() << "Skipping " << std::hex << it->first << std::dec << ", state = " << it->second.status_flags ;
			if(!it->second.tried_friends.empty())
				grouter_debug() << ", stamp=" << it->second.tried_friends.front().time_stamp << " - " << it->second.tried_friends.front().friend_id.toStdString() << std::endl;
			else
				grouter_debug() << std::endl;
#endif
			++it ;
		}
}

uint32_t p3GRouter::computeRandomDistanceIncrement(const RsPeerId& pid,const GRouterKeyId& destination_key)
{
	// This computes a consistent random bias between 0 and 255, which only depends on the
	// destination key and the friend the item is going to be routed through.
	// Makes it much harder for attakcers to figure out what is going on with
	// distances in the network, and makes statistics about multiple sending
	// attempts impossible.
	//
	static const int total_size = RsPeerId::SIZE_IN_BYTES + GRouterKeyId::SIZE_IN_BYTES + sizeof(_random_salt) ;

	unsigned char tmpmem[total_size] ;
	*(uint64_t*)&tmpmem[0] = _random_salt ;
	memcpy(&tmpmem[sizeof(_random_salt)],pid.toByteArray(),RsPeerId::SIZE_IN_BYTES) ;
	memcpy(&tmpmem[sizeof(_random_salt) + RsPeerId::SIZE_IN_BYTES],destination_key.toByteArray(),GRouterKeyId::SIZE_IN_BYTES) ;

	return RsDirUtil::sha1sum(tmpmem,total_size).toByteArray()[5] ;
}

uint32_t p3GRouter::computeBranchingFactor(const std::vector<RsPeerId>& friends,uint32_t dist) 
{
	// The branching factor N should ensure that messages have a constant probability of getting to destination.
	// What we're computing here is the maximum branching factor. Depending on the routing probabilities,
	// the actual branching factor is likely to be less.
	//
	// The output is a number of friends to pick, which we compute from the total number of connected friends.
	// We use a heuristic, based on observations of turtle:
	//
	// 	dist	: 0      1      2      3      4      5     6
	// 	BF		: 1    0.7    0.3    0.1   0.05   0.05  0.05

	static const uint32_t MAX_DIST_INDEX = 7 ;
	static const float branching_factors[MAX_DIST_INDEX] = { 1,0.7,0.3,0.1,0.05,0.05,0.05 } ;

	uint32_t dist_index = std::min( (uint32_t)(dist / (float)GROUTER_ITEM_DISTANCE_UNIT), MAX_DIST_INDEX-1) ;

	return std::max(2, (int)(friends.size()*branching_factors[dist_index])) ;
}

float p3GRouter::computeMatrixContribution(float base,uint32_t time_shift,float probability)
{
	// This function computes the contribution to the routing matrix for an ACK that was
	// received. The different variables are:
	// 	base			: base contribution. 1.0 for directly received items, 0.5 for indirectly received items.
	// 	time_shift	: time in seconds between when the item was sent and when the item was ACKed. This is a clue of
	// 							how far the destination is, and is used to favor fast routes.
	//		probability	: probability with which the item was sent. This should be used for importance-sampling the resulting weight
	
	if(probability == 0.0f)
	{
		std::cerr << "Probability is NULL !!!!! This should not happen." << std::endl;
		return 0.0f ;
	}
	return base * exp(-float(time_shift) / float(RS_GROUTER_MEAN_EXPECTED_RTT)) / probability ;
}

class peer_comparison_function
{
	public:
		bool operator()(const std::pair<float,uint32_t>& p1,const std::pair<float,uint32_t>& p2) const
		{
			return p1.first > p2.first ;
		}
};
std::set<uint32_t> p3GRouter::computeRoutingFriends(const std::vector<RsPeerId>& pids,const std::vector<float>& probas,uint32_t N) 
{
	std::set<uint32_t> res ;

	if(pids.size() != probas.size())
	{
		std::cerr << __PRETTY_FUNCTION__ << ": ERROR!! pids and probas should have the same size! Returning 0 friends!" << std::endl;
		return res ;
	}
#ifdef GROUTER_DEBUG
	grouter_debug() << "    Computing routing friends. Probabilities are: " << std::endl;
	for(uint32_t j=0;j<probas.size();++j)
		grouter_debug() << "      " << j  << " (" << pids[j] << ") : " << probas[j]<< std::endl;
#endif
	// We draw N friends according to the routing probabilitites that are passed as parameter,
	// removing duplicates. This has the nice property to randomly select new friends to 
	// try, but based on how unlikely they are to be correct.
	//
	// Doesn't need to randomise probabilitites, and allows tto compute a sensible importance sampling 
	// value to be used when correcting the trajectory.
	//
	for(uint32_t i=0;i<N;++i)
	{
		int p = probas.size() ;

		// randomly select one peer between 0 and p

		float total = 0.0f ; for(uint32_t j=0;j<probas.size();++j) total += probas[j] ;	// computes the partial sum of the array
		float r = RSRandom::random_f32()*total ;

		int k=0; total=probas[0] ; while(total<r) total += probas[++k]; 

#ifdef GROUTER_DEBUG
		grouter_debug() << "    => Friend " << i << ", between 0 and " << p-1 << ": chose k=" << k << ", peer=" << pids[k] << " with probability " << probas[k] << std::endl;
#endif

		res.insert(k) ;
	}

	// We also add a totally random peer, for the sake of discovery new routes.
	//
	return res ;
}

bool p3GRouter::registerKey(const GRouterKeyId& key,const GRouterServiceId& client_id,const std::string& description) 
{
	RsStackMutex mtx(grMtx) ;

	if(_registered_services.find(client_id) == _registered_services.end())
	{
		std::cerr << __PRETTY_FUNCTION__ << ": unable to register key " << key << " for client id " << client_id << ": client id  is not known." << std::endl;
		return false ;
	}

	GRouterPublishedKeyInfo info ;
	info.service_id = client_id ;
	info.description_string = description.substr(0,20);

	_owned_key_ids[key] = info ;
#ifdef GROUTER_DEBUG
	grouter_debug() << "Registered the following key: " << std::endl;
	grouter_debug() << "   Key id      : " << key.toStdString() << std::endl;
	grouter_debug() << "   Client id   : " << std::hex << client_id << std::dec << std::endl;
	grouter_debug() << "   Description : " << info.description_string << std::endl;
#endif

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

#ifdef GROUTER_DEBUG
	grouter_debug() << "p3GRouter::unregistered the following key: " << std::endl;
	grouter_debug() << "   Key id      : " << key.toStdString() << std::endl;
	grouter_debug() << "   Client id   : " << std::hex << it->second.service_id << std::dec << std::endl;
	grouter_debug() << "   Description : " << it->second.description_string << std::endl;
#endif

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

void p3GRouter::locked_notifyClientAcknowledged(const GRouterKeyId& key,const GRouterMsgPropagationId& msg_id) const
{
#ifdef GROUTER_DEBUG
	grouter_debug() << "  Key is owned by us. Notifying service that item was ACKed." << std::endl;
#endif
	// notify the client
	//
	std::map<GRouterKeyId, GRouterPublishedKeyInfo>::const_iterator it = _owned_key_ids.find(key) ;

	if(it == _owned_key_ids.end())
	{
		std::cerr << "(EE) key " << key << " is not owned by us. That is a weird situation. Probably a bug!" << std::endl;
		return ;
	}
	std::map<GRouterServiceId,GRouterClientService*>::const_iterator its = _registered_services.find(it->second.service_id) ;

	if(its == _registered_services.end())
	{
		std::cerr << "(EE) key " << key << " is attached to service " << it->second.service_id << ", which is unknown!! That is a bug." << std::endl;
		return ;
	}
	its->second->acknowledgeDataReceived(msg_id) ;
} 

void p3GRouter::handleRecvACKItem(RsGRouterACKItem *item)
{
	RsStackMutex mtx(grMtx) ;
#ifdef GROUTER_DEBUG
	grouter_debug() << "Received ACK item, mid=" << std::hex << item->mid << std::dec << ", ACK type = "<< item->state << std::endl;
#endif

	// find the item in the pendign list, 
	// 	- if not found, drop.
	// 	- if we're origin 
	// 			notify the client service
	// 		else
	// 		 	remove item data
	//
	// Item states:
	// 	ARVD  :  item was previously delivered and acknowledge
	// 	PEND  :  item is not yet handled
	// 	SENT  :  item has been sent. Awaiting response from peers.
	//
	// ACK types:
	// 	IRCV	: 	indirectly received
	// 	RCVD  :  received
	// 	GVNP  :  Given up (for various reasons, including timed out, no route, etc)
	//
	// Rules for ACK items:
	//
	//     ACK type/state|   Forward back?       | New state          |    Update Matrix   |  Comment
	//     --------------+-----------------------+--------------------+--------------------+---------------------------------------------------
	//     RCVD          |                       |                    |                    |  
	//         ARVD      |                  N/A  |                    |                    |  
	//         SENT      |   RCVD                | ARVD               |    YES             |  
	//     IRCV          |                       |                    |                    |  
	//         ARVD      |   NO                  | ARVD               |    YES             |  Not forwarded because already frwded by same route
	//         SENT      |   RCVD                | ARVD               |    YES             |  
	//     GVNP          |                       |                    |                    |  
	//            Last   |   YES (Nothing / GVNP)| DEAD               |    NO              |  Just decrease tried friends, and forward when all done.
	//        Not Last   |   NO                  | SENT               |    NO              |  Just decrease tried friends, and forward when all done.
	//  
	// - always decrease tried friends, whatever the answer. A given friend should send back only one answer.
	// 	* a good statistics is the number of un-answered friends still pending
	// 	* when tried friends are empty, send back an ACK that is: 
	// 			- nothing if state = ARVD
	// 			- GVNP if state = SENT
	//
	// - always keep the item in cache for as long as necessary, in order to avoid forwarding items indefinitely
	//
	// 1 - determine all state variables: incoming ACK type and current state
	//
	std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator it(_pending_messages.find(item->mid)) ;

	if(it == _pending_messages.end())
	{
#ifdef GROUTER_DEBUG
		grouter_debug() << "p3GRouter::handleRecvACKItem(): cannot find entry for message id " << std::hex << item->mid << std::dec << ". Dropping it." << std::endl;
#endif
		return ;
	}
	uint32_t next_state = it->second.status_flags;
	uint32_t forward_state = RS_GROUTER_ACK_STATE_UNKN ;
	bool update_routing_matrix = false ;
	bool should_remove = false ;
	bool delete_data = false ;

	time_t now = time(NULL) ;

	switch(item->state)
	{
		case RS_GROUTER_ACK_STATE_RCVD:	
			if(it->second.origin == mLinkMgr->getOwnId())
			{
				locked_notifyClientAcknowledged(it->second.destination_key,it->first) ;
				should_remove = true ;									
			} // no break afterwards. That is on purpose!

		case RS_GROUTER_ACK_STATE_IRCV:				
			// Notify the origin. This is the main route and it was successful.
																				
#ifdef GROUTER_DEBUG
			grouter_debug() << "  updating routing matrix." << std::endl;
#endif

			if(it->second.status_flags == RS_GROUTER_ROUTING_STATE_SENT)
				forward_state = RS_GROUTER_ACK_STATE_RCVD ;

			next_state = RS_GROUTER_ROUTING_STATE_ARVD ;

			update_routing_matrix = true ;
			delete_data = true ;
			break ;
			

		case RS_GROUTER_ACK_STATE_GVNP:				            // route is bad. We forward back and update the routing matrix.
			break ;
	}

	// Just decrement the list of tried friends
	//
	bool found = false ;

	for(std::list<FriendTrialRecord>::iterator it2(it->second.tried_friends.begin());it2!=it->second.tried_friends.end();++it2)
		if( (*it2).friend_id == item->PeerId())
		{
			if(update_routing_matrix)
			{
				// Now compute the weight for that particular item. See with what probabilities it was chosen.
				//
				// The real formula should be:
				// 	weight = w(ACK type) / probability
				//
				// ... where probability is the probability with whitch the item was sent in the first place.
				//
				// The time should also be set so that the routing clue has less importance.
				//
				float base = (item->state == RS_GROUTER_ACK_STATE_RCVD)?1.0f : 0.5 ;
				uint32_t time_shift = now - (*it2).time_stamp ;
				float probability = (*it2).probability;

				float  weight = computeMatrixContribution(base,time_shift,probability) ;
#ifdef GROUTER_DEBUG
				grouter_debug() << "    base contrib  = " << base << std::endl;
				grouter_debug() << "    time shift    = " << time_shift << std::endl;
				grouter_debug() << "    sendind proba = " << probability << std::endl;
				grouter_debug() << "    ==> final weight : " << weight << std::endl;
#endif
				_routing_matrix.addRoutingClue(it->second.destination_key,item->PeerId(),weight) ;
			}
#ifdef GROUTER_DEBUG
			grouter_debug() << "  Removing friend try for peer " << item->PeerId() << ". " << it->second.tried_friends.size() << " tries left." << std::endl;
#endif
			it->second.tried_friends.erase(it2) ;
			found = true ;
			break ;
		}

	if(!found)
		std::cerr << "  (EE) friend try not found!! This should not happen. Needs debugging." << std::endl;

	if(it->second.tried_friends.empty())
	{
#ifdef GROUTER_DEBUG
		grouter_debug() << "  No tries left. Keeping item into pending list or a while." << std::endl;
#endif
		// If no route was found, delete item, but keep the cache entry for a while in order to avoid bouncing.
		//
		if(it->second.status_flags != RS_GROUTER_ROUTING_STATE_ARVD && next_state != RS_GROUTER_ROUTING_STATE_ARVD)
		{
			next_state = RS_GROUTER_ROUTING_STATE_DEAD ;
			forward_state = RS_GROUTER_ACK_STATE_GVNP ;
		} 
	}

	// Now send an ACK if necessary.
	//
#ifdef GROUTER_DEBUG
	static const std::string statusString[5] = { "Unkn","Pend","Sent","Ackn","Dead" };
	static const std::string ackString[6] = { "Unkn","Rcvd","Ircd","Gvnp","Noro","Toof" };

	grouter_debug() << "ACK triage phase ended. Next state = " << statusString[next_state] << ", forwarded ack=" << ackString[forward_state] << std::endl;
#endif

	if(forward_state != RS_GROUTER_ACK_STATE_UNKN && it->second.origin != mLinkMgr->getOwnId())
	{
#ifdef GROUTER_DEBUG
		grouter_debug() << "  forwarding ACK to origin: " << it->second.origin.toStdString() << std::endl;
#endif
		sendACK(it->second.origin,item->mid,item->state) ;
	}
	it->second.status_flags = next_state ;

	if(delete_data)
	{
		delete it->second.data_item ;
		it->second.data_item = NULL ;
	}
	if(should_remove)
	{
#ifdef GROUTER_DEBUG
		grouter_debug() << "  Removing entry from pending messages. " << std::endl;
#endif
		delete it->second.data_item ;
		_pending_messages.erase(it) ;
	}
}

void p3GRouter::handleRecvDataItem(RsGRouterGenericDataItem *item)
{
	RsStackMutex mtx(grMtx) ;
#ifdef GROUTER_DEBUG
	grouter_debug() << "Received data item for key " << item->destination_key << ", distance = " << item->randomized_distance << std::endl;
#endif

	// check the item depth. If too large, send a ACK back.

	if(item->randomized_distance > GROUTER_ITEM_MAX_TRAVEL_DISTANCE)
	{
#ifdef GROUTER_DEBUG
		grouter_debug() << "  Distance is too large: " << item->randomized_distance << " units. Item is dropped." << std::endl;
#endif
		sendACK(item->PeerId(),item->routing_id,RS_GROUTER_ACK_STATE_GVNP) ;
		return ;
	}
	time_t now = time(NULL) ;

	// Do we have this item in the cache already?
	//   - if not, add in the pending items
	//   - if yet. Ignore, or send ACK for shorter route.
	
	// Multiple cases to handle for both the ACK that is sent back and the next state of the flags
	// for current node, depending on whether the item is already here are not, and what is the 
	// current state of the item cache:
	//
	//	                            |    Not in cache     |   STATE_PEND          |   STATE_SENT       |    STATE_ARVD
	//     ------------------------+---------------------+-----------------------+--------------------+-------------------
	// 	    Acknowledgement      |                     |                       |                    |                                       
	// 	                Ours     |    ACK_RCVD         |       -               |       -            |      ACK_IRVD         
	// 	            Not ours     |        -            |       -               |       -            |      ACK_IRVD        
	// 	                         |                     |                       |                    |                      
	// 	    Next state           |                     |                       |                    |                
	// 	                Ours     |    STATE_ARVD       |   STATE_PEND          |   STATE_SENT       |    STATE_ARVD         
	// 	            Not ours     |    STATE_PEND       |   STATE_PEND          |   STATE_SENT       |    STATE_ARVD        
	// 	                         |                     |                       |                    |                      
	//
	// Item not already here  => set to STATE_PEND
	//
	// 	   N = don't send back any acknowledgement
	// 	   - = unrelevant
	// 				
	
	std::map<GRouterKeyId,GRouterPublishedKeyInfo>::const_iterator it = _owned_key_ids.find(item->destination_key) ;
	std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator itr = _pending_messages.find(item->routing_id) ;
	RsGRouterGenericDataItem *item_copy = NULL;

	uint32_t new_status_flags = RS_GROUTER_ROUTING_STATE_UNKN;
	uint32_t returned_ack     = RS_GROUTER_ACK_STATE_UNKN;

	// Is the item known?
	//
	if(itr != _pending_messages.end())
	{
#ifdef GROUTER_DEBUG
		grouter_debug() << "  Item is already there. Nothing to do. Should we update the cache?" << std::endl;
#endif
		item_copy = itr->second.data_item ;

		if(itr->second.status_flags == RS_GROUTER_ROUTING_STATE_ARVD)
			returned_ack = RS_GROUTER_ACK_STATE_IRCV ;
	}
	else		// item is not known. Store it into pending msgs. We make a copy, since the item will be deleted otherwise.
	{
#ifdef GROUTER_DEBUG
		grouter_debug() << "  Item is new. Storing in cache as pending messages." << std::endl;
#endif

		GRouterRoutingInfo info ;

		info.data_item = item->duplicate() ;
		item_copy = info.data_item ;

		info.origin = item->PeerId() ;
		info.received_time = time(NULL) ;
		info.last_sent = 0 ;
		info.destination_key = item->destination_key ;
		info.status_flags = RS_GROUTER_ROUTING_STATE_PEND ;

		_pending_messages[item->routing_id] = info ;
		itr = _pending_messages.find(item->routing_id) ;
		new_status_flags = itr->second.status_flags ;
		itr->second.received_time = now ;
	}

	// Is the item for us? If so, find the client service and send the item back.
	//
	if(it != _owned_key_ids.end())
	{
		if(itr->second.status_flags == RS_GROUTER_ROUTING_STATE_ARVD)
			returned_ack = RS_GROUTER_ACK_STATE_IRCV ;
		else
		{
			returned_ack = RS_GROUTER_ACK_STATE_RCVD ;
			new_status_flags = RS_GROUTER_ROUTING_STATE_ARVD ;

			// notify the client service.
			//
			std::map<GRouterServiceId,GRouterClientService*>::const_iterator its = _registered_services.find(it->second.service_id) ;

			if(its != _registered_services.end())
			{
#ifdef GROUTER_DEBUG
				grouter_debug() << "  Key is owned by us. Notifying service for this item." << std::endl;
#endif
				its->second->receiveGRouterData(it->first,item_copy) ;
			}
#ifdef GROUTER_DEBUG
			else
				grouter_debug() << "  (EE) weird situation. No service registered for a key that we own. Key id = " << item->destination_key.toStdString() << ", service id = " << it->second.service_id << std::endl;
#endif
		}
	}
	else
	{
#ifdef GROUTER_DEBUG
		grouter_debug() << "  item is not for us. Storing in pending mode and not notifying nor ACKs." << std::endl;
#endif
	}

	grouter_debug() << "  after triage: status = " << new_status_flags << ", ack = " << returned_ack << std::endl;

	if(new_status_flags != RS_GROUTER_ROUTING_STATE_UNKN) 
		itr->second.status_flags = new_status_flags ;

	if(returned_ack     != RS_GROUTER_ACK_STATE_UNKN) 
		sendACK(item->PeerId(),item->routing_id,returned_ack) ;

	_changed = true ;
}

bool p3GRouter::registerClientService(const GRouterServiceId& id,GRouterClientService *service)
{
	RsStackMutex mtx(grMtx) ;
	_registered_services[id] = service ;
	return true ;
}

void p3GRouter::sendData(const GRouterKeyId& destination, RsGRouterGenericDataItem *item,GRouterMsgPropagationId& propagation_id)
{
	RsStackMutex mtx(grMtx) ;
	// push the item into pending messages.
	//
	GRouterRoutingInfo info ;

	time_t now = time(NULL) ;

	info.data_item = item ;
	info.status_flags = RS_GROUTER_ROUTING_STATE_PEND ;
	info.origin = mLinkMgr->getOwnId() ;
	info.data_item->randomized_distance = 0 ;
	info.last_sent = 0 ;
	info.received_time = now ;
	info.destination_key = destination ;
	
	// Make sure we have a unique id (at least locally).
	//
	do { propagation_id = RSRandom::random_u32(); } while(_pending_messages.find(propagation_id) != _pending_messages.end()) ;

	item->destination_key = destination  ;
	item->routing_id = propagation_id  ;

#ifdef GROUTER_DEBUG
	grouter_debug() << "p3GRouter::sendGRouterData(): pushing the followign item in the msg pending list:" << std::endl;
	grouter_debug() << "  data_item.size = " << info.data_item->data_size << std::endl;
	grouter_debug() << "  data_item.byte = " << RsDirUtil::sha1sum(info.data_item->data_bytes,info.data_item->data_size) << std::endl;
	grouter_debug() << "  destination    = " << info.destination_key << std::endl;
	grouter_debug() << "  status         = " << info.status_flags << std::endl;
	grouter_debug() << "  distance       = " << info.data_item->randomized_distance << std::endl;
	grouter_debug() << "  origin         = " << info.origin.toStdString() << std::endl;
	grouter_debug() << "  Recv time      = " << info.received_time << std::endl;
#endif

	_pending_messages[propagation_id] = info ;
}

void p3GRouter::sendACK(const RsPeerId& peer, GRouterMsgPropagationId mid, uint32_t ack_flags)
{
	RsGRouterACKItem *item = new RsGRouterACKItem ;

	item->state = ack_flags ;
	item->mid = mid ;
	item->PeerId(peer) ;

	sendItem(item) ;
}

bool p3GRouter::loadList(std::list<RsItem*>& items) 
{
	RsStackMutex mtx(grMtx) ;

#ifdef GROUTER_DEBUG
	grouter_debug() << "p3GRouter::loadList() : " << std::endl;
#endif

	_routing_matrix.loadList(items) ;

#ifdef GROUTER_DEBUG
	// remove all existing objects.
	//
	grouter_debug() << "  removing all existing items (" << _pending_messages.size() << " items to delete)." << std::endl;
#endif

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

#ifdef GROUTER_DEBUG
	grouter_debug() << "p3GRouter::saveList()..." << std::endl;
	grouter_debug() << "  saving routing clues." << std::endl;
#endif

	_routing_matrix.saveList(items) ;

#ifdef GROUTER_DEBUG
	grouter_debug() << "  saving pending items." << std::endl;
#endif

	for(std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::const_iterator it(_pending_messages.begin());it!=_pending_messages.end();++it)
	{
		RsGRouterRoutingInfoItem *item = new RsGRouterRoutingInfoItem ;
		
		*(GRouterRoutingInfo*)item = it->second ;	// copy all members

		item->data_item = it->second.data_item->duplicate() ;	// deep copy, because we call delete on the object, and the item might be removed before we handle it in the client.

		items.push_back(item) ;
	}

	return true ;
}

bool p3GRouter::getRoutingMatrixInfo(RsGRouter::GRouterRoutingMatrixInfo& info) 
{
	info.per_friend_probabilities.clear() ;
	info.friend_ids.clear() ;
	info.published_keys.clear() ;

	std::set<RsPeerId> ids ;
	mServiceControl->getPeersConnected(getServiceInfo().mServiceType,ids) ;

	RsStackMutex mtx(grMtx) ;

	info.published_keys = _owned_key_ids ;

	for(std::set<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
		info.friend_ids.push_back(*it) ;

	std::vector<GRouterKeyId> known_keys ;
	std::vector<float> probas ;
	_routing_matrix.getListOfKnownKeys(known_keys) ;

	for(uint32_t i=0;i<known_keys.size();++i)
	{
		_routing_matrix.computeRoutingProbabilities(known_keys[i],info.friend_ids,probas) ;
		info.per_friend_probabilities[known_keys[i]] = probas ;
	}

	return true ;
}
bool p3GRouter::getRoutingCacheInfo(std::vector<GRouterRoutingCacheInfo>& infos)  
{
	RsStackMutex mtx(grMtx) ;
	infos.clear() ;

	for(std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::const_iterator it(_pending_messages.begin());it!=_pending_messages.end();++it)
	{
		infos.push_back(GRouterRoutingCacheInfo()) ;
		GRouterRoutingCacheInfo& cinfo(infos.back()) ;

		cinfo.mid = it->first ;
		cinfo.local_origin = it->second.origin ;
		cinfo.destination = it->second.destination_key ;
		cinfo.time_stamp = it->second.received_time ;
		cinfo.status = it->second.status_flags ;
		cinfo.data_size = (it->second.data_item==NULL)?0:(it->second.data_item->data_size) ;
	}
	return true ;
}

// Dump everything
//
void p3GRouter::debugDump()
{
	RsStackMutex mtx(grMtx) ;

	time_t now = time(NULL) ;

	grouter_debug() << "Full dump of Global Router state: " << std::endl; 
	grouter_debug() << "  Owned keys : " << std::endl;

	for(std::map<GRouterKeyId, GRouterPublishedKeyInfo>::const_iterator it(_owned_key_ids.begin());it!=_owned_key_ids.end();++it)
	{
		grouter_debug() << "    Key id          : " << it->first.toStdString() << std::endl;
		grouter_debug() << "      Service id    : " << std::hex << it->second.service_id << std::dec << std::endl;
		grouter_debug() << "      Description   : " << it->second.description_string << std::endl;
	}

	grouter_debug() << "  Registered services: " << std::endl;

	for(std::map<GRouterServiceId,GRouterClientService *>::const_iterator it(_registered_services.begin() );it!=_registered_services.end();++it)
		grouter_debug() << "    " << std::hex << it->first << "   " << std::dec << (void*)it->second << std::endl;

#ifdef TO_BE_REMOVE
	grouter_debug() << "  Key diffusion cache: " << std::endl;

	for(std::map<GRouterKeyPropagationId,time_t>::const_iterator it(_key_diffusion_time_stamps.begin() );it!=_key_diffusion_time_stamps.end();++it)
		grouter_debug() << "    " << std::hex << it->first << "   " << std::dec << now - it->second << " secs ago" << std::endl;

	grouter_debug() << "  Key diffusion items: " << std::endl;
	grouter_debug() << "    [Not shown yet]    " << std::endl;
#endif

	grouter_debug() << "  Data items: " << std::endl;

	static const std::string statusString[5] = { "Unkn","Pend","Sent","Ackn","Dead" };

	for(std::map<GRouterMsgPropagationId, GRouterRoutingInfo>::iterator it(_pending_messages.begin());it!=_pending_messages.end();++it)
	{
		grouter_debug() << "    Msg id: " << std::hex << it->first << std::dec << "  Local Origin: " << it->second.origin.toStdString() ;
		grouter_debug() << "    Destination: " << it->second.destination_key ;
		grouter_debug() << "    Received   : " << now - it->second.received_time << " secs ago.";
		grouter_debug() << "    Last sent  : " << now - it->second.last_sent << " secs ago.";
		grouter_debug() << "    Status: " << statusString[it->second.status_flags] << std::endl;
		grouter_debug() << "    Interval: " << computeNextTimeDelay(it->second.last_sent - it->second.received_time) << std::endl;
	}

	grouter_debug() << "  Routing matrix: " << std::endl;

	_routing_matrix.debugDump() ;
}




