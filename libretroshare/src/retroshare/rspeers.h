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

#include <retroshare/rstypes.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rsids.h>

/* The Main Interface Class - for information about your Peers
* A peer is another RS instance, means associated with an SSL certificate
* A same GPG person can have multiple peer running with different SSL certs signed by the same GPG key
* Thus a peer have SSL cert details, and also the parent GPG details
*/
class RsPeers;
extern RsPeers   *rsPeers;

/* Trust Levels. Should be the same values than what is declared in PGPHandler.h */

const uint32_t RS_TRUST_LVL_UNDEFINED  = 0;
const uint32_t RS_TRUST_LVL_UNKNOWN    = 1;
const uint32_t RS_TRUST_LVL_NEVER      = 2;
const uint32_t RS_TRUST_LVL_MARGINAL   = 3;
const uint32_t RS_TRUST_LVL_FULL	      = 4;
const uint32_t RS_TRUST_LVL_ULTIMATE   = 5;

/* Net Mode */
const uint32_t RS_NETMODE_UDP		= 0x0001;
const uint32_t RS_NETMODE_UPNP		= 0x0002;
const uint32_t RS_NETMODE_EXT		= 0x0003;
const uint32_t RS_NETMODE_HIDDEN	= 0x0004;
const uint32_t RS_NETMODE_UNREACHABLE	= 0x0005;

/* Visibility */
const uint32_t RS_VS_DISC_OFF		= 0x0000;
const uint32_t RS_VS_DISC_MINIMAL	= 0x0001;
const uint32_t RS_VS_DISC_FULL		= 0x0002;

const uint32_t RS_VS_DHT_OFF		= 0x0000;
const uint32_t RS_VS_DHT_PASSIVE	= 0x0001;
const uint32_t RS_VS_DHT_FULL		= 0x0002;

/* State */
const uint32_t RS_PEER_STATE_FRIEND	= 0x0001;
const uint32_t RS_PEER_STATE_ONLINE	= 0x0002;
const uint32_t RS_PEER_STATE_CONNECTED  = 0x0004;
const uint32_t RS_PEER_STATE_UNREACHABLE= 0x0008;

// Service option flags.
//
const ServicePermissionFlags RS_NODE_PERM_NONE       ( 0x00000000 ) ;// 0x1, 0x2 and Ox4 are deprecated.
const ServicePermissionFlags RS_NODE_PERM_DIRECT_DL  ( 0x00000008 ) ;// Accept to directly DL from this peer (breaks anonymity)
const ServicePermissionFlags RS_NODE_PERM_ALLOW_PUSH ( 0x00000010 ) ;// Auto-DL files recommended by this peer
const ServicePermissionFlags RS_NODE_PERM_REQUIRE_WL ( 0x00000020 ) ;// Require white list clearance for connection
const ServicePermissionFlags RS_NODE_PERM_DEFAULT    =  RS_NODE_PERM_DIRECT_DL | RS_NODE_PERM_REQUIRE_WL;
const ServicePermissionFlags RS_NODE_PERM_ALL        =  RS_NODE_PERM_DIRECT_DL | RS_NODE_PERM_ALLOW_PUSH | RS_NODE_PERM_REQUIRE_WL;

// ...

/* Connect state */
const uint32_t RS_PEER_CONNECTSTATE_OFFLINE           = 0;
const uint32_t RS_PEER_CONNECTSTATE_TRYING_TCP        = 2;
const uint32_t RS_PEER_CONNECTSTATE_TRYING_UDP        = 3;
const uint32_t RS_PEER_CONNECTSTATE_CONNECTED_TCP     = 4;
const uint32_t RS_PEER_CONNECTSTATE_CONNECTED_UDP     = 5;
const uint32_t RS_PEER_CONNECTSTATE_CONNECTED_TOR     = 6;
const uint32_t RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN = 7;

/* Error codes for certificate cleaning and cert parsing. Numbers should not overlap. */

