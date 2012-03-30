/*
 * libretroshare/src/rsserver: p3peers.cc
 *
 * RetroShare C++ Interface.
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

#include "rsserver/p3peers.h"
#include "rsserver/p3face.h"

#include "pqi/p3linkmgr.h"
#include "pqi/p3peermgr.h"
#include "pqi/p3netmgr.h"

#include "pqi/authssl.h"
#include "pqi/authgpg.h"
#include "retroshare/rsinit.h"
#include "pqi/cleanupxpgp.h"


#include <iostream>
#include <fstream>
#include <sstream>

#include <gpgme.h>

const std::string CERT_SSL_ID = "--SSLID--";
const std::string CERT_LOCATION = "--LOCATION--";
const std::string CERT_LOCAL_IP = "--LOCAL--";
const std::string CERT_EXT_IP = "--EXT--";
const std::string CERT_DYNDNS = "--DYNDNS--";

static const int MAX_TIME_KEEP_LOCATION_WITHOUT_CONTACT = 30*24*3600 ; // 30 days.


#include "pqi/authssl.h"


RsPeers *rsPeers = NULL;

/*******
 * #define P3PEERS_DEBUG 1
 *******/

int ensureExtension(std::string &name, std::string def_ext);

std::string RsPeerTrustString(uint32_t trustLvl)
{

	std::string str;

	switch(trustLvl)
	{
		default:
		case GPGME_VALIDITY_UNKNOWN:
			str = "GPGME_VALIDITY_UNKNOWN";
			break;
		case GPGME_VALIDITY_UNDEFINED:
			str = "GPGME_VALIDITY_UNDEFINED";
			break;
		case GPGME_VALIDITY_NEVER:
			str = "GPGME_VALIDITY_NEVER";
			break;
		case GPGME_VALIDITY_MARGINAL:
			str = "GPGME_VALIDITY_MARGINAL";
			break;
		case GPGME_VALIDITY_FULL:
			str = "GPGME_VALIDITY_FULL";
			break;
		case GPGME_VALIDITY_ULTIMATE:
			str = "GPGME_VALIDITY_ULTIMATE";
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


std::string RsPeerLastConnectString(uint32_t lastConnect)
{
	std::ostringstream out;
	out << lastConnect << " secs ago";
	return out.str();
}


p3Peers::p3Peers(p3LinkMgr *lm, p3PeerMgr *pm, p3NetMgr *nm)
        :mLinkMgr(lm), mPeerMgr(pm), mNetMgr(nm)
{
	return;
}

bool p3Peers::hasExportMinimal()
{
#ifdef GPGME_EXPORT_MODE_MINIMAL
	return true ;
#else
	return false ;
#endif
}

	/* Updates ... */
bool p3Peers::FriendsChanged()
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::FriendsChanged()" << std::endl;
#endif

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
std::string p3Peers::getOwnId()
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getOwnId()" << std::endl;
#endif

        return AuthSSL::getAuthSSL()->OwnId();
}

bool	p3Peers::getOnlineList(std::list<std::string> &ids)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getOnlineList()" << std::endl;
#endif

	/* get from mConnectMgr */
	mLinkMgr->getOnlineList(ids);
	return true;
}

bool	p3Peers::getFriendList(std::list<std::string> &ids)
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

bool p3Peers::getPeerCount (unsigned int *friendCount, unsigned int *onlineCount, bool ssl)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPeerCount()" << std::endl;
#endif

	if (friendCount) *friendCount = mPeerMgr->getFriendCount(ssl, false);
	if (onlineCount) *onlineCount = mPeerMgr->getFriendCount(ssl, true);

	return true;
}

bool    p3Peers::isOnline(const std::string &id)
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

bool    p3Peers::isFriend(const std::string &ssl_id)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::isFriend() " << ssl_id << std::endl;
#endif

        /* get from mConnectMgr */
        return mPeerMgr->isFriend(ssl_id);
}


/* There are too many dependancies of this function
 * to shift it immeidately
 */

