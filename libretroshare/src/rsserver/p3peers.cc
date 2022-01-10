/*******************************************************************************
 * libretroshare/src/rsserver: p3peers.cc                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2008  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2015-2020  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2020       Asociación Civil Altermundi <info@altermundi.net>  *
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

#include "pgp/pgpkeyutil.h"

#include "rsserver/p3peers.h"
#include "rsserver/p3face.h"

#include "pqi/p3linkmgr.h"
#include "pqi/p3peermgr.h"
#include "pqi/p3netmgr.h"

#include "pqi/authssl.h"
#include "pqi/authgpg.h"
#include "retroshare/rsinit.h"
#include "retroshare/rsnotify.h"
#include "retroshare/rsfiles.h"
#include "util/rsurl.h"
#include "util/radix64.h"
#include "util/rsbase64.h"
#include "pgp/rscertificate.h"

#include <iostream>
#include <fstream>

//static const int MAX_TIME_KEEP_LOCATION_WITHOUT_CONTACT = 30*24*3600 ; // 30 days.

#include "pqi/authssl.h"

typedef RsCertificate::RsShortInviteFieldType RsShortInviteFieldType;	// locally in this file to avoid renaming everything.

RsPeers *rsPeers = NULL;

/*******
 * #define P3PEERS_DEBUG 1
 *******/

//int ensureExtension(std::string &name, std::string def_ext);

std::string RsPeerTrustString(uint32_t trustLvl)
{
	std::string str;

	switch(trustLvl)
	{
		default:
		case RS_TRUST_LVL_UNKNOWN:
			str = "VALIDITY_UNKNOWN";
			break;
		case RS_TRUST_LVL_UNDEFINED:
			str = "VALIDITY_UNDEFINED";
			break;
		case RS_TRUST_LVL_NEVER:
			str = "VALIDITY_NEVER";
			break;
		case RS_TRUST_LVL_MARGINAL:
			str = "VALIDITY_MARGINAL";
			break;
		case RS_TRUST_LVL_FULL:
			str = "VALIDITY_FULL";
			break;
		case RS_TRUST_LVL_ULTIMATE:
			str = "VALIDITY_ULTIMATE";
			break;
	}
	return str;
}

std::string RsPeerNetModeString(uint32_t netModel)
{
	std::string str;
	if (netModel == RS_NETMODE_EXT)
	{
		str = "External Port";
	}
	else if (netModel == RS_NETMODE_UPNP)
	{
		str = "Ext (UPnP)";
	}
	else if (netModel == RS_NETMODE_UDP)
	{
		str = "UDP Mode";
	}
	else if (netModel == RS_NETMODE_HIDDEN)
	{
		str = "Hidden";
	}
	else if (netModel == RS_NETMODE_UNREACHABLE)
	{
		str = "UDP Mode (Unreachable)";
	}
	else 
	{
		str = "Unknown NetMode";
	}
	return str;
}


p3Peers::p3Peers(p3LinkMgr *lm, p3PeerMgr *pm, p3NetMgr *nm)
        :mLinkMgr(lm), mPeerMgr(pm), mNetMgr(nm) {}

	/* Updates ... */
bool p3Peers::FriendsChanged(const RsPeerId& pid,bool add)
{
#ifdef P3PEERS_DEBUG
    std::cerr << "p3Peers::FriendsChanged()" << std::endl;
#endif
    if(rsEvents)
    {
        auto ev = std::make_shared<RsPeerStateChangedEvent>(pid);
        rsEvents->postEvent(ev);
    }
    RsServer::notify()->notifyListChange(NOTIFY_LIST_FRIENDS, add? NOTIFY_TYPE_ADD : NOTIFY_TYPE_DEL); // this is meant to disappear

    /* TODO */
    return false;
}

bool p3Peers::OthersChanged()
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::OthersChanged()" << std::endl;
#endif

	/* TODO */
	return false;
}

	/* Peer Details (Net & Auth) */
const RsPeerId& p3Peers::getOwnId()
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getOwnId()" << std::endl;
#endif

        return AuthSSL::getAuthSSL()->OwnId();
}

bool	p3Peers::getOnlineList(std::list<RsPeerId> &ids)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getOnlineList()" << std::endl;
#endif

	/* get from mConnectMgr */
	mLinkMgr->getOnlineList(ids);
	return true;
}

bool	p3Peers::getFriendList(std::list<RsPeerId> &ids)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getFriendList()" << std::endl;
#endif

	/* get from mConnectMgr */
	mLinkMgr->getFriendList(ids);
	return true;
}

//bool	p3Peers::getOthersList(std::list<std::string> &ids)
//{
//#ifdef P3PEERS_DEBUG
//	std::cerr << "p3Peers::getOthersList()";
//	std::cerr << std::endl;
//#endif
//
//	/* get from mAuthMgr */
//        AuthSSL::getAuthSSL()->getAllList(ids);
//	return true;
//}

bool p3Peers::getPeersCount(
        uint32_t& peersCount, uint32_t& onlinePeersCount,
        bool countLocations )
{
	peersCount = mPeerMgr->getFriendCount(countLocations, false);
	onlinePeersCount = mPeerMgr->getFriendCount(countLocations, true);
	return true;
}

bool p3Peers::getPeerCount (unsigned int *friendCount, unsigned int *onlineCount, bool ssl)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPeerCount()" << std::endl;
#endif

	if (friendCount) *friendCount = mPeerMgr->getFriendCount(ssl, false);
	if (onlineCount) *onlineCount = mPeerMgr->getFriendCount(ssl, true);

	return true;
}

bool    p3Peers::isOnline(const RsPeerId &id)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::isOnline() " << id << std::endl;
#endif

	/* get from mConnectMgr */
	peerConnectState state;
	if (mLinkMgr->getFriendNetStatus(id, state) &&
			(state.state & RS_PEER_S_CONNECTED))
	{
		return true;
	}
	return false;
}

bool    p3Peers::isFriend(const RsPeerId &ssl_id)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::isFriend() " << ssl_id << std::endl;
#endif

        /* get from mConnectMgr */
        return mPeerMgr->isFriend(ssl_id);
}

bool p3Peers::getPeerMaximumRates(const RsPeerId& pid,uint32_t& maxUploadRate,uint32_t& maxDownloadRate)
{
    return mPeerMgr->getMaxRates(pid,maxUploadRate,maxDownloadRate) ;
}
bool p3Peers::getPeerMaximumRates(const RsPgpId& pid,uint32_t& maxUploadRate,uint32_t& maxDownloadRate)
{
    return mPeerMgr->getMaxRates(pid,maxUploadRate,maxDownloadRate) ;
}

bool p3Peers::setPeerMaximumRates(const RsPgpId& pid,uint32_t maxUploadRate,uint32_t maxDownloadRate)
{
    return mPeerMgr->setMaxRates(pid,maxUploadRate,maxDownloadRate) ;
}

bool p3Peers::haveSecretKey(const RsPgpId& id)
{
    return AuthPGP::haveSecretKey(id);
}

/* There are too many dependancies of this function
 * to shift it immeidately
 */

