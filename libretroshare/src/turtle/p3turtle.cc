/*
 * libretroshare/src/services: p3turtle.cc
 *
 * Services for RetroShare.
 *
 * Copyright 2009 by Cyril Soler
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

#include <stdexcept>
#include <stdlib.h>
#include <assert.h>
#ifdef P3TURTLE_DEBUG
#include <assert.h>
#endif

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsfiles.h"

#include "pqi/p3authmgr.h"
#include "pqi/p3connmgr.h"
#include "pqi/pqinotify.h"

#include "ft/ftserver.h"
#include "ft/ftdatamultiplex.h"
#include "ft/ftcontroller.h"

#include "p3turtle.h"

#include <iostream>
#include <errno.h>
#include <cmath>

#include <sstream>
#include <stdio.h>

#include "util/rsdebug.h"
#include "util/rsprint.h"
#include "pqi/pqinetwork.h"

// These number may be quite important. I setup them with sensible values, but
// an in-depth test would be better to get an idea of what the ideal values
// could ever be.
//
static const time_t TUNNEL_REQUESTS_LIFE_TIME 	= 120 ;		/// life time for tunnel requests in the cache.
static const time_t SEARCH_REQUESTS_LIFE_TIME 	= 120 ;		/// life time for search requests in the cache
static const time_t REGULAR_TUNNEL_DIGGING_TIME = 300 ;		/// maximum interval between two tunnel digging campaigns.
static const time_t MAXIMUM_TUNNEL_IDLE_TIME 	=  60 ;		/// maximum life time of an unused tunnel.
static const time_t TUNNEL_MANAGEMENT_LAPS_TIME	=  10 ;		/// look into tunnels regularly every 10 sec.

p3turtle::p3turtle(p3ConnectMgr *cm,ftServer *fs)
	:p3Service(RS_SERVICE_TYPE_TURTLE), p3Config(CONFIG_TYPE_TURTLE), mConnMgr(cm)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	_ft_server = fs ;
	_ft_controller = fs->getController() ;

	srand(time(NULL)) ;
	addSerialType(new RsTurtleSerialiser());

	_last_clean_time = 0 ;
	_last_tunnel_management_time = 0 ;
	_last_tunnel_campaign_time = 0 ;
}

int p3turtle::tick()
{
	// Handle tunnel trafic
	//
	handleIncoming();		// handle incoming packets

	time_t now = time(NULL) ;

	// Tunnel management:
	// 	- we digg new tunnels at least every 5 min (300 sec).
	// 	- we digg new tunnels each time a new peer connects
	// 	- we digg new tunnels each time a new hash is asked for
	//
	if(now > TUNNEL_MANAGEMENT_LAPS_TIME+_last_tunnel_management_time || _force_digg_new_tunnels)
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "Calling tunnel management." << std::endl ;
#endif
		manageTunnels() ;

		_last_tunnel_management_time = now ;
	}

	// Clean every 10 sec.
	//
	if(now > 10+_last_clean_time)
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "Calling autowash." << std::endl ;
#endif
		autoWash() ;					// clean old/unused tunnels and file hashes, as well as search and tunnel requests.
		_last_clean_time = now ;
	}

#ifdef P3TURTLE_DEBUG
	// Dump state for debugging, every 20 sec.
	//
	static time_t last_dump = time(NULL) ;

	if(now > 20+last_dump)
	{
		last_dump = now ;
		dumpState() ;
	}
#endif
	return 0 ;
}

// -----------------------------------------------------------------------------------//
// ------------------------------  Tunnel maintenance. ------------------------------ //
// -----------------------------------------------------------------------------------//
//

// This method handles peer connexion/deconnexion
// If A connects, new tunnels should be initiated from A
// If A disconnects, the tunnels passed through A should be closed.
//
void p3turtle::statusChange(const std::list<pqipeer> &plist) // derived from pqiMonitor
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	// We actually do not shut down tunnels when peers get down: Tunnels that
	// are not working properly get automatically removed after some time.

	// save the list of active peers. This is useful for notifying the ftContoller
	_online_peers = plist ;

	std::cerr << "p3turtle: status change triggered. Saving list of " << plist.size() << " peers." << std::endl ;

	/* if any have switched to 'connected' then we force digging new tunnels */

	for(std::list<pqipeer>::const_iterator pit =  plist.begin(); pit != plist.end(); pit++)
		if ((pit->state & RS_PEER_S_FRIEND) && (pit->actions & RS_PEER_CONNECTED))
			_force_digg_new_tunnels = true ;
}

// adds a virtual peer to the list that is communicated ot ftController.
//
void p3turtle::addDistantPeer(const TurtleFileHash&,TurtleTunnelId tid)
{
	char buff[400] ;
	sprintf(buff,"Turtle tunnel %08x",tid) ;

	{
		_virtual_peers[TurtleVirtualPeerId(buff)] = tid ;
#ifdef P3TURTLE_DEBUG
		assert(_local_tunnels.find(tid)!=_local_tunnels.end()) ;
#endif
		_local_tunnels[tid].vpid = TurtleVirtualPeerId(buff) ;
	}
}

void p3turtle::getVirtualPeersList(std::list<pqipeer>& list)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	list.clear() ;

	for(std::map<TurtleVirtualPeerId,TurtleTunnelId>::const_iterator it(_virtual_peers.begin());it!=_virtual_peers.end();++it)
	{
		pqipeer vp ;
		vp.id = it->first ;
		vp.name = "Virtual (distant) peer" ;
		vp.state = RS_PEER_S_CONNECTED ;
		vp.actions = RS_PEER_CONNECTED ;
		list.push_back(vp) ;
	}
}

// This method handles digging new tunnels as needed.
// New tunnels are dug when:
// 	- new peers have connected. The resulting tunnels should be checked against doubling.
// 	- new hashes are submitted for handling.
//
void p3turtle::manageTunnels()
{
	// Collect hashes for which tunnel digging is necessary / recommended

	std::vector<TurtleFileHash> hashes_to_digg ;
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		time_t now = time(NULL) ;
		bool tunnel_campain = false ;
		if(now > _last_tunnel_campaign_time+REGULAR_TUNNEL_DIGGING_TIME)
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Tunnel management: flaging all hashes for tunnels digging." << std::endl ;
#endif
			tunnel_campain = true ;
			_last_tunnel_campaign_time = now ;
		}
		for(std::map<TurtleFileHash,TurtleFileHashInfo>::const_iterator it(_incoming_file_hashes.begin());it!=_incoming_file_hashes.end();++it)
			if(it->second.tunnels.empty()		// digg new tunnels is no tunnels are available
				|| _force_digg_new_tunnels		// digg new tunnels when forced to (e.g. a new peer has connected)
				|| tunnel_campain)				// force digg new tunnels at regular (large) interval
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  Tunnel management: digging new tunnels for hash " << it->first << "." << std::endl ;
#endif
				hashes_to_digg.push_back(it->first) ;
			}

		_force_digg_new_tunnels = false ;
	}

	for(unsigned int i=0;i<hashes_to_digg.size();++i)
		diggTunnel(hashes_to_digg[i]) ;
}

