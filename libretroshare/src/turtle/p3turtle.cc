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

//#define P3TURTLE_DEBUG 

#include <stdexcept>
#include <stdlib.h>
#include <assert.h>
#ifdef P3TURTLE_DEBUG
#include <assert.h>
#endif

#include "retroshare/rsiface.h"

#include "pqi/authssl.h"
#include "pqi/p3linkmgr.h"
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
#include "util/rsrandom.h"
#include "pqi/pqinetwork.h"

#ifdef TUNNEL_STATISTICS
static std::vector<int> TS_tunnel_length(8,0) ;
static std::map<TurtleFileHash, std::vector<std::pair<time_t,TurtleTunnelRequestId> > > TS_request_time_stamps ;
static std::map<TurtleTunnelRequestId, std::vector<time_t> > TS_request_bounces ;
void TS_dumpState() ;
#endif

// These number may be quite important. I setup them with sensible values, but
// an in-depth test would be better to get an idea of what the ideal values
// could ever be.
//
// update of 14-03-11: 
// 	- I raised the cache time for tunnel requests. This avoids inconsistencies such as:
// 		* tunnel requests bouncing back while the original request is not in the cache anymore
// 		* special case of this for own file transfer: an outgoing tunnel is built with no end.
// 	- changed tunnel speed estimate time lapse to 5 secs. Too small a value favors high variations.
// 	- the max number of tunnel requests per second is now enforced. It was before defaulting to
// 	  QUEUE_LENGTH*0.1, meaning 0.5. I set it to 0.1.
//
static const time_t TUNNEL_REQUESTS_LIFE_TIME 	=  60 ;		/// life time for tunnel requests in the cache.
static const time_t SEARCH_REQUESTS_LIFE_TIME 	=  60 ;		/// life time for search requests in the cache
static const time_t REGULAR_TUNNEL_DIGGING_TIME = 300 ;		/// maximum interval between two tunnel digging campaigns.
static const time_t MAXIMUM_TUNNEL_IDLE_TIME 	=  60 ;		/// maximum life time of an unused tunnel.
static const time_t EMPTY_TUNNELS_DIGGING_TIME 	=  50 ;		/// look into tunnels regularly every 50 sec.
static const time_t TUNNEL_SPEED_ESTIMATE_LAPSE	=   5 ;		/// estimate tunnel speed every 5 seconds
static const time_t TUNNEL_CLEANING_LAPS_TIME  	=  10 ;		/// clean tunnels every 10 secs
static const uint32_t MAX_TUNNEL_REQS_PER_SECOND=   1 ;		/// maximum number of tunnel requests issued per second. Was 0.5 before
static const uint32_t MAX_ALLOWED_SR_IN_CACHE   = 120 ;		/// maximum number of search requests allowed in cache. That makes 2 per sec.

static const float depth_peer_probability[7] = { 1.0f,0.99f,0.9f,0.7f,0.4f,0.15f,0.1f } ;
static const int TUNNEL_REQUEST_PACKET_SIZE 	= 50 ;
static const int MAX_TR_FORWARD_PER_SEC 		= 20 ;
static const int DISTANCE_SQUEEZING_POWER 	= 8 ;

p3turtle::p3turtle(p3LinkMgr *lm,ftServer *fs)
	:p3Service(RS_SERVICE_TYPE_TURTLE), p3Config(CONFIG_TYPE_TURTLE), mLinkMgr(lm), mTurtleMtx("p3turtle")
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	_ft_server = fs ;
	_ft_controller = fs->getController() ;

	_random_bias = RSRandom::random_u32() ;

	addSerialType(new RsTurtleSerialiser());

	_last_clean_time = 0 ;
	_last_tunnel_management_time = 0 ;
	_last_tunnel_campaign_time = 0 ;
	_last_tunnel_speed_estimate_time = 0 ;

	_traffic_info.reset() ;
	_sharing_strategy = SHARE_ENTIRE_NETWORK ;
}

int p3turtle::tick()
{
	// Handle tunnel trafic
	//
	handleIncoming();		// handle incoming packets

	time_t now = time(NULL) ;

#ifdef TUNNEL_STATISTICS
	static time_t last_now = now ;
	if(now - last_now > 2)
		std::cerr << "******************* WARNING: now - last_now = " << now - last_now << std::endl;
	last_now = now ;
#endif

	bool should_autowash,should_estimatespeed ;
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		should_autowash 			= now > TUNNEL_CLEANING_LAPS_TIME+_last_clean_time ;
		should_estimatespeed 	= now >= TUNNEL_SPEED_ESTIMATE_LAPSE + _last_tunnel_speed_estimate_time ;
	}

	// Tunnel management:
	// 	- we digg new tunnels at least every 5 min (300 sec).
	// 	- we digg new tunnels each time a new peer connects
	// 	- we digg new tunnels each time a new hash is asked for
	//
	if(now >= _last_tunnel_management_time+1)	// call every second
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "Calling tunnel management." << std::endl ;
#endif
		manageTunnels() ;

		{
			RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
			_last_tunnel_management_time = now ;

			// Update traffic statistics. The constants are important: they allow a smooth variation of the 
			// traffic speed, which is used to moderate tunnel requests statistics.
			//
			_traffic_info = _traffic_info*0.9 + _traffic_info_buffer*0.1 ;
			_traffic_info_buffer.reset() ;
		}
	}

	// Clean every 10 sec.
	//
	if(should_autowash)
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "Calling autowash." << std::endl ;
#endif
		autoWash() ;					// clean old/unused tunnels and file hashes, as well as search and tunnel requests.

		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
		_last_clean_time = now ;
	}

	if(should_estimatespeed)
	{
		estimateTunnelSpeeds() ;

		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
		_last_tunnel_speed_estimate_time = now ;
	}

#ifdef TUNNEL_STATISTICS
	// Dump state for debugging, every 20 sec.
	//
	static time_t TS_last_dump = time(NULL) ;

	if(now > 20+TS_last_dump)
	{
		TS_last_dump = now ;
		TS_dumpState() ;
	}
#endif

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

#ifdef TO_REMOVE
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
#endif

