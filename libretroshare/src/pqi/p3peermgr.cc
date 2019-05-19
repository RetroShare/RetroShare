/*******************************************************************************
 * libretroshare/src/pqi: p3peermgr.cc                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2007-2011  Robert Fernie                                      *
 * Copyright (C) 2015-2018  Gioacchino Mazzurco <gio@eigenlab.org>             *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include <vector>    // for std::vector
#include <algorithm> // for std::random_shuffle

#include "rsserver/p3face.h"
#include "util/rsnet.h"
#include "pqi/authgpg.h"
#include "pqi/authssl.h"

#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"
#include "pqi/p3historymgr.h"
#include "pqi/pqinetwork.h"        // for getLocalAddresses

//#include "pqi/p3dhtmgr.h" // Only need it for constants.
//#include "tcponudp/tou.h"
//#include "util/extaddrfinder.h"
//#include "util/dnsresolver.h"

#include "util/rsprint.h"
#include "util/rsstring.h"
#include "util/rsdebug.h"

#include "rsitems/rsconfigitems.h"

#include "retroshare/rsiface.h" // Needed for rsicontrol (should remove this dependancy)
#include "retroshare/rspeers.h" // Needed for Group Parameters.
#include "retroshare/rsbanlist.h" // Needed for banned IPs

/* Network setup States */

//Defined and used in /libretroshare/src/pqi/p3netmgr.cc
//const uint32_t RS_NET_NEEDS_RESET = 	0x0000;
//const uint32_t RS_NET_UNKNOWN = 	0x0001;
//const uint32_t RS_NET_UPNP_INIT = 	0x0002;
//const uint32_t RS_NET_UPNP_SETUP =  	0x0003;
//const uint32_t RS_NET_EXT_SETUP =  	0x0004;
//const uint32_t RS_NET_DONE =    	0x0005;
//const uint32_t RS_NET_LOOPBACK =    	0x0006;
//const uint32_t RS_NET_DOWN =    	0x0007;

//const uint32_t MIN_TIME_BETWEEN_NET_RESET = 		5;

//const uint32_t PEER_IP_CONNECT_STATE_MAX_LIST_SIZE =     	4;

static struct RsLog::logInfo p3peermgrzoneInfo = {RsLog::Default, "p3peermgr"};
#define p3peermgrzone &p3peermgrzoneInfo

/****
 * #define PEER_DEBUG 1
 * #define PEER_DEBUG_LOG 1
 ***/

#define MAX_AVAIL_PERIOD 230 //times a peer stay in available state when not connected
#define MIN_RETRY_PERIOD 140

static const std::string kConfigDefaultProxyServerIpAddr = "127.0.0.1";
static const uint16_t    kConfigDefaultProxyServerPortTor = 9050; // standard port.
static const uint16_t    kConfigDefaultProxyServerPortI2P = 4447; // I2Pd's standard port

static const std::string kConfigKeyExtIpFinder = "USE_EXTR_IP_FINDER";
static const std::string kConfigKeyProxyServerIpAddrTor = "PROXY_SERVER_IPADDR";
static const std::string kConfigKeyProxyServerPortTor = "PROXY_SERVER_PORT";
static const std::string kConfigKeyProxyServerIpAddrI2P = "PROXY_SERVER_IPADDR_I2P";
static const std::string kConfigKeyProxyServerPortI2P = "PROXY_SERVER_PORT_I2P";

void  printConnectState(std::ostream &out, peerState &peer);

peerState::peerState()
	:netMode(RS_NET_MODE_UNKNOWN), vs_disc(RS_VS_DISC_FULL), vs_dht(RS_VS_DHT_FULL), lastcontact(0),
	 hiddenNode(false), hiddenPort(0), hiddenType(RS_HIDDEN_TYPE_NONE)
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

		// setup default ProxyServerAddress.
		// Tor
		sockaddr_storage_clear(mProxyServerAddressTor);
		sockaddr_storage_ipv4_aton(mProxyServerAddressTor,
				kConfigDefaultProxyServerIpAddr.c_str());
		sockaddr_storage_ipv4_setport(mProxyServerAddressTor,
				kConfigDefaultProxyServerPortTor);
		// I2P
		sockaddr_storage_clear(mProxyServerAddressI2P);
		sockaddr_storage_ipv4_aton(mProxyServerAddressI2P,
				kConfigDefaultProxyServerIpAddr.c_str());
		sockaddr_storage_ipv4_setport(mProxyServerAddressI2P,
				kConfigDefaultProxyServerPortI2P);

		mProxyServerStatusTor = RS_NET_PROXY_STATUS_UNKNOWN ;
		mProxyServerStatusI2P = RS_NET_PROXY_STATUS_UNKNOWN;
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
		mOwnState.hiddenType = hiddenDomainToHiddenType(hiddenAddress);
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
		mOwnState.hiddenType = hiddenDomainToHiddenType(mOwnState.hiddenDomain);

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
    static const rstime_t INTERVAL_BETWEEN_LOCATION_CLEANING = 300 ; // Remove unused locations and clean IPs every 10 minutes.

    static rstime_t last_friends_check = time(NULL) ; // first cleaning after 1 hour.

    rstime_t now = time(NULL) ;

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
	RS_STACK_MUTEX(mPeerMtx);
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

bool p3PeerMgrIMPL::isHidden()
{
	RS_STACK_MUTEX(mPeerMtx);
	return mOwnState.hiddenNode;
}

/**
 * @brief checks the hidden type of the own peer.
 * @param type type to check
 * @return true when the peer has the same hidden type than type
 */
bool	p3PeerMgrIMPL::isHidden(const uint32_t type)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/
	switch (type) {
	case RS_HIDDEN_TYPE_TOR:
		return mOwnState.hiddenType == RS_HIDDEN_TYPE_TOR;
		break;
	case RS_HIDDEN_TYPE_I2P:
		return mOwnState.hiddenType == RS_HIDDEN_TYPE_I2P;
		break;
	default:
#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::isHidden(" << type << ") unkown type -> false";
		std::cerr << std::endl;
#endif
		return false;
		break;
	}
}

bool    p3PeerMgrIMPL::isHiddenPeer(const RsPeerId &ssl_id)
{
	return isHiddenPeer(ssl_id, RS_HIDDEN_TYPE_NONE);
}

/**
 * @brief checks the hidden type of a given ssl id. When type RS_HIDDEN_TYPE_NONE is choosen it returns the 'hiddenNode' value instead
 * @param ssl_id to check
 * @param type type to check. Use RS_HIDDEN_TYPE_NONE to check 'hiddenNode' value
 * @return true when the peer has the same hidden type than type
 */
