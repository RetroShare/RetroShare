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


#include "retroshare/rsiface.h"
#include "retroshare/rsinit.h" /* for PGPSSL flag */
#include "retroshare/rspeers.h"
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

//static int convertTDeltaToTRange(double tdelta);
//static int convertTRangeToTDelta(int trange);

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

//#define P3DISC_DEBUG 	1

/*********** NOTE ***************
 *
 * Only need Mutexs for neighbours information
 */

/******************************************************************************************
 ******************************  NEW DISCOVERY  *******************************************
 ******************************************************************************************
 *****************************************************************************************/

p3disc::p3disc(p3ConnectMgr *cm, pqipersongrp *pqih)
        :p3Service(RS_SERVICE_TYPE_DISC),
				 p3Config(CONFIG_TYPE_P3DISC),
		  mConnMgr(cm), 
		  mPqiPersonGrp(pqih)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	addSerialType(new RsDiscSerialiser());

        lastSentHeartbeatTime = time(NULL);

	//add own version to versions map
        versions[AuthSSL::getAuthSSL()->OwnId()] = RsUtil::retroshareVersion();
#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::p3disc() setup";
	std::cerr << std::endl;
#endif

	return;
}

int p3disc::tick()
{
        //send a heartbeat to all connected peers
        if (time(NULL) - lastSentHeartbeatTime > HEARTBEAT_REPEAT_TIME) 
	{
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
	std::cerr << "p3disc::handleIncoming()" << std::endl;
#endif

	// if off discard item.
        peerConnectState detail;
        if (!mConnMgr->getOwnNetStatus(detail) || (detail.visState & RS_VIS_STATE_NODISC)) 
	{
		while(NULL != (item = recvItem()))
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::handleIncoming() Deleting - Cos RemoteDisc Off!" << std::endl;
			item -> print(std::cerr);
			std::cerr << std::endl;
#endif

			delete item;
		}
		return 0;
	}

	int nhandled = 0;
	// While messages read
	while(NULL != (item = recvItem()))
	{
		RsDiscAskInfo *inf = NULL;
		RsDiscReply *dri = NULL;
		RsDiscVersion *dvi = NULL;
		RsDiscHeartbeat *dta = NULL;

#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::handleIncoming() Received Message!" << std::endl;
		item -> print(std::cerr);
		std::cerr  << std::endl;
#endif


		// if discovery reply then respond if haven't already.
                if (NULL != (dri = dynamic_cast<RsDiscReply *> (item)))	{

			RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

			/* search pending item and remove it, when already exist */
			std::list<RsDiscReply*>::iterator it;
			for (it = pendingDiscReplyInList.begin(); it != pendingDiscReplyInList.end(); it++) {
				if ((*it)->PeerId() == dri->PeerId() && (*it)->aboutId == dri->aboutId) {
					delete (*it);
					pendingDiscReplyInList.erase(it);
					break;
				}
			}

			// add item to list for later process
			pendingDiscReplyInList.push_back(dri); // no delete
		}
                else if (NULL != (dvi = dynamic_cast<RsDiscVersion *> (item))) {
			recvPeerVersionMsg(dvi);
			nhandled++;
			delete item;
		}
                else if (NULL != (inf = dynamic_cast<RsDiscAskInfo *> (item))) /* Ping */ {
			recvAskInfo(inf);
			nhandled++;
			delete item;
		}
                else if (NULL != (dta = dynamic_cast<RsDiscHeartbeat *> (item))) {
			recvHeartbeatMsg(dta);
			nhandled++ ;
			delete item;
		}
		else
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::handleIncoming() Unknown Received Message!" << std::endl;
			item -> print(std::cerr);
			std::cerr  << std::endl;
#endif
			delete item;
		}
	}

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::handleIncoming() finished." << std::endl;
#endif

	return nhandled;
}



        /************* from pqiMonitor *******************/
