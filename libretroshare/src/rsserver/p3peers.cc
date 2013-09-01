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

#include "util/radix64.h"
#include "pgp/pgpkeyutil.h"

#include "rsserver/p3peers.h"
#include "rsserver/p3face.h"

#include "pqi/p3linkmgr.h"
#include "pqi/p3peermgr.h"
#include "pqi/p3netmgr.h"

#include "pqi/authssl.h"
#include "pqi/authgpg.h"
#include "retroshare/rsinit.h"
#include "retroshare/rsfiles.h"

#include "pgp/rscertificate.h"

#include <iostream>
#include <fstream>

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

bool p3Peers::haveSecretKey(const std::string& id)
{
	return AuthGPG::getAuthGPG()->haveSecretKey(id) ;
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
			bool res = getGPGDetails(id, d);

			d.isOnlyGPGdetail = true;

			if(id.length() == 16)
				d.service_perm_flags = mPeerMgr->servicePermissionFlags(id) ;
			else if(id.length() == 32)
				d.service_perm_flags = mPeerMgr->servicePermissionFlags_sslid(id) ;
			else
			{
				std::cerr << "p3Peers::getPeerDetails() ERROR not an correct Id: " << id << std::endl;
				d.service_perm_flags = RS_SERVICE_PERM_NONE ;
			}

			return res ;
		}
	}

	/* get from gpg (first), to fill in the sign and trust details */
	/* don't retrun now, we've got fill in the ssl and connection info */
	getGPGDetails(ps.gpg_id, d);
	d.isOnlyGPGdetail = false;

	//get the ssl details
	d.id 		= id;
	d.location 	= ps.location;

	d.service_perm_flags = mPeerMgr->servicePermissionFlags(ps.gpg_id) ;

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
		std::string toto;
		rs_sprintf(toto, "%u    %ld sec", ntohs(it->mAddr.sin_port), time(NULL) - it->mSeenTime);
		d.ipAddressList.push_back("L:" + rs_inet_ntoa(it->mAddr.sin_addr) + ":" + toto);
	}
	for(it = ps.ipAddrs.mExt.mAddrs.begin(); 
			it != ps.ipAddrs.mExt.mAddrs.end(); it++)
	{
		std::string toto;
		rs_sprintf(toto, "%u    %ld sec", ntohs(it->mAddr.sin_port), time(NULL) - it->mSeenTime);
		d.ipAddressList.push_back("E:" + rs_inet_ntoa(it->mAddr.sin_addr) + ":" + toto);
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
		if(id != sOwnId)
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

	d.connectState = RS_PEER_CONNECTSTATE_OFFLINE;
	d.connectStateString.clear();


	if (pcs.inConnAttempt)
	{
		if (pcs.currentConnAddrAttempt.type & RS_NET_CONN_TUNNEL) {
			d.connectState = RS_PEER_CONNECTSTATE_TRYING_TUNNEL;
		} else if (pcs.currentConnAddrAttempt.type & RS_NET_CONN_TCP_ALL) {
			d.connectState = RS_PEER_CONNECTSTATE_TRYING_TCP;
			rs_sprintf(d.connectStateString, "%s:%u", rs_inet_ntoa(pcs.currentConnAddrAttempt.addr.sin_addr).c_str(), ntohs(pcs.currentConnAddrAttempt.addr.sin_port));
		} else if (pcs.currentConnAddrAttempt.type & RS_NET_CONN_UDP_ALL) {
			d.connectState = RS_PEER_CONNECTSTATE_TRYING_UDP;
			rs_sprintf(d.connectStateString, "%s:%u", rs_inet_ntoa(pcs.currentConnAddrAttempt.addr.sin_addr).c_str(), ntohs(pcs.currentConnAddrAttempt.addr.sin_port));
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

	d.wasDeniedConnection = pcs.wasDeniedConnection;
	d.deniedTS = pcs.deniedTS;

	return true;
}

bool p3Peers::isKeySupported(const std::string& id)
{
	return AuthGPG::getAuthGPG()->isKeySupported(id);
}

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

bool    p3Peers::gpgSignData(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen)
{
	return AuthGPG::getAuthGPG()->SignDataBin(data,len,sign,signlen);
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
bool 	p3Peers::addFriend(const std::string &ssl_id, const std::string &gpg_id,ServicePermissionFlags perm_flags)
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
	return mPeerMgr->addFriend(ssl_id, gpg_id, RS_NET_MODE_UDP, RS_VIS_STATE_STD, now, perm_flags);
}

bool 	p3Peers::removeKeysFromPGPKeyring(const std::list<std::string>& pgp_ids,std::string& backup_file,uint32_t& error_code)
{
	return AuthGPG::getAuthGPG()->removeKeysFromPGPKeyring(pgp_ids,backup_file,error_code) ;
}

bool 	p3Peers::removeFriendLocation(const std::string &sslId)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::removeFriendLocation() " << sslId << std::endl;
#endif
		//will remove if it's a ssl id
        mPeerMgr->removeFriend(sslId, false);
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
p3Peers::GetRetroshareInvite(bool include_signatures,bool old_format)
{
	return GetRetroshareInvite(getOwnId(),include_signatures,old_format);
}

bool p3Peers::GetPGPBase64StringAndCheckSum(	const std::string& gpg_id,
															std::string& gpg_base64_string,
															std::string& gpg_base64_checksum) 
{
	gpg_base64_string = "" ;
	gpg_base64_checksum = "" ;

	unsigned char *mem_block ;
	size_t mem_block_size ;

	if(!AuthGPG::getAuthGPG()->exportPublicKey(PGPIdType(gpg_id),mem_block,mem_block_size,false,false))
		return false ;

	Radix64::encode((const char *)mem_block,mem_block_size,gpg_base64_string) ;

	uint32_t crc = PGPKeyManagement::compute24bitsCRC((unsigned char *)mem_block,mem_block_size) ;

	unsigned char tmp[3] = { (crc >> 16) & 0xff, (crc >> 8) & 0xff, crc & 0xff } ;
	Radix64::encode((const char *)tmp,3,gpg_base64_checksum) ;

	delete[] mem_block ;

	return true ;
}

std::string p3Peers::GetRetroshareInvite(const std::string& ssl_id,bool include_signatures,bool old_format)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::GetRetroshareInvite()" << std::endl;
#endif

	//add the sslid, location, ip local and external address after the signature
	RsPeerDetails Detail;
	std::string invite ;

	if (getPeerDetails(ssl_id, Detail)) 
	{
		unsigned char *mem_block = NULL;
		size_t mem_block_size = 0;

		if(!AuthGPG::getAuthGPG()->exportPublicKey(PGPIdType(Detail.gpg_id),mem_block,mem_block_size,false,include_signatures))
		{
			std::cerr << "Cannot output certificate for id \"" << Detail.gpg_id << "\". Sorry." << std::endl;
			return "" ;
		}

		RsCertificate cert( Detail,mem_block,mem_block_size ) ;

		if(old_format)
			return cert.toStdString_oldFormat() ;
		else
			return cert.toStdString() ;

	}

#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::GetRetroshareInvite() returns : \n" << invite << std::endl;
#endif
	return invite;
}

//===========================================================================

bool 	p3Peers::loadCertificateFromString(const std::string& cert, std::string& ssl_id, std::string& gpg_id, std::string& error_string)
{
	RsCertificate crt(cert) ;
	PGPIdType gpgid ;

	bool res = AuthGPG::getAuthGPG()->LoadCertificateFromString(crt.armouredPGPKey(),gpgid,error_string) ;

	gpg_id = gpgid.toStdString() ;
	ssl_id = crt.sslid_string() ;

	return res ;
}

bool 	p3Peers::loadDetailsFromStringCert(const std::string &certstr, RsPeerDetails &pd,uint32_t& error_code)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::LoadCertificateFromString() ";
	std::cerr << std::endl;
#endif
	//parse the text to get ip address
	try 
	{
		RsCertificate cert(certstr) ;

		if(!AuthGPG::getAuthGPG()->getGPGDetailsFromBinaryBlock(cert.pgp_key(),cert.pgp_key_size(), pd.gpg_id,pd.name,pd.gpgSigners))
			return false;

#ifdef P3PEERS_DEBUG
		std::cerr << "Parsing cert for sslid, location, ext and local address details. : " << certstr << std::endl;
#endif

		pd.id = cert.sslid_string() ;
		pd.location = cert.location_name_string();
		pd.localAddr = cert.loc_ip_string();
		pd.localPort = cert.loc_port_us();
		pd.extAddr = cert.ext_ip_string();
		pd.extPort = cert.ext_port_us();
		pd.dyndns = cert.dns_string() ;
		pd.isOnlyGPGdetail = pd.id.empty();
		pd.service_perm_flags = RS_SERVICE_PERM_ALL ;
	} 
	catch(uint32_t e) 
	{
		std::cerr << "ConnectFriendWizard : Parse ip address error :" << e << std::endl;
		error_code = e;
		return false ;
	}

	if (pd.gpg_id == "") 
		return false;
	else 
		return true;
}

