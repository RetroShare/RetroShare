/*
 * libretroshare/src/pqi: p3peermgr.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2007-2011 by Robert Fernie.
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

#include "util/rsnet.h"
#include "pqi/authgpg.h"
#include "pqi/authssl.h"

#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"
#include "pqi/p3historymgr.h"

//#include "pqi/p3dhtmgr.h" // Only need it for constants.
//#include "tcponudp/tou.h"
//#include "util/extaddrfinder.h"
//#include "util/dnsresolver.h"

#include "util/rsprint.h"
#include "util/rsstring.h"
#include "util/rsdebug.h"
const int p3peermgrzone = 9531;

#include "serialiser/rsconfigitems.h"
#include "pqi/pqinotify.h"

#include "retroshare/rsiface.h" // Needed for rsicontrol (should remove this dependancy)
#include "retroshare/rspeers.h" // Needed for Group Parameters.

/* Network setup States */

const uint32_t RS_NET_NEEDS_RESET = 	0x0000;
const uint32_t RS_NET_UNKNOWN = 	0x0001;
const uint32_t RS_NET_UPNP_INIT = 	0x0002;
const uint32_t RS_NET_UPNP_SETUP =  	0x0003;
const uint32_t RS_NET_EXT_SETUP =  	0x0004;
const uint32_t RS_NET_DONE =    	0x0005;
const uint32_t RS_NET_LOOPBACK =    	0x0006;
const uint32_t RS_NET_DOWN =    	0x0007;

const uint32_t MIN_TIME_BETWEEN_NET_RESET = 		5;

const uint32_t PEER_IP_CONNECT_STATE_MAX_LIST_SIZE =     	4;

#define VERY_OLD_PEER  (90 * 24 * 3600)      // 90 days.

/****
 * #define PEER_DEBUG 1
 ***/

#define MAX_AVAIL_PERIOD 230 //times a peer stay in available state when not connected
#define MIN_RETRY_PERIOD 140

void  printConnectState(std::ostream &out, peerState &peer);

peerState::peerState()
	:id("unknown"), 
         gpg_id("unknown"),
	 netMode(RS_NET_MODE_UNKNOWN), visState(RS_VIS_STATE_STD), lastcontact(0) 
{
        sockaddr_clear(&localaddr);
        sockaddr_clear(&serveraddr);

	return;
}

std::string textPeerConnectState(peerState &state)
{
	std::string out = "Id: " + state.id + "\n";
	rs_sprintf_append(out, "NetMode: %lu\n", state.netMode);
	rs_sprintf_append(out, "VisState: %lu\n", state.visState);
	rs_sprintf_append(out, "laddr: %s:%u\n", rs_inet_ntoa(state.localaddr.sin_addr).c_str(), ntohs(state.localaddr.sin_port));
	rs_sprintf_append(out, "eaddr: %s:%u\n", rs_inet_ntoa(state.serveraddr.sin_addr).c_str(), ntohs(state.serveraddr.sin_port));

	return out;
}


p3PeerMgrIMPL::p3PeerMgrIMPL(	const std::string& ssl_own_id,
										const std::string& gpg_own_id,
										const std::string& gpg_own_name,
										const std::string& ssl_own_location)
	:p3Config(CONFIG_TYPE_PEERS), mPeerMtx("p3PeerMgr"), mStatusChanged(false)
{

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		mLinkMgr = NULL;
		mNetMgr = NULL;

		/* setup basics of own state */
		mOwnState.id = ssl_own_id ;
		mOwnState.gpg_id = gpg_own_id ;
		mOwnState.name = gpg_own_name ;
		mOwnState.location = ssl_own_location ;
		mOwnState.netMode = RS_NET_MODE_UPNP; // Default to UPNP.
		mOwnState.visState = 0;
	
		lastGroupId = 1;


	}
	
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgr() Startup" << std::endl;
#endif


	return;
}

void    p3PeerMgrIMPL::setManagers(p3LinkMgrIMPL *linkMgr, p3NetMgrIMPL *netMgr)
{
	mLinkMgr = linkMgr;
	mNetMgr = netMgr;
}

void p3PeerMgrIMPL::setOwnNetworkMode(uint32_t netMode)
{
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::setOwnNetworkMode() :";
		std::cerr << " Existing netMode: " << mOwnState.netMode;
		std::cerr << " Input netMode: " << netMode;
		std::cerr << std::endl;
#endif

		mOwnState.netMode = (netMode & RS_NET_MODE_ACTUAL);

		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}
	
	// Pass on Flags to NetMgr.
	mNetMgr->setNetworkMode((netMode & RS_NET_MODE_ACTUAL));
}

void p3PeerMgrIMPL::setOwnVisState(uint32_t visState)
{
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		std::string out;
		rs_sprintf(out, "p3PeerMgr::setOwnVisState() Existing vis: %lu Input vis: %lu", mOwnState.visState, visState);
		rslog(RSL_WARNING, p3peermgrzone, out);

#ifdef PEER_DEBUG
		std::cerr << out.str() << std::endl;
#endif

		mOwnState.visState = visState;

		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}
	
	// Pass on Flags to NetMgr.
	mNetMgr->setVisState(visState);
}


void p3PeerMgrIMPL::tick()
{

	static const time_t INTERVAL_BETWEEN_LOCATION_CLEANING = 1860 ; // Remove unused locations every 31 minutes.
	static time_t last_friends_check = time(NULL) + INTERVAL_BETWEEN_LOCATION_CLEANING; // first cleaning after 1 hour.

	time_t now = time(NULL) ;

	if(now - last_friends_check > INTERVAL_BETWEEN_LOCATION_CLEANING)
	{
		std::cerr << "p3PeerMgrIMPL::tick(): cleaning unused locations." << std::endl ;

        	rslog(RSL_WARNING, p3peermgrzone, "p3PeerMgr::tick() removeUnusedLocations()");

		removeUnusedLocations() ;
		last_friends_check = now ;
	}
}


/********************************  Network Status  *********************************
 * Configuration Loading / Saving.
 */


const std::string p3PeerMgrIMPL::getOwnId()
{
                return AuthSSL::getAuthSSL()->OwnId();
}


bool p3PeerMgrIMPL::getOwnNetStatus(peerState &state)
{
        RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
        state = mOwnState;
	return true;
}

bool p3PeerMgrIMPL::isFriend(const std::string &id)
{
#ifdef PEER_DEBUG_COMMON
                std::cerr << "p3PeerMgrIMPL::isFriend(" << id << ") called" << std::endl;
#endif
        RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
        bool ret = (mFriendList.end() != mFriendList.find(id));
#ifdef PEER_DEBUG_COMMON
                std::cerr << "p3PeerMgrIMPL::isFriend(" << id << ") returning : " << ret << std::endl;
#endif
        return ret;
}