bool p3Peers::getPeerDetails(const RsPeerId& id, RsPeerDetails &d)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPeerDetails() called for id : " << id << std::endl;
#endif

	RsPeerId sOwnId = AuthSSL::getAuthSSL()->OwnId();
	peerState ps;

	if (id == sOwnId)
	{
		mPeerMgr->getOwnNetStatus(ps);
        ps.gpg_id = AuthPGP::getPgpOwnId();
	}
	else if (!mPeerMgr->getFriendNetStatus(id, ps))
	{
#ifdef P3PEERS_DEBUG
		std::cerr << "p3Peers::getPeerDetails() ERROR not an SSL Id: " << id << std::endl;
#endif
		return false;
	}

	/* get from gpg (first), to fill in the sign and trust details */
	/* don't return now, we've got fill in the ssl and connection info */

	if(!getGPGDetails(ps.gpg_id, d))
	{
		if(!ps.skip_pgp_signature_validation)
			return false;

		d.gpg_id = ps.gpg_id ;
		d.skip_pgp_signature_validation = true;
	}
	else
		d.skip_pgp_signature_validation = false;

	d.isOnlyGPGdetail = false;

	//get the ssl details
	d.id 		= id;
	d.location 	= ps.location;

	d.service_perm_flags = mPeerMgr->servicePermissionFlags(ps.gpg_id);

	/* generate */
	d.authcode  	= "AUTHCODE";

	/* fill from pcs */
	d.lastConnect	= ps.lastcontact;
	d.connectPeriod = 0;

	if (ps.hiddenNode)
	{
		d.isHiddenNode = true;
		d.hiddenNodeAddress = ps.hiddenDomain;
		d.hiddenNodePort = ps.hiddenPort;
		d.hiddenType = ps.hiddenType;

		if(sockaddr_storage_isnull(ps.localaddr))	// that happens if the address is not initialised.
		{
			d.localAddr	= "INVALID_IP";
			d.localPort	= 0 ;
		}
		else
		{
			d.localAddr	= sockaddr_storage_iptostring(ps.localaddr);
			d.localPort	= sockaddr_storage_port(ps.localaddr);
		}
		d.extAddr = "hidden";
		d.extPort = 0;
		d.dyndns = "";
	}
	else
	{
		d.isHiddenNode = false;
		d.hiddenNodeAddress = "";
		d.hiddenNodePort = 0;
		d.hiddenType = RS_HIDDEN_TYPE_NONE;

		if(!sockaddr_storage_isnull(ps.localaddr))
		{
			sockaddr_storage_ipv6_to_ipv4(ps.localaddr);
			d.localAddr = sockaddr_storage_iptostring(ps.localaddr);
			d.localPort = sockaddr_storage_port(ps.localaddr);
		}
		else
		{
			d.localAddr	= "INVALID_IP";
			d.localPort	= 0;
		}

		if(!sockaddr_storage_isnull(ps.serveraddr))
		{
			sockaddr_storage_ipv6_to_ipv4(ps.serveraddr);
			d.extAddr = sockaddr_storage_iptostring(ps.serveraddr);
			d.extPort = sockaddr_storage_port(ps.serveraddr);
		}
		else
		{
			d.extAddr = "INVALID_IP";
			d.extPort = 0;
		}

		d.dyndns        = ps.dyndns;

		std::list<pqiIpAddress>::iterator it;
		for(it = ps.ipAddrs.mLocal.mAddrs.begin(); 
		    it != ps.ipAddrs.mLocal.mAddrs.end(); ++it)
		{
			sockaddr_storage_ipv6_to_ipv4(it->mAddr);
			std::string toto;
			toto += sockaddr_storage_tostring(it->mAddr);
			rs_sprintf_append(toto, "    %ld sec loc", time(NULL) - it->mSeenTime);
			d.ipAddressList.push_back(toto);
		}
		for(it = ps.ipAddrs.mExt.mAddrs.begin(); it != ps.ipAddrs.mExt.mAddrs.end(); ++it)
		{
			sockaddr_storage_ipv6_to_ipv4(it->mAddr);
			std::string toto;
			toto += sockaddr_storage_tostring(it->mAddr);
			rs_sprintf_append(toto, "    %ld sec ext", time(NULL) - it->mSeenTime);
			d.ipAddressList.push_back(toto);
		}
	}


	switch(ps.netMode & RS_NET_MODE_ACTUAL)
	{
	case RS_NET_MODE_EXT:
		d.netMode	= RS_NETMODE_EXT;
		break;
	case RS_NET_MODE_UPNP:
		d.netMode	= RS_NETMODE_UPNP;
		break;
	case RS_NET_MODE_UDP:
		d.netMode	= RS_NETMODE_UDP;
		break;
	case RS_NET_MODE_HIDDEN:
		d.netMode	= RS_NETMODE_HIDDEN;
		break;
	case RS_NET_MODE_UNREACHABLE:
	case RS_NET_MODE_UNKNOWN:
	default:
		d.netMode	= RS_NETMODE_UNREACHABLE;
		break;
	}

	d.vs_disc = ps.vs_disc;
	d.vs_dht = ps.vs_dht;

	/* Translate */
	peerConnectState pcs;
	if (!mLinkMgr->getFriendNetStatus(id, pcs)) 
	{
		if(id != sOwnId)
			std::cerr << "p3Peers::getPeerDetails() ERROR No Link Information : " << id << std::endl;
		return true;
	}

#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPeerDetails() got a SSL id and is returning SSL and GPG details for id : " << id << std::endl;
#endif

	if (pcs.state & RS_PEER_S_CONNECTED)
	{
		sockaddr_storage_ipv6_to_ipv4(pcs.connectaddr);
		d.connectAddr = sockaddr_storage_iptostring(pcs.connectaddr);
		d.connectPort = sockaddr_storage_port(pcs.connectaddr);
	}
	else
	{
		d.connectAddr = ""; 
		d.connectPort = 0 ;
	}

	d.state		= 0;
	if (pcs.state & RS_PEER_S_FRIEND)      d.state |= RS_PEER_STATE_FRIEND;
	if (pcs.state & RS_PEER_S_ONLINE)      d.state |= RS_PEER_STATE_ONLINE;
	if (pcs.state & RS_PEER_S_CONNECTED)   d.state |= RS_PEER_STATE_CONNECTED;
	if (pcs.state & RS_PEER_S_UNREACHABLE) d.state |= RS_PEER_STATE_UNREACHABLE;

	d.actAsServer = pcs.actAsServer;

	d.linkType = pcs.linkType;

	/* Finally determine AutoConnect Status */
	d.foundDHT = pcs.dht.found;

	d.connectState = RS_PEER_CONNECTSTATE_OFFLINE;
	d.connectStateString.clear();

	if (pcs.inConnAttempt)
	{
		if (pcs.currentConnAddrAttempt.type & RS_NET_CONN_TCP_ALL) {
			d.connectState = RS_PEER_CONNECTSTATE_TRYING_TCP;
			d.connectStateString = sockaddr_storage_tostring(pcs.currentConnAddrAttempt.addr);
		} else if (pcs.currentConnAddrAttempt.type & RS_NET_CONN_UDP_ALL) {
			d.connectState = RS_PEER_CONNECTSTATE_TRYING_UDP;
			d.connectStateString = sockaddr_storage_tostring(pcs.currentConnAddrAttempt.addr);
		}
	}
	else if (pcs.state & RS_PEER_S_CONNECTED)
	{
		/* peer is connected - determine how and set proper connectState */
		if(mPeerMgr->isHidden())
		{
			uint32_t type;
			/* hidden location */
			/* use connection direction to determine connection type */
			if(pcs.actAsServer)
			{
				/* incoming connection */
				/* use own type to set connectState */
				type = mPeerMgr->getHiddenType(AuthSSL::getAuthSSL()->OwnId());
				switch (type) {
				case RS_HIDDEN_TYPE_TOR:
					d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_TOR;
					break;
				case RS_HIDDEN_TYPE_I2P:
					d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_I2P;
					break;
				default:
					d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN;
					break;
				}
			}
			else
			{
				/* outgoing connection */
				/* use peer hidden type to set connectState */
				switch (ps.hiddenType) {
				case RS_HIDDEN_TYPE_TOR:
					d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_TOR;
					break;
				case RS_HIDDEN_TYPE_I2P:
					d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_I2P;
					break;
				default:
					d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN;
					break;
				}
			}
		}
		else if (ps.hiddenType & RS_HIDDEN_TYPE_MASK)
		{
			/* hidden peer */
			/* use hidden type to set connectState */
			switch (ps.hiddenType) {
			case RS_HIDDEN_TYPE_TOR:
				d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_TOR;
				break;
			case RS_HIDDEN_TYPE_I2P:
				d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_I2P;
				break;
			default:
				d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN;
				break;
			}
		}
		else
		{
			/* peer and we are normal nodes */
			/* use normal detection to set connectState */
			if (pcs.connecttype == RS_NET_CONN_TCP_ALL)
			{
				d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_TCP;
			}
			else if (pcs.connecttype == RS_NET_CONN_UDP_ALL)
			{
				d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_UDP;
			}
			else
			{
				d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN;
			}
		}
	}
        
	d.wasDeniedConnection = pcs.wasDeniedConnection;
	d.deniedTS = pcs.deniedTS;

	return true;
}

