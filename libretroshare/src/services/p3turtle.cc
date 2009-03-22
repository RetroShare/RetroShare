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

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsfiles.h"

#include "pqi/p3authmgr.h"
#include "pqi/p3connmgr.h"
#include "pqi/pqinotify.h"

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

#define P3TURTLE_DEBUG 	1

p3turtle::p3turtle(p3ConnectMgr *cm) :p3Service(RS_SERVICE_TYPE_TURTLE), mConnMgr(cm)
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	srand48(time(NULL)) ;
	addSerialType(new RsTurtleSerialiser());
}

int p3turtle::tick()
{
	handleIncoming();		// handle incoming packets
//	handleOutgoing();		// handle outgoing packets

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

	// look for tunnels and stored temportary info that have not been used for a while.
}

uint32_t p3turtle::generateRandomRequestId() 
{
	RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/

	return lrand48() ;
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
		sleep(1) ;
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
	pqiNotify *notify = getPqiNotify();

	if (notify)
		notify->AddSysMessage(0, RS_SYS_WARNING, std::string("Unimplemented"),std::string("turtle download is not yet implemented. Sorry"));
}

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

			// Here will also come handling of file transfer requests, tunnel digging/closing, etc.
			default:
																std::cerr << "p3turtle::handleIncoming: Unknown packet subtype " << item->PacketSubType() << std::endl ;
		}
		delete item;
	}

	return nhandled;
}

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
	if(requests_origins.find(item->request_id) != requests_origins.end())
	{
#ifdef P3TURTLE_DEBUG
		std::cerr << "  This is a bouncing request. Ignoring and deleting it." << std::endl ;
#endif
		return ;
	}

	// This is a new request. Let's add it to the request map, and forward it to 
	// open peers.

	requests_origins[item->request_id] = item->PeerId() ;

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

			if(!result.empty())
			{
				// do something

				// forward item back
				RsTurtleSearchResultItem *res_item = new RsTurtleSearchResultItem ;

				// perhaps we should chop search results items into several items of finite size ?
				res_item->depth = 0 ;
				res_item->result = result ;
				res_item->request_id = item->request_id ;
				res_item->PeerId(item->PeerId()) ;			// send back to the same guy

#ifdef P3TURTLE_DEBUG
				std::cerr << "  " << result.size() << " matches found. Sending back to origin (" << res_item->PeerId() << ")." << std::endl ;
#endif
				sendItem(res_item) ;
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
	std::map<TurtleRequestId,TurtlePeerId>::const_iterator it = requests_origins.find(item->request_id) ;
#ifdef P3TURTLE_DEBUG
	std::cerr << "Received search result:" << std::endl ;
	item->print(std::cerr,0) ;
#endif
	if(it == requests_origins.end())
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

std::ostream& RsTurtleSearchRequestItem::print(std::ostream& o, uint16_t)
{
	o << "Search request:" << std::endl ;
	o << "  direct origin: \"" << PeerId() << "\"" << std::endl ;
	o << "  match string: \"" << match_string << "\"" << std::endl ;
	o << "  Req. Id: " << request_id << std::endl ;
	o << "  Depth  : " << depth << std::endl ;

	return o ;
}

std::ostream& RsTurtleSearchResultItem::print(std::ostream& o, uint16_t)
{
	o << "Search result:" << std::endl ;

	o << "  Peer id: " << PeerId() << std::endl ;
	o << "  Depth  : " << depth << std::endl ;
	o << "  Req. Id: " << request_id << std::endl ;
	o << "  Files:" << std::endl ;
	
	for(std::list<TurtleFileInfo>::const_iterator it(result.begin());it!=result.end();++it)
		o << "    " << it->hash << "  " << it->size << " " << it->name << std::endl ;

	return o ;
}

void p3turtle::returnSearchResult(RsTurtleSearchResultItem *item)
{
	// just cout for now, but it should be notified to the gui
	
#ifdef P3TURTLE_DEBUG
	std::cerr << "  Returning result for search request " << item->request_id << " upwards." << std::endl ;
#endif

	rsicontrol->getNotify().notifyTurtleSearchResult(item->request_id,item->result) ;
}

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

#include "serialiser/rstlvbase.h"
#include "serialiser/rsbaseserial.h"

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

	for(uint i=0;i<s;++i)
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


