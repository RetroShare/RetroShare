#ifndef RETROSHARE_PEER_GUI_INTERFACE_H
#define RETROSHARE_PEER_GUI_INTERFACE_H

/*
 * libretroshare/src/rsiface: rspeer.h
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

#include <inttypes.h>
#include <string>
#include <list>

/* The Main Interface Class - for information about your Peers */
class RsPeers;
extern RsPeers   *rsPeers;

/* Trust Levels */
const uint32_t RS_TRUST_LVL_UNKNOWN	= 0x0001;
const uint32_t RS_TRUST_LVL_MARGINAL	= 0x0002;
const uint32_t RS_TRUST_LVL_GOOD	= 0x0003;


/* Net Mode */
const uint32_t RS_NETMODE_UDP		= 0x0001;
const uint32_t RS_NETMODE_UPNP		= 0x0002;
const uint32_t RS_NETMODE_EXT		= 0x0003;
const uint32_t RS_NETMODE_UNREACHABLE	= 0x0004;

/* Visibility */
const uint32_t RS_VS_DHT_ON		= 0x0001;
const uint32_t RS_VS_DISC_ON		= 0x0002;

/* State */
const uint32_t RS_PEER_STATE_FRIEND	= 0x0001;
const uint32_t RS_PEER_STATE_ONLINE	= 0x0002;
const uint32_t RS_PEER_STATE_CONNECTED  = 0x0004;
const uint32_t RS_PEER_STATE_UNREACHABLE= 0x0008;

/* A couple of helper functions for translating the numbers games */

std::string RsPeerTrustString(uint32_t trustLvl);
std::string RsPeerStateString(uint32_t state);
std::string RsPeerNetModeString(uint32_t netModel);
std::string RsPeerLastConnectString(uint32_t lastConnect);


/* Details class */
class RsPeerDetails
{
	public:

	RsPeerDetails();

	/* Auth details */
	std::string id;
	std::string name;
	std::string email;
	std::string location;
	std::string org;
	
	std::string fpr; /* fingerprint */
	std::string authcode; 
	std::list<std::string> signers;

	uint32_t trustLvl;

	bool ownsign; /* we have signed certificate */
	bool trusted; /* we trust their signature on others */

	/* Network details (only valid if friend) */
	uint32_t		state;

        std::string             localAddr;
        uint16_t                localPort;
        std::string             extAddr;
        uint16_t                extPort;

	uint32_t		netMode;
	uint32_t		tryNetMode; /* only for ownState */
	uint32_t		visState;

	/* basic stats */
	uint32_t		lastConnect; /* how long ago */
	std::string		autoconnect;
	uint32_t		connectPeriod; 
};

std::ostream &operator<<(std::ostream &out, const RsPeerDetails &detail);

class RsPeers 
{
	public:

	RsPeers()  { return; }
virtual ~RsPeers() { return; }

	/* Updates ... */
virtual bool FriendsChanged() 					= 0;
virtual bool OthersChanged() 					= 0;

	/* Peer Details (Net & Auth) */
virtual std::string getOwnId()					= 0;

virtual bool	getOnlineList(std::list<std::string> &ids)	= 0;
virtual bool	getFriendList(std::list<std::string> &ids)	= 0;
virtual bool	getOthersList(std::list<std::string> &ids)	= 0;

virtual bool    isOnline(std::string id)			= 0;
virtual bool    isFriend(std::string id)			= 0;
virtual std::string getPeerName(std::string id)			= 0;
virtual bool	getPeerDetails(std::string id, RsPeerDetails &d) = 0;

	/* Add/Remove Friends */
virtual	bool addFriend(std::string id)        			= 0;
virtual	bool removeFriend(std::string id)  			= 0;

	/* Network Stuff */
virtual	bool connectAttempt(std::string id)			= 0;
virtual	bool setLocalAddress(std::string id, std::string addr, uint16_t port) = 0;
virtual	bool setExtAddress(  std::string id, std::string addr, uint16_t port) = 0;
virtual	bool setNetworkMode(std::string id, uint32_t netMode) 	= 0;
virtual bool setVisState(std::string id, uint32_t vis)		= 0;

	/* Auth Stuff */
virtual	std::string GetRetroshareInvite() 			= 0;

virtual	bool LoadCertificateFromFile(std::string fname, std::string &id)  = 0;
virtual	bool LoadCertificateFromString(std::string cert, std::string &id)  = 0;
virtual	bool SaveCertificateToFile(std::string id, std::string fname)  = 0;
virtual	std::string SaveCertificateToString(std::string id)  	= 0;

virtual	bool AuthCertificate(std::string id, std::string code) 	= 0;
virtual	bool SignCertificate(std::string id)                   	= 0;
virtual	bool TrustCertificate(std::string id, bool trust) 	= 0;

};

#endif