bool p3Peers::isProxyAddress(const uint32_t type, const sockaddr_storage& addr)
{
    uint16_t port ;
    std::string string_addr;
	uint32_t status ;

	if(!getProxyServer(type, string_addr, port, status))
        return false ;

    return sockaddr_storage_iptostring(addr)==string_addr && sockaddr_storage_port(addr)==port ;
}

bool p3Peers::isKeySupported(const RsPgpId& id)
{
    return AuthPGP::isKeySupported(id);
}

std::string p3Peers::getGPGName(const RsPgpId &gpg_id)
{
	/* get from mAuthMgr as it should have more peers? */
        return AuthPGP::getPgpName(gpg_id);
}

bool p3Peers::isPgpFriend(const RsPgpId& pgpId)
{ return AuthPGP::isPGPAccepted(pgpId); }

bool p3Peers::isSslOnlyFriend(const RsPeerId& sslId)
{
    bool has_ssl_only_flag = mPeerMgr->isSslOnlyFriend(sslId) ;

    if(has_ssl_only_flag)
    {
        if(isPgpFriend(getGPGId(sslId)))
        {
            RsErr() << __PRETTY_FUNCTION__ << ": Peer " << sslId << " has SSL-friend-only flag but his PGP id is in the list of friends. This is inconsistent (Bug in the code). Returning false for security reasons." << std::endl;
			return false;
        }
        return true;
	}
    return false;
}

bool p3Peers::isGPGAccepted(const RsPgpId &gpg_id_is_friend)
{ return isPgpFriend(gpg_id_is_friend); }

std::string p3Peers::getPeerName(const RsPeerId& ssl)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPeerName() " << ssl_or_gpg_id << std::endl;
#endif
	std::string name;
	if (ssl == AuthSSL::getAuthSSL()->OwnId()) 
        return AuthPGP::getPgpOwnName();
	
	if (mPeerMgr->getPeerName(ssl, name)) 
	{
#ifdef P3PEERS_DEBUG
		std::cerr << "p3Peers::getPeerName() got a ssl id. Name is : " << name << std::endl;
#endif
		return name;
	}
	return std::string() ;
}


bool	p3Peers::getGPGAllList(std::list<RsPgpId> &ids)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getGPGAllList()" << std::endl;
#endif

        /* get from mAuthMgr */
        AuthPGP::getPgpAllList(ids);
        return true;
}

bool	p3Peers::getGPGValidList(std::list<RsPgpId> &ids)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPGPOthersList()" << std::endl;
#endif

        /* get from mAuthMgr */
        AuthPGP::getPgpValidList(ids);
        return true;
}

bool	p3Peers::getGPGSignedList(std::list<RsPgpId> &ids)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPGPOthersList()" << std::endl;
#endif

        /* get from mAuthMgr */
        AuthPGP::getPgpSignedList(ids);
        return true;
}

bool p3Peers::getPgpFriendList(std::vector<RsPgpId>& pgpIds)
{
	std::list<RsPgpId> ids;
    if(AuthPGP::getPgpAcceptedList(ids))
	{
		pgpIds.clear();
		std::copy(ids.begin(), ids.end(), std::back_inserter(pgpIds));
		return true;
	}
	return false;
}

bool	p3Peers::getGPGAcceptedList(std::list<RsPgpId> &ids)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getGPGAcceptedList()" << std::endl;
#endif
        AuthPGP::getPgpAcceptedList(ids);
        return true;
}


bool	p3Peers::getAssociatedSSLIds(const RsPgpId &gpg_id, std::list<RsPeerId> &ids)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getAssociatedSSLIds() for id : " << gpg_id << std::endl;
#endif

	return mPeerMgr->getAssociatedPeers(gpg_id, ids);
}

bool    p3Peers::gpgSignData(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen, std::string reason /* = "" */)
{
    return AuthPGP::SignDataBin(data,len,sign,signlen, reason);
}

RsPgpId p3Peers::pgpIdFromFingerprint(const RsPgpFingerprint& fpr)
{
	return PGPHandler::pgpIdFromFingerprint(fpr);
}

bool	p3Peers::getGPGDetails(const RsPgpId &pgp_id, RsPeerDetails &d)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPgpDetails() called for id : " << pgp_id << std::endl;
#endif

	/* get from mAuthMgr */
    bool res = AuthPGP::getPgpDetails(pgp_id, d);

	d.isOnlyGPGdetail = true ;
	d.service_perm_flags = mPeerMgr->servicePermissionFlags(pgp_id) ;

	return res ;
}

const RsPgpId& p3Peers::getGPGOwnId()
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPGPOwnId()" << std::endl;
#endif

	/* get from mAuthMgr */
        return AuthPGP::getPgpOwnId();
}

RsPgpId p3Peers::getGPGId(const RsPeerId& sslid)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPGPId()" << std::endl;
#endif

	/* get from mAuthMgr */
	if (sslid == AuthSSL::getAuthSSL()->OwnId()) 
	{
        return AuthPGP::getPgpOwnId();
	}
	peerState pcs;
	if (mPeerMgr->getFriendNetStatus(sslid, pcs))
		return pcs.gpg_id;

	return RsPgpId();
}


	/* These Functions are now the only way to authorize a new gpg user...
	 * if we are passed a ssl_id, then use it... otherwise just auth gpg_id
	 */

	/* Add/Remove Friends */
bool 	p3Peers::addFriend(const RsPeerId &ssl_id, const RsPgpId &gpg_id,ServicePermissionFlags perm_flags)
{

#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::addFriend() with : id : " << id << "; gpg_id : " << gpg_id << std::endl;
#endif
    if(AuthPGP::isPGPId(gpg_id))
	{
#ifdef P3PEERS_DEBUG
		std::cerr << "p3Peers::addFriend() Authorising GPG Id: " << gpg_id << std::endl;
#endif
        if (AuthPGP::AllowConnection(gpg_id, true))
		{
#ifdef P3PEERS_DEBUG
			std::cerr << "p3Peers::addFriend() Authorization OK." << std::endl;
#endif
		}
		else
		{
#ifdef P3PEERS_DEBUG
			std::cerr << "p3Peers::addFriend() Authorization FAILED." << std::endl;
#endif
			return false;
		}
	}
	else
	{
#ifdef P3PEERS_DEBUG
		std::cerr << "p3Peers::addFriend() Bad gpg_id : " << gpg_id << std::endl;
#endif
		return false;
	}

	if(ssl_id.isNull())
	{
#ifdef P3PEERS_DEBUG
		std::cerr << "p3Peers::addFriend() WARNING id is NULL or gpgId" << std::endl;
#endif
		return true;
	} 

    FriendsChanged(ssl_id,true);

	/* otherwise - we install as ssl_id.....
	 * If we are adding an SSL certificate. we flag lastcontact as now. 
	 * This will cause the SSL certificate to be retained for 30 days... and give the person a chance to connect!
	 *  */
	rstime_t now = time(NULL);
	return mPeerMgr->addFriend(ssl_id, gpg_id, RS_NET_MODE_UDP, RS_VS_DISC_FULL, RS_VS_DHT_FULL, now, perm_flags);
}

