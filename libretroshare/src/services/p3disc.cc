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


#include "services/p3disc.h"

#include "pqi/p3authmgr.h"
#include "pqi/p3connmgr.h"

#include <iostream>
#include <errno.h>
#include <cmath>

const uint32_t AUTODISC_LDI_SUBTYPE_PING = 0x01;
const uint32_t AUTODISC_LDI_SUBTYPE_RPLY = 0x02;

#include <sstream>

#include "pqi/pqidebug.h"
#include "util/rsprint.h"

const int pqidisczone = 2482;

static int convertTDeltaToTRange(double tdelta);
static int convertTRangeToTDelta(int trange);

// Operating System specific includes.
#include "pqi/pqinetwork.h"

/* DISC FLAGS */

const uint32_t P3DISC_FLAGS_USE_DISC 		= 0x0001;
const uint32_t P3DISC_FLAGS_USE_DHT 		= 0x0002;
const uint32_t P3DISC_FLAGS_EXTERNAL_ADDR 	= 0x0004;
const uint32_t P3DISC_FLAGS_STABLE_UDP	 	= 0x0008;
const uint32_t P3DISC_FLAGS_PEER_ONLINE 	= 0x0010;
const uint32_t P3DISC_FLAGS_OWN_DETAILS 	= 0x0020;


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

p3disc::p3disc(p3AuthMgr *am, p3ConnectMgr *cm)
	:p3Service(RS_SERVICE_TYPE_DISC), mAuthMgr(am), mConnMgr(cm)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	addSerialType(new RsDiscSerialiser());

	mRemoteDisc = true;
	mLocalDisc  = false;
	return;
}

int p3disc::tick()
{

#ifdef P3DISC_DEBUG

static int count = 0;

	if (++count % 10 == 0)
	{
		idServers();
	}
#endif

	return handleIncoming();
}

int p3disc::handleIncoming()
{
	RsItem *item = NULL;

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::handleIncoming()";
	std::cerr << std::endl;
#endif

	bool discOn;

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
		discOn = mRemoteDisc;
	}

	// if off discard item.
	if (!mRemoteDisc)
	{
		while(NULL != (item = recvItem()))
		{
#ifdef P3DISC_DEBUG
			std::ostringstream out;
			out << "p3disc::handleIncoming()";
			out << " Deleting - Cos RemoteDisc Off!" << std::endl;

			item -> print(out);

			std::cerr << out.str() << std::endl;
#endif

			delete item;
		}
		return 0;
	}

	int nhandled = 0;
	// While messages read
	while(NULL != (item = recvItem()))
	{
		RsDiscItem *di = NULL;
		RsDiscReply *dri = NULL;

		if (NULL == (di = dynamic_cast<RsDiscItem *> (item)))
		{

#ifdef P3DISC_DEBUG
			std::ostringstream out;
			out << "p3disc::handleIncoming()";
			out << "Deleting Non RsDiscItem Msg" << std::endl;
			item -> print(out);

			std::cerr << out.str() << std::endl;
#endif

			// delete and continue to next loop.
			delete item;

			continue;
		}
		nhandled++;

		{
#ifdef P3DISC_DEBUG
			std::ostringstream out;
			out << "p3disc::handleIncoming()";
			out << " Received Message!" << std::endl;
			di -> print(out);

			std::cerr << out.str() << std::endl;
#endif
		}


		// if discovery reply then respond if haven't already.
		if (NULL != (dri = dynamic_cast<RsDiscReply *> (di)))
		{

			recvPeerFriendMsg(dri);
		}
		else /* Ping */
		{
			recvPeerOwnMsg(di);
		}
		delete di;
	}
	return nhandled;
}



        /************* from pqiMonitor *******************/
void p3disc::statusChange(const std::list<pqipeer> &plist)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::statusChange()";
	std::cerr << std::endl;
#endif

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
}

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

	uint32_t certLen = 0;

	unsigned char **binptr = (unsigned char **) &(di -> certDER.bin_data);
	mAuthMgr->SaveCertificateToBinary(about, binptr, &certLen);
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


