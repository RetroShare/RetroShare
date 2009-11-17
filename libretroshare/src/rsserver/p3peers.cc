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
#include "pqi/p3connmgr.h"
#include "pqi/p3authmgr.h"
#include <rsiface/rsinit.h>

#include <iostream>
#include <fstream>
#include <sstream>

#ifdef RS_USE_PGPSSL
	#include <gpgme.h>
#endif

/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
        #include "pqi/authxpgp.h"
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
	#include "pqi/authssl.h"
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/


RsPeers *rsPeers = NULL;

/*******
 * #define P3PEERS_DEBUG 1
 *******/

static uint32_t RsPeerTranslateTrust(uint32_t trustLvl);
int ensureExtension(std::string &name, std::string def_ext);

std::string RsPeerTrustString(uint32_t trustLvl)
{

	std::string str;

#ifdef RS_USE_PGPSSL
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
#endif

	if (trustLvl == RS_TRUST_LVL_GOOD)
	{
		str = "Good";
	}
	else if (trustLvl == RS_TRUST_LVL_MARGINAL)
	{
		str = "Marginal";
	}
	else 
	{
		str = "No Trust";
	}
	return str;
}


		
std::string RsPeerStateString(uint32_t state)
{
	std::string str;
	if (state & RS_PEER_STATE_CONNECTED)
	{
		str = "Connected";
	}
	else if (state & RS_PEER_STATE_UNREACHABLE)
	{
		str = "Unreachable";
	}
	else if (state & RS_PEER_STATE_ONLINE)
	{
		str = "Available";
	}
	else if (state & RS_PEER_STATE_FRIEND)
	{
		str = "Offline";
	}
	else 
	{
		str = "Neighbour";
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


p3Peers::p3Peers(p3ConnectMgr *cm, p3AuthMgr *am)
	:mConnMgr(cm), mAuthMgr(am)
{
	return;
}

	/* Updates ... */
bool p3Peers::FriendsChanged()
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::FriendsChanged()";
	std::cerr << std::endl;
#endif

	/* TODO */
	return false;
}

bool p3Peers::OthersChanged()
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::OthersChanged()";
	std::cerr << std::endl;
#endif

	/* TODO */
	return false;
}

	/* Peer Details (Net & Auth) */
std::string p3Peers::getOwnId()
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getOwnId()";
	std::cerr << std::endl;
#endif

	return mAuthMgr->OwnId();
}

bool	p3Peers::getOnlineList(std::list<std::string> &ids)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getOnlineList()";
	std::cerr << std::endl;
#endif

	/* get from mConnectMgr */
	mConnMgr->getOnlineList(ids);
	return true;
}

bool	p3Peers::getFriendList(std::list<std::string> &ids)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getFriendList()";
	std::cerr << std::endl;
#endif

	/* get from mConnectMgr */
	mConnMgr->getFriendList(ids);
	return true;
}

bool	p3Peers::getOthersList(std::list<std::string> &ids)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getOthersList()";
	std::cerr << std::endl;
#endif

	/* get from mAuthMgr */
	mAuthMgr->getAllList(ids);
	return true;
}

bool    p3Peers::isOnline(std::string id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::isOnline() " << id;
	std::cerr << std::endl;
#endif

	/* get from mConnectMgr */
	peerConnectState state;
	if (mConnMgr->getFriendNetStatus(id, state) &&
			(state.state & RS_PEER_S_CONNECTED))
	{
		return true;
	}
	return false;
}

bool    p3Peers::isTrustingMe(std::string id) const
{
	return mAuthMgr->isTrustingMe(id) ;
}

bool    p3Peers::isFriend(std::string id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::isFriend() " << id;
	std::cerr << std::endl;
#endif

	/* get from mConnectMgr */
	peerConnectState state;
	if (mConnMgr->getFriendNetStatus(id, state) &&
			(state.state & RS_PEER_S_FRIEND))
	{
		return true;
	}
	return false;
}

