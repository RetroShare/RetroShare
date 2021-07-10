/*******************************************************************************
 * libretroshare/src/turtle: p3turtle.h                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2009-2018 by Cyril Soler <csoler@users.sourceforge.net>           *
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
#pragma once

//====================================== General setup of the router ===================================//
//
// This class implements the turtle hopping router. It basically serves as
// - a cache of turtle tunnels which are the communicating ways between distant peers.
// 	- turtle tunnels are either end-point tunnels, or transitory points, in which case items are just
// 		re-serialized and passed on along the tunnel.
// 	- turtle tunnels are dug on request when calling diggTurtleTunnel(const std::string& hash)
// 		this command lets a trace in each peer along the tunnel of where
// 		packets come from and where they should go. Doing so, once a tunnel is
// 		dug, packets are directly forwarded to the correct peer.
// - an entry point for search request from the interface
// - search results, as they come back, are forwarded upwards with some additional info:
// 	- depth					// depth of the file. setup to 1 for immediate friends and 2 for long distance friends.
// 	- hash					// hash of the file found
// 	- name					// name of the file found
// 	- search request id.	// 
//
// - when downloading:
// 	- for a given hash, a set of starting tunnels is maintained. Transitory
// 	tunnels are also maintained for other hashes as requested by distant
// 	peers.
//
//============================================= Operations =============================================//
//
//  A download session works as follows:
//     Initiation:
//      1 - the user searches for files (turtle search), and selects one and clicks download.
//      2 - In parallel:
//           - the ft module gets a request, and searches for peers to provide this using its search modules.
//           - the turtle router is informed that a turtle download will happen with the given hash, so 
//             it initiates tunnels for this hash.
//     In a loop:
//      3 - the ft module asks the hash to the turtle searchModule, and sends file requests to the pqi 
//           interface of this module.
//      4 - the turtle pqi interface forwards these requests to the turtle router, which sends them to 
//           the correct peers, selecting randomly among all the possible tunnels for this hash.
//      5 - when a file data packet gets back, the turtle router forwards it back to the file transfer module.
//
//================================ connection to the file transfer module ===============================//
//
//  The turtle router should provide the ft module with the necessary interface for asking files, and
//  retreiving data:
// 	- a search module that responds with a given fake peer id for hash request for which it has tunnels.
// 	- a pqi interface to ask for file data
// 	- p3turtle sends back file data packets to the file transfer module
//
//========================================== Tunnel usage rules ========================================//
//
// Tunnels should be used according to their capacity. This is an unsolved problem as for now.
//
//======================================= Tunnel maintenance rules =====================================//
//
// P3turtle should derive from pqihandler, just as p3disc, so that newly connected peers should trigger 
// asking for new tunnels, and disconnecting peers should produce a close tunnel packet. To simplify this,
// I maintain a time stamp in tunnels, that is updated each time a file data packet travels in the tunnel.
// Doing so, if a tunnel is not used for some time, it just disapears. Additional rules apply:
//
// 	- when a peer A connects:
// 		- initiate new tunnels for all active file hashes (go through the list of hashes) by 
// 		  asking to A, for the same hash and the same source. Only report tunnels for which the destination 
// 		  endpoint is different, which should not happen in fact, because of bouncing gards.
//
// 	- when a peer A disconnects.
// 		- do nothing.
//
//    - when receive open tunnel from A 
//    	- check whether it's a bouncing request. If yes, give up.
//       - check hash against local files.
//          if > 0 
//             return tunnel ok item. No need to go forward, as sub tunnels are not useful.
//          else
//             forward request to peers, notting source and hashes.
//
//    - when receive tunnel ok from A 
//    	- no need to check whether we already have this tunnel, as bouncing gards prevent this.
//    	- leave a trace for the tunnel, and send (forward) backward.
//
// Ids management:
//    - tunnel ids should be identical for requests between 2 same peers for the same file hash.
//    - tunnel ids should be asymetric
//    - tunnel requests should never be identical, to allow searching multiple times for the same string.
//  So:
//  	- when issuing an open tunnel order, 
//  		- a random request id is generated and used for packet routing
//  		- a partial tunnel id is build, which is unique to the pair (source,file hash)
//  	- when tunnel_ok is sent back, the tunnel id is completed so that it is unique to the
//  		triplet (source, destination, file hash).
//
// For these needs, tunnels are represented by:
// 	- their file hash. Each tunnel is only designed for transferring a single and same file.
// 	- their local endpoints id. These are the ids of the peers in direction to the source and destination.
// 	- the tunnel id, which is unique to the triple hash+global source+global destination.
// 	- there is a difference between source and destination in tunnels. The source is the file asker, the 
// 	  destination is the file provider. This helps sorting tunnels.
// 	- a timestamp, used for cleaning unused tunnels.
//
// The turtle router has:
// 	- a list of search requests and where to bounce them back.
// 	- a list of tunnel digging requests and where to bounce them, back.
// 	- a list of active file hashes, for which is should constantly maintain tunnels.
// 	- a list of active tunnels, some being transitory, some being endpoints.
//
// Turtle router entries:
// 	- a function for performing turtle search
// 	- a function for handling tunnels for a given file hash.
//
// Questions:
// 	- should tunnels be re-used ? nope. The only useful case would be when two peers are exchanging files, which happens quite rarely.
//

#include <string>
#include <list>
#include <set>

#include "pqi/pqinetwork.h"
#include "pqi/pqi.h"
#include "pqi/pqimonitor.h"
#include "ft/ftcontroller.h"
#include "pqi/p3cfgmgr.h"
#include "services/p3service.h"
#include "ft/ftsearch.h"
#include "retroshare/rsturtle.h"
#include "rsturtleitem.h"
#include "turtleclientservice.h"
#include "turtlestatistics.h"

//#define TUNNEL_STATISTICS

class ftServer ;
class p3LinkMgr;
class ftDataMultiplex;
class RsSerialiser;

static const int TURTLE_MAX_SEARCH_DEPTH = 6 ;
static const int TURTLE_MAX_SEARCH_REQ_ACCEPTED_SERIAL_SIZE = 200 ;

// This classes are used to keep trace of requests (searches and tunnels).
//
class TurtleSearchRequestInfo
{
	public:
		TurtlePeerId origin ;         // where the request came from.
		uint32_t	 time_stamp ;     // last time the tunnel was actually used. Used for cleaning old tunnels.
		int          depth ;          // depth of the request. Used to optimize tunnel length.
		uint32_t     result_count;    // responses to this request. Useful to avoid spamming tunnel responses.
		std::string  keywords;
        uint16_t     service_id;      // ID of the client service who issues the request. This is null if the request does not have a local origin.
        uint32_t     max_allowed_hits;// Max number of hits allowed for this search. This actually depends on the type of search (files, GXS groups, GXS group data, etc)
};
class TurtleTunnelRequestInfo
{
	public:
		TurtlePeerId origin ;         // where the request came from.
		uint32_t	time_stamp ;      // last time the tunnel was actually used. Used for cleaning old tunnels.
		int depth ;	                  // depth of the request. Used to optimize tunnel length.
		std::set<uint32_t> responses; // responses to this request. Useful to avoid spamming tunnel responses.
};

class TurtleTunnel
{
	public:
		/* For all tunnels */

		TurtlePeerId local_src ;		// where packets come from. Direction to the source.
		TurtlePeerId local_dst ;		// where packets should go. Direction to the destination.
		uint32_t	time_stamp ;			// last time the tunnel was actually used. Used for cleaning old tunnels.
		uint32_t transfered_bytes ;	// total bytes transferred in this tunnel.
		float speed_Bps ;             // speed of the traffic through the tunnel

		/* For ending/starting tunnels only. */

		TurtleFileHash hash;				// Hash of the file for this tunnel
		TurtleVirtualPeerId vpid;		// Virtual peer id for this tunnel.
};

