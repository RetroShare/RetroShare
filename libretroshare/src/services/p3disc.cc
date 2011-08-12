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
#include "retroshare/rspeers.h"
#include "services/p3disc.h"

#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"

#include "pqi/authssl.h"
#include "pqi/authgpg.h"

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
const uint32_t P3DISC_FLAGS_PEER_TRUSTS_ME 	= 0x0040;
const uint32_t P3DISC_FLAGS_ASK_VERSION		= 0x0080;


/*****
 * #define P3DISC_DEBUG 	1
 ****/

//#define P3DISC_DEBUG 	1

/******************************************************************************************
 ******************************  NEW DISCOVERY  *******************************************
 ******************************************************************************************
 *****************************************************************************************/

p3disc::p3disc(p3PeerMgr *pm, p3LinkMgr *lm, pqipersongrp *pqih)
        :p3Service(RS_SERVICE_TYPE_DISC),
				 p3Config(CONFIG_TYPE_P3DISC),
		  mPeerMgr(pm), mLinkMgr(lm), 
		  mPqiPersonGrp(pqih), mDiscMtx("p3disc")
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	addSerialType(new RsDiscSerialiser());

	mLastSentHeartbeatTime = time(NULL);
	mDiscEnabled = true;

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
	time_t hbTime;
	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
		hbTime = mLastSentHeartbeatTime;
	}

	if (time(NULL) - hbTime > HEARTBEAT_REPEAT_TIME) 
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::tick() sending heartbeat to all peers" << std::endl;
#endif

		std::list<std::string> peers;
		std::list<std::string>::const_iterator pit;

		mLinkMgr->getOnlineList(peers);
		for (pit = peers.begin(); pit != peers.end(); ++pit) 
		{
			sendHeartbeat(*pit);
		}

		/* check our Discovery flag */
		peerState detail;
		mPeerMgr->getOwnNetStatus(detail);

		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		mDiscEnabled = (!(detail.visState & RS_VIS_STATE_NODISC));
		mLastSentHeartbeatTime = time(NULL);
	}

	return handleIncoming();
}

int p3disc::handleIncoming()
{
	RsItem *item = NULL;

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::handleIncoming()" << std::endl;
#endif

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
		if (NULL != (dri = dynamic_cast<RsDiscReply *> (item)))	
		{
			recvDiscReply(dri);
		}
		else if (NULL != (dvi = dynamic_cast<RsDiscVersion *> (item))) 
		{
			recvPeerVersionMsg(dvi);
			nhandled++;
			delete item;
		}
		else if (NULL != (inf = dynamic_cast<RsDiscAskInfo *> (item))) /* Ping */ 
		{
			recvAskInfo(inf);
			nhandled++;
			delete item;
		}
		else if (NULL != (dta = dynamic_cast<RsDiscHeartbeat *> (item))) 
		{
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

	std::list<std::string> friendIds;
	std::list<std::string>::iterator it;
	std::set<std::string> gpgIds;
	std::set<std::string>::iterator git;

	/* We send our full friends list - if we have Discovery Enabled */
	if (mDiscEnabled)
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() Discovery Enabled, sending Friend List" << std::endl;
#endif
		mLinkMgr->getFriendList(friendIds);

		/* send them a list of all friend's details */
		for(it = friendIds.begin(); it != friendIds.end(); it++) 
		{
			/* get details */
			peerState detail;
			if (!mPeerMgr->getFriendNetStatus(*it, detail)) 
			{
				/* major error! */
#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() No Info, Skipping: " << *it;
				std::cerr << std::endl;
#endif
				continue;
			}
	
			if (!(detail.visState & RS_VIS_STATE_NODISC)) 
			{
#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() Adding GPGID: " << detail.gpg_id;
				std::cerr << " (SSLID: " << *it << ")";
				std::cerr << std::endl;
#endif
				gpgIds.insert(detail.gpg_id);
			}
			else
			{
#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() DISC OFF for GPGID: " << detail.gpg_id;
				std::cerr << " (SSLID: " << *it << ")";
				std::cerr << std::endl;
#endif
			}
		}
	}

	//add own info, this info is sent whether discovery is enabled - or not.
	gpgIds.insert(rsPeers->getGPGOwnId());

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		/* refresh with new list */
		std::list<std::string> &idList = mSendIdList[id];
		idList.clear();
		for(git = gpgIds.begin(); git != gpgIds.end(); git++)
		{
			idList.push_back(*git);
		}
	}

        #ifdef P3DISC_DEBUG
	std::cerr << "p3disc::sendAllInfoToJustConnectedPeer() finished." << std::endl;
        #endif
}