void p3disc::statusChange(const std::list<pqipeer> &plist)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::statusChange()" << std::endl;
#endif

	std::list<pqipeer>::const_iterator pit;
	/* if any have switched to 'connected' then we notify */
	for(pit =  plist.begin(); pit != plist.end(); pit++) 
	{
		if ((pit->state & RS_PEER_S_FRIEND) && (pit->actions & RS_PEER_CONNECTED)) 
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::statusChange() Starting Disc with: " << pit->id << std::endl;
#endif
			sendOwnVersion(pit->id);
			sendAllInfoToJustConnectedPeer(pit->id);
			sendJustConnectedPeerInfoToAllPeer(pit->id);
		} 
		else if (!(pit->state & RS_PEER_S_FRIEND) && (pit->actions & RS_PEER_MOVED)) 
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::statusChange() Removing Friend: " << pit->id << std::endl;
#endif
			removeFriend(pit->id);
		}
		else if ((pit->state & RS_PEER_S_FRIEND) && (pit->actions & RS_PEER_NEW)) 
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::statusChange() Adding Friend: " << pit->id << std::endl;
#endif
			askInfoToAllPeers(pit->id);
		}
	}
#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::statusChange() finished." << std::endl;
#endif
	return;
}

void p3disc::sendAllInfoToJustConnectedPeer(const std::string &id)
{
	/* get a peer lists */

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() id: " << id << std::endl;
#endif

	RsPeerDetails pd;
	rsPeers->getPeerDetails(id, pd);

	if (!pd.accept_connection || (!pd.ownsign && pd.gpg_id != rsPeers->getGPGOwnId())) 
	{ 
		//only send info when connection is accepted and gpg key is signed or our own key
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() we're not sending the info because the destination gpg key is not signed or not accepted." << std::cerr << std::endl;
#endif
		return;
	}


	std::list<std::string> friendIds;
	std::list<std::string>::iterator friendIdsIt;
	std::set<std::string> gpgIds;

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

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		/* append gpg id's to the sending list for the id */

		std::list<std::string> &idList = sendIdList[id];

		std::set<std::string>::iterator gpgIdsIt;
		for (gpgIdsIt = gpgIds.begin(); gpgIdsIt != gpgIds.end(); gpgIdsIt++) {
			if (std::find(idList.begin(), idList.end(), *gpgIdsIt) == idList.end()) {
				idList.push_back(*gpgIdsIt);
			}
		}
	}

        #ifdef P3DISC_DEBUG
	std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() finished." << std::endl;
        #endif
}

void p3disc::sendJustConnectedPeerInfoToAllPeer(const std::string &connectedPeerId)
{
	/* get a peer lists */

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() connectedPeerId : " << connectedPeerId << std::endl;
#endif
	std::string gpg_connectedPeerId = rsPeers->getGPGId(connectedPeerId);
	std::list<std::string> onlineIds;

	rsPeers->getOnlineList(onlineIds);

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		/* append gpg id's of all friend's to the sending list */

		std::list<std::string>::iterator onlineIdsIt;
		for (onlineIdsIt = onlineIds.begin(); onlineIdsIt != onlineIds.end(); onlineIdsIt++) {
			std::list<std::string> &idList = sendIdList[*onlineIdsIt];
			if (std::find(idList.begin(), idList.end(), gpg_connectedPeerId) == idList.end()) {
				idList.push_back(gpg_connectedPeerId);
			}
		}
	}
}

 /* (dest (to), source (cert)) */
