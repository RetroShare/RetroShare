/*
 * libretroshare/src/services: p3turtle.h
 *
 * Services for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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
 * Please report all bugs and problems to "retroshare@lunamutt.com".
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

// system specific network headers
#include "pqi/pqinetwork.h"
#include "pqi/pqi.h"
#include "pqi/pqimonitor.h"
#include "serialiser/rsdiscitems.h"
#include "services/p3service.h"

class p3AuthMgr;
class p3ConnectMgr;

typedef uint32_t 		TurtleRequestId ;
typedef std::string 	TurtlePeerId ;
typedef std::string 	TurtleFileHash ;
typedef std::string 	TurtleFileName ;

class RsTurtleItem: public RsItem
{
	public:
		virtual bool serialize(void *data,uint32_t& size) = 0 ;	// isn't it better that items can (de)serialize themselves ?
		virtual RsItem *deserialize(const void *data,uint32_t size) = 0 ;
		virtual uint32_t serial_size() const = 0 ; 

		virtual void print() const = 0 ;
};

class RsTurtleSearchResultItem: public RsTurtleItem
{
	public:
		uint16_t depth ;
		uint8_t peer_id[16];				// peer id. This will eventually be obfuscated in some way.

		TurtleRequestId request_id ;	// randomly generated request id.

		std::map<TurtleFileHash,TurtleFileName> result ;
		virtual void print() const ;

	protected:
		virtual bool serialize(void *data,uint32_t& size) ;
		virtual RsItem *deserialize(const void *data,uint32_t size) ;
		virtual uint32_t serial_size() const ;
};

class RsTurtleSearchRequestItem: public RsTurtleItem
{
	public:
		std::string match_string ;	// string to match
		uint32_t request_id ; 		// randomly generated request id.
		uint16_t depth ;				// Used for limiting search depth.

		virtual void print() const = 0 ;

	protected:
		virtual bool serialize(void *data,uint32_t& size) ;	
		virtual RsItem *deserialize(const void *data,uint32_t size) ;
		virtual uint32_t serial_size() const ; 
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
		virtual RsItem *	deserialise (void *data, uint32_t *size) ;
};

class TurtleTunnel
{
	public:
		TurtlePeerId in ;			// where packets come from
		TurtlePeerId out ;		// where packets should go
		uint32_t	time_stamp ;	// last time the tunnel was actually used. Used for cleaning old tunnels.
};

class p3turtle: public p3Service, public pqiMonitor
{
	public:
		p3turtle(p3AuthMgr *am, p3ConnectMgr *cm);

		// Lauches a search request through the pipes, and immediately returns
		// the request id, which will be further used by the gui to store results
		// as they come back.
		//
		TurtleRequestId performSearch(const std::string& string_to_match) ;

		/************* from pqiMonitor *******************/
		// Informs the turtle router that some peers are (dis)connected. This should initiate digging new tunnels,
		// and closing other tunnels.
		//
		virtual void statusChange(const std::list<pqipeer> &plist);

		/************* from pqiMonitor *******************/

		// Handles incoming and outgoing packets, sort search requests and
		// forward info upward.
		int	tick();

	private:
		inline uint32_t generateRandomRequestId() const { return lrand48() ; }
		void autoclean() ;

		/* Network Input */
		int handleIncoming();
		int handleOutgoing();

		void handleSearchRequest(RsTurtleSearchRequestItem *item);

		std::map<TurtleRequestId,TurtlePeerId> request_origins ; // keeps trace of who emmitted a given request
		std::map<TurtleFileHash,TurtleTunnel> 			file_tunnels ; 	// stores adequate tunnels for each file hash.

		p3AuthMgr *mAuthMgr;
		p3ConnectMgr *mConnMgr;

		time_t _last_clean_time ;

		/* data */
		RsMutex mTurtleMtx;
};

#endif 