void p3disc::sendJustConnectedPeerInfoToAllPeer(const std::string &connectedPeerId)
{

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() connectedPeerId : " << connectedPeerId << std::endl;
#endif

	/* only ask info if discovery is on */
	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
		if (!mDiscEnabled)
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() Disc Disabled => NULL OP" << std::endl;
#endif
			return;
		}
	}

	/* get details */
	peerState detail;
	if (!mPeerMgr->getFriendNetStatus(connectedPeerId, detail)) 
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() No NetStatus => FAILED" << std::endl;
#endif
		return;
	}

	if (detail.visState & RS_VIS_STATE_NODISC)
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() Peer Disc Discable => NULL OP" << std::endl;
#endif
		return;
	}


	std::string gpg_connectedPeerId = rsPeers->getGPGId(connectedPeerId);
	std::list<std::string> onlineIds;

	/* get a peer lists */
	rsPeers->getOnlineList(onlineIds);

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		/* append gpg id's of all friend's to the sending list */

		std::list<std::string>::iterator it;
		for (it = onlineIds.begin(); it != onlineIds.end(); it++) 
		{
			std::list<std::string> &idList = mSendIdList[*it];

			if (std::find(idList.begin(), idList.end(), gpg_connectedPeerId) == idList.end()) 
			{
#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() adding to queue for: ";
				std::cerr << *it << std::endl;
#endif
				idList.push_back(gpg_connectedPeerId);
			}
			else
			{
#ifdef P3DISC_DEBUG
				std::cerr << "p3disc::sendJustConnectedPeerInfoToAllPeer() already in queue for: ";
				std::cerr << *it << std::endl;
#endif
			}
		}
	}
}


bool    isDummyFriend(const std::string &id) 
{
        bool ret = (id.substr(0,5) == "dummy");
	return ret;
}

 /* (dest (to), source (cert)) */
RsDiscReply *p3disc::createDiscReply(const std::string &to, const std::string &about)
{

#ifdef P3DISC_DEBUG
	std::cerr << "p3disc::createDiscReply() called. Sending details of: " << about << " to: " << to << std::endl;
#endif

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
	rsPeers->getAssociatedSSLIds(aboutGpgId, sslChilds);
	bool shouldWeSendGPGKey = false;//the GPG key is send only if we've got a valid friend with DISC enabled

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/
		if (!mDiscEnabled)
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::createDiscReply() Disc Disabled, removing all friend SSL Ids";
			std::cerr << std::endl;
#endif
			sslChilds.clear();
		}
	}

	std::list<std::string>::iterator it;
	for (it = sslChilds.begin(); it != sslChilds.end(); it++) 
	{
		/* skip dummy ones - until they are removed fully */
		if (isDummyFriend(*it))
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::createDiscReply() Skipping Dummy Child SSL Id:" << *it;
			std::cerr << std::endl;
#endif
			continue;
		}

		peerState detail;
		if (!mPeerMgr->getFriendNetStatus(*it, detail) 
			|| detail.visState & RS_VIS_STATE_NODISC)
		{
#ifdef P3DISC_DEBUG
			std::cerr << "p3disc::createDiscReply() Skipping cos No Details or NODISC flag id: " << *it;
			std::cerr << std::endl;
#endif
			continue;
		}