RsDiscReply *p3disc::createDiscReply(const std::string &to, const std::string &about)
{
#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::createDiscReply() called. Sending details of: " << about << " to: " << to << std::endl;
#endif

	RsPeerDetails pd;
	rsPeers->getPeerDetails(to, pd);
	if (!pd.accept_connection || !pd.ownsign) {
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::createDiscReply() we're not sending the info because the destination gpg key is not signed or not accepted." << std::cerr << std::endl;
#endif
		return NULL;
	}


	// if off discard item.
	peerConnectState detail;
        if (!mConnMgr->getOwnNetStatus(detail) || (detail.visState & RS_VIS_STATE_NODISC)) {
		return NULL;
        }

	std::string aboutGpgId = rsPeers->getGPGId(about);
	if (aboutGpgId.empty()) {
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::createDiscReply() no info about this id" << std::endl;
#endif
		return NULL;
	}

	// Construct a message
	RsDiscReply *di = new RsDiscReply();

	// Fill the message
	// Set Target as input cert.
	di -> PeerId(to);
	di -> aboutId = aboutGpgId;

	// set the ip addresse list.
	std::list<std::string> sslChilds;
	rsPeers->getSSLChildListOfGPGId(aboutGpgId, sslChilds);
	bool shouldWeSendGPGKey = false;//the GPG key is send only if we've got a valid friend with DISC enabled

	std::list<std::string>::iterator sslChildIt;
	for (sslChildIt = sslChilds.begin(); sslChildIt != sslChilds.end(); sslChildIt++) 
		if(!rsPeers->isDummyFriend(*sslChildIt))
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::createDiscReply() Found Child SSL Id:" << *sslChildIt;
			std::cerr << std::endl;
#endif
			if(sslChilds.size() == 1 || to != *sslChildIt)	// We don't send info to a peer about itself, when there are more than one ssl children,
				{						// but we allow sending info about peers with the same GPG id. When there is only one ssl child,
					// we must send it to transfer the signers of the gpg key. The receiver is skipping the own id.
					peerConnectState detail;
					if (!mConnMgr->getFriendNetStatus(*sslChildIt, detail) 
							|| detail.visState & RS_VIS_STATE_NODISC)
					{
#ifdef P3DISC_DEBUG
						std::cerr << "p3disc::createDiscReply() Skipping cos No Details or NODISC flag";
						std::cerr << std::endl;
#endif
						continue;
					}

#ifdef P3DISC_DEBUG
					std::cerr << "p3disc::createDiscReply() Adding Child SSL Id Details";
					std::cerr << std::endl;
#endif
					shouldWeSendGPGKey = true;
					RsPeerNetItem rsPeerNetItem ;
					rsPeerNetItem.clear();

					rsPeerNetItem.pid = detail.id;
					rsPeerNetItem.gpg_id = detail.gpg_id;
					rsPeerNetItem.location = detail.location;
					rsPeerNetItem.netMode = detail.netMode;
					rsPeerNetItem.visState = detail.visState;
					rsPeerNetItem.lastContact = detail.lastcontact;
					rsPeerNetItem.currentlocaladdr = detail.currentlocaladdr;
					rsPeerNetItem.currentremoteaddr = detail.currentserveraddr;
					rsPeerNetItem.dyndns = detail.dyndns;
					detail.ipAddrs.mLocal.loadTlv(rsPeerNetItem.localAddrList);
					detail.ipAddrs.mExt.loadTlv(rsPeerNetItem.extAddrList);


					di->rsPeerList.push_back(rsPeerNetItem);
				}
			else
			{
#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::createDiscReply() Skipping cos \"to == sslChildId\"";
				std::cerr << std::endl;
#endif
			}
		}


	//send own details
	if (about == rsPeers->getGPGOwnId()) 
	{
		peerConnectState detail;
		if (mConnMgr->getOwnNetStatus(detail)) 
		{
			shouldWeSendGPGKey = true;
			RsPeerNetItem rsPeerNetItem ;
			rsPeerNetItem.clear();
			rsPeerNetItem.pid = detail.id;
			rsPeerNetItem.gpg_id = detail.gpg_id;
			rsPeerNetItem.location = detail.location;
			rsPeerNetItem.netMode = detail.netMode;
			rsPeerNetItem.visState = detail.visState;
			rsPeerNetItem.lastContact = time(NULL);
			rsPeerNetItem.currentlocaladdr = detail.currentlocaladdr;
			rsPeerNetItem.currentremoteaddr = detail.currentserveraddr;
			rsPeerNetItem.dyndns = detail.dyndns;
			detail.ipAddrs.mLocal.loadTlv(rsPeerNetItem.localAddrList);
			detail.ipAddrs.mExt.loadTlv(rsPeerNetItem.extAddrList);

			di->rsPeerList.push_back(rsPeerNetItem);
		}
	}

	if (!shouldWeSendGPGKey) {
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::createDiscReply() GPG key should not be send, no friend with disc on found about it." << std::endl;
#endif
		// cleanup!
		delete di;
		return NULL;
	}

	return di;
}

