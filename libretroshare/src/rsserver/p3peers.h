#ifndef RETROSHARE_P3_PEER_INTERFACE_H
#define RETROSHARE_P3_PEER_INTERFACE_H

/*
 * libretroshare/src/rsserver: p3peers.h
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

#include "rsiface/rspeers.h"
#include "pqi/p3connmgr.h"

class p3Peers: public RsPeers 
{
	public:

        p3Peers(p3ConnectMgr *cm);
virtual ~p3Peers() { return; }

	/* Updates ... */
virtual bool FriendsChanged();
virtual bool OthersChanged();

	/* Peer Details (Net & Auth) */
virtual std::string getOwnId();

virtual bool	getOnlineList(std::list<std::string> &ids);
virtual bool	getFriendList(std::list<std::string> &ids);
//virtual bool	getOthersList(std::list<std::string> &ids);
virtual void    getPeerCount (unsigned int *pnFriendCount, unsigned int *pnOnlineCount);

virtual bool    isOnline(std::string id);
virtual bool    isFriend(std::string id);
virtual bool    isGPGAccepted(std::string gpg_id_is_friend); //
virtual std::string getGPGName(std::string gpg_id);
virtual std::string getPeerName(std::string ssl_or_gpg_id);
virtual bool	getPeerDetails(std::string id, RsPeerDetails &d);

                /* Using PGP Ids */
virtual std::string getGPGOwnId();
virtual std::string getGPGId(std::string ssl_id);
virtual bool    getGPGAcceptedList(std::list<std::string> &ids);
virtual bool    getGPGSignedList(std::list<std::string> &ids);
virtual bool    getGPGValidList(std::list<std::string> &ids);
virtual bool    getGPGAllList(std::list<std::string> &ids);
virtual bool	getGPGDetails(std::string id, RsPeerDetails &d);
virtual bool    getSSLChildListOfGPGId(std::string gpg_id, std::list<std::string> &ids);

	/* Add/Remove Friends */
virtual	bool addFriend(std::string ssl_id, std::string gpg_id);
virtual	bool addDummyFriend(std::string gpg_id); //we want to add a empty ssl friend for this gpg id
virtual	bool isDummyFriend(std::string ssl_id);
virtual	bool removeFriend(std::string ssl_id);

	/* Network Stuff */
virtual	bool connectAttempt(std::string id);
virtual bool setLocation(std::string ssl_id, std::string location);//location is shown in the gui to differentiate ssl certs
virtual	bool setLocalAddress(std::string id, std::string addr, uint16_t port);
virtual	bool setExtAddress(std::string id, std::string addr, uint16_t port);
virtual	bool setDynDNS(std::string id, std::string dyndns);
virtual	bool setNetworkMode(std::string id, uint32_t netMode);
virtual bool setVisState(std::string id, uint32_t mode); 

virtual void getIPServersList(std::list<std::string>& ip_servers) ;
virtual void allowServerIPDetermination(bool) ;
virtual void allowTunnelConnection(bool) ;
virtual bool getAllowServerIPDetermination() ;
virtual bool getAllowTunnelConnection() ;

	/* Auth Stuff */
virtual	std::string GetRetroshareInvite();

virtual	bool loadCertificateFromFile(std::string fname, std::string &id, std::string &gpg_id);
virtual	bool loadDetailsFromStringCert(std::string cert, RsPeerDetails &pd);
virtual	bool saveCertificateToFile(std::string id, std::string fname);
virtual	std::string saveCertificateToString(std::string id);

virtual	bool setAcceptToConnectGPGCertificate(std::string gpg_id, bool acceptance);
virtual	bool signGPGCertificate(std::string id);
virtual	bool trustGPGCertificate(std::string id, uint32_t trustlvl);

	private:

	p3ConnectMgr *mConnMgr;
};

#endif
