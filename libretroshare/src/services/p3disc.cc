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
#include "rsiface/rsinit.h" /* for PGPSSL flag */
#include "rsiface/rspeers.h"
#include "services/p3disc.h"

#include "pqi/authssl.h"
#include "pqi/authgpg.h"
#include "pqi/p3connmgr.h"

#include <iostream>
#include <errno.h>
#include <cmath>

const uint32_t AUTODISC_LDI_SUBTYPE_PING = 0x01;
const uint32_t AUTODISC_LDI_SUBTYPE_RPLY = 0x02;

#include <sstream>

#include "util/rsdebug.h"
#include "util/rsprint.h"
#include "util/rsversion.h"

const int pqidisczone = 2482;

static int convertTDeltaToTRange(double tdelta);
static int convertTRangeToTDelta(int trange);

// Operating System specific includes.
#include "pqi/pqinetwork.h"

/* DISC FLAGS */

const uint32_t P3DISC_FLAGS_USE_DISC 		= 0x0001;
const uint32_t P3DISC_FLAGS_USE_DHT 		= 0x0002;
const uint32_t P3DISC_FLAGS_EXTERNAL_ADDR	= 0x0004;
const uint32_t P3DISC_FLAGS_STABLE_UDP	 	= 0x0008;
const uint32_t P3DISC_FLAGS_PEER_ONLINE 	= 0x0010;
const uint32_t P3DISC_FLAGS_OWN_DETAILS 	= 0x0020;
const uint32_t P3DISC_FLAGS_PEER_TRUSTS_ME= 0x0040;
const uint32_t P3DISC_FLAGS_ASK_VERSION		= 0x0080;


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

p3disc::p3disc(p3ConnectMgr *cm, pqipersongrp *pqih)
        :p3Service(RS_SERVICE_TYPE_DISC), mConnMgr(cm), mPqiPersonGrp(pqih)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	addSerialType(new RsDiscSerialiser());

	mRemoteDisc = true;
	mLocalDisc  = false;
        lastSentHeartbeatTime = 0;

	//add own version to versions map
        versions[AuthSSL::getAuthSSL()->OwnId()] = RsUtil::retroshareVersion();
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
        //send a heartbeat to all connected peers
        if (time(NULL) - lastSentHeartbeatTime > HEARTBEAT_REPEAT_TIME) {
            #ifdef P3DISC_DEBUG
            std::cerr << "p3disc::tick() sending heartbeat to all peers" << std::endl;
            #endif
            lastSentHeartbeatTime = time(NULL);
            std::list <std::string> peers;
            mConnMgr->getOnlineList(peers);
            for (std::list<std::string>::const_iterator pit = peers.begin(); pit != peers.end(); ++pit) {
                sendHeartbeat(*pit);
            }
        }
        

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
		RsDiscOwnItem *dio = NULL;
		RsDiscReply *dri = NULL;
		RsDiscIssuer *dii = NULL;
		RsDiscVersion *dvi = NULL;
                RsDiscHeartbeat *dta = NULL;

		{
#ifdef P3DISC_DEBUG
                        std::cerr << "p3disc::handleIncoming() Received Message!" << std::endl;
                        item -> print(std::cerr);
                        std::cerr  << std::endl;
#endif
		}


		// if discovery reply then respond if haven't already.
                if (NULL != (dri = dynamic_cast<RsDiscReply *> (item)))	{

                        recvPeerDetails(dri);
			nhandled++;
		}
#ifdef RS_USE_PGPSSL
                else if (NULL != (dii = dynamic_cast<RsDiscIssuer *> (item))) {

                        //recvPeerIssuerMsg(dii);
			nhandled++;
		}
