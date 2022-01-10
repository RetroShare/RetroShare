/*******************************************************************************
 * libretroshare/src/turtle: p3turtle.cc                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2009-2018  Cyril Soler <csoler@users.sourceforge.net>         *
 * Copyright (C) 2018-2021  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2021  Asociación Civil Altermundi <info@altermundi.net>       *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

//#define P3TURTLE_DEBUG

#include <unistd.h>
#include <stdexcept>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <cerrno>
#include <cmath>
#include <cstdio>

#include "rsserver/p3face.h"
#include "crypto/rscrypto.h"

#include "pqi/authssl.h"
#include "pqi/p3linkmgr.h"
#include "retroshare/rspeers.h"

#include "ft/ftserver.h"
#include "ft/ftdatamultiplex.h"
#include "ft/ftcontroller.h"

#include "p3turtle.h"
#include "util/cxx17retrocompat.h"
#include "util/rsdebug.h"
#include "util/rsprint.h"
#include "util/rsrandom.h"
#include "pqi/pqinetwork.h"

#ifdef TUNNEL_STATISTICS
static std::vector<int> TS_tunnel_length(8,0) ;
static std::map<TurtleFileHash, std::vector<std::pair<rstime_t,TurtleTunnelRequestId> > > TS_request_time_stamps ;
static std::map<TurtleTunnelRequestId, std::vector<rstime_t> > TS_request_bounces ;
void TS_dumpState() ;
#endif

#define TURTLE_DEBUG() std::cerr << time(NULL) << " : TURTLE : " << __FUNCTION__ << " : "
#define TURTLE_ERROR() std::cerr << "(EE) TURTLE ERROR : "

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
// update of 19-04-12:
//    - The total number of TR per second emmited from self will be MAX_TUNNEL_REQS_PER_SECOND / TIME_BETWEEN_TUNNEL_MANAGEMENT_CALLS = 0.5
//    - I updated forward probabilities to higher values, and min them to 1/nb_connected_friends to prevent blocking tunnels.
//
static const rstime_t TUNNEL_REQUESTS_LIFE_TIME                = 600 ; /// life time for tunnel requests in the cache.
static const rstime_t TUNNEL_REQUESTS_RESULT_TIME              =  20 ; /// maximum time during which we process/forward results for known tunnel requests
static const rstime_t SEARCH_REQUESTS_LIFE_TIME                = 600 ; /// life time for search requests in the cache
static const rstime_t SEARCH_REQUESTS_RESULT_TIME              =  20 ; /// maximum time during which we process/forward results for known search requests
static const rstime_t REGULAR_TUNNEL_DIGGING_TIME              = 300 ; /// maximum interval between two tunnel digging campaigns.
static const rstime_t MAXIMUM_TUNNEL_IDLE_TIME                 =  60 ; /// maximum life time of an unused tunnel.
static const rstime_t EMPTY_TUNNELS_DIGGING_TIME               =  50 ; /// look into tunnels regularly every 50 sec.
static const rstime_t TUNNEL_SPEED_ESTIMATE_LAPSE              =   5 ; /// estimate tunnel speed every 5 seconds
static const rstime_t TUNNEL_CLEANING_LAPS_TIME                =  10 ; /// clean tunnels every 10 secs
static const rstime_t TIME_BETWEEN_TUNNEL_MANAGEMENT_CALLS     =   2 ; /// Tunnel management calls every 2 secs.
static const uint32_t MAX_TUNNEL_REQS_PER_SECOND               =   1 ; /// maximum number of tunnel requests issued per second. Was 0.5 before
static const uint32_t MAX_ALLOWED_SR_IN_CACHE                  = 120 ; /// maximum number of search requests allowed in cache. That makes 2 per sec.
static const uint32_t TURTLE_SEARCH_RESULT_MAX_HITS_FILES      =5000 ; /// maximum number of search results forwarded back to the source.
static const uint32_t TURTLE_SEARCH_RESULT_MAX_HITS_DEFAULT    = 100 ; /// default maximum number of search results forwarded back source.

static const float depth_peer_probability[7] = { 1.0f,0.99f,0.9f,0.7f,0.6f,0.5,0.4f } ;

static const int TUNNEL_REQUEST_PACKET_SIZE             = 50 ;
static const int MAX_TR_FORWARD_PER_SEC                 = 20 ;
static const int MAX_TR_FORWARD_PER_SEC_UPPER_LIMIT     = 30 ;
static const int MAX_TR_FORWARD_PER_SEC_LOWER_LIMIT     = 10 ;
static const int DISTANCE_SQUEEZING_POWER               =  8 ;

#define HEX_PRINT(a) std::hex << a << std::dec

p3turtle::p3turtle(p3ServiceControl *sc,p3LinkMgr *lm)
	:p3Service(), p3Config(), mServiceControl(sc), mLinkMgr(lm), mTurtleMtx("p3turtle")
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	_own_id = sc->getOwnId() ;

	if(_own_id.isNull())
	{
		std::cerr << "OwnId is null, as returned by the p3ServiceControl object in turtle router. Can't go on!" << std::endl;
		exit(-1) ;
	}
	_turtle_routing_enabled = true ;
	_turtle_routing_session_enabled = true;

	_random_bias = RSRandom::random_u32() ;
	_serialiser = new RsTurtleSerialiser() ;

	addSerialType(_serialiser);

	_last_clean_time = 0 ;
	_last_tunnel_management_time = 0 ;
	_last_tunnel_campaign_time = 0 ;
	_last_tunnel_speed_estimate_time = 0 ;

	_traffic_info.reset() ;
	_max_tr_up_rate = MAX_TR_FORWARD_PER_SEC ;
	_service_type = getServiceInfo().mServiceType ;
}

const std::string TURTLE_APP_NAME = "turtle";
const uint16_t TURTLE_APP_MAJOR_VERSION  =       1;
const uint16_t TURTLE_APP_MINOR_VERSION  =       0;
const uint16_t TURTLE_MIN_MAJOR_VERSION  =       1;
const uint16_t TURTLE_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3turtle::getServiceInfo()
{
	return RsServiceInfo(RS_SERVICE_TYPE_TURTLE,
		TURTLE_APP_NAME,
		TURTLE_APP_MAJOR_VERSION,
		TURTLE_APP_MINOR_VERSION,
		TURTLE_MIN_MAJOR_VERSION,
		TURTLE_MIN_MINOR_VERSION);
}

void p3turtle::getItemNames(std::map<uint8_t,std::string>& names) const
{
	names.clear();

	names[RS_TURTLE_SUBTYPE_STRING_SEARCH_REQUEST	] = "Filename substring search request";
	names[RS_TURTLE_SUBTYPE_GENERIC_SEARCH_REQUEST  ] = "Generic search request";
	names[RS_TURTLE_SUBTYPE_FT_SEARCH_RESULT		] = "File search result";
	names[RS_TURTLE_SUBTYPE_GENERIC_SEARCH_RESULT   ] = "Generic search result";
	names[RS_TURTLE_SUBTYPE_OPEN_TUNNEL    			] = "Tunnel request";
	names[RS_TURTLE_SUBTYPE_TUNNEL_OK      			] = "Tunnel response";
	names[RS_TURTLE_SUBTYPE_FILE_REQUEST   			] = "Data request";
	names[RS_TURTLE_SUBTYPE_FILE_DATA      			] = "Data chunk";
	names[RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST   ] = "Filename RegExp search request";
	names[RS_TURTLE_SUBTYPE_GENERIC_DATA     		] = "Generic data";
	names[RS_TURTLE_SUBTYPE_FILE_MAP                ] = "Chunk map";
	names[RS_TURTLE_SUBTYPE_FILE_MAP_REQUEST        ] = "Chunk map request";
	names[RS_TURTLE_SUBTYPE_CHUNK_CRC               ] = "Chunk CRC";
	names[RS_TURTLE_SUBTYPE_CHUNK_CRC_REQUEST       ] = "Chunk CRC request";
}

void p3turtle::setEnabled(bool b)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	_turtle_routing_enabled = b;

	if(b)
		std::cerr << "Enabling turtle routing" << std::endl;
	else
		std::cerr << "Disabling turtle routing" << std::endl;

	IndicateConfigChanged() ;
}
bool p3turtle::enabled() const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	return _turtle_routing_enabled ;
}


void p3turtle::setSessionEnabled(bool b)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	_turtle_routing_session_enabled = b;

	if(b)
		std::cerr << "Enabling turtle routing for this Session" << std::endl;
	else
		std::cerr << "Disabling turtle routing for this Session" << std::endl;
}

bool p3turtle::sessionEnabled() const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	return _turtle_routing_session_enabled ;
}


int p3turtle::tick()
{
	// Handle tunnel trafic
	//
	handleIncoming();		// handle incoming packets

	rstime_t now = time(NULL) ;

#ifdef TUNNEL_STATISTICS
	static rstime_t last_now = now ;
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
	if(now >= _last_tunnel_management_time+TIME_BETWEEN_TUNNEL_MANAGEMENT_CALLS)	// call every second
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "Calling tunnel management." << std::endl ;
#endif
		if(_turtle_routing_enabled && _turtle_routing_session_enabled)
			manageTunnels() ;

		{
			RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
			_last_tunnel_management_time = now ;

			// Update traffic statistics. The constants are important: they allow a smooth variation of the
			// traffic speed, which is used to moderate tunnel requests statistics.
			//
			_traffic_info = _traffic_info*0.9 + _traffic_info_buffer* (0.1 / (float)TIME_BETWEEN_TUNNEL_MANAGEMENT_CALLS) ;
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
	static rstime_t TS_last_dump = time(NULL) ;

	if(now > 20+TS_last_dump)
	{
		TS_last_dump = now ;
		TS_dumpState() ;
	}
#endif

#ifdef P3TURTLE_DEBUG
	// Dump state for debugging, every 20 sec.
	//
	static rstime_t last_dump = time(NULL) ;

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

// adds a virtual peer to the list that is communicated ot ftController.
//
void p3turtle::locked_addDistantPeer(const TurtleFileHash&,TurtleTunnelId tid)
{
	unsigned char tmp[RsPeerId::SIZE_IN_BYTES] ;
	memset(tmp,0,RsPeerId::SIZE_IN_BYTES) ;

	assert(sizeof(tid) == 4) ;

	for(int i=0;i<4;++i)
		tmp[i] = uint8_t( (tid >> ((3-i)*8)) & 0xff ) ;

	RsPeerId virtual_peer_id(tmp) ;

	_virtual_peers[virtual_peer_id] = tid ;
#ifdef P3TURTLE_DEBUG
	assert(_local_tunnels.find(tid)!=_local_tunnels.end()) ;
#endif
	_local_tunnels[tid].vpid = virtual_peer_id ;
}

void p3turtle::getSourceVirtualPeersList(const TurtleFileHash& hash,std::list<pqipeer>& list)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	list.clear() ;

	std::map<TurtleFileHash,TurtleHashInfo>::const_iterator it = _incoming_file_hashes.find(hash) ;

	if(it != _incoming_file_hashes.end())
		for(uint32_t i=0;i<it->second.tunnels.size();++i)
		{
			std::map<TurtleTunnelId,TurtleTunnel>::const_iterator it2 = _local_tunnels.find( it->second.tunnels[i] ) ;

			if(it2 != _local_tunnels.end())
			{
				pqipeer vp ;
				vp.id = it2->second.vpid ;
				vp.name = "Virtual (distant) peer" ;
				vp.state = RS_PEER_S_CONNECTED ;
				vp.actions = RS_PEER_CONNECTED ;
				list.push_back(vp) ;
			}
			else
				std::cerr << "(EE) getSourceVirtualPeersList(): no tunnels for incoming file hash  " << hash << ": weird!"<< std::endl;
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
		virtual bool operator()(const std::pair<TurtleFileHash,rstime_t>& p1,const std::pair<TurtleFileHash,rstime_t>& p2) const
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

	std::vector<std::pair<TurtleFileHash,rstime_t> > hashes_to_digg ;
	rstime_t now = time(NULL) ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		// digg new tunnels if no tunnels are available and force digg new tunnels at regular (large) interval
		//
		for(std::map<TurtleFileHash,TurtleHashInfo>::const_iterator it(_incoming_file_hashes.begin());it!=_incoming_file_hashes.end();++it)
        {
			// get total tunnel speed.
			//
			uint32_t total_speed = 0 ;
			for(uint32_t i=0;i<it->second.tunnels.size();++i)
				total_speed += _local_tunnels[it->second.tunnels[i]].speed_Bps ;

			static const float grow_speed = 1.0f ;	// speed at which the time increases.

			float tunnel_keeping_factor = (std::max(1.0f,(float)total_speed/(float)(50*1024)) - 1.0f)*grow_speed + 1.0f ;

#ifdef P3TURTLE_DEBUG
			std::cerr << "Total speed = " << total_speed << ", tunel factor = " << tunnel_keeping_factor << " new time = " << rstime_t(REGULAR_TUNNEL_DIGGING_TIME*tunnel_keeping_factor) << std::endl;
#endif

            if( (it->second.tunnels.empty()     && now >= it->second.last_digg_time+EMPTY_TUNNELS_DIGGING_TIME)
                     || (it->second.use_aggressive_mode && now >= it->second.last_digg_time + rstime_t(REGULAR_TUNNEL_DIGGING_TIME*tunnel_keeping_factor)))
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "pushed hash " << it->first << ", for digging. Old = " << now - it->second.last_digg_time << std::endl;
#endif
				hashes_to_digg.push_back(std::pair<TurtleFileHash,rstime_t>(it->first,it->second.last_digg_time)) ;
			}
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

	std::vector<std::pair<RsTurtleClientService*,std::pair<TurtleFileHash,TurtleVirtualPeerId> > > services_vpids_to_remove ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

        for(std::set<RsFileHash>::const_iterator hit(_hashes_to_remove.begin());hit!=_hashes_to_remove.end();++hit)
		{
            std::map<TurtleFileHash,TurtleHashInfo>::iterator it(_incoming_file_hashes.find(*hit)) ;

			if(it == _incoming_file_hashes.end())
			{
#ifdef P3TURTLE_DEBUG
                std::cerr << "p3turtle: asked to stop monitoring file hash " << *hit << ", but this hash is actually not handled by the turtle router." << std::endl ;
#endif
				continue ;
			}

			// copy the list of tunnels to remove.
#ifdef P3TURTLE_DEBUG
            std::cerr << "p3turtle: stopping monitoring for file hash " << *hit << ", and closing " << it->second.tunnels.size() << " tunnels (" ;
#endif
			std::vector<TurtleTunnelId> tunnels_to_remove ;

			for(std::vector<TurtleTunnelId>::const_iterator it2(it->second.tunnels.begin());it2!=it->second.tunnels.end();++it2)
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << HEX_PRINT(*it2) << "," ;
#endif
				tunnels_to_remove.push_back(*it2) ;
			}
#ifdef P3TURTLE_DEBUG
			std::cerr << ")" << std::endl ;
#endif
			for(unsigned int k=0;k<tunnels_to_remove.size();++k)
				locked_closeTunnel(tunnels_to_remove[k],services_vpids_to_remove) ;

			_incoming_file_hashes.erase(it) ;
		}

        _hashes_to_remove.clear() ;
	}

	// look for tunnels and stored temporary info that have not been used for a while.

	rstime_t now = time(NULL) ;

	// Search requests
	//
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		for(std::map<TurtleSearchRequestId,TurtleSearchRequestInfo>::iterator it(_search_requests_origins.begin());it!=_search_requests_origins.end();)
			if(now > (rstime_t)(it->second.time_stamp + SEARCH_REQUESTS_LIFE_TIME))
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  removed search request " << HEX_PRINT(it->first) << ", timeout." << std::endl ;
#endif
				std::map<TurtleSearchRequestId,TurtleSearchRequestInfo>::iterator tmp(it) ;
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

		for(std::map<TurtleTunnelRequestId,TurtleTunnelRequestInfo>::iterator it(_tunnel_requests_origins.begin());it!=_tunnel_requests_origins.end();)
			if(now > (rstime_t)(it->second.time_stamp + TUNNEL_REQUESTS_LIFE_TIME))
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  removed tunnel request " << HEX_PRINT(it->first) << ", timeout." << std::endl ;
#endif
				std::map<TurtleTunnelRequestId,TurtleTunnelRequestInfo>::iterator tmp(it) ;
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
			if(now > (rstime_t)(it->second.time_stamp + MAXIMUM_TUNNEL_IDLE_TIME))
			{
#ifdef P3TURTLE_DEBUG
				std::cerr << "  removing tunnel " << HEX_PRINT(it->first) << ": timeout." << std::endl ;
#endif
				tunnels_to_close.push_back(it->first) ;
			}

		for(unsigned int i=0;i<tunnels_to_close.size();++i)
			locked_closeTunnel(tunnels_to_close[i],services_vpids_to_remove) ;
	}

	// Now remove all the virtual peers ids at the client services. Off mutex!
	//

	for(uint32_t i=0;i<services_vpids_to_remove.size();++i)
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  removing virtual peer id " << services_vpids_to_remove[i].second.second << " for service " << services_vpids_to_remove[i].first <<", for hash " << services_vpids_to_remove[i].second.first << std::endl ;
#endif
		services_vpids_to_remove[i].first->removeVirtualPeer(services_vpids_to_remove[i].second.first,services_vpids_to_remove[i].second.second) ;
	}
}

void p3turtle::forceReDiggTunnels(const TurtleFileHash& hash)
{
    {
        RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

        if( _incoming_file_hashes.find(hash) == _incoming_file_hashes.end())
        {
            std::cerr << "(EE) p3turtle::forceReDiggTunnels(): hash " << hash << " is not currently handled by turtle." << std::endl;
            return ;
        }
    }

    diggTunnel(hash) ;
}

void p3turtle::locked_closeTunnel(TurtleTunnelId tid,std::vector<std::pair<RsTurtleClientService*,std::pair<TurtleFileHash,TurtleVirtualPeerId> > >& sources_to_remove)
{
	// This is closing a given tunnel, removing it from file sources, and from the list of tunnels of its
	// corresponding file hash. In the original turtle4privacy paradigm, they also send back and forward
	// tunnel closing commands. In our case, this is not necessary, because if a tunnel is closed somewhere, its
	// source is not going to be used and the tunnel will eventually disappear.
	//
	std::map<TurtleTunnelId,TurtleTunnel>::iterator it(_local_tunnels.find(tid)) ;

	if(it == _local_tunnels.end())
	{
		std::cerr << "p3turtle: was asked to close tunnel " << reinterpret_cast<void*>(tid) << ", which actually doesn't exist." << std::endl ;
		return ;
	}
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: Closing tunnel " << HEX_PRINT(tid) << std::endl ;
#endif

	if(it->second.local_src == _own_id)	// this is a starting tunnel. We thus remove
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
		std::pair<TurtleFileHash,TurtleVirtualPeerId> hash_vpid(hash,vpid) ;

		// Let's be cautious. Normally we should never be here without consistent information,
		// but still, this happens, rarely.
		//
		if(_virtual_peers.find(vpid) != _virtual_peers.end())
			_virtual_peers.erase(_virtual_peers.find(vpid)) ;

		std::map<TurtleFileHash,TurtleHashInfo>::iterator it(_incoming_file_hashes.find(hash)) ;

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

			sources_to_remove.push_back(std::pair<RsTurtleClientService*,std::pair<TurtleFileHash,TurtleVirtualPeerId> >(it->second.service,hash_vpid)) ;
		}
	}
	else if(it->second.local_dst == _own_id)	// This is a ending tunnel. We also remove the virtual peer id
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "    Tunnel is a ending point. Also removing associated outgoing hash." ;
#endif
        std::map<TurtleTunnelId,RsTurtleClientService*>::iterator itHash = _outgoing_tunnel_client_services.find(tid);

        if(itHash != _outgoing_tunnel_client_services.end())
		{
			TurtleVirtualPeerId vpid = it->second.vpid ;
			TurtleFileHash hash = it->second.hash ;

			std::pair<TurtleFileHash,TurtleVirtualPeerId> hash_vpid(hash,vpid) ;

			sources_to_remove.push_back(std::pair<RsTurtleClientService*,std::pair<TurtleFileHash,TurtleVirtualPeerId> >(itHash->second,hash_vpid)) ;

            _outgoing_tunnel_client_services.erase(itHash) ;

			// Also remove the associated virtual peer
			//
			if(_virtual_peers.find(vpid) != _virtual_peers.end())
				_virtual_peers.erase(_virtual_peers.find(vpid)) ;
		}
	}

	_local_tunnels.erase(it) ;
}

void p3turtle::stopMonitoringTunnels(const RsFileHash& hash)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: Marking hash " << hash << " to be removed during autowash." << std::endl ;
#endif
	// We don't do the deletion in this process, because it can cause a race with tunnel management.
    _hashes_to_remove.insert(hash) ;
}

// -----------------------------------------------------------------------------------//
// --------------------------------  Config functions  ------------------------------ //
// -----------------------------------------------------------------------------------//
//
RsSerialiser *p3turtle::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser ;
	rss->addSerialType(new RsTurtleSerialiser) ;
	rss->addSerialType(new RsGeneralConfigSerialiser());

	return rss ;
}

bool p3turtle::saveList(bool& cleanup, std::list<RsItem*>& lst)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: saving list..." << std::endl ;
#endif
	cleanup = true ;

	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
	RsTlvKeyValue kv;
	kv.key = "TURTLE_CONFIG_MAX_TR_RATE" ;
	rs_sprintf(kv.value, "%g", _max_tr_up_rate);
	vitem->tlvkvs.pairs.push_back(kv) ;

	kv.key = "TURTLE_ENABLED" ;
	kv.value = _turtle_routing_enabled?"TRUE":"FALSE" ;

	vitem->tlvkvs.pairs.push_back(kv) ;

	lst.push_back(vitem) ;

	return true ;
}

bool p3turtle::loadList(std::list<RsItem*>& load)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: loading list..." << std::endl ;
#endif
	for(std::list<RsItem*>::const_iterator it(load.begin());it!=load.end();++it)
	{
		RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet*>(*it) ;

		if(vitem != NULL)
			for(std::list<RsTlvKeyValue>::const_iterator kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit)
			{
				if(kit->key == "TURTLE_CONFIG_MAX_TR_RATE")
				{
					int val ;
					if (sscanf(kit->value.c_str(), "%d", &val) == 1)
					{
						setMaxTRForwardRate(val) ;
						std::cerr << "Setting max TR forward rate to " << val << std::endl ;
					}
				}
				if(kit->key == "TURTLE_ENABLED")
				{
					_turtle_routing_enabled = (kit->value == "TRUE") ;

					if(!_turtle_routing_enabled)
						std::cerr << "WARNING: turtle routing has been disabled. You can enable it again in config->server->turtle router." << std::endl;
				}
			}

		delete *it ;
	}
    load.clear() ;
	return true ;
}
int p3turtle::getMaxTRForwardRate() const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	return _max_tr_up_rate ;
}


void p3turtle::setMaxTRForwardRate(int val)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	if(val > MAX_TR_FORWARD_PER_SEC_UPPER_LIMIT || val < MAX_TR_FORWARD_PER_SEC_LOWER_LIMIT)
		std::cerr << "Warning: MAX_TR_FORWARD_PER_SEC value " << val << " read in config file is off limits [" << MAX_TR_FORWARD_PER_SEC_LOWER_LIMIT << "..." << MAX_TR_FORWARD_PER_SEC_UPPER_LIMIT << "]. Ignoring!" << std::endl;
	else
	{
		_max_tr_up_rate = val ;
		std::cerr << "p3turtle: Set max tr up rate to " << val << std::endl;
	}
	IndicateConfigChanged() ;
}


// -----------------------------------------------------------------------------------//
// --------------------------------  Helper functions  ------------------------------ //
// -----------------------------------------------------------------------------------//
//
uint32_t p3turtle::generateRandomRequestId()
{
	return RSRandom::random_u32() ;
}

uint32_t p3turtle::generatePersonalFilePrint(const TurtleFileHash& hash,uint32_t seed,bool b)
{
	// whatever cooking from the file hash and OwnId that cannot be recovered.
	// The only important thing is that the saem couple (hash,SSL id) produces the same tunnel
	// id. The result uses a boolean to allow generating non symmetric tunnel ids.

	std::string buff(hash.toStdString() + _own_id.toStdString()) ;
	uint32_t res = seed ;
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

		if(!(_turtle_routing_enabled && _turtle_routing_session_enabled))
			delete item ;
		else
		{
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
					case RS_TURTLE_SUBTYPE_GENERIC_SEARCH_REQUEST:
					case RS_TURTLE_SUBTYPE_REGEXP_SEARCH_REQUEST: handleSearchRequest(dynamic_cast<RsTurtleSearchRequestItem *>(item)) ;
																				 break ;

					case RS_TURTLE_SUBTYPE_GENERIC_SEARCH_RESULT :
					case RS_TURTLE_SUBTYPE_FT_SEARCH_RESULT : handleSearchResult(dynamic_cast<RsTurtleSearchResultItem *>(item)) ;
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
	}

	return nhandled;
}

// -----------------------------------------------------------------------------------//
// --------------------------------  Search handling. ------------------------------- //
// -----------------------------------------------------------------------------------//
//
void p3turtle::handleSearchRequest(RsTurtleSearchRequestItem *item)
{
	Dbg3() << __PRETTY_FUNCTION__ << " " << *item << std::endl;

	// take a look at the item and test against inconsistent values
	// 	- If the item destimation is

#ifdef P3TURTLE_DEBUG
	std::cerr << "Received search request from peer " << item->PeerId() << ": " << std::endl ;
	item->print(std::cerr,0) ;
#endif

    uint32_t item_size = RsTurtleSerialiser().size(item);

	if(item_size > TURTLE_MAX_SEARCH_REQ_ACCEPTED_SERIAL_SIZE)
	{
		RsWarn() << __PRETTY_FUNCTION__
		         << " Got a turtle search item with arbitrary large size from "
		         << item->PeerId() << " of size " << item_size << " and depth "
		         << item->depth << ". This is not allowed => dropping."
		         << std::endl;
		return;
	}

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		if(_search_requests_origins.size() > MAX_ALLOWED_SR_IN_CACHE)
		{
			RsWarn() << __PRETTY_FUNCTION__ << " More than "
			         << MAX_ALLOWED_SR_IN_CACHE << " search request in cache. "
			         << "A peer is probably trying to flood your network See "
			            "the depth charts to find him." << std::endl;
			return;
		}

		if( _search_requests_origins.find(item->request_id) !=
		        _search_requests_origins.end() )
		{
			/* If the item contains an already handled search request, give up.
			 * This happens when the same search request gets relayed by
			 * different peers */
			return;
		}
	}

    // Perform local search off-mutex,because this might call some services that are above turtle in the mutex chain.

    uint32_t search_result_count = 0;
    uint32_t max_allowed_hits = TURTLE_SEARCH_RESULT_MAX_HITS_DEFAULT;

	if(item->PeerId() != _own_id) // is the request not coming from us?
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Request not from us. Performing local search" << std::endl ;
#endif
        std::list<RsTurtleSearchResultItem*> search_results ;

        performLocalSearch(item,search_result_count,search_results,max_allowed_hits) ;

        for(auto it(search_results.begin());it!=search_results.end();++it)
        {
            (*it)->request_id = item->request_id ;
            (*it)->PeerId(item->PeerId()) ;

#ifdef P3TURTLE_DEBUG
			std::cerr << "  sending back search result for request " << item->request_id << " to back to peer " << item->PeerId() << std::endl ;
#endif
            sendItem(*it) ;
        }
	}

	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	// This is a new request. Let's add it to the request map, and forward it to
	// open peers.

	TurtleSearchRequestInfo& req( _search_requests_origins[item->request_id] ) ;
	req.origin = item->PeerId() ;
	req.time_stamp = time(NULL) ;
	req.depth = item->depth ;
	req.result_count = search_result_count;
	req.keywords = item->GetKeywords() ;
	req.service_id = item->serviceId() ;
    req.max_allowed_hits = max_allowed_hits;

	// if enough has been sent back already, do not sarch further