#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::createDiscReply() Found Child SSL Id:" << *it;
		std::cerr << std::endl;
#endif

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
		rsPeerNetItem.currentlocaladdr = detail.localaddr;
		rsPeerNetItem.currentremoteaddr = detail.serveraddr;
		rsPeerNetItem.dyndns = detail.dyndns;
		detail.ipAddrs.mLocal.loadTlv(rsPeerNetItem.localAddrList);
		detail.ipAddrs.mExt.loadTlv(rsPeerNetItem.extAddrList);


		di->rsPeerList.push_back(rsPeerNetItem);
	}


	//send own details
	if (about == rsPeers->getGPGOwnId()) 
	{
#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::createDiscReply() Adding Own Id Details";
		std::cerr << std::endl;
#endif
		peerState detail;
		if (mPeerMgr->getOwnNetStatus(detail)) 
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
			rsPeerNetItem.currentlocaladdr = detail.localaddr;
			rsPeerNetItem.currentremoteaddr = detail.serveraddr;
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

	/* only ask info if discovery is on */
	if (!mDiscEnabled)
	{
#ifdef P3DISC_DEBUG
	        std::cerr << "p3disc::sendOwnVersion() Disc Disabled => NULL OP" << std::endl;
#endif
		return;
	}


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
                std::ostringstream out;
                out << "p3disc::sendHeartbeat()";
                out << " to : " << to;
#ifdef P3DISC_DEBUG
                std::cerr << out.str() << std::endl;
#endif
		rslog(RSL_WARNING, pqidisczone, out.str());
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

	// We Still Ask even if Disc isn't Enabled... if they want to give us the info ;)


        peerState connectState;
        if (!mPeerMgr->getFriendNetStatus(about, connectState) || (connectState.visState & RS_VIS_STATE_NODISC)) 
		{
#ifdef P3DISC_DEBUG
            std::cerr << "p3disc::askInfoToAllPeers() friend disc is off, don't send the request." << std::endl;
#endif
            return;
        }

        std::string aboutGpgId = rsPeers->getGPGId(about);
        if (aboutGpgId == "") 
		{
#ifdef P3DISC_DEBUG
            std::cerr << "p3disc::askInfoToAllPeers() no gpg id found" << std::endl;
#endif
        }

        std::list<std::string> onlineIds;
        std::list<std::string>::iterator it;

        rsPeers->getOnlineList(onlineIds);

        /* ask info to trusted friends */
        for(it = onlineIds.begin(); it != onlineIds.end(); it++) 
		{
                RsDiscAskInfo *di = new RsDiscAskInfo();
                di->PeerId(*it);
                di->gpg_id = aboutGpgId; 
                sendItem(di);
#ifdef P3DISC_DEBUG
                std::cerr << "p3disc::askInfoToAllPeers() question sent to : " << *it << std::endl;
#endif
        }
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::askInfoToAllPeers() finished." << std::endl;
#endif
}

