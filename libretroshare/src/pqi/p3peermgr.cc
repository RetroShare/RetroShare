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

#include "rsserver/p3face.h"
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

#include "retroshare/rsiface.h" // Needed for rsicontrol (should remove this dependancy)
#include "retroshare/rspeers.h" // Needed for Group Parameters.
#include "retroshare/rsbanlist.h" // Needed for banned IPs

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

static const std::string kConfigDefaultProxyServerIpAddr = "127.0.0.1";
static const uint16_t    kConfigDefaultProxyServerPort = 9050; // standard port.

static const std::string kConfigKeyExtIpFinder = "USE_EXTR_IP_FINDER";
static const std::string kConfigKeyProxyServerIpAddr = "PROXY_SERVER_IPADDR";
static const std::string kConfigKeyProxyServerPort = "PROXY_SERVER_PORT";
	
void  printConnectState(std::ostream &out, peerState &peer);

peerState::peerState()
	:netMode(RS_NET_MODE_UNKNOWN), vs_disc(RS_VS_DISC_FULL), vs_dht(RS_VS_DHT_FULL), lastcontact(0), 
	 hiddenNode(false), hiddenPort(0) 
{
        sockaddr_storage_clear(localaddr);
        sockaddr_storage_clear(serveraddr);

	return;
}

std::string textPeerConnectState(peerState &state)
{
	std::string out = "Id: " + state.id.toStdString() + "\n";
	rs_sprintf_append(out, "NetMode: %lu\n", state.netMode);
	rs_sprintf_append(out, "VisState: Disc: %u Dht: %u\n", state.vs_disc, state.vs_dht);
	
	out += "laddr: ";
	out += sockaddr_storage_tostring(state.localaddr);
	out += "\neaddr: ";
	out += sockaddr_storage_tostring(state.serveraddr);
	out += "\n";
	
	return out;
}


p3PeerMgrIMPL::p3PeerMgrIMPL(const RsPeerId& ssl_own_id, const RsPgpId& gpg_own_id, const std::string& gpg_own_name, const std::string& ssl_own_location)
	:p3Config(), mPeerMtx("p3PeerMgr"), mStatusChanged(false)
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
		mOwnState.vs_disc = RS_VS_DISC_FULL;
		mOwnState.vs_dht = RS_VS_DHT_FULL;
	
		lastGroupId = 1;

		// setup default ProxyServerAddress.
		sockaddr_storage_clear(mProxyServerAddress);
		sockaddr_storage_ipv4_aton(mProxyServerAddress,
				kConfigDefaultProxyServerIpAddr.c_str());
		sockaddr_storage_ipv4_setport(mProxyServerAddress, 
                kConfigDefaultProxyServerPort);

        mProxyServerStatus = RS_NET_PROXY_STATUS_UNKNOWN ;
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


bool p3PeerMgrIMPL::setupHiddenNode(const std::string &hiddenAddress, const uint16_t hiddenPort)
{
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::setupHiddenNode()";
		std::cerr << " Address: " << hiddenAddress;
		std::cerr << " Port: " << hiddenPort;
		std::cerr << std::endl;
#endif

		mOwnState.hiddenNode = true;
		mOwnState.hiddenPort = hiddenPort;
		mOwnState.hiddenDomain = hiddenAddress;
	}

	forceHiddenNode();
	return true;
}


bool p3PeerMgrIMPL::forceHiddenNode()
{
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
		if (RS_NET_MODE_HIDDEN != mOwnState.netMode)
		{
#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::forceHiddenNode() Required!";
			std::cerr << std::endl;
#endif
		}
		mOwnState.hiddenNode = true;

		// force external address - otherwise its invalid.
		sockaddr_storage_clear(mOwnState.serveraddr);
		sockaddr_storage_ipv4_aton(mOwnState.serveraddr, "0.0.0.0");
		sockaddr_storage_ipv4_setport(mOwnState.serveraddr, 0);
	}

	setOwnNetworkMode(RS_NET_MODE_HIDDEN);

	// switch off DHT too.
	setOwnVisState(mOwnState.vs_disc, RS_VS_DHT_OFF);

	// Force the Port.
	struct sockaddr_storage loopback;
	sockaddr_storage_clear(loopback);
	sockaddr_storage_ipv4_aton(loopback, "127.0.0.1");
	uint16_t port = sockaddr_storage_port(mOwnState.localaddr); 
	sockaddr_storage_ipv4_setport(loopback, port); 

	setLocalAddress(AuthSSL::getAuthSSL()->OwnId(), loopback);

	mNetMgr->setIPServersEnabled(false);

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	return true;
}


bool p3PeerMgrIMPL::setOwnNetworkMode(uint32_t netMode)
{
	bool changed = false;
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::setOwnNetworkMode() :";
		std::cerr << " Existing netMode: " << mOwnState.netMode;
		std::cerr << " Input netMode: " << netMode;
		std::cerr << std::endl;
#endif

		if (mOwnState.netMode != (netMode & RS_NET_MODE_ACTUAL))
		{
			mOwnState.netMode = (netMode & RS_NET_MODE_ACTUAL);
			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
			changed = true;
		}
	}
	
	// Pass on Flags to NetMgr.
	mNetMgr->setNetworkMode((netMode & RS_NET_MODE_ACTUAL));
	return changed;
}

bool p3PeerMgrIMPL::setOwnVisState(uint16_t vs_disc, uint16_t vs_dht)
{
	bool changed = false;
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		std::string out;
		rs_sprintf(out, "p3PeerMgr::setOwnVisState() Existing vis: %u/%u Input vis: %u/%u", 
				   mOwnState.vs_disc, mOwnState.vs_dht, vs_disc, vs_dht);
		rslog(RSL_WARNING, p3peermgrzone, out);

#ifdef PEER_DEBUG
		std::cerr << out.c_str() << std::endl;
#endif

		if (mOwnState.vs_disc != vs_disc || mOwnState.vs_dht != vs_dht) 
		{
			mOwnState.vs_disc = vs_disc;
			mOwnState.vs_dht = vs_dht;
			changed = true;
			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		}
	}
	
	// Pass on Flags to NetMgr.
	mNetMgr->setVisState(vs_disc, vs_dht);

	return changed;
}