bool	p3Peers::getPeerDetails(const std::string &id, RsPeerDetails &d)
{
        #ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPeerDetails() called for id : " << id << std::endl;
        #endif

	// NOW Only for SSL Details.

        std::string sOwnId = AuthSSL::getAuthSSL()->OwnId();
        peerState ps;

	if (id == sOwnId)
	{
                mPeerMgr->getOwnNetStatus(ps);
                ps.gpg_id = AuthGPG::getAuthGPG()->getGPGOwnId();
	}
	else
	{
        	if (!mPeerMgr->getFriendNetStatus(id, ps))
		{
#ifdef P3PEERS_DEBUG
			std::cerr << "p3Peers::getPeerDetails() ERROR not an SSL Id: " << id << std::endl;
#endif
        		d.isOnlyGPGdetail = true;
        		return getGPGDetails(id, d);
		}
	}

        /* get from gpg (first), to fill in the sign and trust details */
        /* don't retrun now, we've got fill in the ssl and connection info */
        getGPGDetails(ps.gpg_id, d);
        d.isOnlyGPGdetail = false;

        //get the ssl details
        d.id 		= id;
        d.location 	= ps.location;

	/* generate */
	d.authcode  	= "AUTHCODE";

	/* fill from pcs */

	d.localAddr	= rs_inet_ntoa(ps.localaddr.sin_addr);
	d.localPort	= ntohs(ps.localaddr.sin_port);
	d.extAddr	= rs_inet_ntoa(ps.serveraddr.sin_addr);
	d.extPort	= ntohs(ps.serveraddr.sin_port);
        d.dyndns        = ps.dyndns;
	d.lastConnect	= ps.lastcontact;
	d.connectPeriod = 0;


	std::list<pqiIpAddress>::iterator it;
	for(it = ps.ipAddrs.mLocal.mAddrs.begin(); 
			it != ps.ipAddrs.mLocal.mAddrs.end(); it++)
	{
	    std::ostringstream toto;
            toto << ntohs(it->mAddr.sin_port) << "    " << (time(NULL) - it->mSeenTime) << " sec";
            d.ipAddressList.push_back("L:" + std::string(rs_inet_ntoa(it->mAddr.sin_addr)) + ":" + toto.str());
	}
	for(it = ps.ipAddrs.mExt.mAddrs.begin(); 
			it != ps.ipAddrs.mExt.mAddrs.end(); it++)
	{
	    std::ostringstream toto;
            toto << ntohs(it->mAddr.sin_port) << "    " << (time(NULL) - it->mSeenTime) << " sec";
            d.ipAddressList.push_back("E:" + std::string(rs_inet_ntoa(it->mAddr.sin_addr)) + ":" + toto.str());
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
		case RS_NET_MODE_UNREACHABLE:
		case RS_NET_MODE_UNKNOWN:
		default:
			d.netMode	= RS_NETMODE_UNREACHABLE;
			break;
	}
	
	d.visState	= 0;
	if (!(ps.visState & RS_VIS_STATE_NODISC))
	{
		d.visState |= RS_VS_DISC_ON;
	}
	
	if (!(ps.visState & RS_VIS_STATE_NODHT))
	{
		d.visState |= RS_VS_DHT_ON;
	}
	
	
	
	
	/* Translate */
	peerConnectState pcs;
	if (!mLinkMgr->getFriendNetStatus(id, pcs)) 
	{
		std::cerr << "p3Peers::getPeerDetails() ERROR No Link Information : " << id << std::endl;
		return true;
	}
	
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPeerDetails() got a SSL id and is returning SSL and GPG details for id : " << id << std::endl;
#endif
	
	
	d.state		= 0;
	if (pcs.state & RS_PEER_S_FRIEND)
		d.state |= RS_PEER_STATE_FRIEND;
	if (pcs.state & RS_PEER_S_ONLINE)
		d.state |= RS_PEER_STATE_ONLINE;
	if (pcs.state & RS_PEER_S_CONNECTED)
		d.state |= RS_PEER_STATE_CONNECTED;
	if (pcs.state & RS_PEER_S_UNREACHABLE)
		d.state |= RS_PEER_STATE_UNREACHABLE;

	d.linkType = pcs.linkType;

	/* Finally determine AutoConnect Status */
	d.foundDHT = pcs.dht.found;

	d.connectState = 0;
	d.connectStateString.clear();


	if (pcs.inConnAttempt)
	{
		if (pcs.currentConnAddrAttempt.type & RS_NET_CONN_TUNNEL) {
			d.connectState = RS_PEER_CONNECTSTATE_TRYING_TUNNEL;
		} else if (pcs.currentConnAddrAttempt.type & RS_NET_CONN_TCP_ALL) {
			d.connectState = RS_PEER_CONNECTSTATE_TRYING_TCP;

			std::ostringstream str;
			str << rs_inet_ntoa(pcs.currentConnAddrAttempt.addr.sin_addr) << ":" <<  ntohs(pcs.currentConnAddrAttempt.addr.sin_port);
			d.connectStateString = str.str();
		} else if (pcs.currentConnAddrAttempt.type & RS_NET_CONN_UDP_ALL) {
			d.connectState = RS_PEER_CONNECTSTATE_TRYING_UDP;

			std::ostringstream str;
			str << rs_inet_ntoa(pcs.currentConnAddrAttempt.addr.sin_addr) << ":" <<  ntohs(pcs.currentConnAddrAttempt.addr.sin_port);
			d.connectStateString = str.str();
		}
	}
	else if (pcs.state & RS_PEER_S_CONNECTED)
	{
		if (pcs.connecttype == RS_NET_CONN_TCP_ALL)
		{
			d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_TCP;
		}
		else if (pcs.connecttype == RS_NET_CONN_UDP_ALL)
		{
			d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_UDP;
		}
		else if (pcs.connecttype == RS_NET_CONN_TUNNEL)
		{
			d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_TUNNEL;
		}
		else
		{
			d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN;
		}
	}

	return true;
}