bool    p3PeerMgrIMPL::getPeerName(const std::string &ssl_id, std::string &name)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<std::string, peerState>::iterator it;
	it = mFriendList.find(ssl_id);
	if (it == mFriendList.end())
	{
		return false;
	}

	name = it->second.name + " (" + it->second.location + ")";
	return true;
}

bool    p3PeerMgrIMPL::getGpgId(const std::string &ssl_id, std::string &gpgId)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<std::string, peerState>::iterator it;
	it = mFriendList.find(ssl_id);
	if (it == mFriendList.end())
	{
		return false;
	}

	gpgId = it->second.gpg_id;
	return true;
}

// Placeholder until we implement this functionality.
uint32_t p3PeerMgrIMPL::getConnectionType(const std::string &/*sslId*/)
{
	return RS_NET_CONN_TYPE_FRIEND;
}

int p3PeerMgrIMPL::getFriendCount(bool ssl, bool online)
{
	if (online) {
		// count only online id's
		std::list<std::string> onlineIds;
		mLinkMgr->getOnlineList(onlineIds);

		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		std::set<std::string> gpgIds;
		int count = 0;

		std::map<std::string, peerState>::iterator it;
		for(it = mFriendList.begin(); it != mFriendList.end(); ++it) {
			if (online && std::find(onlineIds.begin(), onlineIds.end(), it->first) == onlineIds.end()) {
				continue;
			}
			if (ssl) {
				// count ssl id's only
				count++;
			} else {
				// count unique gpg id's
				gpgIds.insert(it->second.gpg_id);
			}
		}

		return ssl ? count : gpgIds.size();
	}

	if (ssl) {
		// count all ssl id's
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
		return mFriendList.size();
	}

	// count all gpg id's
	std::list<std::string> gpgIds;
	AuthGPG::getAuthGPG()->getGPGAcceptedList(gpgIds);

	// add own gpg id, if we have more than one location
	std::list<std::string> ownSslIds;
	getAssociatedPeers(AuthGPG::getAuthGPG()->getGPGOwnId(), ownSslIds);

	return gpgIds.size() + ((ownSslIds.size() > 0) ? 1 : 0);
}

bool p3PeerMgrIMPL::getFriendNetStatus(const std::string &id, peerState &state)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<std::string, peerState>::iterator it;
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
		return false;
	}

	state = it->second;
	return true;
}


bool p3PeerMgrIMPL::getOthersNetStatus(const std::string &id, peerState &state)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<std::string, peerState>::iterator it;
	it = mOthersList.find(id);
	if (it == mOthersList.end())
	{
		return false;
	}

	state = it->second;
	return true;
}

#if 0

void p3PeerMgrIMPL::getFriendList(std::list<std::string> &peers)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
        std::map<std::string, peerState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		peers.push_back(it->first);
	}
	return;
}

#endif

#if 0
void p3PeerMgrIMPL::getOthersList(std::list<std::string> &peers)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
        std::map<std::string, peerState>::iterator it;
	for(it = mOthersList.begin(); it != mOthersList.end(); it++)
	{
		peers.push_back(it->first);
	}
	return;
}
#endif



int p3PeerMgrIMPL::getConnectAddresses(const std::string &id, 
					struct sockaddr_in &lAddr, struct sockaddr_in &eAddr, 
					pqiIpAddrSet &histAddrs, std::string &dyndns)
{

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	
	/* check for existing */
	std::map<std::string, peerState>::iterator it;
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
		/* ERROR */
		std::cerr << "p3PeerMgrIMPL::getConnectAddresses() ERROR unknown Peer";
		std::cerr << std::endl;
		return 0;
	}
	
	lAddr = it->second.localaddr;
	eAddr = it->second.serveraddr;
	histAddrs = it->second.ipAddrs;
	dyndns = it->second.dyndns;

	return 1;
}



bool    p3PeerMgrIMPL::haveOnceConnected()
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
        std::map<std::string, peerState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		if (it->second.lastcontact > 0)
		{
#ifdef PEER_DEBUG
 			std::cerr << "p3PeerMgrIMPL::haveOnceConnected() lastcontact: ";
			std::cerr << time(NULL) - it->second.lastcontact << " for id: " << it->first;
			std::cerr << std::endl;
#endif

			return true;
		}
	}

#ifdef PEER_DEBUG
 	std::cerr << "p3PeerMgrIMPL::haveOnceConnected() all Last Contacts = 0";
	std::cerr << std::endl;
#endif

	return false;
}



/*******************************************************************/
/*******************************************************************/

bool p3PeerMgrIMPL::addFriend(const std::string& input_id, const std::string& input_gpg_id, uint32_t netMode, uint32_t visState, time_t lastContact,ServicePermissionFlags service_flags)
{
	bool notifyLinkMgr = false;
	std::string id = input_id ;
	std::string gpg_id = input_gpg_id ;

	rslog(RSL_WARNING, p3peermgrzone, "p3PeerMgr::addFriend() id: " + id);

	// For safety, make sure ssl_id is lower case and GPG id is upper case.
	//
	for(uint32_t i=0;i<id.length();++i)
		if(id[i] >= 'A' && id[i] <= 'F')
			id[i] += 'a' - 'A' ;

	for(uint32_t i=0;i<gpg_id.length();++i)
		if(gpg_id[i] >= 'a' && gpg_id[i] <= 'f')
			gpg_id[i] += 'A' - 'a' ;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/


		if (id == AuthSSL::getAuthSSL()->OwnId()) 
		{
#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::addFriend() cannot add own id as a friend." << std::endl;
#endif
			/* (1) already exists */
			return false;
		}
		/* so four possibilities
		 * (1) already exists as friend -> do nothing.
		 * (2) is in others list -> move over.
		 * (3) is non-existant -> create new one.
		 */

#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::addFriend() " << id << "; gpg_id : " << gpg_id << std::endl;
#endif

		std::map<std::string, peerState>::iterator it;
		if (mFriendList.end() != mFriendList.find(id))
		{
#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::addFriend() Already Exists" << std::endl;
#endif
			/* (1) already exists */
			return true;
		}

		//Authentication is now tested at connection time, we don't store the ssl cert anymore
		//
		if (!AuthGPG::getAuthGPG()->isGPGAccepted(gpg_id) &&  gpg_id != AuthGPG::getAuthGPG()->getGPGOwnId())
		{
#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::addFriend() gpg is not accepted" << std::endl;
#endif
			/* no auth */
			return false;
		}


		/* check if it is in others */
		if (mOthersList.end() != (it = mOthersList.find(id)))
		{
			/* (2) in mOthersList -> move over */
#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::addFriend() Move from Others" << std::endl;
#endif

			mFriendList[id] = it->second;
			mOthersList.erase(it);

			it = mFriendList.find(id);

			/* setup connectivity parameters */
			it->second.visState = visState;
			it->second.netMode  = netMode;
			it->second.lastcontact = lastContact;

			mStatusChanged = true;

			notifyLinkMgr = true;

			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}
		else
		{
#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::addFriend() Creating New Entry" << std::endl;
#endif

			/* create a new entry */
			peerState pstate;

			pstate.id = id;
			pstate.gpg_id = gpg_id;
			pstate.name = AuthGPG::getAuthGPG()->getGPGName(gpg_id);
			
			pstate.visState = visState;
			pstate.netMode = netMode;
			pstate.lastcontact = lastContact;

			/* addr & timestamps -> auto cleared */

			mFriendList[id] = pstate;

			mStatusChanged = true;

			notifyLinkMgr = true;

			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}
	}

	if (notifyLinkMgr)
	{
		mLinkMgr->addFriend(id, !(visState & RS_VIS_STATE_NODHT));
	}

	service_flags &= servicePermissionFlags(gpg_id) ; // Always reduce the permissions. 
	setServicePermissionFlags(gpg_id,service_flags) ;