#ifdef P3TURTLE_DEBUG
	std::cerr << "  result count = " << req.result_count << std::endl;
#endif
	if(req.result_count >= max_allowed_hits)
		return ;

	// If search depth not too large, also forward this search request to all other peers.
	//
	// We use a random factor on the depth test that is biased by a mix between the session id and the partial tunnel id
	// to scramble a possible search-by-depth attack.
	//
	bool random_bypass = (item->depth >= TURTLE_MAX_SEARCH_DEPTH && (((_random_bias ^ item->request_id)&0x7)==2)) ;
	bool random_dshift = (item->depth == 1                       && (((_random_bias ^ item->request_id)&0x7)==6)) ;

	if(item->depth < TURTLE_MAX_SEARCH_DEPTH || random_bypass)
	{
		std::set<RsPeerId> onlineIds ;
		mServiceControl->getPeersConnected(_service_type, onlineIds);
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Looking for online peers" << std::endl ;
#endif

		for(std::set<RsPeerId>::const_iterator it(onlineIds.begin());it!=onlineIds.end();++it)
		{
//			if(!mServiceControl->isPeerConnected(RS_SERVICE_TYPE_TURTLE,*it))
//				continue ;

			uint32_t linkType = mLinkMgr->getLinkType(*it);

			if ((linkType & RS_NET_CONN_SPEED_TRICKLE) || (linkType & RS_NET_CONN_SPEED_LOW)) 	// don't forward searches to slow link types (e.g relay peers)!
				continue ;

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
	}
#ifdef P3TURTLE_DEBUG
	else
		std::cout << "  Dropping this item, as search depth is " << item->depth << std::endl ;
#endif
}