const int RS_PEER_CERT_CLEANING_CODE_NO_ERROR         = 0x00 ;
const int RS_PEER_CERT_CLEANING_CODE_UNKOWN_ERROR     = 0x01 ;
const int RS_PEER_CERT_CLEANING_CODE_NO_BEGIN_TAG     = 0x02 ;
const int RS_PEER_CERT_CLEANING_CODE_NO_END_TAG       = 0x03 ;
const int RS_PEER_CERT_CLEANING_CODE_NO_CHECKSUM      = 0x04 ;
const int RS_PEER_CERT_CLEANING_CODE_WRONG_NUMBER     = 0x05 ;
const int RS_PEER_CERT_CLEANING_CODE_WRONG_RADIX_CHAR = 0x06 ;

const uint32_t CERTIFICATE_PARSING_ERROR_NO_ERROR                  = 0x10 ;
const uint32_t CERTIFICATE_PARSING_ERROR_SIZE_ERROR                = 0x11 ;
const uint32_t CERTIFICATE_PARSING_ERROR_INVALID_LOCATION_ID       = 0x12 ;
const uint32_t CERTIFICATE_PARSING_ERROR_INVALID_EXTERNAL_IP       = 0x13 ;
const uint32_t CERTIFICATE_PARSING_ERROR_INVALID_LOCAL_IP          = 0x14 ;
const uint32_t CERTIFICATE_PARSING_ERROR_INVALID_CHECKSUM_SECTION  = 0x15 ;
const uint32_t CERTIFICATE_PARSING_ERROR_CHECKSUM_ERROR            = 0x16 ;
const uint32_t CERTIFICATE_PARSING_ERROR_UNKNOWN_SECTION_PTAG      = 0x17 ;
const uint32_t CERTIFICATE_PARSING_ERROR_MISSING_CHECKSUM          = 0x18 ;
const uint32_t CERTIFICATE_PARSING_ERROR_WRONG_VERSION             = 0x19 ;

const uint32_t PGP_KEYRING_REMOVAL_ERROR_NO_ERROR                  = 0x20 ;
const uint32_t PGP_KEYRING_REMOVAL_ERROR_CANT_REMOVE_SECRET_KEYS   = 0x21 ;
const uint32_t PGP_KEYRING_REMOVAL_ERROR_CANNOT_CREATE_BACKUP      = 0x22 ;
const uint32_t PGP_KEYRING_REMOVAL_ERROR_CANNOT_WRITE_BACKUP       = 0x23 ;
const uint32_t PGP_KEYRING_REMOVAL_ERROR_DATA_INCONSISTENCY        = 0x24 ;

/* LinkType Flags */

// CONNECTION
const uint32_t RS_NET_CONN_TRANS_MASK			= 0x0000ffff;
const uint32_t RS_NET_CONN_TRANS_TCP_MASK		= 0x0000000f;
const uint32_t RS_NET_CONN_TRANS_TCP_UNKNOWN		= 0x00000001;
const uint32_t RS_NET_CONN_TRANS_TCP_LOCAL		= 0x00000002;
const uint32_t RS_NET_CONN_TRANS_TCP_EXTERNAL		= 0x00000004;

const uint32_t RS_NET_CONN_TRANS_UDP_MASK		= 0x000000f0;
const uint32_t RS_NET_CONN_TRANS_UDP_UNKNOWN		= 0x00000010;
const uint32_t RS_NET_CONN_TRANS_UDP_DIRECT		= 0x00000020;
const uint32_t RS_NET_CONN_TRANS_UDP_PROXY		= 0x00000040;
const uint32_t RS_NET_CONN_TRANS_UDP_RELAY		= 0x00000080;

const uint32_t RS_NET_CONN_TRANS_OTHER_MASK		= 0x00000f00;

const uint32_t RS_NET_CONN_TRANS_UNKNOWN		= 0x00001000;


const uint32_t RS_NET_CONN_SPEED_MASK			= 0x000f0000;
const uint32_t RS_NET_CONN_SPEED_UNKNOWN		= 0x00000000;
const uint32_t RS_NET_CONN_SPEED_TRICKLE		= 0x00010000;
const uint32_t RS_NET_CONN_SPEED_LOW			= 0x00020000;
const uint32_t RS_NET_CONN_SPEED_NORMAL			= 0x00040000;
const uint32_t RS_NET_CONN_SPEED_HIGH			= 0x00080000;