// adds a virtual peer to the list that is communicated ot ftController.
//
void p3turtle::locked_addDistantPeer(const TurtleFileHash&,TurtleTunnelId tid)
{
	char buff[400] ;
	sprintf(buff,"Anonymous F2F tunnel %08x",tid) ;

	_virtual_peers[TurtleVirtualPeerId(buff)] = tid ;
#ifdef P3TURTLE_DEBUG
	assert(_local_tunnels.find(tid)!=_local_tunnels.end()) ;
#endif
	_local_tunnels[tid].vpid = TurtleVirtualPeerId(buff) ;
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
class hashPairComparator
{
	public:
		virtual bool operator()(const std::pair<TurtleFileHash,time_t>& p1,const std::pair<TurtleFileHash,time_t>& p2) const
		{
			return p1.second < p2.second ;
		}
};
void p3turtle::manageTunnels()
{
	// Collect hashes for which tunnel digging is necessary / recommended. Hashes get in the list for two reasons:
	//  - the hash has no tunnel -> tunnel digging every EMPTY_TUNNELS_DIGGING_TIME seconds
	//  - the hash hasn't been tunneled for more than REGULAR_TUNNEL_DIGGING_TIME seconds, even if downloading.
	//
	// Candidate hashes are sorted, by olderness. The older gets tunneled first. At most MAX_TUNNEL_REQS_PER_SECOND are
	// treated at once, as this method is called every second. 
	// Note: Because REGULAR_TUNNEL_DIGGING_TIME is larger than EMPTY_TUNNELS_DIGGING_TIME, files being downloaded get 
	// re-tunneled in priority. As this happens less, they don't obliterate tunneling for files that have no tunnels yet.

	std::vector<std::pair<TurtleFileHash,time_t> > hashes_to_digg ;
	time_t now = time(NULL) ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		// digg new tunnels if no tunnels are available and force digg new tunnels at regular (large) interval
		//
		for(std::map<TurtleFileHash,TurtleFileHashInfo>::const_iterator it(_incoming_file_hashes.begin());it!=_incoming_file_hashes.end();++it)
			if( (it->second.tunnels.empty() && now >= it->second.last_digg_time+EMPTY_TUNNELS_DIGGING_TIME) || now >= it->second.last_digg_time + REGULAR_TUNNEL_DIGGING_TIME)	
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "pushed hash " << it->first << ", for digging. Old = " << now - it->second.last_digg_time << std::endl;
#endif
				hashes_to_digg.push_back(std::pair<TurtleFileHash,time_t>(it->first,it->second.last_digg_time)) ;
			}
	}
#ifdef TUNNEL_STATISTICS
	std::cerr << hashes_to_digg.size() << " hashes candidate for tunnel digging." << std::endl;
#endif

	std::sort(hashes_to_digg.begin(),hashes_to_digg.end(),hashPairComparator()) ;

	for(unsigned int i=0;i<MAX_TUNNEL_REQS_PER_SECOND && i<hashes_to_digg.size();++i)// Digg at most n tunnels per second.
	{
#ifdef TUNNEL_STATISTICS
		std::cerr << "!!!!!!!!!!!! Digging for " << hashes_to_digg[i].first << std::endl;
#endif
		diggTunnel(hashes_to_digg[i].first) ;
	}
}

void p3turtle::estimateTunnelSpeeds()
{
	RsStackMutex stack(mTurtleMtx) ;

	for(std::map<TurtleTunnelId,TurtleTunnel>::iterator it(_local_tunnels.begin());it!=_local_tunnels.end();++it)
	{
		TurtleTunnel& tunnel(it->second) ;

		float speed_estimate = tunnel.transfered_bytes / float(TUNNEL_SPEED_ESTIMATE_LAPSE) ;
		tunnel.speed_Bps = 0.75*tunnel.speed_Bps + 0.25*speed_estimate ;
		tunnel.transfered_bytes = 0 ;
	}
}

void p3turtle::autoWash()
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  In autowash." << std::endl ;
#endif
	// Remove hashes that are marked as such.
	//

	std::vector<std::pair<TurtleFileHash,TurtleVirtualPeerId> > peers_to_remove ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		for(unsigned int i=0;i<_hashes_to_remove.size();++i)
		{
			std::map<TurtleFileHash,TurtleFileHashInfo>::iterator it(_incoming_file_hashes.find(_hashes_to_remove[i])) ;

			if(it == _incoming_file_hashes.end())
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "p3turtle: asked to stop monitoring file hash " << _hashes_to_remove[i] << ", but this hash is actually not handled by the turtle router." << std::endl ;
#endif
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
				locked_closeTunnel(tunnels_to_remove[k],peers_to_remove) ;

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

		for(std::map<TurtleSearchRequestId,TurtleRequestInfo>::iterator it(_search_requests_origins.begin());it!=_search_requests_origins.end();)
			if(now > (time_t)(it->second.time_stamp + SEARCH_REQUESTS_LIFE_TIME))
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  removed search request " << (void *)it->first << ", timeout." << std::endl ;
#endif
				std::map<TurtleSearchRequestId,TurtleRequestInfo>::iterator tmp(it) ;
				++tmp ;
				_search_requests_origins.erase(it) ;
				it = tmp ;
			}
			else
				++it;
	}

	// Tunnel requests
	//
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		for(std::map<TurtleTunnelRequestId,TurtleRequestInfo>::iterator it(_tunnel_requests_origins.begin());it!=_tunnel_requests_origins.end();)
			if(now > (time_t)(it->second.time_stamp + TUNNEL_REQUESTS_LIFE_TIME))
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  removed tunnel request " << (void *)it->first << ", timeout." << std::endl ;
#endif
				std::map<TurtleTunnelRequestId,TurtleRequestInfo>::iterator tmp(it) ;
				++tmp ;
				_tunnel_requests_origins.erase(it) ;
				it = tmp ;
			}
			else
				++it ;
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
			locked_closeTunnel(tunnels_to_close[i],peers_to_remove) ;
	}

	// File hashes can only be removed by calling the 'stopMonitoringFileTunnels()' command.
	
	// All calls to _ft_controller are done off-mutex, to avoir cross-lock
	for(uint32_t i=0;i<peers_to_remove.size();++i)
		_ft_controller->removeFileSource(peers_to_remove[i].first,peers_to_remove[i].second) ;

}

void p3turtle::locked_closeTunnel(TurtleTunnelId tid,std::vector<std::pair<TurtleFileHash,TurtleVirtualPeerId> >& sources_to_remove)
{
	// This is closing a given tunnel, removing it from file sources, and from the list of tunnels of its
	// corresponding file hash. In the original turtle4privacy paradigm, they also send back and forward
	// tunnel closing commands. In our case, this is not necessary, because if a tunnel is closed somewhere, its
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

	if(it->second.local_src == mLinkMgr->getOwnId())	// this is a starting tunnel. We thus remove
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
		sources_to_remove.push_back(std::pair<TurtleFileHash,TurtleVirtualPeerId>(hash,vpid)) ;

		// Let's be cautious. Normally we should never be here without consistent information,
		// but still, this happens, rarely.
		//
		if(_virtual_peers.find(vpid) != _virtual_peers.end())  
			_virtual_peers.erase(_virtual_peers.find(vpid)) ;

		std::map<TurtleFileHash,TurtleFileHashInfo>::iterator it(_incoming_file_hashes.find(hash)) ;

		if(it != _incoming_file_hashes.end())
		{
			std::vector<TurtleTunnelId>& tunnels(it->second.tunnels) ;

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
	}
	else if(it->second.local_dst == mLinkMgr->getOwnId())	// This is a ending tunnel. We also remove the virtual peer id
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "    Tunnel is a ending point. Also removing associated outgoing hash." ;
#endif
		std::map<TurtleFileHash,FileInfo>::iterator itHash = _outgoing_file_hashes.find(it->second.hash);
		if(itHash != _outgoing_file_hashes.end())
			_outgoing_file_hashes.erase(itHash) ;
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

bool p3turtle::saveList(bool& cleanup, std::list<RsItem*>&)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: saving list..." << std::endl ;
#endif
	cleanup = true ;
	 ;
#ifdef TO_REMOVE
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
#endif
	return true ;
}