#ifdef PEER_DEBUG
	printPeerLists(std::cerr);
	mLinkMgr->printPeerLists(std::cerr);
#endif
	
	return true;
}


bool p3PeerMgrIMPL::removeFriend(const std::string &id, bool removePgpId)
{

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::removeFriend() for id : " << id << std::endl;
	std::cerr << "p3PeerMgrIMPL::removeFriend() mFriendList.size() : " << mFriendList.size() << std::endl;
#endif

        rslog(RSL_WARNING, p3peermgrzone, "p3PeerMgr::removeFriend() id: " + id);

	std::list<std::string> sslid_toRemove; // This is a list of SSLIds.
	std::list<std::string> pgpid_toRemove; // This is a list of SSLIds.

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		/* move to othersList */
		bool success = false;
		std::map<std::string, peerState>::iterator it;
		//remove ssl and gpg_ids
		for(it = mFriendList.begin(); it != mFriendList.end(); it++)
		{
			if (it->second.id == id || it->second.gpg_id == id) {
#ifdef PEER_DEBUG
				std::cerr << "p3PeerMgrIMPL::removeFriend() friend found in the list." << id << std::endl;
#endif
				peerState peer = it->second;

				sslid_toRemove.push_back(it->second.id);
				if(removePgpId)
					pgpid_toRemove.push_back(it->second.gpg_id);

				mOthersList[id] = peer;
				mStatusChanged = true;

				success = true;
			}
		}

		std::list<std::string>::iterator rit;
		for(rit = sslid_toRemove.begin(); rit != sslid_toRemove.end(); rit++) 
			if (mFriendList.end() != (it = mFriendList.find(*rit))) 
				mFriendList.erase(it);

		std::map<std::string,ServicePermissionFlags>::iterator it2 ;

		for(rit = pgpid_toRemove.begin(); rit != pgpid_toRemove.end(); rit++) 
			if (mFriendsPermissionFlags.end() != (it2 = mFriendsPermissionFlags.find(*rit))) 
				mFriendsPermissionFlags.erase(it2);

#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::removeFriend() new mFriendList.size() : " << mFriendList.size() << std::endl;
#endif
	}

	std::list<std::string>::iterator rit;
	for(rit = sslid_toRemove.begin(); rit != sslid_toRemove.end(); rit++) 
	{
		mLinkMgr->removeFriend(*rit);
	}

	/* remove id from all groups */
	std::list<std::string> peerIds;
	peerIds.push_back(id);

	assignPeersToGroup("", peerIds, false);

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

#ifdef PEER_DEBUG
	printPeerLists(std::cerr);
	mLinkMgr->printPeerLists(std::cerr);
#endif

        return !sslid_toRemove.empty();
}


void p3PeerMgrIMPL::printPeerLists(std::ostream &out)
{
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		out << "p3PeerMgrIMPL::printPeerLists() Friend List";
		out << std::endl;


		std::map<std::string, peerState>::iterator it;
		for(it = mFriendList.begin(); it != mFriendList.end(); it++)
		{
			out << "\t SSL ID: " << it->second.id;
			out << "\t GPG ID: " << it->second.gpg_id;
			out << std::endl;
		}

		out << "p3PeerMgrIMPL::printPeerLists() Others List";
		out << std::endl;
		for(it = mOthersList.begin(); it != mOthersList.end(); it++)
		{
			out << "\t SSL ID: " << it->second.id;
			out << "\t GPG ID: " << it->second.gpg_id;
			out << std::endl;
		}
	}

	return;
}



#if 0
bool p3PeerMgrIMPL::addNeighbour(std::string id)
{

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::addNeighbour() not implemented anymore." << id << std::endl;
#endif

	/* so three possibilities 
	 * (1) already exists as friend -> do nothing.
	 * (2) already in others list -> do nothing.
	 * (3) is non-existant -> create new one.
	 */

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

        std::map<std::string, peerState>::iterator it;
	if (mFriendList.end() == mFriendList.find(id))
	{
		/* (1) already exists */
		return false;
	}

	if (mOthersList.end() == mOthersList.find(id))
	{
		/* (2) already exists */
		return true;
	}

	/* check with the AuthMgr if its valid */
        if (!AuthSSL::getAuthSSL()->isAuthenticated(id))
	{
		/* no auth */
		return false;
	}

	/* get details from AuthMgr */
        sslcert detail;
        if (!AuthSSL::getAuthSSL()->getCertDetails(id, detail))
	{
		/* no details */
		return false;
	}

	/* create a new entry */
	peerState pstate;

	pstate.id = id;
        pstate.name = detail.name;

	pstate.state = 0;
	pstate.actions = 0; //RS_PEER_NEW;
	pstate.visState = RS_VIS_STATE_STD;
	pstate.netMode = RS_NET_MODE_UNKNOWN;

	/* addr & timestamps -> auto cleared */
	mOthersList[id] = pstate;

        return false;
}

#endif

/*******************************************************************/
/*******************************************************************/


/**********************************************************************
 **********************************************************************
 ******************** External Setup **********************************
 **********************************************************************
 **********************************************************************/


/* This function should only be called from NetMgr,
 * as it doesn't call back to there.
 */