const uint32_t RS_NET_CONN_QUALITY_MASK			= 0x00f00000;
const uint32_t RS_NET_CONN_QUALITY_UNKNOWN		= 0x00000000;

// THIS INFO MUST BE SUPPLIED BY PEERMGR....
const uint32_t RS_NET_CONN_TYPE_MASK			= 0x0f000000;
const uint32_t RS_NET_CONN_TYPE_UNKNOWN			= 0x00000000;
const uint32_t RS_NET_CONN_TYPE_ACQUAINTANCE		= 0x01000000;
const uint32_t RS_NET_CONN_TYPE_FRIEND			= 0x02000000;
const uint32_t RS_NET_CONN_TYPE_SERVER			= 0x04000000;
const uint32_t RS_NET_CONN_TYPE_CLIENT			= 0x08000000;

// working state of proxy

const uint32_t RS_NET_PROXY_STATUS_UNKNOWN  = 0x0000 ;
const uint32_t RS_NET_PROXY_STATUS_OK  	    = 0x0001 ;

// Potential certificate parsing errors.


/* Groups */
#define RS_GROUP_ID_FRIENDS    "Friends"
#define RS_GROUP_ID_FAMILY     "Family"
#define RS_GROUP_ID_COWORKERS  "Co-Workers"
#define RS_GROUP_ID_OTHERS     "Other Contacts"
#define RS_GROUP_ID_FAVORITES  "Favorites"

const uint32_t RS_GROUP_FLAG_STANDARD = 0x0001;

/* A couple of helper functions for translating the numbers games */

std::string RsPeerTrustString(uint32_t trustLvl);
std::string RsPeerNetModeString(uint32_t netModel);
std::string RsPeerLastConnectString(uint32_t lastConnect);


/* Details class */
class RsPeerDetails
{
	public:

	RsPeerDetails();

	/* Auth details */
	bool isOnlyGPGdetail;
	RsPeerId id;
	RsPgpId gpg_id;

	std::string name;
	std::string email;
	std::string location;
	std::string org;
	
	RsPgpId issuer;

	PGPFingerprintType fpr; /* pgp fingerprint */
	std::string authcode; 	// (cyril) what is this used for ?????
	std::list<RsPgpId> gpgSigners;

	uint32_t trustLvl;
	uint32_t validLvl;

	bool ownsign; /* we have signed the remote peer GPG key */
	bool hasSignedMe; /* the remote peer has signed my GPG key */

	bool accept_connection;

    /* Peer permission flags. What services the peer can use (Only valid if friend).*/
    ServicePermissionFlags service_perm_flags ;

    /* Network details (only valid if friend) */
	uint32_t	state;
	bool		actAsServer;

	std::string				connectAddr ; // current address if connected.
	uint16_t				connectPort ;

	// Hidden Node details.
	bool 			isHiddenNode;
	std::string		hiddenNodeAddress;
	uint16_t		hiddenNodePort;

	// Filled in for Standard Node.
	std::string             localAddr;
	uint16_t                localPort;
	std::string             extAddr;
	uint16_t                extPort;
	std::string             dyndns;
	std::list<std::string>  ipAddressList;

	uint32_t		netMode;
	/* vis State */
	uint16_t		vs_disc;
	uint16_t		vs_dht;

	/* basic stats */
	uint32_t		lastConnect;           /* how long ago */
	uint32_t		lastUsed;              /* how long ago since last used: signature verif, connect attempt, etc */
	uint32_t		connectState;          /* RS_PEER_CONNECTSTATE_... */
	std::string		connectStateString; /* Additional string like ip address */
	uint32_t		connectPeriod;
	bool			foundDHT;

	/* have we been denied */
	bool			wasDeniedConnection;
	time_t			deniedTS;

	/* linkType */
	uint32_t		linkType;
};

// This class is used to get info about crytographic algorithms used with a
// particular peer.
//
class RsPeerCryptoParams
{
	public:
		int         connexion_state ;
		std::string cipher_name ; 
		int         cipher_bits_1 ; 
		int         cipher_bits_2 ; 
		std::string cipher_version ; 
};

class RsGroupInfo
{
public:
	RsGroupInfo();

