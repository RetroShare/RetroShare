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

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsfiles.h"

#include "pqi/p3authmgr.h"
#include "pqi/p3connmgr.h"
#include "pqi/pqinotify.h"

#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"

#include "p3turtle.h"

#include <iostream>
#include <errno.h>
#include <cmath>

#include <sstream>

#include "util/rsdebug.h"
#include "util/rsprint.h"

// Operating System specific includes.
#include "pqi/pqinetwork.h"

/* TURTLE FLAGS */

p3turtle::p3turtle(p3ConnectMgr *cm) :p3Service(RS_SERVICE_TYPE_TURTLE), mConnMgr(cm)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	srand(time(NULL)) ;
	addSerialType(new RsTurtleSerialiser());
}

int p3turtle::tick()
{
	handleIncoming();		// handle incoming packets

	// Clean every 10 sec.
	time_t now = time(NULL) ;

	if(now > 10+_last_clean_time)
	{
		autoWash() ;			// clean old/unused tunnels and search requests.
		_last_clean_time = now ;
	}

	return 0 ;
}

void p3turtle::autoWash()
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

#ifdef P3TURTLE_DEBUG
	std::cerr << "Calling autowash." << std::endl ;
#endif

	// look for tunnels and stored temportary info that have not been used for a while.
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
uint32_t p3turtle::generatePersonalFilePrint(const TurtleFileHash& hash) 
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	// whatever cooking from the file hash and OwnId that cannot be recovered.
	// The only important thing is that the saem hash produces the same tunnel
	// id.

	std::string buff(hash + mConnMgr->getOwnId()) ;
	uint32_t res = 0 ;
	uint32_t decal = 0 ;

	for(uint i=0;i<buff.length();++i)
	{
		res += 7*buff[i] + decal ;
		decal = decal*44497+15641+(res%86243) ;
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
			case RS_TURTLE_SUBTYPE_SEARCH_REQUEST: handleSearchRequest(dynamic_cast<RsTurtleSearchRequestItem *>(item)) ;
																break ;

			case RS_TURTLE_SUBTYPE_SEARCH_RESULT : handleSearchResult(dynamic_cast<RsTurtleSearchResultItem *>(item)) ;
																break ;

			case RS_TURTLE_SUBTYPE_OPEN_TUNNEL   : handleTunnelRequest(dynamic_cast<RsTurtleOpenTunnelItem *>(item)) ;
																break ;

			case RS_TURTLE_SUBTYPE_TUNNEL_OK     : handleTunnelResult(dynamic_cast<RsTurtleTunnelOkItem *>(item)) ;
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

	_search_requests_origins[item->request_id] = item->PeerId() ;

	// If it's not for us, perform a local search. If something found, forward the search result back.
	
	if(item->PeerId() != mConnMgr->getOwnId())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Request not from us. Performing local search" << std::endl ;
#endif
		if(_sharing_strategy != SHARE_FRIENDS_ONLY || item->depth < 2)
		{
			std::list<TurtleFileInfo> result ;
			performLocalSearch(item->match_string,result) ;

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
				RsTurtleSearchRequestItem *fwd_item = new RsTurtleSearchRequestItem(*item) ;

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
	std::map<TurtleRequestId,TurtlePeerId>::const_iterator it = _search_requests_origins.find(item->request_id) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "Received search result:" << std::endl ;
	item->print(std::cerr,0) ;
#endif
	if(it == _search_requests_origins.end())
	{
		// This is an error: how could we receive a search result corresponding to a search item we 
		// have forwarded but that it not in the list ??

		std::cerr << __PRETTY_FUNCTION__ << ": search result has no peer direction!" << std::endl ;
		delete item ;
		return ;
	}

	// Is this result's target actually ours ?
	
	++(item->depth) ;			// increase depth

	if(it->second == mConnMgr->getOwnId())
		returnSearchResult(item) ;		// Yes, so send upward.
	else
	{											// Nope, so forward it back.
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Forwarding result back to " << it->second << std::endl;
#endif
		RsTurtleSearchResultItem *fwd_item = new RsTurtleSearchResultItem(*item) ;	// copy the item

		// normally here, we should setup the forward adress, so that the owner's of the files found can be further reached by a tunnel.

		fwd_item->PeerId(it->second) ;

		sendItem(fwd_item) ;
	}
}

// -----------------------------------------------------------------------------------//
// -------------------------------------  IO  --------------------------------------- // 
// -----------------------------------------------------------------------------------//
//
std::ostream& RsTurtleSearchRequestItem::print(std::ostream& o, uint16_t)
{
	o << "Search request:" << std::endl ;
	o << "  direct origin: \"" << PeerId() << "\"" << std::endl ;
	o << "  match string: \"" << match_string << "\"" << std::endl ;
	o << "  Req. Id: " << (void *)request_id << std::endl ;
	o << "  Depth  : " << depth << std::endl ;

	return o ;
}

std::ostream& RsTurtleSearchResultItem::print(std::ostream& o, uint16_t)
{
	o << "Search result:" << std::endl ;

	o << "  Peer id: " << PeerId() << std::endl ;
	o << "  Depth  : " << depth << std::endl ;
	o << "  Req. Id: " << (void *)request_id << std::endl ;
	o << "  Files:" << std::endl ;
	
	for(std::list<TurtleFileInfo>::const_iterator it(result.begin());it!=result.end();++it)
		o << "    " << it->hash << "  " << it->size << " " << it->name << std::endl ;

	return o ;
}

std::ostream& RsTurtleOpenTunnelItem::print(std::ostream& o, uint16_t)
{
	o << "Open Tunnel:" << std::endl ;

	o << "  Peer id    : " << PeerId() << std::endl ;
	o << "  Partial tId: " << (void *)partial_tunnel_id << std::endl ;
	o << "  Req. Id    : " << (void *)request_id << std::endl ;
	o << "  Depth      : " << depth << std::endl ;
	o << "  Hash       : " << file_hash << std::endl ;

	return o ;
}

std::ostream& RsTurtleTunnelOkItem::print(std::ostream& o, uint16_t)
{
	o << "Tunnel Ok:" << std::endl ;

	o << "  Peer id   : " << PeerId() << std::endl ;
	o << "  tunnel id : " << (void*)tunnel_id << std::endl ;
	o << "  Req. Id   : " << (void *)request_id << std::endl ;

	return o ;
}
// -----------------------------------------------------------------------------------//
// --------------------------------  Search handling. ------------------------------- // 
// -----------------------------------------------------------------------------------//
//

void p3turtle::diggTunnel(const TurtleFileHash& hash)
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

	// Form a tunnel request packet that simulates a request from us.
	//
	RsTurtleOpenTunnelItem *item = new RsTurtleOpenTunnelItem ;

	item->PeerId(mConnMgr->getOwnId()) ;
	item->file_hash = hash ;
	item->request_id = id ;
	item->partial_tunnel_id = generatePersonalFilePrint(hash) ;
	item->depth = 0 ;
	
	// send it 
	
	handleTunnelRequest(item) ;

	delete item ;
}