bool 	p3PeerMgrIMPL::UpdateOwnAddress(const struct sockaddr_in &localAddr, const struct sockaddr_in &extAddr)
{
	std::cerr << "p3PeerMgrIMPL::UpdateOwnAddress(";
	std::cerr << rs_inet_ntoa(localAddr.sin_addr) << ":" << htons(localAddr.sin_port);
	std::cerr << ", ";
	std::cerr << rs_inet_ntoa(extAddr.sin_addr) << ":" << htons(extAddr.sin_port);
	std::cerr << ")" << std::endl;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		//update ip address list
		pqiIpAddress ipAddressTimed;
		ipAddressTimed.mAddr = localAddr;
		ipAddressTimed.mSeenTime = time(NULL);
		ipAddressTimed.mSrc = 0 ;
		mOwnState.ipAddrs.updateLocalAddrs(ipAddressTimed);

		mOwnState.localaddr = localAddr;
	}


	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		//update ip address list
		pqiIpAddress ipAddressTimed;
		ipAddressTimed.mAddr = extAddr;
		ipAddressTimed.mSeenTime = time(NULL);
		ipAddressTimed.mSrc = 0 ;
		mOwnState.ipAddrs.updateExtAddrs(ipAddressTimed);

		/* Attempted Fix to MANUAL FORWARD Mode....
		 * don't update the server address - if we are in this mode
		 *
		 * It is okay - if they get it wrong, as we put the address in the address list anyway.
		 * This should keep people happy, and allow for misconfiguration!
		 */

 		if (mOwnState.netMode & RS_NET_MODE_TRY_EXT)
		{
			/**** THIS CASE SHOULD NOT BE TRIGGERED ****/
			std::cerr << "p3PeerMgrIMPL::UpdateOwnAddress() Disabling Update of Server Port ";
			std::cerr << " as MANUAL FORWARD Mode (ERROR - SHOULD NOT BE TRIGGERED: TRY_EXT_MODE)";
			std::cerr << std::endl;
			std::cerr << "Address is Now: ";
			std::cerr << rs_inet_ntoa(mOwnState.serveraddr.sin_addr);
			std::cerr << ":" << htons(mOwnState.serveraddr.sin_port);
			std::cerr << std::endl;
		}
        	else if (mOwnState.netMode & RS_NET_MODE_EXT)
		{
			mOwnState.serveraddr.sin_addr.s_addr = extAddr.sin_addr.s_addr;
			std::cerr << "p3PeerMgrIMPL::UpdateOwnAddress() Disabling Update of Server Port ";
			std::cerr << " as MANUAL FORWARD Mode";
			std::cerr << std::endl;
			std::cerr << "Address is Now: ";
			std::cerr << rs_inet_ntoa(mOwnState.serveraddr.sin_addr);
			std::cerr << ":" << htons(mOwnState.serveraddr.sin_port);
			std::cerr << std::endl;
		}
		else
		{
			mOwnState.serveraddr = extAddr;
		}
	}

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	mLinkMgr->setLocalAddress(localAddr);

	return true;
}




bool    p3PeerMgrIMPL::setLocalAddress(const std::string &id, struct sockaddr_in addr)
{
	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		bool changed = false;
		{
			RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
			if (mOwnState.localaddr.sin_addr.s_addr != addr.sin_addr.s_addr ||
			    mOwnState.localaddr.sin_port != addr.sin_port)
			{
				changed = true;
			}

			mOwnState.localaddr = addr;
		}

		if (changed)
		{
			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
			
			mNetMgr->setLocalAddress(addr);
			mLinkMgr->setLocalAddress(addr);
		}
		return true;
	}

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	/* check if it is a friend */
	std::map<std::string, peerState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::setLocalAddress() cannot add addres info : peer id not found in friend list  id: " << id << std::endl;
#endif
			return false;
		}
	}

	/* "it" points to peer */
	it->second.localaddr = addr;

#if 0
	//update ip address list
	IpAddressTimed ipAddressTimed;
	ipAddressTimed.ipAddr = addr;
	ipAddressTimed.seenTime = time(NULL);
	it->second.updateIpAddressList(ipAddressTimed);
#endif

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return true;
}

bool    p3PeerMgrIMPL::setExtAddress(const std::string &id, struct sockaddr_in addr)
{
	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		{
			RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
			mOwnState.serveraddr = addr;
		}
		
		mNetMgr->setExtAddress(addr);
		
		return true;
	}

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	/* check if it is a friend */
	std::map<std::string, peerState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::setLocalAddress() cannot add addres info : peer id not found in friend list  id: " << id << std::endl;
#endif
			return false;
		}
	}

	/* "it" points to peer */
	it->second.serveraddr = addr;

#if 0
	//update ip address list
	IpAddressTimed ipAddressTimed;
	ipAddressTimed.ipAddr = addr;
	ipAddressTimed.seenTime = time(NULL);
	it->second.updateIpAddressList(ipAddressTimed);
#endif

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return true;
}


bool p3PeerMgrIMPL::setDynDNS(const std::string &id, const std::string &dyndns)
{
    if (id == AuthSSL::getAuthSSL()->OwnId())
    {
        RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
        mOwnState.dyndns = dyndns;
        IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
        return true;
    }

    RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
    /* check if it is a friend */
    std::map<std::string, peerState>::iterator it;
    if (mFriendList.end() == (it = mFriendList.find(id)))
    {
            if (mOthersList.end() == (it = mOthersList.find(id)))
            {
                    #ifdef PEER_DEBUG
                                    std::cerr << "p3PeerMgrIMPL::setDynDNS() cannot add dyn dns info : peer id not found in friend list  id: " << id << std::endl;
                    #endif
                    return false;
            }
    }

    /* "it" points to peer */
    it->second.dyndns = dyndns;

    IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

    return true;
}

bool    p3PeerMgrIMPL::updateAddressList(const std::string& id, const pqiIpAddrSet &addrs)
{
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::setAddressList() called for id : " << id << std::endl;
#endif

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check if it is our own ip */
	if (id == getOwnId()) 
	{
		mOwnState.ipAddrs.updateAddrs(addrs);
		return true;
	}

	/* check if it is a friend */
	std::map<std::string, peerState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
            if (mOthersList.end() == (it = mOthersList.find(id)))
            {
#ifdef PEER_DEBUG
				std::cerr << "p3PeerMgrIMPL::setLocalAddress() cannot add addres info : peer id not found in friend list. id: " << id << std::endl;
#endif
                    return false;
            }
	}

	/* "it" points to peer */
	it->second.ipAddrs.updateAddrs(addrs);
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::setLocalAddress() Updated Address for: " << id;
	std::cerr << std::endl;
	it->second.ipAddrs.printAddrs(std::cerr);
	std::cerr << std::endl;