#if 0
bool	p3Peers::getPeerDetails(const std::string &id, RsPeerDetails &d)
{
        #ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPeerDetails() called for id : " << id << std::endl;
        #endif
        //first, check if it's a gpg or a ssl id.
        std::string sOwnId = AuthSSL::getAuthSSL()->OwnId();
        peerState ps;
        if (id != sOwnId && !mPeerMgr->getFriendNetStatus(id, ps)) {
            //assume is not SSL, because every ssl_id has got a friend correspondance in mConnMgr
            #ifdef P3PEERS_DEBUG
            std::cerr << "p3Peers::getPeerDetails() got a gpg id and is returning GPG details only for id : " << id << std::endl;
            #endif
            d.isOnlyGPGdetail = true;
            return this->getGPGDetails(id, d);
        }
        #ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPeerDetails() got a SSL id and is returning SSL and GPG details for id : " << id << std::endl;
        #endif

        if (id == sOwnId) {
                mPeerMgr->getOwnNetStatus(ps);
                ps.gpg_id = AuthGPG::getAuthGPG()->getGPGOwnId();
        }

        /* get from gpg (first), to fill in the sign and trust details */
        /* don't retrun now, we've got fill in the ssl and connection info */
        this->getGPGDetails(ps.gpg_id, d);
        d.isOnlyGPGdetail = false;

        //get the ssl details
        d.id 		= id;
        d.location 	= ps.location;

	/* generate */
	d.authcode  	= "AUTHCODE";

	/* fill from pcs */

	d.localAddr	= rs_inet_ntoa(ps.localaddr.sin_addr);
	d.localPort	= ntohs(ps.localaddr.sin_port);
	d.extAddr	= rs_inet_ntoa(ps.serveraddr.sin_addr);
	d.extPort	= ntohs(ps.serveraddr.sin_port);
        d.dyndns        = ps.dyndns;
	d.lastConnect	= ps.lastcontact;
	d.connectPeriod = 0;


	std::list<pqiIpAddress>::iterator it;
	for(it = ps.ipAddrs.mLocal.mAddrs.begin(); 
			it != ps.ipAddrs.mLocal.mAddrs.end(); it++)
	{
	    std::ostringstream toto;
            toto << ntohs(it->mAddr.sin_port) << "    " << (time(NULL) - it->mSeenTime) << " sec";
            d.ipAddressList.push_back("L:" + std::string(rs_inet_ntoa(it->mAddr.sin_addr)) + ":" + toto.str());
	}
	for(it = ps.ipAddrs.mExt.mAddrs.begin(); 
			it != ps.ipAddrs.mExt.mAddrs.end(); it++)
	{
	    std::ostringstream toto;
            toto << ntohs(it->mAddr.sin_port) << "    " << (time(NULL) - it->mSeenTime) << " sec";
            d.ipAddressList.push_back("E:" + std::string(rs_inet_ntoa(it->mAddr.sin_addr)) + ":" + toto.str());
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
		case RS_NET_MODE_UNREACHABLE:
		case RS_NET_MODE_UNKNOWN:
		default:
			d.netMode	= RS_NETMODE_UNREACHABLE;
			break;
	}
	
	
	if (ps.netMode & RS_NET_MODE_TRY_EXT)
	{
		d.tryNetMode	= RS_NETMODE_EXT;
	}
	else if (ps.netMode & RS_NET_MODE_TRY_UPNP)
	{
		d.tryNetMode	= RS_NETMODE_UPNP;
	}
	else
	{
		d.tryNetMode 	= RS_NETMODE_UDP;
	}
	
	d.visState	= 0;
	if (!(ps.visState & RS_VIS_STATE_NODISC))
	{
		d.visState |= RS_VS_DISC_ON;
	}
	
	if (!(ps.visState & RS_VIS_STATE_NODHT))
	{
		d.visState |= RS_VS_DHT_ON;
	}
	
	
	
	
	/* Translate */
	peerConnectState pcs;
	if (!mLinkMgr->getFriendNetStatus(id, pcs)) 
	{
		std::cerr << "p3Peers::getPeerDetails() ERROR No Link Information : " << id << std::endl;
		return true;
	}
	
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPeerDetails() got a SSL id and is returning SSL and GPG details for id : " << id << std::endl;
#endif
	
	
	d.state		= 0;
	if (pcs.state & RS_PEER_S_FRIEND)
		d.state |= RS_PEER_STATE_FRIEND;
	if (pcs.state & RS_PEER_S_ONLINE)
		d.state |= RS_PEER_STATE_ONLINE;
	if (pcs.state & RS_PEER_S_CONNECTED)
		d.state |= RS_PEER_STATE_CONNECTED;
	if (pcs.state & RS_PEER_S_UNREACHABLE)
		d.state |= RS_PEER_STATE_UNREACHABLE;



	/* Finally determine AutoConnect Status */
	d.foundDHT = pcs.dht.found;

	d.connectState = 0;
	d.connectStateString.clear();


	if (pcs.inConnAttempt)
	{
		if (pcs.currentConnAddrAttempt.type & RS_NET_CONN_TUNNEL) {
			d.connectState = RS_PEER_CONNECTSTATE_TRYING_TUNNEL;
		} else if (pcs.currentConnAddrAttempt.type & RS_NET_CONN_TCP_ALL) {
			d.connectState = RS_PEER_CONNECTSTATE_TRYING_TCP;

			std::ostringstream str;
			str << rs_inet_ntoa(pcs.currentConnAddrAttempt.addr.sin_addr) << ":" <<  ntohs(pcs.currentConnAddrAttempt.addr.sin_port);
			d.connectStateString = str.str();
		} else if (pcs.currentConnAddrAttempt.type & RS_NET_CONN_UDP_ALL) {
			d.connectState = RS_PEER_CONNECTSTATE_TRYING_UDP;

			std::ostringstream str;
			str << rs_inet_ntoa(pcs.currentConnAddrAttempt.addr.sin_addr) << ":" <<  ntohs(pcs.currentConnAddrAttempt.addr.sin_port);
			d.connectStateString = str.str();
		}
	}
	else if (pcs.state & RS_PEER_S_CONNECTED)
	{
		if (pcs.connecttype == RS_NET_CONN_TCP_ALL)
		{
			d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_TCP;
		}
		else if (pcs.connecttype == RS_NET_CONN_UDP_ALL)
		{
			d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_UDP;
		}
		else if (pcs.connecttype == RS_NET_CONN_TUNNEL)
		{
			d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_TUNNEL;
		}
		else
		{
			d.connectState = RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN;
		}
	}

	return true;
}
#endif