void p3disc::sendOwnVersion(std::string to)
{
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::sendOwnVersion() Sending rs version to: " << to << std::endl;
#endif

	RsDiscVersion *di = new RsDiscVersion();
	di->PeerId(to);
	di->version = RsUtil::retroshareVersion();

	/* send the message */
	sendItem(di);

#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::sendOwnVersion() finished." << std::endl;
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

void p3disc::askInfoToAllPeers(std::string about)
{

#ifdef P3DISC_DEBUG
        std::cerr <<"p3disc::askInfoToAllPeers() about " << about << std::endl;
#endif


        peerConnectState connectState;
        if (!mConnMgr->getFriendNetStatus(about, connectState)) // || (connectState.visState & RS_VIS_STATE_NODISC)) {
		  {
#ifdef P3DISC_DEBUG
            std::cerr << "p3disc::askInfoToAllPeers() friend disc is off, don't send the request." << std::endl;
#endif
            return;
        }

        std::string aboutGpgId = rsPeers->getGPGId(about);
        if (aboutGpgId == "") {
#ifdef P3DISC_DEBUG
            std::cerr << "p3disc::askInfoToAllPeers() no gpg id found" << std::endl;
#endif
        }

        // if off discard item.
        if (!mConnMgr->getOwnNetStatus(connectState) || (connectState.visState & RS_VIS_STATE_NODISC)) {
#ifdef P3DISC_DEBUG
            std::cerr << "p3disc::askInfoToAllPeers() no gpg id found" << std::endl;
#endif
            return;
        }

        std::list<std::string> onlineIds;
        std::list<std::string>::iterator onlineIdsIt;

        rsPeers->getOnlineList(onlineIds);

        /* ask info to trusted friends */
        for(onlineIdsIt = onlineIds.begin(); onlineIdsIt != onlineIds.end(); onlineIdsIt++) 
	{
                RsPeerDetails details;
                rsPeers->getPeerDetails(*onlineIdsIt, details);
                if (!details.accept_connection || !details.ownsign) 
		{
#ifdef P3DISC_DEBUG
                    std::cerr << "p3disc::askInfoToAllPeers() don't ask info message to untrusted peer." << std::endl;
#endif
                    continue;
                }
                RsDiscAskInfo *di = new RsDiscAskInfo();
                di->PeerId(*onlineIdsIt);
                di->gpg_id = about;
                sendItem(di);
#ifdef P3DISC_DEBUG
                std::cerr << "p3disc::askInfoToAllPeers() question sent to : " << *onlineIdsIt << std::endl;
#endif
        }
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::askInfoToAllPeers() finished." << std::endl;
#endif
}

void p3disc::recvPeerDetails(RsDiscReply *item, const std::string &certGpgId)
{
	// discovery is only disabled for sending, not for receiving.
//	// if off discard item.
//	peerConnectState detail;
//	if (!mConnMgr->getOwnNetStatus(detail) || (detail.visState & RS_VIS_STATE_NODISC)) {
//		return;
//	}

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::recvPeerFriendMsg() From: " << item->PeerId() << " About " << item->aboutId << std::endl;
#endif

	if (certGpgId.empty()) {
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::recvPeerFriendMsg() gpg cert is not good, aborting" << std::endl;
#endif
		return;
	}
        if (item->aboutId == "" || item->aboutId != certGpgId) {
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::recvPeerFriendMsg() Error : about id is not the same as gpg id." << std::endl;
#endif
		return;
	}

  bool should_notify_discovery = false ;

	for (std::list<RsPeerNetItem>::iterator pitem = item->rsPeerList.begin(); pitem != item->rsPeerList.end(); pitem++) 
		if(!rsPeers->isDummyFriend(pitem->pid)) 
		{
			bool new_info ;
			addDiscoveryData(item->PeerId(), pitem->pid,rsPeers->getGPGId(item->PeerId()),item->aboutId, pitem->currentlocaladdr, pitem->currentremoteaddr, 0, time(NULL),new_info);

			if(new_info)
				should_notify_discovery = true ;

#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::recvPeerFriendMsg() Peer Config Item:" << std::endl;

			pitem->print(std::cerr, 10);
			std::cerr << std::endl;
#endif
			if (pitem->pid != rsPeers->getOwnId())
			{
				// Apparently, the connect manager won't add a friend if the gpg id is not
				// trusted. However, this should be tested here for consistency and security 
				// in case of modifications in mConnMgr.
				//
				if(AuthGPG::getAuthGPG()->isGPGAccepted(pitem->gpg_id) ||  pitem->gpg_id == AuthGPG::getAuthGPG()->getGPGOwnId())
				{
					// Add with no disc by default. If friend already exists, it will do nothing
					//
#ifdef P3DISC_DEBUG
					std::cerr << "--> Adding to friends list " << pitem->pid << " - " << pitem->gpg_id << std::endl;
#endif
					mConnMgr->addFriend(pitem->pid, pitem->gpg_id, pitem->netMode, 0, 0); 
					RsPeerDetails storedDetails;

					// Update if know this peer
					if(rsPeers->getPeerDetails(pitem->pid, storedDetails))
					{ 
						// Update if it's fresh info or if it's from the peer itself
						// their info is fresher than ours, update ours
						//
						if(!(storedDetails.state & RS_PEER_CONNECTED)) 
						{
#ifdef P3DISC_DEBUG
							std::cerr << "Friend is not connected -> updating info" << std::endl;
							std::cerr << "   -> network mode: " << pitem->netMode << std::endl;
							std::cerr << "   ->     location: " << pitem->location << std::endl;
#endif
							mConnMgr->setNetworkMode(pitem->pid, pitem->netMode);
							mConnMgr->setLocation(pitem->pid, pitem->location);
						}

						// The info from the peer itself is ultimately trustable, so we can override some info,
						// such as:
						// 	- local and global addresses
						// 	- address list
						//
						// If we enter here, we're necessarily connected to this peer.
						//
						if (item->PeerId() == pitem->pid) 
						{
#ifdef P3DISC_DEBUG
							std::cerr << "Info sent by the peer itself -> updating self info:" << std::endl;
							std::cerr << "  -> current local  addr = " << pitem->currentlocaladdr << std::endl;
							std::cerr << "  -> current remote addr = " << pitem->currentremoteaddr << std::endl;
							std::cerr << "  -> clearing NODISC flag " << std::endl;
#endif

							// When the peer sends his own list of IPs, the info replaces the existing info, because the
							// peer is the primary source of his own IPs.

							mConnMgr->setLocalAddress(pitem->pid, pitem->currentlocaladdr);
							mConnMgr->setExtAddress(pitem->pid, pitem->currentremoteaddr);
							pitem->visState &= ~RS_VIS_STATE_NODISC ;
							mConnMgr->setVisState(pitem->pid, pitem->visState); 
						}
					}
					else
					{
						std::cerr << "p3disc:: ERROR HOW DID WE GET HERE?" << std::endl;
					}

					pqiIpAddrSet addrsFromPeer;	
					addrsFromPeer.mLocal.extractFromTlv(pitem->localAddrList);
					addrsFromPeer.mExt.extractFromTlv(pitem->extAddrList);

#ifdef P3DISC_DEBUG
					std::cerr << "Setting address list to peer " << pitem->pid << ", to be:" << std::endl ;

					addrsFromPeer.printAddrs(std::cerr);
					std::cerr << std::endl;
#endif
					// allways update address list and dns, except if it's ours
					if (pitem->dyndns != "") 
						mConnMgr->setDynDNS(pitem->pid, pitem->dyndns);

					mConnMgr->updateAddressList(pitem->pid, addrsFromPeer);
				}
#ifdef P3DISC_DEBUG
				else
				{
					std::cerr << "  skipping unknown gpg id " << pitem->gpg_id << std::endl ;
				}
#endif
			}
#ifdef P3DISC_DEBUG
			else
			{
				std::cerr << "Skipping info about own id " << pitem->pid << std::endl ;
			}
#endif

		}

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_NEIGHBOURS, NOTIFY_TYPE_MOD);

	if(should_notify_discovery)
		rsicontrol->getNotify().notifyDiscInfoChanged();

	/* cleanup (handled by caller) */
}

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

        mPqiPersonGrp->tagHeartbeatRecvd(item->PeerId());

        return;
}