static struct sockaddr_in getPreferredAddress(	const struct sockaddr_in& addr1,time_t ts1,
																const struct sockaddr_in& addr2,time_t ts2,
																const struct sockaddr_in& addr3,time_t ts3)
{
	time_t ts = ts1 ;
	struct sockaddr_in addr = addr1 ;

	if(ts2 > ts && strcmp(inet_ntoa(addr2.sin_addr),"0.0.0.0")) { ts = ts2 ; addr = addr2 ; }
	if(ts3 > ts && strcmp(inet_ntoa(addr3.sin_addr),"0.0.0.0")) { ts = ts3 ; addr = addr3 ; }

	return addr ;
}

bool	p3Peers::getPeerDetails(std::string id, RsPeerDetails &d)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPeerDetails() " << id;
	std::cerr << std::endl;
#endif

	/* get from mAuthMgr (first) */
	pqiAuthDetails authDetail;
	if (!mAuthMgr->getDetails(id, authDetail))
	{
		return false;
	}

	d.fpr		= authDetail.fpr;
	d.id 		= authDetail.id;
	d.name 		= authDetail.name;
	d.email 	= authDetail.email;
	d.location 	= authDetail.location;
	d.org 		= authDetail.org;
	d.signers 	= authDetail.signers;

	d.issuer 	= authDetail.issuer;

	d.ownsign 	= authDetail.ownsign;
	d.trusted 	= authDetail.trusted;

#ifdef RS_USE_PGPSSL
	d.trustLvl 	= authDetail.trustLvl;
	d.validLvl 	= authDetail.validLvl;
#else
	d.trustLvl 	= RsPeerTranslateTrust(authDetail.trustLvl);
	d.validLvl 	= RsPeerTranslateTrust(authDetail.trustLvl);
#endif

	/* generate */
	d.authcode  	= "AUTHCODE";

	/* get from mConnectMgr */
	peerConnectState pcs;

	if (id == mAuthMgr->OwnId())
	{
		mConnMgr->getOwnNetStatus(pcs);
	}
	else if (!mConnMgr->getFriendNetStatus(id, pcs))
	{
		if (!mConnMgr->getOthersNetStatus(id, pcs))
		{
			/* fill in blank data */
			d.localPort = 0;
			d.extPort = 0;
			d.lastConnect = 0;
			d.connectPeriod = 0;
			d.state = 0;
			d.netMode = 0;

			return true;
		}
	}

	//TODO : check use of this details
	// From all addresses, show the most recent one if no address is currently in use.
	struct sockaddr_in best_local_addr = (!strcmp(inet_ntoa(pcs.currentlocaladdr.sin_addr),"0.0.0.0"))?getPreferredAddress(pcs.dht.laddr,pcs.dht.ts,pcs.disc.laddr,pcs.disc.ts,pcs.peer.laddr,pcs.peer.ts):pcs.currentlocaladdr ;
	struct sockaddr_in best_servr_addr = (!strcmp(inet_ntoa(pcs.currentserveraddr.sin_addr),"0.0.0.0"))?getPreferredAddress(pcs.dht.raddr,pcs.dht.ts,pcs.disc.raddr,pcs.disc.ts,pcs.peer.raddr,pcs.peer.ts):pcs.currentserveraddr ;
		

	/* fill from pcs */

	d.localAddr	= inet_ntoa(best_local_addr.sin_addr);
	d.localPort	= ntohs(best_local_addr.sin_port);
	d.extAddr	= inet_ntoa(best_servr_addr.sin_addr);
	d.extPort	= ntohs(best_servr_addr.sin_port);
	d.lastConnect	= pcs.lastcontact;
	d.connectPeriod = 0;
	std::list<std::string> ipAddressList;
	std::list<IpAddressTimed> pcsList = pcs.getIpAddressList();
	for (std::list<IpAddressTimed>::iterator ipListIt = pcsList.begin(); ipListIt!=(pcsList.end()); ipListIt++) {
	    std::ostringstream toto;
	    toto << ntohs(ipListIt->ipAddr.sin_port);
	    ipAddressList.push_back(std::string(inet_ntoa(ipListIt->ipAddr.sin_addr)) + ":" + toto.str());
	}
	d.ipAddressList = ipAddressList;


	/* Translate */
	
	d.state		= 0;
	if (pcs.state & RS_PEER_S_FRIEND)
		d.state |= RS_PEER_STATE_FRIEND;
	if (pcs.state & RS_PEER_S_ONLINE)
		d.state |= RS_PEER_STATE_ONLINE;
	if (pcs.state & RS_PEER_S_CONNECTED)
		d.state |= RS_PEER_STATE_CONNECTED;
	if (pcs.state & RS_PEER_S_UNREACHABLE)
		d.state |= RS_PEER_STATE_UNREACHABLE;

	switch(pcs.netMode & RS_NET_MODE_ACTUAL)
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

	if (pcs.netMode & RS_NET_MODE_TRY_EXT)
	{
		d.tryNetMode	= RS_NETMODE_EXT;
	}
	else if (pcs.netMode & RS_NET_MODE_TRY_UPNP)
	{
		d.tryNetMode	= RS_NETMODE_UPNP;
	}
	else
	{
		d.tryNetMode 	= RS_NETMODE_UDP;
	}

	d.visState	= 0;
	if (!(pcs.visState & RS_VIS_STATE_NODISC))
	{
		d.visState |= RS_VS_DISC_ON;
	}

	if (!(pcs.visState & RS_VIS_STATE_NODHT))
	{
		d.visState |= RS_VS_DHT_ON;
	}


	/* Finally determine AutoConnect Status */
	std::ostringstream autostr;
	if (pcs.inConnAttempt)
	{
		autostr << "Trying " << inet_ntoa(pcs.currentConnAddrAttempt.addr.sin_addr) << ":" <<  ntohs(pcs.currentConnAddrAttempt.addr.sin_port);
	}
	else if (pcs.state & RS_PEER_S_CONNECTED)
	{
		if (pcs.connecttype == RS_NET_CONN_TCP_ALL)
		{
			autostr << "Connected: TCP";
		}
		else if (pcs.connecttype == RS_NET_CONN_UDP_ALL)
		{
			autostr << "Connected: UDP";
		}
		else 
		{
			autostr << "Connected: Unknown";
		}
	}
	else
	{
		autostr << RsPeerStateString(pcs.state);
	}

	d.autoconnect = autostr.str();

	return true;
}