void p3turtle::handleTunnelRequest(RsTurtleOpenTunnelItem *item)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

#ifdef P3TURTLE_DEBUG
	std::cerr << "Received tunnel request from peer " << item->PeerId() << ": " << std::endl ;
	item->print(std::cerr,0) ;
#endif
	// If the item contains an already handled tunnel request, give up.  This
	// happens when the same tunnel request gets relayed by different peers
	//
	if(_tunnel_requests_origins.find(item->request_id) != _tunnel_requests_origins.end())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  This is a bouncing request. Ignoring and deleting item." << std::endl ;
#endif
		return ;
	}

	// This is a new request. Let's add it to the request map, and forward it to 
	// open peers.

	_tunnel_requests_origins[item->request_id] = item->PeerId() ;

	// If it's not for us, perform a local search. If something found, forward the search result back.
	
	if(item->PeerId() != mConnMgr->getOwnId())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Request not from us. Performing local search" << std::endl ;
#endif
		if((_sharing_strategy != SHARE_FRIENDS_ONLY || item->depth < 2) && performLocalHashSearch(item->file_hash))
		{
#ifdef P3TURTLE_DEBUG
			std::cerr << "  Local hash found. Sending tunnel ok to origin (" << item->PeerId() << ")." << std::endl ;
#endif
			// Send back tunnel ok to the same guy
			//
			RsTurtleTunnelOkItem *res_item = new RsTurtleTunnelOkItem ;

			res_item->request_id = item->request_id ;
			res_item->tunnel_id = item->partial_tunnel_id ^ generatePersonalFilePrint(item->file_hash) ;
			res_item->PeerId(item->PeerId()) ;			

			sendItem(res_item) ;

			// Note in the tunnels list that we have an ending tunnel here.
			TurtleTunnel tt ;
			tt.local_src = item->PeerId() ;
			tt.local_dst = mConnMgr->getOwnId() ;	// this means us
			tt.time_stamp = time(NULL) ;

			_local_tunnels[res_item->tunnel_id] = tt ;

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
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	// Find who actually sent the corresponding turtle tunnel request.
	//
	std::map<TurtleTunnelRequestId,TurtlePeerId>::const_iterator it = _tunnel_requests_origins.find(item->request_id) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "Received tunnel result:" << std::endl ;
	item->print(std::cerr,0) ;
#endif
	if(it == _tunnel_requests_origins.end())
	{
		// This is an error: how could we receive a tunnel result corresponding to a tunnel item we 
		// have forwarded but that it not in the list ??

		std::cerr << __PRETTY_FUNCTION__ << ": tunnel result has no peer direction!" << std::endl ;
		delete item ;
		return ;
	}

	// store tunnel info.
	if(_local_tunnels.find(item->tunnel_id) != _local_tunnels.end())
		std::cerr << "Tunnel already there. This is an error !!" << std::endl ;
	
	TurtleTunnel& tunnel(_local_tunnels[item->tunnel_id]) ;

	tunnel.local_src = it->second ;
	tunnel.local_dst = item->PeerId() ;
	tunnel.time_stamp = time(NULL) ;

#ifdef P3TURTLE_DEBUG
	std::cerr << "  storing tunnel info. src=" << tunnel.local_src << ", dst=" << tunnel.local_dst << ", id=" << item->tunnel_id << std::endl ;
#endif

	// Is this result's target actually ours ?

	if(it->second == mConnMgr->getOwnId())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Tunnel starting point. Storing id=" << item->tunnel_id << " for hash (unknown) and tunnel request id " << it->second << std::endl;
#endif
		// Tunnel is ending here. Add it to the list of tunnels for the given hash.

		// a connexion between the file hash and the tunnel id is missing at this point.
		//_file_hashes_tunnels.insert(item->tunnel_id) ;
	}
	else
	{											// Nope, forward it back.
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Forwarding result back to " << it->second << std::endl;
#endif
		RsTurtleTunnelOkItem *fwd_item = new RsTurtleTunnelOkItem(*item) ;	// copy the item
		fwd_item->PeerId(it->second) ;

		sendItem(fwd_item) ;
	}
}
// -----------------------------------------------------------------------------------//
// ------------------------------  Tunnel maintenance. ------------------------------ // 
// -----------------------------------------------------------------------------------//
//

