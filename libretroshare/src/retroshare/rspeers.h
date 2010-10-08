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

/* The Main Interface Class - for information about your Peers
* A peer is another RS instance, means associated with an SSL certificate
* A same GPG person can have multiple peer running with different SSL certs signed by the same GPG key
* Thus a peer have SSL cert details, and also the parent GPG details
*/
class RsPeers;
extern RsPeers   *rsPeers;

/* Trust Levels */
const uint32_t RS_TRUST_LVL_NONE	= 2;
const uint32_t RS_TRUST_LVL_MARGINAL	= 3;
const uint32_t RS_TRUST_LVL_FULL	= 4;
const uint32_t RS_TRUST_LVL_ULTIMATE	= 5;


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

/* Groups */
#define RS_GROUP_ID_FRIENDS    "Friends"
#define RS_GROUP_ID_FAMILY     "Family"
#define RS_GROUP_ID_COWORKERS  "Co-Workers"
#define RS_GROUP_ID_OTHERS     "Other Contacts"
#define RS_GROUP_ID_FAVORITES  "Favorites"

const uint32_t RS_GROUP_FLAG_STANDARD = 0x0001;

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
        bool isOnlyGPGdetail;
	std::string id;
        std::string gpg_id;
	std::string name;
	std::string email;
	std::string location;
	std::string org;
	
	std::string issuer;

	std::string fpr; /* pgp fingerprint */
	std::string authcode; 
        std::list<std::string> gpgSigners;

	uint32_t trustLvl;
	uint32_t validLvl;

        bool ownsign; /* we have signed the remote peer GPG key */
        bool hasSignedMe; /* the remote peer has signed my GPG key */

        bool accept_connection;

	/* Network details (only valid if friend) */
	uint32_t		state;

        std::string             localAddr;
        uint16_t                localPort;
        std::string             extAddr;
        uint16_t                extPort;
        std::string             dyndns;
	std::list<std::string>  ipAddressList;

	uint32_t		netMode;
	uint32_t		tryNetMode; /* only for ownState */
	uint32_t		visState;

	/* basic stats */
	uint32_t		lastConnect; /* how long ago */
	std::string		autoconnect;
        uint32_t		connectPeriod;
};

class RsGroupInfo
{
public:
	RsGroupInfo();

	std::string id;
	std::string name;
	uint32_t    flag;

	std::list<std::string> peerIds;
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

virtual bool	getOnlineList(std::list<std::string> &ssl_ids)	= 0;
virtual bool	getFriendList(std::list<std::string> &ssl_ids)	= 0;
//virtual bool	getOthersList(std::list<std::string> &ssl_ids)	= 0;
virtual bool    getPeerCount (unsigned int *pnFriendCount, unsigned int *pnnOnlineCount, bool ssl) = 0;

virtual bool    isOnline(std::string ssl_id)			= 0;
virtual bool    isFriend(std::string ssl_id)			= 0;
virtual bool    isGPGAccepted(std::string gpg_id_is_friend)			= 0; //
virtual std::string getPeerName(std::string ssl_or_gpg_id)			= 0;
virtual std::string getGPGName(std::string gpg_id)	= 0;
virtual bool	getPeerDetails(std::string ssl_or_gpg_id, RsPeerDetails &d) = 0; //get Peer detail accept SSL and PGP certs

		/* Using PGP Ids */
virtual std::string getGPGOwnId()				= 0;
virtual std::string getGPGId(std::string sslid_or_gpgid)	= 0; //return the gpg id of the given gpg or ssl id
virtual bool    getGPGAcceptedList(std::list<std::string> &gpg_ids)   = 0;
virtual bool    getGPGSignedList(std::list<std::string> &gpg_ids)   = 0;//friends that we accpet to connect with but we don't want to sign their gpg key
virtual bool    getGPGValidList(std::list<std::string> &gpg_ids)   = 0;
virtual bool    getGPGAllList(std::list<std::string> &gpg_ids) 	= 0;
virtual bool	getGPGDetails(std::string gpg_id, RsPeerDetails &d) = 0;
virtual bool    getSSLChildListOfGPGId(std::string gpg_id, std::list<std::string> &ssl_ids) = 0;

	/* Add/Remove Friends */
virtual	bool addFriend(std::string ssl_id, std::string gpg_id)        			= 0;
virtual	bool addDummyFriend(std::string gpg_id)        			= 0; //we want to add a empty ssl friend for this gpg id
virtual	bool isDummyFriend(std::string ssl_id)                  = 0;
virtual	bool removeFriend(std::string ssl_or_gpg_id)  			= 0;

	/* Network Stuff */
virtual	bool connectAttempt(std::string ssl_id)			= 0;
virtual bool setLocation(std::string ssl_id, std::string location) = 0;//location is shown in the gui to differentiate ssl certs
virtual	bool setLocalAddress(std::string ssl_id, std::string addr, uint16_t port) = 0;
virtual	bool setExtAddress(  std::string ssl_id, std::string addr, uint16_t port) = 0;
virtual	bool setDynDNS(std::string id, std::string addr) = 0;
virtual	bool setNetworkMode(std::string ssl_id, uint32_t netMode) 	= 0;
virtual bool setVisState(std::string ssl_id, uint32_t vis)		= 0;

virtual void getIPServersList(std::list<std::string>& ip_servers) = 0;
virtual void allowServerIPDetermination(bool) = 0;
virtual void allowTunnelConnection(bool) = 0;
virtual bool getAllowServerIPDetermination() = 0 ;
virtual bool getAllowTunnelConnection() = 0 ;

	/* Auth Stuff */
virtual	std::string GetRetroshareInvite(const std::string& ssl_id) 			= 0;
virtual	std::string GetRetroshareInvite() 			= 0;

virtual	bool loadCertificateFromFile(std::string fname, std::string &ssl_id, std::string &gpg_id)  = 0;
virtual	bool loadDetailsFromStringCert(std::string certGPG, RsPeerDetails &pd) = 0;
virtual	bool saveCertificateToFile(std::string id, std::string fname)  = 0;
virtual	std::string saveCertificateToString(std::string id)  	= 0;

virtual	bool setAcceptToConnectGPGCertificate(std::string gpg_id, bool acceptance) = 0;
virtual	bool signGPGCertificate(std::string gpg_id)                   	= 0;
virtual	bool trustGPGCertificate(std::string gpg_id, uint32_t trustlvl) 	= 0;

	/* Group Stuff */
virtual bool    addGroup(RsGroupInfo &groupInfo) = 0;
virtual bool    editGroup(const std::string &groupId, RsGroupInfo &groupInfo) = 0;
virtual bool    removeGroup(const std::string &groupId) = 0;
virtual bool    getGroupInfo(const std::string &groupId, RsGroupInfo &groupInfo) = 0;
virtual bool    getGroupInfoList(std::list<RsGroupInfo> &groupInfoList) = 0;
// groupId == "" && assign == false -> remove from all groups
virtual bool    assignPeerToGroup(const std::string &groupId, const std::string &peerId, bool assign) = 0;
virtual bool    assignPeersToGroup(const std::string &groupId, const std::list<std::string> &peerIds, bool assign) = 0;

};

#endif