std::string p3Peers::getGPGName(const std::string &gpg_id)
{
	/* get from mAuthMgr as it should have more peers? */
        return AuthGPG::getAuthGPG()->getGPGName(gpg_id);
}

bool p3Peers::isGPGAccepted(const std::string &gpg_id_is_friend)
{
        /* get from mAuthMgr as it should have more peers? */
        return AuthGPG::getAuthGPG()->isGPGAccepted(gpg_id_is_friend);
}

std::string p3Peers::getPeerName(const std::string &ssl_or_gpg_id)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPeerName() " << ssl_or_gpg_id << std::endl;
#endif
	std::string name;
	if (ssl_or_gpg_id == AuthSSL::getAuthSSL()->OwnId()) 
	{
		return AuthGPG::getAuthGPG()->getGPGOwnName();
	}
	
	if (mPeerMgr->getPeerName(ssl_or_gpg_id, name)) 
	{
#ifdef P3PEERS_DEBUG
		std::cerr << "p3Peers::getPeerName() got a ssl id. Name is : " << name << std::endl;
#endif
		return name;
	}
	
	return AuthGPG::getAuthGPG()->getGPGName(ssl_or_gpg_id);
}


bool	p3Peers::getGPGAllList(std::list<std::string> &ids)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getGPGAllList()" << std::endl;
#endif

        /* get from mAuthMgr */
        AuthGPG::getAuthGPG()->getGPGAllList(ids);
        return true;
}

bool	p3Peers::getGPGValidList(std::list<std::string> &ids)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPGPOthersList()" << std::endl;
#endif

        /* get from mAuthMgr */
        AuthGPG::getAuthGPG()->getGPGValidList(ids);
        return true;
}

bool	p3Peers::getGPGSignedList(std::list<std::string> &ids)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPGPOthersList()" << std::endl;
#endif

        /* get from mAuthMgr */
        AuthGPG::getAuthGPG()->getGPGSignedList(ids);
        return true;
}

bool	p3Peers::getGPGAcceptedList(std::list<std::string> &ids)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getGPGAcceptedList()" << std::endl;
#endif
        AuthGPG::getAuthGPG()->getGPGAcceptedList(ids);
        return true;
}


bool	p3Peers::getAssociatedSSLIds(const std::string &gpg_id, std::list<std::string> &ids)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getAssociatedSSLIds() for id : " << gpg_id << std::endl;
#endif

	return mPeerMgr->getAssociatedPeers(gpg_id, ids);
}


bool	p3Peers::getGPGDetails(const std::string &id, RsPeerDetails &d)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPgpDetails() called for id : " << id << std::endl;
#endif

        /* get from mAuthMgr */
        return AuthGPG::getAuthGPG()->getGPGDetails(id, d);
}

std::string p3Peers::getGPGOwnId()
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPGPOwnId()" << std::endl;
#endif

	/* get from mAuthMgr */
        return AuthGPG::getAuthGPG()->getGPGOwnId();
}

std::string p3Peers::getGPGId(const std::string &sslid_or_gpgid)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getPGPId()" << std::endl;
#endif

        /* get from mAuthMgr */
        if (sslid_or_gpgid == AuthSSL::getAuthSSL()->OwnId()) {
            return AuthGPG::getAuthGPG()->getGPGOwnId();
        }
        peerState pcs;
        if (mPeerMgr->getFriendNetStatus(sslid_or_gpgid, pcs) || mPeerMgr->getOthersNetStatus(sslid_or_gpgid, pcs)) {
            return pcs.gpg_id;
        } else {
            if ( AuthGPG::getAuthGPG()->isGPGId(sslid_or_gpgid)) {
                #ifdef P3PEERS_DEBUG
                std::cerr << "p3Peers::getPGPId() given id is already an gpg id : " << sslid_or_gpgid << std::endl;
                #endif
                return sslid_or_gpgid;
            }
        }
        return "";
}


	/* These Functions are now the only way to authorize a new gpg user...
	 * if we are passed a ssl_id, then use it... otherwise just auth gpg_id
	 */

	/* Add/Remove Friends */