/************* from pqiMonitor *******************/
void p3turtle::statusChange(const std::list<pqipeer> &plist)
{
	// Do we shutdown tunnels whne peers are down, or automatically find a new tunnel ?
	// I'll see that later...
#ifdef TO_DO
	/* get a list of all online peers */
	std::list<std::string> onlineIds;
	mConnMgr->getOnlineList(onlineIds);

	std::list<pqipeer>::const_iterator pit;
	/* if any have switched to 'connected' then we notify */
	for(pit =  plist.begin(); pit != plist.end(); pit++)
	{
		if ((pit->state & RS_PEER_S_FRIEND) &&
			(pit->actions & RS_PEER_CONNECTED))
		{
			/* send our details to them */
			sendOwnDetails(pit->id);
		}
	}
#endif
}
// -----------------------------------------------------------------------------------//
// ------------------------------  IO with libretroshare  ----------------------------// 
// -----------------------------------------------------------------------------------//
//
void p3turtle::performLocalSearch(const std::string& s,std::list<TurtleFileInfo>& result) 
{
	/* call to core */
	std::list<FileDetail> initialResults;
	std::list<std::string> words ;

	// to do: split search string into words.
	words.push_back(s) ;
	
	// now, search!
	rsFiles->SearchKeywords(words, initialResults,DIR_FLAGS_LOCAL);

	result.clear() ;

	for(std::list<FileDetail>::const_iterator it(initialResults.begin());it!=initialResults.end();++it)
	{
		TurtleFileInfo i ;
		i.hash = it->hash ;
		i.size = it->size ;
		i.name = it->name ;

		result.push_back(i) ;
	}
}

TurtleRequestId p3turtle::turtleSearch(const std::string& string_to_match) 
{
	// generate a new search id.
	
	TurtleRequestId id = generateRandomRequestId() ;

	// Form a request packet that simulates a request from us.
	//
	RsTurtleSearchRequestItem *item = new RsTurtleSearchRequestItem ;

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


void p3turtle::turtleDownload(const std::string& file_hash) 
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	if(_file_hashes_tunnels.find(file_hash) != _file_hashes_tunnels.end())	// download already asked.
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "p3turtle: File hash " << file_hash << " already in pool. Returning." << std::endl ;
#endif
		return ;
	}

	// No tunnels at start, but this triggers digging new tunnels.
	//
	_file_hashes_tunnels[file_hash] = std::list<TurtleTunnelId>() ;		

	// also should send associated request to the file transfer module.

	// todo!
}