	std::string id;
	std::string name;
	uint32_t    flag;

    std::set<RsPgpId> peerIds;
};

std::ostream &operator<<(std::ostream &out, const RsPeerDetails &detail);

class RsPeers 
{
	public:

		RsPeers()  { return; }
		virtual ~RsPeers() { return; }

		/* Updates ... */
        // not implemented
        //virtual bool FriendsChanged() 					= 0;
        //virtual bool OthersChanged() 					= 0;

		/* Peer Details (Net & Auth) */
		virtual const RsPeerId& getOwnId()					= 0;

		virtual bool   haveSecretKey(const RsPgpId& gpg_id) = 0 ;

		virtual bool	getOnlineList(std::list<RsPeerId> &ssl_ids)	= 0;
		virtual bool	getFriendList(std::list<RsPeerId> &ssl_ids)	= 0;
		virtual bool    getPeerCount (unsigned int *pnFriendCount, unsigned int *pnnOnlineCount, bool ssl) = 0;

		virtual bool    isOnline(const RsPeerId &ssl_id)			= 0;
		virtual bool    isFriend(const RsPeerId &ssl_id)			= 0;
		virtual bool    isGPGAccepted(const RsPgpId &gpg_id_is_friend)			= 0; //
		virtual std::string getPeerName(const RsPeerId &ssl_id)			= 0;
		virtual std::string getGPGName(const RsPgpId& gpg_id)	= 0;
		virtual bool	 getPeerDetails(const RsPeerId& ssl_id, RsPeerDetails &d) = 0; 
		virtual bool	 getGPGDetails(const RsPgpId& gpg_id, RsPeerDetails &d) = 0;

		/* Using PGP Ids */
		virtual const RsPgpId& getGPGOwnId()				= 0;
		virtual RsPgpId getGPGId(const RsPeerId& sslid)	= 0; //return the gpg id of the given ssl id
		virtual bool    isKeySupported(const RsPgpId& gpg_ids)   = 0;
		virtual bool    getGPGAcceptedList(std::list<RsPgpId> &gpg_ids)   = 0;
		virtual bool    getGPGSignedList(std::list<RsPgpId> &gpg_ids)   = 0;//friends that we accpet to connect with but we don't want to sign their gpg key
		virtual bool    getGPGValidList(std::list<RsPgpId> &gpg_ids)   = 0;
		virtual bool    getGPGAllList(std::list<RsPgpId> &gpg_ids) 	= 0;
        virtual bool    getAssociatedSSLIds(const RsPgpId& gpg_id, std::list<RsPeerId>& ids) = 0;
		virtual bool    gpgSignData(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen) = 0;

		/* Add/Remove Friends */
        virtual	bool addFriend(const RsPeerId &ssl_id, const RsPgpId &gpg_id,ServicePermissionFlags flags = RS_NODE_PERM_DEFAULT)    = 0;
		virtual	bool removeFriend(const RsPgpId& pgp_id)  			= 0;
		virtual bool removeFriendLocation(const RsPeerId& sslId) 			= 0;

		/* keyring management */
        virtual bool removeKeysFromPGPKeyring(const std::set<RsPgpId>& pgp_ids,std::string& backup_file,uint32_t& error_code)=0 ;

		/* Network Stuff */
		virtual	bool connectAttempt(const RsPeerId& ssl_id)			= 0;
		virtual bool setLocation(const RsPeerId &ssl_id, const std::string &location) = 0;//location is shown in the gui to differentiate ssl certs

		virtual bool setHiddenNode(const RsPeerId &id, const std::string &hidden_node_address) = 0;
		virtual bool setHiddenNode(const RsPeerId &id, const std::string &address, uint16_t port) = 0;

		virtual	bool setLocalAddress(const RsPeerId &ssl_id, const std::string &addr, uint16_t port) = 0;
		virtual	bool setExtAddress(  const RsPeerId &ssl_id, const std::string &addr, uint16_t port) = 0;
		virtual	bool setDynDNS(const RsPeerId &id, const std::string &addr) = 0;
		virtual	bool setNetworkMode(const RsPeerId &ssl_id, uint32_t netMode) 	= 0;
		virtual bool setVisState(const RsPeerId &ssl_id, uint16_t vs_disc, uint16_t vs_dht)	= 0;