std::string p3Peers::getPeerPGPName(std::string id)
{
	/* get from mAuthMgr as it should have more peers? */
	return mAuthMgr->getIssuerName(id);
}

std::string p3Peers::getPeerName(std::string id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPeerName() " << id;
	std::cerr << std::endl;
#endif

	/* get from mAuthMgr as it should have more peers? */
	return mAuthMgr->getName(id);
}


bool	p3Peers::getPGPFriendList(std::list<std::string> &ids)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPGPFriendList()";
	std::cerr << std::endl;
#endif

	std::list<std::string> certids;
	std::list<std::string>::iterator it;

	mConnMgr->getFriendList(certids);

        /* get from mAuthMgr (first) */
	for(it = certids.begin(); it != certids.end(); it++)
	{
     		pqiAuthDetails detail;
        	if (!mAuthMgr->getDetails(*it, detail))
        	{
			continue;
		}

#ifdef P3PEERS_DEBUG
		std::cerr << "p3Peers::getPGPFriendList() Cert Id: " << *it;
		std::cerr << " Issuer: " << detail.issuer;
		std::cerr << std::endl;
#endif

#if 0
		if (!mAuthMgr->isPGPvalid(detail.issuer))
		{
			continue;
		}
#endif

		if (ids.end() == std::find(ids.begin(),ids.end(),detail.issuer))
		{

#ifdef P3PEERS_DEBUG
			std::cerr << "p3Peers::getPGPFriendList() Adding Friend: ";
			std::cerr << detail.issuer;
			std::cerr << std::endl;
#endif
			
			ids.push_back(detail.issuer);
		}
	}
	return true;
}



bool	p3Peers::getPGPAllList(std::list<std::string> &ids)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPGPOthersList()";
	std::cerr << std::endl;
#endif

	/* get from mAuthMgr */
	mAuthMgr->getPGPAllList(ids);
	return true;
}

std::string p3Peers::getPGPOwnId()
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::getPGPOwnId()";
	std::cerr << std::endl;
#endif

	/* get from mAuthMgr */
	return mAuthMgr->PGPOwnId();
}




	/* Add/Remove Friends */
bool 	p3Peers::addFriend(std::string id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::addFriend() " << id;
	std::cerr << std::endl;
#endif

	return mConnMgr->addFriend(id);
}