void p3disc::recvAskInfo(RsDiscAskInfo *item) {
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::recvAskInfo() From: " << item->PeerId();
        std::cerr << std::endl;
#endif

        std::list<std::string> &idList = sendIdList[item->PeerId()];

        if (std::find(idList.begin(), idList.end(), item->gpg_id) == idList.end()) {
            idList.push_back(item->gpg_id);
        }
}

void p3disc::removeFriend(std::string ssl_id) {
#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::removeFriend() called for : " << ssl_id << std::endl;
#endif
	//if we deleted the gpg_id, don't store the friend deletion as if we add back the gpg_id, we won't have the ssl friends back
	std::string gpg_id = rsPeers->getGPGId(ssl_id);
#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::removeFriend() gpg_id : " << gpg_id << std::endl;
#endif
	if (gpg_id == AuthGPG::getAuthGPG()->getGPGOwnId() || rsPeers->isGPGAccepted(rsPeers->getGPGId(ssl_id))) {
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::removeFriend() storing the friend deletion." << ssl_id << std::endl;
#endif
		deletedSSLFriendsIds[ssl_id] = time(NULL);//just keep track of the deleted time
		IndicateConfigChanged();
	}
}

/*************************************************************************************/
/*			AuthGPGService						     */
/*************************************************************************************/
AuthGPGOperation *p3disc::getGPGOperation()
{
	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		/* process disc reply in list */
		if (pendingDiscReplyInList.empty() == false) {
			RsDiscReply *item = pendingDiscReplyInList.front();

			return new AuthGPGOperationLoadOrSave(true, item->certGPG, item);
		}
	}

	/* process disc reply out list */

	std::string destId;
	std::string srcId;

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		while (!sendIdList.empty()) {
			std::map<std::string, std::list<std::string> >::iterator sendIdIt = sendIdList.begin();

			if (!sendIdIt->second.empty() && mConnMgr->isOnline(sendIdIt->first)) {
				std::string gpgId = sendIdIt->second.front();
				sendIdIt->second.pop_front();

				destId = sendIdIt->first;
				srcId = gpgId;

				break;
			} else {
				/* peer is not online anymore ... try next */
				sendIdList.erase(sendIdIt);
			}
		}
	}

	if (!destId.empty() && !srcId.empty()) {
		RsDiscReply *item = createDiscReply(destId, srcId);
		if (item) {
			return new AuthGPGOperationLoadOrSave(false, item->aboutId, item);
		}
	}

	return NULL;
}