void p3turtle::autoWash()
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  In autowash." << std::endl ;
#endif
	// Remove hashes that are marked as such.
	//
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		for(unsigned int i=0;i<_hashes_to_remove.size();++i)
		{
			std::map<TurtleFileHash,TurtleFileHashInfo>::iterator it(_incoming_file_hashes.find(_hashes_to_remove[i])) ;

			if(it == _incoming_file_hashes.end())
			{
				std::cerr << "p3turtle: asked to stop monitoring file hash " << _hashes_to_remove[i] << ", but this hash is actually not handled by the turtle router." << std::endl ;
				continue ;
			}

			// copy the list of tunnels to remove.
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: stopping monitoring for file hash " << _hashes_to_remove[i] << ", and closing " << it->second.tunnels.size() << " tunnels (" ;
#endif
			std::vector<TurtleTunnelId> tunnels_to_remove ;

			for(std::vector<TurtleTunnelId>::const_iterator it2(it->second.tunnels.begin());it2!=it->second.tunnels.end();++it2)
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << (void*)*it2 << "," ;
#endif
				tunnels_to_remove.push_back(*it2) ;
			}
#ifdef P3TURTLE_DEBUG
			std::cerr << ")" << std::endl ;
#endif
			for(unsigned int k=0;k<tunnels_to_remove.size();++k)
				locked_closeTunnel(tunnels_to_remove[k]) ;

			_incoming_file_hashes.erase(it) ;
		}
		if(!_hashes_to_remove.empty())
		{
			IndicateConfigChanged() ;	// initiates saving of handled hashes.
			_hashes_to_remove.clear() ;
		}
	}

	// look for tunnels and stored temporary info that have not been used for a while.

	time_t now = time(NULL) ;

	// Search requests
	//
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		for(std::map<TurtleSearchRequestId,TurtleRequestInfo>::iterator it(_search_requests_origins.begin());it!=_search_requests_origins.end();++it)
			if(now > (time_t)(it->second.time_stamp + SEARCH_REQUESTS_LIFE_TIME))
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  removed search request " << (void *)it->first << ", timeout." << std::endl ;
#endif
				_search_requests_origins.erase(it) ;
			}
	}

	// Tunnel requests
	//
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		for(std::map<TurtleTunnelRequestId,TurtleRequestInfo>::iterator it(_tunnel_requests_origins.begin());it!=_tunnel_requests_origins.end();++it)
			if(now > (time_t)(it->second.time_stamp + TUNNEL_REQUESTS_LIFE_TIME))
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  removed tunnel request " << (void *)it->first << ", timeout." << std::endl ;
#endif
				_tunnel_requests_origins.erase(it) ;
			}
	}

	// Tunnels.
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		std::vector<TurtleTunnelId> tunnels_to_close ;

		for(std::map<TurtleTunnelId,TurtleTunnel>::iterator it(_local_tunnels.begin());it!=_local_tunnels.end();++it)
			if(now > (time_t)(it->second.time_stamp + MAXIMUM_TUNNEL_IDLE_TIME))
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  removing tunnel " << (void *)it->first << ": timeout." << std::endl ;
#endif
				tunnels_to_close.push_back(it->first) ;
			}
		for(unsigned int i=0;i<tunnels_to_close.size();++i)
			locked_closeTunnel(tunnels_to_close[i]) ;
	}

	// File hashes can only be removed by calling the 'stopMonitoringFileTunnels()' command.
}

void p3turtle::locked_closeTunnel(TurtleTunnelId tid)
{
	// This is closing a given tunnel, removing it from file sources, and from the list of tunnels of its
	// corresponding file hash. In the original turtle4privacy paradigm, they also send back and forward
	// tunnel closing commands. I'm not sure this is necessary, because if a tunnel is closed somewhere, it's
	// source is not going to be used and the tunnel will eventually disappear.
	//
	std::map<TurtleTunnelId,TurtleTunnel>::iterator it(_local_tunnels.find(tid)) ;

	if(it == _local_tunnels.end())
	{
		std::cerr << "p3turtle: was asked to close tunnel " << (void*)tid << ", which actually doesn't exist." << std::endl ;
		return ;
	}
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: Closing tunnel " << (void*)tid << std::endl ;
#endif

	if(it->second.local_src == mConnMgr->getOwnId())	// this is a starting tunnel. We thus remove
																		// 	- the virtual peer from the vpid list
																		// 	- the tunnel id from the file hash
																		// 	- the virtual peer from the file sources in the file transfer controller.
	{
		TurtleTunnelId tid = it->first ;
		TurtleVirtualPeerId vpid = it->second.vpid ;
		TurtleFileHash hash = it->second.hash ;

#ifdef P3TURTLE_DEBUG
		std::cerr << "    Tunnel is a starting point. Also removing:" << std::endl ;
		std::cerr << "      Virtual Peer Id " << vpid << std::endl ;
		std::cerr << "      Associated file source." << std::endl ;
#endif
		_ft_controller->removeFileSource(hash,vpid) ;
		_virtual_peers.erase(_virtual_peers.find(vpid)) ;

		std::vector<TurtleTunnelId>& tunnels(_incoming_file_hashes[hash].tunnels) ;

		// Remove tunnel id from it's corresponding hash. For security we
		// go through the whole tab, although the tunnel id should only be listed once
		// in this tab.
		//
		for(unsigned int i=0;i<tunnels.size();)
			if(tunnels[i] == tid)
			{
				tunnels[i] = tunnels.back() ;
				tunnels.pop_back() ;
			}
			else
				++i ;
	}
	else if(it->second.local_dst == mConnMgr->getOwnId())	// This is a ending tunnel. We also remove the virtual peer id
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "    Tunnel is a ending point. Also removing associated outgoing hash." ;
#endif
		_outgoing_file_hashes.erase(_outgoing_file_hashes.find(it->second.hash)) ;
	}

	_local_tunnels.erase(it) ;
}

void p3turtle::stopMonitoringFileTunnels(const std::string& hash)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: Marking hash " << hash << " to be removed during autowash." << std::endl ;
#endif
	// We don't do the deletion in this process, because it can cause a race with tunnel management.
	_hashes_to_remove.push_back(hash) ;
}