#ifdef TO_REMOVE
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
#endif


// -----------------------------------------------------------------------------------//
// --------------------------------  Helper functions  ------------------------------ //
// -----------------------------------------------------------------------------------//
//
uint32_t p3turtle::generateRandomRequestId()
{
	return RSRandom::random_u32() ;
}

uint32_t p3turtle::generatePersonalFilePrint(const TurtleFileHash& hash,bool b)
{
	// whatever cooking from the file hash and OwnId that cannot be recovered.
	// The only important thing is that the saem couple (hash,SSL id) produces the same tunnel
	// id. The result uses a boolean to allow generating non symmetric tunnel ids.

	std::string buff(hash + mLinkMgr->getOwnId()) ;
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
	int nhandled = 0;
	// While messages read
	//
	RsItem *item = NULL;

	while(NULL != (item = recvItem()))
	{
		nhandled++;

		RsTurtleGenericTunnelItem *gti = dynamic_cast<RsTurtleGenericTunnelItem *>(item) ;

		if(gti != NULL)
			routeGenericTunnelItem(gti) ;	/// Generic packets, that travel through established tunnels. 
		else			 							/// These packets should be destroyed by the client.
		{
			/// Special packets that require specific treatment, because tunnels do not exist for these packets.
			/// These packets are destroyed here, after treatment.
			//
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
				default:
																	std::cerr << "p3turtle::handleIncoming: Unknown packet subtype " << item->PacketSubType() << std::endl ;
			}
			delete item;
		}
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
	if(_search_requests_origins.size() > MAX_ALLOWED_SR_IN_CACHE)
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Dropping, because the search request cache is full." << std::endl ;
#endif
		std::cerr << "  More than " << MAX_ALLOWED_SR_IN_CACHE << " search request in cache. A peer is probably trying to flood your network See the depth charts to find him." << std::endl;
		return ;
	}

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
	req.depth = item->depth ;

	// If it's not for us, perform a local search. If something found, forward the search result back.

	if(item->PeerId() != mLinkMgr->getOwnId())
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
	// We use a random factor on the depth test that is biased by a mix between the session id and the partial tunnel id
	// to scramble a possible search-by-depth attack.
	//
	bool random_bypass = (item->depth == TURTLE_MAX_SEARCH_DEPTH && (((_random_bias ^ item->request_id)&0x7)==2)) ;
	bool random_dshift = (item->depth == 1                       && (((_random_bias ^ item->request_id)&0x7)==6)) ;

	if(item->depth < TURTLE_MAX_SEARCH_DEPTH || random_bypass)
	{
		std::list<std::string> onlineIds ;
		mLinkMgr->getOnlineList(onlineIds);
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

				// increase search depth, except in some rare cases, to prevent correlation between 
				// TR sniffing and friend names. The strategy is to not increase depth if the depth
				// is 1:
				// 	If B receives a TR of depth 1 from A, B cannot deduice that A is downloading the
				// 	file, since A might have shifted the depth.
				//
				if(!random_dshift)
					++(fwd_item->depth) ;		

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

	if(it->second.origin == mLinkMgr->getOwnId())
		returnSearchResult(item) ;		// Yes, so send upward.
	else
	{											// Nope, so forward it back.
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Forwarding result back to " << it->second.origin << std::endl;
#endif
		RsTurtleSearchResultItem *fwd_item = new RsTurtleSearchResultItem(*item) ;	// copy the item

		// Normally here, we should setup the forward adress, so that the owner's
		// of the files found can be further reached by a tunnel.

		fwd_item->PeerId(it->second.origin) ;
		fwd_item->depth = 2 + (rand() % 256) ; // obfuscate the depth for non immediate friends.

		sendItem(fwd_item) ;
	}
}

// -----------------------------------------------------------------------------------//
// ---------------------------------  File Transfer. -------------------------------- //
// -----------------------------------------------------------------------------------//

// Routing of turtle tunnel items in a generic manner. Most tunnel packets will use this function, except packets designed for 
// contructing the tunnels and searching, namely TurtleSearchRequests/Results and OpenTunnel/TunnelOkItems
//
// Only packets coming from handleIncoming() end up here, so this function is able to catch the transiting traffic.
//
void p3turtle::routeGenericTunnelItem(RsTurtleGenericTunnelItem *item)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3Turtle: treating generic tunnel item:" << std::endl ;
	item->print(std::cerr,1) ;
#endif
	RsTurtleGenericTunnelItem::Direction direction = item->travelingDirection() ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		// look for the tunnel id.
		//
		std::map<TurtleTunnelId,TurtleTunnel>::iterator it(_local_tunnels.find(item->tunnelId())) ;

		if(it == _local_tunnels.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: got file map with unknown tunnel id " << (void*)item->tunnelId() << std::endl ;
#endif
			delete item;
			return ;
		}

		TurtleTunnel& tunnel(it->second) ;

		// Only file data transfer updates tunnels time_stamp field, to avoid maintaining tunnel that are incomplete.
		if(item->shouldStampTunnel())
			tunnel.time_stamp = time(NULL) ;

		tunnel.transfered_bytes += static_cast<RsTurtleItem*>(item)->serial_size() ;

		// Let's figure out whether this packet is for us or not.

		if(direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT && tunnel.local_src != mLinkMgr->getOwnId())
		{														 	
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Forwarding generic item to peer " << tunnel.local_src << std::endl ;
#endif
			item->PeerId(tunnel.local_src) ;

			_traffic_info_buffer.unknown_updn_Bps += static_cast<RsTurtleItem*>(item)->serial_size() ;

			if(dynamic_cast<RsTurtleFileDataItem*>(item) != NULL)
				item->setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FORWARD_FILE_DATA) ;

			sendItem(item) ;
			return ;
		}

		if(direction == RsTurtleGenericTunnelItem::DIRECTION_SERVER && tunnel.local_dst != mLinkMgr->getOwnId())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Forwarding generic item to peer " << tunnel.local_dst << std::endl ;