// This function should be removed in the future, when file search will also use generic search items.

void p3turtle::performLocalSearch(
        RsTurtleSearchRequestItem *item, uint32_t& req_result_count,
        std::list<RsTurtleSearchResultItem*>& search_results,
        uint32_t& max_allowed_hits )
{
	Dbg3() << __PRETTY_FUNCTION__ << " " << item << std::endl;

	RsTurtleFileSearchRequestItem* ftsearch =
	        dynamic_cast<RsTurtleFileSearchRequestItem*>(item);

    if(ftsearch != NULL)
    {
		performLocalSearch_files(
		            ftsearch, req_result_count, search_results,
		            max_allowed_hits );
        return ;
    }

    RsTurtleGenericSearchRequestItem *gnsearch = dynamic_cast<RsTurtleGenericSearchRequestItem*>(item) ;

    if(gnsearch != NULL)
    {
        performLocalSearch_generic(gnsearch,req_result_count,search_results,max_allowed_hits) ;
        return ;
    }
}

void p3turtle::performLocalSearch_generic(RsTurtleGenericSearchRequestItem *item, uint32_t& /*req_result_count*/, std::list<RsTurtleSearchResultItem*>& result,uint32_t& max_allowed_hits)
{
    unsigned char *search_result_data = NULL ;
    uint32_t search_result_data_len = 0 ;

    RsTurtleClientService *client = NULL ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
		auto it = _registered_services.find(item->service_id) ;

		if(it == _registered_services.end())
			return ;

		client = it->second ;
	}

    if(client->receiveSearchRequest(item->search_data,item->search_data_len,search_result_data,search_result_data_len,max_allowed_hits))
    {
		RsTurtleGenericSearchResultItem *result_item = new RsTurtleGenericSearchResultItem ;

        result_item->result_data = search_result_data ;
        result_item->result_data_len = search_result_data_len ;

        result.push_back(result_item) ;
    }
}