        virtual bool getProxyServer(std::string &addr, uint16_t &port,uint32_t& status_flags) = 0;
		virtual bool setProxyServer(const std::string &addr, const uint16_t port) = 0;

		virtual void getIPServersList(std::list<std::string>& ip_servers) = 0;
		virtual void allowServerIPDetermination(bool) = 0;
        virtual bool resetOwnExternalAddressList() = 0;
        virtual bool getAllowServerIPDetermination() = 0 ;

		/* Auth Stuff */
		virtual	std::string GetRetroshareInvite(const RsPeerId& ssl_id,bool include_signatures) 			= 0;
		virtual	std::string getPGPKey(const RsPgpId& pgp_id,bool include_signatures) 			= 0;
		virtual bool GetPGPBase64StringAndCheckSum(const RsPgpId& gpg_id,std::string& gpg_base64_string,std::string& gpg_base64_checksum) = 0 ;
		virtual	std::string GetRetroshareInvite(bool include_signatures) 			= 0;
		virtual  bool hasExportMinimal() = 0 ;

		// Add keys to the keyring
		virtual	bool loadCertificateFromString(const std::string& cert, RsPeerId& ssl_id,RsPgpId& pgp_id, std::string& error_string)  = 0;

		// Gets the GPG details, but does not add the key to the keyring.
		virtual	bool loadDetailsFromStringCert(const std::string& certGPG, RsPeerDetails &pd,uint32_t& error_code) = 0;

		// Certificate utils
		virtual	bool cleanCertificate(const std::string &certstr, std::string &cleanCert,int& error_code) = 0;
		virtual	bool saveCertificateToFile(const RsPeerId& id, const std::string &fname)  = 0;
		virtual	std::string saveCertificateToString(const RsPeerId &id)  	= 0;

		virtual	bool signGPGCertificate(const RsPgpId &gpg_id)                   	= 0;
		virtual	bool trustGPGCertificate(const RsPgpId &gpg_id, uint32_t trustlvl) 	= 0;

		/* Group Stuff */
		virtual bool    addGroup(RsGroupInfo &groupInfo) = 0;
		virtual bool    editGroup(const std::string &groupId, RsGroupInfo &groupInfo) = 0;
		virtual bool    removeGroup(const std::string &groupId) = 0;
		virtual bool    getGroupInfo(const std::string &groupId, RsGroupInfo &groupInfo) = 0;
		virtual bool    getGroupInfoList(std::list<RsGroupInfo> &groupInfoList) = 0;
		// groupId == "" && assign == false -> remove from all groups
		virtual bool    assignPeerToGroup(const std::string &groupId, const RsPgpId& peerId, bool assign) = 0;
		virtual bool    assignPeersToGroup(const std::string &groupId, const std::list<RsPgpId> &peerIds, bool assign) = 0;

		/* Group sharing permission */

		// Given 
		// 	- the peer id
		// 	- the permission flags of a given hash, e.g. a combination of 
		// 			RS_DIR_FLAGS_NETWORK_WIDE_OTHERS, RS_DIR_FLAGS_NETWORK_WIDE_GROUPS, RS_DIR_FLAGS_BROWSABLE_OTHERS and RS_DIR_FLAGS_BROWSABLE_GROUPS
		// 	- the parent groups of the file
		//
		// ... computes the sharing file permission hint flags set for this peer, that is a combination of 
		// 		RS_FILE_HINTS_NETWORK_WIDE and RS_FILE_HINTS_BROWSABLE.
		//
		virtual FileSearchFlags computePeerPermissionFlags(const RsPeerId& peer_id,FileStorageFlags file_sharing_flags,const std::list<std::string>& file_parent_groups) = 0;

        /* Service permission flags */

        virtual ServicePermissionFlags servicePermissionFlags(const RsPgpId& gpg_id) = 0;
        virtual ServicePermissionFlags servicePermissionFlags(const RsPeerId& ssl_id) = 0;
        virtual void setServicePermissionFlags(const RsPgpId& gpg_id,const ServicePermissionFlags& flags) = 0;
};

#endif
