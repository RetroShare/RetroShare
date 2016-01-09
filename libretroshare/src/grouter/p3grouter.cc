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
//     - Messenger
//        - sends/receives messages to distant peers
//     - Channels, forums, posted, etc.
//        - send messages to the origin of the channel/forum/posted
// 
// Decentralized routing algorithm:
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
//          		* depends on the depth of the items. Currently branching is 3 at origin and 1 elsewhere.
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
//    - Main difficulties:
//       * have a good re-try strategy if a msg does not arrive.
//       * handle peer availability. In forward mode: easy. In backward mode:
//         difficult. We should wait, and send back the packet if possible.
//       * robustness
//       * security: avoid flooding, and message alteration.
// 
//  Data pipeline
//  =============
//
//  sendData()
//    |
//    +--> encrypt/sign ---> store in _pending_messages
//
//  receiveTurtleData()
//              |
//              +-------------------------------------------------+
//  tick()                                                        |
//    |                                                           |
//    +--> HandleLowLevelServiceItems()                           |
//    |         |                                                 |
//    |         +--> handleLowLevelServiceItem(item) <------------+
//    |                     |
//    |                     +--> handleIncomingTransactionAckItem()
//    |                     |
//    |                     +--> handleIncomingTransactionChunkItem()
//    |                                |
//    |                                +---> addDataChunk()
//    |                                |
//    |                                +---> push item to _incoming_items list
//    |
//    +--> handleIncoming()
//    |            |
//    |            +---> handleIncomingReceiptItem(GRouterSignedReceiptItem*)
//    |            |                                                 |
//    |            +---> handleIncomingDataItem(GRouterDataItem*)    |
//    |                                 |                            |
//    |                                 +----------------------------+
//    |                                 |
//    |                                 |
//    |                             [for US?] ----<Y>----------------+-----> verifySignedData()
//    |                                 |                            |
//    |                                <N>                           +-----> notifyClient()
//    |                                 |                            |
//    |                                 |                            +-----> send Receipt item ---+
//    |                                 |                                                         |
//    |                                 +----> Store In _pending_messages                         |
//    |                                                                                           |
//    +--> routePendingObjects()                                                                  |
//    |         |                                                                                 |
//    |         +--> locked_collectAvailablePeers()/locked_collectAvailableTunnels()              |
//    |         |                                                                                 |
//    |         +--> sliceDataItem()                                                              |
//    |         |                                                                                 |
//    |         +--> locked_sendTransactionData() <-----------------------------------------------+
//    |                         |
//    |                         +--> mTurtle->sendTurtleData(virtual_pid,turtle_item)  /  sendItem()
//    |
//    +--> handleTunnels()
//    |         |
//    |         +---> mTurtle->stopMonitoringTunnels(hash) ;
//    |         +---> mTurtle->monitoringTunnels(hash) ;
//    |
//    +--> autoWash()
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <math.h>

#include "util/rsrandom.h"
#include "util/rsprint.h"
#include "util/rsmemory.h"
#include "serialiser/rsconfigitems.h"
#include "services/p3idservice.h"
#include "turtle/p3turtle.h"
#include "gxs/rsgixs.h"

#include "p3grouter.h"
#include "grouteritems.h"
#include "groutertypes.h"
#include "grouterclientservice.h"

/**********************/
//#define GROUTER_DEBUG
/**********************/

const std::string p3GRouter::SERVICE_INFO_APP_NAME = "Global Router" ;

p3GRouter::p3GRouter(p3ServiceControl *sc, RsGixs *is)
    : p3Service(), p3Config(), mServiceControl(sc), mGixs(is), grMtx("GRouter")
{
	addSerialType(new RsGRouterSerialiser()) ;

	_last_autowash_time = 0 ;
	_last_debug_output_time = 0 ;
	_last_config_changed = 0 ;
	_last_matrix_update_time = 0 ;
	_debug_enabled = true ;

	_random_salt = RSRandom::random_u64() ;

	_changed = false ;
}