void p3turtle::performLocalSearch_files(
        RsTurtleFileSearchRequestItem *item, uint32_t& req_result_count,
        std::list<RsTurtleSearchResultItem*>& result,
        uint32_t& max_allowed_hits )
{
	Dbg3() << __PRETTY_FUNCTION__ << " " << *item << std::endl;

    std::list<TurtleFileInfo> initialResults ;
    item->search(initialResults) ;

#ifdef P3TURTLE_DEBUG
	std::cerr << initialResults.size() << " matches found." << std::endl ;
#endif
	result.clear() ;
	RsTurtleFTSearchResultItem *res_item = NULL ;
	uint32_t item_size = 0 ;

	static const uint32_t RSTURTLE_MAX_SEARCH_RESPONSE_SIZE = 10000 ;
    max_allowed_hits = TURTLE_SEARCH_RESULT_MAX_HITS_FILES;

	for(auto it(initialResults.begin());it!=initialResults.end();++it)
	{
		if(res_item == NULL)
		{
			res_item = new RsTurtleFTSearchResultItem ;
			item_size = 0 ;

            result.push_back(res_item) ;
		}
		res_item->result.push_back(*it);

		// Let's chop search results items into several chunks of finite size to avoid exceeding streamer's capacity.
		//
		++req_result_count ;	// increase hit number for this particular search request.

		item_size += 8 /* size */ + it->hash.serial_size() + it->name.size() ;

		if(item_size > RSTURTLE_MAX_SEARCH_RESPONSE_SIZE || req_result_count >= max_allowed_hits)
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Sending back chunk of size " << item_size << ", for " << res_item->result.size() << " elements." << std::endl ;
#endif
			res_item = NULL ;	// forces creation of a new item.
		}
	}

	Dbg3() << __PRETTY_FUNCTION__ << " found " << req_result_count << " results"
	       << std::endl;
}

void p3turtle::handleSearchResult(RsTurtleSearchResultItem *item)
{
    // Filter out banned hashes from the result.

	RsTurtleFTSearchResultItem *ftsr_tmp = dynamic_cast<RsTurtleFTSearchResultItem*>(item) ;

    if(ftsr_tmp != NULL)
    {
        for(auto it(ftsr_tmp->result.begin());it!=ftsr_tmp->result.end();)
            if( rsFiles->isHashBanned((*it).hash) )
            {
                std::cerr << "(II) filtering out banned hash " << (*it).hash << " from turtle result " << std::hex << item->request_id << std::dec << std::endl;
                it = ftsr_tmp->result.erase(it);
            }
			else
                ++it;

        if(ftsr_tmp->result.empty())
            return ;
    }

    // Then handle the result

    std::list<std::pair<RsTurtleSearchResultItem*,RsTurtleClientService*> > results_to_notify_off_mutex ;

	{
		RS_STACK_MUTEX(mTurtleMtx);
		// Find who actually sent the corresponding request.
		//
		std::map<TurtleRequestId,TurtleSearchRequestInfo>::iterator it = _search_requests_origins.find(item->request_id) ;

#ifdef P3TURTLE_DEBUG
		std::cerr << "Received search result:" << std::endl ;
		item->print(std::cerr,0) ;
#endif
		if(it == _search_requests_origins.end())
		{
			// This is an error: how could we receive a search result corresponding to a search item we
			// have forwarded but that it not in the list ??

			std::cerr << __PRETTY_FUNCTION__ << ": search result for request " << std::hex << item->request_id << std::dec << " has no peer direction!" << std::endl ;
			return ;
		}

		// Is this result too old?
		// Search Requests younger than SEARCH_REQUESTS_LIFE_TIME are kept in the cache, so that they are not duplicated if they bounce in the network
		// Nevertheless results received for Search Requests older than SEARCH_REQUESTS_RESULT_TIME are considered obsolete and discarded
		if (time(NULL) > it->second.time_stamp + SEARCH_REQUESTS_RESULT_TIME)
		{
#ifdef P3TURTLE_DEBUG
			RsDbg() << "TURTLE p3turtle::handleSearchResult Search Request is known, but result arrives too late, dropping";
#endif
			return;
		}

		// Is this result's target actually ours ?

		if(it->second.origin == _own_id)
		{
			it->second.result_count += item->count() ;

            auto it2 = _registered_services.find(it->second.service_id) ;

            if(it2 != _registered_services.end())
				results_to_notify_off_mutex.push_back(std::make_pair(item,it2->second)) ;
            else
                std::cerr << "(EE) cannot find client service for ID " << std::hex << it->second.service_id << std::dec << ": search result item will be dropped." << std::endl;
		}
		else
		{									// Nope, so forward it back.
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Forwarding result back to " << it->second.origin << std::endl;
#endif
			// We update the total count forwarded back, and chop it to TURTLE_SEARCH_RESULT_MAX_HITS.

			uint32_t n = item->count(); // not so good!

			if(it->second.result_count >= it->second.max_allowed_hits)
			{
				std::cerr << "(WW) exceeded turtle search result to forward. Req=" << std::hex << item->request_id << std::dec
				          << " already forwarded: " << it->second.result_count << ", max_allowed: " << it->second.max_allowed_hits << ": dropping item with " << n << " elements." << std::endl;
				return ;
			}

			if(it->second.result_count + n > it->second.max_allowed_hits)
			{
				for(uint32_t i=it->second.result_count + n; i>it->second.max_allowed_hits;--i)
					item->pop() ;

				it->second.result_count = it->second.max_allowed_hits ;
			}
			else
				it->second.result_count += n ;

			RsTurtleSearchResultItem *fwd_item = item->duplicate();

			// Normally here, we should setup the forward adress, so that the owner's
			// of the files found can be further reached by a tunnel.

			fwd_item->PeerId(it->second.origin) ;

			sendItem(fwd_item) ;
		}
	} // mTurtleMtx end

    // now we notify clients off-mutex.

    for(auto it(results_to_notify_off_mutex.begin());it!=results_to_notify_off_mutex.end();++it)
    {
        // Hack to use the old search result handling in ftServer. Normally ftServer should use the new method with serialized result.

#warning make sure memory is correctly deleted here
        RsTurtleFTSearchResultItem *ftsr = dynamic_cast<RsTurtleFTSearchResultItem*>(it->first) ;

        if(ftsr!=NULL)
        {
            ftServer *client = dynamic_cast<ftServer*>((*it).second) ;

            if(!client)
            {
                std::cerr << "(EE) received turtle FT search result but the service is not a ftServer!!" << std::endl;
                continue;
            }
			//RsServer::notify()->notifyTurtleSearchResult(ftsr->request_id,ftsr->result) ;

            client->ftReceiveSearchResult(ftsr);
            continue ;
        }

        RsTurtleGenericSearchResultItem *gnsr = dynamic_cast<RsTurtleGenericSearchResultItem*>(it->first) ;

        if(gnsr!=NULL)
			(*it).second->receiveSearchResult(gnsr->request_id,gnsr->result_data,gnsr->result_data_len) ;
    }
}