// This class keeps trace of the activity for the file hashes the turtle router is asked to monitor.
//

class TurtleHashInfo
{
	public:
		std::vector<TurtleTunnelId> tunnels ;		// list of active tunnel ids for this file hash
        TurtleRequestId last_request ;			// last request for the tunnels of this hash
        rstime_t last_digg_time ;				// last time the tunnel digging happenned.
        RsTurtleClientService *service ; 		// client service to which items should be sent. Never NULL.
        bool use_aggressive_mode ;			// allow to re-digg tunnels even when some are already available
};

// Subclassing:
//
//		Class      | Brings what      | Usage
// 	-----------+------------------+------------------------------------------------------
// 	p3Service  | sendItem()       | handle packet sending/receiving to/from friend peers.
// 	pqiMonitor | configChanged()  | handle who's connecting/disconnecting to dig new tunnels
// 	RsTurtle   | start/stop file()| brings interface for turtle service
// 	ftSearch   | search()         | used to allow searching for monitored files.
// 	p3Config   | ConfigChanged()  | used to load/save .cfg file for turtle variales.
// 	-----------+------------------+------------------------------------------------------
//
class p3turtle: public p3Service, public RsTurtle, public p3Config
{
	public:
		p3turtle(p3ServiceControl *sc,p3LinkMgr *lm) ;
		virtual RsServiceInfo getServiceInfo();