#endif
			item->PeerId(tunnel.local_dst) ;

			_traffic_info_buffer.unknown_updn_Bps += static_cast<RsTurtleItem*>(item)->serial_size() ;

			sendItem(item) ;
			return ;
		}
	}

	// The packet was not forwarded, so it is for us. Let's treat it.
	// This is done off-mutex, to avoid various deadlocks
	//
	
	switch(item->PacketSubType())
	{
		case RS_TURTLE_SUBTYPE_FILE_REQUEST: 		handleRecvFileRequest(dynamic_cast<RsTurtleFileRequestItem *>(item)) ;
																break ;

		case RS_TURTLE_SUBTYPE_FILE_DATA : 			handleRecvFileData(dynamic_cast<RsTurtleFileDataItem *>(item)) ;
																break ;

		case RS_TURTLE_SUBTYPE_FILE_MAP : 			handleRecvFileMap(dynamic_cast<RsTurtleFileMapItem *>(item)) ;
																break ;

		case RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST:	handleRecvFileMapRequest(dynamic_cast<RsTurtleFileMapRequestItem *>(item)) ;
																break ;

		case RS_TURTLE_SUBTYPE_FILE_CRC : 			handleRecvFileCRC32Map(dynamic_cast<RsTurtleFileCrcItem *>(item)) ;
																break ;

		case RS_TURTLE_SUBTYPE_FILE_CRC_REQUEST:	handleRecvFileCRC32MapRequest(dynamic_cast<RsTurtleFileCrcRequestItem *>(item)) ;
																break ;
	default:
																std::cerr << "WARNING: Unknown packet type received: id=" << (void*)(item->PacketSubType()) << ". Is somebody trying to poison you ?" << std::endl ;
#ifdef P3TURTLE_DEBUG
																exit(-1) ;
#endif
	}

	delete item ;
}

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

		std::map<TurtleTunnelId,TurtleTunnel>::iterator it2(_local_tunnels.find(item->tunnel_id)) ;

		if(it2 == _local_tunnels.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: got file request with unknown tunnel id " << (void*)item->tunnel_id << std::endl ;
#endif
			return ;
		}

		TurtleTunnel& tunnel(it2->second) ;
		std::map<TurtleFileHash,FileInfo>::const_iterator it(_outgoing_file_hashes.find(tunnel.hash)) ;
#ifdef P3TURTLE_DEBUG
		assert(!tunnel.hash.empty()) ;
		assert(it != _outgoing_file_hashes.end()) ;

		std::cerr << "  This is an endpoint for this file request." << std::endl ;
		std::cerr << "  Forwarding data request to the multiplexer." << std::endl ;
		std::cerr << "  using peer_id=" << tunnel.vpid << ", hash=" << tunnel.hash << std::endl ;
#endif
		size = it->second.size ;
		vpid = tunnel.vpid ;
		hash = tunnel.hash ;
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
	TurtleVirtualPeerId vpid ;
	uint64_t size ;
	TurtleFileHash hash ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		_traffic_info_buffer.data_dn_Bps += static_cast<RsTurtleItem*>(item)->serial_size() ;

		std::map<TurtleTunnelId,TurtleTunnel>::iterator it2(_local_tunnels.find(item->tunnel_id)) ;

		if(it2 == _local_tunnels.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: got file data with unknown tunnel id " << (void*)item->tunnel_id << std::endl ;
#endif
			return ;
		}

		TurtleTunnel& tunnel(it2->second) ;
		std::map<TurtleFileHash,TurtleFileHashInfo>::iterator it( _incoming_file_hashes.find(tunnel.hash) ) ;
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

		// also update the hash time stamp to show that it's actually being downloaded.
		//it->second.time_stamp = time(NULL) ;
	}

	_ft_server->getMultiplexer()->recvData(vpid,hash,size,item->chunk_offset,item->chunk_size,item->chunk_data) ;
	item->chunk_data = NULL ;	// this prevents deletion in the destructor of RsFileDataItem, because data will be deleted
										// down _ft_server->getMultiplexer()->recvData()...in ftTransferModule::recvFileData
}

void p3turtle::handleRecvFileMapRequest(RsTurtleFileMapRequestItem *item)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3Turtle: received file Map item:" << std::endl ;
	item->print(std::cerr,1) ;
#endif
	std::string hash,vpid ;
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		std::map<TurtleTunnelId,TurtleTunnel>::iterator it2(_local_tunnels.find(item->tunnel_id)) ;

		if(it2 == _local_tunnels.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: got file data with unknown tunnel id " << (void*)item->tunnel_id << std::endl ;
#endif
			return ;
		}

		TurtleTunnel& tunnel(it2->second) ;
#ifdef P3TURTLE_DEBUG
		assert(!tunnel.hash.empty()) ;

		std::cerr << "  This is an endpoint for this file map request." << std::endl ;
		std::cerr << "  Forwarding data to the multiplexer." << std::endl ;
		std::cerr << "  using peer_id=" << tunnel.vpid << ", hash=" << tunnel.hash << std::endl ;
#endif
		// we should check that there is no backward call to the turtle router!
		//
		hash = tunnel.hash ;
		vpid = tunnel.vpid ;
	}

	_ft_server->getMultiplexer()->recvChunkMapRequest(vpid,hash,item->direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT) ;
}

void p3turtle::handleRecvFileMap(RsTurtleFileMapItem *item)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3Turtle: received file Map item:" << std::endl ;
	item->print(std::cerr,1) ;
#endif
	std::string hash,vpid ;
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		std::map<TurtleTunnelId,TurtleTunnel>::iterator it2(_local_tunnels.find(item->tunnel_id)) ;

		if(it2 == _local_tunnels.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: got file data with unknown tunnel id " << (void*)item->tunnel_id << std::endl ;
#endif
			return ;
		}

		TurtleTunnel& tunnel(it2->second) ;

#ifdef P3TURTLE_DEBUG
		assert(!tunnel.hash.empty()) ;

		std::cerr << "  This is an endpoint for this file map." << std::endl ;
		std::cerr << "  Forwarding data to the multiplexer." << std::endl ;
		std::cerr << "  using peer_id=" << tunnel.vpid << ", hash=" << tunnel.hash << std::endl ;
#endif
		// We should check that there is no backward call to the turtle router!
		//
		vpid = tunnel.vpid ;
		hash = tunnel.hash ;
	}
	_ft_server->getMultiplexer()->recvChunkMap(vpid,hash,item->compressed_map,item->direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT) ;
}

void p3turtle::handleRecvFileCRC32MapRequest(RsTurtleFileCrcRequestItem *item)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3Turtle: received file CRC32 Map Request item:" << std::endl ;
	item->print(std::cerr,1) ;