// -----------------------------------------------------------------------------------//
// ---------------------------------  File Transfer. -------------------------------- //
// -----------------------------------------------------------------------------------//

// Routing of turtle tunnel items in a generic manner. Most tunnel packets will
// use this function, except packets designed for contructing the tunnels and
// searching, namely TurtleSearchRequests/Results and OpenTunnel/TunnelOkItems
//
// Only packets coming from handleIncoming() end up here, so this function is
// able to catch the transiting traffic.
//
void p3turtle::routeGenericTunnelItem(RsTurtleGenericTunnelItem *item)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3Turtle: treating generic tunnel item:" << std::endl ;
	item->print(std::cerr,1) ;
#endif

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		// look for the tunnel id.
		//
		std::map<TurtleTunnelId,TurtleTunnel>::iterator it(_local_tunnels.find(item->tunnelId())) ;

		if(it == _local_tunnels.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle: got file map with unknown tunnel id " << HEX_PRINT(item->tunnelId()) << std::endl ;
#endif
			delete item;
			return ;
		}

		TurtleTunnel& tunnel(it->second) ;

		// Only file data transfer updates tunnels time_stamp field, to avoid maintaining tunnel that are incomplete.
		if(item->shouldStampTunnel())
			tunnel.time_stamp = time(NULL) ;

		tunnel.transfered_bytes += RsTurtleSerialiser().size(item);

		if(item->PeerId() == tunnel.local_dst)
			item->setTravelingDirection(RsTurtleGenericTunnelItem::DIRECTION_CLIENT) ;
		else if(item->PeerId() == tunnel.local_src)
			item->setTravelingDirection(RsTurtleGenericTunnelItem::DIRECTION_SERVER) ;
		else
		{
			std::cerr << "(EE) p3turtle::routeGenericTunnelItem(): item mismatches tunnel src/dst ids." << std::endl;
			std::cerr << "(EE)          tunnel.local_src = " << tunnel.local_src << std::endl;
			std::cerr << "(EE)          tunnel.local_dst = " << tunnel.local_dst << std::endl;
			std::cerr << "(EE)            item->PeerId() = " << item->PeerId()    << std::endl;
			std::cerr << "(EE) This item is probably lost while tunnel route got redefined. Deleting this item." << std::endl ;
			delete item ;
			return ;
		}

		// Let's figure out whether this packet is for us or not.

		if(item->PeerId() == tunnel.local_dst && tunnel.local_src != _own_id) //direction == RsTurtleGenericTunnelItem::DIRECTION_CLIENT &&
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Forwarding generic item to peer " << tunnel.local_src << std::endl ;
#endif
			item->PeerId(tunnel.local_src) ;

			_traffic_info_buffer.unknown_updn_Bps += RsTurtleSerialiser().size(item) ;

			// This has been disabled for compilation reasons. Not sure we actually need it.
			//
			//if(dynamic_cast<RsTurtleFileDataItem*>(item) != NULL)
			//	item->setPriorityLevel(QOS_PRIORITY_RS_TURTLE_FORWARD_FILE_DATA) ;

			sendItem(item) ;
			return ;
		}

		if(item->PeerId() == tunnel.local_src && tunnel.local_dst != _own_id) //direction == RsTurtleGenericTunnelItem::DIRECTION_SERVER &&
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Forwarding generic item to peer " << tunnel.local_dst << std::endl ;
#endif
			item->PeerId(tunnel.local_dst) ;

			_traffic_info_buffer.unknown_updn_Bps += RsTurtleSerialiser().size(item);

			sendItem(item) ;
			return ;
        }

        // item is for us. Use the locked region to record the data.

        _traffic_info_buffer.data_dn_Bps += RsTurtleSerialiser().size(item);
    }

	// The packet was not forwarded, so it is for us. Let's treat it.
	// This is done off-mutex, to avoid various deadlocks
	//

    handleRecvGenericTunnelItem(item) ;

	delete item ;
}

void p3turtle::handleRecvGenericTunnelItem(RsTurtleGenericTunnelItem *item)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3Turtle: received Generic tunnel item:" << std::endl ;
	item->print(std::cerr,1) ;
#endif
	RsFileHash hash ;
	RsPeerId vpid ;
	RsTurtleClientService *service ;

    if(!getTunnelServiceInfo(item->tunnelId(),vpid,hash,service))
		return ;

	service->receiveTurtleData(item,hash,vpid,item->travelingDirection()) ;
}

bool p3turtle::getTunnelServiceInfo(TurtleTunnelId tunnel_id,RsPeerId& vpid,RsFileHash& hash,RsTurtleClientService *& service)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	std::map<TurtleTunnelId,TurtleTunnel>::iterator it2(_local_tunnels.find(tunnel_id)) ;

	if(it2 == _local_tunnels.end())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "p3turtle: unknown tunnel id " << std::hex << tunnel_id << std::dec << std::endl ;
#endif
		return false;
	}

	TurtleTunnel& tunnel(it2->second) ;

#ifdef P3TURTLE_DEBUG
	assert(!tunnel.hash.isNull()) ;

	std::cerr << "  This is an endpoint for this file map." << std::endl ;
	std::cerr << "  Forwarding data to the multiplexer." << std::endl ;
	std::cerr << "  using peer_id=" << tunnel.vpid << ", hash=" << tunnel.hash << std::endl ;
#endif
	// We should check that there is no backward call to the turtle router!
	//
	vpid = tunnel.vpid ;
	hash = tunnel.hash ;

	// Now sort out the case of client vs. server side items.
	//
	if(tunnel.local_src == _own_id)
	{
		std::map<TurtleFileHash,TurtleHashInfo>::const_iterator it = _incoming_file_hashes.find(hash) ;

		if(it == _incoming_file_hashes.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "p3turtle::handleRecvGenericTunnelItem(): hash " << hash << " for client side tunnel endpoint " << std::hex << tunnel_id << std::dec << " has been removed (probably a late response)! Dropping the item. " << std::endl;
#endif
			return false;
		}

		service = it->second.service ;
	}
	else if(tunnel.local_dst == _own_id)
	{
        std::map<TurtleTunnelId,RsTurtleClientService*>::const_iterator it = _outgoing_tunnel_client_services.find(tunnel_id) ;

        if(it == _outgoing_tunnel_client_services.end())
		{
#ifdef P3TURTLE_DEBUG
            std::cerr << "p3turtle::handleRecvGenericTunnelItem(): hash " << tunnel.hash << " for server side tunnel endpoint " << std::hex << tunnel_id << std::dec << " has been removed (probably a late response)! Dropping the item. " << std::endl;
#endif
			return false;
		}

		service = it->second;
	}
	else
	{
		std::cerr << "p3turtle::handleRecvGenericTunnelItem(): hash " << hash << " for tunnel " << std::hex << it2->first << std::dec << ". Tunnel is not a end-point or a starting tunnel!! This is a serious consistency error." << std::endl;
		return false ;
	}

	return true ;
}
// Send a data request into the correct tunnel for the given file hash
//
void p3turtle::sendTurtleData(const RsPeerId& virtual_peer_id,RsTurtleGenericTunnelItem *item)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	// get the proper tunnel for this file hash and peer id.
	std::map<TurtleVirtualPeerId,TurtleTunnelId>::const_iterator it(_virtual_peers.find(virtual_peer_id)) ;

	if(it == _virtual_peers.end())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "p3turtle::senddataRequest: cannot find virtual peer " << virtual_peer_id << " in VP list." << std::endl ;
#endif
		delete item ;
		return ;
	}
	TurtleTunnelId tunnel_id = it->second ;
	std::map<TurtleTunnelId,TurtleTunnel>::iterator it2( _local_tunnels.find(tunnel_id) ) ;

	if(it2 == _local_tunnels.end())
	{
		std::cerr << "p3turtle::client asked to send a packet through tunnel that has previously been deleted. Not a big issue unless it happens in masses." << std::endl;
		delete item ;
		return ;
	}
	TurtleTunnel& tunnel(it2->second) ;

	item->tunnel_id = tunnel_id ;	// we should randomly select a tunnel, or something more clever.

	uint32_t ss = RsTurtleSerialiser().size(item);

	if(item->shouldStampTunnel())
		tunnel.time_stamp = time(NULL) ;

	tunnel.transfered_bytes += ss ;

	if(tunnel.local_src == _own_id)
	{
		item->setTravelingDirection(RsTurtleGenericTunnelItem::DIRECTION_SERVER) ;
		item->PeerId(tunnel.local_dst) ;
		_traffic_info_buffer.data_dn_Bps += ss ;
	}
	else if(tunnel.local_dst == _own_id)
	{
		item->setTravelingDirection(RsTurtleGenericTunnelItem::DIRECTION_CLIENT) ;
		item->PeerId(tunnel.local_src) ;
		_traffic_info_buffer.data_up_Bps += ss ;
	}
	else
	{
		std::cerr << "p3Turtle::sendTurtleData(): asked to send a packet into a tunnel that is not registered. Dropping packet." << std::endl ;
		delete item ;
		return ;
	}