// -----------------------------------------------------------------------------------//
// --------------------------------  Config functions  ------------------------------ //
// -----------------------------------------------------------------------------------//
//
RsSerialiser *p3turtle::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser ;
	rss->addSerialType(new RsTurtleSerialiser) ;

	return rss ;
}
std::list<RsItem*> p3turtle::saveList(bool& cleanup)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: saving list..." << std::endl ;
#endif
	cleanup = true ;
	std::list<RsItem*> lst ;

	RsTurtleSearchResultItem *item = new RsTurtleSearchResultItem ;
	item->PeerId("") ;

	for(std::map<TurtleFileHash,TurtleFileHashInfo>::const_iterator it(_incoming_file_hashes.begin());it!=_incoming_file_hashes.end();++it)
	{
		TurtleFileInfo finfo ;
		finfo.name = it->second.name ;
		finfo.size = it->second.size ;
		finfo.hash = it->first ;
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Saving item " << finfo.name << ", " << finfo.hash << ", " << finfo.size << std::endl ;
#endif

		item->result.push_back(finfo) ;
	}
	lst.push_back(item) ;

	return lst ;
}
bool p3turtle::loadList(std::list<RsItem*> load)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: loading list..." << std::endl ;
#endif
	for(std::list<RsItem*>::const_iterator it(load.begin());it!=load.end();++it)
	{
		RsTurtleSearchResultItem *item = dynamic_cast<RsTurtleSearchResultItem*>(*it) ;

#ifdef P3TURTLE_DEBUG
		assert(item!=NULL) ;
#endif
		if(item == NULL)
			continue ;

		for(std::list<TurtleFileInfo>::const_iterator it2(item->result.begin());it2!=item->result.end();++it2)
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "   Restarting tunneling for: " << it2->hash << "  " << it2->size << " " << it2->name << std::endl ;
#endif
			monitorFileTunnels(it2->name,it2->hash,it2->size) ;
		}
		delete item ;
	}
	return true ;
}


// -----------------------------------------------------------------------------------//
// --------------------------------  Helper functions  ------------------------------ //
// -----------------------------------------------------------------------------------//
//
uint32_t p3turtle::generateRandomRequestId()
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	return rand() ;
}
uint32_t p3turtle::generatePersonalFilePrint(const TurtleFileHash& hash,bool b)
{
	// whatever cooking from the file hash and OwnId that cannot be recovered.
	// The only important thing is that the saem hash produces the same tunnel
	// id. The result uses a boolean to allow generating non symmetric tunnel ids.

	std::string buff(hash + mConnMgr->getOwnId()) ;
	uint32_t res = 0 ;
	uint32_t decal = 0 ;

	for(int i=0;i<(int)buff.length();++i)
	{
		res += 7*buff[i] + decal ;

		if(b)
			decal = decal*44497+15641+(res%86243) ;
		else
			decal = decal*86243+15649+(res%44497) ;
	}

	return res ;
}
// -----------------------------------------------------------------------------------//
// --------------------------------  Global routing. -------------------------------- //
// -----------------------------------------------------------------------------------//
//
int p3turtle::handleIncoming()
{
#ifdef P3TURTLE_DEBUG
//	std::cerr << "p3turtle::handleIncoming()";
//	std::cerr << std::endl;
#endif

	int nhandled = 0;
	// While messages read
	//
	RsItem *item = NULL;

	while(NULL != (item = recvItem()))
	{
		nhandled++;

		switch(item->PacketSubType())
		{
			case RS_TURTLE_SUBTYPE_STRING_SEARCH_REQUEST:
			case RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST: handleSearchRequest(dynamic_cast<RsTurtleSearchRequestItem *>(item)) ;
																		 break ;

			case RS_TURTLE_SUBTYPE_SEARCH_RESULT : handleSearchResult(dynamic_cast<RsTurtleSearchResultItem *>(item)) ;
																break ;

			case RS_TURTLE_SUBTYPE_OPEN_TUNNEL   : handleTunnelRequest(dynamic_cast<RsTurtleOpenTunnelItem *>(item)) ;
																break ;

			case RS_TURTLE_SUBTYPE_TUNNEL_OK     : handleTunnelResult(dynamic_cast<RsTurtleTunnelOkItem *>(item)) ;
																break ;

			case RS_TURTLE_SUBTYPE_FILE_REQUEST  : handleRecvFileRequest(dynamic_cast<RsTurtleFileRequestItem *>(item)) ;
																break ;

			case RS_TURTLE_SUBTYPE_FILE_DATA     : handleRecvFileData(dynamic_cast<RsTurtleFileDataItem *>(item)) ;
																break ;

			// Here will also come handling of file transfer requests, tunnel digging/closing, etc.
			default:
																std::cerr << "p3turtle::handleIncoming: Unknown packet subtype " << item->PacketSubType() << std::endl ;
		}
		delete item;
	}

	return nhandled;
}

// -----------------------------------------------------------------------------------//
// --------------------------------  Search handling. ------------------------------- //
// -----------------------------------------------------------------------------------//
//
void p3turtle::handleSearchRequest(RsTurtleSearchRequestItem *item)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	// take a look at the item:
	// 	- If the item destimation is

#ifdef P3TURTLE_DEBUG
	std::cerr << "Received search request from peer " << item->PeerId() << ": " << std::endl ;
	item->print(std::cerr,0) ;
#endif
	// If the item contains an already handled search request, give up.  This
	// happens when the same search request gets relayed by different peers
	//
	if(_search_requests_origins.find(item->request_id) != _search_requests_origins.end())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  This is a bouncing request. Ignoring and deleting it." << std::endl ;
#endif
		return ;
	}

	// This is a new request. Let's add it to the request map, and forward it to
	// open peers.

	TurtleRequestInfo& req( _search_requests_origins[item->request_id] ) ;
	req.origin = item->PeerId() ;
	req.time_stamp = time(NULL) ;

	// If it's not for us, perform a local search. If something found, forward the search result back.

	if(item->PeerId() != mConnMgr->getOwnId())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Request not from us. Performing local search" << std::endl ;
#endif
		if(_sharing_strategy != SHARE_FRIENDS_ONLY || item->depth < 2)
		{
			std::list<TurtleFileInfo> result ;

			item->performLocalSearch(result) ;

			RsTurtleSearchResultItem *res_item = NULL ;
			uint32_t item_size = 0 ;

#ifdef P3TURTLE_DEBUG
			if(!result.empty())
				std::cerr << "  " << result.size() << " matches found. Sending back to origin (" << item->PeerId() << ")." << std::endl ;
#endif
			while(!result.empty())
			{
				// Let's chop search results items into several chunks of finite size to avoid exceeding streamer's capacity.
				//
				static const uint32_t RSTURTLE_MAX_SEARCH_RESPONSE_SIZE = 10000 ;

				if(res_item == NULL)
				{
					res_item = new RsTurtleSearchResultItem ;
					item_size = 0 ;

					res_item->depth = 0 ;
					res_item->request_id = item->request_id ;
					res_item->PeerId(item->PeerId()) ;			// send back to the same guy
				}
				res_item->result.push_back(result.front()) ;

				item_size += 8 /* size */ + result.front().hash.size() + result.front().name.size() ;
				result.pop_front() ;

				if(item_size > RSTURTLE_MAX_SEARCH_RESPONSE_SIZE || result.empty())
				{
#ifdef P3TURTLE_DEBUG
					std::cerr << "  Sending back chunk of size " << item_size << ", for " << res_item->result.size() << " elements." << std::endl ;
#endif
					sendItem(res_item) ;
					res_item = NULL ;
				}
			}
		}