void p3PeerMgrIMPL::tick()
{
    static const time_t INTERVAL_BETWEEN_LOCATION_CLEANING = 300 ; // Remove unused locations and clean IPs every 10 minutes.

    static time_t last_friends_check = time(NULL) ; // first cleaning after 1 hour.

    time_t now = time(NULL) ;

    if(now > INTERVAL_BETWEEN_LOCATION_CLEANING + last_friends_check )
    {
#ifdef PEER_DEBUG
        std::cerr << "p3PeerMgrIMPL::tick(): cleaning unused locations." << std::endl ;
#endif

        rslog(RSL_WARNING, p3peermgrzone, "p3PeerMgr::tick() removeUnusedLocations()");

        removeUnusedLocations() ;

#ifdef PEER_DEBUG
        std::cerr << "p3PeerMgrIMPL::tick(): cleaning banned/old IPs." << std::endl ;
#endif
        removeBannedIps() ;

        last_friends_check = now ;
    }
}


/********************************  Network Status  *********************************
 * Configuration Loading / Saving.
 */


const RsPeerId& p3PeerMgrIMPL::getOwnId()
{
                return AuthSSL::getAuthSSL()->OwnId();
}


bool p3PeerMgrIMPL::getOwnNetStatus(peerState &state)
{
        RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
        state = mOwnState;
	return true;
}

bool p3PeerMgrIMPL::isFriend(const RsPeerId& id)
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

bool    p3PeerMgrIMPL::getPeerName(const RsPeerId &ssl_id, std::string &name)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<RsPeerId, peerState>::iterator it;
	it = mFriendList.find(ssl_id);
	if (it == mFriendList.end())
	{
		return false;
	}

	name = it->second.name + " (" + it->second.location + ")";
	return true;
}

bool    p3PeerMgrIMPL::getGpgId(const RsPeerId &ssl_id, RsPgpId &gpgId)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<RsPeerId, peerState>::iterator it;
	it = mFriendList.find(ssl_id);
	if (it == mFriendList.end())
	{
		return false;
	}

	gpgId = it->second.gpg_id;
	return true;
}

/**** HIDDEN STUFF ****/

bool    p3PeerMgrIMPL::isHidden()
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	return mOwnState.hiddenNode;
}


bool    p3PeerMgrIMPL::isHiddenPeer(const RsPeerId &ssl_id)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<RsPeerId, peerState>::iterator it;
	it = mFriendList.find(ssl_id);
	if (it == mFriendList.end())
	{
#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::isHiddenPeer(" << ssl_id << ") Missing Peer => false";
		std::cerr << std::endl;
#endif

		return false;
	}

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::isHiddenPeer(" << ssl_id << ") = " << (it->second).hiddenNode;
	std::cerr << std::endl;
#endif
	return (it->second).hiddenNode;
}

bool p3PeerMgrIMPL::setHiddenDomainPort(const RsPeerId &ssl_id, const std::string &domain_addr, const uint16_t domain_port)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::setHiddenDomainPort()";
	std::cerr << std::endl;
#endif

	std::string domain = domain_addr;
	// trim whitespace!
	size_t pos = domain.find_last_not_of(" \t\n");
	if (std::string::npos != pos)
	{
		domain = domain.substr(0, pos + 1);
	}
	pos = domain.find_first_not_of(" \t\n");
	if (std::string::npos != pos)
	{
		domain = domain.substr(pos);
	}

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	if (ssl_id == AuthSSL::getAuthSSL()->OwnId()) 
	{
		mOwnState.hiddenNode = true;
		mOwnState.hiddenDomain = domain;
		mOwnState.hiddenPort = domain_port;
#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::setHiddenDomainPort() Set own State";
		std::cerr << std::endl;
#endif
		return true;
	}

	/* check for existing */
	std::map<RsPeerId, peerState>::iterator it;
	it = mFriendList.find(ssl_id);
	if (it == mFriendList.end())
	{
#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::setHiddenDomainPort() Peer Not Found";
		std::cerr << std::endl;
#endif
		return false;
	}

	it->second.hiddenDomain = domain;
	it->second.hiddenPort = domain_port;
	it->second.hiddenNode = true;
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::setHiddenDomainPort() Set Peers State";
	std::cerr << std::endl;
#endif

	return true;
}

bool p3PeerMgrIMPL::setProxyServerAddress(const struct sockaddr_storage &proxy_addr)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	if (!sockaddr_storage_same(mProxyServerAddress,proxy_addr))
	{
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		mProxyServerAddress = proxy_addr;
	}
	return true;
}

bool p3PeerMgrIMPL::resetOwnExternalAddressList()
{
    RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

    mOwnState.ipAddrs.mLocal.mAddrs.clear() ;
    mOwnState.ipAddrs.mExt.mAddrs.clear() ;

    IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

    return true ;
}

bool p3PeerMgrIMPL::getProxyServerStatus(uint32_t& proxy_status)
{
    RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

    proxy_status = mProxyServerStatus;
    return true;
}
bool p3PeerMgrIMPL::getProxyServerAddress(struct sockaddr_storage &proxy_addr)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	proxy_addr = mProxyServerAddress;
	return true;
}
	
bool p3PeerMgrIMPL::getProxyAddress(const RsPeerId &ssl_id, struct sockaddr_storage &proxy_addr, std::string &domain_addr, uint16_t &domain_port)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<RsPeerId, peerState>::iterator it;
	it = mFriendList.find(ssl_id);
	if (it == mFriendList.end())
	{
		return false;
	}

	if (!it->second.hiddenNode)
	{
		return false;
	}

	domain_addr = it->second.hiddenDomain;
	domain_port = it->second.hiddenPort;

	proxy_addr = mProxyServerAddress;
	return true;
}