#endif

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return true;
}


bool    p3PeerMgrIMPL::updateCurrentAddress(const std::string& id, const pqiIpAddress &addr)
{
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::updateCurrentAddress() called for id : " << id << std::endl;
#endif
	
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	
	/* cannot be own id */
	
	/* check if it is a friend */
	std::map<std::string, peerState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
			std::cerr << "p3PeerMgrIMPL::updateCurrentAddress() ERROR peer id not found: " << id << std::endl;
			return false;
		}
	}

	if (isPrivateNet(&(addr.mAddr.sin_addr)))
	{
		it->second.ipAddrs.updateLocalAddrs(addr);
		it->second.localaddr = addr.mAddr;
	}
	else
	{
		it->second.ipAddrs.updateExtAddrs(addr);
		it->second.serveraddr = addr.mAddr;
	}

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::updatedCurrentAddress() Updated Address for: " << id;
	std::cerr << std::endl;
	it->second.ipAddrs.printAddrs(std::cerr);
	std::cerr << std::endl;
#endif
	
	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	
	return true;
}
	

bool    p3PeerMgrIMPL::updateLastContact(const std::string& id)
{
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::updateLastContact() called for id : " << id << std::endl;
#endif
	
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	
	/* cannot be own id */
	
	/* check if it is a friend */
	std::map<std::string, peerState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
			std::cerr << "p3PeerMgrIMPL::updateLastContact() ERROR peer id not found: " << id << std::endl;
			return false;
		}
	}

	it->second.lastcontact = time(NULL);

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	
	return true;
}

	


bool    p3PeerMgrIMPL::setNetworkMode(const std::string &id, uint32_t netMode)
{
	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		setOwnNetworkMode(netMode);
		return true;
	}

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	/* check if it is a friend */
	std::map<std::string, peerState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
			return false;
		}
	}

	/* "it" points to peer */
	it->second.netMode = netMode;
	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return false;
}

bool    p3PeerMgrIMPL::setLocation(const std::string &id, const std::string &location)
{
        RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
        std::cerr << "p3PeerMgrIMPL::setLocation() called for id : " << id << "; with location " << location << std::endl;
#endif
        if (id == AuthSSL::getAuthSSL()->OwnId())
        {
                mOwnState.location = location;
                return true;
        }

        /* check if it is a friend */
        std::map<std::string, peerState>::iterator it;
        if (mFriendList.end() == (it = mFriendList.find(id))) {
            return false;
        } else {
            it->second.location = location;
            return true;
        }
}

bool    p3PeerMgrIMPL::setVisState(const std::string &id, uint32_t visState)
{
	{
		std::string out;
		rs_sprintf(out, "p3PeerMgr::setVisState(%s, %lu)", id.c_str(), visState);
		rslog(RSL_WARNING, p3peermgrzone, out);
	}

	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		setOwnVisState(visState);
		return true;
	}

	bool dht_state ;
	bool isFriend = false;
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		/* check if it is a friend */
		std::map<std::string, peerState>::iterator it;
		if (mFriendList.end() == (it = mFriendList.find(id)))
		{
			if (mOthersList.end() == (it = mOthersList.find(id)))
			{
				return false;
			}
		}
		else
		{
			isFriend = true;
		}

		/* "it" points to peer */
		it->second.visState = visState;
		dht_state = it->second.visState & RS_VIS_STATE_NODHT ;

		std::cerr << "p3PeerMgrIMPL::setVisState(" << id << ", " << std::hex << visState << std::dec << ") ";
		std::cerr << " NAME: " << it->second.name;

		if (it->second.visState & RS_VIS_STATE_NODHT)
		{
			std::cerr << " NO-DHT ";
		}
		else
		{
			std::cerr << " DHT-OK ";
		}
		if (it->second.visState & RS_VIS_STATE_NODISC)
		{
			std::cerr << " NO-DISC ";
		}
		else
		{
			std::cerr << " DISC-OK ";
		}
		std::cerr << std::endl;

	}
	if(isFriend)
	{
		/* toggle DHT state */
		if(dht_state)
		{

			std::cerr << "p3PeerMgrIMPL::setVisState() setFriendVisibility => false";
			std::cerr << std::endl;

			/* hidden from DHT world */
			mLinkMgr->setFriendVisibility(id, false);
		}
		else
		{
			std::cerr << "p3PeerMgrIMPL::setVisState() setFriendVisibility => true";
			std::cerr << std::endl;

			mLinkMgr->setFriendVisibility(id, true);
		}
	}

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return false;
}




/*******************************************************************/


/**********************************************************************
 **********************************************************************
 ******************** p3Config functions ******************************
 **********************************************************************
 **********************************************************************/

        /* Key Functions to be overloaded for Full Configuration */

RsSerialiser *p3PeerMgrIMPL::setupSerialiser()
{
	RsSerialiser *rss = new RsSerialiser();
	rss->addSerialType(new RsPeerConfigSerialiser());
	rss->addSerialType(new RsGeneralConfigSerialiser()) ;

	return rss;
}