bool p3Peers::cleanCertificate(const std::string &certstr, std::string &cleanCert,int& error_code)
{
	RsCertificate::Format format ;

	return RsCertificate::cleanCertificate(certstr,cleanCert,format,error_code) ;
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


//int ensureExtension(std::string &name, std::string def_ext)
//{
//	/* if it has an extension, don't change */
//	int len = name.length();
//	int extpos = name.find_last_of('.');
	
//	std::string out;
//	rs_sprintf_append(out, "ensureExtension() name: %s\n\t\t extpos: %d len: \n", name.c_str(), extpos, len);
	
//	/* check that the '.' has between 1 and 4 char after it (an extension) */
//	if ((extpos > 0) && (extpos < len - 1) && (extpos + 6 > len))
//	{
//		/* extension there */
//		std::string curext = name.substr(extpos, len);
//		out += "ensureExtension() curext: " + curext;
//		std::cerr << out << std::endl;
//		return 0;
//	}
	
//	if (extpos != len - 1)
//	{
//		name += ".";
//	}
//	name += def_ext;
	
//	out += "ensureExtension() added ext: " + name;
	
//	std::cerr << out << std::endl;
//	return 1;
//}

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

bool p3Peers::editGroup(const std::string &groupId, RsGroupInfo &groupInfo)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::editGroup()" << std::endl;
#endif

	bool res = mPeerMgr->editGroup(groupId, groupInfo);
	rsFiles->updateSinceGroupPermissionsChanged() ;

	return res ;
}