// Placeholder until we implement this functionality.
uint32_t p3PeerMgrIMPL::getConnectionType(const RsPeerId &/*sslId*/)
{
	return RS_NET_CONN_TYPE_FRIEND;
}

int p3PeerMgrIMPL::getFriendCount(bool ssl, bool online)
{
	if (online) {
		// count only online id's
		std::list<RsPeerId> onlineIds;
		mLinkMgr->getOnlineList(onlineIds);

		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		std::set<RsPgpId> gpgIds;
		int count = 0;

		std::map<RsPeerId, peerState>::iterator it;
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
	std::list<RsPgpId> gpgIds;
	AuthGPG::getAuthGPG()->getGPGAcceptedList(gpgIds);

	// add own gpg id, if we have more than one location
	std::list<RsPeerId> ownSslIds;
	getAssociatedPeers(AuthGPG::getAuthGPG()->getGPGOwnId(), ownSslIds);

	return gpgIds.size() + ((ownSslIds.size() > 0) ? 1 : 0);
}

bool p3PeerMgrIMPL::getFriendNetStatus(const RsPeerId &id, peerState &state)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<RsPeerId, peerState>::iterator it;
	it = mFriendList.find(id);
	if (it == mFriendList.end())
	{
		return false;
	}

	state = it->second;
	return true;
}


bool p3PeerMgrIMPL::getOthersNetStatus(const RsPeerId &id, peerState &state)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check for existing */
	std::map<RsPeerId, peerState>::iterator it;
	it = mOthersList.find(id);
	if (it == mOthersList.end())
	{
		return false;
	}

	state = it->second;
	return true;
}

int p3PeerMgrIMPL::getConnectAddresses(const RsPeerId &id, 
					struct sockaddr_storage &lAddr, struct sockaddr_storage &eAddr, 
					pqiIpAddrSet &histAddrs, std::string &dyndns)
{

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	
	/* check for existing */
	std::map<RsPeerId, peerState>::iterator it;
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
        std::map<RsPeerId, peerState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
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

bool p3PeerMgrIMPL::addFriend(const RsPeerId& input_id, const RsPgpId& input_gpg_id, uint32_t netMode, uint16_t vs_disc, uint16_t vs_dht, time_t lastContact,ServicePermissionFlags service_flags)
{
	bool notifyLinkMgr = false;
	RsPeerId id = input_id ;
	RsPgpId gpg_id = input_gpg_id ;

	rslog(RSL_WARNING, p3peermgrzone, "p3PeerMgr::addFriend() id: " + id.toStdString());

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

		std::map<RsPeerId, peerState>::iterator it;
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
			it->second.vs_disc = vs_disc;
			it->second.vs_dht = vs_dht;
			
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
			
			pstate.vs_disc = vs_disc;
			pstate.vs_dht = vs_dht;
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
		mLinkMgr->addFriend(id, vs_dht != RS_VS_DHT_OFF);
	}

	service_flags &= servicePermissionFlags(gpg_id) ; // Always reduce the permissions. 
	setServicePermissionFlags(gpg_id,service_flags) ;

#ifdef PEER_DEBUG
	printPeerLists(std::cerr);
	mLinkMgr->printPeerLists(std::cerr);
#endif
	
	return true;
}

bool p3PeerMgrIMPL::removeFriend(const RsPgpId &id)
{
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::removeFriend() for id : " << id << std::endl;
	std::cerr << "p3PeerMgrIMPL::removeFriend() mFriendList.size() : " << mFriendList.size() << std::endl;
#endif

        rslog(RSL_WARNING, p3peermgrzone, "p3PeerMgr::removeFriend() id: " + id.toStdString());

	std::list<RsPeerId> sslid_toRemove; // This is a list of SSLIds.
	rsPeers->getAssociatedSSLIds(id,sslid_toRemove) ;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		/* move to othersList */
        bool success = false;
		std::map<RsPeerId, peerState>::iterator it;
		//remove ssl and gpg_ids
		for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
		{
			if (find(sslid_toRemove.begin(),sslid_toRemove.end(),it->second.id) != sslid_toRemove.end())
			{
#ifdef PEER_DEBUG
				std::cerr << "p3PeerMgrIMPL::removeFriend() friend found in the list." << id << std::endl;
#endif
				peerState peer = it->second;

				sslid_toRemove.push_back(it->second.id);

				mOthersList[it->second.id] = peer;
				mStatusChanged = true;

				success = true;
			}
		}

		for(std::list<RsPeerId>::iterator rit = sslid_toRemove.begin(); rit != sslid_toRemove.end(); ++rit)
			if (mFriendList.end() != (it = mFriendList.find(*rit))) 
				mFriendList.erase(it);

		std::map<RsPgpId,ServicePermissionFlags>::iterator it2 = mFriendsPermissionFlags.find(id) ; 

		if(it2 != mFriendsPermissionFlags.end())
			mFriendsPermissionFlags.erase(it2);

#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::removeFriend() new mFriendList.size() : " << mFriendList.size() << std::endl;
#endif
	}

	std::list<RsPeerId>::iterator rit;
	for(rit = sslid_toRemove.begin(); rit != sslid_toRemove.end(); ++rit)
	{
		mLinkMgr->removeFriend(*rit);
	}

	/* remove id from all groups */

	std::list<RsPgpId> ids ;
	ids.push_back(id) ;
	assignPeersToGroup("", ids, false);

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

#ifdef PEER_DEBUG
	printPeerLists(std::cerr);
	mLinkMgr->printPeerLists(std::cerr);