int p3GRouter::tick()
{
    time_t now = time(NULL) ;

    // Sort incoming service data
    //
    handleLowLevelServiceItems() ;

    // Handle high level global router data
    //
    handleIncoming() ;

    // Take each item in the list of pending messages and receipts. If the destination peer is available
    // or if the tunnel is available, the item will be sent there.
    //
    routePendingObjects() ;

    // clean things up. Remove unused requests, old stuff etc.

    autoWash() ;

    // Go through the list of active tunnel requests and pending objects to ask for new tunnels
    // or close existing tunnel requests.
    //
    handleTunnels() ;

    // Update routing matrix
    //
    if(now > _last_matrix_update_time + RS_GROUTER_MATRIX_UPDATE_PERIOD)
    {
        RsStackMutex mtx(grMtx) ;

        _last_matrix_update_time = now ;
        _routing_matrix.updateRoutingProbabilities() ;		// This should be locked.
        _routing_matrix.cleanUp() ;				// This should be locked.
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

RsSerialiser *p3GRouter::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser ;

	rss->addSerialType(new RsGRouterSerialiser) ;
	rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss ;
}

bool p3GRouter::registerKey(const RsGxsId& authentication_key,const GRouterServiceId& client_id,const std::string& description)
{
    RS_STACK_MUTEX(grMtx) ;

    if(_registered_services.find(client_id) == _registered_services.end())
    {
        std::cerr << __PRETTY_FUNCTION__ << ": unable to register key " << authentication_key << " for client id " << client_id << ": client id  is not known." << std::endl;
        return false ;
    }

    GRouterPublishedKeyInfo info ;
    info.service_id = client_id ;
    info.authentication_key = authentication_key ;
    info.description_string = description.substr(0,20);

    Sha1CheckSum hash = makeTunnelHash(authentication_key,client_id) ;

    _owned_key_ids[hash] = info ;
#ifdef GROUTER_DEBUG
    grouter_debug() << "Registered the following key: " << std::endl;
    grouter_debug() << "   Auth GXS Id : " << authentication_key << std::endl;
    grouter_debug() << "   Client id   : " << std::hex << client_id << std::dec << std::endl;
    grouter_debug() << "   Description : " << info.description_string << std::endl;
    grouter_debug() << "   Hash        : " << hash << std::endl;
#endif

    return true ;
}
bool p3GRouter::unregisterKey(const RsGxsId& key_id,const GRouterServiceId& sid)
{
        RS_STACK_MUTEX(grMtx) ;

    Sha1CheckSum hash = makeTunnelHash(key_id,sid) ;

    std::map<Sha1CheckSum,GRouterPublishedKeyInfo>::iterator it = _owned_key_ids.find(hash) ;

	if(it == _owned_key_ids.end())
	{
#ifdef GROUTER_DEBUG
        std::cerr << "p3GRouter::unregisterKey(): key " << key_id << " not found." << std::endl;
#endif
		return false ;
	}

#ifdef GROUTER_DEBUG
	grouter_debug() << "p3GRouter::unregistered the following key: " << std::endl;
    grouter_debug() << "   Key id      : " << key_id.toStdString() << std::endl;
	grouter_debug() << "   Client id   : " << std::hex << it->second.service_id << std::dec << std::endl;
	grouter_debug() << "   Description : " << it->second.description_string << std::endl;
#endif

	_owned_key_ids.erase(it) ;

	return true ;
}
//===========================================================================================================================//
//                                                  Service data handling                                                    //
//===========================================================================================================================//

void p3GRouter::handleLowLevelServiceItems()
{
    // While messages read
    //
    RsItem *item = NULL;

    while(NULL != (item = recvItem()))
    {
        RsGRouterTransactionItem *gtitem = dynamic_cast<RsGRouterTransactionItem*>(item);
        if (gtitem)
        {
            handleLowLevelServiceItem(gtitem) ;
        }
        else
        {
            delete(item);
        }
    }
}

void p3GRouter::handleLowLevelServiceItem(RsGRouterTransactionItem *item)
{
    switch(item->PacketSubType())
    {
    case  RS_PKT_SUBTYPE_GROUTER_TRANSACTION_ACKN:
    {
        RsGRouterTransactionAcknItem  *trans_ack_item = dynamic_cast<RsGRouterTransactionAcknItem*>(item);
        if (trans_ack_item)
        {
            handleLowLevelTransactionAckItem(trans_ack_item) ;
        }
        break ;
    }
    case  RS_PKT_SUBTYPE_GROUTER_TRANSACTION_CHUNK:
    {
        RsGRouterTransactionChunkItem *chunk_item = dynamic_cast<RsGRouterTransactionChunkItem*>(item);
        if (chunk_item)
        {
            handleLowLevelTransactionChunkItem(chunk_item) ;
        }
        break ;
    }
    default:
        std::cerr << "p3GRouter::handleIncoming: Unknown packet subtype " << item->PacketSubType() << std::endl ;
    }
    delete item;
}

//===========================================================================================================================//
//                                                    Turtle management                                                      //
//===========================================================================================================================//

bool p3GRouter::handleTunnelRequest(const RsFileHash& hash,const RsPeerId& /*peer_id*/)
{
    // tunnel request is answered according to the following rules:
    // 	- we are the destination => always accept
    //	- we know the destination and have RCPT items to send back => always accept
    //	- we know the destination and have a route (according to matrix) => accept with high probability
    //	- we don't know the destination => accept with very low probability

    if(_owned_key_ids.find(hash) == _owned_key_ids.end())
        return false ;

#ifdef GROUTER_DEBUG
    std::cerr  << "p3GRouter::handleTunnelRequest(). Got req for hash " << hash << ", responding OK" << std::endl;
#endif
    return true ;
}

void p3GRouter::handleLowLevelTransactionChunkItem(RsGRouterTransactionChunkItem *chunk_item)
{
#ifdef GROUTER_DEBUG
    std::cerr << "  item is a transaction item." << std::endl;
#endif

    RsPeerId pid = chunk_item->PeerId() ;

    RsGRouterAbstractMsgItem *generic_item = NULL;
    {
        RS_STACK_MUTEX(grMtx) ;

        generic_item = _incoming_data_pipes[pid].addDataChunk(dynamic_cast<RsGRouterTransactionChunkItem*>(chunk_item->duplicate())) ;// addDataChunk takes ownership over chunk_item
    }

    // send to client off-mutex

    if(generic_item == NULL)
        return ;

    generic_item->PeerId(pid) ;

#ifdef GROUTER_DEBUG
    std::cerr << "  transaction is finished. Passing newly created item to client." << std::endl;
    std::cerr << "  sending a ACK item" << std::endl;
#endif

    RsGRouterTransactionAcknItem ackn_item ;
    ackn_item.propagation_id = generic_item->routing_id ;
    locked_sendTransactionData(pid,ackn_item) ;

    {
        RS_STACK_MUTEX(grMtx) ;
        _incoming_items.push_back(generic_item) ;
    }
}

void p3GRouter::handleLowLevelTransactionAckItem(RsGRouterTransactionAcknItem *trans_ack_item)
{
#ifdef GROUTER_DEBUG
    std::cerr << "  item is a transaction ACK." << std::endl;
#endif
    RS_STACK_MUTEX(grMtx) ;

    std::map<GRouterMsgPropagationId, GRouterRoutingInfo>::iterator it=_pending_messages.find(trans_ack_item->propagation_id) ;

    if(it != _pending_messages.end() && it->second.data_status == RS_GROUTER_DATA_STATUS_ONGOING)
    {
        it->second.data_status = RS_GROUTER_DATA_STATUS_SENT;
        it->second.last_sent_TS = time(NULL) ;
#ifdef GROUTER_DEBUG
        std::cerr << "  setting new status as sent/awaiting receipt." << std::endl;
#endif
    }
#ifdef GROUTER_DEBUG
    else
        std::cerr << "  Note: no routing ID corresponds to this ACK item. This probably corresponds to a signed receipt" << std::endl;
#endif
}

void p3GRouter::receiveTurtleData(RsTurtleGenericTunnelItem *gitem,const RsFileHash& hash,const RsPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction direction)
{
#ifdef GROUTER_DEBUG
    std::cerr << "p3GRouter::receiveTurtleData() " << std::endl;
    std::cerr << "  Received data for hash : " << hash << std::endl;
    std::cerr << "  Virtual peer id        : " << virtual_peer_id << std::endl;
    std::cerr << "  Direction              : " << direction << std::endl;
#endif

    // turtle data is received.
    // This function
    //  - possibly packs multi-item blocks back together
    // 	- converts it into a grouter generic item (by deserialising it)

    RsTurtleGenericDataItem *item = dynamic_cast<RsTurtleGenericDataItem*>(gitem) ;

    if(item == NULL)
    {
        std::cerr << "  ERROR: item is not a data item. That is an error." << std::endl;
        return ;
    }
#ifdef GROUTER_DEBUG
    std::cerr << "  data size          : " << item->data_size << std::endl;
    std::cerr << "  data bytes         : " << RsDirUtil::sha1sum((unsigned char*)item->data_bytes,item->data_size) << std::endl;
#endif

    // Items come out of the pipe in order. We need to recover all chunks before we de-serialise the content and have it handled by handleIncoming()

    RsItem *itm = RsGRouterSerialiser().deserialise(item->data_bytes,&item->data_size) ;

if(itm == NULL)
{
    std::cerr << "(EE) p3GRouter::receiveTurtleData(): cannot de-serialise data. Somthing wrong in the format. Item data (size="<< item->data_size << "): " << RsUtil::BinToHex((char*)item->data_bytes,item->data_size) << std::endl;
    return ;
}

    itm->PeerId(virtual_peer_id) ;

    // At this point we can have either a transaction chunk, or a transaction ACK.
    // We handle them both here

    RsGRouterTransactionChunkItem *chunk_item = dynamic_cast<RsGRouterTransactionChunkItem*>(itm) ;
    RsGRouterTransactionAcknItem  *trans_ack_item = NULL;

    if(chunk_item != NULL)
        handleLowLevelTransactionChunkItem(chunk_item) ;
    else if(NULL != (trans_ack_item = dynamic_cast<RsGRouterTransactionAcknItem*>(itm)))
        handleLowLevelTransactionAckItem(trans_ack_item) ;
    else
    {
        std::cerr << "  ERROR: cannot deserialise turtle item." << std::endl;
        if(itm)
            delete itm ;
    }
}

void GRouterTunnelInfo::removeVirtualPeer(const TurtleVirtualPeerId& vpid)
{
    std::set<TurtleVirtualPeerId,RsGRouterTransactionChunkItem*>::iterator it = virtual_peers.find(vpid) ;

    if(it == virtual_peers.end())
    {
        std::cerr << "  ERROR: removing a virtual peer that does not exist. This is an error!" << std::endl;
        return ;
    }

    virtual_peers.erase(it) ;
}
void GRouterTunnelInfo::addVirtualPeer(const TurtleVirtualPeerId& vpid)
{
    if(virtual_peers.find(vpid) != virtual_peers.end())
        std::cerr << "  ERROR: adding a virtual peer that already exist. This is an error!" << std::endl;

    virtual_peers.insert(vpid) ;

    time_t now = time(NULL) ;

    if(first_tunnel_ok_TS == 0) first_tunnel_ok_TS = now ;
    last_tunnel_ok_TS = now ;
}

RsGRouterAbstractMsgItem *GRouterDataInfo::addDataChunk(RsGRouterTransactionChunkItem *chunk)
{
    last_activity_TS = time(NULL) ;

    // perform some checking

    if(chunk->total_size > MAX_GROUTER_DATA_SIZE + 10000 || chunk->chunk_size > chunk->total_size || chunk->chunk_start >= chunk->total_size)
    {
        std::cerr << "  ERROR: chunk size is unconsistent, or too large: size=" << chunk->chunk_size << ", start=" << chunk->chunk_start << ", total size=" << chunk->total_size << ". Chunk will be dropped. Data pipe will be reset." << std::endl;
        clear() ;
        delete chunk ;
        return NULL ;
    }

    // now add that chunk.

    if(incoming_data_buffer == NULL)
    {
        if(chunk->chunk_start != 0)
        {
            std::cerr << "  ERROR: chunk numbering is wrong. First chunk is not starting at 0. Dropping." << std::endl;
            delete chunk;
            return NULL;
        }
        incoming_data_buffer = chunk ;
    }
    else
    {
        if(incoming_data_buffer->chunk_size != chunk->chunk_start || incoming_data_buffer->total_size != chunk->total_size)
        {
            std::cerr << "  ERROR: chunk numbering is wrong. Dropping." << std::endl;
            delete chunk ;
            delete incoming_data_buffer ;
            incoming_data_buffer = NULL ;
	    return NULL;
        }
        incoming_data_buffer->chunk_data = (uint8_t*)realloc((uint8_t*)incoming_data_buffer->chunk_data,incoming_data_buffer->chunk_size + chunk->chunk_size) ;
        memcpy(&incoming_data_buffer->chunk_data[incoming_data_buffer->chunk_size],chunk->chunk_data,chunk->chunk_size) ;
        incoming_data_buffer->chunk_size += chunk->chunk_size ;

        delete chunk ;
    }

    // if finished, return it.

    if(incoming_data_buffer->total_size == incoming_data_buffer->chunk_size)
    {
        RsItem *data_item = RsGRouterSerialiser().deserialise(incoming_data_buffer->chunk_data,&incoming_data_buffer->chunk_size) ;

        incoming_data_buffer->chunk_data = NULL;
        delete incoming_data_buffer;
        incoming_data_buffer = NULL ;

        return dynamic_cast<RsGRouterAbstractMsgItem*>(data_item) ;
    }
    else
        return NULL ;
}

void p3GRouter::addVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id,RsTurtleGenericTunnelItem::Direction dir)
{
    RS_STACK_MUTEX(grMtx) ;

    // Server side tunnels. This is incoming data. Nothing to do.

#ifdef GROUTER_DEBUG
    std::cerr << "p3GRouter::addVirtualPeer(). Received vpid " << virtual_peer_id << " for hash " << hash << ", direction=" << dir << std::endl;
    std::cerr << "  direction = " << dir << std::endl;
#endif

    // client side. We set the tunnel flags to READY.

    if(dir == RsTurtleGenericTunnelItem::DIRECTION_SERVER)
    {
        bool found = false ;

        // linear search. Bad, but not really a problem. New virtual peers come quite rarely.
        for(std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator it(_pending_messages.begin());it!=_pending_messages.end();++it)
            if(it->second.tunnel_hash == hash)
            {
#ifdef GROUTER_DEBUG
                std::cerr << "  setting tunnel state to READY." << std::endl;
#endif
                it->second.tunnel_status = RS_GROUTER_TUNNEL_STATUS_READY ;
                found = true ;

        // don't break here, because we might send multiple items though the same tunnel.
            }

        if(!found)
        {
            std::cerr << "  ERROR: cannot find corresponding pending message." << std::endl;
            return ;
        }
    }
    if(dir == RsTurtleGenericTunnelItem::DIRECTION_CLIENT)
    {
    }

#ifdef GROUTER_DEBUG
    std::cerr << "  adding VPID." << std::endl;
#endif

    _tunnels[hash].addVirtualPeer(virtual_peer_id) ;

}

void p3GRouter::removeVirtualPeer(const TurtleFileHash& hash,const TurtleVirtualPeerId& virtual_peer_id)
{
    RS_STACK_MUTEX(grMtx) ;

#ifdef GROUTER_DEBUG
    std::cerr << "p3GRouter::removeVirtualPeer(). Removing vpid " << virtual_peer_id << " for hash " << hash << std::endl;
    std::cerr << "  removing VPID." << std::endl;
#endif

    // make sure the VPID exists.

    std::map<TurtleFileHash,GRouterTunnelInfo>::iterator it = _tunnels.find(hash) ;

    if(it == _tunnels.end())
    {
        std::cerr << "  no virtual peers at all for this hash: " << hash << "! This is a consistency error." << std::endl;
        return ;
    }
    it->second.removeVirtualPeer(virtual_peer_id) ;

#ifdef GROUTER_DEBUG
    std::cerr << "  setting tunnel status in pending message." << std::endl;
#endif

     for(std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator it2(_pending_messages.begin());it2!=_pending_messages.end();++it2)
         if(it2->second.tunnel_hash == hash && it->second.virtual_peers.empty())
             it2->second.tunnel_status = RS_GROUTER_TUNNEL_STATUS_PENDING ;

     if(it->second.virtual_peers.empty())
     {
#ifdef GROUTER_DEBUG
         std::cerr << "  last virtual peer removed. Also deleting hash entry." << std::endl;
#endif
         _tunnels.erase(it) ;
     }

}