		// Enables/disable the service. Still ticks, but does nothing. Default is true.
		//
		virtual void setEnabled(bool) ;	
		virtual bool enabled() const ;	

		// This is temporary, used by Operating Mode. 
		// Turtle operates when both enabled() && sessionEnabled() are true.
		virtual void setSessionEnabled(bool);
		virtual bool sessionEnabled() const;

		// Lauches a search request through the pipes, and immediately returns
		// the request id, which will be further used by the gui to store results
		// as they come back.
		//
        // The first two methods are old style search requests for FT, while the 3rd one is using a generic search data type, that is only to
        // be deserialized by the service. The memory ownership is kept by the calling function. Similarly, the search response will be a
        // generic data type that is to be deserialized by the client service.
        //
        // Eventually, search requests will use the generic system
        // even for FT. We need to keep the old method for a while for backward compatibility.
        //
        virtual TurtleRequestId turtleSearch(const RsRegularExpression::LinearizedExpression& expr) ;
        virtual TurtleRequestId turtleSearch(const std::string& string_to_match) ;
        virtual TurtleRequestId turtleSearch(unsigned char *search_bin_data,uint32_t search_bin_data_len,RsTurtleClientService *client_service) ;

		// Initiates tunnel handling for the given file hash.  tunnels.  Launches
		// an exception if an error occurs during the initialization process. The
		// turtle router itself does not initiate downloads, it only maintains
		// tunnels for the given hash. The download should be driven by the file
		// transfer module. Maybe this function can do the whole thing:
		//  - initiate tunnel handling
		//  - send the file request to the file transfer module
		//  - populate the file transfer module with the adequate pqi interface and search module.
		//
		//  This function should be called in addition to ftServer::FileRequest() so that the turtle router
		//  automatically provide tunnels for the file to download.
		//
        virtual void monitorTunnels(const RsFileHash& file_hash,RsTurtleClientService *client_service, bool allow_multi_tunnels) ;

		/// This should be called when canceling a file download, so that the turtle router stops
		/// handling tunnels for this file.
		///
		virtual void stopMonitoringTunnels(const RsFileHash& file_hash) ;

        /// This is provided to turtle clients to force the TR to ask tunnels again. To be used wisely:
        /// too many tunnel requests will kill the network. This might be useful to speed-up the re-establishment
        /// of tunnels that have become irresponsive.

        virtual void forceReDiggTunnels(const TurtleFileHash& hash) ;

        /// Adds a client tunnel service. This means that the service will be added
		/// to the list of services that might respond to tunnel requests.
		/// Example tunnel services include:
		///
		///	p3ChatService:		tunnels correspond to private distant chatting
		///	ftServer		 : 	tunnels correspond to file data transfer
		///
		virtual void registerTunnelService(RsTurtleClientService *service) ;

		virtual std::string getPeerNameForVirtualPeerId(const RsPeerId& virtual_peer_id);
		
		/// get info about tunnels
		virtual void getInfo(std::vector<std::vector<std::string> >&,
									std::vector<std::vector<std::string> >&,
									std::vector<TurtleSearchRequestDisplayInfo >&,
									std::vector<TurtleTunnelRequestDisplayInfo >&) const ;
		std::string GetFilenameForHash(TurtleFileHash hash, RsTurtleClientService *service) const;
		