#endif
                else if (NULL != (dvi = dynamic_cast<RsDiscVersion *> (item))) {
			recvPeerVersionMsg(dvi);
			nhandled++;
		}
                else if (NULL != (dio = dynamic_cast<RsDiscOwnItem *> (item))) /* Ping */ {
                        //recvPeerOwnMsg(dio);
			nhandled++;
		}
                else if (NULL != (dta = dynamic_cast<RsDiscHeartbeat *> (item))) {
                        recvHeartbeatMsg(dta);
                        return 1;
                }
		delete item;
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

	std::list<pqipeer>::const_iterator pit;
	/* if any have switched to 'connected' then we notify */
        for(pit =  plist.begin(); pit != plist.end(); pit++) {
                if ((pit->state & RS_PEER_S_FRIEND) && (pit->actions & RS_PEER_CONNECTED)) {
                        /* send their own details to them. Usefull for ext ip address detection */
//                        sendPeerDetails(pit->id, pit->id);
//                        /* send our details to them */
                        sendOwnVersion(pit->id);
                        sendAllInfoToPeer(pit->id);
		}
	}
}

void p3disc::sendAllInfoToPeer(std::string id)
{
	/* get a peer lists */

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::respondToPeer() id: " << id;
	std::cerr << std::endl;
#endif

        std::list<std::string> friendIds;
        std::list<std::string>::iterator friendIdsIt;
        std::set<std::string> gpgIds;;

        rsPeers->getFriendList(friendIds);

	/* send them a list of all friend's details */
        for(friendIdsIt = friendIds.begin(); friendIdsIt != friendIds.end(); friendIdsIt++) {
		/* get details */
		peerConnectState detail;
                if (!mConnMgr->getFriendNetStatus(*friendIdsIt, detail)) {
			/* major error! */
			continue;
		}

                if (!(detail.visState & RS_VIS_STATE_NODISC)) {
                        gpgIds.insert(detail.gpg_id);
		}
        }

        //add own info
        gpgIds.insert(rsPeers->getGPGOwnId());

        //send details for each gpg Ids
        std::set<std::string>::iterator gpgIdsIt;
        for (gpgIdsIt = gpgIds.begin(); gpgIdsIt != gpgIds.end(); gpgIdsIt++) {
            sendPeerDetails(id, *gpgIdsIt);
        }
}

 /* (dest (to), source (cert)) */
void p3disc::sendPeerDetails(std::string to, std::string about) {
	{
#ifdef P3DISC_DEBUG
		std::ostringstream out;
		out << "p3disc::sendPeerDetails()";
		out << " Sending details of: " << about;
		out << " to: " << to << std::endl;
		std::cerr << out.str() << std::endl;
#endif
	}

        about = rsPeers->getGPGId(about);

	// Construct a message
	RsDiscReply *di = new RsDiscReply();

	// Fill the message
	// Set Target as input cert.
	di -> PeerId(to);
	di -> aboutId = about;
        di -> certGPG = AuthGPG::getAuthGPG()->SaveCertificateToString(about);

        // set the ip addresse list.
        std::list<std::string> sslChilds;
        rsPeers->getSSLChildListOfGPGId(about, sslChilds);
        for (std::list<std::string>::iterator sslChildIt = sslChilds.begin(); sslChildIt != sslChilds.end(); sslChildIt++) {
            peerConnectState detail;
            if (!mConnMgr->getFriendNetStatus(*sslChildIt, detail)) {
                    continue;
            }
            RsPeerNetItem *rsPeerNetItem = new RsPeerNetItem();
            rsPeerNetItem->clear();

            rsPeerNetItem->pid = detail.id;
            rsPeerNetItem->gpg_id = detail.gpg_id;
            rsPeerNetItem->location = detail.location;
            rsPeerNetItem->netMode = detail.netMode;
            rsPeerNetItem->visState = detail.visState;
            rsPeerNetItem->lastContact = detail.lastcontact;
            rsPeerNetItem->currentlocaladdr = detail.currentlocaladdr;
            rsPeerNetItem->currentremoteaddr = detail.currentserveraddr;
            rsPeerNetItem->ipAddressList = detail.getIpAddressList();

            di->rsPeerList.push_back(*rsPeerNetItem);

        }

        //send own details
        if (about == rsPeers->getGPGOwnId()) {
            peerConnectState detail;
            if (mConnMgr->getOwnNetStatus(detail)) {
                RsPeerNetItem *rsPeerNetItem = new RsPeerNetItem();
                rsPeerNetItem->clear();
                rsPeerNetItem->pid = detail.id;
                rsPeerNetItem->gpg_id = detail.gpg_id;
                rsPeerNetItem->location = detail.location;
                rsPeerNetItem->netMode = detail.netMode;
                rsPeerNetItem->visState = detail.visState;
                rsPeerNetItem->lastContact = time(NULL);
                rsPeerNetItem->currentlocaladdr = detail.currentlocaladdr;
                rsPeerNetItem->currentremoteaddr = detail.currentserveraddr;
                rsPeerNetItem->ipAddressList = detail.getIpAddressList();

                di->rsPeerList.push_back(*rsPeerNetItem);
            }
        }

        // Send off message
#ifdef P3DISC_DEBUG
        di->print(std::cerr, 5);
#endif
        sendItem(di);

#ifdef P3DISC_DEBUG
	std::cerr << "Sent DI Message" << std::endl;
#endif
}