#ifdef P3TURTLE_DEBUG
		else
			std::cerr << "  Rejecting local search because strategy is FRIENDS_ONLY and item depth=" << item->depth << std::endl ;
#endif
	}

	// If search depth not too large, also forward this search request to all other peers.
	//
	if(item->depth < TURTLE_MAX_SEARCH_DEPTH)
	{
		std::list<std::string> onlineIds ;
		mConnMgr->getOnlineList(onlineIds);
#ifdef P3TURTLE_DEBUG
				std::cerr << "  Looking for online peers" << std::endl ;
#endif

		for(std::list<std::string>::const_iterator it(onlineIds.begin());it!=onlineIds.end();++it)
			if(*it != item->PeerId())
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  Forwarding request to peer = " << *it << std::endl ;
#endif
				// Copy current item and modify it.
				RsTurtleSearchRequestItem *fwd_item = item->clone() ;

				++(fwd_item->depth) ;		// increase search depth
				fwd_item->PeerId(*it) ;

				sendItem(fwd_item) ;
			}
	}
#ifdef P3TURTLE_DEBUG
	else
		std::cout << "  Dropping this item, as search depth is " << item->depth << std::endl ;
#endif
}

void p3turtle::handleSearchResult(RsTurtleSearchResultItem *item)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	// Find who actually sent the corresponding request.
	//
	std::map<TurtleRequestId,TurtleRequestInfo>::const_iterator it = _search_requests_origins.find(item->request_id) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "Received search result:" << std::endl ;
	item->print(std::cerr,0) ;
#endif
	if(it == _search_requests_origins.end())
	{
		// This is an error: how could we receive a search result corresponding to a search item we
		// have forwarded but that it not in the list ??

		std::cerr << __PRETTY_FUNCTION__ << ": search result has no peer direction!" << std::endl ;
		return ;
	}

	// Is this result's target actually ours ?

	++(item->depth) ;			// increase depth

	if(it->second.origin == mConnMgr->getOwnId())
		returnSearchResult(item) ;		// Yes, so send upward.
	else
	{											// Nope, so forward it back.
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Forwarding result back to " << it->second.origin << std::endl;
#endif
		RsTurtleSearchResultItem *fwd_item = new RsTurtleSearchResultItem(*item) ;	// copy the item

		// normally here, we should setup the forward adress, so that the owner's of the files found can be further reached by a tunnel.

		fwd_item->PeerId(it->second.origin) ;
		fwd_item->depth = 2 + (rand() % 256) ; // obfuscate the depth for non immediate friends.

		sendItem(fwd_item) ;
	}
}

// -----------------------------------------------------------------------------------//
// ---------------------------------  File Transfer. -------------------------------- //
// -----------------------------------------------------------------------------------//


void p3turtle::handleRecvFileRequest(RsTurtleFileRequestItem *item)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3Turtle: received file request item:" << std::endl ;
	item->print(std::cerr,1) ;
#endif
	// This is a new request. Let's add it to the request map, and forward it to
	// open peers.

	TurtleVirtualPeerId vpid ;
	uint64_t size ;
	TurtleFileHash hash ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		std::map<TurtleTunnelId,TurtleTunnel>::iterator it(_local_tunnels.find(item->tunnel_id)) ;

		if(it == _local_tunnels.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: got file request with unknown tunnel id " << (void*)item->tunnel_id << std::endl ;
#endif
			return ;
		}

		TurtleTunnel& tunnel(it->second) ;

		// Let's figure out whether this reuqest is for us or not.

		if(tunnel.local_dst == mConnMgr->getOwnId()) // Yes, we have to pass on the request to the data multiplexer
		{
			std::map<TurtleFileHash,FileInfo>::const_iterator it(_outgoing_file_hashes.find(tunnel.hash)) ;
#ifdef P3TURTLE_DEBUG
			assert(!tunnel.hash.empty()) ;
			assert(it != _outgoing_file_hashes.end()) ;

			std::cerr << "  This is an endpoint for this file request." << std::endl ;
			std::cerr << "  Forwarding data request to the multiplexer." << std::endl ;
			std::cerr << "  using peer_id=" << tunnel.vpid << ", hash=" << tunnel.hash << std::endl ;
#endif
			// _ft_server->getMultiplexer()->recvDataRequest(tunnel.vpid,tunnel.hash,it->second.size,item->chunk_offset,item->chunk_size) ;
			//
			size = it->second.size ;
			vpid = tunnel.vpid ;
			hash = tunnel.hash ;
		}
		else	// No, it's a request we should forward down the pipe.
		{
			RsTurtleFileRequestItem *res_item = new RsTurtleFileRequestItem(*item) ;

			res_item->PeerId(tunnel.local_dst) ;

			sendItem(res_item) ;
			return ;
		}
	}

	// This call is voluntarily off-mutex gards because it can cause cross mutex locks with the multiplexer.
	// (Yeah, this bug was a shity hard one to catch).
	//
	_ft_server->getMultiplexer()->recvDataRequest(vpid,hash,size,item->chunk_offset,item->chunk_size) ;
}

void p3turtle::handleRecvFileData(RsTurtleFileDataItem *item)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3Turtle: received file data item:" << std::endl ;
	item->print(std::cerr,1) ;
#endif
	// This is a new request. Let's add it to the request map, and forward it to
	// open peers.

	TurtleVirtualPeerId vpid ;
	uint64_t size ;
	TurtleFileHash hash ;
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		std::map<TurtleTunnelId,TurtleTunnel>::iterator it(_local_tunnels.find(item->tunnel_id)) ;

		if(it == _local_tunnels.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: got file data with unknown tunnel id " << (void*)item->tunnel_id << std::endl ;
#endif
			return ;
		}

		TurtleTunnel& tunnel(it->second) ;

		// Only file data transfer updates tunnels time_stamp field, to avoid maintaining tunnel that are incomplete.
		tunnel.time_stamp = time(NULL) ;
		// also update the hash time stamp to show that it's actually being downloaded.
		_incoming_file_hashes[tunnel.hash].time_stamp = time(NULL) ;

		// Let's figure out whether this reuqest is for us or not.

		if(tunnel.local_src == mConnMgr->getOwnId()) // Yes, we have to pass on the data to the multiplexer
		{
			std::map<TurtleFileHash,TurtleFileHashInfo>::const_iterator it( _incoming_file_hashes.find(tunnel.hash) ) ;
#ifdef P3TURTLE_DEBUG
			assert(!tunnel.hash.empty()) ;
#endif
			if(it==_incoming_file_hashes.end())
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "No tunnel for incoming data. Maybe the tunnel is being closed." << std::endl ;
#endif
				return ;
			}

			const TurtleFileHashInfo& hash_info(it->second) ;
#ifdef P3TURTLE_DEBUG
			std::cerr << "  This is an endpoint for this data chunk." << std::endl ;
			std::cerr << "  Forwarding data to the multiplexer." << std::endl ;
			std::cerr << "  using peer_id=" << tunnel.vpid << ", hash=" << tunnel.hash << std::endl ;
