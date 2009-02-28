/*
 * libretroshare/src/services: p3disc.cc
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


#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "services/p3disc.h"

#include "pqi/p3authmgr.h"
#include "pqi/p3connmgr.h"

#include <iostream>
#include <errno.h>
#include <cmath>

const uint8_t RS_TURTLE_SUBTYPE_SEARCH_REQUEST = 0x01;
const uint8_t RS_TURTLE_SUBTYPE_SEARCH_RESULT  = 0x02;

#include <sstream>

#include "util/rsdebug.h"
#include "util/rsprint.h"

// Operating System specific includes.
#include "pqi/pqinetwork.h"

/* DISC FLAGS */

/*****
 * #define P3DISC_DEBUG 	1
 ****/

/*********** NOTE ***************
 *
 * Only need Mutexs for neighbours information
 */

/******************************************************************************************
 ******************************  NEW DISCOVERY  *******************************************
 ******************************************************************************************
 *****************************************************************************************/

p3turtle::p3turtle(p3AuthMgr *am, p3ConnectMgr *cm) :p3Service(RS_SERVICE_TYPE_TURTLE), mAuthMgr(am), mConnMgr(cm)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	addSerialType(new RsTurtleSerialiser());
}

int p3turtle::tick()
{
	handleIncoming();		// handle incoming packets
	handleOutgoing();		// handle outgoing packets

	autoclean() ;			// clean old/unused tunnels and search requests.
}

int p3turtle::handleIncoming()
{
#ifdef DEBUG_TURTLE
	std::cerr << "p3turtle::handleIncoming()";
	std::cerr << std::endl;
#endif

	{
		RsStackMutex stack(mTurtleMtx); /********** STACK LOCKED MTX ******/
	}

	int nhandled = 0;
	// While messages read
	//
	RsItem *item = NULL;

	while(NULL != (item = recvItem()))
	{
		nhandled++;

		switch(item->subType())
		{
			case RS_TURTLE_SUBTYPE_SEARCH_REQUEST: handleSearchRequest(dynamic_cast<RsTurtleSearchRequest *>(item)) ;
																break ;

			case RS_TURTLE_SUBTYPE_SEARCH_RESULT : handleSearchResult(dynamic_cast<RsTurtleSearchResult *>(item)) ;
																break ;

			// Here will also come handling of file transfer requests, tunnel digging/closing, etc.
			default:
																std::cerr << "p3turtle::handleIncoming: Unknown packet subtype " << item->subType() << std::endl ;
		}
		delete item;
	}

	return nhandled;
}

void p3turtle::handleSearchRequest(const RsSearchRequestItem *item)
{
	// take a look at the item: 
	// 	- If the item destimation is 

	// If the item contains an already handled search request, give up.  This
	// happens when the same search request gets relayed by different peers
	//
	if(requests_origins.find(item->request_id) != requests_origins.end())
		return ;

	// This is a new request. Let's add it to the request map, and forward it to 
	// open peers.

	requests_origins[item->request_id] = item->peerId() ;

	// Perform local search. If something found, forward the search result back.
	
	std::map<FileHash,FileName> result ;
	performLocalSearch(item->match_string,result) ;

	if(!result.empty())
	{
		// do something

		// forward item back
		RsTurtleSearchResultItem *res_item = new RsTurtleSearchResultItem ;

		res_item->depth = 0 ;
		res_item->result = result ;
		res_item->request_id = item->request_id ;
		res_item->peer_id = item->peer_id ;			// send back to the same guy

		sendItem(res_item) ;
	}

	// If search depth not too large, also forward this search request to all other peers.
	//
	if(item->depth < TURTLE_MAX_SEARCH_DEPTH)
		for(std::map<>::const_iterator it(openned_peers.begin());it!=openned_peers.end();++it)
			if(*it != item->peerId())
			{
				// Copy current item and modify it.
				RsTurtleSearchRequestItem *fwd_item = new RsTurtleSearchRequestItem(item) ;

				++(fwd_item->depth) ;		// increase search depth
				fwd_item->peerId = *it ;

				sendItem(fwd_item) ;
			}
}