void p3disc::setGPGOperation(AuthGPGOperation *operation)
{
	AuthGPGOperationLoadOrSave *loadOrSave = dynamic_cast<AuthGPGOperationLoadOrSave*>(operation);
	if (loadOrSave) {
		if (loadOrSave->m_load) {
			/* search in pending in list */
			RsDiscReply *item = NULL;

			{
				RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

				std::list<RsDiscReply*>::iterator it = std::find(pendingDiscReplyInList.begin(), pendingDiscReplyInList.end(), loadOrSave->m_userdata);
				if (it != pendingDiscReplyInList.end()) {
					item = *it;
					pendingDiscReplyInList.erase(it);
				}
			}

			if (item) {
				recvPeerDetails(item, loadOrSave->m_certGpgId);
				delete item;
			}
		} else {
			RsDiscReply *item = (RsDiscReply*) loadOrSave->m_userdata;

			if (item) {
				if (loadOrSave->m_certGpg.empty()) {
#ifdef P3DISC_DEBUG
					std::cerr << "p3disc::setGPGOperation() don't send details because the gpg cert is not good" << std::endl;
#endif
					delete item;
					return;
				}

				// Send off message
				item->certGPG = loadOrSave->m_certGpg;

#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::setGPGOperation() About to Send Message:" << std::endl;
				item->print(std::cerr, 5);
#endif

				sendItem(item);

#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::cbkGPGOperationSave() discovery reply sent." << std::endl;
#endif
			}
		}
		return;
	}

	/* ignore other operations */
}