#endif
			//_ft_server->getMultiplexer()->recvData(tunnel.vpid,tunnel.hash,hash_info.size,item->chunk_offset,item->chunk_size,item->chunk_data) ;
			vpid = tunnel.vpid ;
			hash = tunnel.hash ;
			size = hash_info.size ;
		}
		else	// No, it's a request we should forward down the pipe.
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Forwarding data chunk to peer " << tunnel.local_src << std::endl ;
#endif
			RsTurtleFileDataItem *res_item = new RsTurtleFileDataItem(*item) ;

			res_item->chunk_data = malloc(res_item->chunk_size) ;

			if(res_item->chunk_data == NULL)
			{
				std::cerr << "p3turtle: Warning: failed malloc of " << res_item->chunk_size << " bytes for received data packet." << std::endl ;
				return ;
			}
			memcpy(res_item->chunk_data,item->chunk_data,res_item->chunk_size) ;

			res_item->PeerId(tunnel.local_src) ;

			sendItem(res_item) ;
			return ;
		}
	}
	_ft_server->getMultiplexer()->recvData(vpid,hash,size,item->chunk_offset,item->chunk_size,item->chunk_data) ;
	item->chunk_data = NULL ;	// this prevents deletion in the destructor of RsFileDataItem, because data will be deleted
										// down _ft_server->getMultiplexer()->recvData()...in ftTransferModule::recvFileData
}

// Send a data request into the correct tunnel for the given file hash
void p3turtle::sendDataRequest(const std::string& peerId, const std::string& hash, uint64_t, uint64_t offset, uint32_t chunksize)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	// get the proper tunnel for this file hash and peer id.
	std::map<TurtleVirtualPeerId,TurtleTunnelId>::const_iterator it(_virtual_peers.find(peerId)) ;

	if(it == _virtual_peers.end())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "p3turtle::senddataRequest: cannot find virtual peer " << peerId << " in VP list." << std::endl ;
#endif
		return ;
	}
	TurtleTunnelId tunnel_id = it->second ;
	TurtleTunnel& tunnel(_local_tunnels[tunnel_id]) ;

//	tunnel.time_stamp = time(NULL) ;

#ifdef P3TURTLE_DEBUG
	assert(hash == tunnel.hash) ;
#endif
	RsTurtleFileRequestItem *item = new RsTurtleFileRequestItem ;
	item->tunnel_id = tunnel_id ;	// we should randomly select a tunnel, or something more clever.
	item->chunk_offset = offset ;
	item->chunk_size = chunksize ;
	item->PeerId(tunnel.local_dst) ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: sending file req (chunksize=" << item->chunk_size << ", offset=" << item->chunk_offset << ", hash=0x" << hash << ") through tunnel " << (void*)item->tunnel_id << ", next peer=" << tunnel.local_dst << std::endl ;
#endif
	sendItem(item) ;
}

// Send file data into the correct tunnel for the given file hash
void p3turtle::sendFileData(const std::string& peerId, const std::string& hash, uint64_t, uint64_t offset, uint32_t chunksize, void *data)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	// get the proper tunnel for this file hash and peer id.
	std::map<TurtleVirtualPeerId,TurtleTunnelId>::const_iterator it(_virtual_peers.find(peerId)) ;

	if(it == _virtual_peers.end())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "p3turtle::sendData: cannot find virtual peer " << peerId << " in VP list." << std::endl ;
#endif
		return ;
	}
	TurtleTunnelId tunnel_id = it->second ;
	TurtleTunnel& tunnel(_local_tunnels[tunnel_id]) ;

	tunnel.time_stamp = time(NULL) ;
#ifdef P3TURTLE_DEBUG
	assert(hash == tunnel.hash) ;
#endif
	RsTurtleFileDataItem *item = new RsTurtleFileDataItem ;
	item->tunnel_id = tunnel_id ;
	item->chunk_offset = offset ;
	item->chunk_size = chunksize ;
	item->chunk_data = malloc(chunksize) ;

	if(item->chunk_data == NULL)
	{
		std::cerr << "p3turtle: Warning: failed malloc of " << chunksize << " bytes for sending data packet." << std::endl ;
		return ;
	}
	memcpy(item->chunk_data,(void*)((uint8_t*)data),chunksize) ;
	item->PeerId(tunnel.local_src) ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: sending file data (chunksize=" << item->chunk_size << ", offset=" << item->chunk_offset << ", hash=0x" << hash << ") through tunnel " << (void*)item->tunnel_id << ", next peer=" << tunnel.local_src << std::endl ;
#endif
	sendItem(item) ;
}

bool p3turtle::search(std::string hash, uint64_t, uint32_t hintflags, FileInfo &info) const
{
	if(! (hintflags & RS_FILE_HINTS_TURTLE))		// this should not happen, but it's a security.
		return false;

	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: received file search request for hash " << hash << "." << std::endl ;
#endif

	std::map<TurtleFileHash,TurtleFileHashInfo>::const_iterator it = _incoming_file_hashes.find(hash) ;

	if(_incoming_file_hashes.end() != it)
	{
		info.fname = it->second.name;
		info.size = it->second.size;
		info.hash = it->first;

		for(unsigned int i=0;i<it->second.tunnels.size();++i)
		{
			TransferInfo ti;
			ti.peerId = getTurtlePeerId(it->second.tunnels[i]);
			ti.name = "Distant peer for hash=" + hash ;
			ti.tfRate = 0;
			info.peers.push_back(ti);
		}

#ifdef P3TURTLE_DEBUG
		std::cerr << "  Found these tunnels for that hash:. "<< std::endl ;
		for(unsigned int i=0;i<it->second.tunnels.size();++i)
			std::cerr << "    " << (void*)it->second.tunnels[i] << std::endl ;

		std::cerr << "  answered yes. "<< std::endl ;
#endif
		return true ;
	}
	else
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  responding false." << std::endl ;
#endif
		return false ;
	}
}

bool p3turtle::isTurtlePeer(const std::string& peer_id) const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	return _virtual_peers.find(peer_id) != _virtual_peers.end() ;

//	if(it->second.tunnels.empty())
//		return false ;
//
}

std::string p3turtle::getTurtlePeerId(TurtleTunnelId tid) const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	std::map<TurtleTunnelId,TurtleTunnel>::const_iterator it( _local_tunnels.find(tid) ) ;

#ifdef P3TURTLE_DEBUG
	assert(it!=_local_tunnels.end()) ;
	assert(it->second.vpid != "") ;
#endif

	return it->second.vpid ;
}

bool p3turtle::isOnline(const std::string& peer_id) const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	// we could do something mre clever here...
	//
	return _virtual_peers.find(peer_id) != _virtual_peers.end() ;
}


// -----------------------------------------------------------------------------------//
// --------------------------------  Tunnel handling. ------------------------------- //
// -----------------------------------------------------------------------------------//
//