void p3turtle::handleSearchResult(const RsSearchResultItem *item)
{
	// Find who actually sent the corresponding request.
	//
	std::map<TurtleRequestId,TurtlePeerId>::const_iterator it = requests_origins.find(item->request_id) ;
	
	if(it == requests_origins.end())
	{
		// This is an error: how could we receive a search result corresponding to a search item we 
		// have forwarded but that it not in the list ??

		std::cerr << __PRETTY_FUNCTION__ << ": search result has no peer direction!" << std::endl ;
		delete item ;
		return ;
	}

	// Is this result's target actually ours ?
	
	if(it->second == own_peer_id)
		returnSearchResult(item) ;		// Yes, so send upward.
	else
	{											// Nope, so forward it back.
		RsSearchResultItem *fwd_item = new RsSearchResultItem(item) ;	// copy the item

		++(fwd_item->depth) ;			// increase depth

		// normally here, we should setup the forward adress, so that the owner's of the files found can be further reached by a tunnel.

		fwd_item->peerId = it->second ;

		sendItem(fwd_item) ;
	}
}

/************* from pqiMonitor *******************/
void p3turtle::statusChange(const std::list<pqipeer> &plist)
{
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

#ifdef A_VIRER

void p3disc::respondToPeer(std::string id)
{
	/* get a peer lists */

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::respondToPeer() id: " << id;
	std::cerr << std::endl;
#endif

	std::list<std::string> friendIds;
	std::list<std::string> onlineIds;
	std::list<std::string>::iterator it;

	mConnMgr->getFriendList(friendIds);
	mConnMgr->getOnlineList(onlineIds);

	/* Check that they have DISC on */
	{
		/* get details */
		peerConnectState detail;
		if (!mConnMgr->getFriendNetStatus(id, detail))
		{
			/* major error! */
			return;
		}

		if (detail.visState & RS_VIS_STATE_NODISC)
		{
			/* don't have DISC enabled */
			return;
		}
	}

	/* send them a list of all friend's details */
	for(it = friendIds.begin(); it != friendIds.end(); it++)
	{
		/* get details */
		peerConnectState detail;
		if (!mConnMgr->getFriendNetStatus(*it, detail))
		{
			/* major error! */
			continue;
		}

		if (!(detail.visState & RS_VIS_STATE_NODISC))
		{
			sendPeerDetails(id, *it); /* (dest (to), source (cert)) */
		}
	}

	/* send their details to all online peers */
	for(it = onlineIds.begin(); it != onlineIds.end(); it++)
	{
		peerConnectState detail;
		if (!mConnMgr->getFriendNetStatus(*it, detail))
		{
			/* major error! */
			continue;
		}

		if (!(detail.visState & RS_VIS_STATE_NODISC))
		{
			sendPeerDetails(*it, id); /* (dest (to), source (cert)) */
		}
	}
}

/*************************************************************************************/
/*				Output Network Msgs				     */
/*************************************************************************************/
void p3disc::sendOwnDetails(std::string to)
{
	/* setup:
	 * IP local / external
	 * availability (TCP LOCAL / EXT, UDP ...)
	 */

	// Then send message.
	{
#ifdef P3DISC_DEBUG
	  	  std::ostringstream out;
		  out << "p3disc::sendOwnDetails()";
		  out << "Constructing a RsDiscItem Message!" << std::endl;
		  out << "Sending to: " << to;
		  std::cerr << out.str() << std::endl;
#endif
	}

	// Construct a message
	RsDiscItem *di = new RsDiscItem();

	/* components:
	 * laddr
	 * saddr
	 * contact_tf
	 * discFlags
	 */

	peerConnectState detail;
	if (!mConnMgr->getOwnNetStatus(detail))
	{
		/* major error! */
		return;
	}

	// Fill the message
	di -> PeerId(to);
	di -> laddr = detail.localaddr;
	di -> saddr = detail.serveraddr;
	di -> contact_tf = 0; 

	/* construct disc flags */
	di -> discFlags = 0;
	if (!(detail.visState & RS_VIS_STATE_NODISC))
	{
		di->discFlags |= P3DISC_FLAGS_USE_DISC;
	}

	if (!(detail.visState & RS_VIS_STATE_NODHT))
	{
		di->discFlags |= P3DISC_FLAGS_USE_DHT;
	}

	if ((detail.netMode & RS_NET_MODE_EXT) ||
		(detail.netMode & RS_NET_MODE_UPNP))
	{
		di->discFlags |= P3DISC_FLAGS_EXTERNAL_ADDR;
	}
	else if (detail.netMode & RS_NET_MODE_UDP)
	{
		di->discFlags |= P3DISC_FLAGS_STABLE_UDP;
	}

	di->discFlags |= P3DISC_FLAGS_OWN_DETAILS;

	/* send msg */
	sendItem(di);
}

 /* (dest (to), source (cert)) */
void p3disc::sendPeerDetails(std::string to, std::string about)
{
	/* setup:
	 * Certificate.
	 * IP local / external
	 * availability ...
	 * last connect (0) if online.
	 */

	/* send it off */
	{
#ifdef P3DISC_DEBUG
		std::ostringstream out;
		out << "p3disc::sendPeerDetails()";
		out << " Sending details of: " << about;
		out << " to: " << to << std::endl;
		std::cerr << out.str() << std::endl;
#endif
	}


	peerConnectState detail;
	if (!mConnMgr->getFriendNetStatus(about, detail))
	{
		/* major error! */
		return;
	}

	// Construct a message
	RsDiscReply *di = new RsDiscReply();

	// Fill the message
	// Set Target as input cert.
	di -> PeerId(to);
	di -> aboutId = about;

	// set the server address.
	di -> laddr = detail.localaddr;
	di -> saddr = detail.serveraddr;

	if (detail.state & RS_PEER_S_CONNECTED)
	{
		di -> contact_tf = 0;
	}
	else
	{
		di -> contact_tf = convertTDeltaToTRange(time(NULL) - detail.lastcontact);
	}

	/* construct disc flags */
	di->discFlags = 0;

	/* NOTE we should not be sending packet if NODISC is set....
	 * checked elsewhere... so don't check.
	 */
	di->discFlags |= P3DISC_FLAGS_USE_DISC;

	if (!(detail.visState & RS_VIS_STATE_NODHT))
	{
		di->discFlags |= P3DISC_FLAGS_USE_DHT;
	}

	if (detail.netMode & RS_NET_MODE_EXT)
	{
		di->discFlags |= P3DISC_FLAGS_EXTERNAL_ADDR;
	}
	else if (detail.netMode & RS_NET_MODE_UDP)
	{
		di->discFlags |= P3DISC_FLAGS_STABLE_UDP;
	}

	if (detail.state & RS_PEER_S_CONNECTED)
	{
		di->discFlags |= P3DISC_FLAGS_PEER_ONLINE;
	}

	// Add 3rd party trust info
	// We look at peers that trust 'to', by looking into 'to''s tigners list. The problem is that
	// signers are accessible through their names instead of their id, so there is ambiguity if too peers
	// have the same names. @DrBob: that would be cool to save signers using their ids...
	//
	RsPeerDetails pd ;
	std::string name = rsPeers->getPeerName(about) ;
	if(rsPeers->getPeerDetails(to,pd)) 
		for(std::list<std::string>::const_iterator it(pd.signers.begin());it!=pd.signers.end();++it)
			if(*it == name)
			{
				di->discFlags |= P3DISC_FLAGS_PEER_TRUSTS_ME;
#ifdef P3DISC_DEBUG
				std::cerr << "   Peer " << about << "(" << name << ")" << " is trusting " << to << ", sending info." << std::endl ;
#endif
			}

	uint32_t certLen = 0;

	unsigned char **binptr = (unsigned char **) &(di -> certDER.bin_data);

	mAuthMgr->SaveCertificateToBinary(about, binptr, &certLen);
#ifdef P3DISC_DEBUG
	std::cerr << "Saved certificate to binary in p3discReply. Length=" << certLen << std::endl ;
#endif
	if (certLen > 0)
	{
		di -> certDER.bin_len = certLen;
#ifdef P3DISC_DEBUG
		std::cerr << "Cert Encoded(" << certLen << ")" << std::endl;
#endif
	}
	else
	{
#ifdef P3DISC_DEBUG
		std::cerr << "Failed to Encode Cert" << std::endl;
#endif
		di -> certDER.bin_len = 0;
	}

	// Send off message
	sendItem(di);

#ifdef P3DISC_DEBUG
	std::cerr << "Sent DI Message" << std::endl;
#endif
}

#endif



