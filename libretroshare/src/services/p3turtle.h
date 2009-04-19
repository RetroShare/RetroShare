/*
 * libretroshare/src/services: p3turtle.h
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
// 	- depth					// depth of the file. This is here for debug bug will disapear for anonymity.
// 	- peer id				// peer id owning the file. This is here for debug bug will disapear for anonymity.
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
//================================ connexion to the file transfer module ===============================//
//
//  The turtle router should provide the ft module with the necessary interface for asking files, and
//  retreiving data:
// 	- a search module that responds with a given fake peer id for hash request for which it has tunnels.
// 	- a pqi interface to ask for file data
// 	- p3turtle sends back file data packets to the file transfer module
//
//========================================== Tunnel usage rules ========================================//
//
// Tunnels should be used according to their capacity. This is an unresolved problem as for now.
//
//======================================= Tunnel maintenance rules =====================================//
//
// P3turtle should derive from pqihandler, just as p3disc, so that newly connected peers should trigger 
// asking for new tunnels, and disconnecting peers should produce a close tunnel packet.
//
// 	- when a peer A connects:
// 		- initiate new tunnels for all active file hashes (go through the list of hashes) by 
// 		  asking to A, for the same hash and the same source. Only report tunnels for which the destination 
// 		  endpoint is different, which should not happen in fact, because of bouncing gards.
//
// 	- when a peer A disconnects.
// 		- close tunnels whose destination is beyond A by sending a close request backward.
// 		- close tunnels whose source is beyond A by sending a forward close request.
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
//    - when receive close tunnel from A 
//    	- if I am the endpoint
//    		- locally close the tunnel.
//    		- respond with tunnel closed.
//    	- otherwise, block the tunnel, and forward close tunnel to tunnel destination.
//
//    - when receive tunnel closed from A 
//    	- locally close the tunnel
//    	- forward back
//
// Ids management:
//    - tunnel ids should be identical for requests between 2 same peers for the same file hash.
//    - tunnel requests ids do not need to be identical.
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
// 	- a function for downloading files.
//
// Questions:
// 	- should tunnels be re-used ? nope. The only useful case would be when two peers are exchanging files, which happens quite rarely.
// 	- at a given moment, there is at most 1 tunnel for a given triplet (hash, source, destination).


#ifndef MRK_PQI_TURTLE_H
#define MRK_PQI_TURTLE_H

#include <string>
#include <list>

#include "pqi/pqinetwork.h"
#include "pqi/pqi.h"
#include "pqi/pqimonitor.h"
#include "services/p3service.h"
#include "serialiser/rsserviceids.h"
#include "rsiface/rsturtle.h"

class p3AuthMgr;
class p3ConnectMgr;

const uint8_t RS_TURTLE_SUBTYPE_SEARCH_REQUEST = 0x01 ;
const uint8_t RS_TURTLE_SUBTYPE_SEARCH_RESULT  = 0x02 ;
const uint8_t RS_TURTLE_SUBTYPE_OPEN_TUNNEL    = 0x03 ;
const uint8_t RS_TURTLE_SUBTYPE_TUNNEL_OK      = 0x04 ;
const uint8_t RS_TURTLE_SUBTYPE_CLOSE_TUNNEL   = 0x05 ;
const uint8_t RS_TURTLE_SUBTYPE_TUNNEL_CLOSED  = 0x06 ;

static const int TURTLE_MAX_SEARCH_DEPTH = 6 ;

typedef std::string 	TurtlePeerId ;
typedef std::string 	TurtleFileHash ;
typedef std::string 	TurtleFileName ;
typedef TurtleRequestId TurtleSearchRequestId ;

typedef uint32_t TurtleTunnelRequestId ;
typedef uint32_t TurtleTunnelId ;

class RsTurtleItem: public RsItem
{
	public:
		RsTurtleItem(uint8_t turtle_subtype) : RsItem(RS_PKT_VERSION_SERVICE,RS_SERVICE_TYPE_TURTLE,turtle_subtype) {}

		virtual bool serialize(void *data,uint32_t& size) = 0 ;	// Isn't it better that items can serialize themselves ?
		virtual uint32_t serial_size() = 0 ; 							// deserialise is handled using a constructor

		virtual void clear() {} 
};

class RsTurtleSearchResultItem: public RsTurtleItem
{
	public:
		RsTurtleSearchResultItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_SEARCH_RESULT) {}
		RsTurtleSearchResultItem(void *data,uint32_t size) ;		// deserialization

		uint16_t depth ;
		uint8_t peer_id[16];				// peer id. This will eventually be obfuscated in some way.

		TurtleSearchRequestId request_id ;	// randomly generated request id.

		std::list<TurtleFileInfo> result ;

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

	protected:
		virtual bool serialize(void *data,uint32_t& size) ;
		virtual uint32_t serial_size() ;
};

class RsTurtleSearchRequestItem: public RsTurtleItem
{
	public:
		RsTurtleSearchRequestItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_SEARCH_REQUEST) {}
		RsTurtleSearchRequestItem(void *data,uint32_t size) ;		// deserialization

		std::string match_string ;	// string to match
		uint32_t request_id ; 		// randomly generated request id.
		uint16_t depth ;				// Used for limiting search depth.

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleOpenTunnelItem: public RsTurtleItem
{
	public:
		RsTurtleOpenTunnelItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_OPEN_TUNNEL) {}
		RsTurtleOpenTunnelItem(void *data,uint32_t size) ;		// deserialization

		TurtleFileHash file_hash ;	  // hash to match
		uint32_t request_id ;		  // randomly generated request id.
		uint32_t partial_tunnel_id ; // uncomplete tunnel id. Will be completed at destination.
		uint16_t depth ;				  // Used for limiting search depth.

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleTunnelOkItem: public RsTurtleItem
{
	public:
		RsTurtleTunnelOkItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_TUNNEL_OK) {}
		RsTurtleTunnelOkItem(void *data,uint32_t size) ;		// deserialization

		uint32_t tunnel_id ;		// id of the tunnel. Should be identical for a tunnel between two same peers for the same hash.
		uint32_t request_id ;	// randomly generated request id corresponding to the intial request.

		virtual std::ostream& print(std::ostream& o, uint16_t) ;

	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleCloseTunnelItem: public RsTurtleItem
{
	public:
		RsTurtleCloseTunnelItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_CLOSE_TUNNEL) {}
		RsTurtleCloseTunnelItem(void *data,uint32_t size) ;		// deserialization

		uint32_t tunnel_id ;		// id of the tunnel to close.

		virtual std::ostream& print(std::ostream& o, uint16_t) ;
	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};

class RsTurtleTunnelClosedItem: public RsTurtleItem
{
	public:
		RsTurtleTunnelClosedItem() : RsTurtleItem(RS_TURTLE_SUBTYPE_TUNNEL_CLOSED) {}
		RsTurtleTunnelClosedItem(void *data,uint32_t size) ;		// deserialization

		uint32_t tunnel_id ;		// id of the tunnel to close.

		virtual std::ostream& print(std::ostream& o, uint16_t) ;
	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual uint32_t serial_size() ; 
};


// Class responsible for serializing/deserializing all turtle items.
//
class RsTurtleSerialiser: public RsSerialType
{
	public:
		RsTurtleSerialiser() : RsSerialType(RS_PKT_VERSION_SERVICE, RS_SERVICE_TYPE_TURTLE) {}

		virtual uint32_t 	size (RsItem *item) 
		{ 
			return static_cast<RsTurtleItem *>(item)->serial_size() ; 
		}
		virtual bool serialise(RsItem *item, void *data, uint32_t *size) 
		{ 
			return static_cast<RsTurtleItem *>(item)->serialize(data,*size) ; 
		}
		virtual RsItem *deserialise (void *data, uint32_t *size) ;
};

// This class is used to keep trace of requests (searches and tunnels).
//
class TurtleRequestInfo
{
	public:
		TurtlePeerId origin ;			// where the request came from.
		uint32_t	time_stamp ;			// last time the tunnel was actually used. Used for cleaning old tunnels.
};

class TurtleTunnel
{
	public:
		TurtlePeerId local_src ;		// where packets come from. Direction to the source.
		TurtlePeerId local_dst ;		// where packets should go. Direction to the destination.
		uint32_t	time_stamp ;			// last time the tunnel was actually used. Used for cleaning old tunnels.
};

// This class keeps trace of the activity for the file hashes the turtle router is asked to monitor.
//
class TurtleFileHashInfo
{
	public:
		std::list<TurtleTunnelId> tunnels ;			// list of active tunnel ids for this file hash
		TurtleRequestId last_request ;				// last request for the tunnels of this hash
};

class p3turtle: public p3Service, public pqiMonitor, public RsTurtle
{
	public:
		p3turtle(p3ConnectMgr *cm);

		// Lauches a search request through the pipes, and immediately returns
		// the request id, which will be further used by the gui to store results
		// as they come back.
		//
		virtual TurtleSearchRequestId turtleSearch(const std::string& string_to_match) ;

		// Initiates tunnel handling for the given file hash.
		// tunnels.  Launches an exception if an error occurs during the
		// initialization process. The turtle router itself does not initiate downloads, 
		// it only maintains tunnels for the given hash. The download should be 
		// driven by the file transfer module. Maybe this function can do the whole thing:
		//  - initiate tunnel handling
		//  - send the file request to the file transfer module
		//  - populate the file transfer module with the adequate pqi interface and search module.
		//
		virtual void turtleDownload(const std::string& file_hash) ;

		/************* from pqiMonitor *******************/
		// Informs the turtle router that some peers are (dis)connected. This should initiate digging new tunnels,
		// and closing other tunnels.
		//
		virtual void statusChange(const std::list<pqipeer> &plist);

		/************* from pqiMonitor *******************/

		// This function does many things:
		// 	- It handles incoming and outgoing packets
		// 	- it sorts search requests and forwards search results upward.
		// 	- it cleans unused (tunnel+search) requests.
		// 	- it maintains the pool of tunnels, for each request file hash.
		//
		virtual int tick();

	private:
		//--------------------------- Admin/Helper functions -------------------------//
		
		uint32_t generatePersonalFilePrint(const TurtleFileHash&) ;	/// Generates a cyphered combination of ownId() and file hash
		uint32_t generateRandomRequestId() ;								/// Generates a random uint32_t number.

		void autoWash() ;															/// Auto cleaning of unused tunnels, search requests and tunnel requests.

		//------------------------------ Tunnel handling -----------------------------//

		TurtleRequestId diggTunnel(const TurtleFileHash& hash) ;	/// initiates tunnels from here to any peers having the given file hash

		//----------------------------- Routing functions ----------------------------//
		
		void manageTunnels() ;					/// Handle tunnel digging for current file hashes
		int handleIncoming(); 					/// Main routing function

		void handleSearchRequest(RsTurtleSearchRequestItem *item);		/// specific routing functions for handling particular packets.
		void handleSearchResult(RsTurtleSearchResultItem *item);
		void handleTunnelRequest(RsTurtleOpenTunnelItem *item);		
		void handleTunnelResult(RsTurtleTunnelOkItem *item);		

		//------ Functions connecting the turtle router to other components.----------//
		
		// Performs a search calling local cache and search structure.
		void performLocalSearch(const std::string& match_string,std::list<TurtleFileInfo>& result) ;

		// Returns a search result upwards (possibly to the gui)
		void returnSearchResult(RsTurtleSearchResultItem *item) ;

		// Returns true if the file with given hash is hosted locally.
		bool performLocalHashSearch(const TurtleFileHash& hash) ;

		//--------------------------- Local variables --------------------------------//
		
		/* data */
		p3ConnectMgr *mConnMgr;

		RsMutex mTurtleMtx;

		std::map<TurtleSearchRequestId,TurtleRequestInfo> 	_search_requests_origins ; /// keeps trace of who emmitted a given search request
		std::map<TurtleTunnelRequestId,TurtleRequestInfo> 	_tunnel_requests_origins ; /// keeps trace of who emmitted a tunnel request

		std::map<TurtleFileHash,TurtleFileHashInfo>			_file_hashes_tunnels ;		/// stores adequate tunnels for each file hash locally asked

		std::map<TurtleTunnelId,TurtleTunnel > 				_local_tunnels ;				/// local tunnels, stored by ids (Either transiting or ending).

		time_t _last_clean_time ;
		time_t _last_tunnel_management_time ;

#ifdef P3TURTLE_DEBUG
		void dumpState() ;
#endif
};

#endif 