TurtleRequestId p3turtle::diggTunnel(const TurtleFileHash& hash)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "performing tunnel request. OwnId = " << mConnMgr->getOwnId() << std::endl ;
#endif
	while(mConnMgr->getOwnId() == "")
	{
		std::cerr << "... waitting for connect manager to form own id." << std::endl ;
#ifdef WIN32
		Sleep(1000) ;
#else
		sleep(1) ;
#endif
	}

	TurtleRequestId id = generateRandomRequestId() ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
		// Store the request id, so that we can find the hash back when we get the response.
		//
		_incoming_file_hashes[hash].last_request = id ;
	}

	// Form a tunnel request packet that simulates a request from us.
	//
	RsTurtleOpenTunnelItem *item = new RsTurtleOpenTunnelItem ;

	item->PeerId(mConnMgr->getOwnId()) ;
	item->file_hash = hash ;
	item->request_id = id ;
	item->partial_tunnel_id = generatePersonalFilePrint(hash,true) ;
	item->depth = 0 ;

	// send it

	handleTunnelRequest(item) ;

	delete item ;

	return id ;
}

void p3turtle::handleTunnelRequest(RsTurtleOpenTunnelItem *item)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "Received tunnel request from peer " << item->PeerId() << ": " << std::endl ;
	item->print(std::cerr,0) ;
#endif
	// If the item contains an already handled tunnel request, give up.  This
	// happens when the same tunnel request gets relayed by different peers
	//
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		if(_tunnel_requests_origins.find(item->request_id) != _tunnel_requests_origins.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  This is a bouncing request. Ignoring and deleting item." << std::endl ;
#endif
			return ;
		}

		// This is a new request. Let's add it to the request map, and forward it to
		// open peers.

		TurtleRequestInfo& req( _tunnel_requests_origins[item->request_id] ) ;
		req.origin = item->PeerId() ;
		req.time_stamp = time(NULL) ;

		// If it's not for us, perform a local search. If something found, forward the search result back.

		if(item->PeerId() != mConnMgr->getOwnId())
		{
			FileInfo info ;
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Request not from us. Performing local search" << std::endl ;
#endif
			if((_sharing_strategy != SHARE_FRIENDS_ONLY || item->depth < 2) && performLocalHashSearch(item->file_hash,info))
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  Local hash found. Sending tunnel ok to origin (" << item->PeerId() << ")." << std::endl ;
#endif
				// Send back tunnel ok to the same guy
				//
				RsTurtleTunnelOkItem *res_item = new RsTurtleTunnelOkItem ;

				res_item->request_id = item->request_id ;
				res_item->tunnel_id = item->partial_tunnel_id ^ generatePersonalFilePrint(item->file_hash,false) ;
				res_item->PeerId(item->PeerId()) ;

				sendItem(res_item) ;

				// Note in the tunnels list that we have an ending tunnel here.
				TurtleTunnel tt ;
				tt.local_src = item->PeerId() ;
				tt.hash = item->file_hash ;
				tt.local_dst = mConnMgr->getOwnId() ;	// this means us
				tt.time_stamp = time(NULL) ;

				_local_tunnels[res_item->tunnel_id] = tt ;

				// We add a virtual peer for that tunnel+hash combination.
				//
				addDistantPeer(item->file_hash,res_item->tunnel_id) ;

				// Store the size of the file, to be able to re-form data requests to the multiplexer.
				//
				_outgoing_file_hashes[item->file_hash] = info ;

				// We return straight, because when something is found, there's no need to digg a tunnel further.
				return ;
			}
#ifdef P3TURTLE_DEBUG
			else
				std::cerr << "  No hash found locally, or local file not allowed for distant peers. Forwarding. " << std::endl ;
#endif
		}
	}

	// If search depth not too large, also forward this search request to all other peers.
	//
	if(item->depth < TURTLE_MAX_SEARCH_DEPTH)
	{
		std::list<std::string> onlineIds ;
		mConnMgr->getOnlineList(onlineIds);
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Forwarding tunnel request: Looking for online peers" << std::endl ;
#endif

		for(std::list<std::string>::const_iterator it(onlineIds.begin());it!=onlineIds.end();++it)
			if(*it != item->PeerId())
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  Forwarding request to peer = " << *it << std::endl ;
#endif
				// Copy current item and modify it.
				RsTurtleOpenTunnelItem *fwd_item = new RsTurtleOpenTunnelItem(*item) ;

				++(fwd_item->depth) ;		// increase tunnel depth
				fwd_item->PeerId(*it) ;

				sendItem(fwd_item) ;
			}
	}
#ifdef P3TURTLE_DEBUG
	else
		std::cout << "  Dropping this item, as tunnel depth is " << item->depth << std::endl ;
#endif
}

void p3turtle::handleTunnelResult(RsTurtleTunnelOkItem *item)
{
	bool new_tunnel = false ;
	TurtleFileHash new_hash ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		// Find who actually sent the corresponding turtle tunnel request.
		//
		std::map<TurtleTunnelRequestId,TurtleRequestInfo>::const_iterator it = _tunnel_requests_origins.find(item->request_id) ;
#ifdef P3TURTLE_DEBUG
		std::cerr << "Received tunnel result:" << std::endl ;
		item->print(std::cerr,0) ;
#endif
		if(it == _tunnel_requests_origins.end())
		{
			// This is an error: how could we receive a tunnel result corresponding to a tunnel item we
			// have forwarded but that it not in the list ??

			std::cerr << __PRETTY_FUNCTION__ << ": tunnel result has no peer direction!" << std::endl ;
			return ;
		}

		// store tunnel info.
		bool found = (_local_tunnels.find(item->tunnel_id) != _local_tunnels.end()) ;
		TurtleTunnel& tunnel(_local_tunnels[item->tunnel_id]) ;

		if(found)
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "Tunnel id " << (void*)item->tunnel_id << " is already there. Not storing." << std::endl ;
#endif
		}
		else
		{
			tunnel.local_src = it->second.origin ;
			tunnel.local_dst = item->PeerId() ;
			tunnel.hash = "" ;
			tunnel.time_stamp = time(NULL) ;

#ifdef P3TURTLE_DEBUG
			std::cerr << "  storing tunnel info. src=" << tunnel.local_src << ", dst=" << tunnel.local_dst << ", id=" << item->tunnel_id << std::endl ;
#endif
		}

		// Is this result's target actually ours ?

		if(it->second.origin == mConnMgr->getOwnId())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Tunnel starting point. Storing id=" << (void*)item->tunnel_id << " for hash (unknown) and tunnel request id " << it->second.origin << std::endl;