bool p3Peers::addSslOnlyFriend( const RsPeerId& sslId, const RsPgpId& pgp_id,const RsPeerDetails& details )
{
    if( mPeerMgr->addSslOnlyFriend(sslId, pgp_id,details))
    {
        FriendsChanged(sslId,true);
        return true;
    }
    else
		return false;
}

bool 	p3Peers::removeKeysFromPGPKeyring(const std::set<RsPgpId>& pgp_ids,std::string& backup_file,uint32_t& error_code)
{
    return AuthPGP::removeKeysFromPGPKeyring(pgp_ids,backup_file,error_code) ;
}

bool 	p3Peers::removeFriendLocation(const RsPeerId &sslId)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::removeFriendLocation() " << sslId << std::endl;
#endif
		//will remove if it's a ssl id
        mPeerMgr->removeFriend(sslId, false);

        FriendsChanged(sslId,false);
        return true;
}

bool 	p3Peers::removeFriend(const RsPgpId& gpgId)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::removeFriend() " << gpgId << std::endl;
#endif
    if (gpgId == AuthPGP::getPgpOwnId()) {
        std::cerr << "p3Peers::removeFriend() ERROR  we're not going to remove our own GPG id."  << std::endl;
		return false;
	}

#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::removeFriend() Removing GPG Id: " << gpgId << std::endl;
#endif
    if (AuthPGP::AllowConnection(gpgId, false))
	{
#ifdef P3PEERS_DEBUG
		std::cerr << "p3Peers::removeFriend() OK." << std::endl;
#endif
		mPeerMgr->removeAllFriendLocations(gpgId);	
		return true;
	}
	else
	{
#ifdef P3PEERS_DEBUG
		std::cerr << "p3Peers::removeFriend() FAILED." << std::endl;
#endif
		mPeerMgr->removeAllFriendLocations(gpgId);
		return false;
	}
}



	/* Network Stuff */
bool 	p3Peers::connectAttempt(const RsPeerId &id)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::connectAttempt() " << id << std::endl;
#endif

	return mLinkMgr->retryConnect(id);
}

void p3Peers::getIPServersList(std::list<std::string>& ip_servers) 
{
	mNetMgr->getIPServersList(ip_servers) ;
}
void p3Peers::getCurrentExtIPList(std::list<std::string>& ip_list)
{
	mNetMgr->getCurrentExtIPList(ip_list) ;
}
bool p3Peers::resetOwnExternalAddressList()
{
	//TODO Phenom 2021-10-30: Need to call something like mNetMgr->netReset();
	// to update this addresslist.
	return mPeerMgr->resetOwnExternalAddressList();
}
void p3Peers::allowServerIPDetermination(bool b)
{
	mNetMgr->setIPServersEnabled(b) ;
}

bool p3Peers::getAllowServerIPDetermination()
{
	return mNetMgr->getIPServersEnabled() ;
}

bool 	p3Peers::setLocation(const RsPeerId &ssl_id, const std::string &location)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setLocation() " << ssl_id << std::endl;
#endif

        return mPeerMgr->setLocation(ssl_id, location);
}


bool	splitAddressString(const std::string &addr, std::string &domain, uint16_t &port)
{
        std::cerr << "splitAddressString() Input: " << addr << std::endl;

	size_t cpos = addr.rfind(':');
	if (cpos == std::string::npos)
	{
        	std::cerr << "splitAddressString Failed to parse (:)";
		std::cerr << std::endl;
		return false;
	}

	int lenport = addr.length() - (cpos + 1); // +1 to skip over : char.
	if (lenport <= 0)
	{
        	std::cerr << "splitAddressString() Missing Port ";
		std::cerr << std::endl;
		return false;
	}

	domain = addr.substr(0, cpos);
	std::string portstr = addr.substr(cpos + 1, std::string::npos);
	int portint = atoi(portstr.c_str());

	if ((portint < 0) || (portint > 65535))
	{
        	std::cerr << "splitAddressString() Invalid Port";
		std::cerr << std::endl;
		return false;
	}
	port = portint;

        std::cerr << "splitAddressString() Domain: " << domain << " Port: " << port;
	std::cerr << std::endl;
	return true;
}


bool 	p3Peers::setHiddenNode(const RsPeerId &id, const std::string &hidden_node_address)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setHiddenNode() " << id << std::endl;
#endif

	std::string domain;
	uint16_t port;
	if (!splitAddressString(hidden_node_address, domain, port))
	{
		return false;
	}
	mPeerMgr->setNetworkMode(id, RS_NET_MODE_HIDDEN);
	mPeerMgr->setHiddenDomainPort(id, domain, port);
	return true;
}


bool p3Peers::isHiddenNode(const RsPeerId &id)
{
	return mPeerMgr->isHiddenNode(id) ;
}

bool 	p3Peers::setHiddenNode(const RsPeerId &id, const std::string &address, uint16_t port)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setHiddenNode() " << id << std::endl;
#endif
        std::cerr << "p3Peers::setHiddenNode() Domain: " << address << " Port: " << port;
	std::cerr << std::endl;

	mPeerMgr->setNetworkMode(id, RS_NET_MODE_HIDDEN);
	mPeerMgr->setHiddenDomainPort(id, address, port);
	return true;
}

bool p3Peers::addPeerLocator(const RsPeerId &ssl_id, const RsUrl& locator)
{ return mPeerMgr->addPeerLocator(ssl_id, locator); }

bool p3Peers::setLocalAddress(const RsPeerId &id,
                              const std::string &addr_str, uint16_t port)
{
#ifdef P3PEERS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " " << id << " " << addr_str << " "
	          << port << std::endl;
#endif

	sockaddr_storage addr;
	if (sockaddr_storage_inet_pton(addr, addr_str))
		if (sockaddr_storage_setport(addr, port))
			return mPeerMgr->setLocalAddress(id, addr);
	return false;
}

bool p3Peers::setExtAddress(const RsPeerId &id,
                            const std::string &addr_str, uint16_t port)
{
#ifdef P3PEERS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " " << id << " " << addr_str << " "
	          << port << std::endl;
#endif

	sockaddr_storage addr;
	if (sockaddr_storage_inet_pton(addr, addr_str))
		if (sockaddr_storage_setport(addr, port))
			return mPeerMgr->setExtAddress(id, addr);
	return false;
}

bool p3Peers::setDynDNS(const RsPeerId &id, const std::string &dyndns)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setDynDNS() called with id: " << id << " dyndns: " << dyndns <<std::endl;
#endif
    return mPeerMgr->setDynDNS(id, dyndns);
}

bool 	p3Peers::setNetworkMode(const RsPeerId &id, uint32_t extNetMode)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setNetworkMode() " << id << std::endl;
#endif

	/* translate */
	uint32_t netMode = 0;
	switch(extNetMode)
	{
		case RS_NETMODE_EXT:
			netMode = RS_NET_MODE_EXT;
			break;
		case RS_NETMODE_UPNP:
			netMode = RS_NET_MODE_UPNP;
			break;
		case RS_NETMODE_UDP:
			netMode = RS_NET_MODE_UDP;
			break;
		case RS_NETMODE_HIDDEN:
			netMode = RS_NET_MODE_HIDDEN;
			break;
		case RS_NETMODE_UNREACHABLE:
			netMode = RS_NET_MODE_UNREACHABLE;
			break;
		default:
			break;
	}

	return mPeerMgr->setNetworkMode(id, netMode);
}