void p3turtle::returnSearchResult(RsTurtleSearchResultItem *item)
{
	// just cout for now, but it should be notified to the gui
	
#ifdef P3TURTLE_DEBUG
	std::cerr << "  Returning result for search request " << item->request_id << " upwards." << std::endl ;
#endif

	rsicontrol->getNotify().notifyTurtleSearchResult(item->request_id,item->result) ;
}

bool p3turtle::performLocalHashSearch(const TurtleFileHash& hash) 
{
	FileInfo info ;

	return rsFiles->FileDetails(hash, RS_FILE_HINTS_LOCAL, info);
}

// -----------------------------------------------------------------------------------//
// --------------------------------  Serialization. --------------------------------- // 
// -----------------------------------------------------------------------------------//
//

//
// ---------------------------------- Packet sizes -----------------------------------//
//
uint32_t RsTurtleSearchRequestItem::serial_size() 
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += GetTlvStringSize(match_string) ;
	s += 4 ; // request_id
	s += 2 ; // depth

	return s ;
}

uint32_t RsTurtleSearchResultItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // search request id
	s += 2 ; // depth
	s += 4 ; // number of results

	for(std::list<TurtleFileInfo>::const_iterator it(result.begin());it!=result.end();++it)
	{
		s += 8 ;											// file size
		s += GetTlvStringSize(it->hash) ;		// file hash
		s += GetTlvStringSize(it->name) ;		// file name
	}

	return s ;
}

uint32_t RsTurtleOpenTunnelItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += GetTlvStringSize(file_hash) ;		// file hash
	s += 4 ; // tunnel request id
	s += 4 ; // partial tunnel id 
	s += 2 ; // depth

	return s ;
}

uint32_t RsTurtleTunnelOkItem::serial_size()
{
	uint32_t s = 0 ;

	s += 8 ; // header
	s += 4 ; // tunnel id 
	s += 4 ; // tunnel request id

	return s ;
}
//
// ---------------------------------- Serialization ----------------------------------//
//
RsItem *RsTurtleSerialiser::deserialise(void *data, uint32_t *size) 
{
	// look what we have...
	
	/* get the type */
	uint32_t rstype = getRsItemId(data);
#ifdef P3TURTLE_DEBUG
	std::cerr << "p3turtle: deserialising packet: " << std::endl ;
#endif
	if ((RS_PKT_VERSION_SERVICE != getRsItemVersion(rstype)) || (RS_SERVICE_TYPE_TURTLE != getRsItemService(rstype))) 
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  Wrong type !!" << std::endl ;
#endif
		return NULL; /* wrong type */
	}

	try
	{
		switch(getRsItemSubType(rstype))
		{
			case RS_TURTLE_SUBTYPE_SEARCH_REQUEST:	return new RsTurtleSearchRequestItem(data,*size) ;
			case RS_TURTLE_SUBTYPE_SEARCH_RESULT:	return new RsTurtleSearchResultItem(data,*size) ;

			default:
																std::cerr << "Unknown packet type in RsTurtle!" << std::endl ;
																return NULL ;
		}
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception raised: " << e.what() << std::endl ;
		return NULL ;
	}
}

bool RsTurtleSearchRequestItem::serialize(void *data,uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_VALUE, match_string);
	ok &= setRawUInt32(data, tlvsize, &offset, request_id);
	ok &= setRawUInt16(data, tlvsize, &offset, depth);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsTurtleSearchRequestItem::RsTurtleSearchRequestItem(void *data,uint32_t pktsize)
	: RsTurtleItem(RS_TURTLE_SUBTYPE_SEARCH_REQUEST)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = search request" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);
	bool ok = true ;

	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_VALUE, match_string); 	// file hash
	ok &= getRawUInt32(data, pktsize, &offset, &request_id);
	ok &= getRawUInt16(data, pktsize, &offset, &depth);

	if (offset != rssize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
}

bool RsTurtleSearchResultItem::serialize(void *data,uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= setRawUInt32(data, tlvsize, &offset, request_id);
	ok &= setRawUInt16(data, tlvsize, &offset, depth);
	ok &= setRawUInt32(data, tlvsize, &offset, result.size());

	for(std::list<TurtleFileInfo>::const_iterator it(result.begin());it!=result.end();++it)
	{
		ok &= setRawUInt64(data, tlvsize, &offset, it->size); 								// file size
		ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_HASH_SHA1, it->hash);	// file hash
		ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_NAME, it->name); 		// file name
	}

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsFileConfigSerialiser::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsTurtleSearchResultItem::RsTurtleSearchResultItem(void *data,uint32_t pktsize)
	: RsTurtleItem(RS_TURTLE_SUBTYPE_SEARCH_RESULT)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = search result" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);

	/* add mandatory parts first */

	bool ok = true ;
	uint32_t s ;
	ok &= getRawUInt32(data, pktsize, &offset, &request_id);
	ok &= getRawUInt16(data, pktsize, &offset, &depth);
	ok &= getRawUInt32(data, pktsize, &offset, &s) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "  reuqest_id=" << request_id << ", depth=" << depth << ", s=" << s << std::endl ;