void p3disc::recvPeerDetails(RsDiscReply *item, const std::string &certGpgId)
{

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

	std::list<RsPeerNetItem>::iterator pit;
	for (pit = item->rsPeerList.begin(); pit != item->rsPeerList.end(); pit++) 
	{
		if(isDummyFriend(pit->pid)) 
		{
			continue;
		}

		bool new_info = false;
		addDiscoveryData(item->PeerId(), pit->pid,rsPeers->getGPGId(item->PeerId()),
			item->aboutId, pit->currentlocaladdr, pit->currentremoteaddr, 0, time(NULL),new_info);

		if(new_info)
			should_notify_discovery = true ;

#ifdef P3DISC_DEBUG
		std::cerr << "p3disc::recvPeerFriendMsg() Peer Config Item:" << std::endl;

		pit->print(std::cerr, 10);
		std::cerr << std::endl;
#endif
		if (pit->pid != rsPeers->getOwnId())
		{
			// Apparently, the connect manager won't add a friend if the gpg id is not
			// trusted. However, this should be tested here for consistency and security 
			// in case of modifications in mConnMgr.
			//

			// Check if already friend.
			if(AuthGPG::getAuthGPG()->isGPGAccepted(pit->gpg_id) ||  pit->gpg_id == AuthGPG::getAuthGPG()->getGPGOwnId())
			{
				if (!mPeerMgr->isFriend(pit->pid))
				{
					// Add with no disc by default. If friend already exists, it will do nothing
					// NO DISC is important - otherwise, we'll just enter a nasty loop, 
					// where every addition triggers requests, then they are cleaned up, and readded...

					// This way we get their addresses, but don't advertise them until we get a
					// connection.
#ifdef P3DISC_DEBUG
					std::cerr << "--> Adding to friends list " << pit->pid << " - " << pit->gpg_id << std::endl;
#endif
					mPeerMgr->addFriend(pit->pid, pit->gpg_id, pit->netMode, RS_VIS_STATE_NODISC, 0); 
				}
			}

			/* skip if not one of our peers */
			if (!mPeerMgr->isFriend(pit->pid))
			{
				continue;
			}

			if (item->PeerId() == pit->pid) 
			{
#ifdef P3DISC_DEBUG
				std::cerr << "Info sent by the peer itself -> updating self info:" << std::endl;
				std::cerr << "  -> current local  addr = " << pit->currentlocaladdr << std::endl;
				std::cerr << "  -> current remote addr = " << pit->currentremoteaddr << std::endl;
				std::cerr << "  -> visState            =  " << std::hex << pit->visState << std::dec;
				std::cerr << "   -> network mode: " << pit->netMode << std::endl;
				std::cerr << "   ->     location: " << pit->location << std::endl;
				std::cerr << std::endl;
#endif

				// When the peer sends his own list of IPs, the info replaces the existing info, because the
				// peer is the primary source of his own IPs.
				mPeerMgr->setNetworkMode(pit->pid, pit->netMode);
				mPeerMgr->setLocation(pit->pid, pit->location);
				mPeerMgr->setLocalAddress(pit->pid, pit->currentlocaladdr);
				mPeerMgr->setExtAddress(pit->pid,   pit->currentremoteaddr);
				mPeerMgr->setVisState(pit->pid, pit->visState); 

				if (pit->dyndns != "") 
					mPeerMgr->setDynDNS(pit->pid, pit->dyndns);
			}

			// always update historical address list... this should be enough to let us connect.

			pqiIpAddrSet addrsFromPeer;	
			addrsFromPeer.mLocal.extractFromTlv(pit->localAddrList);
			addrsFromPeer.mExt.extractFromTlv(pit->extAddrList);

#ifdef P3DISC_DEBUG
			std::cerr << "Setting address list to peer " << pit->pid << ", to be:" << std::endl ;

			addrsFromPeer.printAddrs(std::cerr);
			std::cerr << std::endl;
#endif
			mPeerMgr->updateAddressList(pit->pid, addrsFromPeer);

		}
#ifdef P3DISC_DEBUG
		else
		{
			std::cerr << "Skipping info about own id " << pit->pid << std::endl ;
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

void p3disc::recvAskInfo(RsDiscAskInfo *item) 
{
#ifdef P3DISC_DEBUG
        std::cerr << "p3disc::recvAskInfo() From: " << item->PeerId();
        std::cerr << std::endl;
#endif
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	/* only provide info if discovery is on */
	if (!mDiscEnabled)
	{
#ifdef P3DISC_DEBUG
        	std::cerr << "p3disc::recvAskInfo() Disc Disabled => NULL OP";
        	std::cerr << std::endl;
#endif
		return;
	}

        std::list<std::string> &idList = mSendIdList[item->PeerId()];

        if (std::find(idList.begin(), idList.end(), item->gpg_id) == idList.end()) {
            idList.push_back(item->gpg_id);
        }
}

void p3disc::recvDiscReply(RsDiscReply *dri)
{
	RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

	/* search pending item and remove it, when already exist */
	std::list<RsDiscReply*>::iterator it;
	for (it = mPendingDiscReplyInList.begin(); it != mPendingDiscReplyInList.end(); it++) 
	{
		if ((*it)->PeerId() == dri->PeerId() && (*it)->aboutId == dri->aboutId) 
		{
			delete (*it);
			mPendingDiscReplyInList.erase(it);
			break;
		}
	}

	// add item to list for later process
	mPendingDiscReplyInList.push_back(dri); // no delete
}




void p3disc::removeFriend(std::string /*ssl_id*/)
{

// DON'T KNOW WHY SSL IDS were saved -> the results are never used
#if 0

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

#endif

}

/*************************************************************************************/
/*			AuthGPGService						     */
/*************************************************************************************/
AuthGPGOperation *p3disc::getGPGOperation()
{
	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		/* process disc reply in list */
		if (mPendingDiscReplyInList.empty() == false) {
			RsDiscReply *item = mPendingDiscReplyInList.front();

			return new AuthGPGOperationLoadOrSave(true, item->certGPG, item);
		}
	}

	/* process disc reply out list */

	std::string destId;
	std::string srcId;

	{
		RsStackMutex stack(mDiscMtx); /********** STACK LOCKED MTX ******/

		while (!mSendIdList.empty()) 
		{
			std::map<std::string, std::list<std::string> >::iterator it = mSendIdList.begin();

			if (!it->second.empty() && mLinkMgr->isOnline(it->first)) {
				std::string gpgId = it->second.front();
				it->second.pop_front();

				destId = it->first;
				srcId = gpgId;

				break;
			} else {
				/* peer is not online anymore ... try next */
				mSendIdList.erase(it);
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

				std::list<RsDiscReply*>::iterator it;
				it = std::find(mPendingDiscReplyInList.begin(), mPendingDiscReplyInList.end(), loadOrSave->m_userdata);
				if (it != mPendingDiscReplyInList.end()) {
					item = *it;
					mPendingDiscReplyInList.erase(it);
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
		for (it = mSendIdList.begin(); it != mSendIdList.end(); it++) {
			*sendCount += it->second.size();
		}
	}

	if (recvCount) {
		*recvCount = mPendingDiscReplyInList.size();
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

bool p3disc::saveList(bool& cleanup, std::list<RsItem*>& /*lst*/)
{
        #ifdef P3DISC_DEBUG
        std::cerr << "p3disc::saveList() called" << std::endl;
        #endif
        cleanup = true ;

// DON'T KNOW WHY SSL IDS were saved -> the results are never used
#if 0
        // Now save config for network digging strategies
        RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;
        std::map<std::string, time_t>::iterator mapIt;
        for (mapIt = deletedSSLFriendsIds.begin(); mapIt != deletedSSLFriendsIds.end(); mapIt++) 
		{
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
#endif

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
        for(it = load.begin(); it != load.end(); it++) 
		{
// DON'T KNOW WHY SSL IDS were saved -> the results are never used
#if 0
                RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet *>(*it);

                if(vitem) 
				{
                        #ifdef P3DISC_DEBUG
                        std::cerr << "p3disc::loadList() General Variable Config Item:" << std::endl;
                        vitem->print(std::cerr, 10);
                        std::cerr << std::endl;
                        #endif

                        std::list<RsTlvKeyValue>::iterator kit;
                        for(kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); kit++) 
						{
                            std::istringstream instream(kit->value);
                            time_t deleted_time_t;
                            instream >> deleted_time_t;
                            deletedSSLFriendsIds[kit->key] = deleted_time_t;
                        }
                }
#endif

                delete (*it);
        }
        return true;
} 