#endif

        return !sslid_toRemove.empty();
}
bool p3PeerMgrIMPL::removeFriend(const RsPeerId &id, bool removePgpId)
{

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::removeFriend() for id : " << id << std::endl;
	std::cerr << "p3PeerMgrIMPL::removeFriend() mFriendList.size() : " << mFriendList.size() << std::endl;
#endif

        rslog(RSL_WARNING, p3peermgrzone, "p3PeerMgr::removeFriend() id: " + id.toStdString());

	std::list<RsPeerId> sslid_toRemove; // This is a list of SSLIds.
	std::list<RsPgpId> pgpid_toRemove; // This is a list of SSLIds.

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		/* move to othersList */
		bool success = false;
		std::map<RsPeerId, peerState>::iterator it;
		//remove ssl and gpg_ids
		for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
		{
			if (it->second.id == id)
			{
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

		for(std::list<RsPeerId>::iterator rit = sslid_toRemove.begin(); rit != sslid_toRemove.end(); ++rit)
			if (mFriendList.end() != (it = mFriendList.find(*rit))) 
				mFriendList.erase(it);

		std::map<RsPgpId,ServicePermissionFlags>::iterator it2 ;

		for(std::list<RsPgpId>::iterator rit = pgpid_toRemove.begin(); rit != pgpid_toRemove.end(); ++rit)
			if (mFriendsPermissionFlags.end() != (it2 = mFriendsPermissionFlags.find(*rit))) 
				mFriendsPermissionFlags.erase(it2);

#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::removeFriend() new mFriendList.size() : " << mFriendList.size() << std::endl;
#endif
	}

	std::list<RsPeerId>::iterator rit;
	for(rit = sslid_toRemove.begin(); rit != sslid_toRemove.end(); ++rit)
	{
		mLinkMgr->removeFriend(*rit);
	}

	/* remove id from all groups */

	assignPeersToGroup("", pgpid_toRemove, false);

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


		std::map<RsPeerId, peerState>::iterator it;
		for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
		{
			out << "\t SSL ID: " << it->second.id;
			out << "\t GPG ID: " << it->second.gpg_id;
			out << std::endl;
		}

		out << "p3PeerMgrIMPL::printPeerLists() Others List";
		out << std::endl;
		for(it = mOthersList.begin(); it != mOthersList.end(); ++it)
		{
			out << "\t SSL ID: " << it->second.id;
			out << "\t GPG ID: " << it->second.gpg_id;
			out << std::endl;
		}
	}

	return;
}



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

bool 	p3PeerMgrIMPL::UpdateOwnAddress(const struct sockaddr_storage &localAddr, const struct sockaddr_storage &extAddr)
{
#ifdef PEER_DEBUG
    std::cerr << "p3PeerMgrIMPL::UpdateOwnAddress(";
    std::cerr << sockaddr_storage_tostring(localAddr);
    std::cerr << ", ";
    std::cerr << sockaddr_storage_tostring(extAddr);
    std::cerr << ")" << std::endl;
#endif

    if(!rsBanList->isAddressAccepted(localAddr, RSBANLIST_CHECKING_FLAGS_BLACKLIST))
    {
        std::cerr << "(SS) Trying to set own IP to a banned IP " << sockaddr_storage_iptostring(localAddr) << ". This probably means that a friend in under traffic re-routing attack." << std::endl;
        return false ;
    }

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
            std::cerr << sockaddr_storage_tostring(mOwnState.serveraddr);
            std::cerr << std::endl;
        }
        else if (mOwnState.netMode & RS_NET_MODE_EXT)
        {
            sockaddr_storage_copyip(mOwnState.serveraddr,extAddr);

            std::cerr << "p3PeerMgrIMPL::UpdateOwnAddress() Disabling Update of Server Port ";
            std::cerr << " as MANUAL FORWARD Mode";
            std::cerr << std::endl;
            std::cerr << "Address is Now: ";
            std::cerr << sockaddr_storage_tostring(mOwnState.serveraddr);
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




bool    p3PeerMgrIMPL::setLocalAddress(const RsPeerId &id, const struct sockaddr_storage &addr)
{
	bool changed = false;

	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		{
			RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
			if (!sockaddr_storage_same(mOwnState.localaddr, addr))
			{
				mOwnState.localaddr = addr;
				changed = true;
			}
		}

		if (changed)
		{
			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
			
			mNetMgr->setLocalAddress(addr);
			mLinkMgr->setLocalAddress(addr);
		}
		return changed;
	}

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	/* check if it is a friend */
	std::map<RsPeerId, peerState>::iterator it;
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
	if (!sockaddr_storage_same(it->second.localaddr, addr))
	{
		it->second.localaddr = addr;
		changed = true;
	}

#if 0
	//update ip address list
	IpAddressTimed ipAddressTimed;
	ipAddressTimed.ipAddr = addr;
	ipAddressTimed.seenTime = time(NULL);
	it->second.updateIpAddressList(ipAddressTimed);
#endif

	if (changed) {
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}

	return changed;
}

bool    p3PeerMgrIMPL::setExtAddress(const RsPeerId &id, const struct sockaddr_storage &addr)
{
	bool changed = false;

	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		{
			RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
			if (!sockaddr_storage_same(mOwnState.serveraddr, addr))
			{
				mOwnState.serveraddr = addr;
				changed = true;
			}
		}
		
		mNetMgr->setExtAddress(addr);
		
		return changed;
	}

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	/* check if it is a friend */
	std::map<RsPeerId, peerState>::iterator it;
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
	if (!sockaddr_storage_same(it->second.serveraddr, addr))
	{
		it->second.serveraddr = addr;
		changed = true;
	}

#if 0
	//update ip address list
	IpAddressTimed ipAddressTimed;
	ipAddressTimed.ipAddr = addr;
	ipAddressTimed.seenTime = time(NULL);
	it->second.updateIpAddressList(ipAddressTimed);
#endif

	if (changed) {
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}

	return changed;
}