#endif
	std::string hash,vpid ;
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		std::map<TurtleTunnelId,TurtleTunnel>::iterator it2(_local_tunnels.find(item->tunnel_id)) ;

		if(it2 == _local_tunnels.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: got file data with unknown tunnel id " << (void*)item->tunnel_id << std::endl ;
#endif
			return ;
		}

		TurtleTunnel& tunnel(it2->second) ;
#ifdef P3TURTLE_DEBUG
		assert(!tunnel.hash.empty()) ;

		std::cerr << "  This is an endpoint for this file crc request." << std::endl ;
		std::cerr << "  Forwarding data to the multiplexer." << std::endl ;
		std::cerr << "  using peer_id=" << tunnel.vpid << ", hash=" << tunnel.hash << std::endl ;
#endif
		// we should check that there is no backward call to the turtle router!
		//
		hash = tunnel.hash ;
		vpid = tunnel.vpid ;
	}

	_ft_server->getMultiplexer()->recvCRC32MapRequest(vpid,hash) ;
}

void p3turtle::handleRecvFileCRC32Map(RsTurtleFileCrcItem *item)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3Turtle: received file CRC32 Map item:" << std::endl ;
	item->print(std::cerr,1) ;
#endif
	std::string hash,vpid ;
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		std::map<TurtleTunnelId,TurtleTunnel>::iterator it2(_local_tunnels.find(item->tunnel_id)) ;

		if(it2 == _local_tunnels.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: got file CRC32 map with unknown tunnel id " << (void*)item->tunnel_id << std::endl ;
#endif
			return ;
		}

		TurtleTunnel& tunnel(it2->second) ;

#ifdef P3TURTLE_DEBUG
		assert(!tunnel.hash.empty()) ;

		std::cerr << "  This is an endpoint for this file map." << std::endl ;
		std::cerr << "  Forwarding data to the multiplexer." << std::endl ;
		std::cerr << "  using peer_id=" << tunnel.vpid << ", hash=" << tunnel.hash << std::endl ;
#endif
		// We should check that there is no backward call to the turtle router!
		//
		vpid = tunnel.vpid ;
		hash = tunnel.hash ;
	}
	_ft_server->getMultiplexer()->recvCRC32Map(vpid,hash,item->crc_map) ;
}
// Send a data request into the correct tunnel for the given file hash
void p3turtle::sendDataRequest(const std::string& peerId, const std::string& , uint64_t, uint64_t offset, uint32_t chunksize)
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
void p3turtle::sendFileData(const std::string& peerId, const std::string& , uint64_t, uint64_t offset, uint32_t chunksize, void *data)
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

	tunnel.transfered_bytes += static_cast<RsTurtleItem*>(item)->serial_size();

	if(item->chunk_data == NULL)
	{
		std::cerr << "p3turtle: Warning: failed malloc of " << chunksize << " bytes for sending data packet." << std::endl ;
		delete item;
		return ;
	}
	memcpy(item->chunk_data,(void*)((uint8_t*)data),chunksize) ;
	item->PeerId(tunnel.local_src) ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: sending file data (chunksize=" << item->chunk_size << ", offset=" << item->chunk_offset << ", hash=0x" << hash << ") through tunnel " << (void*)item->tunnel_id << ", next peer=" << tunnel.local_src << std::endl ;
#endif
	_traffic_info_buffer.data_up_Bps += static_cast<RsTurtleItem*>(item)->serial_size() ;

	sendItem(item) ;
}

void p3turtle::sendChunkMapRequest(const std::string& peerId,const std::string& ,bool is_client)
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

#ifdef P3TURTLE_DEBUG
	assert(hash == tunnel.hash) ;
#endif
	RsTurtleFileMapRequestItem *item = new RsTurtleFileMapRequestItem ;
	item->tunnel_id = tunnel_id ;	

	std::string ownid = mLinkMgr->getOwnId() ;

	if(tunnel.local_src == ownid)
	{
		assert(!is_client) ;
		item->direction = RsTurtleGenericTunnelItem::DIRECTION_SERVER ;	
		item->PeerId(tunnel.local_dst) ;
	}
	else if(tunnel.local_dst == ownid)
	{
		assert(is_client) ;
		item->direction = RsTurtleGenericTunnelItem::DIRECTION_CLIENT ;	
		item->PeerId(tunnel.local_src) ;
	}
	else
		std::cerr << "p3turtle::sendChunkMapRequest: consistency error!" << std::endl ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: sending chunk map req to peer " << peerId << ", hash=0x" << hash << ") through tunnel " << (void*)item->tunnel_id << ", next peer=" << item->PeerId() << std::endl ;
#endif
	sendItem(item) ;
}

void p3turtle::sendChunkMap(const std::string& peerId,const std::string& ,const CompressedChunkMap& cmap,bool is_client)
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

#ifdef P3TURTLE_DEBUG
	assert(hash == tunnel.hash) ;
#endif
	RsTurtleFileMapItem *item = new RsTurtleFileMapItem ;
	item->tunnel_id = tunnel_id ;	
	item->compressed_map = cmap ;

	std::string ownid = mLinkMgr->getOwnId() ;

	if(tunnel.local_src == ownid)
	{
		assert(!is_client) ;
		item->direction = RsTurtleGenericTunnelItem::DIRECTION_SERVER ;	
		item->PeerId(tunnel.local_dst) ;
	}
	else if(tunnel.local_dst == ownid)
	{
		assert(is_client) ;
		item->direction = RsTurtleGenericTunnelItem::DIRECTION_CLIENT ;	
		item->PeerId(tunnel.local_src) ;
	}
	else
		std::cerr << "p3turtle::sendChunkMap: consistency error!" << std::endl ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: sending chunk map to peer " << peerId << ", hash=0x" << hash << ") through tunnel " << (void*)item->tunnel_id << ", next peer=" << item->PeerId() << std::endl ;
#endif
	sendItem(item) ;
}

void p3turtle::sendCRC32MapRequest(const std::string& peerId,const std::string& )
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	// get the proper tunnel for this file hash and peer id.
	std::map<TurtleVirtualPeerId,TurtleTunnelId>::const_iterator it(_virtual_peers.find(peerId)) ;

	if(it == _virtual_peers.end())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "p3turtle::sendCRC32MapRequest: cannot find virtual peer " << peerId << " in VP list." << std::endl ;
#endif
		return ;
	}
	TurtleTunnelId tunnel_id = it->second ;
	TurtleTunnel& tunnel(_local_tunnels[tunnel_id]) ;

#ifdef P3TURTLE_DEBUG
	assert(hash == tunnel.hash) ;
#endif
	RsTurtleFileCrcRequestItem *item = new RsTurtleFileCrcRequestItem;
	item->tunnel_id = tunnel_id ;	