#endif
			// Tunnel is ending here. Add it to the list of tunnels for the given hash.

			// 1 - find which file hash issued this request. This is not costly, because there is not too much file hashes to be active
			// 	at a time, and this mostly prevents from sending the hash back in the tunnel.

			bool found = false ;
			for(std::map<TurtleFileHash,TurtleFileHashInfo>::iterator it(_incoming_file_hashes.begin());it!=_incoming_file_hashes.end();++it)
				if(it->second.last_request == item->request_id)
				{
					found = true ;

					// add the tunnel uniquely
					bool found = false ;

					for(unsigned int j=0;j<it->second.tunnels.size();++j)
						if(it->second.tunnels[j] == item->tunnel_id)
							found = true ;

					if(!found)
						it->second.tunnels.push_back(item->tunnel_id) ;

					tunnel.hash = it->first ;		// because it's a local tunnel

					// Adds a virtual peer to the list of online peers.
					// We do this later, because of the mutex protection.
					//
					new_tunnel = true ;
					new_hash = it->first ;

					addDistantPeer(new_hash,item->tunnel_id) ;
				}
			if(!found)
				std::cerr << "p3turtle: error. Could not find hash that emmitted tunnel request " << (void*)item->tunnel_id << std::endl ;
		}
		else
		{											// Nope, forward it back.
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Forwarding result back to " << it->second.origin << std::endl;
#endif
			RsTurtleTunnelOkItem *fwd_item = new RsTurtleTunnelOkItem(*item) ;	// copy the item
			fwd_item->PeerId(it->second.origin) ;

			sendItem(fwd_item) ;
		}
	}

	// A new tunnel has been created. Add the corresponding virtual peer to the list, and
	// notify the file transfer controller for the new file source. This should be done off-mutex
	// so we deported this code here.
	//
	if(new_tunnel)
	{
		_ft_controller->addFileSource(new_hash,_local_tunnels[item->tunnel_id].vpid) ;
		_ft_controller->statusChange(_online_peers) ;
	}
}

// -----------------------------------------------------------------------------------//
// ------------------------------  IO with libretroshare  ----------------------------//
// -----------------------------------------------------------------------------------//
//
void RsTurtleStringSearchRequestItem::performLocalSearch(std::list<TurtleFileInfo>& result) const
{
	/* call to core */
	std::list<DirDetails> initialResults;
	std::list<std::string> words ;

	// to do: split search string into words.
	words.push_back(match_string) ;

	// now, search!
	rsFiles->SearchKeywords(words, initialResults,DIR_FLAGS_LOCAL | DIR_FLAGS_NETWORK_WIDE);

	result.clear() ;

	for(std::list<DirDetails>::const_iterator it(initialResults.begin());it!=initialResults.end();++it)
	{
		// retain only file type
		if (it->type == DIR_TYPE_DIR) continue;

		TurtleFileInfo i ;
		i.hash = it->hash ;
		i.size = it->count ;
		i.name = it->name ;

		result.push_back(i) ;
	}
}
void RsTurtleRegExpSearchRequestItem::performLocalSearch(std::list<TurtleFileInfo>& result) const
{
	/* call to core */
	std::list<DirDetails> initialResults;

	// to do: split search string into words.
	Expression *exp = LinearizedExpression::toExpr(expr) ;

	// now, search!
	rsFiles->SearchBoolExp(exp,initialResults,DIR_FLAGS_LOCAL | DIR_FLAGS_NETWORK_WIDE);

	result.clear() ;

	for(std::list<DirDetails>::const_iterator it(initialResults.begin());it!=initialResults.end();++it)
	{
		TurtleFileInfo i ;
		i.hash = it->hash ;
		i.size = it->count ;
		i.name = it->name ;

		result.push_back(i) ;
	}
	delete exp ;
}

TurtleRequestId p3turtle::turtleSearch(const std::string& string_to_match)
{
	// generate a new search id.

	TurtleRequestId id = generateRandomRequestId() ;

	// Form a request packet that simulates a request from us.
	//
	RsTurtleStringSearchRequestItem *item = new RsTurtleStringSearchRequestItem ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "performing search. OwnId = " << mConnMgr->getOwnId() << std::endl ;
#endif
	while(mConnMgr->getOwnId() == "")
	{
		std::cerr << "... waitting for connect manager to form own id." << std::endl ;
#ifdef WIN32
		Sleep(1000) ;
#else
		sleep(1) ;
#endif
	}

	item->PeerId(mConnMgr->getOwnId()) ;
	item->match_string = string_to_match ;
	item->request_id = id ;
	item->depth = 0 ;

	// send it

	handleSearchRequest(item) ;

	delete item ;

	return id ;
}
TurtleRequestId p3turtle::turtleSearch(const LinearizedExpression& expr)
{
	// generate a new search id.

	TurtleRequestId id = generateRandomRequestId() ;

	// Form a request packet that simulates a request from us.
	//
	RsTurtleRegExpSearchRequestItem *item = new RsTurtleRegExpSearchRequestItem ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "performing search. OwnId = " << mConnMgr->getOwnId() << std::endl ;
#endif
	while(mConnMgr->getOwnId() == "")
	{
		std::cerr << "... waitting for connect manager to form own id." << std::endl ;
#ifdef WIN32
		Sleep(1000) ;
#else
		sleep(1) ;
#endif
	}

	item->PeerId(mConnMgr->getOwnId()) ;
	item->expr = expr ;
	item->request_id = id ;
	item->depth = 0 ;

	// send it

	handleSearchRequest(item) ;

	delete item ;

	return id ;
}

void p3turtle::monitorFileTunnels(const std::string& name,const std::string& file_hash,uint64_t size)
{
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		if(_incoming_file_hashes.find(file_hash) != _incoming_file_hashes.end())	// download already asked.
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: File hash " << file_hash << " already in pool. Returning." << std::endl ;
#endif
			return ;
		}
#ifdef P3TURTLE_DEBUG
		std::cerr << "p3turtle: Received order for turtle download fo hash " << file_hash << std::endl ;
#endif

		// No tunnels at start, but this triggers digging new tunnels.
		//
		_incoming_file_hashes[file_hash].tunnels.clear();
		_force_digg_new_tunnels = true ;

		// also should send associated request to the file transfer module.
		_incoming_file_hashes[file_hash].size = size ;
		_incoming_file_hashes[file_hash].name = name ;
		_incoming_file_hashes[file_hash].time_stamp = time(NULL) ;
	}

	IndicateConfigChanged() ;	// initiates saving of handled hashes.
}

void p3turtle::returnSearchResult(RsTurtleSearchResultItem *item)
{
	// just cout for now, but it should be notified to the gui

#ifdef P3TURTLE_DEBUG
	std::cerr << "  Returning result for search request " << (void*)item->request_id << " upwards." << std::endl ;
#endif

	rsicontrol->getNotify().notifyTurtleSearchResult(item->request_id,item->result) ;
}

bool p3turtle::performLocalHashSearch(const TurtleFileHash& hash,FileInfo& info)
{
	return rsFiles->FileDetails(hash, RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_SPEC_ONLY, info);
}

static std::string printNumber(uint64_t num,bool hex=false)
{
	if(hex)
	{
		char tmp[100] ;
		sprintf(tmp,"0x%08lx",num) ;
		return std::string(tmp) ;
	}
	else
	{
		std::ostringstream out ;
		out << num ;
		return out.str() ;
	}
}