bool p3Peers::setVisState(const RsPeerId &id, uint16_t vs_disc, uint16_t vs_dht)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setVisState() " << id << std::endl;
#endif
	std::cerr << "p3Peers::setVisState() " << id << " DISC: " << vs_disc;
	std::cerr << " DHT: " << vs_dht << std::endl;

	return mPeerMgr->setVisState(id, vs_disc, vs_dht);
}

bool p3Peers::getProxyServer(const uint32_t type, std::string &addr, uint16_t &port, uint32_t &status)
{
	#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getProxyServer()" << std::endl;
    #endif

	struct sockaddr_storage proxy_addr;
	mPeerMgr->getProxyServerAddress(type, proxy_addr);
	addr = sockaddr_storage_iptostring(proxy_addr);
	port = sockaddr_storage_port(proxy_addr);
	mPeerMgr->getProxyServerStatus(type, status);
    return true;
}

bool p3Peers::setProxyServer(const uint32_t type, const std::string &addr_str, const uint16_t port)
{
	#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setProxyServer() " << std::endl;
    #endif

		if(port < 1024)
        {
            std::cerr << "(EE) attempt to set proxy server address to something not allowed: " << addr_str << ":" << port << std::endl;
            return false ;
        }

        std::cerr << "Settign proxy server address to " << addr_str << ":" << port << std::endl;

	struct sockaddr_storage addr;
	struct sockaddr_in *addrv4p = (struct sockaddr_in *) &addr;
	addrv4p->sin_family = AF_INET;
	addrv4p->sin_port = htons(port);

	int ret = 1;
/********************************** WINDOWS/UNIX SPECIFIC PART *******************/
#ifndef WINDOWS_SYS
	if (ret && (0 != inet_aton(addr_str.c_str(), &(addrv4p->sin_addr))))
#else
	addrv4p->sin_addr.s_addr = inet_addr(addr_str.c_str());
	if (ret)
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART *******************/
	{
		return mPeerMgr->setProxyServerAddress(type, addr);
	}
	else
	{
        	std::cerr << "p3Peers::setProxyServer() Failed to Parse Address" << std::endl;
	}

	return false;
}





//===========================================================================
	/* Auth Stuff */

std::string p3Peers::getPGPKey(const RsPgpId& pgp_id,bool include_signatures)
{
	rs_owner_ptr<unsigned char> mem_block = nullptr;
	size_t mem_block_size = 0;

    if( !AuthPGP::exportPublicKey( RsPgpId(pgp_id), mem_block, mem_block_size, false, include_signatures ) )
	{
		RsErr() << __PRETTY_FUNCTION__
		        << " Failure retriving certificate for id " << pgp_id
		        << std::endl;
		return "";
	}

	RsPeerDetails details;
	if(!getGPGDetails(pgp_id, details)) return "";

	auto certPtr =
	        RsCertificate::fromMemoryBlock(details, mem_block, mem_block_size);

	free(mem_block);

	if(certPtr) return certPtr->armouredPGPKey();

	return "";
}


bool p3Peers::GetPGPBase64StringAndCheckSum(
        const RsPgpId& gpg_id,
        std::string& gpg_base64_string, std::string& gpg_base64_checksum )
{
	gpg_base64_string = "" ;
	gpg_base64_checksum = "" ;

	rs_owner_ptr<unsigned char> mem_block = nullptr;
	size_t mem_block_size = 0;
    if(!AuthPGP::exportPublicKey( gpg_id,mem_block,mem_block_size,false,false ))
		return false;

	RsBase64::encode(mem_block, mem_block_size, gpg_base64_string, true, false);

	uint32_t crc = PGPKeyManagement::compute24bitsCRC(mem_block,mem_block_size);

	free(mem_block);

	unsigned char tmp[3] = {
	    uint8_t((crc >> 16) & 0xff),
	    uint8_t((crc >> 8) & 0xff),
	    uint8_t(crc & 0xff) } ;
	RsBase64::encode(tmp, 3, gpg_base64_checksum, true, false);

	return true;
}

static void addPacketHeader(RsShortInviteFieldType ptag, size_t size, unsigned char *& buf, uint32_t& offset, uint32_t& buf_size)
{
	// Check that the buffer has sufficient size. If not, increase it.

	while(offset + size + 6 >= buf_size)
	{
		unsigned char *newbuf = new unsigned char[2*buf_size] ;

		memcpy(newbuf, buf, buf_size) ;
		buf_size *= 2 ;
		delete[] buf ;
		buf = newbuf ;
	}

	// Write ptag and size

	buf[offset] = static_cast<uint8_t>(ptag) ;
	offset += 1 ;

	offset += PGPKeyParser::write_125Size(&buf[offset],size) ;
}