bool 	p3Peers::addFriend(const std::string &ssl_id, const std::string &gpg_id)
{

#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::addFriend() with : id : " << id << "; gpg_id : " << gpg_id << std::endl;
#endif
	if (AuthGPG::getAuthGPG()->isGPGId(gpg_id)) 
	{
#ifdef P3PEERS_DEBUG
        	std::cerr << "p3Peers::addFriend() Authorising GPG Id: " << gpg_id << std::endl;
#endif
		if (AuthGPG::getAuthGPG()->AllowConnection(gpg_id, true))
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

	if (ssl_id == gpg_id || ssl_id == "") 
	{
#ifdef P3PEERS_DEBUG
		std::cerr << "p3Peers::addFriend() WARNING id is NULL or gpgId" << std::endl;
#endif
		return true;
	} 

	/* otherwise - we install as ssl_id..... 
	 * If we are adding an SSL certificate. we flag lastcontact as now. 
	 * This will cause the SSL certificate to be retained for 30 days... and give the person a chance to connect!
	 *  */
	time_t now = time(NULL);
	return mPeerMgr->addFriend(ssl_id, gpg_id, RS_NET_MODE_UDP, RS_VIS_STATE_STD, now);
}






bool 	p3Peers::removeFriendLocation(const std::string &sslId)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::removeFriendLocation() " << sslId << std::endl;
#endif
		//will remove if it's a ssl id
        mPeerMgr->removeFriend(sslId);
        return true;

}

bool 	p3Peers::removeFriend(const std::string &gpgId)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::removeFriend() " << gpgId << std::endl;
#endif
	if (gpgId == AuthGPG::getAuthGPG()->getGPGOwnId()) {
        std::cerr << "p3Peers::removeFriend() ERROR  we're not going to remove our own GPG id."  << std::endl;
		return false;
	}

	if (AuthGPG::getAuthGPG()->isGPGId(gpgId)) 
	{
#ifdef P3PEERS_DEBUG
        	std::cerr << "p3Peers::removeFriend() Removing GPG Id: " << gpgId << std::endl;
#endif
		if (AuthGPG::getAuthGPG()->AllowConnection(gpgId, false))
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
	else
	{
#ifdef P3PEERS_DEBUG
        	std::cerr << "p3Peers::removeFriend() Not GPG Id: " << gpg_id << std::endl;
#endif
		return removeFriendLocation(gpgId);
	}

	return false;
}



	/* Network Stuff */
bool 	p3Peers::connectAttempt(const std::string &id)
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
void p3Peers::allowServerIPDetermination(bool b) 
{
	mNetMgr->setIPServersEnabled(b) ;
}

void p3Peers::allowTunnelConnection(bool b)
{
        #ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::allowTunnelConnection() set tunnel to : " << b << std::endl;
        #endif
        mLinkMgr->setTunnelConnection(b) ;
}

bool p3Peers::getAllowServerIPDetermination()
{
	return mNetMgr->getIPServersEnabled() ;
}

bool p3Peers::getAllowTunnelConnection()
{
        #ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::getAllowTunnelConnection() tunnel is : " << mConnMgr->getTunnelConnection()  << std::endl;
        #endif
        return mLinkMgr->getTunnelConnection() ;
}

bool 	p3Peers::setLocalAddress(const std::string &id, const std::string &addr_str, uint16_t port)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setLocalAddress() " << id << std::endl;
#endif

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	int ret = 1;
/********************************** WINDOWS/UNIX SPECIFIC PART *******************/
#ifndef WINDOWS_SYS
	if (ret && (0 != inet_aton(addr_str.c_str(), &(addr.sin_addr))))
#else
	addr.sin_addr.s_addr = inet_addr(addr_str.c_str());
	if (ret)
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART *******************/
	{
		return mPeerMgr->setLocalAddress(id, addr);
	}
	return false;
}

bool 	p3Peers::setLocation(const std::string &ssl_id, const std::string &location)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setLocation() " << ssl_id << std::endl;
#endif

        return mPeerMgr->setLocation(ssl_id, location);
}
bool 	p3Peers::setExtAddress(const std::string &id, const std::string &addr_str, uint16_t port)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setExtAddress() " << id << std::endl;
#endif

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	int ret = 1;
/********************************** WINDOWS/UNIX SPECIFIC PART *******************/
#ifndef WINDOWS_SYS
	if (ret && (0 != inet_aton(addr_str.c_str(), &(addr.sin_addr))))
#else
	addr.sin_addr.s_addr = inet_addr(addr_str.c_str());
	if (ret)
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART *******************/
	{
		return mPeerMgr->setExtAddress(id, addr);
	}
	return false;
}

bool p3Peers::setDynDNS(const std::string &id, const std::string &dyndns)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setDynDNS() called with id: " << id << " dyndns: " << dyndns <<std::endl;
#endif
    return mPeerMgr->setDynDNS(id, dyndns);
}

bool 	p3Peers::setNetworkMode(const std::string &id, uint32_t extNetMode)
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
		case RS_NETMODE_UNREACHABLE:
			netMode = RS_NET_MODE_UNREACHABLE;
			break;
		default:
			break;
	}

	return mPeerMgr->setNetworkMode(id, netMode);
}