#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: sending service packet to virtual peer id " << virtual_peer_id << ", hash=0x" << tunnel.hash << ", tunnel = " << HEX_PRINT(item->tunnel_id) << ", next peer=" << tunnel.local_dst << std::endl ;
#endif
	sendItem(item) ;
}

bool p3turtle::isTurtlePeer(const RsPeerId& peer_id) const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	return _virtual_peers.find(peer_id) != _virtual_peers.end() ;
}

RsPeerId p3turtle::getTurtlePeerId(TurtleTunnelId tid) const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	std::map<TurtleTunnelId,TurtleTunnel>::const_iterator it( _local_tunnels.find(tid) ) ;

#ifdef P3TURTLE_DEBUG
	assert(it!=_local_tunnels.end()) ;
	assert(!it->second.vpid.isNull()) ;
#endif

	return it->second.vpid ;
}

bool p3turtle::isOnline(const RsPeerId& peer_id) const
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
	std::cerr << "DiggTunnel: performing tunnel request. OwnId = " << _own_id << " for hash=" << hash << std::endl ;
#endif
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

	item->PeerId(_own_id) ;
	item->file_hash = hash ;
	item->request_id = id ;
	item->partial_tunnel_id = generatePersonalFilePrint(hash,_random_bias,true) ;
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

 	// check first if the hash is in the ban list. If so, drop the request.

 	if(rsFiles->isHashBanned(item->file_hash))
    {
        std::cerr << "(II) Rejecting tunnel request to ban hash " << item->file_hash << std::endl;
        return ;
    }

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
		_traffic_info_buffer.tr_dn_Bps += RsTurtleSerialiser().size(item);

		float distance_to_maximum	= std::min(100.0f,_traffic_info.tr_up_Bps/(float)(TUNNEL_REQUEST_PACKET_SIZE*_max_tr_up_rate)) ;
		float corrected_distance 	= pow(distance_to_maximum,DISTANCE_SQUEEZING_POWER) ;
		forward_probability	= pow(depth_peer_probability[std::min((uint16_t)6,item->depth)],corrected_distance) ;
#ifdef P3TURTLE_DEBUG
		std::cerr << "Forwarding probability: depth=" << item->depth << ", distance to max speed=" << distance_to_maximum << ", corrected=" << corrected_distance << ", prob.=" << forward_probability << std::endl;
#endif
	}

	// If the item contains an already handled tunnel request, give up.  This
	// happens when the same tunnel request gets relayed by different peers.  We
	// have to be very careful here, not to call ftController while mTurtleMtx is
	// locked.
	//
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		std::map<TurtleTunnelRequestId,TurtleTunnelRequestInfo>::iterator it = _tunnel_requests_origins.find(item->request_id) ;

		if(it != _tunnel_requests_origins.end())
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  This is a bouncing request. Ignoring and deleting item." << std::endl ;
#endif
			return ;
		}
		// This is a new request. Let's add it to the request map, and forward
		// it to open peers, while the mutex is locked, so no-one can trigger the
		// lock before the data is consistent.

		TurtleTunnelRequestInfo& req( _tunnel_requests_origins[item->request_id] ) ;
		req.origin = item->PeerId() ;
		req.time_stamp = time(NULL) ;
		req.depth = item->depth ;

#ifdef TUNNEL_STATISTICS
		std::cerr << "storing tunnel request " << (void*)(item->request_id) << std::endl ;

		++TS_tunnel_length[item->depth] ;
		TS_request_time_stamps[item->file_hash].push_back(std::pair<rstime_t,TurtleTunnelRequestId>(time(NULL),item->request_id)) ;
#endif
	}

	// If it's not for us, perform a local search. If something found, forward the search result back.
	// We're off-mutex here.

	bool found = false ;
	//std::string info ;
	RsTurtleClientService *service = NULL ;

	if(item->PeerId() != _own_id)
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Request not from us. Performing local search" << std::endl ;
#endif
		found = performLocalHashSearch(item->file_hash,item->PeerId(),service) ;
	}

	{
		if(found)
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Local hash found. Sending tunnel ok to origin (" << item->PeerId() << ")." << std::endl ;
#endif
			// Send back tunnel ok to the same guy
			//
			RsTurtleTunnelOkItem *res_item = new RsTurtleTunnelOkItem ;
            TurtleVirtualPeerId vpid ;

			res_item->request_id = item->request_id ;
			{
				RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
				res_item->tunnel_id = item->partial_tunnel_id ^ generatePersonalFilePrint(item->file_hash,_random_bias,false) ;

				res_item->PeerId(item->PeerId()) ;

				TurtleTunnelId t_id = res_item->tunnel_id ;	// save it because sendItem deletes the item

				sendItem(res_item) ;

				// Note in the tunnels list that we have an ending tunnel here.
				TurtleTunnel tt ;
				tt.local_src = item->PeerId() ;
				tt.hash = item->file_hash ;
				tt.local_dst = _own_id ;	// this means us
				tt.time_stamp = time(NULL) ;
				tt.transfered_bytes = 0 ;
				tt.speed_Bps = 0.0f ;

				_local_tunnels[t_id] = tt ;

				// We add a virtual peer for that tunnel+hash combination.
				//
				locked_addDistantPeer(item->file_hash,t_id) ;

				// Store some info string about the tunnel.
				//
				_outgoing_tunnel_client_services[t_id] = service ;

                vpid = _local_tunnels[t_id].vpid;
			}

			// Notify the client service that there's a new virtual peer id available as a client.
			//
			service->addVirtualPeer(item->file_hash,vpid,RsTurtleGenericTunnelItem::DIRECTION_CLIENT) ;

			// We return straight, because when something is found, there's no need to digg a tunnel further.
			//

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

	// Multi-tunneling trick: consistently perturbate the half-tunnel id:
	// - the tunnel id will now be unique for a given route
	// - allows a better balance of bandwidth for a given transfer
	// - avoid the waste of items that get lost when re-routing a tunnel

#ifdef P3TURTLE_DEBUG
	std::cerr << "Perturbating partial tunnel id. Original=" << std::hex << item->partial_tunnel_id ;
#endif
	item->partial_tunnel_id = generatePersonalFilePrint(item->file_hash,item->partial_tunnel_id ^ _random_bias,true) ;

#ifdef P3TURTLE_DEBUG
	std::cerr << " new=" << item->partial_tunnel_id << std::dec << std::endl;
#endif

	if(item->depth < TURTLE_MAX_SEARCH_DEPTH || random_bypass)
	{
		std::set<RsPeerId> onlineIds ;
		mServiceControl->getPeersConnected(_service_type, onlineIds);

//		for(std::set<RsPeerId>::iterator it(onlineIds.begin());it!=onlineIds.end();)
//			if(!mServiceControl->isPeerConnected(RS_SERVICE_PERM_TURTLE,*it))
//			{
//				std::set<RsPeerId>::iterator tmp = it++ ;
//				onlineIds.erase(tmp) ;
//			}
//			else
//				++it ;

		int nb_online_ids = onlineIds.size() ;

		if(forward_probability * nb_online_ids < 1.0f && nb_online_ids > 0)
		{
			forward_probability = 1.0f / nb_online_ids ;

			// Setting forward_probability to 1/nb_online_ids forces at most one TR up per TR dn. But if we are overflooded by
			// TR dn, we still need to control them to avoid flooding the pqiHandler outqueue. So we additionally moderate the
			// forward probability so as to reduct the output rate accordingly.
			//
			if(_traffic_info.tr_dn_Bps / (float)TUNNEL_REQUEST_PACKET_SIZE > _max_tr_up_rate)
				forward_probability *= _max_tr_up_rate*TUNNEL_REQUEST_PACKET_SIZE / (float)_traffic_info.tr_dn_Bps ;
		}

#ifdef P3TURTLE_DEBUG
		std::cerr << "  Forwarding tunnel request: Looking for online peers" << std::endl ;
#endif

		for(std::set<RsPeerId>::const_iterator it(onlineIds.begin());it!=onlineIds.end();++it)
		{
			uint32_t linkType = mLinkMgr->getLinkType(*it);

			if ((linkType & RS_NET_CONN_SPEED_TRICKLE) || (linkType & RS_NET_CONN_SPEED_LOW)) 	// don't forward tunnel requests to slow link types (e.g relay peers)!
				continue ;

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
					_traffic_info_buffer.tr_up_Bps += RsTurtleSerialiser().size(fwd_item);
				}

				sendItem(fwd_item) ;
			}
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
	RsPeerId new_vpid ;
	RsTurtleClientService *service = NULL ;

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		// Find who actually sent the corresponding turtle tunnel request.
		//
		std::map<TurtleTunnelRequestId,TurtleTunnelRequestInfo>::iterator it = _tunnel_requests_origins.find(item->request_id) ;
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
			std::cerr << "Tunnel id " << HEX_PRINT(item->tunnel_id) << " is already there. Not storing." << std::endl ;
#endif
		}
		else
		{
			tunnel.local_src = it->second.origin ;
			tunnel.local_dst = item->PeerId() ;
			tunnel.hash.clear() ;
			tunnel.time_stamp = time(NULL) ;
			tunnel.transfered_bytes = 0 ;
			tunnel.speed_Bps = 0.0f ;

#ifdef P3TURTLE_DEBUG
			std::cerr << "  storing tunnel info. src=" << tunnel.local_src << ", dst=" << tunnel.local_dst << ", id=" << item->tunnel_id << std::endl ;
#endif
		}

		// Is this result too old?
		// Tunnel Requests younger than TUNNEL_REQUESTS_LIFE_TIME are kept in the cache, so that they are not duplicated if they bounce in the network
		// Nevertheless results received for Tunnel Requests older than TUNNEL_REQUESTS_RESULT_TIME are considered obsolete and discarded
		if (time(NULL) > it->second.time_stamp + TUNNEL_REQUESTS_RESULT_TIME)
		{
#ifdef P3TURTLE_DEBUG
			RsDbg() << "TURTLE p3turtle::handleTunnelResult Tunnel Request is known, but result arrives too late, dropping";
#endif
			return;
		}

		// Is this result's target actually ours ?

		if(it->second.origin == _own_id)
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Tunnel starting point. Storing id=" << HEX_PRINT(item->tunnel_id) << " for hash (unknown) and tunnel request id " << it->second.origin << std::endl;
#endif
			// Tunnel is ending here. Add it to the list of tunnels for the given hash.

			// 1 - find which file hash issued this request. This is not costly,
			// 	because there is not too much file hashes to be active at a time,
			// 	and this mostly prevents from sending the hash back in the tunnel.