bool p3Peers::removeGroup(const std::string &groupId)
{
#ifdef P3PEERS_DEBUG
        std::cerr << "p3Peers::removeGroup()" << std::endl;
#endif

		  bool res = mPeerMgr->removeGroup(groupId);
		  rsFiles->updateSinceGroupPermissionsChanged() ;
		  return res ;
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

	bool res = mPeerMgr->assignPeersToGroup(groupId, peerIds, assign);
	rsFiles->updateSinceGroupPermissionsChanged() ;

	return res ;
}

FileSearchFlags p3Peers::computePeerPermissionFlags(const std::string& peer_ssl_id,
																		FileStorageFlags share_flags,
																		const std::list<std::string>& directory_parent_groups)
{
	// We should be able to do that in O(1), using groups based on packs of bits.
	//
	// But for now, because the implementation of groups is not totally decided yet, we revert to this
	// very simple algorithm.
	//

	bool found = false ;
	std::string pgp_id = getGPGId(peer_ssl_id) ;

	for(std::list<std::string>::const_iterator it(directory_parent_groups.begin());it!=directory_parent_groups.end() && !found;++it)
	{
		RsGroupInfo info ;
		if(!getGroupInfo(*it,info))
		{
			std::cerr << "(EE) p3Peers::computePeerPermissionFlags: no group named " << *it << ": cannot get info." << std::endl;
			continue ;
		}

		for(std::list<std::string>::const_iterator it2(info.peerIds.begin());it2!=info.peerIds.end() && !found;++it2)
			if(*it2 == pgp_id)
				found = true ;
	}

	bool network_wide = (share_flags & DIR_FLAGS_NETWORK_WIDE_OTHERS) ;//|| ( (share_flags & DIR_FLAGS_NETWORK_WIDE_GROUPS) && found) ;
	bool browsable    = (share_flags &    DIR_FLAGS_BROWSABLE_OTHERS) || ( (share_flags &    DIR_FLAGS_BROWSABLE_GROUPS) && found) ;

	FileSearchFlags final_flags ;

	if(network_wide) final_flags |= RS_FILE_HINTS_NETWORK_WIDE ;
	if(browsable   ) final_flags |= RS_FILE_HINTS_BROWSABLE ;

	return final_flags ;
}

RsPeerDetails::RsPeerDetails()
        :isOnlyGPGdetail(false),
	 id(""),gpg_id(""),
	 name(""),email(""),location(""),
	org(""),issuer(""),fpr(""),authcode(""),
		  trustLvl(0), validLvl(0),ownsign(false), 
	hasSignedMe(false),accept_connection(false),
	state(0),localAddr(""),localPort(0),extAddr(""),extPort(0),netMode(0),visState(0),
	lastConnect(0),connectState(0),connectStateString(""),connectPeriod(0),foundDHT(false), 
	wasDeniedConnection(false), deniedTS(0)
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

ServicePermissionFlags p3Peers::servicePermissionFlags_sslid(const std::string& ssl_id) 
{
	return mPeerMgr->servicePermissionFlags_sslid(ssl_id) ;
}
ServicePermissionFlags p3Peers::servicePermissionFlags(const std::string& gpg_id) 
{
	return mPeerMgr->servicePermissionFlags(gpg_id) ;
}
void p3Peers::setServicePermissionFlags(const std::string& gpg_id,const ServicePermissionFlags& flags) 
{
	mPeerMgr->setServicePermissionFlags(gpg_id,flags) ;
}