bool
p3Peers::setVisState(const std::string &id, uint32_t extVisState)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::setVisState() " << id << std::endl;
#endif
        std::cerr << "p3Peers::setVisState() " << id << " " << extVisState << std::endl;

	uint32_t visState = 0;
	if (!(extVisState & RS_VS_DHT_ON))
		visState |= RS_VIS_STATE_NODHT;
	if (!(extVisState & RS_VS_DISC_ON))
		visState |= RS_VIS_STATE_NODISC;

	return mPeerMgr->setVisState(id, visState);
}

//===========================================================================
	/* Auth Stuff */
std::string
p3Peers::GetRetroshareInvite(bool include_signatures)
{
	return GetRetroshareInvite(getOwnId(),include_signatures);
}

std::string
p3Peers::GetRetroshareInvite(const std::string& ssl_id,bool include_signatures)
{
        #ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::GetRetroshareInvite()" << std::endl;
        #endif

        //add the sslid, location, ip local and external address after the signature
        RsPeerDetails Detail;
		  std::string invite ;

        if (getPeerDetails(ssl_id, Detail)) 
		  {
			  invite = AuthGPG::getAuthGPG()->SaveCertificateToString(Detail.gpg_id,include_signatures);

			  if(!Detail.isOnlyGPGdetail)
			  {
				  invite += CERT_SSL_ID + Detail.id + ";";
				  invite += CERT_LOCATION + Detail.location + ";\n";
				  invite += CERT_LOCAL_IP + Detail.localAddr + ":";
				  std::ostringstream out;
				  out << Detail.localPort;
				  invite += out.str() + ";";
				  invite += CERT_EXT_IP + Detail.extAddr + ":";
				  std::ostringstream out2;
				  out2 << Detail.extPort;
				  invite += out2.str() + ";";
				  if (!Detail.dyndns.empty()) {
					  invite += "\n" + CERT_DYNDNS + Detail.dyndns + ";";
				  }
			  }
		  }

        #ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::GetRetroshareInvite() returns : \n" << invite << std::endl;
        #endif
        return invite;
}

//===========================================================================

bool 	p3Peers::loadCertificateFromFile(const std::string &/*fname*/, std::string &/*id*/, std::string &/*gpg_id*/)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::LoadCertificateFromFile() not implemented yet";
	std::cerr << std::endl;
#endif

        return false;
}


//bool splitCerts(std::string in, std::string &sslcert, std::string &pgpcert)
//{
//	std::cerr << "splitCerts():" << in;
//	std::cerr << std::endl;
//
//	/* search for -----END CERTIFICATE----- */
//	std::string sslend("-----END CERTIFICATE-----");
//	std::string pgpend("-----END PGP PUBLIC KEY BLOCK-----");
//	size_t pos = in.find(sslend);
//	size_t pos2 = in.find(pgpend);
//	size_t ssllen, pgplen;
//
//	if (pos != std::string::npos)
//	{
//		std::cerr << "splitCerts(): Found SSL Cert";
//		std::cerr << std::endl;
//
//		ssllen = pos + sslend.length();
//		sslcert = in.substr(0, ssllen);
//
//		if (pos2 != std::string::npos)
//		{
//			std::cerr << "splitCerts(): Found SSL + PGP Cert";
//			std::cerr << std::endl;
//
//			pgplen = pos2 + pgpend.length() - ssllen;
//			pgpcert = in.substr(ssllen, pgplen);
//		}
//		return true;
//	}
//	else if (pos2 != std::string::npos)
//	{
//		std::cerr << "splitCerts(): Found PGP Cert Only";
//		std::cerr << std::endl;
//
//		pgplen = pos2 + pgpend.length();
//		pgpcert = in.substr(0, pgplen);
//		return true;
//	}
//	return false;
//}

static bool splitCert(const std::string &certstr, std::string &cert, std::string &peerInfo)
{
	cert.erase();
	peerInfo.erase();

	/* search for -----END CERTIFICATE----- */
	std::string pgpend("-----END PGP PUBLIC KEY BLOCK-----");

	size_t pos = certstr.find(pgpend);

	if (pos != std::string::npos) {
		pos += pgpend.length();
		cert = certstr.substr(0, pos);
		if (pos + 1 < certstr.length())
			peerInfo = certstr.substr(pos + 1);
	}

	return !cert.empty();
}

bool 	p3Peers::loadDetailsFromStringCert(const std::string &certstr, RsPeerDetails &pd,std::string& error_string)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::LoadCertificateFromString() ";
	std::cerr << std::endl;