bool 	p3Peers::removeFriend(std::string id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::removeFriend() " << id;
	std::cerr << std::endl;
#endif

	return mConnMgr->removeFriend(id);
}

	/* Network Stuff */
bool 	p3Peers::connectAttempt(std::string id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::connectAttempt() " << id;
	std::cerr << std::endl;
#endif

	return mConnMgr->retryConnect(id);
}

void p3Peers::getIPServersList(std::list<std::string>& ip_servers) 
{
	mConnMgr->getIPServersList(ip_servers) ;
}
void p3Peers::allowServerIPDetermination(bool b) 
{
	mConnMgr->setIPServersEnabled(b) ;
}
bool p3Peers::getAllowServerIPDetermination() 
{
	return mConnMgr->getIPServersEnabled() ;
}

bool 	p3Peers::setLocalAddress(std::string id, std::string addr_str, uint16_t port)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::setLocalAddress() " << id;
	std::cerr << std::endl;
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
		return mConnMgr->setLocalAddress(id, addr);
	}
	return false;
}

bool 	p3Peers::setExtAddress(std::string id, std::string addr_str, uint16_t port)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::setExtAddress() " << id;
	std::cerr << std::endl;
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
		return mConnMgr->setExtAddress(id, addr);
	}
	return false;
}


bool 	p3Peers::setNetworkMode(std::string id, uint32_t extNetMode)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::setNetworkMode() " << id;
	std::cerr << std::endl;
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

	return mConnMgr->setNetworkMode(id, netMode);
}


bool
p3Peers::setVisState(std::string id, uint32_t extVisState)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::setVisState() " << id;
	std::cerr << std::endl;
#endif

	uint32_t visState = 0;
	if (!(extVisState & RS_VS_DHT_ON))
		visState |= RS_VIS_STATE_NODHT;
	if (!(extVisState & RS_VS_DISC_ON))
		visState |= RS_VIS_STATE_NODISC;

	return mConnMgr->setVisState(id, visState);
}

//===========================================================================
	/* Auth Stuff */
std::string
p3Peers::GetRetroshareInvite()
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::GetRetroshareInvite()";
	std::cerr << std::endl;
#endif

	std::cerr << "p3Peers::GetRetroshareInvite()";
	std::cerr << std::endl;

	std::string ownId = mAuthMgr->OwnId();
	std::string certstr = mAuthMgr->SaveCertificateToString(ownId);
	std::string name = mAuthMgr->getName(ownId);
	
	std::string pgpownId = mAuthMgr->PGPOwnId();
	std::string pgpcertstr = mAuthMgr->SaveCertificateToString(pgpownId);
	
	std::cerr << "p3Peers::GetRetroshareInvite() SSL Cert:";
	std::cerr << std::endl;
	std::cerr << certstr;
	std::cerr << std::endl;

	std::cerr << "p3Peers::GetRetroshareInvite() PGP Cert:";
	std::cerr << std::endl;
	std::cerr << pgpcertstr;
	std::cerr << std::endl;
	
	std::string combinedcerts = certstr;
	combinedcerts += '\n';
	combinedcerts += pgpcertstr;
	combinedcerts += '\n';
	
	return combinedcerts;
}

//===========================================================================

bool 	p3Peers::LoadCertificateFromFile(std::string fname, std::string &id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::LoadCertificateFromFile() ";
	std::cerr << std::endl;
#endif

	return mAuthMgr->LoadCertificateFromFile(fname, id);
}


bool splitCerts(std::string in, std::string &sslcert, std::string &pgpcert)
{
	std::cerr << "splitCerts():" << in;
	std::cerr << std::endl;

	/* search for -----END CERTIFICATE----- */
	std::string sslend("-----END CERTIFICATE-----");
	std::string pgpend("-----END PGP PUBLIC KEY BLOCK-----");
	size_t pos = in.find(sslend);
	size_t pos2 = in.find(pgpend);
	size_t ssllen, pgplen;

	if (pos != std::string::npos)
	{
		std::cerr << "splitCerts(): Found SSL Cert";
		std::cerr << std::endl;

		ssllen = pos + sslend.length();
		sslcert = in.substr(0, ssllen);

		if (pos2 != std::string::npos)
		{
			std::cerr << "splitCerts(): Found SSL + PGP Cert";
			std::cerr << std::endl;

			pgplen = pos2 + pgpend.length() - ssllen;
			pgpcert = in.substr(ssllen, pgplen);
		}
		return true;
	}
	else if (pos2 != std::string::npos)
	{
		std::cerr << "splitCerts(): Found PGP Cert Only";
		std::cerr << std::endl;

		pgplen = pos2 + pgpend.length();
		pgpcert = in.substr(0, pgplen);
		return true;
	}
	return false;
}