/*************************************************************************************/
/*				Storing Network Graph				     */
/*************************************************************************************/
int	p3disc::addDiscoveryData(const std::string& fromId, const std::string& aboutId,const std::string& from_gpg_id,const std::string& about_gpg_id, const struct sockaddr_in& laddr, const struct sockaddr_in& raddr, uint32_t flags, time_t ts,bool& new_info)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	new_info = false ;

	gpg_neighbors[from_gpg_id].insert(about_gpg_id) ;

#ifdef P3DISC_DEBUG
	std::cerr << "Adding discovery data " << fromId << " - " << aboutId << std::endl ;
#endif
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
		new_info = true ;
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

	if(it->second.neighbour_of.find(fromId) == it->second.neighbour_of.end())
	{
		(it->second).neighbour_of[fromId] = as;
		new_info =true ;
	}

	/* do we update network address info??? */
	return 1;

}



/*************************************************************************************/
/*			   Extracting Network Graph Details			     */
/*************************************************************************************/
bool p3disc::potentialproxies(const std::string& id, std::list<std::string> &proxyIds)
{
	/* find id -> and extract the neighbour_of ids */

	if(id == rsPeers->getOwnId()) // SSL id			// This is treated appart, because otherwise we don't receive any disc info about us
		return rsPeers->getFriendList(proxyIds) ;

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, autoneighbour>::iterator it;
	std::map<std::string, autoserver>::iterator sit;
	if (neighbours.end() == (it = neighbours.find(id)))
	{
		return false;
	}

	for(sit = it->second.neighbour_of.begin(); sit != it->second.neighbour_of.end(); sit++)
	{
		proxyIds.push_back(sit->first);
	}
	return true;
}
bool p3disc::potentialGPGproxies(const std::string& gpg_id, std::list<std::string> &proxyGPGIds)
{
	/* find id -> and extract the neighbour_of ids */

	if(gpg_id == rsPeers->getGPGOwnId()) // SSL id			// This is treated appart, because otherwise we don't receive any disc info about us
		return rsPeers->getGPGAcceptedList(proxyGPGIds) ;

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	std::map<std::string, std::set<std::string> >::iterator it = gpg_neighbors.find(gpg_id) ;

	if(it == gpg_neighbors.end())
		return false;

	for(std::set<std::string>::const_iterator sit(it->second.begin()); sit != it->second.end(); ++sit)
		proxyGPGIds.push_back(*sit);

	return true;
}

void p3disc::getversions(std::map<std::string, std::string> &versions)
{
	versions = this->versions;
}