bool p3PeerMgrIMPL::saveList(bool &cleanup, std::list<RsItem *>& saveData)
{
	/* create a list of current peers */
	cleanup = false;
	bool useExtAddrFinder = mNetMgr->getIPServersEnabled();
	bool allowTunnelConnection = mLinkMgr->getTunnelConnection();

	mPeerMtx.lock(); /****** MUTEX LOCKED *******/ 

	RsPeerNetItem *item = new RsPeerNetItem();
	item->clear();

	item->pid = getOwnId();
        item->gpg_id = mOwnState.gpg_id;
        item->location = mOwnState.location;
#if 0
	if (mOwnState.netMode & RS_NET_MODE_TRY_EXT)
	{
		item->netMode = RS_NET_MODE_EXT;
	}
	else if (mOwnState.netMode & RS_NET_MODE_TRY_UPNP)
	{
		item->netMode = RS_NET_MODE_UPNP;
	}
	else
	{
		item->netMode = RS_NET_MODE_UDP;
	}
#endif
	item->netMode = mOwnState.netMode;

	item->visState = mOwnState.visState;
	item->lastContact = mOwnState.lastcontact;

        item->currentlocaladdr = mOwnState.localaddr;
        item->currentremoteaddr = mOwnState.serveraddr;
        item->dyndns = mOwnState.dyndns;
        mOwnState.ipAddrs.mLocal.loadTlv(item->localAddrList);
        mOwnState.ipAddrs.mExt.loadTlv(item->extAddrList);

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::saveList() Own Config Item:" << std::endl;
	item->print(std::cerr, 10);
	std::cerr << std::endl;
#endif

	saveData.push_back(item);
	saveCleanupList.push_back(item);

	/* iterate through all friends and save */
        std::map<std::string, peerState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		item = new RsPeerNetItem();
		item->clear();

		item->pid = it->first;
		item->gpg_id = (it->second).gpg_id;
		item->location = (it->second).location;
		item->netMode = (it->second).netMode;
		item->visState = (it->second).visState;
		item->lastContact = (it->second).lastcontact;
		item->currentlocaladdr = (it->second).localaddr;
		item->currentremoteaddr = (it->second).serveraddr;
		item->dyndns = (it->second).dyndns;
		(it->second).ipAddrs.mLocal.loadTlv(item->localAddrList);
		(it->second).ipAddrs.mExt.loadTlv(item->extAddrList);

		saveData.push_back(item);
		saveCleanupList.push_back(item);
#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::saveList() Peer Config Item:" << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
#endif
	}

	RsPeerServicePermissionItem *sitem = new RsPeerServicePermissionItem ;

	for(std::map<std::string,ServicePermissionFlags>::const_iterator it(mFriendsPermissionFlags.begin());it!=mFriendsPermissionFlags.end();++it)
	{
		sitem->pgp_ids.push_back(it->first) ;
		sitem->service_flags.push_back(it->second) ;
	}

	saveData.push_back(sitem) ;
	saveCleanupList.push_back(sitem);

	// Now save config for network digging strategies
	
	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;

	RsTlvKeyValue kv;
	kv.key = "USE_EXTR_IP_FINDER" ;
	kv.value = (useExtAddrFinder)?"TRUE":"FALSE" ;
	vitem->tlvkvs.pairs.push_back(kv) ;

#ifdef PEER_DEBUG
	std::cout << "Pushing item for use_extr_addr_finder = " << useExtAddrFinder << std::endl ;
#endif
	saveData.push_back(vitem);
	saveCleanupList.push_back(vitem);

                // Now save config for network digging strategies

        RsConfigKeyValueSet *vitem2 = new RsConfigKeyValueSet ;

        RsTlvKeyValue kv2;
        kv2.key = "ALLOW_TUNNEL_CONNECTION" ;
        kv2.value = (allowTunnelConnection)?"TRUE":"FALSE" ;
        vitem2->tlvkvs.pairs.push_back(kv2) ;

#ifdef PEER_DEBUG
        std::cout << "Pushing item for allow_tunnel_connection = " << allowTunnelConnection << std::endl ;
#endif
        saveData.push_back(vitem2);
	saveCleanupList.push_back(vitem2);

	/* save groups */

	std::list<RsPeerGroupItem *>::iterator groupIt;
	for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
		saveData.push_back(*groupIt); // no delete
	}

	return true;
}

void    p3PeerMgrIMPL::saveDone()
{
	/* clean up the save List */
	std::list<RsItem *>::iterator it;
	for(it = saveCleanupList.begin(); it != saveCleanupList.end(); it++)
	{
		delete (*it);
	}

	saveCleanupList.clear();

	/* unlock mutex */
	mPeerMtx.unlock(); /****** MUTEX UNLOCKED *******/
}

bool  p3PeerMgrIMPL::loadList(std::list<RsItem *>& load)
{

	// DEFAULTS.
	bool useExtAddrFinder = true;
	bool allowTunnelConnection = true;
	
        if (load.size() == 0) {
            std::cerr << "p3PeerMgrIMPL::loadList() list is empty, it may be a configuration problem."  << std::endl;
            return false;
        }

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::loadList() Item Count: " << load.size() << std::endl;
#endif

	std::string ownId = getOwnId();

	/* load the list of peers */
	std::list<RsItem *>::iterator it;
	for(it = load.begin(); it != load.end(); it++)
	{
		RsPeerNetItem *pitem = dynamic_cast<RsPeerNetItem *>(*it);
		if (pitem)
		{
			if (pitem->pid == ownId)
			{
#ifdef PEER_DEBUG
				std::cerr << "p3PeerMgrIMPL::loadList() Own Config Item:" << std::endl;
				pitem->print(std::cerr, 10);
				std::cerr << std::endl;
#endif
				/* add ownConfig */
				setOwnNetworkMode(pitem->netMode);
				setOwnVisState(pitem->visState);
				
				mOwnState.gpg_id = AuthGPG::getAuthGPG()->getGPGOwnId();
				mOwnState.location = AuthSSL::getAuthSSL()->getOwnLocation();
			}
			else
			{
#ifdef PEER_DEBUG
				std::cerr << "p3PeerMgrIMPL::loadList() Peer Config Item:" << std::endl;
				pitem->print(std::cerr, 10);
				std::cerr << std::endl;
#endif
				/* ************* */
				addFriend(pitem->pid, pitem->gpg_id, pitem->netMode, pitem->visState, pitem->lastContact, RS_SERVICE_PERM_ALL);
				setLocation(pitem->pid, pitem->location);
			}
			
			setLocalAddress(pitem->pid, pitem->currentlocaladdr);
			setExtAddress(pitem->pid, pitem->currentremoteaddr);
			setDynDNS (pitem->pid, pitem->dyndns);

			/* convert addresses */
			pqiIpAddrSet addrs;
			addrs.mLocal.extractFromTlv(pitem->localAddrList);
			addrs.mExt.extractFromTlv(pitem->extAddrList);
			
			updateAddressList(pitem->pid, addrs);

			delete(*it);

			continue;
		}

		RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet *>(*it) ;
		if (vitem)
		{
			RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::loadList() General Variable Config Item:" << std::endl;
			vitem->print(std::cerr, 10);
			std::cerr << std::endl;
#endif
			std::list<RsTlvKeyValue>::iterator kit;
			for(kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); kit++) {
				if(kit->key == "USE_EXTR_IP_FINDER") {
					useExtAddrFinder = (kit->value == "TRUE");
					std::cerr << "setting use_extr_addr_finder to " << useExtAddrFinder << std::endl ;
				} else if (kit->key == "ALLOW_TUNNEL_CONNECTION") {
					allowTunnelConnection = (kit->value == "TRUE");
					std::cerr << "setting allow_tunnel_connection to " << allowTunnelConnection << std::endl ;
				}
			}

			delete(*it);

			continue;
		}

		RsPeerGroupItem *gitem = dynamic_cast<RsPeerGroupItem *>(*it) ;
		if (gitem)
		{
			RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::loadList() Peer group item:" << std::endl;
			gitem->print(std::cerr, 10);
			std::cerr << std::endl;
#endif

			groupList.push_back(gitem); // don't delete

			if ((gitem->flag & RS_GROUP_FLAG_STANDARD) == 0) {
				/* calculate group id */
				uint32_t groupId = atoi(gitem->id.c_str());
				if (groupId > lastGroupId) {
					lastGroupId = groupId;
				}
			}

			continue;
		}
		RsPeerServicePermissionItem *sitem = dynamic_cast<RsPeerServicePermissionItem*>(*it) ;

		if(sitem)
		{
			RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

			std::cerr << "Loaded service permission item: " << std::endl;

			for(uint32_t i=0;i<sitem->pgp_ids.size();++i)
				if(AuthGPG::getAuthGPG()->isGPGAccepted(sitem->pgp_ids[i]) || sitem->pgp_ids[i] == AuthGPG::getAuthGPG()->getGPGOwnId())
				{
					mFriendsPermissionFlags[sitem->pgp_ids[i]] = sitem->service_flags[i] ;
					std::cerr << "   " << sitem->pgp_ids[i] << " - " << sitem->service_flags[i] << std::endl;
				}
				else
					std::cerr << "   " << sitem->pgp_ids[i] << " - Not a friend!" << std::endl;
		}

		delete (*it);
	}

	{
		/* set missing groupIds */

		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		/* Standard groups */
		const int standardGroupCount = 5;
		const char *standardGroup[standardGroupCount] = { RS_GROUP_ID_FRIENDS, RS_GROUP_ID_FAMILY, RS_GROUP_ID_COWORKERS, RS_GROUP_ID_OTHERS, RS_GROUP_ID_FAVORITES };
		bool foundStandardGroup[standardGroupCount] = { false, false, false, false, false };

		std::list<RsPeerGroupItem *>::iterator groupIt;
		for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
			if ((*groupIt)->flag & RS_GROUP_FLAG_STANDARD) {
				int i;
				for (i = 0; i < standardGroupCount; i++) {
					if ((*groupIt)->id == standardGroup[i]) {
						foundStandardGroup[i] = true;
						break;
					}
				}
				
				if (i >= standardGroupCount) {
					/* No more a standard group, remove the flag standard */
					(*groupIt)->flag &= ~RS_GROUP_FLAG_STANDARD;
				}
			} else {
				uint32_t groupId = atoi((*groupIt)->id.c_str());
				if (groupId == 0) {
					rs_sprintf((*groupIt)->id, "%lu", lastGroupId++);
				}
			}
		}
		
		/* Initialize standard groups */
		for (int i = 0; i < standardGroupCount; i++) {
			if (foundStandardGroup[i] == false) {
				RsPeerGroupItem *gitem = new RsPeerGroupItem;
				gitem->id = standardGroup[i];
				gitem->name = standardGroup[i];
				gitem->flag |= RS_GROUP_FLAG_STANDARD;
				groupList.push_back(gitem);
			}
		}
	}

	mNetMgr->setIPServersEnabled(useExtAddrFinder);
	mLinkMgr->setTunnelConnection(allowTunnelConnection);
	
	return true;
}