void p3turtle::getInfo(	std::vector<std::vector<std::string> >& hashes_info,
								std::vector<std::vector<std::string> >& tunnels_info,
								std::vector<std::vector<std::string> >& search_reqs_info,
								std::vector<std::vector<std::string> >& tunnel_reqs_info) const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	time_t now = time(NULL) ;

	hashes_info.clear() ;

	for(std::map<TurtleFileHash,TurtleFileHashInfo>::const_iterator it(_incoming_file_hashes.begin());it!=_incoming_file_hashes.end();++it)
	{
		hashes_info.push_back(std::vector<std::string>()) ;

		std::vector<std::string>& hashes(hashes_info.back()) ;

		hashes.push_back(it->first) ;
		hashes.push_back(it->second.name) ;
		hashes.push_back(printNumber(it->second.tunnels.size())) ;
		hashes.push_back(printNumber(now - it->second.time_stamp)+" secs ago") ;
	}
#ifdef A_VOIR
	for(std::map<TurtleFileHash,FileInfo>::const_iterator it(_outgoing_file_hashes.begin());it!=_outgoing_file_hashes.end();++it)
		std::cerr << "    hash=0x" << it->first << ", name=" << it->second.fname << ", size=" << it->second.size << std::endl ;
#endif
	tunnels_info.clear();

	for(std::map<TurtleTunnelId,TurtleTunnel>::const_iterator it(_local_tunnels.begin());it!=_local_tunnels.end();++it)
	{
		tunnels_info.push_back(std::vector<std::string>()) ;
		std::vector<std::string>& tunnel(tunnels_info.back()) ;

		tunnel.push_back(printNumber(it->first,true)) ;
		tunnel.push_back(rsPeers->getPeerName(it->second.local_src)) ;
		tunnel.push_back(rsPeers->getPeerName(it->second.local_dst)) ;
		tunnel.push_back(it->second.hash) ;
		tunnel.push_back(printNumber(now-it->second.time_stamp) + " secs ago") ;
	}

	search_reqs_info.clear();

	for(std::map<TurtleSearchRequestId,TurtleRequestInfo>::const_iterator it(_search_requests_origins.begin());it!=_search_requests_origins.end();++it)
	{
		search_reqs_info.push_back(std::vector<std::string>()) ;
		std::vector<std::string>& search_req(search_reqs_info.back()) ;

		search_req.push_back(printNumber(it->first,true)) ;
		search_req.push_back(rsPeers->getPeerName(it->second.origin)) ;
		search_req.push_back(printNumber(now - it->second.time_stamp) + " secs ago") ;
	}

	tunnel_reqs_info.clear();

	for(std::map<TurtleSearchRequestId,TurtleRequestInfo>::const_iterator it(_tunnel_requests_origins.begin());it!=_tunnel_requests_origins.end();++it)
	{
		tunnel_reqs_info.push_back(std::vector<std::string>()) ;
		std::vector<std::string>& tunnel_req(tunnel_reqs_info.back()) ;

		tunnel_req.push_back(printNumber(it->first,true)) ;
		tunnel_req.push_back(rsPeers->getPeerName(it->second.origin)) ;
		tunnel_req.push_back(printNumber(now - it->second.time_stamp) + " secs ago") ;
	}
}

#ifdef P3TURTLE_DEBUG
void p3turtle::dumpState()
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	time_t now = time(NULL) ;

	std::cerr << std::endl ;
	std::cerr << "********************** Turtle router dump ******************" << std::endl ;
	std::cerr << "  Active incoming file hashes: " << _incoming_file_hashes.size() << std::endl ;
	for(std::map<TurtleFileHash,TurtleFileHashInfo>::const_iterator it(_incoming_file_hashes.begin());it!=_incoming_file_hashes.end();++it)
	{
		std::cerr << "    hash=0x" << it->first << ", name=" << it->second.name << ", size=" << it->second.size << ", tunnel ids =" ;
		for(std::vector<TurtleTunnelId>::const_iterator it2(it->second.tunnels.begin());it2!=it->second.tunnels.end();++it2)
			std::cerr << " " << (void*)*it2 ;
		std::cerr << ", last_req=" << (void*)it->second.last_request << ", time_stamp = " << it->second.time_stamp << "(" << now-it->second.time_stamp << " secs ago)" << std::endl ;
	}
	std::cerr << "  Active outgoing file hashes: " << _outgoing_file_hashes.size() << std::endl ;
	for(std::map<TurtleFileHash,FileInfo>::const_iterator it(_outgoing_file_hashes.begin());it!=_outgoing_file_hashes.end();++it)
		std::cerr << "    hash=0x" << it->first << ", name=" << it->second.fname << ", size=" << it->second.size << std::endl ;

	std::cerr << "  Local tunnels:" << std::endl ;
	for(std::map<TurtleTunnelId,TurtleTunnel>::const_iterator it(_local_tunnels.begin());it!=_local_tunnels.end();++it)
		std::cerr << "    " << (void*)it->first << ": from="
					<< it->second.local_src << ", to=" << it->second.local_dst
					<< ", hash=0x" << it->second.hash << ", ts=" << it->second.time_stamp << " (" << now-it->second.time_stamp << " secs ago)"
					<< ", peer id =" << it->second.vpid << std::endl ;

	std::cerr << "  buffered request origins: " << std::endl ;
	std::cerr << "    Search requests: " << _search_requests_origins.size() << std::endl ;

	for(std::map<TurtleSearchRequestId,TurtleRequestInfo>::const_iterator it(_search_requests_origins.begin());it!=_search_requests_origins.end();++it)
		std::cerr 	<< "      " << (void*)it->first << ": from=" << it->second.origin
						<< ", ts=" << it->second.time_stamp << " (" << now-it->second.time_stamp
						<< " secs ago)" << std::endl ;

	std::cerr << "    Tunnel requests: " << _tunnel_requests_origins.size() << std::endl ;
	for(std::map<TurtleTunnelRequestId,TurtleRequestInfo>::const_iterator it(_tunnel_requests_origins.begin());it!=_tunnel_requests_origins.end();++it)
		std::cerr 	<< "      " << (void*)it->first << ": from=" << it->second.origin
						<< ", ts=" << it->second.time_stamp << " (" << now-it->second.time_stamp
						<< " secs ago)" << std::endl ;

	std::cerr << "  Virtual peers:" << std::endl ;
	for(std::map<TurtleVirtualPeerId,TurtleTunnelId>::const_iterator it(_virtual_peers.begin());it!=_virtual_peers.end();++it)
		std::cerr << "    id=" << it->first << ", tunnel=" << (void*)(it->second) << std::endl ;
	std::cerr << "  Online peers: " << std::endl ;
	for(std::list<pqipeer>::const_iterator it(_online_peers.begin());it!=_online_peers.end();++it)
		std::cerr << "    id=" << it->id << ", name=" << it->name << ", state=" << it->state << ", actions=" << it->actions << std::endl ;
}
#endif