void p3disc::sendOwnVersion(std::string to)
{
	{
#ifdef P3DISC_DEBUG
		std::ostringstream out;
		out << "p3disc::sendOwnVersion()";
		out << " Sending rs version to: " << to << std::endl;
		std::cerr << out.str() << std::endl;
#endif
	}

	RsDiscVersion *di = new RsDiscVersion();
	di->PeerId(to);
	di->version = RsUtil::retroshareVersion();

	/* send the message */
	sendItem(di);

#ifdef P3DISC_DEBUG
	std::cerr << "Sent DI Message" << std::endl;
#endif
}

void p3disc::sendHeartbeat(std::string to)
{
        {
#ifdef P3DISC_DEBUG
                std::ostringstream out;
                out << "p3disc::sendHeartbeat()";
                out << " Sending tick to : " << to << std::endl;
                std::cerr << out.str() << std::endl;
#endif
        }

        RsDiscHeartbeat *di = new RsDiscHeartbeat();
        di->PeerId(to);

        /* send the message */
        sendItem(di);

#ifdef P3DISC_DEBUG
        std::cerr << "Sent tick Message" << std::endl;
#endif
}

void p3disc::recvPeerDetails(RsDiscReply *item)
{

#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::recvPeerFriendMsg() From: " << item->PeerId() << " About " << item->aboutId << std::endl;
#endif
        std::string certGpgId;
        AuthGPG::getAuthGPG()->LoadCertificateFromString(item->certGPG, certGpgId);
        if (item->aboutId != certGpgId) {
 #ifdef P3DISC_DEBUG
        std::cerr << "p3disc::recvPeerFriendMsg() Error : about id is not the same as gpg id." << std::endl;
#endif
           return;
        }

        for (std::list<RsPeerNetItem>::iterator pitem = item->rsPeerList.begin(); pitem != item->rsPeerList.end(); pitem++) {
            //don't update dummy friends
            if (rsPeers->isDummyFriend(pitem->pid)) {
                continue;
            }

            addDiscoveryData(item->PeerId(), pitem->pid, pitem->currentlocaladdr, pitem->currentremoteaddr, 0, time(NULL));

            #ifdef P3DISC_DEBUG
            std::cerr << "pp3disc::recvPeerFriendMsg() Peer Config Item:" << std::endl;
            pitem->print(std::cerr, 10);
            std::cerr << std::endl;
            #endif
            if (pitem->pid != rsPeers->getOwnId()) {
                mConnMgr->addFriend(pitem->pid, pitem->gpg_id, pitem->netMode, pitem->visState, 0);
                RsPeerDetails storedDetails;
                rsPeers->getPeerDetails(pitem->pid, storedDetails);
                if (    (!(storedDetails.state & RS_PEER_CONNECTED) && storedDetails.lastConnect < (pitem->lastContact - 10000))
                        || item->PeerId() == pitem->pid) { //update if it's fresh info or if it's from the peer itself
                    //their info is fresher than ours (there is a 10000 seconds margin), update ours
                    mConnMgr->setLocalAddress(pitem->pid, pitem->currentlocaladdr);
                    mConnMgr->setExtAddress(pitem->pid, pitem->currentremoteaddr);
                    mConnMgr->setVisState(pitem->pid, pitem->visState);
                    mConnMgr->setNetworkMode(pitem->pid, pitem->netMode);
                    if (storedDetails.location == "") {
                        mConnMgr->setLocation(pitem->pid, pitem->location);
                    }
                }
            } else {
                if (pitem->currentremoteaddr.sin_addr.s_addr != 0 && pitem->currentremoteaddr.sin_port != 0 &&
                    pitem->currentremoteaddr.sin_addr.s_addr != 1 && pitem->currentremoteaddr.sin_port != 1 &&
                    std::string(inet_ntoa(pitem->currentremoteaddr.sin_addr)) != "1.1.1.1" &&
                    (!isLoopbackNet(&pitem->currentremoteaddr.sin_addr)) &&
                    (!isPrivateNet(&pitem->currentremoteaddr.sin_addr))
                    ) {
                    //the current server address given by the peer looks nice, let's use it for our own ext address if needed
                    sockaddr_in tempAddr;
                    if (!mConnMgr->getExtFinderExtAddress(tempAddr) && !mConnMgr->getUpnpExtAddress(tempAddr)) {
                        //don't change the port, just the ip
                        pitem->currentremoteaddr.sin_port = mConnMgr->ownState.currentserveraddr.sin_port;
                        mConnMgr->setExtAddress(mConnMgr->getOwnId(), pitem->currentremoteaddr);
                    }
                }
            }
            //allways update address list
            mConnMgr->setAddressList(pitem->pid, pitem->ipAddressList);

        }
	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_NEIGHBOURS, NOTIFY_TYPE_MOD);

        /* cleanup (handled by caller) */
}