//	item->crc_map = cmap ;
	item->PeerId(tunnel.local_dst) ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: sending CRC32 map request to peer " << peerId << ", hash=0x" << hash << ") through tunnel " << (void*)item->tunnel_id << ", next peer=" << item->PeerId() << std::endl ;
#endif
	sendItem(item) ;
}
void p3turtle::sendCRC32Map(const std::string& peerId,const std::string& ,const CRC32Map& cmap)
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

#ifdef P3TURTLE_DEBUG
	assert(hash == tunnel.hash) ;
#endif
	RsTurtleFileCrcItem *item = new RsTurtleFileCrcItem ;
	item->tunnel_id = tunnel_id ;	
	item->crc_map = cmap ;
	item->PeerId(tunnel.local_src) ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: sending CRC32 map to peer " << peerId << ", hash=0x" << hash << ") through tunnel " << (void*)item->tunnel_id << ", next peer=" << item->PeerId() << std::endl ;
#endif
	sendItem(item) ;
}

bool p3turtle::isTurtlePeer(const std::string& peer_id) const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	return _virtual_peers.find(peer_id) != _virtual_peers.end() ;
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
	std::cerr << "DiggTunnel: performing tunnel request. OwnId = " << mLinkMgr->getOwnId() << " for hash=" << hash << std::endl ;
#endif
	while(mLinkMgr->getOwnId() == "")
	{
		std::cerr << "... waiting for connect manager to form own id." << std::endl ;
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
		_incoming_file_hashes[hash].last_digg_time = time(NULL) ;
	}

	// Form a tunnel request packet that simulates a request from us.
	//
	RsTurtleOpenTunnelItem *item = new RsTurtleOpenTunnelItem ;

	item->PeerId(mLinkMgr->getOwnId()) ;
	item->file_hash = hash ;
	item->request_id = id ;
	item->partial_tunnel_id = generatePersonalFilePrint(hash,true) ;
	item->depth = 0 ;

	// send it

#ifdef TUNNEL_STATISTICS
	TS_request_bounces[item->request_id].clear() ;	// forces initialization
#endif
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

#ifdef TUNNEL_STATISTICS
	if(TS_request_bounces.find(item->request_id) != TS_request_bounces.end())
		TS_request_bounces[item->request_id].push_back(time(NULL)) ;
#endif
	// TR forwarding. We must pay attention not to flood the network. The policy is to force a statistical behavior
	// according to the followin grules:
	// 	- below a number of tunnel request forwards per second MAX_TR_FORWARD_PER_SEC, we keep the traffic
	// 	- if we get close to that limit, we drop long tunnels first with a probability that is larger for long tunnels
	//
	// Variables involved:
	// 	distance_to_maximum		: in [0,inf] is the proportion of the current up TR speed with respect to the maximum allowed speed. This is estimated
	// 										as an average between the average number of TR over the 60 last seconds and the current TR up speed.
	// 	corrected_distance 		: in [0,inf] is a squeezed version of distance: small values become very small and large values become very large.
	// 	depth_peer_probability	: basic probability of forwarding when the speed limit is reached.
	// 	forward_probability		: final probability of forwarding the packet, per peer.
	//
	// When the number of peers increases, the speed limit is reached faster, but the behavior per peer is the same.
	//

	float forward_probability ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
		_traffic_info_buffer.tr_dn_Bps += static_cast<RsTurtleItem*>(item)->serial_size() ;

		float distance_to_maximum	= std::min(100.0f,_traffic_info.tr_up_Bps/(float)(TUNNEL_REQUEST_PACKET_SIZE*MAX_TR_FORWARD_PER_SEC)) ;
		float corrected_distance 	= pow(distance_to_maximum,DISTANCE_SQUEEZING_POWER) ;
		forward_probability	= pow(depth_peer_probability[std::min((uint16_t)6,item->depth)],corrected_distance) ;
#ifdef P3TURTLE_DEBUG
		std::cerr << "Forwarding probability: depth=" << item->depth << ", distance to max speed=" << distance_to_maximum << ", corrected=" << corrected_distance << ", prob.=" << forward_probability << std::endl;
#endif
//		if(forward_probability < 0.1)
//		{
//#ifdef P3TURTLE_DEBUG
//			std::cerr << "Dropped packet!" << std::endl;
//#endif
//			return ;
//		}
	}

	// If the item contains an already handled tunnel request, give up.  This
	// happens when the same tunnel request gets relayed by different peers.  We
	// have to be very careful here, not to call ftController while mTurtleMtx is
	// locked.
	//
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		std::map<TurtleTunnelRequestId,TurtleRequestInfo>::iterator it = _tunnel_requests_origins.find(item->request_id) ;
		
		if(it != _tunnel_requests_origins.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  This is a bouncing request. Ignoring and deleting item." << std::endl ;
#endif
			// This trick allows to shorten tunnels, favoring tunnels of smallest length, with a bias that
			// depends on a mix between a session-based constant and the tunnel partial id. This means
			// that for a given couple of (source,hash), the optimisation always performs the same.
			// overall, 80% tunnels are re-routed. The probability of a tunnel to have optimal length is
			// thus 0.875^n where n is the length of the tunnel, supposing that it has 2 branching peers
			// at each node. This makes:
			// 		n				probability
			// 		1				0.875
			// 		2				0.76
			// 		3				0.67
			// 		4				0.58
			// 		5				0.512
			//
			//  The lower the probability, the higher the anonymity level.
			//
			if(it->second.depth > item->depth && ((item->partial_tunnel_id ^ _random_bias)&0x7)>0)
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  re-routing tunnel request. Item age difference = " << time(NULL)-it->second.time_stamp << std::endl;
				std::cerr << "     - old source: " << it->second.origin << ", old depth=" << it->second.depth << std::endl ;
				std::cerr << "     - new source: " << item->PeerId() << ", new depth=" << item->depth << std::endl ;
				std::cerr << "     - half id: " << (void*)it->first << std::endl ;
#endif
				it->second.origin = item->PeerId() ;
				it->second.depth = item->depth ;
			}
			return ;
		}
		// This is a new request. Let's add it to the request map, and forward
		// it to open peers, while the mutex is locked, so no-one can trigger the
		// lock before the data is consistent.

		TurtleRequestInfo& req( _tunnel_requests_origins[item->request_id] ) ;
		req.origin = item->PeerId() ;
		req.time_stamp = time(NULL) ;
		req.depth = item->depth ;

#ifdef TUNNEL_STATISTICS
		std::cerr << "storing tunnel request " << (void*)(item->request_id) << std::endl ;

		++TS_tunnel_length[item->depth] ;
		TS_request_time_stamps[item->file_hash].push_back(std::pair<time_t,TurtleTunnelRequestId>(time(NULL),item->request_id)) ;