#ifdef P3TURTLE_DEBUG
			bool ext_found = false ;
#endif
			for(std::map<TurtleFileHash,TurtleHashInfo>::iterator it(_incoming_file_hashes.begin());it!=_incoming_file_hashes.end();++it)
				if(it->second.last_request == item->request_id)
				{
#ifdef P3TURTLE_DEBUG
					ext_found = true ;
#endif

					{
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
						service = it->second.service ;

						locked_addDistantPeer(new_hash,item->tunnel_id) ;
						new_vpid = _local_tunnels[item->tunnel_id].vpid ; // save it for off-mutex usage.
					}
				}
#ifdef P3TURTLE_DEBUG
			if(!ext_found)
				std::cerr << "p3turtle: error. Could not find hash that emmitted tunnel request " << reinterpret_cast<void*>(item->tunnel_id) << std::endl ;
#endif
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
	if(new_tunnel && service != NULL)
		service->addVirtualPeer(new_hash,new_vpid,RsTurtleGenericTunnelItem::DIRECTION_SERVER) ;
}

// -----------------------------------------------------------------------------------//
// ------------------------------  IO with libretroshare  ----------------------------//
// -----------------------------------------------------------------------------------//
//


void RsTurtleStringSearchRequestItem::search(
        std::list<TurtleFileInfo>& result ) const
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
	rsFiles->SearchKeywords(
	            words, initialResults,
	            RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_SEARCHABLE, PeerId() );

#ifdef P3TURTLE_DEBUG
	std::cerr << initialResults.size() << " matches found." << std::endl ;
#endif
	result.clear() ;

	for(auto& it: std::as_const(initialResults))
	{
		// retain only file type
		if (it.type == DIR_TYPE_DIR)
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Skipping directory " << it->name << std::endl ;
#endif
			continue;
		}

		TurtleFileInfo i;
		i.hash = it.hash;
		i.size = it.size;
		i.name = it.name;

		result.push_back(i);
	}
}
void RsTurtleRegExpSearchRequestItem::search(std::list<TurtleFileInfo>& result) const
{
	/* call to core */
	std::list<DirDetails> initialResults;

	// to do: split search string into words.
    RsRegularExpression::Expression *exp = RsRegularExpression::LinearizedExpression::toExpr(expr) ;

	if(exp == NULL)
		return ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "Local search on exp: " << exp->toStdString() << std::endl;
#endif

	// now, search!
    rsFiles->SearchBoolExp(exp,initialResults,RS_FILE_HINTS_LOCAL | RS_FILE_HINTS_SEARCHABLE,PeerId());

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
        i.size = it->size ;
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
	std::cerr << "performing search. OwnId = " << _own_id << std::endl ;
#endif

	item->PeerId(_own_id) ;
	item->match_string = string_to_match ;
	item->request_id = id ;
	item->depth = 0 ;

	// send it

	handleSearchRequest(item) ;

	delete item ;

	return id ;
}
TurtleRequestId p3turtle::turtleSearch(const RsRegularExpression::LinearizedExpression& expr)
{
	// generate a new search id.

	TurtleRequestId id = generateRandomRequestId() ;

	// Form a request packet that simulates a request from us.
	//
	RsTurtleRegExpSearchRequestItem *item = new RsTurtleRegExpSearchRequestItem ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "performing search. OwnId = " << _own_id << std::endl ;
#endif

	item->PeerId(_own_id) ;
	item->expr = expr ;
	item->request_id = id ;
	item->depth = 0 ;

	// send it

	handleSearchRequest(item) ;

	delete item ;

	return id ;
}

TurtleRequestId p3turtle::turtleSearch(unsigned char *search_bin_data,uint32_t search_bin_data_len,RsTurtleClientService *client_service)
{
	// generate a new search id.

	TurtleRequestId id = generateRandomRequestId() ;

	// Form a request packet that simulates a request from us.
	//
	RsTurtleGenericSearchRequestItem item ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "performing search. OwnId = " << _own_id << std::endl ;
#endif

	item.PeerId(_own_id) ;
    item.service_id = client_service->serviceId();
	item.search_data = search_bin_data ;
	item.search_data_len = search_bin_data_len ;
	item.request_id = id ;
	item.depth = 0 ;

	// send it

	handleSearchRequest(&item) ;

	return id ;
}

void p3turtle::monitorTunnels(const RsFileHash& hash,RsTurtleClientService *client_service,bool allow_multi_tunnels)
{
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		// First, check if the hash is tagged for removal (there's a delay)

		if(_hashes_to_remove.erase(hash) > 0)
		{
#ifdef P3TURTLE_DEBUG
			TURTLE_DEBUG() << "p3turtle: File hash " << hash << " Was scheduled for removal. Canceling the removal." << std::endl ;
#endif
		}

		// Then, check if the hash is already there
		//
		if(_incoming_file_hashes.find(hash) != _incoming_file_hashes.end())	// download already asked.
		{
#ifdef P3TURTLE_DEBUG
			TURTLE_DEBUG() << "p3turtle: File hash " << hash << " already in pool. Returning." << std::endl ;
#endif
			return ;
		}
#ifdef P3TURTLE_DEBUG
		TURTLE_DEBUG() << "p3turtle: Received order for turtle download fo hash " << hash << std::endl ;
#endif

		// No tunnels at start, but this triggers digging new tunnels.
		//
		_incoming_file_hashes[hash].tunnels.clear();
        _incoming_file_hashes[hash].use_aggressive_mode = allow_multi_tunnels ;

		// also should send associated request to the file transfer module.
		_incoming_file_hashes[hash].last_digg_time = RSRandom::random_u32()%10 ;
		_incoming_file_hashes[hash].service = client_service ;
	}
}


//    RsTurtleGxsSearchResultGroupSummaryItem *gxs_sr_gs = dynamic_cast<RsTurtleGxsSearchResultGroupSummaryItem*>(item) ;
//
//    if(gxs_sr_gs != NULL)
//    {
//		RsServer::notify()->notifyTurtleSearchResult(gxs_sr_gs->request_id,gxs_sr_gs->result) ;
//        return ;
//    }
//    RsTurtleGxsSearchResultGroupDataItem *gxs_sr_gd = dynamic_cast<RsTurtleGxsSearchResultGroupDataItem*>(item) ;
//
//    if(gxs_sr_gd != NULL)
//    {
//#warning MISSING CODE HERE TO HANDLE ENCRYPTED INCOMING GROUP DATA.
//		//RsServer::notify()->notifyTurtleSearchResult(gxs_sr_gd->request_id,gxs_sr_gd->encrypted_nxs_group) ;
//        return ;
//    }

/// Warning: this function should never be called while the turtle mutex is locked.
/// Otherwize this is a possible source of cross-lock with the File mutex.
//
bool p3turtle::performLocalHashSearch(const TurtleFileHash& hash,const RsPeerId& peer_id,RsTurtleClientService *& service)
{
    std::map<uint16_t,RsTurtleClientService*> client_map ;
	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

		if(_registered_services.empty())
        {
			std::cerr << "Turtle router has no services registered. Tunnel requests cannot be handled." << std::endl;
            return false ;
        }

		client_map = _registered_services ;
	}

	for(auto it(client_map.begin());it!=client_map.end();++it)
		if( (*it).second->handleTunnelRequest(hash,peer_id))
		{
			service = it->second ;
			return true ;
		}

	return false ;
}


void p3turtle::registerTunnelService(RsTurtleClientService *service)
{
	std::cerr << "p3turtle: registered new tunnel service with ID=" << std::hex << service->serviceId() << std::dec << " and pointer " << (void*)service << std::endl;

	_registered_services[service->serviceId()] = service ;
	_serialiser->registerClientService(service) ;
}

static std::string printFloatNumber(float num,bool friendly=false)
{
	if(friendly)
	{
		char tmp[100] ;
		std::string units[4] = { "B/s","KB/s","MB/s","GB/s" } ;

		int k=0 ;
        while(num >= 800.0f && k<4)
			num /= 1024.0f,++k;

		sprintf(tmp,"%3.2f %s",num,units[k].c_str()) ;
		return std::string(tmp) ;
	}
	else
	{
		std::string out ;
		rs_sprintf(out, "%g", num) ;
		return out ;
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
		std::string out ;
		rs_sprintf(out, "%lld", num) ;
		return out ;
	}
}

