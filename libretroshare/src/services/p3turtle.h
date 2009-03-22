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

		TurtleRequestId request_id ;	// randomly generated request id.

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

class TurtleTunnel
{
	public:
		TurtlePeerId in ;			// where packets come from
		TurtlePeerId out ;		// where packets should go
		uint32_t	time_stamp ;	// last time the tunnel was actually used. Used for cleaning old tunnels.
};

class p3turtle: public p3Service, public pqiMonitor, public RsTurtle
{
	public:
		p3turtle(p3ConnectMgr *cm);

		// Lauches a search request through the pipes, and immediately returns
		// the request id, which will be further used by the gui to store results
		// as they come back.
		//
		virtual TurtleRequestId turtleSearch(const std::string& string_to_match) ;

		// Launches a complete download file operation: diggs one or more
		// tunnels.  Launches an exception if an error occurs during the
		// initialization process.
		//
		virtual void turtleDownload(const std::string& file_hash) ;

		/************* from pqiMonitor *******************/
		// Informs the turtle router that some peers are (dis)connected. This should initiate digging new tunnels,
		// and closing other tunnels.
		//
		virtual void statusChange(const std::list<pqipeer> &plist);

		/************* from pqiMonitor *******************/

		// Handles incoming and outgoing packets, sort search requests and
		// forward info upward.
		virtual int tick();

	private:
		uint32_t generateRandomRequestId() ;
		void autoWash() ;

		/* Network Input */
		int handleIncoming();
//		int handleOutgoing();

		// Performs a search calling local cache and search structure.
		void performLocalSearch(const std::string& s,std::list<TurtleFileInfo>& result) ;

		void handleSearchRequest(RsTurtleSearchRequestItem *item);
		void handleSearchResult(RsTurtleSearchResultItem *item);

		// returns a search result upwards (possibly to the gui)
		void returnSearchResult(RsTurtleSearchResultItem *item) ;

		/* data */
		p3ConnectMgr *mConnMgr;

		RsMutex mTurtleMtx;

		std::map<TurtleRequestId,TurtlePeerId> requests_origins ; 	// keeps trace of who emmitted a given request
		std::map<TurtleFileHash,TurtleTunnel> 	file_tunnels ; 		// stores adequate tunnels for each file hash.

		time_t _last_clean_time ;
};

#endif 