bool	p3PeerMgrIMPL::isHiddenPeer(const RsPeerId &ssl_id, const uint32_t type)
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
	switch (type) {
	case RS_HIDDEN_TYPE_TOR:
		return (it->second).hiddenType == RS_HIDDEN_TYPE_TOR;
		break;
	case RS_HIDDEN_TYPE_I2P:
		return (it->second).hiddenType == RS_HIDDEN_TYPE_I2P;
		break;
	default:
		return (it->second).hiddenNode;
		break;
	}
}

bool hasEnding (std::string const &fullString, std::string const &ending) {
	if (fullString.length() < ending.length())
		return false;

	return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
}

/**
 * @brief resolves the hidden type (tor or i2p) from a domain
 * @param domain to check
 * @return RS_HIDDEN_TYPE_TOR, RS_HIDDEN_TYPE_I2P or RS_HIDDEN_TYPE_NONE
 *
 * Tor: ^[a-z2-7]{16}\.onion$
 *
 * I2P: There is more than one address:
 *       - pub. key in base64
 *       - hash in base32 ( ^[a-z2-7]{52}\.b32\.i2p$ )
 *       - "normal" .i2p domains
 */
uint32_t p3PeerMgrIMPL::hiddenDomainToHiddenType(const std::string &domain)
{
	if(hasEnding(domain, ".onion"))
		return RS_HIDDEN_TYPE_TOR;
	if(hasEnding(domain, ".i2p"))
		return RS_HIDDEN_TYPE_I2P;

#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::hiddenDomainToHiddenType() unknown hidden type: " << domain;
		std::cerr << std::endl;
#endif
	return RS_HIDDEN_TYPE_UNKNOWN;
}

/**
 * @brief returns the hidden type of a peer
 * @param ssl_id peer id
 * @return hidden type
 */
uint32_t p3PeerMgrIMPL::getHiddenType(const RsPeerId &ssl_id)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	if (ssl_id == AuthSSL::getAuthSSL()->OwnId())
		return mOwnState.hiddenType;

	/* check for existing */
	std::map<RsPeerId, peerState>::iterator it;
	it = mFriendList.find(ssl_id);
	if (it == mFriendList.end())
	{
#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::getHiddenType(" << ssl_id << ") Missing Peer => false";
		std::cerr << std::endl;
#endif

		return false;
	}

#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::getHiddenType(" << ssl_id << ") = " << (it->second).hiddenType;
	std::cerr << std::endl;
#endif
	return (it->second).hiddenType;
}

bool p3PeerMgrIMPL::isHiddenNode(const RsPeerId& id)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	if (id == AuthSSL::getAuthSSL()->OwnId())
		return mOwnState.hiddenNode ;
	else
	{
		std::map<RsPeerId,peerState>::const_iterator it = mFriendList.find(id);

		if (it == mFriendList.end())
		{
			std::cerr << "p3PeerMgrIMPL::isHiddenNode() Peer Not Found" << std::endl;
			return false;
		}
		return it->second.hiddenNode ;
	}
}

/**
 * @brief sets hidden domain and port for a given ssl ID
 * @param ssl_id peer to set domain and port for
 * @param domain_addr
 * @param domain_port
 * @return true on success
 */
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
		mOwnState.hiddenType = hiddenDomainToHiddenType(domain);
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
	it->second.hiddenType = hiddenDomainToHiddenType(domain);
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::setHiddenDomainPort() Set Peers State";
	std::cerr << std::endl;
#endif

	return true;
}

/**
 * @brief sets the proxy server address for a hidden service
 * @param type hidden service type
 * @param proxy_addr proxy address
 * @return true on success
 */
bool p3PeerMgrIMPL::setProxyServerAddress(const uint32_t type, const struct sockaddr_storage &proxy_addr)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	switch (type) {
	case RS_HIDDEN_TYPE_I2P:
		if (!sockaddr_storage_same(mProxyServerAddressI2P, proxy_addr))
		{
			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
			mProxyServerAddressI2P = proxy_addr;
		}
		break;
	case RS_HIDDEN_TYPE_TOR:
		if (!sockaddr_storage_same(mProxyServerAddressTor, proxy_addr))
		{
			IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
			mProxyServerAddressTor = proxy_addr;
		}
		break;
	case RS_HIDDEN_TYPE_UNKNOWN:
	default:
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::setProxyServerAddress() unknown hidden type " << type << " -> false";
	std::cerr << std::endl;
#endif
		return false;
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

/**
 * @brief returs proxy server status for a hidden service proxy
 * @param type hidden service type
 * @param proxy_status
 * @return true on success
 */
bool p3PeerMgrIMPL::getProxyServerStatus(const uint32_t type, uint32_t& proxy_status)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	switch (type) {
	case RS_HIDDEN_TYPE_I2P:
		proxy_status = mProxyServerStatusI2P;
		break;
	case RS_HIDDEN_TYPE_TOR:
		proxy_status = mProxyServerStatusTor;
		break;
	case RS_HIDDEN_TYPE_UNKNOWN:
	default:
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::getProxyServerStatus() unknown hidden type " << type << " -> false";
	std::cerr << std::endl;
#endif
		return false;
	}

	return true;
}

/**
 * @brief returs proxy server address for a hidden service proxy
 * @param type hidden service type
 * @param proxy_addr
 * @return true on success
 */
bool p3PeerMgrIMPL::getProxyServerAddress(const uint32_t type, struct sockaddr_storage &proxy_addr)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	switch (type) {
	case RS_HIDDEN_TYPE_I2P:
		proxy_addr = mProxyServerAddressI2P;
		break;
	case RS_HIDDEN_TYPE_TOR:
		proxy_addr = mProxyServerAddressTor;
		break;
	case RS_HIDDEN_TYPE_UNKNOWN:
	default:
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::getProxyServerAddress() unknown hidden type " << type << " -> false";
	std::cerr << std::endl;
#endif
		return false;
	}
	return true;
}