#endif

        //parse the text to get ip address
        try {

            std::string cert;
            std::string peerInfo;

            if (splitCert(certstr, cert, peerInfo)) {
                    std::string gpg_id;
                    AuthGPG::getAuthGPG()->LoadCertificateFromString(cert, gpg_id,error_string);
                    AuthGPG::getAuthGPG()->getGPGDetails(gpg_id, pd);
                    if (gpg_id.empty()) {
                        return false;
                    }
            } else {
                    return false;
            }

            #ifdef P3PEERS_DEBUG
            std::cerr << "Parsing cert for sslid, location, ext and local address details. : " << certstr << std::endl;
            #endif

            //let's parse the ssl id
            size_t parsePosition = peerInfo.find(CERT_SSL_ID);
            std::cerr << "sslid position : " << parsePosition << std::endl;
            if (parsePosition != std::string::npos) {
                parsePosition += CERT_SSL_ID.length();
                std::string subCert = peerInfo.substr(parsePosition);
                parsePosition = subCert.find(";");
                if (parsePosition != std::string::npos) {
                    std::string ssl_id = subCert.substr(0, parsePosition);
                    std::cerr << "SSL id : " << ssl_id << std::endl;
                    pd.id = ssl_id;
                    pd.isOnlyGPGdetail = false;
                }
            }

            //let's parse the location
            parsePosition = peerInfo.find(CERT_LOCATION);
            std::cerr << "location position : " << parsePosition << std::endl;
            if (parsePosition != std::string::npos) {
                parsePosition += CERT_LOCATION.length();
                std::string subCert = peerInfo.substr(parsePosition);
                parsePosition = subCert.find(";");
                if (parsePosition != std::string::npos) {
                    std::string location = subCert.substr(0, parsePosition);
                    std::cerr << "location : " << location << std::endl;
                    pd.location = location;
                }
            }

            //let's parse ip local address
            parsePosition = peerInfo.find(CERT_LOCAL_IP);
            std::cerr << "local ip position : " << parsePosition << std::endl;
            if (parsePosition != std::string::npos) {
                parsePosition += CERT_LOCAL_IP.length();
                std::string subCert = peerInfo.substr(parsePosition);
                parsePosition = subCert.find(":");
                if (parsePosition != std::string::npos) {
                    std::string local_ip = subCert.substr(0, parsePosition);
                    std::cerr << "Local Ip : " << local_ip << std::endl;
                    pd.localAddr = local_ip;

                    //let's parse local port
                    subCert = subCert.substr(parsePosition + 1);
                    parsePosition = subCert.find(";");
                    if (parsePosition != std::string::npos) {
                        std::string local_port = subCert.substr(0, parsePosition);
                        std::cerr << "Local port : " << local_port << std::endl;
                        std::istringstream instream(local_port);
                        uint16_t local_port_int;
                        instream >> local_port_int;
                        pd.localPort = (local_port_int);
                    }
                }
            }

            //let's parse ip ext address
            parsePosition = peerInfo.find(CERT_EXT_IP);
            std::cerr << "Ext ip position : " << parsePosition << std::endl;
            if (parsePosition != std::string::npos) {
                parsePosition = parsePosition + CERT_EXT_IP.length();
                std::string subCert = peerInfo.substr(parsePosition);
                parsePosition = subCert.find(":");
                if (parsePosition != std::string::npos) {
                    std::string ext_ip = subCert.substr(0, parsePosition);
                    std::cerr << "Ext Ip : " << ext_ip << std::endl;
                    pd.extAddr = ext_ip;

                    //let's parse ext port
                    subCert = subCert.substr(parsePosition + 1);
                    parsePosition = subCert.find(";");
                    if (parsePosition != std::string::npos) {
                        std::string ext_port = subCert.substr(0, parsePosition);
                        std::cerr << "Ext port : " << ext_port << std::endl;
                        std::istringstream instream(ext_port);
                        uint16_t ext_port_int;
                        instream >> ext_port_int;
                        pd.extPort = (ext_port_int);
                    }
                }
            }

            //let's parse DynDNS
            parsePosition = peerInfo.find(CERT_DYNDNS);
            std::cerr << "location DynDNS : " << parsePosition << std::endl;
            if (parsePosition != std::string::npos) {
                parsePosition += CERT_DYNDNS.length();
                std::string subCert = peerInfo.substr(parsePosition);
                parsePosition = subCert.find(";");
                if (parsePosition != std::string::npos) {
                    std::string DynDNS = subCert.substr(0, parsePosition);
                    std::cerr << "DynDNS : " << DynDNS << std::endl;
                    pd.dyndns = DynDNS;
                }
            }

        } catch (...) {
            std::cerr << "ConnectFriendWizard : Parse ip address error." << std::endl;
        }

        if (pd.gpg_id == "") {
            return false;
        } else {
            return true;
        }
}

bool p3Peers::cleanCertificate(const std::string &certstr, std::string &cleanCert,int& error_code)
{
	std::string cert;
	std::string peerInfo;

	if (splitCert(certstr, cert, peerInfo)) {
		cleanCert = cleanUpCertificate(cert,error_code);
		if (!cleanCert.empty()) {
			if (!peerInfo.empty()) {
				if (*cleanCert.rbegin() != '\n') {
					cleanCert += "\n";
				}
				cleanCert += peerInfo;
			}
			return true;
		}
	}

	return false;
}

bool 	p3Peers::saveCertificateToFile(const std::string &id, const std::string &/*fname*/)
{
	/* remove unused parameter warnings */
	(void) id;

#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::SaveCertificateToFile() not implemented yet " << id;
	std::cerr << std::endl;
#endif

//	ensureExtension(fname, "pqi");
//
//        return AuthSSL::getAuthSSL()->SaveCertificateToFile(id, fname);
        return false;
}