bool 	p3Peers::LoadCertificateFromString(std::string cert, std::string &id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::LoadCertificateFromString() ";
	std::cerr << std::endl;
#endif

	std::string sslcert;
	std::string pgpcert;
	bool ret = false;
	if (splitCerts(cert, sslcert, pgpcert))
	{
		if (pgpcert != "")
		{
			std::cerr << "pgpcert .... " << std::endl;
			std::cerr << pgpcert << std::endl;

			ret = mAuthMgr->LoadCertificateFromString(pgpcert, id);
		}
		if (sslcert != "")
		{
			std::cerr << "sslcert .... " << std::endl;
			std::cerr << sslcert << std::endl;

			ret = mAuthMgr->LoadCertificateFromString(sslcert, id);
		}
	}

	return ret;
}





bool 	p3Peers::SaveCertificateToFile(std::string id, std::string fname)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::SaveCertificateToFile() " << id;
	std::cerr << std::endl;
#endif

	ensureExtension(fname, "pqi");

	return mAuthMgr->SaveCertificateToFile(id, fname);
}

std::string p3Peers::SaveCertificateToString(std::string id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::SaveCertificateToString() " << id;
	std::cerr << std::endl;
#endif

	return mAuthMgr->SaveCertificateToString(id);
}

bool 	p3Peers::AuthCertificate(std::string id, std::string code)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::AuthCertificate() " << id;
	std::cerr << std::endl;
#endif

	if (mAuthMgr->AuthCertificate(id))
	{
#ifdef P3PEERS_DEBUG
		std::cerr << "p3Peers::AuthCertificate() OK ... Adding as Friend";
		std::cerr << std::endl;
#endif

		/* add in as a friend */
		return mConnMgr->addFriend(id);
	}
	return false;
}

bool 	p3Peers::SignCertificate(std::string id)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::SignCertificate() " << id;
	std::cerr << std::endl;
#endif

	return mAuthMgr->SignCertificate(id);
}

bool 	p3Peers::TrustCertificate(std::string id, bool trust)
{
#ifdef P3PEERS_DEBUG
	std::cerr << "p3Peers::TrustCertificate() " << id;
	std::cerr << std::endl;
#endif

	return mAuthMgr->TrustCertificate(id, trust);
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





RsPeerDetails::RsPeerDetails()
	:trustLvl(0), ownsign(false), trusted(false), state(0), netMode(0),
	lastConnect(0), connectPeriod(0)
{
	return;
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
	for(it = detail.signers.begin();
		it != detail.signers.end(); it++)
	{
		out << "\t" << *it;
		out << std::endl;
	}
	out << std::endl;

	out << " trustLvl:    " << detail.trustLvl;
	out << " ownSign:     " << detail.ownsign;
	out << " trusted:     " << detail.trusted;
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


/********** TRANSLATION ****/


uint32_t RsPeerTranslateTrust(uint32_t trustLvl)
{
/**************** PQI_USE_XPGP ******************/
#if defined(PQI_USE_XPGP)
	switch(trustLvl)
	{
		case TRUST_SIGN_OWN:
		case TRUST_SIGN_TRSTED:
		case TRUST_SIGN_AUTHEN:
			return RS_TRUST_LVL_GOOD;
			break;

		case TRUST_SIGN_BASIC:
			return RS_TRUST_LVL_MARGINAL;
			break;

		case TRUST_SIGN_UNTRUSTED:
		case TRUST_SIGN_UNKNOWN:
		case TRUST_SIGN_NONE:
		default:
			return RS_TRUST_LVL_UNKNOWN;
			break;
	}
#else /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/
#endif /* X509 Certificates */
/**************** PQI_USE_XPGP ******************/

	return RS_TRUST_LVL_UNKNOWN;
}