/**
 * @brief looks up the proxy address and domain/port that have to be used when connecting to a peer
 * @param ssl_id peer to connect to
 * @param proxy_addr proxy address to be used
 * @param domain_addr domain to connect to
 * @param domain_port port to connect to
 * @return true on success
 */
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

	switch (it->second.hiddenType) {
	case RS_HIDDEN_TYPE_I2P:
		proxy_addr = mProxyServerAddressI2P;
		break;
	case RS_HIDDEN_TYPE_TOR:
		proxy_addr = mProxyServerAddressTor;
		break;
	case RS_HIDDEN_TYPE_UNKNOWN:
	default:
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::getProxyAddress() no valid hidden type (" << it->second.hiddenType << ") for peer id " << ssl_id << " -> false";
	std::cerr << std::endl;
#endif
		return false;
	}
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
	RS_STACK_MUTEX(mPeerMtx);

	/* check for existing */
	std::map<RsPeerId, peerState>::iterator it;
	it = mFriendList.find(id);
	if (it == mFriendList.end()) return false;

	state = it->second;
	return true;
}


bool p3PeerMgrIMPL::getOthersNetStatus(const RsPeerId &id, peerState &state)
{
	RS_STACK_MUTEX(mPeerMtx);

	/* check for existing */
	std::map<RsPeerId, peerState>::iterator it;
	it = mOthersList.find(id);
	if (it == mOthersList.end()) return false;

	state = it->second;
	return true;
}