void p3GRouter::connectToTurtleRouter(p3turtle *pt)
{
    mTurtle = pt ;
    pt->registerTunnelService(this) ;
}

//===========================================================================================================================//
//                                                    Tunnel management                                                      //
//===========================================================================================================================//

class item_comparator_001
{
public:
    template<typename N,class T>
    bool operator()(const std::pair<N,T>& p1,const std::pair<N,T>& p2) const
    {
        return p1.first < p2.first ;
    }
};

void p3GRouter::handleTunnels()
{
    // This function is responsible for asking for tunnels, and removing requests from the turtle router.
    // To remove the unnecessary TR activity generated by multiple peers trying to send the same message,
    // only peers which haven't passed on any data to direct friends, or for which the best friends are not online
    // will be allowed to monitor tunnels.

    // Go through the list of pending messages
    // - if tunnels are pending for too long   => remove from turtle
    // - if item is waiting for too long       => tunnels are waitin

    // We need a priority queue of items to handle, starting from the most ancient items, with a delay that varies with
    // how much time they have been waiting. When a turtle slot it freed, we take the next item in the queue and
    // activate tunnel handling for it.

    // possible pending message status:
    //	- RS_GROUTER_PENDING_MSG_STATUS_TUNNEL_READY	: tunnel is ready. Waiting a few seconds to be used (this is to allow multiple tunnels to come).
    //	- RS_GROUTER_PENDING_MSG_STATUS_TUNNEL_PENDING	: tunnel was asked.
    //	- RS_GROUTER_PENDING_MSG_STATUS_TUNNEL_UNMANAGED: not tunnel managed at the moment.

    // 1 - make a priority list of messages to ask tunnels for

    // Compute the priority of pending messages, according to the number of attempts and how far in the past they have been tried for the last time.
    // Delay after which a message is re-sent, depending on the number of attempts already made.

    RS_STACK_MUTEX(grMtx) ;

#ifdef GROUTER_DEBUG
    if(!_pending_messages.empty())
    {
        grouter_debug() << "p3GRouter::handleTunnels()" << std::endl;
        grouter_debug() << "  building priority list of items to send..." << std::endl;
    }
#endif

    time_t now = time(NULL) ;
    std::vector<std::pair<int,GRouterRoutingInfo*> > priority_list ;

    for(std::map<GRouterMsgPropagationId, GRouterRoutingInfo>::iterator it=_pending_messages.begin();it!=_pending_messages.end();++it)
    {
#ifdef GROUTER_DEBUG
        grouter_debug() << "    " << std::hex << it->first << std::dec
                        << ", data_status=" << it->second.data_status << ", tunnel_status=" << it->second.tunnel_status
                        << ", last received: "<< now - it->second.received_time_TS << " (secs ago)"
                        << ", last TR: "<< now - it->second.last_tunnel_request_TS << " (secs ago)"
                        << ", last sent: " << now - it->second.last_sent_TS << " (secs ago) "<< std::endl;
#endif

        if(it->second.data_status == RS_GROUTER_DATA_STATUS_PENDING && (it->second.routing_flags & GRouterRoutingInfo::ROUTING_FLAGS_ALLOW_TUNNELS))
        {
            if(it->second.tunnel_status == RS_GROUTER_TUNNEL_STATUS_UNMANAGED && it->second.last_tunnel_request_TS + MAX_TUNNEL_UNMANAGED_TIME < now)
            {
                uint32_t item_delay = now - it->second.last_tunnel_request_TS ;
                int item_priority = item_delay ;

#ifdef GROUTER_DEBUG
                grouter_debug() << "  delay=" << item_delay << " attempts=" << it->second.sending_attempts << ", priority=" << item_priority << std::endl;
#endif

                if(item_priority > 0)
                    priority_list.push_back(std::make_pair(item_priority,&it->second)) ;
            }
            else if(it->second.tunnel_status == RS_GROUTER_TUNNEL_STATUS_PENDING && it->second.last_tunnel_request_TS + MAX_TUNNEL_WAIT_TIME < now)
            {
                mTurtle->stopMonitoringTunnels(it->second.tunnel_hash) ;

                it->second.tunnel_status = RS_GROUTER_TUNNEL_STATUS_UNMANAGED ;

#ifdef GROUTER_DEBUG
                grouter_debug() << "  stopping tunnels for this message." << std::endl; ;
#endif
            }
#ifdef GROUTER_DEBUG
            else if(it->second.tunnel_status == RS_GROUTER_TUNNEL_STATUS_READY)
                grouter_debug() << "  tunnel is available. " << std::endl;
            else
                grouter_debug() << "  doing nothing." << std::endl;

            grouter_debug() << std::endl;
#endif
        }
        else
        {
#ifdef GROUTER_DEBUG
            std::cerr << "  closing pending tunnels." << std::endl;
#endif
            mTurtle->stopMonitoringTunnels(it->second.tunnel_hash) ;

            it->second.tunnel_status = RS_GROUTER_TUNNEL_STATUS_UNMANAGED ;
        }

        // also check that all tunnels are actually active, to remove any old dead tunnels
        //
        //        if(it->second.tunnel_status == RS_GROUTER_TUNNEL_STATUS_READY)
        //        {
        //            std::map<TurtleFileHash,GRouterTunnelInfo>::iterator it2 = _tunnels.find(it->second.tunnel_hash) ;
        //
        //            if(it2 == _tunnels.end() || it2->second.virtual_peers.empty()) ;
        //            {
        //                std::cerr << "  re-setting tunnel status to PENDING, as no tunnels are actually present." << std::endl;
        //                it->second.tunnel_status = RS_GROUTER_TUNNEL_STATUS_PENDING ;
        //            }
        //        }
    }
#ifdef GROUTER_DEBUG
    if(!priority_list.empty())
        grouter_debug() << "  sorting..." << std::endl;
#endif


    std::sort(priority_list.begin(),priority_list.end(),item_comparator_001()) ;

    // take tunnels from item priority list, and enable tunnel handling, while respecting max number of active tunnels limit

    for(uint32_t i=0;i<priority_list.size();++i)
    {
#ifdef GROUTER_DEBUG
        grouter_debug() << "  asking tunnel management for msg=" << priority_list[i].second->data_item->routing_id << " hash=" << priority_list[i].second->tunnel_hash << std::endl;
#endif

        mTurtle->monitorTunnels(priority_list[i].second->tunnel_hash,this,false) ;

        priority_list[i].second->tunnel_status = RS_GROUTER_TUNNEL_STATUS_PENDING ;
        priority_list[i].second->last_tunnel_request_TS = now ;
        priority_list[i].second->sending_attempts++ ;
    }
}