		virtual void getTrafficStatistics(TurtleTrafficStatisticsInfo& info) const ;

		/************* from p3service *******************/

		/// This function does many things:
		/// 	- It handles incoming and outgoing packets
		/// 	- it sorts search requests and forwards search results upward.
		/// 	- it cleans unused (tunnel+search) requests.
		/// 	- it maintains the pool of tunnels, for each request file hash.
		///
		virtual int tick();

		virtual void getItemNames(std::map<uint8_t,std::string>& names) const;

		/************* from p3Config *******************/
		virtual RsSerialiser *setupSerialiser() ;
		virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
		virtual bool loadList(std::list<RsItem*>& /*load*/) ;

		/************* Communication with clients *******************/
		/// Does the turtle router manages tunnels to this peer ? (this is not a
		/// real id, but a fake one, that the turtle router is capable of connecting with a tunnel id).
		virtual bool isTurtlePeer(const RsPeerId& peer_id) const ;

		/// sets/gets the max number of forwarded tunnel requests per second.
		virtual void setMaxTRForwardRate(int max_tr_up_rate) ;
		virtual int getMaxTRForwardRate() const ;
		
		/// Examines the peer id, finds the turtle tunnel in it, and respond yes if the tunnel is ok and operational.
		bool isOnline(const RsPeerId& peer_id) const ;

		/// Returns a unique peer id, corresponding to the given tunnel.
		RsPeerId getTurtlePeerId(TurtleTunnelId tid) const ;
	
		/// returns the list of virtual peers for all tunnels.
		void getSourceVirtualPeersList(const TurtleFileHash& hash,std::list<pqipeer>& list) ;

		/// Send a data request into the correct tunnel for the given file hash
		void sendTurtleData(const RsPeerId& virtual_peer_id, RsTurtleGenericTunnelItem *item) ;

		/// Encrypts/decrypts an item, using a autenticated construction + chacha20, based on the given 32 bytes master key.
		/// Input values are not touched (memory is not released). Memory ownership of outputs is left to the client.
		///
		static bool encryptData(const unsigned char *clear_data,uint32_t clear_data_size,uint8_t *encryption_master_key,RsTurtleGenericDataItem *& encrypted_item);
		static bool decryptItem(const RsTurtleGenericDataItem *item, uint8_t* encryption_master_key, unsigned char *& decrypted_data,uint32_t& decrypted_data_size);

	private:
		//--------------------------- Admin/Helper functions -------------------------//
		
		/// Generates a cyphered combination of ownId() and file hash
		uint32_t generatePersonalFilePrint(const TurtleFileHash&,uint32_t seed,bool) ;	

		/// Generates a random uint32_t number.
		uint32_t generateRandomRequestId() ;								

		/// Auto cleaning of unused tunnels, search requests and tunnel requests.
		void autoWash() ;															

		//------------------------------ Tunnel handling -----------------------------//

		/// initiates tunnels from here to any peers having the given file hash
		TurtleRequestId diggTunnel(const TurtleFileHash& hash) ;	

		/// adds info related to a new virtual peer.
		void locked_addDistantPeer(const TurtleFileHash&, TurtleTunnelId) ;	

		/// estimates the speed of the traffic into tunnels.
		void estimateTunnelSpeeds() ;

		//----------------------------- Routing functions ----------------------------//
		
		/// Handle tunnel digging for current file hashes
		void manageTunnels() ;									

		/// Closes a given tunnel. Should be called with mutex set.
		/// The hashes and peers to remove (by calling 
		/// ftController::removeFileSource() are happended to the supplied vector 
		/// so that they can be removed off the turtle mutex.
		void locked_closeTunnel(TurtleTunnelId tid,std::vector<std::pair<RsTurtleClientService*,std::pair<TurtleFileHash,TurtleVirtualPeerId> > >& peers_to_remove) ;	

		/// Main routing function
		int handleIncoming(); 									

		/// Generic routing function for all tunnel packets that derive from RsTurtleGenericTunnelItem
		void routeGenericTunnelItem(RsTurtleGenericTunnelItem *item) ;