#if 0

void  printConnectState(std::ostream &out, peerState &peer)
{

	out << "Friend: " << peer.name << " Id: " << peer.id << " State: " << peer.state;
	if (peer.state & RS_PEER_S_FRIEND)
		out << " S:RS_PEER_S_FRIEND";
	if (peer.state & RS_PEER_S_ONLINE)
		out << " S:RS_PEER_S_ONLINE";
	if (peer.state & RS_PEER_S_CONNECTED)
		out << " S:RS_PEER_S_CONNECTED";
	out << " Actions: " << peer.actions;
	if (peer.actions & RS_PEER_NEW)
		out << " A:RS_PEER_NEW";
	if (peer.actions & RS_PEER_MOVED)
		out << " A:RS_PEER_MOVED";
	if (peer.actions & RS_PEER_CONNECTED)
		out << " A:RS_PEER_CONNECTED";
	if (peer.actions & RS_PEER_DISCONNECTED)
		out << " A:RS_PEER_DISCONNECTED";
	if (peer.actions & RS_PEER_CONNECT_REQ)
		out << " A:RS_PEER_CONNECT_REQ";

	out << std::endl;
	return;
}

#endif


/**********************************************************************
 **********************************************************************
 ************************** Groups ************************************
 **********************************************************************
 **********************************************************************/

bool p3PeerMgrIMPL::addGroup(RsGroupInfo &groupInfo)
{
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		RsPeerGroupItem *groupItem = new RsPeerGroupItem;
		groupItem->set(groupInfo);

		rs_sprintf(groupItem->id, "%lu", ++lastGroupId);

		// remove standard flag
		groupItem->flag &= ~RS_GROUP_FLAG_STANDARD;

		groupItem->PeerId(getOwnId());

		groupList.push_back(groupItem);

		groupInfo.id = groupItem->id;
	}

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_ADD);

	IndicateConfigChanged();

	return true;
}

bool p3PeerMgrIMPL::editGroup(const std::string &groupId, RsGroupInfo &groupInfo)
{
	if (groupId.empty()) {
		return false;
	}

	bool changed = false;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		std::list<RsPeerGroupItem*>::iterator groupIt;
		for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
			if ((*groupIt)->id == groupId) {
				break;
			}
		}

		if (groupIt != groupList.end()) {
			if ((*groupIt)->flag & RS_GROUP_FLAG_STANDARD) {
				// can't edit standard groups
			} else {
				changed = true;
				(*groupIt)->set(groupInfo);
			}
		}
	}

	if (changed) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_MOD);

		IndicateConfigChanged();
	}

	return changed;
}

bool p3PeerMgrIMPL::removeGroup(const std::string &groupId)
{
	if (groupId.empty()) {
		return false;
	}

	bool changed = false;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		std::list<RsPeerGroupItem*>::iterator groupIt;
		for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
			if ((*groupIt)->id == groupId) {
				break;
			}
		}

		if (groupIt != groupList.end()) {
			if ((*groupIt)->flag & RS_GROUP_FLAG_STANDARD) {
				// can't remove standard groups
			} else {
				changed = true;
				delete(*groupIt);
				groupList.erase(groupIt);
			}
		}
	}

	if (changed) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_DEL);

		IndicateConfigChanged();
	}

	return changed;
}

bool p3PeerMgrIMPL::getGroupInfo(const std::string &groupId, RsGroupInfo &groupInfo)
{
	if (groupId.empty()) {
		return false;
	}

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	std::list<RsPeerGroupItem*>::iterator groupIt;
	for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
		if ((*groupIt)->id == groupId) {
			(*groupIt)->get(groupInfo);

			return true;
		}
	}

	return false;
}