void p3disc::getWaitingDiscCount(unsigned int *sendCount, unsigned int *recvCount)
{
	if (sendCount == NULL && recvCount == NULL) {
		/* Nothing to do */
		return;
	}

	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	if (sendCount) {
		*sendCount = 0;

		std::map<std::string, std::list<std::string> >::iterator it;
		for (it = sendIdList.begin(); it != sendIdList.end(); it++) {
			*sendCount += it->second.size();
		}
	}

	if (recvCount) {
		*recvCount = pendingDiscReplyInList.size();
	}
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
		out <<  rs_inet_ntoa(nit->second.localAddr.sin_addr);
		out << ":" << ntohs(nit->second.localAddr.sin_port) << std::endl;
		out << "-> RemoteAddr: ";
		out <<  rs_inet_ntoa(nit->second.remoteAddr.sin_addr);
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
			out <<  rs_inet_ntoa(sit->second.localAddr.sin_addr);
			out <<":"<< ntohs(sit->second.localAddr.sin_port);
			out << std::endl;
			out << "\t\tRemoteAddr: ";
			out <<  rs_inet_ntoa(sit->second.remoteAddr.sin_addr);
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

//int convertTDeltaToTRange(double tdelta)
//{
//	if (tdelta < 0)
//		return 0;
//	int trange = 1 + (int) log10(tdelta + 1.0);
//	return trange;
//
//}

// trange     -> tdelta
// -inf...0	  -1 (invalid)
//    1            8
//    2           98
//    3          998
//    4         9998
//  etc...

//int convertTRangeToTDelta(int trange)
//{
//	if (trange <= 0)
//		return -1;
//
//	return (int) (pow(10.0, trange) - 1.5); // (int) xxx98.5 -> xxx98
//}

// -----------------------------------------------------------------------------------//
// --------------------------------  Config functions  ------------------------------ //
// -----------------------------------------------------------------------------------//
//
RsSerialiser *p3disc::setupSerialiser()
{
        RsSerialiser *rss = new RsSerialiser ;
        rss->addSerialType(new RsGeneralConfigSerialiser());
        return rss ;
}

bool p3disc::saveList(bool& cleanup, std::list<RsItem*>& lst)
{
        #ifdef P3DISC_DEBUG
        std::cerr << "p3disc::saveList() called" << std::endl;
        #endif
        cleanup = true ;


        // Now save config for network digging strategies
        RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
        std::map<std::string, time_t>::iterator mapIt;
        for (mapIt = deletedSSLFriendsIds.begin(); mapIt != deletedSSLFriendsIds.end(); mapIt++) {
            RsTlvKeyValue kv;
            kv.key = mapIt->first;
            std::ostringstream time_string;
            time_string << mapIt->second;
            kv.value = time_string.str();
            vitem->tlvkvs.pairs.push_back(kv) ;
            #ifdef P3DISC_DEBUG
            std::cerr << "p3disc::saveList() saving : " << mapIt->first << " ; " << mapIt->second << std::endl ;
            #endif
        }
        lst.push_back(vitem);

        return true ;
}

bool p3disc::loadList(std::list<RsItem*>& load)
{
        #ifdef P3DISC_DEBUG
        std::cerr << "p3disc::loadList() Item Count: " << load.size() << std::endl;
        #endif

        RsStackMutex stack(mDiscMtx); /****** STACK LOCK MUTEX *******/

        /* load the list of accepted gpg keys */
        std::list<RsItem *>::iterator it;
        for(it = load.begin(); it != load.end(); it++) {
                RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet *>(*it);

                if(vitem) {
                        #ifdef P3DISC_DEBUG
                        std::cerr << "p3disc::loadList() General Variable Config Item:" << std::endl;
                        vitem->print(std::cerr, 10);
                        std::cerr << std::endl;
                        #endif

                        std::list<RsTlvKeyValue>::iterator kit;
                        for(kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); kit++) {
                            std::istringstream instream(kit->value);
                            time_t deleted_time_t;
                            instream >> deleted_time_t;
                            deletedSSLFriendsIds[kit->key] = deleted_time_t;
                        }
                }
                delete (*it);
        }
        return true;
}