#endif
	}

	// If it's not for us, perform a local search. If something found, forward the search result back.
	// We're off-mutex here.

	bool found = false ;
	FileInfo info ;

	if(item->PeerId() != mLinkMgr->getOwnId())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Request not from us. Performing local search" << std::endl ;
#endif
		found = (_sharing_strategy != SHARE_FRIENDS_ONLY || item->depth < 2) && performLocalHashSearch(item->file_hash,info) ;
	}

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		if(found)
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
			tt.local_dst = mLinkMgr->getOwnId() ;	// this means us
			tt.time_stamp = time(NULL) ;
			tt.transfered_bytes = 0 ;
			tt.speed_Bps = 0.0f ;

			_local_tunnels[res_item->tunnel_id] = tt ;

			// We add a virtual peer for that tunnel+hash combination.
			//
			locked_addDistantPeer(item->file_hash,res_item->tunnel_id) ;

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

	// If search depth not too large, also forward this search request to all other peers.
	//
	bool random_bypass = (item->depth >= TURTLE_MAX_SEARCH_DEPTH && (((_random_bias ^ item->partial_tunnel_id)&0x7)==2)) ;
	bool random_dshift = (item->depth == 1                       && (((_random_bias ^ item->partial_tunnel_id)&0x7)==6)) ;

	if(item->depth < TURTLE_MAX_SEARCH_DEPTH || random_bypass)
	{
		std::list<std::string> onlineIds ;
		mLinkMgr->getOnlineList(onlineIds);
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Forwarding tunnel request: Looking for online peers" << std::endl ;
#endif

		for(std::list<std::string>::const_iterator it(onlineIds.begin());it!=onlineIds.end();++it)
			if(*it != item->PeerId() && RSRandom::random_f32() <= forward_probability)
			{
#ifdef P3TURTLE_DEBUG
 			std::cerr << "  Forwarding request to peer = " << *it << std::endl ;
#endif
				// Copy current item and modify it.
				RsTurtleOpenTunnelItem *fwd_item = new RsTurtleOpenTunnelItem(*item) ;

				// increase search depth, except in some rare cases, to prevent correlation between 
				// TR sniffing and friend names. The strategy is to not increase depth if the depth
				// is 1:
				// 	If B receives a TR of depth 1 from A, B cannot deduice that A is downloading the
				// 	file, since A might have shifted the depth.
				//
				if(!random_dshift)
					++(fwd_item->depth) ;		// increase tunnel depth

				fwd_item->PeerId(*it) ;

				{
					RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
					_traffic_info_buffer.tr_up_Bps += static_cast<RsTurtleItem*>(fwd_item)->serial_size() ;
				}

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
		std::map<TurtleTunnelRequestId,TurtleRequestInfo>::iterator it = _tunnel_requests_origins.find(item->request_id) ;
#ifdef P3TURTLE_DEBUG
		std::cerr << "Received tunnel result:" << std::endl ;
		item->print(std::cerr,0) ;
#endif

		if(it == _tunnel_requests_origins.end())
		{
			// This is an error: how could we receive a tunnel result corresponding to a tunnel item we
			// have forwarded but that it not in the list ?? Actually that happens, when tunnel requests
			// get too old, before the tunnelOk item gets back. But this is quite unusual.

#ifdef P3TURTLE_DEBUG
			std::cerr << __PRETTY_FUNCTION__ << ": tunnel result has no peer direction!" << std::endl ;
#endif
			return ;
		}
		if(it->second.responses.find(item->tunnel_id) != it->second.responses.end())
		{
			std::cerr << "p3turtle: ERROR: received a tunnel response twice. That should not happen." << std::endl;
			return ;
		}
		else
			it->second.responses.insert(item->tunnel_id) ;

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
			tunnel.transfered_bytes = 0 ;
			tunnel.speed_Bps = 0.0f ;

#ifdef P3TURTLE_DEBUG
			std::cerr << "  storing tunnel info. src=" << tunnel.local_src << ", dst=" << tunnel.local_dst << ", id=" << item->tunnel_id << std::endl ;
#endif
		}

		// Is this result's target actually ours ?

		if(it->second.origin == mLinkMgr->getOwnId())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Tunnel starting point. Storing id=" << (void*)item->tunnel_id << " for hash (unknown) and tunnel request id " << it->second.origin << std::endl;
#endif
			// Tunnel is ending here. Add it to the list of tunnels for the given hash.

			// 1 - find which file hash issued this request. This is not costly,
			// 	because there is not too much file hashes to be active at a time,
			// 	and this mostly prevents from sending the hash back in the tunnel.

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

					locked_addDistantPeer(new_hash,item->tunnel_id) ;
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

#ifdef P3TURTLE_DEBUG
	std::cerr << "Performing rsFiles->search()" << std::endl ;
#endif
	// now, search!
	rsFiles->SearchKeywords(words, initialResults,DIR_FLAGS_LOCAL | DIR_FLAGS_NETWORK_WIDE);

#ifdef P3TURTLE_DEBUG
	std::cerr << initialResults.size() << " matches found." << std::endl ;
#endif
	result.clear() ;

	for(std::list<DirDetails>::const_iterator it(initialResults.begin());it!=initialResults.end();++it)
	{
		// retain only file type
		if (it->type == DIR_TYPE_DIR) 
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Skipping directory " << it->name << std::endl ;
#endif
			continue;
		}

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

	if(exp == NULL)
		return ;

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
	std::cerr << "performing search. OwnId = " << mLinkMgr->getOwnId() << std::endl ;
#endif
	while(mLinkMgr->getOwnId() == "")
	{
		std::cerr << "... waitting for connect manager to form own id." << std::endl ;
#ifdef WIN32
		Sleep(1000) ;
#else
		sleep(1) ;
#endif
	}

	item->PeerId(mLinkMgr->getOwnId()) ;
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
	std::cerr << "performing search. OwnId = " << mLinkMgr->getOwnId() << std::endl ;
#endif
	while(mLinkMgr->getOwnId() == "")
	{
		std::cerr << "... waitting for connect manager to form own id." << std::endl ;
#ifdef WIN32
		Sleep(1000) ;
#else
		sleep(1) ;
#endif
	}

	item->PeerId(mLinkMgr->getOwnId()) ;
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

		// First, check if the hash is tagged for removal (there's a delay)

		for(uint32_t i=0;i<_hashes_to_remove.size();++i)
			if(_hashes_to_remove[i] == file_hash)
			{
				_hashes_to_remove[i] = _hashes_to_remove.back() ;
				_hashes_to_remove.pop_back() ;
#ifdef P3TURTLE_DEBUG
				std::cerr << "p3turtle: File hash " << file_hash << " Was scheduled for removal. Canceling the removal." << std::endl ;
#endif
			}

		// Then, check if the hash is already there
		//
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

		// also should send associated request to the file transfer module.
		_incoming_file_hashes[file_hash].size = size ;
		_incoming_file_hashes[file_hash].name = name ;
		_incoming_file_hashes[file_hash].last_digg_time = RSRandom::random_u32()%10 ;
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

/// Warning: this function should never be called while the turtle mutex is locked.
/// Otherwize this is a possible source of cross-lock with the File mutex.
//
bool p3turtle::performLocalHashSearch(const TurtleFileHash& hash,FileInfo& info)
{
	return rsFiles->FileDetails(hash, RS_FILE_HINTS_NETWORK_WIDE | RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_EXTRA | RS_FILE_HINTS_SPEC_ONLY | RS_FILE_HINTS_DOWNLOAD, info);
}

static std::string printFloatNumber(float num,bool friendly=false)
{
	std::ostringstream out ;

	if(friendly)
	{
		char tmp[100] ;
		std::string units[4] = { "B/s","KB/s","MB/s","GB/s" } ;

		int k=0 ;
		while(num >= 800.0f && k<5)
			num /= 1024.0f,++k;

		sprintf(tmp,"%3.2f %s",num,units[k].c_str()) ;
		return std::string(tmp) ;
	}
	else
	{
		out << num ;
		return out.str() ;
	}
}
static std::string printNumber(uint64_t num,bool hex=false)
{
	if(hex)
	{
		char tmp[100] ;

		if(num < (((uint64_t)1)<<32))
			sprintf(tmp,"%08x", uint32_t(num)) ;
		else
			sprintf(tmp,"%08x%08x", uint32_t(num >> 32),uint32_t(num & ( (((uint64_t)1)<<32)-1 ))) ;
		return std::string(tmp) ;
	}
	else
	{
		std::ostringstream out ;
		out << num ;
		return out.str() ;
	}
}

void p3turtle::getTrafficStatistics(TurtleTrafficStatisticsInfo& info) const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	info = _traffic_info ;

	float distance_to_maximum	= std::min(100.0f,info.tr_up_Bps/(float)(TUNNEL_REQUEST_PACKET_SIZE*MAX_TR_FORWARD_PER_SEC)) ;
	info.forward_probabilities.clear() ;

	for(int i=0;i<=6;++i)
	{
		float corrected_distance 	= pow(distance_to_maximum,DISTANCE_SQUEEZING_POWER) ;
		float forward_probability	= pow(depth_peer_probability[i],corrected_distance) ;

		info.forward_probabilities.push_back(forward_probability) ;
	}
}

