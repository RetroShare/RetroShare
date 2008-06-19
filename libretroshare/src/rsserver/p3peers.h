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
#include "pqi/p3authmgr.h"

class p3Peers: public RsPeers 
{
	public:

	p3Peers(p3ConnectMgr *cm, p3AuthMgr *am);
virtual ~p3Peers() { return; }

	/* Updates ... */
virtual bool FriendsChanged();
virtual bool OthersChanged();

	/* Peer Details (Net & Auth) */
virtual std::string getOwnId();

virtual bool	getOnlineList(std::list<std::string> &ids);
virtual bool	getFriendList(std::list<std::string> &ids);
virtual bool	getOthersList(std::list<std::string> &ids);

virtual bool    isOnline(std::string id);
virtual bool    isFriend(std::string id);
virtual std::string getPeerName(std::string id);
virtual bool	getPeerDetails(std::string id, RsPeerDetails &d);

	/* Add/Remove Friends */
virtual	bool addFriend(std::string id);
virtual	bool removeFriend(std::string id);

	/* Network Stuff */
virtual	bool connectAttempt(std::string id);
virtual	bool setLocalAddress(std::string id, std::string addr, uint16_t port);
virtual	bool setExtAddress(std::string id, std::string addr, uint16_t port);
virtual	bool setNetworkMode(std::string id, uint32_t netMode);
virtual bool setVisState(std::string id, uint32_t mode); 

	/* Auth Stuff */
virtual	std::string GetRetroshareInvite();

virtual	bool LoadCertificateFromFile(std::string fname, std::string &id);
virtual	bool LoadCertificateFromString(std::string cert, std::string &id);
virtual	bool SaveCertificateToFile(std::string id, std::string fname);
virtual	std::string SaveCertificateToString(std::string id);

virtual	bool AuthCertificate(std::string id, std::string code);
virtual	bool SignCertificate(std::string id);
virtual	bool TrustCertificate(std::string id, bool trust);
virtual bool certToFile(void);

	private:

	p3ConnectMgr *mConnMgr;
	p3AuthMgr    *mAuthMgr;
};

#endif