bool p3PeerMgrIMPL::setDynDNS(const RsPeerId &id, const std::string &dyndns)
{
    bool changed = false;

    if (id == AuthSSL::getAuthSSL()->OwnId())
    {
        RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

        if (mOwnState.dyndns.compare(dyndns) != 0) {
            mOwnState.dyndns = dyndns;
            IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
            changed = true;
        }
        return changed;
    }

    RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
    /* check if it is a friend */
    std::map<RsPeerId, peerState>::iterator it;
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
    if (it->second.dyndns.compare(dyndns) != 0) {
        it->second.dyndns = dyndns;
        IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
        changed = true;
    }

    return changed;
}

bool p3PeerMgrIMPL::addCandidateForOwnExternalAddress(const RsPeerId &from, const sockaddr_storage &addr)
{
    //#ifdef PEER_DEBUG
    std::cerr << "Own external address is " << sockaddr_storage_iptostring(addr) << ", as reported by friend " << from << std::endl;
    //#endif

    // disconnect every friend that has reported a wrong external address

    sockaddr_storage own_addr ;

    if(mNetMgr->getExtAddress(own_addr) && !sockaddr_storage_sameip(own_addr,addr))
    {
        std::cerr << "(WW) peer reports an address that is not our current external address. This is weird." << std::endl;

        //mLinkMgr->disconnectFriend(from) ;
    }
    return true ;
}

bool    p3PeerMgrIMPL::updateAddressList(const RsPeerId& id, const pqiIpAddrSet &addrs)
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
	std::map<RsPeerId, peerState>::iterator it;
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
	std::string addrstr;
	it->second.ipAddrs.printAddrs(addrstr);
	std::cerr << addrstr;
	std::cerr << std::endl;
#endif

	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/

	return true;
}


bool    p3PeerMgrIMPL::updateCurrentAddress(const RsPeerId& id, const pqiIpAddress &addr)
{
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::updateCurrentAddress() called for id : " << id << std::endl;
#endif
	
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	
	/* cannot be own id */
	
	/* check if it is a friend */
	std::map<RsPeerId, peerState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
			std::cerr << "p3PeerMgrIMPL::updateCurrentAddress() ERROR peer id not found: " << id << std::endl;
			return false;
		}
	}

	if (sockaddr_storage_isPrivateNet(addr.mAddr))
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
	std::string addrstr;
	it->second.ipAddrs.printAddrs(addrstr);
	std::cerr << addrstr;
	std::cerr << std::endl;
#endif
	
	IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	
	return true;
}
	

bool    p3PeerMgrIMPL::updateLastContact(const RsPeerId& id)
{
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::updateLastContact() called for id : " << id << std::endl;
#endif
	
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	
	/* cannot be own id */
	
	/* check if it is a friend */
	std::map<RsPeerId, peerState>::iterator it;
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

bool    p3PeerMgrIMPL::setNetworkMode(const RsPeerId &id, uint32_t netMode)
{
	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		return setOwnNetworkMode(netMode);
	}

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	/* check if it is a friend */
	std::map<RsPeerId, peerState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
			return false;
		}
	}

	bool changed = false;

	/* "it" points to peer */
	if (it->second.netMode != netMode)
	{
		it->second.netMode = netMode;
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
		changed = true;
	}

	return changed;
}

bool    p3PeerMgrIMPL::setLocation(const RsPeerId &id, const std::string &location)
{
        bool changed = false;

        RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
        std::cerr << "p3PeerMgrIMPL::setLocation() called for id : " << id << "; with location " << location << std::endl;
#endif
        if (id == AuthSSL::getAuthSSL()->OwnId())
        {
            if (mOwnState.location.compare(location) != 0) {
                mOwnState.location = location;
                changed = true;
            }
            return changed;
        }

        /* check if it is a friend */
        std::map<RsPeerId, peerState>::iterator it;
        if (mFriendList.end() != (it = mFriendList.find(id))) {
            if (it->second.location.compare(location) != 0) {
                it->second.location = location;
                changed = true;
            }
        }
        return changed;
}

bool    p3PeerMgrIMPL::setVisState(const RsPeerId &id, uint16_t vs_disc, uint16_t vs_dht)
{
	{
		std::string out;
		rs_sprintf(out, "p3PeerMgr::setVisState(%s, %u, %u)", id.toStdString().c_str(), vs_disc, vs_dht);
		rslog(RSL_WARNING, p3peermgrzone, out);
	}

	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		return setOwnVisState(vs_disc, vs_dht);
	}

	bool isFriend = false;
	bool changed = false;
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		/* check if it is a friend */
		std::map<RsPeerId, peerState>::iterator it;
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
		if ((it->second.vs_disc != vs_disc) || (it->second.vs_dht = vs_dht))
		{
			it->second.vs_disc = vs_disc;
			it->second.vs_dht = vs_dht;
			changed = true;

#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::setVisState(" << id << ", DISC: " << vs_disc << " DHT: " << vs_dht << ") ";
			std::cerr << " NAME: " << it->second.name;

			switch(it->second.vs_disc)
			{
				default:
				case RS_VS_DISC_OFF:
					std::cerr << " NO-DISC ";
					break;
				case RS_VS_DISC_MINIMAL:
					std::cerr << " MIN-DISC ";
					break;
				case RS_VS_DISC_FULL:
					std::cerr << " FULL-DISC ";
					break;
			}
			switch(it->second.vs_dht)
			{
				default:
				case RS_VS_DHT_OFF:
					std::cerr << " NO-DHT ";
					break;
				case RS_VS_DHT_PASSIVE:
					std::cerr << " PASSIVE-DHT ";
					break;
				case RS_VS_DHT_FULL:
					std::cerr << " FULL-DHT ";
					break;
			}
			std::cerr << std::endl;
#endif
		}
	}
	if(isFriend && changed)
	{
		mLinkMgr->setFriendVisibility(id, vs_dht != RS_VS_DHT_OFF);
	}

	if (changed) {
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
	}

	return changed;
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

	// Store Proxy Server.
	struct sockaddr_storage proxy_addr;
	getProxyServerAddress(proxy_addr);

	mPeerMtx.lock(); /****** MUTEX LOCKED *******/ 

	RsPeerNetItem *item = new RsPeerNetItem();
	item->clear();

	item->peerId = getOwnId();
	item->pgpId = mOwnState.gpg_id;
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

	item->vs_disc = mOwnState.vs_disc;
	item->vs_dht = mOwnState.vs_dht;
	
	item->lastContact = mOwnState.lastcontact;

	item->localAddrV4.addr = mOwnState.localaddr;
	item->extAddrV4.addr = mOwnState.serveraddr;
	sockaddr_storage_clear(item->localAddrV6.addr);
	sockaddr_storage_clear(item->extAddrV6.addr);
	
	item->dyndns = mOwnState.dyndns;
	mOwnState.ipAddrs.mLocal.loadTlv(item->localAddrList);
	mOwnState.ipAddrs.mExt.loadTlv(item->extAddrList);
	item->domain_addr = mOwnState.hiddenDomain;
	item->domain_port = mOwnState.hiddenPort;

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::saveList() Own Config Item:" << std::endl;
	item->print(std::cerr, 10);
	std::cerr << std::endl;