void p3turtle::getInfo(	std::vector<std::vector<std::string> >& hashes_info,
								std::vector<std::vector<std::string> >& tunnels_info,
								std::vector<TurtleRequestDisplayInfo >& search_reqs_info,
								std::vector<TurtleRequestDisplayInfo >& tunnel_reqs_info) const
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
		//hashes.push_back(printNumber(now - it->second.time_stamp)+" secs ago") ;
	}

	tunnels_info.clear();

	for(std::map<TurtleTunnelId,TurtleTunnel>::const_iterator it(_local_tunnels.begin());it!=_local_tunnels.end();++it)
	{
		tunnels_info.push_back(std::vector<std::string>()) ;
		std::vector<std::string>& tunnel(tunnels_info.back()) ;

		tunnel.push_back(printNumber(it->first,true)) ;

		std::string name;
		if(mLinkMgr->getPeerName(it->second.local_src,name)) 
			tunnel.push_back(name) ;
		else
			tunnel.push_back(it->second.local_src) ;

		if(mLinkMgr->getPeerName(it->second.local_dst,name)) 
			tunnel.push_back(name) ;
		else
			tunnel.push_back(it->second.local_dst);

		tunnel.push_back(it->second.hash) ;
		tunnel.push_back(printNumber(now-it->second.time_stamp) + " secs ago") ;
		tunnel.push_back(printFloatNumber(it->second.speed_Bps,true)) ;
	}

	search_reqs_info.clear();

	for(std::map<TurtleSearchRequestId,TurtleRequestInfo>::const_iterator it(_search_requests_origins.begin());it!=_search_requests_origins.end();++it)
	{
		TurtleRequestDisplayInfo info ;

		info.request_id 		= it->first ;
		info.source_peer_id 	= it->second.origin ;
		info.age 				= now - it->second.time_stamp ;
		info.depth 				= it->second.depth ;

		search_reqs_info.push_back(info) ;
	}

	tunnel_reqs_info.clear();

	for(std::map<TurtleSearchRequestId,TurtleRequestInfo>::const_iterator it(_tunnel_requests_origins.begin());it!=_tunnel_requests_origins.end();++it)
	{
		TurtleRequestDisplayInfo info ;

		info.request_id 		= it->first ;
		info.source_peer_id 	= it->second.origin ;
		info.age 				= now - it->second.time_stamp ;
		info.depth 				= it->second.depth ;

		tunnel_reqs_info.push_back(info) ;
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

#ifdef TUNNEL_STATISTICS
void p3turtle::TS_dumpState()
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	time_t now = time(NULL) ;
	std::cerr << "Dumping tunnel statistics:" << std::endl;

	std::cerr << "TR Bounces: " << TS_request_bounces.size() << std::endl;
	for(std::map<TurtleTunnelRequestId,std::vector<time_t> >::const_iterator it(TS_request_bounces.begin());it!=TS_request_bounces.end();++it)
	{
		std::cerr << (void*)it->first << ": " ;
		for(uint32_t i=0;i<it->second.size();++i)
			std::cerr << it->second[i] - it->second[0] << " " ;
		std::cerr << std::endl;
	}
	std::cerr << "TR in cache: " << _tunnel_requests_origins.size() << std::endl;
	std::cerr << "TR by size: " ;
	for(int i=0;i<8;++i)
		std::cerr << "N(" << i << ")=" << TS_tunnel_length[i] << ", " ;
	std::cerr << std::endl;

	std::cerr << "Total different requested files: " << TS_request_time_stamps.size() << std::endl;
	for(std::map<TurtleFileHash, std::vector<std::pair<time_t,TurtleTunnelRequestId> > >::const_iterator it(TS_request_time_stamps.begin());it!=TS_request_time_stamps.end();++it)
	{
		std::cerr << "hash = " << it->first << ": seconds ago: " ;
		float average = 0 ;
		for(uint32_t i=std::max(0,(int)it->second.size()-25);i<it->second.size();++i)
		{
			std::cerr << now - it->second[i].first << " (" << (void*)it->second[i].second << ") " ;

			if(i>0)
				average += it->second[i].first - it->second[i-1].first ;
		}

		if(it->second.size()>1)
			std::cerr << ", average delay=" << average/(float)(it->second.size()-1) << std::endl;
		else
			std::cerr << std::endl;
	}
}
#endif