/*************************************************************************************/
/*				Input Network Msgs				     */
/*************************************************************************************/
void p3disc::recvPeerOwnMsg(RsDiscItem *item)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::recvPeerOwnMsg() From: " << item->PeerId() << std::endl;
#endif

	/* tells us their exact address (mConnectMgr can ignore if it looks wrong) */
	uint32_t type = 0; 
	uint32_t flags = 0; 

	/* translate flags */
	if (item->discFlags & P3DISC_FLAGS_USE_DISC)
	{
		flags |= RS_NET_FLAGS_USE_DISC;
	}
	if (item->discFlags & P3DISC_FLAGS_USE_DHT)
	{
		flags |= RS_NET_FLAGS_USE_DHT;
	}
	if (item->discFlags & P3DISC_FLAGS_PEER_ONLINE)
	{
		flags |= RS_NET_FLAGS_ONLINE;
	}

	/* generate type */
	type = RS_NET_CONN_TCP_LOCAL;
	if (item->discFlags & P3DISC_FLAGS_EXTERNAL_ADDR)
	{
		type |= RS_NET_CONN_TCP_EXTERNAL;
		flags |= RS_NET_FLAGS_EXTERNAL_ADDR;
	}

	if (item->discFlags & P3DISC_FLAGS_STABLE_UDP)
	{
		type |= RS_NET_CONN_UDP_DHT_SYNC;
		flags |= RS_NET_FLAGS_STABLE_UDP;
	}

	mConnMgr->peerStatus(item->PeerId(), item->laddr, item->saddr, 
				type, flags, RS_CB_PERSON);

	/* also add as potential stun buddy */
	std::string hashid1 = RsUtil::HashId(item->PeerId(), false);
	mConnMgr->stunStatus(hashid1, item->saddr, type, 
				RS_STUN_ONLINE | RS_STUN_FRIEND);

	/* now reply with all details */
	respondToPeer(item->PeerId());

	addDiscoveryData(item->PeerId(), item->PeerId(), 
			item->laddr, item->saddr, item->discFlags, time(NULL));

	/* cleanup (handled by caller) */
}


void p3disc::recvPeerFriendMsg(RsDiscReply *item)
{

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::recvPeerFriendMsg() From: " << item->PeerId();
	std::cerr << " About " << item->aboutId;
	std::cerr << std::endl;
#endif

	/* tells us their exact address (mConnectMgr can ignore if it looks wrong) */

	/* load certificate */
	std::string peerId;

	uint8_t *certptr = (uint8_t *) item->certDER.bin_data;
	uint32_t len = item->certDER.bin_len;

	bool loaded = mAuthMgr->LoadCertificateFromBinary(certptr, len, peerId);

	uint32_t type = 0; 
	uint32_t flags = 0; 

	/* translate flags */
	if (item->discFlags & P3DISC_FLAGS_USE_DISC)
	{
		flags |= RS_NET_FLAGS_USE_DISC;
	}
	if (item->discFlags & P3DISC_FLAGS_USE_DHT)
	{
		flags |= RS_NET_FLAGS_USE_DHT;
	}
	if (item->discFlags & P3DISC_FLAGS_PEER_ONLINE)
	{
		flags |= RS_NET_FLAGS_ONLINE;
	}

	/* generate type */
	type = RS_NET_CONN_TCP_LOCAL;
	if (item->discFlags & P3DISC_FLAGS_EXTERNAL_ADDR)
	{
		type |= RS_NET_CONN_TCP_EXTERNAL;
		flags |= RS_NET_FLAGS_EXTERNAL_ADDR;
	}

	if (item->discFlags & P3DISC_FLAGS_STABLE_UDP)
	{
		type |= RS_NET_CONN_UDP_DHT_SYNC;
		flags |= RS_NET_FLAGS_STABLE_UDP;
	}

	/* only valid certs, and not ourselves */
	if ((loaded) && (peerId != mConnMgr->getOwnId()))
	{
		mConnMgr->peerStatus(peerId, item->laddr, 
					item->saddr, type, flags, RS_CB_DISC);

		std::string hashid1 = RsUtil::HashId(peerId, false);
		mConnMgr->stunStatus(hashid1, item->saddr, type, 
						RS_STUN_FRIEND_OF_FRIEND);

	}

	addDiscoveryData(item->PeerId(), peerId, item->laddr, item->saddr, item->discFlags, time(NULL));

	/* cleanup (handled by caller) */
}


/*************************************************************************************/
/*				Storing Network Graph				     */
/*************************************************************************************/