#endif

	saveData.push_back(item);
	saveCleanupList.push_back(item);

	/* iterate through all friends and save */
        std::map<RsPeerId, peerState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
	{
		item = new RsPeerNetItem();
		item->clear();

		item->peerId = it->first;
		item->pgpId = (it->second).gpg_id;
		item->location = (it->second).location;
		item->netMode = (it->second).netMode;
		item->vs_disc = (it->second).vs_disc;
		item->vs_dht = (it->second).vs_dht;

		item->lastContact = (it->second).lastcontact;
		
		item->localAddrV4.addr = (it->second).localaddr;
		item->extAddrV4.addr = (it->second).serveraddr;
		sockaddr_storage_clear(item->localAddrV6.addr);
		sockaddr_storage_clear(item->extAddrV6.addr);
		
		
		item->dyndns = (it->second).dyndns;
		(it->second).ipAddrs.mLocal.loadTlv(item->localAddrList);
		(it->second).ipAddrs.mExt.loadTlv(item->extAddrList);

		item->domain_addr = (it->second).hiddenDomain;
		item->domain_port = (it->second).hiddenPort;

		saveData.push_back(item);
		saveCleanupList.push_back(item);
#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::saveList() Peer Config Item:" << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
#endif
	}

	RsPeerServicePermissionItem *sitem = new RsPeerServicePermissionItem ;

	for(std::map<RsPgpId,ServicePermissionFlags>::const_iterator it(mFriendsPermissionFlags.begin());it!=mFriendsPermissionFlags.end();++it)
	{
		sitem->pgp_ids.push_back(it->first) ;
		sitem->service_flags.push_back(it->second) ;
	}

	saveData.push_back(sitem) ;
	saveCleanupList.push_back(sitem);

	// Now save config for network digging strategies
	
	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;

	RsTlvKeyValue kv;
	kv.key = kConfigKeyExtIpFinder;
	kv.value = (useExtAddrFinder)?"TRUE":"FALSE" ;
	vitem->tlvkvs.pairs.push_back(kv) ;


#ifdef PEER_DEBUG
	std::cerr << "Saving proxyServerAddress: " << sockaddr_storage_tostring(proxy_addr);
	std::cerr << std::endl;
#endif

	kv.key = kConfigKeyProxyServerIpAddr;
	kv.value = sockaddr_storage_iptostring(proxy_addr);
	vitem->tlvkvs.pairs.push_back(kv) ;

	kv.key = kConfigKeyProxyServerPort;
	kv.value = sockaddr_storage_porttostring(proxy_addr);
	vitem->tlvkvs.pairs.push_back(kv) ;
	
	saveData.push_back(vitem);
	saveCleanupList.push_back(vitem);

	/* save groups */

	std::list<RsPeerGroupItem *>::iterator groupIt;
	for (groupIt = groupList.begin(); groupIt != groupList.end(); ++groupIt) {
		saveData.push_back(*groupIt); // no delete
	}

	return true;
}