int p3PeerMgrIMPL::getConnectAddresses(
        const RsPeerId &id, sockaddr_storage &lAddr, sockaddr_storage &eAddr,
                    pqiIpAddrSet &histAddrs, std::string &dyndns )
{

	RS_STACK_MUTEX(mPeerMtx);

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

bool p3PeerMgrIMPL::addFriend(const RsPeerId& input_id, const RsPgpId& input_gpg_id, uint32_t netMode, uint16_t vs_disc, uint16_t vs_dht, rstime_t lastContact,ServicePermissionFlags service_flags)
{
	bool notifyLinkMgr = false;
	RsPeerId id = input_id ;
	RsPgpId gpg_id = input_gpg_id ;

#ifdef PEER_DEBUG_LOG
	rslog(RSL_WARNING, p3peermgrzone, "p3PeerMgr::addFriend() id: " + id.toStdString());
#endif
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
#ifdef RS_CHATSERVER //Defined by chatserver
	setServicePermissionFlags(gpg_id,RS_NODE_PERM_NONE) ;
#else
	setServicePermissionFlags(gpg_id,service_flags) ;
#endif

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
        //bool success = false;
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

				//success = true;
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
    assignPeersToGroup(RsNodeGroupId(), ids, false);

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
		//bool success = false;
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

				//success = true;
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

    assignPeersToGroup(RsNodeGroupId(), pgpid_toRemove, false);

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

bool p3PeerMgrIMPL::UpdateOwnAddress( const sockaddr_storage& pLocalAddr,
                                      const sockaddr_storage& pExtAddr )
{
	sockaddr_storage localAddr;
	sockaddr_storage_copy(pLocalAddr, localAddr);
	sockaddr_storage_ipv6_to_ipv4(localAddr);

	sockaddr_storage extAddr;
	sockaddr_storage_copy(pExtAddr, extAddr);
	sockaddr_storage_ipv6_to_ipv4(extAddr);

//#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::UpdateOwnAddress("
	          << sockaddr_storage_tostring(localAddr) << ", "
	          << sockaddr_storage_tostring(extAddr) << ")" << std::endl;
//#endif

	if( rsBanList &&
	         !rsBanList->isAddressAccepted(localAddr,
	                                       RSBANLIST_CHECKING_FLAGS_BLACKLIST) )
	{
		std::cerr << "(SS) Trying to set own IP to a banned IP "
		          << sockaddr_storage_iptostring(localAddr) << ". This probably"
		          << "means that a friend in under traffic re-routing attack."
		          << std::endl;
		return false;
	}

	{
		RS_STACK_MUTEX(mPeerMtx);

		//update ip address list
		pqiIpAddress ipAddressTimed;
		sockaddr_storage_copy(localAddr, ipAddressTimed.mAddr);
		ipAddressTimed.mSeenTime = time(NULL);
		ipAddressTimed.mSrc = 0;
		mOwnState.ipAddrs.updateLocalAddrs(ipAddressTimed);

		if(!mOwnState.hiddenNode)
		{
			/* Workaround to spread multiple local ip addresses when presents.
			 * This is needed because RS wrongly assumes that there is just one
			 * active local ip address at time. */
			std::vector<sockaddr_storage> addrs;
			if(getLocalAddresses(addrs))
			{
				/* To work around MAX_ADDRESS_LIST_SIZE addresses limitation,
				 *  let's shuffle the list of local addresses in the hope that
				 *  with enough time every local address is advertised to
				 *  trusted nodes so they may try to connect to all of them
				 *  including the most convenient if a local connection exists.
				 */
				std::random_shuffle(addrs.begin(), addrs.end());

				for (auto it = addrs.begin(); it!=addrs.end(); ++it)
				{
					sockaddr_storage& addr(*it);
					if( sockaddr_storage_isValidNet(addr) &&
					    !sockaddr_storage_isLoopbackNet(addr) &&
					    /* Avoid IPv6 link local addresses as we don't have
						 * implemented the logic needed to handle sin6_scope_id.
						 * To properly handle sin6_scope_id it would probably
						 * require deep reenginering of the RetroShare
						 * networking stack */
					    !sockaddr_storage_ipv6_isLinkLocalNet(addr) )
					{
						sockaddr_storage_ipv6_to_ipv4(addr);
						pqiIpAddress pqiIp;
						sockaddr_storage_clear(pqiIp.mAddr);
						pqiIp.mAddr.ss_family = addr.ss_family;
						sockaddr_storage_copyip(pqiIp.mAddr, addr);
						sockaddr_storage_setport(
						            pqiIp.mAddr,
						            sockaddr_storage_port(localAddr) );
						pqiIp.mSeenTime = time(nullptr);
						pqiIp.mSrc = 0;
						mOwnState.ipAddrs.updateLocalAddrs(pqiIp);
					}
				}
			}
		}

		sockaddr_storage_copy(localAddr, mOwnState.localaddr);
	}


	{
		RS_STACK_MUTEX(mPeerMtx);

		//update ip address list
		pqiIpAddress ipAddressTimed;
		sockaddr_storage_copy(extAddr, ipAddressTimed.mAddr);
		ipAddressTimed.mSeenTime = time(NULL);
		ipAddressTimed.mSrc = 0;
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
			sockaddr_storage_copyip(mOwnState.serveraddr, extAddr);

            std::cerr << "p3PeerMgrIMPL::UpdateOwnAddress() Disabling Update of Server Port ";
            std::cerr << " as MANUAL FORWARD Mode";
            std::cerr << std::endl;
            std::cerr << "Address is Now: ";
            std::cerr << sockaddr_storage_tostring(mOwnState.serveraddr);
            std::cerr << std::endl;
        }
        else
		{
			sockaddr_storage_copy(extAddr, mOwnState.serveraddr);
        }
    }

    IndicateConfigChanged(); /**** INDICATE MSG CONFIG CHANGED! *****/
    mLinkMgr->setLocalAddress(localAddr);

    return true;
}


bool p3PeerMgrIMPL::addPeerLocator(const RsPeerId &sslId, const RsUrl& locator)
{
	std::string host(locator.host());
	pqiIpAddress ip;
	if(!locator.hasPort() || host.empty() ||
	   !sockaddr_storage_inet_pton(ip.mAddr, host) ||
	   !sockaddr_storage_setport(ip.mAddr, locator.port())) return false;
	ip.mSeenTime = time(NULL);

	bool changed = false;

	if (sslId == AuthSSL::getAuthSSL()->OwnId())
	{
		RS_STACK_MUTEX(mPeerMtx);
		changed = mOwnState.ipAddrs.updateLocalAddrs(ip);
	}
	else
	{
		RS_STACK_MUTEX(mPeerMtx);
		auto it =  mFriendList.find(sslId);
		if (it == mFriendList.end())
		{
			it = mOthersList.find(sslId);
			if (it == mOthersList.end())
			{
#ifdef PEER_DEBUG
				std::cerr << __PRETTY_FUNCTION__ << "cannot add address "
				          << "info, peer id: " << sslId << " not found in list"
				          << std::endl;
#endif
				return false;
			}
		}

		changed = it->second.ipAddrs.updateLocalAddrs(ip);
	}

	if (changed)
	{
#ifdef PEER_DEBUG
		std::cerr << __PRETTY_FUNCTION__ << " Added locator: "
		          << locator.toString() << std::endl;
#endif
		IndicateConfigChanged();
	}
	return changed;
}

bool p3PeerMgrIMPL::setLocalAddress( const RsPeerId &id,
                                     const sockaddr_storage &addr )
{
	bool changed = false;

	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		{
			RS_STACK_MUTEX(mPeerMtx);
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

	RS_STACK_MUTEX(mPeerMtx);
	/* check if it is a friend */
	std::map<RsPeerId, peerState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::setLocalAddress() cannot add addres "
			          << "info : peer id not found in friend list  id: "
			          << id << std::endl;
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

	if (changed) IndicateConfigChanged();
	return changed;
}

bool p3PeerMgrIMPL::setExtAddress( const RsPeerId &id,
                                   const sockaddr_storage &addr )
{
	bool changed = false;
	uint32_t check_res = 0;

	if(rsBanList && !rsBanList->isAddressAccepted(
	            addr, RSBANLIST_CHECKING_FLAGS_BLACKLIST, check_res ))
	{
		RsErr() << __PRETTY_FUNCTION__ << " trying to set external contact "
		        << "address for peer: " << id << " to a banned address " << addr
		        << std::endl;
		return false;
	}

	if (id == AuthSSL::getAuthSSL()->OwnId())
	{
		{
			RS_STACK_MUTEX(mPeerMtx);
			if (!sockaddr_storage_same(mOwnState.serveraddr, addr))
			{
				mOwnState.serveraddr = addr;
				changed = true;
			}
		}

		mNetMgr->setExtAddress(addr);

		return changed;
	}

	RS_STACK_MUTEX(mPeerMtx);
	/* check if it is a friend */
	std::map<RsPeerId, peerState>::iterator it;
	if (mFriendList.end() == (it = mFriendList.find(id)))
	{
		if (mOthersList.end() == (it = mOthersList.find(id)))
		{
#ifdef PEER_DEBUG
			std::cerr << "p3PeerMgrIMPL::setLocalAddress() cannot add addres "
			          << "info : peer id not found in friend list  id: " << id
			          << std::endl;
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

namespace pqi {

struct ZeroedInt
{
    ZeroedInt() { n=0 ;}
    int n ;
};

}

bool p3PeerMgrIMPL::addCandidateForOwnExternalAddress(const RsPeerId &from, const sockaddr_storage &addr)
{
    // The algorithm is the following:
    // - collect for each friend the last external connection address that is reported
    // - everytime the list is changed, parse it entirely and
    //		* emit a warnign when the address is unknown
    // 		* if multiple peers report the same address => notify the LinkMgr that the external address had changed.

	sockaddr_storage addr_filtered ;
	sockaddr_storage_clear(addr_filtered) ;
	sockaddr_storage_copyip(addr_filtered,addr) ;

#ifdef PEER_DEBUG
	std::cerr << "Own external address is " << sockaddr_storage_iptostring(addr_filtered) << ", as reported by friend " << from << std::endl;
#endif

	if(!sockaddr_storage_isExternalNet(addr_filtered))
	{
#ifdef PEER_DEBUG
		std::cerr << "  address is not an external address. Returning false" << std::endl ;
#endif
		return false ;
	}

    // Update a list of own IPs:
    //	- remove old values for that same peer
    //	- remove values for non connected peers

    {
	    RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	    mReportedOwnAddresses[from] = addr_filtered ;

	    for(std::map<RsPeerId,sockaddr_storage>::iterator it(mReportedOwnAddresses.begin());it!=mReportedOwnAddresses.end();)
		    if(!mLinkMgr->isOnline(it->first))
		    {
			    std::map<RsPeerId,sockaddr_storage>::iterator tmp(it) ;
			    ++tmp ;
			    mReportedOwnAddresses.erase(it) ;
			    it=tmp ;
		    }
	    else
		    ++it ;

	    sockaddr_storage current_best_ext_address_guess ;
	    uint32_t count ;

	    locked_computeCurrentBestOwnExtAddressCandidate(current_best_ext_address_guess,count) ;

	    std::cerr << "p3PeerMgr::  Current external address is calculated to be: " << sockaddr_storage_iptostring(current_best_ext_address_guess) << " (simultaneously reported by " << count << " peers)." << std::endl;
    }

    // now current

    sockaddr_storage own_addr ;

    if(!mNetMgr->getExtAddress(own_addr))
    {
#ifdef PEER_DEBUG
        std::cerr << "  cannot get current external address. Returning false" << std::endl;
#endif
        return false ;
    }
#ifdef PEER_DEBUG
    std::cerr << "  current external address is known to be " << sockaddr_storage_iptostring(own_addr) << std::endl;
#endif

    // Notify for every friend that has reported a wrong external address, except if that address is in the IP whitelist.

    if((rsBanList!=NULL && !rsBanList->isAddressAccepted(addr_filtered,RSBANLIST_CHECKING_FLAGS_WHITELIST)) && (!sockaddr_storage_sameip(own_addr,addr_filtered)))
    {
        std::cerr << "  Peer " << from << " reports a connection address (" << sockaddr_storage_iptostring(addr_filtered) <<") that is not your current external address (" << sockaddr_storage_iptostring(own_addr) << "). This is weird." << std::endl;

        RsServer::notify()->AddFeedItem(RS_FEED_ITEM_SEC_IP_WRONG_EXTERNAL_IP_REPORTED, from.toStdString(), sockaddr_storage_iptostring(own_addr), sockaddr_storage_iptostring(addr));
    }

    // we could also sweep over all connected friends and see if some report a different address.

    return true ;
}

bool p3PeerMgrIMPL::locked_computeCurrentBestOwnExtAddressCandidate(sockaddr_storage& addr, uint32_t& count)
{
	sockaddr_storage_clear(addr);
    std::map<sockaddr_storage, pqi::ZeroedInt> addr_counts ;

    for(std::map<RsPeerId,sockaddr_storage>::iterator it(mReportedOwnAddresses.begin());it!=mReportedOwnAddresses.end();++it)
	    ++addr_counts[it->second].n ;

#ifdef PEER_DEBUG
    std::cerr << "Current ext addr statistics:" << std::endl;
#endif

    count = 0 ;

    for(std::map<sockaddr_storage, pqi::ZeroedInt>::const_iterator it(addr_counts.begin());it!=addr_counts.end();++it)
    {
        if(uint32_t(it->second.n) > count)
        {
            addr = it->first ;
            count = it->second.n ;
        }

#ifdef PEER_DEBUG
        std::cerr << sockaddr_storage_iptostring(it->first) << " : " << it->second.n << std::endl;
#endif
    }

    return count > 0 ;
}

bool p3PeerMgrIMPL::getExtAddressReportedByFriends(sockaddr_storage &addr, uint8_t& /*isstable*/)
{
        RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

        uint32_t count ;

        locked_computeCurrentBestOwnExtAddressCandidate(addr,count) ;

#ifdef PEER_DEBUG
        std::cerr << "Estimation count = " << count << ". Trusted? = " << (count>=2) << std::endl;
#endif

        return count >= 2 ;// 2 is not conservative enough. 3 should be probably better.
}

static bool cleanIpList(std::list<pqiIpAddress>& lst,const RsPeerId& pid,p3LinkMgr *link_mgr)
{
    bool changed = false ;
#ifdef PEER_DEBUG
    rstime_t now = time(NULL) ;
#endif

    for(std::list<pqiIpAddress>::iterator it2(lst.begin());it2 != lst.end();)
    {
#ifdef PEER_DEBUG
    std::cerr << "Checking IP address " << sockaddr_storage_iptostring( (*it2).mAddr) << " for peer " << pid << ", age = " << now - (*it2).mSeenTime << std::endl;
#else
    /* remove unused parameter warnings */
    (void) pid;
#endif
		if(!link_mgr->checkPotentialAddr((*it2).mAddr))
      {
#ifdef PEER_DEBUG
        std::cerr << "  (SS) Removing Banned/old IP address " << sockaddr_storage_iptostring( (*it2).mAddr) << " from peer " << pid << ", age = " << now - (*it2).mSeenTime << std::endl;
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

bool    p3PeerMgrIMPL::updateAddressList(const RsPeerId& id, const pqiIpAddrSet &addrs)
{
#ifdef PEER_DEBUG
	std::cerr << "p3PeerMgrIMPL::setAddressList() called for id : " << id << std::endl;
#endif
    // first clean the list from potentially banned IPs.

    pqiIpAddrSet clean_set = addrs ;

    cleanIpList(clean_set.mExt.mAddrs,id,mLinkMgr) ;
    cleanIpList(clean_set.mLocal.mAddrs,id,mLinkMgr) ;

	bool am_I_a_hidden_node = isHiddenNode(getOwnId()) ;

	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	/* check if it is our own ip */
	if (id == getOwnId())
	{
        mOwnState.ipAddrs.updateAddrs(clean_set);
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

	if(!am_I_a_hidden_node)
		it->second.ipAddrs.updateAddrs(clean_set);
	else
		it->second.ipAddrs.clear();

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
    cleanup = true;
	bool useExtAddrFinder = mNetMgr->getIPServersEnabled();

	/* gather these information before mPeerMtx is locked! */
	struct sockaddr_storage proxy_addr_tor, proxy_addr_i2p;
	getProxyServerAddress(RS_HIDDEN_TYPE_TOR, proxy_addr_tor);
	getProxyServerAddress(RS_HIDDEN_TYPE_I2P, proxy_addr_i2p);

	mPeerMtx.lock(); /****** MUTEX LOCKED *******/

	RsPeerNetItem *item = new RsPeerNetItem();
	item->clear();

	item->nodePeerId = getOwnId();
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

	/* iterate through all friends and save */
        std::map<RsPeerId, peerState>::iterator it;
	for(it = mFriendList.begin(); it != mFriendList.end(); ++it)
	{
		item = new RsPeerNetItem();
		item->clear();

		item->nodePeerId = it->first;
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
#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgrIMPL::saveList() Peer Config Item:" << std::endl;
		item->print(std::cerr, 10);
		std::cerr << std::endl;
#endif
	}

	RsPeerBandwidthLimitsItem *pblitem = new RsPeerBandwidthLimitsItem ;
    	pblitem->peers = mPeerBandwidthLimits ;
	saveData.push_back(pblitem) ;

	RsPeerServicePermissionItem *sitem = new RsPeerServicePermissionItem ;

	for(std::map<RsPgpId,ServicePermissionFlags>::const_iterator it(mFriendsPermissionFlags.begin());it!=mFriendsPermissionFlags.end();++it)
	{
		sitem->pgp_ids.push_back(it->first) ;
		sitem->service_flags.push_back(it->second) ;
	}

	saveData.push_back(sitem) ;

	// Now save config for network digging strategies

	RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;

	RsTlvKeyValue kv;
	kv.key = kConfigKeyExtIpFinder;
	kv.value = (useExtAddrFinder)?"TRUE":"FALSE" ;
	vitem->tlvkvs.pairs.push_back(kv) ;


	// Store Proxy Server.
	// Tor
#ifdef PEER_DEBUG
	std::cerr << "Saving proxyServerAddress for Tor: " << sockaddr_storage_tostring(proxy_addr_tor);
	std::cerr << std::endl;
#endif

	kv.key = kConfigKeyProxyServerIpAddrTor;
	kv.value = sockaddr_storage_iptostring(proxy_addr_tor);
	vitem->tlvkvs.pairs.push_back(kv) ;

	kv.key = kConfigKeyProxyServerPortTor;
	kv.value = sockaddr_storage_porttostring(proxy_addr_tor);
	vitem->tlvkvs.pairs.push_back(kv) ;

	// I2P
#ifdef PEER_DEBUG
	std::cerr << "Saving proxyServerAddress for I2P: " << sockaddr_storage_tostring(proxy_addr_i2p);
	std::cerr << std::endl;
#endif

	kv.key = kConfigKeyProxyServerIpAddrI2P;
	kv.value = sockaddr_storage_iptostring(proxy_addr_i2p);
	vitem->tlvkvs.pairs.push_back(kv) ;

	kv.key = kConfigKeyProxyServerPortI2P;
	kv.value = sockaddr_storage_porttostring(proxy_addr_i2p);
	vitem->tlvkvs.pairs.push_back(kv) ;

	saveData.push_back(vitem);

	/* save groups */

    for ( std::map<RsNodeGroupId,RsGroupInfo>::iterator groupIt = groupList.begin(); groupIt != groupList.end(); ++groupIt)
    {
        RsNodeGroupItem *itm = new RsNodeGroupItem(groupIt->second);
        saveData.push_back(itm) ;
    }

	return true;
}

bool p3PeerMgrIMPL::getMaxRates(const RsPeerId& pid,uint32_t& maxUp,uint32_t& maxDn)
{
	RsPgpId pgp_id ;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

		std::map<RsPeerId, peerState>::const_iterator it = mFriendList.find(pid) ;

		if(it == mFriendList.end())
		{
			maxUp = 0;
			maxDn = 0;
			return false ;
		}

		pgp_id = it->second.gpg_id ;
	}

	return getMaxRates(pgp_id,maxUp,maxDn) ;
}

bool p3PeerMgrIMPL::getMaxRates(const RsPgpId& pid,uint32_t& maxUp,uint32_t& maxDn)
{
    RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

    std::map<RsPgpId,PeerBandwidthLimits>::const_iterator it2 = mPeerBandwidthLimits.find(pid) ;

    if(it2 != mPeerBandwidthLimits.end())
    {
	    maxUp = it2->second.max_up_rate_kbs ;
	    maxDn = it2->second.max_dl_rate_kbs ;
	    return true ;
    }
    else
    {
	    maxUp = 0;
	    maxDn = 0;
	    return false ;
    }
}
bool p3PeerMgrIMPL::setMaxRates(const RsPgpId& pid,uint32_t maxUp,uint32_t maxDn)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

        PeerBandwidthLimits& p(mPeerBandwidthLimits[pid]) ;

        if(maxUp == p.max_up_rate_kbs && maxDn == p.max_dl_rate_kbs)
            return true ;

        std::cerr << "Updating max rates for peer " << pid << " to " << maxUp << " kB/s (up), " << maxDn << " kB/s (dn)" << std::endl;

        p.max_up_rate_kbs = maxUp ;
        p.max_dl_rate_kbs = maxDn ;

        IndicateConfigChanged();

        return true ;
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
    std::string proxyIpAddressTor = kConfigDefaultProxyServerIpAddr;
    uint16_t    proxyPortTor = kConfigDefaultProxyServerPortTor;
    std::string proxyIpAddressI2P = kConfigDefaultProxyServerIpAddr;
    uint16_t    proxyPortI2P = kConfigDefaultProxyServerPortI2P;

    if (load.empty()) {
	    std::cerr << "p3PeerMgrIMPL::loadList() list is empty, it may be a configuration problem."  << std::endl;
	    return false;
    }

#ifdef PEER_DEBUG
    std::cerr << "p3PeerMgrIMPL::loadList() Item Count: " << load.size() << std::endl;
#endif

    RsPeerId ownId = getOwnId();
	bool am_I_a_hidden_node = isHiddenNode(ownId) ;

    /* load the list of peers */
    std::list<RsItem *>::iterator it;
    for(it = load.begin(); it != load.end(); ++it)
    {
	    RsPeerNetItem *pitem = dynamic_cast<RsPeerNetItem *>(*it);
	    if (pitem)
	    {
		    RsPeerId peer_id = pitem->nodePeerId ;
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
			    setLocation(pitem->nodePeerId, pitem->location);
		    }

		    if (pitem->netMode == RS_NET_MODE_HIDDEN)
		    {
			    /* set only the hidden stuff & localAddress */
			    setLocalAddress(peer_id, pitem->localAddrV4.addr);
			    setHiddenDomainPort(peer_id, pitem->domain_addr, pitem->domain_port);

		    }
		    else
		    {
			    pqiIpAddrSet addrs;

				if(!am_I_a_hidden_node)	// clear IPs if w're a hidden node. Friend's clear node IPs where previously sent.
				{
					setLocalAddress(peer_id, pitem->localAddrV4.addr);
					setExtAddress(peer_id, pitem->extAddrV4.addr);
					setDynDNS (peer_id, pitem->dyndns);

					/* convert addresses */
					addrs.mLocal.extractFromTlv(pitem->localAddrList);
					addrs.mExt.extractFromTlv(pitem->extAddrList);
				}

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
			    // Tor
			    else if (kit->key == kConfigKeyProxyServerIpAddrTor)
			    {
				    proxyIpAddressTor = kit->value;
#ifdef PEER_DEBUG
				    std::cerr << "Loaded proxyIpAddress for Tor: " << proxyIpAddressTor;
				    std::cerr << std::endl ;
#endif

			    }
			    else if (kit->key == kConfigKeyProxyServerPortTor)
			    {
                    uint16_t p = atoi(kit->value.c_str());

                    if(p >= 1024)
						proxyPortTor = p;
#ifdef PEER_DEBUG
				    std::cerr << "Loaded proxyPort for Tor: " << proxyPortTor;
				    std::cerr << std::endl ;
#endif
			    }
			    // I2p
			    else if (kit->key == kConfigKeyProxyServerIpAddrI2P)
			    {
				    proxyIpAddressI2P = kit->value;
#ifdef PEER_DEBUG
				    std::cerr << "Loaded proxyIpAddress for I2P: " << proxyIpAddressI2P;
				    std::cerr << std::endl ;
#endif
			    }
			    else if (kit->key == kConfigKeyProxyServerPortI2P)
			    {
                    uint16_t p = atoi(kit->value.c_str());

                    if(p >= 1024)
						proxyPortI2P = p;
#ifdef PEER_DEBUG
				    std::cerr << "Loaded proxyPort for I2P: " << proxyPortI2P;
				    std::cerr << std::endl ;
#endif
			    }
		    }

		    delete(*it);

		    continue;
	    }

        RsNodeGroupItem *gitem2 = dynamic_cast<RsNodeGroupItem*>(*it) ;

        if (gitem2)
        {
            RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
            std::cerr << "p3PeerMgrIMPL::loadList() Peer group item:" << std::endl;
            gitem->print(std::cerr, 10);
            std::cerr << std::endl;
#endif
            RsGroupInfo info ;
            info.peerIds = gitem2->pgpList.ids ;
            info.id      = gitem2->id ;
            info.name    = gitem2->name ;
            info.flag    = gitem2->flag ;

            std::cerr << "(II) Loaded group in new format. ID = " << info.id << std::endl;
            groupList[info.id] = info ;

			delete *it ;
            continue;
        }
	    RsPeerBandwidthLimitsItem *pblitem = dynamic_cast<RsPeerBandwidthLimitsItem*>(*it) ;

	    if(pblitem)
	    {
		    RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
		    std::cerr << "Loaded service permission item: " << std::endl;
#endif
	    		mPeerBandwidthLimits = pblitem->peers ;
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
        const RsNodeGroupId   standardGroupIds  [standardGroupCount] = { RS_GROUP_ID_FRIENDS,           RS_GROUP_ID_FAMILY,           RS_GROUP_ID_COWORKERS,           RS_GROUP_ID_OTHERS,           RS_GROUP_ID_FAVORITES };
        const char           *standardGroupNames[standardGroupCount] = { RS_GROUP_DEFAULT_NAME_FRIENDS, RS_GROUP_DEFAULT_NAME_FAMILY, RS_GROUP_DEFAULT_NAME_COWORKERS, RS_GROUP_DEFAULT_NAME_OTHERS, RS_GROUP_DEFAULT_NAME_FAVORITES };

        for(uint32_t k=0;k<standardGroupCount;++k)
            if(groupList.find(standardGroupIds[k]) == groupList.end())
            {
                RsGroupInfo info ;
                info.id = standardGroupIds[k];
                info.name = standardGroupNames[k];
                info.flag |= RS_GROUP_FLAG_STANDARD;

                groupList[info.id] = info;
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
    // Tor
    sockaddr_storage_clear(proxy_addr);
    sockaddr_storage_ipv4_aton(proxy_addr, proxyIpAddressTor.c_str());
    sockaddr_storage_ipv4_setport(proxy_addr, proxyPortTor);

    if (sockaddr_storage_isValidNet(proxy_addr))
    {
	    setProxyServerAddress(RS_HIDDEN_TYPE_TOR, proxy_addr);
    }

    // I2P
    sockaddr_storage_clear(proxy_addr);
    sockaddr_storage_ipv4_aton(proxy_addr, proxyIpAddressI2P.c_str());
    sockaddr_storage_ipv4_setport(proxy_addr, proxyPortI2P);

    if (sockaddr_storage_isValidNet(proxy_addr))
    {
	    setProxyServerAddress(RS_HIDDEN_TYPE_I2P, proxy_addr);
    }

    load.clear() ;
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

        do { groupInfo.id = RsNodeGroupId::random(); } while(groupList.find(groupInfo.id) != groupList.end()) ;

        RsGroupInfo groupItem(groupInfo) ;

		// remove standard flag

        groupItem.flag &= ~RS_GROUP_FLAG_STANDARD;
        groupList[groupInfo.id] = groupItem;

        std::cerr << "(II) Added new group with ID " << groupInfo.id << ", name=\"" << groupInfo.name << "\"" << std::endl;
	}

	RsServer::notify()->notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_ADD);

	IndicateConfigChanged();

	return true;
}

bool p3PeerMgrIMPL::editGroup(const RsNodeGroupId& groupId, RsGroupInfo &groupInfo)
{
    if (groupId.isNull())
		return false;

	bool changed = false;

    {
        RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

        std::map<RsNodeGroupId,RsGroupInfo>::iterator it = groupList.find(groupId) ;

        if(it == groupList.end())
        {
            std::cerr << "(EE) cannot find local node group with ID " << groupId << std::endl;
            return false ;
        }

        if (it->second.flag & RS_GROUP_FLAG_STANDARD)
        {
            // can't edit standard groups
            std::cerr << "(EE) cannot edit standard group with ID " << groupId << std::endl;
            return false ;
        }
        else
        {
            changed = true;
            it->second = groupInfo;
        }
    }

    if (changed)
    {
		RsServer::notify()->notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_MOD);

		IndicateConfigChanged();
	}

	return changed;
}

bool p3PeerMgrIMPL::removeGroup(const RsNodeGroupId& groupId)
{
	bool changed = false;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

        std::map<RsNodeGroupId,RsGroupInfo>::iterator it = groupList.find(groupId) ;

        if (it != groupList.end()) {
            if (it->second.flag & RS_GROUP_FLAG_STANDARD)
            {
				// can't remove standard groups
                std::cerr << "(EE) cannot remove standard group with ID " << groupId << std::endl;
                return false ;
            }
#warning csoler: we need to check that the local group is not used. Otherwise deleting it is going to cause problems!
//            else if(!it->second.used_gxs_groups.empty())
//            {
//                std::cerr << "(EE) cannot remove standard group with ID " << groupId << " because it is used in the following groups: " << std::endl;
//                for(std::set<RsGxsGroupId>::const_iterator it2(it->second.used_gxs_groups.begin());it2!=it->second.used_gxs_groups.end();++it2)
//                    std::cerr << "  " << *it2 << std::endl;
//
//                return false ;
//            }
            else
            {
				changed = true;
                groupList.erase(it);
			}
		}
	}

	if (changed) {
		RsServer::notify()->notifyListChange(NOTIFY_LIST_GROUPLIST, NOTIFY_TYPE_DEL);

		IndicateConfigChanged();
	}

	return changed;
}

bool p3PeerMgrIMPL::getGroupInfoByName(const std::string& groupName, RsGroupInfo &groupInfo)
{
    RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

    for(std::map<RsNodeGroupId,RsGroupInfo>::iterator it = groupList.begin();it!=groupList.end();++it)
        if(it->second.name == groupName)
        {
            groupInfo = it->second ;
            return true ;
        }

    std::cerr << "(EE) getGroupInfoByName: no known group for name " << groupName << std::endl;
    return false ;
}
bool p3PeerMgrIMPL::getGroupInfo(const RsNodeGroupId& groupId, RsGroupInfo &groupInfo)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

    std::map<RsNodeGroupId,RsGroupInfo>::iterator it = groupList.find(groupId) ;

    if(it == groupList.end())
        return false ;

    groupInfo =  it->second;

    return true;
}

bool p3PeerMgrIMPL::getGroupInfoList(std::list<RsGroupInfo>& groupInfoList)
{
	RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

	for(std::map<RsNodeGroupId,RsGroupInfo> ::const_iterator groupIt = groupList.begin(); groupIt != groupList.end(); ++groupIt)
		groupInfoList.push_back(groupIt->second) ;

	return true;
}

// groupId.isNull() && assign == false -> remove from all groups

bool p3PeerMgrIMPL::assignPeersToGroup(const RsNodeGroupId &groupId, const std::list<RsPgpId> &peerIds, bool assign)
{
    if (groupId.isNull() && assign == true)
		return false;

    if (peerIds.empty())
		return false;

	bool changed = false;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

        for (std::map<RsNodeGroupId,RsGroupInfo>::iterator groupIt = groupList.begin(); groupIt != groupList.end(); ++groupIt)
            if (groupId.isNull() || groupIt->first == groupId)
            {
                RsGroupInfo& groupItem = groupIt->second;

                for (std::list<RsPgpId>::const_iterator peerIt = peerIds.begin(); peerIt != peerIds.end(); ++peerIt)
                {
                    //std::set<RsPgpId>::iterator peerIt1 = groupItem.peerIds.find(*peerIt);

                    if (assign)
                    {
                        groupItem.peerIds.insert(*peerIt);
                        changed = true;
                    }
                    else
                    {
                        groupItem.peerIds.erase(*peerIt);
                        changed = true;
					}
				}

                if (!groupId.isNull())
					break;
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


bool p3PeerMgrIMPL::removeBannedIps()
{
    RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

#ifdef PEER_DEBUG
    std::cerr << "Cleaning known IPs for all peers." << std::endl;
#endif

    bool changed = false ;
    for( std::map<RsPeerId, peerState>::iterator it = mFriendList.begin(); it != mFriendList.end(); ++it)
    {
        if(cleanIpList(it->second.ipAddrs.mExt.mAddrs,it->first,mLinkMgr)) changed = true ;
        if(cleanIpList(it->second.ipAddrs.mLocal.mAddrs,it->first,mLinkMgr)) changed = true ;

        if(rsBanList!=NULL && !rsBanList->isAddressAccepted(it->second.serveraddr,RSBANLIST_CHECKING_FLAGS_BLACKLIST))
        {
            sockaddr_storage_clear(it->second.serveraddr) ;
            std::cerr << "(SS) Peer " << it->first << " has a banned server address. Wiping it out." << std::endl;
        }
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

/**
 * @brief p3PeerMgrIMPL::removeUnusedLocations Removes all location offline for RS_PEER_OFFLINE_DELETE seconds or more. Keeps the most recent location per PGP id.
 * @return true on success
 *
 * This function removes all location that are offline for too long defined by RS_PEER_OFFLINE_DELETE.
 * It also makes sure that at least one location (the most recent) is kept.
 *
 * The idea of the function is the following:
 *  - keep track if there is at least one location per PGP id that is not offline for too long
 *      -> hasRecentLocation
 *  - keep track of most recent location per PGP id that is offline for too long (and its time stamp)
 *      -> mostRecentLocation
 *      -> mostRecentTime
 *
 * When a location is found that is offline for too long the following points are checked from the top to the bottom:
 * 1) remove it when the PGP id has a location that is not offline for too long
 * 2) remove it when the PGP id has a more recent location
 * 3) keep it when it is the most recent location
 *      This location will possibly be removed when a more recent (but still offline for too long) is found
 */
bool p3PeerMgrIMPL::removeUnusedLocations()
{
	std::list<RsPeerId> toRemove;
	std::map<RsPgpId, rstime_t>   mostRecentTime;

	const rstime_t now = time(NULL);

	std::list<RsPgpId> pgpList ;

	if (!rsPeers->getGPGAcceptedList(pgpList))
		return false ;

	{
		RsStackMutex stack(mPeerMtx); /****** STACK LOCK MUTEX *******/

        // First put a sensible number in all PGP ids

        for(std::list<RsPgpId>::const_iterator it = pgpList.begin(); it != pgpList.end(); ++it)
            mostRecentTime[*it] = (rstime_t)0;

#ifdef PEER_DEBUG
		std::cerr << "p3PeerMgr::removeUnusedLocations()" << std::endl;
#endif
        // Then compute the most recently used location for all PGP ids

        for( std::map<RsPeerId, peerState>::iterator it = mFriendList.begin(); it != mFriendList.end(); ++it)
        {
            rstime_t& bst(mostRecentTime[it->second.gpg_id]) ;
            bst = std::max(bst,it->second.lastcontact) ;
        }

        // And remove all locations that are too old and also older than the most recent location. Doing this we're sure to always keep at least one location per PGP id.

        for( std::map<RsPeerId, peerState>::iterator it = mFriendList.begin(); it != mFriendList.end(); ++it)
        {
            if (now > it->second.lastcontact + RS_PEER_OFFLINE_DELETE && it->second.lastcontact < mostRecentTime[it->second.gpg_id])
                toRemove.push_back(it->first);
#ifdef PEER_DEBUG
            std::cerr << "Location " << it->first << " PGP id " << it->second.gpg_id << " last contact " << it->second.lastcontact << " remove: " << (now > it->second.lastcontact + RS_PEER_OFFLINE_DELETE)  << " most recent: " << mostRecentTime[it->second.gpg_id]
                      << ". Final result remove: " << (it->second.lastcontact < mostRecentTime[it->second.gpg_id] && now > it->second.lastcontact + RS_PEER_OFFLINE_DELETE )<< std::endl;
#endif
        }
	}

	for (std::list<RsPeerId>::iterator it = toRemove.begin(); it != toRemove.end(); ++it)
		removeFriend(*it, false) ;

	return true;
}