void p3turtle::getTrafficStatistics(TurtleTrafficStatisticsInfo& info) const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	info = _traffic_info ;

	float distance_to_maximum	= std::min(100.0f,info.tr_up_Bps/(float)(TUNNEL_REQUEST_PACKET_SIZE*_max_tr_up_rate)) ;
	info.forward_probabilities.clear() ;

	std::set<RsPeerId> onlineIds ;
	mServiceControl->getPeersConnected(_service_type, onlineIds);

	int nb_online_ids = onlineIds.size() ;

	for(int i=0;i<=6;++i)
	{
		float corrected_distance 	= pow(distance_to_maximum,DISTANCE_SQUEEZING_POWER) ;
		float forward_probability	= pow(depth_peer_probability[i],corrected_distance) ;

		if(forward_probability * nb_online_ids < 1.0f && nb_online_ids > 0)
		{
			forward_probability = 1.0f / nb_online_ids ;

			if(_traffic_info.tr_dn_Bps / (float)TUNNEL_REQUEST_PACKET_SIZE > _max_tr_up_rate)
				forward_probability *= _max_tr_up_rate*TUNNEL_REQUEST_PACKET_SIZE / (float)_traffic_info.tr_dn_Bps ;
		}

		info.forward_probabilities.push_back(forward_probability) ;
	}
}

std::string p3turtle::getPeerNameForVirtualPeerId(const RsPeerId& virtual_peer_id)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	std::string name = "unknown";
	std::map<TurtleVirtualPeerId,TurtleTunnelId>::const_iterator it(_virtual_peers.find(virtual_peer_id)) ;
	if(it != _virtual_peers.end())
	{
		std::map<TurtleTunnelId,TurtleTunnel>::iterator it2( _local_tunnels.find(it->second) ) ;
		if(it2 != _local_tunnels.end())
		{
			if(it2->second.local_src == _own_id)
				mLinkMgr->getPeerName(it2->second.local_dst,name);
			else
				mLinkMgr->getPeerName(it2->second.local_src,name);
		}
	}
	return name;
}

bool p3turtle::encryptData(const unsigned char *clear_data,uint32_t clear_data_size,uint8_t *encryption_master_key,RsTurtleGenericDataItem *& encrypted_item)
{
    unsigned char *encrypted_data = NULL ;
    uint32_t encrypted_data_len = 0 ;

    if(!librs::crypto::encryptAuthenticateData(clear_data,clear_data_size,encryption_master_key,encrypted_data,encrypted_data_len))
    {
        delete encrypted_item ;
        return false ;
    }
	encrypted_item = new RsTurtleGenericDataItem ;

    encrypted_item->data_bytes = encrypted_data ;
    encrypted_item->data_size = encrypted_data_len ;
    return true;
}

bool p3turtle::decryptItem(const RsTurtleGenericDataItem* encrypted_item, uint8_t *encryption_master_key, unsigned char *& decrypted_data, uint32_t& decrypted_data_size)
{
   return librs::crypto::decryptAuthenticateData((unsigned char*)encrypted_item->data_bytes,encrypted_item->data_size,encryption_master_key,decrypted_data,decrypted_data_size);
}

void p3turtle::getInfo(	std::vector<std::vector<std::string> >& hashes_info,
								std::vector<std::vector<std::string> >& tunnels_info,
								std::vector<TurtleSearchRequestDisplayInfo >& search_reqs_info,
								std::vector<TurtleTunnelRequestDisplayInfo >& tunnel_reqs_info) const
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	rstime_t now = time(NULL) ;

	hashes_info.clear() ;

	for(std::map<TurtleFileHash,TurtleHashInfo>::const_iterator it(_incoming_file_hashes.begin());it!=_incoming_file_hashes.end();++it)
	{
		hashes_info.push_back(std::vector<std::string>()) ;

		std::vector<std::string>& hashes(hashes_info.back()) ;

        hashes.push_back(it->first.toStdString()) ;
		//hashes.push_back(it->second.name) ;
		hashes.push_back("Name not available") ;
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
			tunnel.push_back(it->second.local_src.toStdString()) ;

		if(mLinkMgr->getPeerName(it->second.local_dst,name))
			tunnel.push_back(name) ;
		else
			tunnel.push_back(it->second.local_dst.toStdString());

        tunnel.push_back(it->second.hash.toStdString()) ;
		tunnel.push_back(printNumber(now-it->second.time_stamp) + " secs ago") ;
		tunnel.push_back(printFloatNumber(it->second.speed_Bps,false)) ; //
	}

	search_reqs_info.clear();

	for(std::map<TurtleSearchRequestId,TurtleSearchRequestInfo>::const_iterator it(_search_requests_origins.begin());it!=_search_requests_origins.end();++it)
	{
		TurtleSearchRequestDisplayInfo info ;

		info.request_id 		= it->first ;
		info.source_peer_id 	= it->second.origin ;
		info.age 				= now - it->second.time_stamp ;
		info.depth 				= it->second.depth ;
		info.keywords           = it->second.keywords ;
		info.hits               = it->second.result_count ;

		search_reqs_info.push_back(info) ;
	}

	tunnel_reqs_info.clear();

	for(std::map<TurtleSearchRequestId,TurtleTunnelRequestInfo>::const_iterator it(_tunnel_requests_origins.begin());it!=_tunnel_requests_origins.end();++it)
	{
		TurtleTunnelRequestDisplayInfo info ;

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

	rstime_t now = time(NULL) ;

	std::cerr << std::endl ;
	std::cerr << "********************** Turtle router dump ******************" << std::endl ;
	std::cerr << "  Active incoming file hashes: " << _incoming_file_hashes.size() << std::endl ;
	for(std::map<TurtleFileHash,TurtleHashInfo>::const_iterator it(_incoming_file_hashes.begin());it!=_incoming_file_hashes.end();++it)
	{
		std::cerr << "    hash=0x" << it->first << ", tunnel ids =" ;
		for(std::vector<TurtleTunnelId>::const_iterator it2(it->second.tunnels.begin());it2!=it->second.tunnels.end();++it2)
			std::cerr << " " << HEX_PRINT(*it2) ;
		//std::cerr << ", last_req=" << (void*)it->second.last_request << ", time_stamp = " << it->second.time_stamp << "(" << now-it->second.time_stamp << " secs ago)" << std::endl ;
	}
	std::cerr << "  Active outgoing file hashes: " << _outgoing_tunnel_client_services.size() << std::endl ;
    for(std::map<TurtleTunnelId,RsTurtleClientService*>::const_iterator it(_outgoing_tunnel_client_services.begin());it!=_outgoing_tunnel_client_services.end();++it)
        std::cerr << "    TID=0x" << it->first << std::endl ;

	std::cerr << "  Local tunnels:" << std::endl ;
	for(std::map<TurtleTunnelId,TurtleTunnel>::const_iterator it(_local_tunnels.begin());it!=_local_tunnels.end();++it)
		std::cerr << "    " << HEX_PRINT(it->first) << ": from="
					<< it->second.local_src << ", to=" << it->second.local_dst
					<< ", hash=0x" << it->second.hash << ", ts=" << it->second.time_stamp << " (" << now-it->second.time_stamp << " secs ago)"
					<< ", peer id =" << it->second.vpid << std::endl ;

	std::cerr << "  buffered request origins: " << std::endl ;
	std::cerr << "    Search requests: " << _search_requests_origins.size() << std::endl ;

	for(std::map<TurtleSearchRequestId,TurtleSearchRequestInfo>::const_iterator it(_search_requests_origins.begin());it!=_search_requests_origins.end();++it)
		std::cerr 	<< "      " << HEX_PRINT(it->first) << ": from=" << it->second.origin
						<< ", ts=" << it->second.time_stamp << " (" << now-it->second.time_stamp
						<< " secs ago)"
		                << it->second.result_count << " hits" << std::endl ;

	std::cerr << "    Tunnel requests: " << _tunnel_requests_origins.size() << std::endl ;
	for(std::map<TurtleTunnelRequestId,TurtleTunnelRequestInfo>::const_iterator it(_tunnel_requests_origins.begin());it!=_tunnel_requests_origins.end();++it)
		std::cerr 	<< "      " << HEX_PRINT(it->first) << ": from=" << it->second.origin
						<< ", ts=" << it->second.time_stamp << " (" << now-it->second.time_stamp
						<< " secs ago)" << std::endl ;

	std::cerr << "  Virtual peers:" << std::endl ;
	for(std::map<TurtleVirtualPeerId,TurtleTunnelId>::const_iterator it(_virtual_peers.begin());it!=_virtual_peers.end();++it)
		std::cerr << "    id=" << it->first << ", tunnel=" << HEX_PRINT(it->second) << std::endl ;
	std::cerr << "  Online peers: " << std::endl ;
//	for(std::list<pqipeer>::const_iterator it(_online_peers.begin());it!=_online_peers.end();++it)
//		std::cerr << "    id=" << it->id << ", name=" << it->name << ", state=" << it->state << ", actions=" << it->actions << std::endl ;
}
#endif

#ifdef TUNNEL_STATISTICS
void p3turtle::TS_dumpState()
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	rstime_t now = time(NULL) ;
	std::cerr << "Dumping tunnel statistics:" << std::endl;

	std::cerr << "TR Bounces: " << TS_request_bounces.size() << std::endl;
	for(std::map<TurtleTunnelRequestId,std::vector<rstime_t> >::const_iterator it(TS_request_bounces.begin());it!=TS_request_bounces.end();++it)
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
	for(std::map<TurtleFileHash, std::vector<std::pair<rstime_t,TurtleTunnelRequestId> > >::const_iterator it(TS_request_time_stamps.begin());it!=TS_request_time_stamps.end();++it)
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