void    p3PeerMgrIMPL::saveDone()
{
	/* clean up the save List */
	std::list<RsItem *>::iterator it;
	for(it = saveCleanupList.begin(); it != saveCleanupList.end(); ++it)
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
	std::string proxyIpAddress = kConfigDefaultProxyServerIpAddr;
	uint16_t    proxyPort = kConfigDefaultProxyServerPort;

        if (load.empty()) {
            std::cerr << "p3PeerMgrIMPL::loadList() list is empty, it may be a configuration problem."  << std::endl;
            return false;
        }

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::loadList() Item Count: " << load.size() << std::endl;
#endif

	RsPeerId ownId = getOwnId();

	/* load the list of peers */
	std::list<RsItem *>::iterator it;
	for(it = load.begin(); it != load.end(); ++it)
	{
		RsPeerNetItem *pitem = dynamic_cast<RsPeerNetItem *>(*it);
		if (pitem)
		{
			RsPeerId peer_id = pitem->peerId ;
			RsPgpId peer_pgp_id = pitem->pgpId ;

			if (peer_id == ownId)
			{
#ifdef PEER_DEBUG
				std::cerr << "p3PeerMgrIMPL::loadList() Own Config Item:" << std::endl;
				pitem->print(std::cerr, 10);
				std::cerr << std::endl;
#endif
				/* add ownConfig */
				setOwnNetworkMode(pitem->netMode);
                setOwnVisState(pitem->vs_disc, pitem->vs_dht);

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
                // permission flags is used as a mask for the existing perms, so we set it to 0xffff
                addFriend(peer_id, peer_pgp_id, pitem->netMode, pitem->vs_disc, pitem->vs_dht, pitem->lastContact, RS_NODE_PERM_ALL);
				setLocation(pitem->peerId, pitem->location);
			}

			if (pitem->netMode == RS_NET_MODE_HIDDEN)
			{
				/* set only the hidden stuff & localAddress */
				setLocalAddress(peer_id, pitem->localAddrV4.addr);
				setHiddenDomainPort(peer_id, pitem->domain_addr, pitem->domain_port);

			}
			else
			{
				setLocalAddress(peer_id, pitem->localAddrV4.addr);
                setExtAddress(peer_id, pitem->extAddrV4.addr);
				setDynDNS (peer_id, pitem->dyndns);

				/* convert addresses */
				pqiIpAddrSet addrs;
				addrs.mLocal.extractFromTlv(pitem->localAddrList);
				addrs.mExt.extractFromTlv(pitem->extAddrList);
			
				updateAddressList(peer_id, addrs);
			}

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
			for(kit = vitem->tlvkvs.pairs.begin(); kit != vitem->tlvkvs.pairs.end(); ++kit)
			{
				if (kit->key == kConfigKeyExtIpFinder)
				{
					useExtAddrFinder = (kit->value == "TRUE");
#ifdef PEER_DEBUG
					std::cerr << "setting use_extr_addr_finder to " << useExtAddrFinder << std::endl ;
#endif
				} 
				else if (kit->key == kConfigKeyProxyServerIpAddr)
				{
					proxyIpAddress = kit->value;
#ifdef PEER_DEBUG
					std::cerr << "Loaded proxyIpAddress: " << proxyIpAddress;
					std::cerr << std::endl ;
#endif
					
				}
				else if (kit->key == kConfigKeyProxyServerPort)
				{
					proxyPort = atoi(kit->value.c_str());
#ifdef PEER_DEBUG
					std::cerr << "Loaded proxyPort: " << proxyPort;
					std::cerr << std::endl ;
#endif
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

#ifdef PEER_DEBUG
			std::cerr << "Loaded service permission item: " << std::endl;
#endif

			for(uint32_t i=0;i<sitem->pgp_ids.size();++i)
				if(AuthGPG::getAuthGPG()->isGPGAccepted(sitem->pgp_ids[i]) || sitem->pgp_ids[i] == AuthGPG::getAuthGPG()->getGPGOwnId())
				{
					mFriendsPermissionFlags[sitem->pgp_ids[i]] = sitem->service_flags[i] ;
#ifdef PEER_DEBUG
					std::cerr << "   " << sitem->pgp_ids[i] << " - " << sitem->service_flags[i] << std::endl;
#endif
				}
#ifdef PEER_DEBUG
				else
					std::cerr << "   " << sitem->pgp_ids[i] << " - Not a friend!" << std::endl;
#endif
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
		for (groupIt = groupList.begin(); groupIt != groupList.end(); ++groupIt) {
			if ((*groupIt)->flag & RS_GROUP_FLAG_STANDARD) {
				int i;
				for (i = 0; i < standardGroupCount; ++i) {
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
		for (int i = 0; i < standardGroupCount; ++i) {
			if (foundStandardGroup[i] == false) {
				RsPeerGroupItem *gitem = new RsPeerGroupItem;
				gitem->id = standardGroup[i];
				gitem->name = standardGroup[i];
				gitem->flag |= RS_GROUP_FLAG_STANDARD;
				groupList.push_back(gitem);
			}
		}
	}

	// If we are hidden - don't want ExtAddrFinder - ever!
	if (isHidden())
	{
		useExtAddrFinder = false;
	}

	mNetMgr->setIPServersEnabled(useExtAddrFinder);

	// Configure Proxy Server.
	struct sockaddr_storage proxy_addr;
	sockaddr_storage_clear(proxy_addr);
	sockaddr_storage_ipv4_aton(proxy_addr, proxyIpAddress.c_str());
	sockaddr_storage_ipv4_setport(proxy_addr, proxyPort);

	if (sockaddr_storage_isValidNet(proxy_addr))
	{
		setProxyServerAddress(proxy_addr);
	}

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

	RsServer::notify()->notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_ADD);

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
		for (groupIt = groupList.begin(); groupIt != groupList.end(); ++groupIt) {
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
		RsServer::notify()->notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_MOD);

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
		for (groupIt = groupList.begin(); groupIt != groupList.end(); ++groupIt) {
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
		RsServer::notify()->notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_DEL);

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
	for (groupIt = groupList.begin(); groupIt != groupList.end(); ++groupIt) {
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
	for (groupIt = groupList.begin(); groupIt != groupList.end(); ++groupIt) {
		RsGroupInfo groupInfo;
		(*groupIt)->get(groupInfo);
		groupInfoList.push_back(groupInfo);
	}

	return true;
}

// groupId == "" && assign == false -> remove from all groups
bool p3PeerMgrIMPL::assignPeersToGroup(const std::string &groupId, const std::list<RsPgpId> &peerIds, bool assign)
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
		for (groupIt = groupList.begin(); groupIt != groupList.end(); ++groupIt) {
			if (groupId.empty() || (*groupIt)->id == groupId) {
				RsPeerGroupItem *groupItem = *groupIt;

				std::list<RsPgpId>::const_iterator peerIt;
				for (peerIt = peerIds.begin(); peerIt != peerIds.end(); ++peerIt) {
                    std::set<RsPgpId>::iterator peerIt1 = std::find(groupItem->pgpList.ids.begin(), groupItem->pgpList.ids.end(), *peerIt);
					if (assign) {
						if (peerIt1 == groupItem->pgpList.ids.end()) {
                            groupItem->pgpList.ids.insert(*peerIt);
							changed = true;
						}
					} else {
						if (peerIt1 != groupItem->pgpList.ids.end()) {
							groupItem->pgpList.ids.erase(peerIt1);
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
		RsServer::notify()->notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_MOD);

		IndicateConfigChanged();
	}

	return changed;
}


/**********************************************************************
 **********************************************************************
 ******************** Service permission stuff ************************
 **********************************************************************
 **********************************************************************/

ServicePermissionFlags p3PeerMgrIMPL::servicePermissionFlags(const RsPeerId& ssl_id)
{
	RsPgpId gpg_id ;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		std::map<RsPeerId, peerState>::const_iterator it = mFriendList.find(ssl_id);

		if(it == mFriendList.end())
            return RS_NODE_PERM_DEFAULT ;

		gpg_id = it->second.gpg_id ;
	}

	return servicePermissionFlags(gpg_id) ;
}


ServicePermissionFlags p3PeerMgrIMPL::servicePermissionFlags(const RsPgpId& pgp_id)
{
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		std::map<RsPgpId,ServicePermissionFlags>::const_iterator it = mFriendsPermissionFlags.find( pgp_id ) ;

		if(it == mFriendsPermissionFlags.end())
            return RS_NODE_PERM_DEFAULT ;
		else
			return it->second ;
	}
}
void p3PeerMgrIMPL::setServicePermissionFlags(const RsPgpId& pgp_id, const ServicePermissionFlags& flags)
{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		// Check that we have a PGP id. This should not be necessary, but because
		// we use std::string, anything can get passed down here.
		//

		mFriendsPermissionFlags[pgp_id] = flags ;
		IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
}

/**********************************************************************
 **********************************************************************
 ******************** Stuff moved from p3peers ************************
 **********************************************************************
 **********************************************************************/

bool p3PeerMgrIMPL::removeAllFriendLocations(const RsPgpId &gpgid)
{
	std::list<RsPeerId> sslIds;
	if (!getAssociatedPeers(gpgid, sslIds))
	{
		return false;
	}
	
	std::list<RsPeerId>::iterator it;
	for(it = sslIds.begin(); it != sslIds.end(); ++it)
	{
		removeFriend(*it, true);
	}
	
	return true;
}


bool	p3PeerMgrIMPL::getAssociatedPeers(const RsPgpId &gpg_id, std::list<RsPeerId> &ids)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgr::getAssociatedPeers() for id : " << gpg_id << std::endl;
#endif
	
	int count = 0;
	std::map<RsPeerId, peerState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
	{
		if (it->second.gpg_id == gpg_id)
		{
			count++;
			ids.push_back(it->first);

#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgr::getAssociatedPeers() found ssl id :  " << it->first << std::endl;
#endif
			
		}
	}
	
	return (count > 0);
}

// goes through the list of known friend IPs and remove the ones that are banned by p3LinkMgr.

static bool cleanIpList(std::list<pqiIpAddress>& lst,const RsPeerId& pid,p3LinkMgr *link_mgr)
{
    bool changed = false ;
    time_t now = time(NULL) ;

    for(std::list<pqiIpAddress>::iterator it2(lst.begin());it2 != lst.end();)
    {
#ifdef PEER_DEBUG
      std::cerr << "Checking IP address " << sockaddr_storage_iptostring( (*it2).mAddr) << " for peer " << pid << ", age = " << now - (*it2).mSeenTime << std::endl;
#endif
        if(!link_mgr->checkPotentialAddr( (*it2).mAddr,now - (*it2).mSeenTime))
        {
#ifdef PEER_DEBUG
            std::cerr << "  ==> Removing Banned/old IP address " << sockaddr_storage_iptostring( (*it2).mAddr) << " from peer " << pid << ", age = " << now - (*it2).mSeenTime << std::endl;
#endif

            std::list<pqiIpAddress>::iterator ittmp = it2 ;
            ++ittmp ;
            lst.erase(it2) ;
            it2 = ittmp ;

            changed = true ;
        }
        else
            ++it2 ;
    }

    return changed ;
}

bool p3PeerMgrIMPL::removeBannedIps()
{
    RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

    bool changed = false ;
    for( std::map<RsPeerId, peerState>::iterator it = mFriendList.begin(); it != mFriendList.end(); ++it)
    {
        if(cleanIpList(it->second.ipAddrs.mExt.mAddrs,it->first,mLinkMgr)) changed = true ;
        if(cleanIpList(it->second.ipAddrs.mLocal.mAddrs,it->first,mLinkMgr)) changed = true ;
    }

    if(cleanIpList(mOwnState.ipAddrs.mExt.mAddrs,mOwnState.id,mLinkMgr) ) changed = true ;
    if(cleanIpList(mOwnState.ipAddrs.mLocal.mAddrs,mOwnState.id,mLinkMgr) )  changed = true ;

    if(changed)
        IndicateConfigChanged();

    return true ;
}

// /* This only removes SSL certs, that are old... Can end up with no Certs per GPG Id 
//  * We are removing the concept of a "DummyId" - There is no need for it.
//  */
// 
// bool isDummyFriend(RsPeerId id)
// {
// 	bool ret = (id.substr(0,5) == "dummy");
// 	return ret;
// }


bool p3PeerMgrIMPL::removeUnusedLocations()
{
	std::list<RsPeerId> toRemove;
	
	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	
#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgr::removeUnusedLocations()" << std::endl;
#endif
		
		time_t now = time(NULL);
		
		std::map<RsPeerId, peerState>::iterator it;
		for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
		{
			if (now - it->second.lastcontact > VERY_OLD_PEER)
			{
				toRemove.push_back(it->first);

#ifdef PEER_DEBUG
                std::cerr << "p3PeerMgr::removeUnusedLocations() removing Old SSL Id: " << it->first << std::endl;
#endif
				
			}
			
//			if (isDummyFriend(it->first))
//			{
//				toRemove.push_back(it->first);
//				
//#ifdef PEER_DEBUG
//				std::cerr << "p3PeerMgr::removeUnusedLocations() removing Dummy Id: " << it->first << std::endl;
//#endif
//				
//			}
			
		}
	}
	std::list<RsPeerId>::iterator it;
	
	for(it = toRemove.begin(); it != toRemove.end(); ++it)
	{
		removeFriend(*it,false);
	}

	return true;
}

	