void p3GRouter::routePendingObjects()
{
    // Go throught he list of pending messages. For those with a peer ready, send the message to that peer.
    // The peer might be:
    // 	- a virtual peer id that actually is a tunnel
    //	- a real friend node
    // Tunnels and friends will used whenever available. Of course this might cause a message to arrive multiple times, but we
    // don't really care since the GR takes care of duplicates already.
    //
    // Which tunnels are available is handled by handleTunnels()
    //

    time_t now = time(NULL) ;

    RS_STACK_MUTEX(grMtx) ;
#ifdef GROUTER_DEBUG
    if(!_pending_messages.empty())
        std::cerr << "p3GRouter::routePendingObjects()" << std::endl;
#endif
    bool pending_messages_changed = false ;

    for(std::map<GRouterMsgPropagationId, GRouterRoutingInfo>::iterator it=_pending_messages.begin();it!=_pending_messages.end();++it)
    {
#ifdef GROUTER_DEBUG
        std::cerr << "  message " << std::hex << it->first << std::dec << std::endl;
#endif
        if(it->second.data_status == RS_GROUTER_DATA_STATUS_PENDING)
        {
            // Look for tunnels and friends where to send the data. Send to both.

            std::list<RsPeerId> peers ;

            if(it->second.routing_flags & GRouterRoutingInfo::ROUTING_FLAGS_ALLOW_TUNNELS)
                locked_collectAvailableTunnels(it->second.tunnel_hash,peers);

            // For now, disable friends. We'll first check that the good old tunnel system works as before.

            if(it->second.routing_flags & GRouterRoutingInfo::ROUTING_FLAGS_ALLOW_FRIENDS)
                locked_collectAvailableFriends(it->second.data_item->destination_key,peers, it->second.incoming_routes.ids, it->second.routing_flags & GRouterRoutingInfo::ROUTING_FLAGS_IS_ORIGIN);

            if(peers.empty())
            {
#ifdef GROUTER_DEBUG
                std::cerr << "  no direct friends available" << std::endl;
#endif

                if(it->second.received_time_TS + DIRECT_FRIEND_TRY_DELAY < now && !(it->second.routing_flags & GRouterRoutingInfo::ROUTING_FLAGS_ALLOW_TUNNELS))
                {
#ifdef GROUTER_DEBUG
                    std::cerr << "  enabling tunnels for this message." << std::endl;
#endif
                    it->second.routing_flags |= GRouterRoutingInfo::ROUTING_FLAGS_ALLOW_TUNNELS ;
                }
                continue ;
            }

            // slice the data appropriately and send.

            std::list<RsGRouterTransactionChunkItem*> chunks ;
            sliceDataItem(it->second.data_item,chunks) ;

#ifdef GROUTER_DEBUG
            if(!peers.empty())
                std::cerr << "  sending to peers:" << std::endl;
#endif
            for(std::list<RsPeerId>::const_iterator itpid(peers.begin());itpid!=peers.end();++itpid)
                for(std::list<RsGRouterTransactionChunkItem*>::const_iterator it2(chunks.begin());it2!=chunks.end();++it2)
                    locked_sendTransactionData(*itpid,*(*it2) ) ;

            // delete temporary items

            for(std::list<RsGRouterTransactionChunkItem*>::const_iterator cit=chunks.begin();cit!=chunks.end();++cit)
                delete *cit;

            // change item state in waiting list

            it->second.data_status = RS_GROUTER_DATA_STATUS_ONGOING ;
            it->second.data_transaction_TS = now ;

        pending_messages_changed = true ;
        }
        else if(it->second.data_status == RS_GROUTER_DATA_STATUS_ONGOING && now > MAX_TRANSACTION_ACK_WAITING_TIME + it->second.data_transaction_TS)
        {
#ifdef GROUTER_DEBUG
            std::cerr << "  waited too long for this transation. Switching back to PENDING." << std::endl;
#endif

            it->second.data_status = RS_GROUTER_DATA_STATUS_PENDING ;
        }
        else if(it->second.data_status == RS_GROUTER_DATA_STATUS_SENT)
        {
            if( (it->second.routing_flags & GRouterRoutingInfo::ROUTING_FLAGS_IS_ORIGIN) && it->second.last_sent_TS + MAX_DELAY_FOR_RESEND < now)
            {
#ifdef GROUTER_DEBUG
                std::cerr << "  item was not received. Re-setting status to PENDING" << std::endl;
#endif
                it->second.data_status = RS_GROUTER_DATA_STATUS_PENDING ;
            }
            else
            {
#ifdef GROUTER_DEBUG
                std::cerr << "  item was sent. Desactivating tunnels." << std::endl;
#endif
                it->second.routing_flags &= ~GRouterRoutingInfo::ROUTING_FLAGS_ALLOW_TUNNELS ;
            }
        }

        // We treat this case apart, so as to make sure that receipt items are always forwarded wen possible even if the data_status
        // is not set correctly.

        if(it->second.receipt_item != NULL && !it->second.incoming_routes.ids.empty())
        {
            // send the receipt through all incoming routes, as soon as it gets delivered.

#ifdef GROUTER_DEBUG
            std::cerr << "  receipt should be sent back. Trying all incoming routes..." << std::endl;
#endif

            std::list<RsGRouterTransactionChunkItem*> chunks ;

            for(std::set<RsPeerId>::iterator it2=it->second.incoming_routes.ids.begin();it2!=it->second.incoming_routes.ids.end();)
                if(mServiceControl->isPeerConnected(getServiceInfo().mServiceType,*it2) || mTurtle->isTurtlePeer(*it2))
                {
#ifdef GROUTER_DEBUG
                    std::cerr << "  sending receipt back to " << *it2 << " which is online." << std::endl;
#endif
                    if(chunks.empty())
                        sliceDataItem(it->second.receipt_item,chunks) ;

                    for(std::list<RsGRouterTransactionChunkItem*>::const_iterator it3(chunks.begin());it3!=chunks.end();++it3)
                        locked_sendTransactionData(*it2,*(*it3) ) ;

                    // then remove from the set.
                    std::set<RsPeerId>::iterator it2tmp = it2 ;
                    ++it2tmp ;
                    it->second.incoming_routes.ids.erase(it2) ;
                    it2 = it2tmp ;

            pending_messages_changed = true ;
                }
                else
                    ++it2 ;

            for(std::list<RsGRouterTransactionChunkItem*>::const_iterator cit=chunks.begin();cit!=chunks.end();++cit)
                delete *cit;

            // Because signed receipts are small items, we take the bet that if the item could be sent, then it was received.
            // otherwise, we should mark that incomng route as being handled, wait for the ACK and deal with it by updating
            // it->second.data_status at that time.

            if(it->second.incoming_routes.ids.empty())
                it->second.data_status = RS_GROUTER_DATA_STATUS_DONE ;
        }
    }

    if(pending_messages_changed)
        IndicateConfigChanged() ;
}