bool p3Peers::getShortInvite(std::string& invite, const RsPeerId& _sslId, RetroshareInviteFlags invite_flags, const std::string& baseUrl )
{
	RsPeerId sslId = _sslId;
	if(sslId.isNull()) sslId = getOwnId();

	RsPeerDetails tDetails;
	if(!getPeerDetails(sslId, tDetails)) return false;

    uint32_t buf_size = 100;
    uint32_t offset = 0;
    unsigned char *buf = (unsigned char*)malloc(buf_size);

    addPacketHeader(RsShortInviteFieldType::SSL_ID,RsPeerId::SIZE_IN_BYTES,buf,offset,buf_size);
	sslId.serialise(buf,buf_size,offset);

    addPacketHeader(RsShortInviteFieldType::PGP_FINGERPRINT,RsPgpFingerprint::SIZE_IN_BYTES,buf,offset,buf_size);
	tDetails.fpr.serialise(buf,buf_size,offset);

    addPacketHeader(RsShortInviteFieldType::PEER_NAME,tDetails.name.size(),buf,offset,buf_size);
	memcpy(&buf[offset],tDetails.name.c_str(),tDetails.name.size());
    offset += tDetails.name.size();

    /* If it is a hidden node, always use hidden address and port as locator */

    if(tDetails.isHiddenNode)
    {
        addPacketHeader(RsShortInviteFieldType::HIDDEN_LOCATOR,4 + 2 + tDetails.hiddenNodeAddress.size(),buf,offset,buf_size);

        buf[offset+0] = (uint8_t)((tDetails.hiddenType >> 24) & 0xff);
        buf[offset+1] = (uint8_t)((tDetails.hiddenType >> 16) & 0xff);
        buf[offset+2] = (uint8_t)((tDetails.hiddenType >>  8) & 0xff);
        buf[offset+3] = (uint8_t)((tDetails.hiddenType      ) & 0xff);

        buf[offset+4] = (uint8_t)((tDetails.hiddenNodePort >> 8) & 0xff);
        buf[offset+5] = (uint8_t)((tDetails.hiddenNodePort     ) & 0xff);

        memcpy(&buf[offset+6],tDetails.hiddenNodeAddress.c_str(),tDetails.hiddenNodeAddress.size());
        offset += 4 + 2 + tDetails.hiddenNodeAddress.size();
    }

    if( !!(invite_flags & RetroshareInviteFlags::DNS) && !tDetails.dyndns.empty() && (tDetails.extPort || tDetails.localPort))
    {
        uint16_t tPort = tDetails.extPort ? tDetails.extPort : tDetails.localPort;

        addPacketHeader(RsShortInviteFieldType::DNS_LOCATOR, 2 + tDetails.dyndns.size(),buf,offset,buf_size);

        buf[offset+0] = (uint8_t)((tPort >> 8) & 0xff);
        buf[offset+1] = (uint8_t)((tPort     ) & 0xff);

        memcpy(&buf[offset+2],tDetails.dyndns.c_str(),tDetails.dyndns.size());
        offset += 2 + tDetails.dyndns.size();
    }

    if( !!(invite_flags & RetroshareInviteFlags::FULL_IP_HISTORY) && (!tDetails.ipAddressList.empty()))
        for(auto& s: tDetails.ipAddressList)
        {
            const std::string& tLc = s;
            std::string tLocator = tLc.substr(0, tLc.find_first_of(" ")-1);

            addPacketHeader(RsShortInviteFieldType::LOCATOR, tLocator.size(),buf,offset,buf_size);
            memcpy(&buf[offset],tLocator.c_str(),tLocator.size());

            offset += tLocator.size();
        }
    else if( !!(invite_flags & RetroshareInviteFlags::CURRENT_IP) )	// only add at least the local and external IPs
    {
#ifdef USE_NEW_LOCATOR_SYSTEM
        // This new locator system as some advantages, but here it also has major drawbacks: (1) it cannot differentiate local and external addresses,
        // and (2) it's quite larger than the old system, which tends to make certificates more than 1 line long.

        sockaddr_storage tLocal;

        if(sockaddr_storage_inet_pton(tLocal, tDetails.localAddr) && sockaddr_storage_isValidNet(tLocal) && tDetails.localPort )
        {
            addPacketHeader(RsShortInviteFieldType::LOCATOR, tDetails.localAddr.size(),buf,offset,buf_size);
            memcpy(&buf[offset],tDetails.localAddr.c_str(),tDetails.localAddr.size());

            offset += tDetails.localAddr.size();
        }
        sockaddr_storage tExt;

        if(sockaddr_storage_inet_pton(tExt, tDetails.extAddr) && sockaddr_storage_isValidNet(tExt) && tDetails.extPort )
        {
            addPacketHeader(RsShortInviteFieldType::LOCATOR, tDetails.extAddr.size(),buf,offset,buf_size);
            memcpy(&buf[offset],tDetails.extAddr.c_str(),tDetails.extAddr.size());

            offset += tDetails.extAddr.size();
        }
#else
        sockaddr_storage tLocal;
        bool validLoc =   sockaddr_storage_inet_pton(tLocal, tDetails.localAddr)
                       && sockaddr_storage_isValidNet(tLocal)
                       && tDetails.localPort;
        bool isLocIpv4 = sockaddr_storage_ipv6_to_ipv4(tLocal);
        if(validLoc && isLocIpv4)
        {
            uint32_t t4Addr = reinterpret_cast<sockaddr_in&>(tLocal).sin_addr.s_addr;

            addPacketHeader(RsShortInviteFieldType::LOC4_LOCATOR, 4 + 2,buf,offset,buf_size);

            buf[offset+0] = (uint8_t)((t4Addr >> 24) & 0xff);
            buf[offset+1] = (uint8_t)((t4Addr >> 16) & 0xff);
            buf[offset+2] = (uint8_t)((t4Addr >>  8) & 0xff);
            buf[offset+3] = (uint8_t)((t4Addr      ) & 0xff);

            buf[offset+4] = (uint8_t)((tDetails.localPort >> 8) & 0xff);
            buf[offset+5] = (uint8_t)((tDetails.localPort     ) & 0xff);

            offset += 4+2;
        }

        sockaddr_storage tExt;
        bool validExt =   sockaddr_storage_inet_pton(tExt, tDetails.extAddr)
                       && sockaddr_storage_isValidNet(tExt)
                       && tDetails.extPort;
        bool isExtIpv4 = sockaddr_storage_ipv6_to_ipv4(tExt);
        if(validExt && isExtIpv4)
        {
            uint32_t t4Addr = reinterpret_cast<sockaddr_in&>(tExt).sin_addr.s_addr;

            addPacketHeader(RsShortInviteFieldType::EXT4_LOCATOR, 4 + 2,buf,offset,buf_size);

            buf[offset+0] = (uint8_t)((t4Addr >> 24) & 0xff);
            buf[offset+1] = (uint8_t)((t4Addr >> 16) & 0xff);
            buf[offset+2] = (uint8_t)((t4Addr >>  8) & 0xff);
            buf[offset+3] = (uint8_t)((t4Addr      ) & 0xff);

            buf[offset+4] = (uint8_t)((tDetails.extPort >> 8) & 0xff);
            buf[offset+5] = (uint8_t)((tDetails.extPort     ) & 0xff);

            offset += 4+2;
        }
        else if(validExt && !isExtIpv4)
        {
            // External address is IPv6, save it on LOCATOR
            sockaddr_storage_setport(tExt,tDetails.extPort);
            std::string tLocator = sockaddr_storage_tostring(tExt);

            addPacketHeader(RsShortInviteFieldType::LOCATOR, tLocator.size(),buf,offset,buf_size);
            memcpy(&buf[offset],tLocator.c_str(),tLocator.size());

            offset += tLocator.size();
        }
#endif

    }

    uint32_t computed_crc = PGPKeyManagement::compute24bitsCRC(buf,offset) ;

	// handle endian issues.
	unsigned char mem[3] ;
	mem[0] =  computed_crc        & 0xff ;
	mem[1] = (computed_crc >> 8 ) & 0xff ;
	mem[2] = (computed_crc >> 16) & 0xff ;

	addPacketHeader( RsShortInviteFieldType::CHECKSUM,3,buf,offset,buf_size);
	memcpy(&buf[offset],mem,3);
    offset += 3;

	Radix64::encode(buf, static_cast<int>(offset), invite);

    if(!(invite_flags & RetroshareInviteFlags::RADIX_FORMAT))
	{
		RsUrl inviteUrl(baseUrl);
		inviteUrl.setQueryKV("rsInvite", invite);
		invite = inviteUrl.toString();
	}

	return true;
}

bool p3Peers::parseShortInvite(const std::string& inviteStrUrl, RsPeerDetails& details, uint32_t &err_code )
{
    if(inviteStrUrl.empty())
    {
        RsErr() << __PRETTY_FUNCTION__ << " can't parse empty invite"
                << std::endl;
        return false;
    }
    std::string rsInvite = inviteStrUrl;

    RsUrl inviteUrl(inviteStrUrl);

    if(inviteUrl.hasQueryK("rsInvite"))
        rsInvite = *inviteUrl.getQueryV("rsInvite");

    if(!RsCertificate::decodeRadix64ShortInvite(rsInvite, details, err_code))
        return false;

    // Also check if the PGP key is available. If so, add it in the PeerDetails:

    RsPeerDetails pgp_det ;
    if(getGPGDetails(PGPHandler::pgpIdFromFingerprint(details.fpr),pgp_det) && pgp_det.fpr == details.fpr)
    {
        details.issuer      = pgp_det.issuer;
        details.gpg_id      = pgp_det.gpg_id;
        details.gpgSigners  = pgp_det.gpgSigners;
        details.trustLvl    = pgp_det.trustLvl;
        details.validLvl    = pgp_det.validLvl;
        details.ownsign     = pgp_det.ownsign;
        details.hasSignedMe = pgp_det.hasSignedMe;
        details.accept_connection = pgp_det.accept_connection;
    }
    else
        details.skip_pgp_signature_validation = true;

    if(details.gpg_id.isNull())
    {
		err_code = CERTIFICATE_PARSING_ERROR_MISSING_PGP_FINGERPRINT;
        return false;
    }
    err_code = CERTIFICATE_PARSING_ERROR_NO_ERROR;
    return true;
}