bool p3PeerMgrIMPL::getGroupInfoList(std::list<RsGroupInfo> &groupInfoList)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	std::list<RsPeerGroupItem*>::iterator groupIt;
	for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
		RsGroupInfo groupInfo;
		(*groupIt)->get(groupInfo);
		groupInfoList.push_back(groupInfo);
	}

	return true;
}

// groupId == "" && assign == false -> remove from all groups
bool p3PeerMgrIMPL::assignPeersToGroup(const std::string &groupId, const std::list<std::string> &peerIds, bool assign)
{
	if (groupId.empty() && assign == true) {
		return false;
	}

	if (peerIds.empty()) {
		return false;
	}

	bool changed = false;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		std::list<RsPeerGroupItem*>::iterator groupIt;
		for (groupIt = groupList.begin(); groupIt != groupList.end(); groupIt++) {
			if (groupId.empty() || (*groupIt)->id == groupId) {
				RsPeerGroupItem *groupItem = *groupIt;

				std::list<std::string>::const_iterator peerIt;
				for (peerIt = peerIds.begin(); peerIt != peerIds.end(); peerIt++) {
					std::list<std::string>::iterator peerIt1 = std::find(groupItem->peerIds.begin(), groupItem->peerIds.end(), *peerIt);
					if (assign) {
						if (peerIt1 == groupItem->peerIds.end()) {
							groupItem->peerIds.push_back(*peerIt);
							changed = true;
						}
					} else {
						if (peerIt1 != groupItem->peerIds.end()) {
							groupItem->peerIds.erase(peerIt1);
							changed = true;
						}
					}
				}

				if (groupId.empty() == false) {
					break;
				}
			}
		}
	}

	if (changed) {
		rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_MOD);

		IndicateConfigChanged();
	}

	return changed;
}


/**********************************************************************
 **********************************************************************
 ******************** Service permission stuff ************************
 **********************************************************************
 **********************************************************************/

ServicePermissionFlags p3PeerMgrIMPL::servicePermissionFlags_sslid(const std::string& ssl_id)
{
	std::string gpg_id ;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		if(ssl_id.length() != 32)
		{
			std::cerr << "(EE) p3PeerMgrIMPL::servicePermissionFlags_sslid() called with inconsistent id " << ssl_id << std::endl;
			return RS_SERVICE_PERM_ALL ;
		}
		std::map<std::string, peerState>::const_iterator it = mFriendList.find(ssl_id);

		if(it == mFriendList.end())
			return RS_SERVICE_PERM_ALL ;

		gpg_id = it->second.gpg_id ;
	}

	return servicePermissionFlags(gpg_id) ;
}


ServicePermissionFlags p3PeerMgrIMPL::servicePermissionFlags(const std::string& pgp_id)
{
	// 
	if(pgp_id.length() != 16)
	{
		std::cerr << "(EE) p3PeerMgrIMPL::servicePermissionFlags() called with inconsistent id " << pgp_id << std::endl;
		return RS_SERVICE_PERM_ALL ;
	}

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		std::map<std::string,ServicePermissionFlags>::const_iterator it = mFriendsPermissionFlags.find( pgp_id ) ;

		if(it == mFriendsPermissionFlags.end())
			return RS_SERVICE_PERM_ALL ;
		else
			return it->second ;
	}
}
void p3PeerMgrIMPL::setServicePermissionFlags(const std::string& pgp_id, const ServicePermissionFlags& flags)
{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		// Check that we have a PGP id. This should not be necessary, but because
		// we use std::string, anything can get passed down here.
		//
		if(pgp_id.length() != 16)
		{
			std::cerr << "Bad parameter passed to setServicePermissionFlags(): " << pgp_id << std::endl;
			return ;
		}

		mFriendsPermissionFlags[pgp_id] = flags ;
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
}

/**********************************************************************
 **********************************************************************
 ******************** Stuff moved from p3peers ************************
 **********************************************************************
 **********************************************************************/

bool p3PeerMgrIMPL::removeAllFriendLocations(const std::string &gpgid)
{
	std::list<std::string> sslIds;
	if (!getAssociatedPeers(gpgid, sslIds))
	{
		return false;
	}
	
	std::list<std::string>::iterator it;
	for(it = sslIds.begin(); it != sslIds.end(); it++)
	{
		removeFriend(*it, true);
	}
	
	return true;
}


bool	p3PeerMgrIMPL::getAssociatedPeers(const std::string &gpg_id, std::list<std::string> &ids)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef P3PEERS_DEBUG
	std::cerr << "p3PeerMgr::getAssociatedPeers() for id : " << gpg_id << std::endl;
#endif
	
	int count = 0;
	std::map<std::string, peerState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); it++)
	{
		if (it->second.gpg_id == gpg_id)
		{
			count++;
			ids.push_back(it->first);

#ifdef P3PEERS_DEBUG
			std::cerr << "p3PeerMgr::getAssociatedPeers() found ssl id :  " << it->first << std::endl;
#endif
			
		}
	}
	
	return (count > 0);
}




/* This only removes SSL certs, that are old... Can end up with no Certs per GPG Id 
 * We are removing the concept of a "DummyId" - There is no need for it.
 */

bool isDummyFriend(std::string id)
{
	bool ret = (id.substr(0,5) == "dummy");
	return ret;
}


bool p3PeerMgrIMPL::removeUnusedLocations()
{
	std::list<std::string> toRemove;
	
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	
#ifdef P3PEERS_DEBUG
		std::cerr << "p3PeerMgr::removeUnusedLocations()" << std::endl;
#endif
		
		time_t now = time(NULL);
		
		std::map<std::string, peerState>::iterator it;
		for(it = mFriendList.begin(); it != mFriendList.end(); it++)
		{
			if (now - it->second.lastcontact > VERY_OLD_PEER)
			{
				toRemove.push_back(it->first);

#ifdef P3PEERS_DEBUG
				std::cerr << "p3PeerMgr::removeUnusedLocations() removing Old SSL Id: " << it->first << std::endl;
#endif
				
			}
			
			if (isDummyFriend(it->first))
			{
				toRemove.push_back(it->first);
				
#ifdef P3PEERS_DEBUG
				std::cerr << "p3PeerMgr::removeUnusedLocations() removing Dummy Id: " << it->first << std::endl;
#endif
				
			}
			
		}
	}
	std::list<std::string>::iterator it;
	
	for(it = toRemove.begin(); it != toRemove.end(); it++)
	{
		removeFriend(*it,false);
	}

	return true;
}

	