		/// specific routing functions for handling particular packets.
		void handleRecvGenericTunnelItem(RsTurtleGenericTunnelItem *item);
		bool getTunnelServiceInfo(TurtleTunnelId, RsPeerId& virtual_peer_id, RsFileHash& hash, RsTurtleClientService*&) ;

		// following functions should go to ftServer
		void handleSearchRequest(RsTurtleSearchRequestItem *item);		
		void handleSearchResult(RsTurtleSearchResultItem *item);
		void handleTunnelRequest(RsTurtleOpenTunnelItem *item);		
		void handleTunnelResult(RsTurtleTunnelOkItem *item);		

		//------ Functions connecting the turtle router to other components.----------//
		
		/// Performs a search calling local cache and search structure.
		void performLocalSearch        (RsTurtleSearchRequestItem        *item, uint32_t& req_result_count,std::list<RsTurtleSearchResultItem*>& result,uint32_t& max_allowed_hits) ;
		void performLocalSearch_files  (RsTurtleFileSearchRequestItem    *item, uint32_t& req_result_count, std::list<RsTurtleSearchResultItem*>& result, uint32_t &max_allowed_hits) ;
		void performLocalSearch_generic(RsTurtleGenericSearchRequestItem *item, uint32_t& req_result_count, std::list<RsTurtleSearchResultItem*>& result, uint32_t &max_allowed_hits) ;

		/// Returns true if the file with given hash is hosted locally, and accessible in anonymous mode the supplied peer.
		virtual bool performLocalHashSearch(const TurtleFileHash& hash,const RsPeerId& client_peer_id,RsTurtleClientService *& service);

		//--------------------------- Local variables --------------------------------//
		
		/* data */
		p3ServiceControl   *mServiceControl;
		p3LinkMgr          *mLinkMgr;
		RsTurtleSerialiser *_serialiser ;
		RsPeerId            _own_id ;

		mutable RsMutex mTurtleMtx;

		/// keeps trace of who emmitted a given search request
		std::map<TurtleSearchRequestId,TurtleSearchRequestInfo> 	_search_requests_origins ;

		/// keeps trace of who emmitted a tunnel request
		std::map<TurtleTunnelRequestId,TurtleTunnelRequestInfo> 	_tunnel_requests_origins ;

		/// stores adequate tunnels for each file hash locally managed
		std::map<TurtleFileHash,TurtleHashInfo>			   	_incoming_file_hashes ;

		/// stores file info for each file we provide.
        std::map<TurtleTunnelId,RsTurtleClientService *>	_outgoing_tunnel_client_services ;

		/// local tunnels, stored by ids (Either transiting or ending).
		std::map<TurtleTunnelId,TurtleTunnel > 				_local_tunnels ;				

		/// Peers corresponding to each tunnel.
		std::map<TurtleVirtualPeerId,TurtleTunnelId>			_virtual_peers ;				

		/// Hashes marked to be deleted.
        std::set<TurtleFileHash>								_hashes_to_remove ;

		/// List of client services that have regitered.
		std::map<uint16_t,RsTurtleClientService*>						_registered_services ;

		rstime_t _last_clean_time ;
		rstime_t _last_tunnel_management_time ;
		rstime_t _last_tunnel_campaign_time ;
		rstime_t _last_tunnel_speed_estimate_time ;

		std::list<pqipeer> _online_peers;

		/// used to force digging new tunnels
		//bool _force_digg_new_tunnels ;

		/// used as a bias to introduce randomness in a consistent way, for
		/// altering tunnel request depths, and tunnel re-routing actions.
		///
		uint32_t _random_bias ;

		// Used to collect statistics on turtle traffic.
		//
		TurtleTrafficStatisticsInfoOp _traffic_info ;			// used for recording speed
		TurtleTrafficStatisticsInfoOp _traffic_info_buffer ;	// used as a buffer to collect bytes

		float _max_tr_up_rate ;
		bool  _turtle_routing_enabled ;
		bool  _turtle_routing_session_enabled ;

		// p3ServiceControl service type

		uint32_t _service_type ;

	RS_SET_CONTEXT_DEBUG_LEVEL(1)

#ifdef P3TURTLE_DEBUG
		// debug function
		void dumpState() ;
#endif
#ifdef TUNNEL_STATISTICS
		void TS_dumpState();
#endif
};