bool p3Peers::acceptInvite( const std::string& invite,
                            ServicePermissionFlags flags )
{
	if(invite.empty()) return false;

	const std::string* radixPtr(&invite);

	RsUrl url(invite);
	std::map<std::string, std::string> query(url.query());

	if(query.find("radix") != query.end())
		radixPtr = &query["radix"];

	const std::string& radix(*radixPtr);
	if(radix.empty()) return false;

	RsPgpId pgpId;
	RsPeerId sslId;
	std::string errorString;

	if(!loadCertificateFromString(radix, sslId, pgpId, errorString))
		return false;

	RsPeerDetails peerDetails;
	uint32_t errorCode;

	if(!loadDetailsFromStringCert(radix, peerDetails, errorCode))
		return false;

	if(peerDetails.gpg_id.isNull())
		return false;

	addFriend(peerDetails.id, peerDetails.gpg_id, flags);

	if (!peerDetails.location.empty())
		setLocation(peerDetails.id, peerDetails.location);

	// Update new address even the peer already existed.
	if (peerDetails.isHiddenNode)
	{
		setHiddenNode( peerDetails.id,
		               peerDetails.hiddenNodeAddress,
		               peerDetails.hiddenNodePort );
	}
	else
	{
		//let's check if there is ip adresses in the certificate.
		if (!peerDetails.extAddr.empty() && peerDetails.extPort)
			setExtAddress( peerDetails.id,
			               peerDetails.extAddr,
			               peerDetails.extPort );
		if (!peerDetails.localAddr.empty() && peerDetails.localPort)
			setLocalAddress( peerDetails.id,
			                 peerDetails.localAddr,
			                 peerDetails.localPort );
		if (!peerDetails.dyndns.empty())
			setDynDNS(peerDetails.id, peerDetails.dyndns);
		for(auto&& ipr : peerDetails.ipAddressList)
			addPeerLocator(
			            peerDetails.id,
			            RsUrl(ipr.substr(0, ipr.find(' '))) );
	}

	return true;
}

std::string p3Peers::GetRetroshareInvite( const RsPeerId& sslId, RetroshareInviteFlags invite_flags)
{
#ifdef P3PEERS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << std::endl;
#endif
	const RsPeerId& ssl_id(sslId.isNull() ? getOwnId() : sslId);

	//add the sslid, location, ip local and external address after the signature
	RsPeerDetails detail;
	std::string invite;

	if (getPeerDetails(ssl_id, detail))
	{
		if(   !(invite_flags & RetroshareInviteFlags::FULL_IP_HISTORY)
		   || detail.isHiddenNode)
			detail.ipAddressList.clear();

		//Check if external address is IPv6, then move it to ipAddressList as RsCertificate only allow 4 numbers.
		sockaddr_storage tExt;
		bool validExt =   sockaddr_storage_inet_pton(tExt, detail.extAddr)
		               && sockaddr_storage_isValidNet(tExt)
		               && detail.extPort;
		bool isExtIpv4 = sockaddr_storage_ipv6_to_ipv4(tExt);

		if(   !(invite_flags & RetroshareInviteFlags::FULL_IP_HISTORY)
		   && !detail.isHiddenNode
		   && validExt && !isExtIpv4)
		{
			sockaddr_storage_setport(tExt,detail.extPort);
			detail.ipAddressList.push_front(sockaddr_storage_tostring(tExt) + " "); // Space needed to later parse.
			detail.extAddr = ""; //Clear it to not trigg error.
			detail.extPort = 0;
		}

		unsigned char *mem_block = nullptr;
		size_t mem_block_size = 0;

		if(!AuthPGP::exportPublicKey( RsPgpId(detail.gpg_id), mem_block, mem_block_size, false, !!(invite_flags & RetroshareInviteFlags::PGP_SIGNATURES) ))
		{
			std::cerr << "Cannot output certificate for id \"" << detail.gpg_id
			          << "\". Sorry." << std::endl;
			return "";
		}

		RsCertificate cert(detail, mem_block, mem_block_size);
		free(mem_block);

		return cert.toStdString();
	}

#ifdef P3PEERS_DEBUG
	std::cerr << __PRETTY_FUNCTION__ << " returns : \n" << invite << std::endl;
#endif
	return invite;
}

//===========================================================================

bool p3Peers::loadCertificateFromString(
        const std::string& cert, RsPeerId& ssl_id,
        RsPgpId& gpg_id, std::string& error_string )
{
	uint32_t errNum = 0;
	auto crt = RsCertificate::fromString(cert, errNum);

	if(!crt)
	{
		error_string = "RsCertificate failed with errno: "
		        + std::to_string(errNum) + " parsing: " + cert;
		return false;
	}

	RsPgpId gpgid;
    bool res = AuthPGP::LoadCertificateFromString( crt->armouredPGPKey(), gpgid, error_string );

	gpg_id = gpgid;
	ssl_id = crt->sslid();

    // now get all friends who declare this key ID to be the one needed to check connections, and clear their "skip_pgp_signature_validation" flag

    if(res)
    {
		mPeerMgr->notifyPgpKeyReceived(gpgid);
        FriendsChanged(ssl_id,true);
    }

	return res;
}
bool p3Peers::loadPgpKeyFromBinaryData( const unsigned char *bin_key_data,uint32_t bin_key_len, RsPgpId& gpg_id, std::string& error_string )
{
    bool res = AuthPGP::LoadPGPKeyFromBinaryData( bin_key_data,bin_key_len, gpg_id, error_string );

    if(res)
		mPeerMgr->notifyPgpKeyReceived(gpg_id);

	return res;
}

bool p3Peers::loadDetailsFromStringCert( const std::string &certstr,
                                         RsPeerDetails &pd,
                                         uint32_t& error_code )
{
	Dbg3() << __PRETTY_FUNCTION__ << std::endl;

	auto certPtr = RsCertificate::fromString(certstr, error_code);
	if(!certPtr) return false;

	RsCertificate& cert = *certPtr;

    if(!AuthPGP::getPgpDetailsFromBinaryBlock( cert.pgp_key(), cert.pgp_key_size(), pd.gpg_id, pd.name, pd.gpgSigners ))
		return false;

	Dbg4() << __PRETTY_FUNCTION__ << " Parsing cert for sslid, location, ext "
	       << " and local address details. : " << certstr << std::endl;

	pd.id = cert.sslid();
	pd.location = cert.location_name_string();

	pd.isOnlyGPGdetail = pd.id.isNull();
	pd.service_perm_flags = RS_NODE_PERM_DEFAULT;

	if (!cert.hidden_node_string().empty())
	{
		pd.isHiddenNode = true;

		std::string domain;
		uint16_t port;
		if (splitAddressString(cert.hidden_node_string(), domain, port))
		{
			pd.hiddenNodeAddress = domain;
			pd.hiddenNodePort = port;
			pd.hiddenType = mPeerMgr->hiddenDomainToHiddenType(domain);
		}
	}
	else
	{
		pd.isHiddenNode = false;
		pd.localAddr = cert.loc_ip_string();
		pd.localPort = cert.loc_port_us();
		pd.extAddr = cert.ext_ip_string();
		pd.extPort = cert.ext_port_us();
		pd.dyndns = cert.dns_string();
		for(const RsUrl& locator : cert.locators())
			pd.ipAddressList.push_back(locator.toString());
	}

	return true;
}