int	p3disc::addDiscoveryData(std::string fromId, std::string aboutId, 
		struct sockaddr_in laddr, struct sockaddr_in raddr, uint32_t flags, time_t ts)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	/* Store Network information */
	std::map<std::string, autoneighbour>::iterator it;
	if (neighbours.end() == (it = neighbours.find(aboutId)))
	{
		/* doesn't exist */
		autoneighbour an;

		/* an data */
		an.id = aboutId;
		an.validAddrs = false;
		an.discFlags = 0;
		an.ts = 0;

		neighbours[aboutId] = an;

		it = neighbours.find(aboutId);
	}

	/* it always valid */

	/* just update packet */

	autoserver as;

	as.id = fromId;
	as.localAddr = laddr;
	as.remoteAddr = raddr;
	as.discFlags = flags;
	as.ts = ts;

	bool authDetails = (as.id == it->second.id);

	/* KEY decision about address */
	if ((authDetails) || 
		((!(it->second.authoritative)) && (as.ts > it->second.ts)))
	{
		/* copy details to an */
		it->second.authoritative = authDetails;
		it->second.ts = as.ts;
		it->second.validAddrs = true;
		it->second.localAddr = as.localAddr;
		it->second.remoteAddr = as.remoteAddr;
		it->second.discFlags = as.discFlags;
	}

	(it->second).neighbour_of[fromId] = as;

	/* do we update network address info??? */
	return 1;

}



/*************************************************************************************/
/*			   Extracting Network Graph Details			     */
/*************************************************************************************/

bool p3disc::potentialproxies(std::string id, std::list<std::string> &proxyIds)
{
	/* find id -> and extract the neighbour_of ids */

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, autoneighbour>::iterator it;
	std::map<std::string, autoserver>::iterator sit;
	if (neighbours.end() == (it = neighbours.find(id)))
	{
		return false;
	}

	for(sit = it->second.neighbour_of.begin();
		sit != it->second.neighbour_of.end(); sit++)
	{
		proxyIds.push_back(sit->first);
	}
	return true;
}


int p3disc::idServers()
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, autoneighbour>::iterator nit;
	std::map<std::string, autoserver>::iterator sit;
	int cts = time(NULL);

	std::ostringstream out;
	out << "::::AutoDiscovery Neighbours::::" << std::endl;
	for(nit = neighbours.begin(); nit != neighbours.end(); nit++)
	{
		out << "Neighbour: " << (nit->second).id;
		out << std::endl;
		out << "-> LocalAddr: ";
		out <<  inet_ntoa(nit->second.localAddr.sin_addr);
		out << ":" << ntohs(nit->second.localAddr.sin_port) << std::endl;
		out << "-> RemoteAddr: ";
		out <<  inet_ntoa(nit->second.remoteAddr.sin_addr);
		out << ":" << ntohs(nit->second.remoteAddr.sin_port) << std::endl;
		out << "  Last Contact: ";
		out << cts - (nit->second.ts) << " sec ago";
		out << std::endl;

		out << " -->DiscFlags: 0x" << std::hex << nit->second.discFlags;
		out << std::dec << std::endl;

		for(sit = (nit->second.neighbour_of).begin(); 
				sit != (nit->second.neighbour_of).end(); sit++)
		{
			out << "\tConnected via: " << (sit->first);
			out << std::endl;
			out << "\t\tLocalAddr: ";
			out <<  inet_ntoa(sit->second.localAddr.sin_addr);
			out <<":"<< ntohs(sit->second.localAddr.sin_port);
			out << std::endl;
			out << "\t\tRemoteAddr: ";
			out <<  inet_ntoa(sit->second.remoteAddr.sin_addr);
			out <<":"<< ntohs(sit->second.remoteAddr.sin_port);

			out << std::endl;
			out << "\t\tLast Contact:";
			out << cts - (sit->second.ts) << " sec ago";
			out << std::endl;
			out << "\t\tDiscFlags: 0x" << std::hex << (sit->second.discFlags);
			out << std::dec << std::endl;
		}
	}

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::idServers()" << std::endl;
	std::cerr << out.str();
	std::cerr << std::endl;
#endif

	return 1;
}


// tdelta     -> trange.
// -inf...<0	   0 (invalid)
//    0.. <9       1
//    9...<99      2
//   99...<999	   3
//  999...<9999    4
//  etc...

int convertTDeltaToTRange(double tdelta)
{
	if (tdelta < 0)
		return 0;
	int trange = 1 + (int) log10(tdelta + 1.0);
	return trange;

}

// trange     -> tdelta
// -inf...0	  -1 (invalid)
//    1            8
//    2           98
//    3          998
//    4         9998
//  etc...

int convertTRangeToTDelta(int trange)
{
	if (trange <= 0)
		return -1;
	
	return (int) (pow(10.0, trange) - 1.5); // (int) xxx98.5 -> xxx98
}