//void p3disc::recvPeerIssuerMsg(RsDiscIssuer *item)
//{
//
//#ifdef P3DISC_DEBUG
//	std::cerr << "p3disc::recvPeerIssuerMsg() From: " << item->PeerId();
//	std::cerr << std::endl;
//	std::cerr << "p3disc::recvPeerIssuerMsg() Cert: " << item->issuerCert;
//	std::cerr << std::endl;
//#endif
//
//	/* tells us their exact address (mConnectMgr can ignore if it looks wrong) */
//
//	/* load certificate */
//        std::string gpgId;
//        bool loaded = AuthGPG::getAuthGPG()->LoadCertificateFromString(item->issuerCert, gpgId);
//
//	/* cleanup (handled by caller) */
//
//	return;
//}

void p3disc::recvPeerVersionMsg(RsDiscVersion *item)
{
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::recvPeerVersionMsg() From: " << item->PeerId();
        std::cerr << std::endl;
#endif

        // dont need protection
        versions[item->PeerId()] = item->version;

        return;
}

void p3disc::recvHeartbeatMsg(RsDiscHeartbeat *item)
{
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::recvHeartbeatMsg() From: " << item->PeerId();
        std::cerr << std::endl;
#endif

        mPqiPersonGrp->getPeer(item->PeerId())->receiveHeartbeat();

        return;
}

/*************************************************************************************/
/*				Storing Network Graph				     */
/*************************************************************************************/
int	p3disc::addDiscoveryData(std::string fromId, std::string aboutId, struct sockaddr_in laddr, struct sockaddr_in raddr, uint32_t flags, time_t ts)
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

void p3disc::getversions(std::map<std::string, std::string> &versions)
{
	versions = this->versions;
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