std::string p3Peers::saveCertificateToString(const std::string &id)
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

bool 	p3Peers::signGPGCertificate(const std::string &id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::SignCertificate() " << id;
	std::cerr << std::endl;
#endif


        AuthGPG::getAuthGPG()->AllowConnection(id, true);
        return AuthGPG::getAuthGPG()->SignCertificateLevel0(id);
}


bool 	p3Peers::trustGPGCertificate(const std::string &id, uint32_t trustlvl)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::TrustCertificate() " << id;
	std::cerr << std::endl;
#endif
        //check if we've got a ssl or gpg id
        std::string gpgId = getGPGId(id);
        if (gpgId.empty()) {
            //if no result then it must be a gpg id
            return AuthGPG::getAuthGPG()->TrustCertificate(id, trustlvl);
        } else {
            return AuthGPG::getAuthGPG()->TrustCertificate(gpgId, trustlvl);
        }
}


int ensureExtension(std::string &name, std::string def_ext)
{
	/* if it has an extension, don't change */
	int len = name.length();
	int extpos = name.find_last_of('.');
	
	std::ostringstream out;
	out << "ensureExtension() name: " << name << std::endl;
	out << "\t\t extpos: " << extpos;
	out << " len: " << len << std::endl;
	
	/* check that the '.' has between 1 and 4 char after it (an extension) */
	if ((extpos > 0) && (extpos < len - 1) && (extpos + 6 > len))
	{
		/* extension there */
		std::string curext = name.substr(extpos, len);
		out << "ensureExtension() curext: " << curext << std::endl;
		std::cerr << out.str();
		return 0;
	}
	
	if (extpos != len - 1)
	{
		name += ".";
	}
	name += def_ext;
	
	out << "ensureExtension() added ext: " << name << std::endl;
	
	std::cerr << out.str();
	return 1;
}

	/* Group Stuff */
bool p3Peers::addGroup(RsGroupInfo &groupInfo)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::addGroup()" << std::endl;
#endif

	return mPeerMgr->addGroup(groupInfo);
}

bool p3Peers::editGroup(const std::string &groupId, RsGroupInfo &groupInfo)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::editGroup()" << std::endl;
#endif

	return mPeerMgr->editGroup(groupId, groupInfo);
}

bool p3Peers::removeGroup(const std::string &groupId)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::removeGroup()" << std::endl;
#endif

	return mPeerMgr->removeGroup(groupId);
}

bool p3Peers::getGroupInfo(const std::string &groupId, RsGroupInfo &groupInfo)
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

bool p3Peers::assignPeerToGroup(const std::string &groupId, const std::string &peerId, bool assign)
{
	std::list<std::string> peerIds;
	peerIds.push_back(peerId);

	return assignPeersToGroup(groupId, peerIds, assign);
}

bool p3Peers::assignPeersToGroup(const std::string &groupId, const std::list<std::string> &peerIds, bool assign)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::assignPeersToGroup()" << std::endl;
#endif

	return mPeerMgr->assignPeersToGroup(groupId, peerIds, assign);
}


RsPeerDetails::RsPeerDetails()
        :isOnlyGPGdetail(false),
	 id(""),gpg_id(""),
	 name(""),email(""),location(""),
	org(""),issuer(""),fpr(""),authcode(""),
		  trustLvl(0), validLvl(0),ownsign(false), 
	hasSignedMe(false),accept_connection(false),
	state(0),localAddr(""),localPort(0),extAddr(""),extPort(0),netMode(0),visState(0),
	lastConnect(0),connectState(0),connectStateString(""),connectPeriod(0)
{
}

std::ostream &operator<<(std::ostream &out, const RsPeerDetails &detail)
{
	out << "RsPeerDetail: " << detail.name << "  <" << detail.id << ">";
	out << std::endl;

	out << " email:   " << detail.email;
	out << " location:" << detail.location;
	out << " org:     " << detail.org;
	out << std::endl;

	out << " fpr:     " << detail.fpr;
	out << " authcode:" << detail.authcode;
	out << std::endl;

	out << " signers:";
	out << std::endl;

	std::list<std::string>::const_iterator it;
        for(it = detail.gpgSigners.begin();
                it != detail.gpgSigners.end(); it++)
	{
		out << "\t" << *it;
		out << std::endl;
	}
	out << std::endl;

	out << " trustLvl:    " << detail.trustLvl;
        out << " ownSign:     " << detail.ownsign;
	out << std::endl;

	out << " state:       " << detail.state;
	out << " netMode:     " << detail.netMode;
	out << std::endl;

	out << " localAddr:   " << detail.localAddr;
	out << ":" << detail.localPort;
	out << std::endl;
	out << " extAddr:   " << detail.extAddr;
	out << ":" << detail.extPort;
	out << std::endl;


	out << " lastConnect:       " << detail.lastConnect;
	out << " connectPeriod:     " << detail.connectPeriod;
	out << std::endl;

	return out;
}

RsGroupInfo::RsGroupInfo()
{
	flag = 0;
}