#endif

	result.clear() ;

	for(int i=0;i<(int)s;++i)
	{
		TurtleFileInfo f ;

		ok &= getRawUInt64(data, pktsize, &offset, &(f.size)); 									// file size
		ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_HASH_SHA1, f.hash); 	// file hash
		ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_NAME, f.name); 			// file name

		result.push_back(f) ;
	}

	if (offset != rssize)
		throw std::runtime_error("Size error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("Unknown error while deserializing.") ;
}

bool RsTurtleOpenTunnelItem::serialize(void *data,uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= SetTlvString(data, tlvsize, &offset, TLV_TYPE_STR_HASH_SHA1, file_hash);	// file hash
	ok &= setRawUInt32(data, tlvsize, &offset, request_id);
	ok &= setRawUInt32(data, tlvsize, &offset, partial_tunnel_id);
	ok &= setRawUInt16(data, tlvsize, &offset, depth);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsTurtleOpenTunnelItem::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsTurtleOpenTunnelItem::RsTurtleOpenTunnelItem(void *data,uint32_t pktsize)
	: RsTurtleItem(RS_TURTLE_SUBTYPE_OPEN_TUNNEL)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = open tunnel" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);

	/* add mandatory parts first */

	bool ok = true ;
	ok &= GetTlvString(data, pktsize, &offset, TLV_TYPE_STR_HASH_SHA1, file_hash); 	// file hash
	ok &= getRawUInt32(data, pktsize, &offset, &request_id);
	ok &= getRawUInt32(data, pktsize, &offset, &partial_tunnel_id) ;
	ok &= getRawUInt16(data, pktsize, &offset, &depth);
#ifdef P3TURTLE_DEBUG
	std::cerr << "  reuqest_id=" << (void*)request_id << ", partial_id=" << (void*)partial_tunnel_id << ", depth=" << depth << ", hash=" << file_hash << std::endl ;
#endif

	if (offset != rssize)
		throw std::runtime_error("RsTurtleOpenTunnelItem::() error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("RsTurtleOpenTunnelItem::() unknown error while deserializing.") ;
}

bool RsTurtleTunnelOkItem::serialize(void *data,uint32_t& pktsize)
{
	uint32_t tlvsize = serial_size();
	uint32_t offset = 0;

	if (pktsize < tlvsize)
		return false; /* not enough space */

	pktsize = tlvsize;

	bool ok = true;

	ok &= setRsItemHeader(data,tlvsize,PacketId(), tlvsize);

	/* skip the header */
	offset += 8;

	/* add mandatory parts first */

	ok &= setRawUInt32(data, tlvsize, &offset, tunnel_id);
	ok &= setRawUInt32(data, tlvsize, &offset, request_id);

	if (offset != tlvsize)
	{
		ok = false;
#ifdef RSSERIAL_DEBUG
		std::cerr << "RsTurtleTunnelOkItem::serialiseTransfer() Size Error! " << std::endl;
#endif
	}

	return ok;
}

RsTurtleTunnelOkItem::RsTurtleTunnelOkItem(void *data,uint32_t pktsize)
	: RsTurtleItem(RS_TURTLE_SUBTYPE_TUNNEL_OK)
{
#ifdef P3TURTLE_DEBUG
	std::cerr << "  type = tunnel ok" << std::endl ;
#endif
	uint32_t offset = 8; // skip the header 
	uint32_t rssize = getRsItemSize(data);

	/* add mandatory parts first */

	bool ok = true ;
	ok &= getRawUInt32(data, pktsize, &offset, &tunnel_id) ;
	ok &= getRawUInt32(data, pktsize, &offset, &request_id);
#ifdef P3TURTLE_DEBUG
	std::cerr << "  reuqest_id=" << (void*)request_id << ", tunnel_id=" << (void*)tunnel_id << std::endl ;
#endif

	if (offset != rssize)
		throw std::runtime_error("RsTurtleTunnelOkItem::() error while deserializing.") ;
	if (!ok)
		throw std::runtime_error("RsTurtleTunnelOkItem::() unknown error while deserializing.") ;
}