void p3GRouter::locked_collectAvailableFriends(const GRouterKeyId& gxs_id,std::list<RsPeerId>& friend_peers,const std::set<RsPeerId>& incoming_routes,bool is_origin)
{
    // The strategy is the following:
    //  	if origin
    //		send to multiple neighbors : best and random
    //	else
    //		send to a single "best" neighbor (determined by threshold over routing probability),

    std::set<RsPeerId> ids ;
    mServiceControl->getPeersConnected(getServiceInfo().mServiceType,ids) ;

    std::vector<float> probas;
    std::vector<RsPeerId> tmp_peers;

    // remove previous peers

    for(std::set<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
        if(incoming_routes.find(*it) == incoming_routes.end())
            tmp_peers.push_back(*it) ;

    if(tmp_peers.empty())
        return ;

    _routing_matrix.computeRoutingProbabilities(gxs_id, tmp_peers, probas) ;

#ifdef GROUTER_DEBUG
    std::cerr << "locked_getAvailableFriends()" << std::endl;
    std::cerr << "  getting connected friends, computing routing probabilities" << std::endl;
    for(uint32_t i=0;i<tmp_peers.size();++i)
        std::cerr << "    " << tmp_peers[i] << ", probability: " << probas[i] << std::endl;
#endif
    uint32_t max_count = is_origin?3:1 ;
    float probability_threshold = is_origin?0.0:0.5 ;

#ifdef GROUTER_DEBUG
    std::cerr << "  position at origin: " << is_origin << " => mac_count=" << max_count << ", proba threshold=" << probability_threshold << std::endl;
#endif

    std::vector<std::pair<float,RsPeerId> > mypairs ;

    for(uint32_t i=0;i<tmp_peers.size();++i)
        mypairs.push_back(std::make_pair(probas[i],tmp_peers[i])) ;

    // now sort them up
    std::sort(mypairs.begin(),mypairs.end(),item_comparator_001()) ;

    // take the max_count peers that are still above min_probability

    uint32_t n=0 ;

    for(std::vector<std::pair<float,RsPeerId> >::const_reverse_iterator it = mypairs.rbegin();it!=mypairs.rend() && n<max_count;++it)
        if( (*it).first >= probability_threshold )
    {
            friend_peers.push_back( (*it).second ), ++n ;
#ifdef GROUTER_DEBUG
        std::cerr << "    keeping " << (*it).second << std::endl;
#endif

        if(!is_origin)	// only collect one peer if we're not at origin.
            break ;
    }
}

void p3GRouter::locked_collectAvailableTunnels(const TurtleFileHash& hash,std::list<RsPeerId>& tunnel_peers)
{
    time_t now = time(NULL) ;

    // Now go through available virtual peers. Select the ones that are interesting, and set them as potential destinations.

    std::map<TurtleFileHash,GRouterTunnelInfo>::const_iterator vpit=_tunnels.find(hash) ;

    if(vpit == _tunnels.end())
        return ;

    if(vpit->second.virtual_peers.empty())
    {
#ifdef GROUTER_DEBUG
        std::cerr << "  no peers available. Cannot send!!" << std::endl;
#endif
        return ;
    }
    if(vpit->second.last_tunnel_ok_TS + TUNNEL_OK_WAIT_TIME > now)
    {
#ifdef GROUTER_DEBUG
        std::cerr << ". Still waiting delay (stabilisation)." << std::endl;
#endif
        return ;
    }

    // for now, just take one. But in the future, we will need some policy to temporarily store objects at proxy peers, etc.

#ifdef GROUTER_DEBUG
    std::cerr << "  " << vpit->second.virtual_peers.size() << " virtual peers available. " << std::endl;
#endif
    TurtleVirtualPeerId vpid = *(vpit->second.virtual_peers.begin()) ;

    tunnel_peers.push_back(vpid) ;
}

bool p3GRouter::locked_sendTransactionData(const RsPeerId& pid,const RsGRouterTransactionItem& trans_item)
{
    if(mTurtle->isTurtlePeer(pid))
    {
#ifdef GROUTER_DEBUG
        std::cerr << "  sending to tunnel vpid " << pid << std::endl;
#endif
        uint32_t turtle_data_size = trans_item.serial_size() ;
        uint8_t *turtle_data = (uint8_t*)malloc(turtle_data_size) ;

        if(turtle_data == NULL)
        {
            std::cerr << "  ERROR: Cannot allocate turtle data memory for size " << turtle_data_size << std::endl;
            return false ;
        }
        if(!trans_item.serialise(turtle_data,turtle_data_size))
        {
            std::cerr << "  ERROR: cannot serialise RsGRouterTransactionChunkItem." << std::endl;

            free(turtle_data) ;
            return false ;
        }

        RsTurtleGenericDataItem *turtle_item = new RsTurtleGenericDataItem ;

        turtle_item->data_size  = turtle_data_size ;
        turtle_item->data_bytes = turtle_data ;

#ifdef GROUTER_DEBUG
        std::cerr << "  sending to vpid " << pid << std::endl;
#endif
        mTurtle->sendTurtleData(pid,turtle_item) ;

        return true ;
    }
    else
    {
#ifdef GROUTER_DEBUG
        std::cerr << "  sending to pid " << pid << std::endl;
#endif
        RsGRouterTransactionItem *item_copy = trans_item.duplicate() ;
    item_copy->PeerId(pid) ;

        sendItem(item_copy) ;

        return true ;
    }
}

void p3GRouter::autoWash()
{
    bool items_deleted = false ;
    time_t now = time(NULL) ;

    std::map<GRouterMsgPropagationId,GRouterClientService *> failed_msgs ;

    {
        RS_STACK_MUTEX(grMtx) ;

        for(std::map<GRouterMsgPropagationId, GRouterRoutingInfo>::iterator it=_pending_messages.begin();it!=_pending_messages.end();)
            if( (it->second.data_status == RS_GROUTER_DATA_STATUS_DONE &&
                (!(it->second.routing_flags & GRouterRoutingInfo::ROUTING_FLAGS_IS_DESTINATION)
                    || it->second.received_time_TS + MAX_DESTINATION_KEEP_TIME < now))
            || ((it->second.received_time_TS + GROUTER_ITEM_MAX_CACHE_KEEP_TIME < now)
                                && !(it->second.routing_flags & GRouterRoutingInfo::ROUTING_FLAGS_IS_ORIGIN)
                    && !(it->second.routing_flags & GRouterRoutingInfo::ROUTING_FLAGS_IS_DESTINATION)
            ))	// is the item too old for cache
            {
#ifdef GROUTER_DEBUG
                grouter_debug() << "  Removing cached item " << std::hex << it->first << std::dec << std::endl;
#endif
                //GRouterClientService *client = NULL ;
                //GRouterServiceId service_id = 0;

                if( it->second.data_status != RS_GROUTER_DATA_STATUS_DONE )
		{
                    GRouterClientService *client = NULL;
                    
			if(locked_getLocallyRegisteredClientFromServiceId(it->second.client_id,client))
				failed_msgs[it->first] = client ;
			else
				std::cerr << "  ERROR: client id " << it->second.client_id << " not registered. Consistency error." << std::endl;
		}

                delete it->second.data_item ;

                if(it->second.receipt_item != NULL)
                    delete it->second.receipt_item ;

                std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator tmp(it) ;
                ++tmp ;
                _pending_messages.erase(it) ;
                it = tmp ;

                items_deleted = true ;
            }
            else
                ++it ;

        // also check all existing tunnels

        for(std::map<TurtleFileHash,GRouterTunnelInfo>::iterator it = _tunnels.begin();it!=_tunnels.end();++it)
        {
            std::list<TurtleVirtualPeerId> vpids_to_remove ;
            for(std::set<TurtleVirtualPeerId>::iterator it2 = it->second.virtual_peers.begin();it2!=it->second.virtual_peers.end();++it2)
                if(!mTurtle->isTurtlePeer(*it2))
                {
                    vpids_to_remove.push_back(*it2) ;
#ifdef GROUTER_DEBUG
                    std::cerr << "  " << *it2 << " is not an active tunnel for hash " << it->first << ". Removing virtual peer id." << std::endl;
#endif
                }

            for(std::list<TurtleVirtualPeerId>::const_iterator it2=vpids_to_remove.begin();it2!=vpids_to_remove.end();++it2)
                it->second.removeVirtualPeer(*it2) ;
        }


        // Also clean incoming data pipes

        for(std::map<RsPeerId,GRouterDataInfo>::iterator it(_incoming_data_pipes.begin());it!=_incoming_data_pipes.end();)
            if(it->second.last_activity_TS + MAX_INACTIVE_DATA_PIPE_DELAY < now)
            {
#ifdef GROUTER_DEBUG
                std::cerr << "  removing data pipe for peer " << it->first << " which is too old." << std::endl;
#endif
                std::map<RsPeerId,GRouterDataInfo>::iterator ittmp = it ;
                ++ittmp ;
                it->second.clear() ;
                _incoming_data_pipes.erase(it) ;
                it = ittmp ;
            }
            else
                ++it ;
    }
    // Look into pending items.

    for(std::map<GRouterMsgPropagationId,GRouterClientService*>::const_iterator it(failed_msgs.begin());it!=failed_msgs.end();++it)
    {
#ifdef GROUTER_DEBUG
        std::cerr << "  notifying client for message id " << std::hex << it->first << " state = FAILED" << std::endl;
#endif
        it->second->notifyDataStatus(it->first ,GROUTER_CLIENT_SERVICE_DATA_STATUS_FAILED) ;
    }

    if(items_deleted)
        _changed = true ;
}

bool p3GRouter::sliceDataItem(RsGRouterAbstractMsgItem *item,std::list<RsGRouterTransactionChunkItem*>& chunks)
{
    try
    {
        // Split the item into chunks. This function ensures that chunks in the list are valid. Memory ownership is left to the
        // calling client. In case of error, all allocated memory is deleted.

#ifdef GROUTER_DEBUG
        std::cerr << "p3GRouter::sliceDataItem()" << std::endl;
        std::cerr << "item dump before send:" << std::endl;
        item->print(std::cerr, 2) ;
#endif

        uint32_t size = item->serial_size();

        RsTemporaryMemory data(size) ;	// data will be freed on return, whatever the route taken.

        if(data == NULL)
        {
            std::cerr << "  ERROR: cannot allocate memory. Size=" << size << std::endl;
            throw ;
        }

        if(!item->serialise(data,size))
        {
            std::cerr << "  ERROR: cannot serialise." << std::endl;
            throw ;
        }

        uint32_t offset = 0 ;
        static const uint32_t CHUNK_SIZE = 15000 ;

        while(offset < size)
        {
            uint32_t chunk_size = std::min(size - offset, CHUNK_SIZE) ;

            RsGRouterTransactionChunkItem *chunk_item = new RsGRouterTransactionChunkItem ;

            chunk_item->propagation_id = item->routing_id ;
            chunk_item->total_size = size;
            chunk_item->chunk_start= offset;
            chunk_item->chunk_size = chunk_size ;
            chunk_item->chunk_data = (uint8_t*)malloc(chunk_size) ;
#ifdef GROUTER_DEBUG
            std::cerr << "  preparing to send a chunk [" << offset << " -> " << offset + chunk_size << " / " << size << "]" << std::endl;
#endif

            if(chunk_item->chunk_data == NULL)
            {
                std::cerr << "  ERROR: Cannot allocate memory for size " << chunk_size << std::endl;
                delete chunk_item;
                throw ;
            }
            memcpy(chunk_item->chunk_data,&data[offset],chunk_size) ;

            offset += chunk_size ;

            chunks.push_back(chunk_item) ;
        }

        return true ;
    }
    catch(...)
    {
        for(std::list<RsGRouterTransactionChunkItem*>::const_iterator it(chunks.begin());it!=chunks.end();++it)
            delete *it ;

        chunks.clear() ;

        return false ;
    }
}

void p3GRouter::handleIncoming()
{
    while(!_incoming_items.empty())
    {
        RsGRouterAbstractMsgItem *item = _incoming_items.front() ;
        _incoming_items.pop_front() ;

        RsGRouterGenericDataItem *generic_data_item ;
        RsGRouterSignedReceiptItem *receipt_item ;

        if(NULL != (generic_data_item  = dynamic_cast<RsGRouterGenericDataItem*>(item)))
            handleIncomingDataItem(generic_data_item) ;
        else if(NULL != (receipt_item = dynamic_cast<RsGRouterSignedReceiptItem*>(item)))
            handleIncomingReceiptItem(receipt_item) ;
        else
            std::cerr << "Item has unknown type (not data nor signed receipt). Dropping!" << std::endl;

        delete item ;
    }
}

void p3GRouter::handleIncomingReceiptItem(RsGRouterSignedReceiptItem *receipt_item)
{
    bool changed = false ;
#ifdef GROUTER_DEBUG
    std::cerr << "Handling incoming signed receipt item." << std::endl;
    std::cerr << "Item content:" << std::endl;
    receipt_item->print(std::cerr,2) ;
#endif

    // Because we don't do proxy-transmission yet, the client needs to be notified. Otherwise, we will need to
    // first check if we're a proxy or not. We also remove the message from the global router sending list.
    // in the proxy case, we should only store the receipt.

    GRouterClientService *client_service = NULL;
    GRouterServiceId service_id ;
    GRouterMsgPropagationId mid = 0 ;

    {
        RS_STACK_MUTEX (grMtx) ;

        std::map<GRouterMsgPropagationId, GRouterRoutingInfo>::iterator it=_pending_messages.find(receipt_item->routing_id) ;
        if(it == _pending_messages.end())
        {
            std::cerr << "  ERROR: no routing ID corresponds to this message. Inconsistency!" << std::endl;
            return ;
        }

        // check hash.

        if(receipt_item->data_hash != it->second.item_hash)
        {
            std::cerr << "  checking receipt hash : FAILED. Receipt is dropped." << std::endl;
            return ;
        }
#ifdef GROUTER_DEBUG
        else
            std::cerr << "  checking receipt hash : OK" << std::endl;
#endif
        // check signature.

        if(! verifySignedDataItem(receipt_item))
        {
            std::cerr << "  checking receipt signature : FAILED. Receipt is dropped." << std::endl;
            return ;
        }
#ifdef GROUTER_DEBUG
        std::cerr << "  checking receipt signature : OK. " << std::endl;
        std::cerr << "  removing messsage from cache." << std::endl;
#endif

        if(it->second.routing_flags & GRouterRoutingInfo::ROUTING_FLAGS_IS_ORIGIN)
        {
#ifdef GROUTER_DEBUG
            std::cerr << "  message is at origin. Setting message transmission to DONE" << std::endl;
#endif
            it->second.data_status  = RS_GROUTER_DATA_STATUS_DONE;

            if(locked_getLocallyRegisteredClientFromServiceId(it->second.client_id,client_service))
                mid = it->first ;
            else
            {
                mid = 0 ;
                std::cerr << " ERROR: cannot retrieve service ID for message " << std::hex << it->first << std::dec << std::endl;
            }
        }
        else
        {
#ifdef GROUTER_DEBUG
            std::cerr << "  message is not at origin. Setting message transmission to RECEIPT_OK" << std::endl;
#endif
            it->second.data_status  = RS_GROUTER_DATA_STATUS_RECEIPT_OK;
            it->second.receipt_item = receipt_item->duplicate() ;
        }

        changed = true ;
    }

    if(mid != 0)
    {
#ifdef GROUTER_DEBUG
        std::cerr << "  notifying client " << (void*)client_service << " that msg " << std::hex << mid << std::dec << " was received." << std::endl;
#endif
        client_service->notifyDataStatus(mid, GROUTER_CLIENT_SERVICE_DATA_STATUS_RECEIVED) ;
    }

    // also note the incoming route in the routing matrix

    if(!mTurtle->isTurtlePeer(receipt_item->PeerId()))
    {
#ifdef GROUTER_DEBUG
        std::cerr << "  receipt item comes from a direct friend. Marking route in routing matrix." << std::endl;
#endif
        addRoutingClue(receipt_item->signature.keyId,receipt_item->PeerId()) ;
    }

    if(changed)
        IndicateConfigChanged() ;
}

Sha1CheckSum p3GRouter::computeDataItemHash(RsGRouterGenericDataItem *data_item)
{
    uint32_t total_size = data_item->signed_data_size() + data_item->signature.TlvSize() ;
    RsTemporaryMemory mem(total_size) ;

    uint32_t offset = 0 ;
    data_item->serialise_signed_data(mem,total_size) ;
    offset += data_item->signed_data_size() ;

    data_item->signature.SetTlv(mem, total_size,&offset) ;

    return RsDirUtil::sha1sum(mem,total_size) ;
}

void p3GRouter::handleIncomingDataItem(RsGRouterGenericDataItem *data_item)
{
#ifdef GROUTER_DEBUG
    std::cerr << "Handling incoming data item. " << std::endl;
    std::cerr << "Item content:" << std::endl;
    data_item->print(std::cerr,2) ;
#endif

    // we find 3 things:
    // A - is the item for us ?
    // B - signature and hash check ?
    // C - item is already known ?

    // Store the item?           if                !C
    // Send a receipt?           if     A &&  B
    // Notify client?            if     A &&       !C
    //
    GRouterClientService *client = NULL ;
    GRouterServiceId service_id = data_item->service_id ;
    RsGRouterSignedReceiptItem *receipt_item = NULL ;

    Sha1CheckSum item_hash = computeDataItemHash(data_item) ;

    bool item_is_already_known = false ;
    bool item_is_for_us = false ;
    bool cache_has_changed = false ;

    // A - Find client and service ID from destination key.
#ifdef GROUTER_DEBUG
    std::cerr << "  step A: find if the item is for us or not, and whether it's aready in cache or not." << std::endl;
#endif
    {
        RS_STACK_MUTEX(grMtx) ;

        std::map<GRouterServiceId,GRouterClientService*>::const_iterator its = _registered_services.find(service_id) ;

        if(its == _registered_services.end())
        {
            std::cerr << "    ERROR: client id " << service_id << " not registered. Consistency error." << std::endl;
            return ;
        }
        client = its->second ;

        // also check wether this item is for us or not

        item_is_for_us = _owned_key_ids.find( makeTunnelHash(data_item->destination_key,service_id) ) != _owned_key_ids.end() ;

#ifdef GROUTER_DEBUG
        std::cerr << "    item is " << (item_is_for_us?"":"not") << " for us." << std::endl;
#endif
        std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator it = _pending_messages.find(data_item->routing_id) ;

        if(it != _pending_messages.end())
        {
            if(it->second.item_hash != item_hash)
            {
#ifdef GROUTER_DEBUG
                std::cerr << "    ERROR: item is already known but data hash does not match. Dropping that item." << std::endl;
#endif
                return ;
            }
            item_is_already_known = true ;
            receipt_item = it->second.receipt_item ;
#ifdef GROUTER_DEBUG
            std::cerr << "    item is already in cache." << std::endl;
#endif
        }
#ifdef GROUTER_DEBUG
        else
            std::cerr << "    item is new." << std::endl;
#endif
    }
    // At this point, if item is already known, it is guarrantied to be identical to the stored item.
    // If the item is for us, and not already known, check the signature and hash, and generate a signed receipt

    if(item_is_for_us && !item_is_already_known)
    {
#ifdef GROUTER_DEBUG
        std::cerr << "  step B: item is for us and is new, so make sure it's authentic and create a receipt" << std::endl;
#endif
        if(!verifySignedDataItem(data_item))	// we should get proper flags out of this
        {
            std::cerr << "    verifying item signature: FAILED! Droping that item" ;
            std::cerr << "    You probably received a message from a person you don't have key." << std::endl;
            std::cerr << "    Signature key ID: " << data_item->signature.keyId << std::endl;
        return ;
        }
#ifdef GROUTER_DEBUG
        else
            std::cerr << "    verifying item signature: CHECKED!" ;
#endif
        // No we need to send a signed receipt to the sender.

        receipt_item = new RsGRouterSignedReceiptItem;
        receipt_item->data_hash = item_hash ;
        receipt_item->routing_id = data_item->routing_id ;
        receipt_item->destination_key = data_item->signature.keyId ;
        receipt_item->flags = 0 ;

#ifdef GROUTER_DEBUG
        std::cerr << "    preparing signed receipt." << std::endl;
#endif

        if(!signDataItem(receipt_item,data_item->destination_key))
        {
            std::cerr << "    signing: FAILED. Receipt dropped. ERROR. Packet dropped as well." << std::endl;
            return ;
        }
#ifdef GROUTER_DEBUG
        std::cerr << "    signing: OK." << std::endl;
#endif
    }
#ifdef GROUTER_DEBUG
    else
        std::cerr << "  step B: skipped, since item is not for us, or already known." << std::endl;
#endif

    // now store the item in _pending_messages whether it is for us or not (if the item is for us, this prevents receiving multiple
    // copies of the message)
#ifdef GROUTER_DEBUG
        std::cerr << "  step C: store the item is cache." << std::endl;
#endif
    {
        RS_STACK_MUTEX(grMtx) ;

        GRouterRoutingInfo& info(_pending_messages[data_item->routing_id]) ;

        if(info.data_item == NULL) // item is not for us
        {
#ifdef GROUTER_DEBUG
            std::cerr << "    item is new. Storing it." << std::endl;
#endif

            info.data_item = data_item->duplicate() ;
            info.receipt_item = receipt_item ;	// inited before, or NULL.
            info.tunnel_status = RS_GROUTER_TUNNEL_STATUS_UNMANAGED ;
            info.last_sent_TS = 0 ;
            info.client_id = data_item->service_id ;
            info.item_hash = item_hash ;
            info.last_tunnel_request_TS = 0 ;
            info.sending_attempts = 0 ;
            info.received_time_TS = time(NULL) ;
            info.tunnel_hash = makeTunnelHash(data_item->destination_key,data_item->service_id) ;

            if(item_is_for_us)
        {
            // don't store if item is for us. No need to take that much memory.

            free(info.data_item->data_bytes) ;
            info.data_item->data_size = 0 ;
            info.data_item->data_bytes = NULL ;

            info.routing_flags = GRouterRoutingInfo::ROUTING_FLAGS_IS_DESTINATION | GRouterRoutingInfo::ROUTING_FLAGS_ALLOW_FRIENDS ;
            info.data_status = RS_GROUTER_DATA_STATUS_RECEIPT_OK ;
        }
            else
            {
                info.routing_flags = GRouterRoutingInfo::ROUTING_FLAGS_ALLOW_FRIENDS ;	// don't allow tunnels just yet
                info.data_status = RS_GROUTER_DATA_STATUS_PENDING ;
            }
        }
#ifdef GROUTER_DEBUG
        else
            std::cerr << "    item is already in cache." << std::endl;

        std::cerr << "    storing incoming route from " << data_item->PeerId() << std::endl;
#endif
        info.incoming_routes.ids.insert(data_item->PeerId()) ;
    cache_has_changed = true ;
    }

    // if the item is for us and is not already known, notify the client.

    if(item_is_for_us && !item_is_already_known)
    {
        // compute the hash before decryption.

#ifdef GROUTER_DEBUG
        std::cerr << "  step D: item is for us and is new: decrypting and notifying client." << std::endl;
#endif
        RsGRouterGenericDataItem *decrypted_item = data_item->duplicate() ;

        if(!decryptDataItem(decrypted_item))
        {
            std::cerr << "    decrypting item : FAILED! Item cannot be passed to the client." << std::endl;
            delete decrypted_item ;

            return ;
        }
#ifdef GROUTER_DEBUG
        else
            std::cerr << "    decrypting item : OK!" << std::endl;

        std::cerr << "    notyfying client." << std::endl;
#endif
        if(client->acceptDataFromPeer(decrypted_item->signature.keyId)) 
	{
		client->receiveGRouterData(decrypted_item->destination_key,decrypted_item->signature.keyId,service_id,decrypted_item->data_bytes,decrypted_item->data_size);
                
		decrypted_item->data_bytes = NULL ;
		decrypted_item->data_size = 0 ;
	}

        delete decrypted_item ;
    }
#ifdef GROUTER_DEBUG
    else
        std::cerr << "  step D: item is not for us or not new: skipping this step." << std::endl;
#endif

    if(cache_has_changed)
        IndicateConfigChanged() ;
}

bool p3GRouter::locked_getLocallyRegisteredClientFromServiceId(const GRouterServiceId& service_id,GRouterClientService *& client)
{
    client = NULL ;
    
    std::map<GRouterServiceId,GRouterClientService*>::const_iterator its = _registered_services.find(service_id) ;

    if(its == _registered_services.end())
    {
        std::cerr << "  ERROR: client id " << service_id << " not registered. Consistency error." << std::endl;
        return false;
    }

    client = its->second ;

    return true ;
}

void p3GRouter::addTrackingInfo(const RsGxsMessageId& mid,const RsPeerId& peer_id)
{
    RS_STACK_MUTEX(grMtx) ;
#ifdef GROUTER_DEBUG
    grouter_debug() << "Received new routing clue for key " << mid << " from peer " << peer_id << std::endl;
#endif
    _routing_matrix.addTrackingInfo(mid,peer_id) ;
    _changed = true ;
}
void p3GRouter::addRoutingClue(const GRouterKeyId& id,const RsPeerId& peer_id)
{
    RS_STACK_MUTEX(grMtx) ;
#ifdef GROUTER_DEBUG
    grouter_debug() << "Received new routing clue for key " << id << " from peer " << peer_id << std::endl;
#endif
    _routing_matrix.addRoutingClue(id,peer_id,RS_GROUTER_BASE_WEIGHT_GXS_PACKET) ;
    _changed = true ;
}

bool p3GRouter::registerClientService(const GRouterServiceId& id,GRouterClientService *service)
{
    RS_STACK_MUTEX(grMtx) ;
	_registered_services[id] = service ;
	return true ;
}

bool p3GRouter::encryptDataItem(RsGRouterGenericDataItem *item,const RsGxsId& destination_key)
{
    assert(!(item->flags & RS_GROUTER_DATA_FLAGS_ENCRYPTED)) ;

#ifdef GROUTER_DEBUG
    std::cerr << "  Encrypting data for key " << destination_key << std::endl;
    std::cerr << "    Decrypted size = " << item->data_size << std::endl;
#endif

    uint8_t *encrypted_data =NULL;
    uint32_t encrypted_size =0;
    uint32_t error_status ;

    if(!mGixs->encryptData(item->data_bytes,item->data_size,encrypted_data,encrypted_size,destination_key,true,error_status))
    {
        std::cerr << "(EE) Cannot encrypt: " ;
        if(error_status == RsGixs::RS_GIXS_ERROR_KEY_NOT_AVAILABLE) std::cerr << " key not available for ID = " << destination_key << std::endl;
        if(error_status == RsGixs::RS_GIXS_ERROR_UNKNOWN          ) std::cerr << " unknown error for ID = " << destination_key << std::endl;

        return false ;
    }

    free(item->data_bytes) ;
    item->data_bytes = encrypted_data ;
    item->data_size = encrypted_size ;
    item->flags |= RS_GROUTER_DATA_FLAGS_ENCRYPTED ;

#ifdef GROUTER_DEBUG
    std::cerr << "  Encrypted size = " << encrypted_size << std::endl;
    std::cerr << "  First bytes of encrypted data: " << RsUtil::BinToHex((const char *)encrypted_data,std::min(encrypted_size,30u)) << "..."<< std::endl;
    std::cerr << "  Encrypted data hash = " << RsDirUtil::sha1sum((const uint8_t *)encrypted_data,encrypted_size) << std::endl;
#endif
return true ;
}
bool p3GRouter::decryptDataItem(RsGRouterGenericDataItem *item)
{
    assert(item->flags & RS_GROUTER_DATA_FLAGS_ENCRYPTED) ;

#ifdef GROUTER_DEBUG
    std::cerr << "  decrypting data for key " << item->destination_key << std::endl;
    std::cerr << "  encrypted size = " << item->data_size << std::endl;
#endif

    uint8_t *decrypted_data =NULL;
    uint32_t decrypted_size =0;
    uint32_t error_status ;

    if(!mGixs->decryptData(item->data_bytes,item->data_size,decrypted_data,decrypted_size,item->destination_key,error_status))
    {
        if(error_status == RsGixs::RS_GIXS_ERROR_KEY_NOT_AVAILABLE)
            std::cerr << "(EE) Cannot decrypt incoming message. Key " << item->destination_key << " unknown." << std::endl;
        else
            std::cerr << "(EE) Cannot decrypt incoming message. Unknown error. " << std::endl;

    return false ;
    }

    free(item->data_bytes) ;
    item->data_bytes = decrypted_data ;
    item->data_size = decrypted_size ;
    item->flags &= ~RS_GROUTER_DATA_FLAGS_ENCRYPTED ;

    return true ;
}

bool p3GRouter::signDataItem(RsGRouterAbstractMsgItem *item,const RsGxsId& signing_id)
{
    try
    {
        RsTlvSecurityKey signature_key ;

#ifdef GROUTER_DEBUG
        std::cerr << "p3GRouter::signDataItem()" << std::endl;
        std::cerr << "     Key ID = " << signing_id << std::endl;
        std::cerr << "     Getting key material..." << std::endl;
#endif
        uint32_t data_size = item->signed_data_size() ;
    RsTemporaryMemory data(data_size) ;

        if(data == NULL)
            throw std::runtime_error("Cannot allocate memory for signing data.") ;

        if(!item->serialise_signed_data(data,data_size))
            throw std::runtime_error("Cannot serialise signed data.") ;

    uint32_t error_status ;

    if(!mGixs->signData(data,data_size,signing_id,item->signature,error_status))
            throw std::runtime_error("Cannot sign for id " + signing_id.toStdString() + ". Signature call failed.") ;

#ifdef GROUTER_DEBUG
        std::cerr << "Created    signature for data hash: " << RsDirUtil::sha1sum(data,data_size) << " and key id=" << signing_id << std::endl;
#endif
        return true ;
    }
    catch(std::exception& e)
    {
        std::cerr << "  signing failed. Error: " << e.what() << std::endl;

        item->signature.TlvClear() ;
        return false ;
    }
}
bool p3GRouter::verifySignedDataItem(RsGRouterAbstractMsgItem *item)
{
    try
    {
        RsTlvSecurityKey signature_key ;

        uint32_t data_size = item->signed_data_size() ;
          RsTemporaryMemory data(data_size) ;

		  if(data == NULL)
            throw std::runtime_error("Cannot allocate data.") ;

        if(!item->serialise_signed_data(data,data_size))
            throw std::runtime_error("Cannot serialise signed data.") ;


        uint32_t error_status ;

        if(!mGixs->validateData(data,data_size,item->signature,true,error_status))
        {
            switch(error_status)
            {
                case RsGixs::RS_GIXS_ERROR_KEY_NOT_AVAILABLE: std::cerr << "(EE) Key for GXS Id " << item->signature.keyId << " is not available. Cannot verify." << std::endl;
                                        break ;
                case RsGixs::RS_GIXS_ERROR_SIGNATURE_MISMATCH: std::cerr << "(EE) Signature mismatch. Spoofing/Corrupted/MITM?." << std::endl;
                                        break ;
            default:  std::cerr << "(EE) Signature verification failed on GRouter message. Unknown error status: " << error_status << std::endl;
                break ;
            }
            return false;
        }

        return true ;
    }
    catch(std::exception& e)
    {
        std::cerr << "  signature verification failed. Error: " << e.what() << std::endl;
        return false ;
    }
}

bool p3GRouter::cancel(GRouterMsgPropagationId mid)
{
    {
        RS_STACK_MUTEX(grMtx) ;

#ifdef GROUTER_DEBUG
        std::cerr << "p3GRouter::cancel(). Canceling message ID " << mid << std::endl;
#endif

        std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator it = _pending_messages.find(mid) ;

        if(it == _pending_messages.end())
        {
            std::cerr << "  ERROR: message ID is unknown." << std::endl;
            return false ;
        }

        delete it->second.data_item ;
        if(it->second.receipt_item)
        delete it->second.receipt_item;

        _pending_messages.erase(it) ;
    }

    IndicateConfigChanged() ;

    return true ;
}

bool p3GRouter::sendData(const RsGxsId& destination,const GRouterServiceId& client_id,const uint8_t *data, uint32_t data_size,const RsGxsId& signing_id, GRouterMsgPropagationId &propagation_id)
{
//    std::cerr << "GRouter currently disabled." << std::endl;
//    return false;

    if(data_size > MAX_GROUTER_DATA_SIZE)
    {
        std::cerr << "GRouter max size limit exceeded (size=" << data_size << ", max=" << MAX_GROUTER_DATA_SIZE << "). Please send a smaller object!" << std::endl;
    return false ;
    }

    // Make sure we have a unique id (at least locally). There's little chances that an id of the same value is out there anyway.
    //
    {
        RsStackMutex mtx(grMtx) ;
        do { propagation_id = RSRandom::random_u64(); } while(_pending_messages.find(propagation_id) != _pending_messages.end()) ;
    }

    // create the signed data item

    RsGRouterGenericDataItem *data_item = new RsGRouterGenericDataItem ;

    data_item->data_bytes = (uint8_t*)malloc(data_size) ;
    memcpy(data_item->data_bytes,data,data_size) ;

    data_item->data_size = data_size ;
    data_item->routing_id = propagation_id  ;
    data_item->randomized_distance = 0 ;
    data_item->service_id = client_id ;
    data_item->destination_key = destination  ;
    data_item->flags = 0 ;	// this is unused for now.

    // First, encrypt.

    if(!encryptDataItem(data_item,destination))
    {
        std::cerr << "Cannot encrypt data item. Some error occured!" << std::endl;
	delete data_item;
        return false;
    }

    // Then, sign the encrypted data, so that the signature can be checked by non priviledged users.

    if(!signDataItem(data_item,signing_id))
    {
        std::cerr << "Cannot sign data item. Some error occured!" << std::endl;
	delete data_item;
        return false;
    }

    // Verify the signature. If that fails, there's a bug somewhere!!

    if(!verifySignedDataItem(data_item))
    {
        std::cerr << "Cannot verify data item that was just signed. Some error occured!" << std::endl;
	delete data_item;
        return false;
    }
    // push the item into pending messages.
    //
    GRouterRoutingInfo info ;

    time_t now = time(NULL) ;

    info.data_item = data_item ;
    info.receipt_item = NULL ;
    info.data_status = RS_GROUTER_DATA_STATUS_PENDING ;
    info.tunnel_status = RS_GROUTER_TUNNEL_STATUS_UNMANAGED ;
    info.last_sent_TS = 0 ;
    info.client_id = client_id ;
    info.last_tunnel_request_TS = 0 ;
    info.item_hash = computeDataItemHash(data_item) ;
    info.sending_attempts = 0 ;
    info.routing_flags =  GRouterRoutingInfo::ROUTING_FLAGS_IS_ORIGIN | GRouterRoutingInfo::ROUTING_FLAGS_ALLOW_FRIENDS ;// don't allow tunnels just yet
    info.received_time_TS = now ;
    info.tunnel_hash = makeTunnelHash(destination,client_id) ;

#ifdef GROUTER_DEBUG
    grouter_debug() << "p3GRouter::sendGRouterData(): pushing the following item in the msg pending list:" << std::endl;
    grouter_debug() << "  routing id     = " << std::hex << propagation_id << std::dec << std::endl;
    grouter_debug() << "  data_item.size = " << info.data_item->data_size << std::endl;
    grouter_debug() << "  data_item.byte = " << RsDirUtil::sha1sum(info.data_item->data_bytes,info.data_item->data_size) << std::endl;
    grouter_debug() << "  destination    = " << data_item->destination_key << std::endl;
    grouter_debug() << "  signed by key  = " << data_item->signature.keyId << std::endl;
    grouter_debug() << "  data status    = " << info.data_status << std::endl;
    grouter_debug() << "  tunnel status  = " << info.tunnel_status << std::endl;
    grouter_debug() << "  sending attempt= " << info.sending_attempts << std::endl;
    grouter_debug() << "  distance       = " << info.data_item->randomized_distance << std::endl;
    grouter_debug() << "  recv time      = " << info.received_time_TS << std::endl;
    grouter_debug() << "  client id      = " << std::hex << data_item->service_id << std::dec << std::endl;
    grouter_debug() << "  tunnel hash    = " << info.tunnel_hash << std::endl;
    grouter_debug() << "  item   hash    = " << info.item_hash << std::endl;
    grouter_debug() << "  routing flags  = " << info.routing_flags << std::endl;
#endif

    {
        RS_STACK_MUTEX(grMtx) ;
        _pending_messages[propagation_id] = info ;
    }
    IndicateConfigChanged() ;

return true ;
}

Sha1CheckSum p3GRouter::makeTunnelHash(const RsGxsId& destination,const GRouterServiceId& client)
{
    assert(  destination.SIZE_IN_BYTES == 16) ;
    assert(Sha1CheckSum::SIZE_IN_BYTES == 20) ;

    uint8_t bytes[20] ;
    memcpy(bytes,destination.toByteArray(),16) ;
    bytes[16] = 0 ;
    bytes[17] = 0 ;
    bytes[18] = (client >> 8) & 0xff ;
    bytes[19] =  client       & 0xff ;

    return RsDirUtil::sha1sum(bytes,20) ;
}
#ifdef TO_REMOVE
bool p3GRouter::locked_getGxsOwnIdAndClientIdFromHash(const TurtleFileHash& sum,RsGxsId& gxs_id,GRouterServiceId& client_id)
{
    assert(       gxs_id.SIZE_IN_BYTES == 16) ;
    assert(Sha1CheckSum::SIZE_IN_BYTES == 20) ;

    //gxs_id = RsGxsId(sum.toByteArray());// takes the first 16 bytes
    //client_id = sum.toByteArray()[19] + (sum.toByteArray()[18] << 8) ;
    
    std::map<Sha1CheckSum, GRouterPublishedKeyInfo>::const_iterator it = _owned_key_ids.find(sum);
    
    if(it == _owned_key_ids.end())
        return false ;
    
    gxs_id = it->second.authentication_key ;
    client_id = it->second.service_id ;
    
    return true ;
}
#endif
bool p3GRouter::loadList(std::list<RsItem*>& items)
{
    {
        RS_STACK_MUTEX(grMtx) ;

#ifdef GROUTER_DEBUG
        grouter_debug() << "p3GRouter::loadList() : " << std::endl;
#endif

        _routing_matrix.loadList(items) ;

#ifdef GROUTER_DEBUG
        // remove all existing objects.
        //
        grouter_debug() << "  removing all existing items (" << _pending_messages.size() << " items to delete)." << std::endl;
#endif

        if(!_pending_messages.empty())
            std::cerr << "  WARNING: pending msg list is not empty. List will be cleared." << std::endl;

        // clear the existing list.
        //
        for(std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::iterator it(_pending_messages.begin());it!=_pending_messages.end();++it)
            delete it->second.data_item ;

        _pending_messages.clear() ;

        for(std::list<RsItem*>::const_iterator it(items.begin());it!=items.end();++it)
        {
            RsGRouterRoutingInfoItem *itm1 = NULL ;

            if(NULL != (itm1 = dynamic_cast<RsGRouterRoutingInfoItem*>(*it)))
            {
        // clean data state
        itm1->tunnel_status = RS_GROUTER_TUNNEL_STATUS_UNMANAGED ;

                _pending_messages[itm1->data_item->routing_id] = *itm1 ;

                itm1->data_item = NULL ;	// prevents deletion.
                itm1->receipt_item = NULL ;	// prevents deletion.
            }

            delete *it ;
        }
    }
#ifdef GROUTER_DEBUG
    debugDump();
#endif
    items.clear() ;
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

    RS_STACK_MUTEX(grMtx) ;

    _routing_matrix.saveList(items) ;

#ifdef GROUTER_DEBUG
    grouter_debug() << "  saving pending items." << std::endl;
#endif

    for(std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::const_iterator it(_pending_messages.begin());it!=_pending_messages.end();++it)
    {
        RsGRouterRoutingInfoItem *item = new RsGRouterRoutingInfoItem ;

        *(GRouterRoutingInfo*)item = it->second ;	// copy all members

        item->data_item = it->second.data_item->duplicate() ;	// deep copy, because we call delete on the object, and the item might be removed before we handle it in the client.
        if(it->second.receipt_item != NULL)
            item->receipt_item = it->second.receipt_item->duplicate() ;

        items.push_back(item) ;
    }

    return true ;
}

bool p3GRouter::getRoutingMatrixInfo(RsGRouter::GRouterRoutingMatrixInfo& info) 
{
        RS_STACK_MUTEX(grMtx) ;

    info.per_friend_probabilities.clear() ;
	info.friend_ids.clear() ;
	info.published_keys.clear() ;

	std::set<RsPeerId> ids ;
	mServiceControl->getPeersConnected(getServiceInfo().mServiceType,ids) ;

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
    RS_STACK_MUTEX(grMtx) ;

    infos.clear() ;

    for(std::map<GRouterMsgPropagationId,GRouterRoutingInfo>::const_iterator it(_pending_messages.begin());it!=_pending_messages.end();++it)
    {
        GRouterRoutingCacheInfo cinfo ;

        cinfo.mid = it->first ;
        cinfo.local_origin = it->second.incoming_routes.ids ;
        cinfo.destination = it->second.data_item->destination_key ;
        cinfo.routing_time = it->second.received_time_TS ;
        cinfo.last_tunnel_attempt_time = it->second.last_tunnel_request_TS ;
        cinfo.last_sent_time = it->second.last_sent_TS ;
        cinfo.receipt_available = (it->second.receipt_item != NULL);
        cinfo.data_status = it->second.data_status ;
        cinfo.tunnel_status = it->second.tunnel_status ;
        cinfo.data_size = it->second.data_item->data_size ;
        cinfo.item_hash = it->second.item_hash;

        infos.push_back(cinfo) ;
    }
    return true ;
}

bool p3GRouter::getTrackingInfo(const RsGxsMessageId &mid, RsPeerId &provider_id)
{
        RS_STACK_MUTEX(grMtx) ;
        
        return _routing_matrix.getTrackingInfo(mid,provider_id) ;
}

// Dump everything
//
void p3GRouter::debugDump()
{
        RS_STACK_MUTEX(grMtx) ;

	time_t now = time(NULL) ;

	grouter_debug() << "Full dump of Global Router state: " << std::endl; 
	grouter_debug() << "  Owned keys : " << std::endl;

    for(std::map<Sha1CheckSum, GRouterPublishedKeyInfo>::const_iterator it(_owned_key_ids.begin());it!=_owned_key_ids.end();++it)
	{
	    grouter_debug() << "    Hash            : " << it->first << std::endl;
	    grouter_debug() << "    Key             : " << it->second.authentication_key << std::endl;
		grouter_debug() << "      Service id    : " << std::hex << it->second.service_id << std::dec << std::endl;
		grouter_debug() << "      Description   : " << it->second.description_string << std::endl;
	}

	grouter_debug() << "  Registered services: " << std::endl;

	for(std::map<GRouterServiceId,GRouterClientService *>::const_iterator it(_registered_services.begin() );it!=_registered_services.end();++it)
		grouter_debug() << "    " << std::hex << it->first << "   " << std::dec << (void*)it->second << std::endl;

	grouter_debug() << "  Data items: " << std::endl;

    static const std::string statusString[6] = { "Unkn","Pend","Sent","Receipt OK","Ongoing","Done" };

	for(std::map<GRouterMsgPropagationId, GRouterRoutingInfo>::iterator it(_pending_messages.begin());it!=_pending_messages.end();++it)
    {
        grouter_debug() << " Msg id: " << std::hex << it->first << std::dec ;
        grouter_debug() << " data hash: " << it->second.item_hash ;
        grouter_debug() << " client id: " << std::hex << it->second.client_id << std::dec;
        grouter_debug() << " Flags: " << std::hex << it->second.routing_flags << std::dec;
        grouter_debug() << " Destination: " << it->second.data_item->destination_key ;
        grouter_debug() << " Received: " << now - it->second.received_time_TS << " secs ago.";
        grouter_debug() << " Last sent: " << now - it->second.last_sent_TS << " secs ago.";
        grouter_debug() << " Transaction TS: " << now - it->second.data_transaction_TS << " secs ago.";
        grouter_debug() << " Data Status: " << statusString[it->second.data_status] << std::endl;
        grouter_debug() << " Tunl Status: " << statusString[it->second.tunnel_status] << std::endl;
	grouter_debug() << " Receipt ok: " << (it->second.receipt_item != NULL) << std::endl;
    }

    grouter_debug() << "  Tunnels: " << std::endl;

    for(std::map<TurtleFileHash,GRouterTunnelInfo>::const_iterator it(_tunnels.begin());it!=_tunnels.end();++it)
    {
        grouter_debug() << "    hash: " << it->first << ", first received: " << now - it->second.last_tunnel_ok_TS << " (secs ago), last received: " << now - it->second.last_tunnel_ok_TS << std::endl;

        for(std::set<TurtleVirtualPeerId>::const_iterator it2 = it->second.virtual_peers.begin();it2!=it->second.virtual_peers.end();++it2)
            grouter_debug() << "      " << (*it2) << std::endl;
    }

    grouter_debug() << "  Incoming data pipes: " << std::endl;

    for(std::map<RsPeerId,GRouterDataInfo>::const_iterator it(_incoming_data_pipes.begin());it!=_incoming_data_pipes.end();++it)
        if(it->second.incoming_data_buffer != NULL)
            grouter_debug() << "    " << it->first << ": offset=" << it->second.incoming_data_buffer->chunk_start << " size=" << it->second.incoming_data_buffer->chunk_size << " over " << it->second.incoming_data_buffer->total_size << std::endl;
        else
            grouter_debug() << "    " << it->first << " empty." << std::endl;

    grouter_debug() << "  Routing matrix: " << std::endl;

   if(_debug_enabled)
       _routing_matrix.debugDump() ;
}