bool p3Peers::cleanCertificate(const std::string &certstr, std::string &cleanCert,bool& is_short_format,uint32_t& error_code,RsPeerDetails& details)
{
	RsCertificate::Format format ;

	bool res = RsCertificate::cleanCertificate(certstr,cleanCert,format,error_code,true,details) ;

    if(format == RsCertificate::RS_CERTIFICATE_RADIX)
        is_short_format = false;
    else if(format == RsCertificate::RS_CERTIFICATE_SHORT_RADIX)
        is_short_format = true;
    else
        return false ;

    return res;
}

std::string p3Peers::saveCertificateToString(const RsPeerId &id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::SaveCertificateToString() " << id;
	std::cerr << std::endl;
#endif
        if (id == AuthSSL::getAuthSSL()->OwnId()) {
            return AuthSSL::getAuthSSL()->SaveOwnCertificateToString();
        } else {
            return "";
        }
}

bool 	p3Peers::signGPGCertificate(const RsPgpId &id, const std::string &gpg_passphrase)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::SignCertificate() " << id;
	std::cerr << std::endl;
#endif
        rsNotify->cachePgpPassphrase(gpg_passphrase);
        rsNotify->setDisableAskPassword(true);

        bool res = AuthPGP::SignCertificateLevel0(id);

        rsNotify->clearPgpPassphrase();
        rsNotify->setDisableAskPassword(false);

        return res;
}

bool 	p3Peers::trustGPGCertificate(const RsPgpId &id, uint32_t trustlvl)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::TrustCertificate() " << id;
	std::cerr << std::endl;
#endif
    return AuthPGP::TrustCertificate(id, trustlvl);
}

	/* Group Stuff */
bool p3Peers::addGroup(RsGroupInfo &groupInfo)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::addGroup()" << std::endl;
#endif

		  bool res = mPeerMgr->addGroup(groupInfo);
		  rsFiles->updateSinceGroupPermissionsChanged() ;
		  return res ;
}

bool p3Peers::editGroup(const RsNodeGroupId &groupId, RsGroupInfo &groupInfo)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::editGroup()" << std::endl;
#endif

	bool res = mPeerMgr->editGroup(groupId, groupInfo);
	rsFiles->updateSinceGroupPermissionsChanged() ;

	return res ;
}

bool p3Peers::removeGroup(const RsNodeGroupId &groupId)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::removeGroup()" << std::endl;
#endif

		  bool res = mPeerMgr->removeGroup(groupId);
		  rsFiles->updateSinceGroupPermissionsChanged() ;
		  return res ;
}

bool p3Peers::getGroupInfoByName(const std::string& groupName, RsGroupInfo &groupInfo)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getGroupInfo()" << std::endl;
#endif

    return mPeerMgr->getGroupInfoByName(groupName, groupInfo);
}
bool p3Peers::getGroupInfo(const RsNodeGroupId &groupId, RsGroupInfo &groupInfo)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getGroupInfo()" << std::endl;
#endif

	return mPeerMgr->getGroupInfo(groupId, groupInfo);
}

bool p3Peers::getGroupInfoList(std::list<RsGroupInfo> &groupInfoList)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getGroupInfoList()" << std::endl;
#endif

	return mPeerMgr->getGroupInfoList(groupInfoList);
}

bool p3Peers::assignPeerToGroup(const RsNodeGroupId &groupId, const RsPgpId& peerId, bool assign)
{
	std::list<RsPgpId> peerIds;
	peerIds.push_back(peerId);

	return assignPeersToGroup(groupId, peerIds, assign);
}

bool p3Peers::assignPeersToGroup(const RsNodeGroupId &groupId, const std::list<RsPgpId> &peerIds, bool assign)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::assignPeersToGroup()" << std::endl;
#endif

	bool res = mPeerMgr->assignPeersToGroup(groupId, peerIds, assign);
	rsFiles->updateSinceGroupPermissionsChanged() ;

	return res ;
}

FileSearchFlags p3Peers::computePeerPermissionFlags(const RsPeerId& peer_ssl_id,
																		FileStorageFlags share_flags,
                                                                        const std::list<RsNodeGroupId>& directory_parent_groups)
{
	// We should be able to do that in O(1), using groups based on packs of bits.
	//
	// But for now, because the implementation of groups is not totally decided yet, we revert to this
	// very simple algorithm.
	//

    bool found = directory_parent_groups.empty() ;	// by default, empty list means browsable by everyone.
	RsPgpId pgp_id = getGPGId(peer_ssl_id) ;

    for(std::list<RsNodeGroupId>::const_iterator it(directory_parent_groups.begin());it!=directory_parent_groups.end() && !found;++it)
	{
		RsGroupInfo info ;
		if(!getGroupInfo(*it,info))
		{
			std::cerr << "(EE) p3Peers::computePeerPermissionFlags: no group named " << *it << ": cannot get info." << std::endl;
			continue ;
		}

        found = found || (info.peerIds.find(pgp_id) != info.peerIds.end()) ;

        //for(std::set<RsPgpId>::const_iterator it2(info.peerIds.begin());it2!=info.peerIds.end() && !found;++it2)
        //	if(*it2 == pgp_id)
        //		found = true ;
	}

	bool network_wide = (share_flags & DIR_FLAGS_ANONYMOUS_DOWNLOAD) ;//|| ( (share_flags & DIR_FLAGS_NETWORK_WIDE_GROUPS) && found) ;
    bool browsable    = (share_flags & DIR_FLAGS_BROWSABLE) && found ;
    bool searchable   = (share_flags & DIR_FLAGS_ANONYMOUS_SEARCH) ;

	FileSearchFlags final_flags ;

	if(network_wide) final_flags |= RS_FILE_HINTS_NETWORK_WIDE ;
	if(browsable   ) final_flags |= RS_FILE_HINTS_BROWSABLE ;
    if(searchable  ) final_flags |= RS_FILE_HINTS_SEARCHABLE ;

	return final_flags ;
}

RsPeerDetails::RsPeerDetails()
        :isOnlyGPGdetail(false),
          name(""),email(""),location(""),
          org(""),authcode(""),
          trustLvl(0), validLvl(0),skip_pgp_signature_validation(false),ownsign(false),
          hasSignedMe(false),accept_connection(false),
          state(0),actAsServer(false),
          connectPort(0),
          isHiddenNode(false),
          hiddenNodePort(0),
          hiddenType(RS_HIDDEN_TYPE_NONE),
          localAddr(""),localPort(0),extAddr(""),extPort(0),netMode(0),vs_disc(0), vs_dht(0),
          lastConnect(0),lastUsed(0),connectState(0),connectStateString(""),
          connectPeriod(0),
          foundDHT(false), wasDeniedConnection(false), deniedTS(0),
          linkType ( RS_NET_CONN_TRANS_TCP_UNKNOWN) {}

RsGroupInfo::RsGroupInfo() : flag(0) {}

ServicePermissionFlags p3Peers::servicePermissionFlags(const RsPeerId& ssl_id) 
{
	return mPeerMgr->servicePermissionFlags(ssl_id) ;
}
ServicePermissionFlags p3Peers::servicePermissionFlags(const RsPgpId& gpg_id) 
{
	return mPeerMgr->servicePermissionFlags(gpg_id) ;
}
void p3Peers::setServicePermissionFlags(const RsPgpId& gpg_id,const ServicePermissionFlags& flags) 
{
	mPeerMgr->setServicePermissionFlags(gpg_id,flags) ;
}

RsPeerStateChangedEvent::RsPeerStateChangedEvent(RsPeerId sslId) :
    RsEvent(RsEventType::PEER_STATE_CHANGED), mSslId(sslId) {}

RsPeers::~RsPeers() = default;
RsAuthSslConnectionAutenticationEvent::~RsAuthSslConnectionAutenticationEvent() = default;
RsConnectionEvent::~RsConnectionEvent() = default;
