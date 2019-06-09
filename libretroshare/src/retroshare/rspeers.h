/*******************************************************************************
 * libretroshare/src/retroshare: rspeers.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#pragma once

#include <inttypes.h>
#include <string>
#include <list>

#include "retroshare/rstypes.h"
#include "retroshare/rsfiles.h"
#include "retroshare/rsids.h"
#include "util/rsurl.h"
#include "util/rsdeprecate.h"
#include "util/rstime.h"
#include "retroshare/rsevents.h"

class RsPeers;

/**
 * Pointer to global instance of RsPeers service implementation
 * @jsonapi{development}
 */
extern RsPeers* rsPeers;

/* TODO: 2015/12/31 As for type safetyness all those constant must be declared as enum!
 * C++ now supports typed enum so there is no ambiguity in serialization size
 */

/* Trust Levels. Should be the same values than what is declared in PGPHandler.h */

const uint32_t RS_TRUST_LVL_UNDEFINED  = 0;
const uint32_t RS_TRUST_LVL_UNKNOWN    = 1;
const uint32_t RS_TRUST_LVL_NEVER      = 2;
const uint32_t RS_TRUST_LVL_MARGINAL   = 3;
const uint32_t RS_TRUST_LVL_FULL	      = 4;
const uint32_t RS_TRUST_LVL_ULTIMATE   = 5;


const uint32_t SELF_SIGNATURE_RESULT_PENDING = 0x00;
const uint32_t SELF_SIGNATURE_RESULT_SUCCESS = 0x01;
const uint32_t SELF_SIGNATURE_RESULT_FAILED  = 0x02;

/* Net Mode */
const uint32_t RS_NETMODE_UDP		= 0x0001;
const uint32_t RS_NETMODE_UPNP		= 0x0002;
const uint32_t RS_NETMODE_EXT		= 0x0003;
const uint32_t RS_NETMODE_HIDDEN	= 0x0004;
const uint32_t RS_NETMODE_UNREACHABLE	= 0x0005;

/* Hidden Type */
const uint32_t RS_HIDDEN_TYPE_NONE	= 0x0000;
const uint32_t RS_HIDDEN_TYPE_UNKNOWN	= 0x0001;
const uint32_t RS_HIDDEN_TYPE_TOR	= 0x0002;
const uint32_t RS_HIDDEN_TYPE_I2P	= 0x0004;
/* mask to match all valid hidden types */
const uint32_t RS_HIDDEN_TYPE_MASK	= RS_HIDDEN_TYPE_I2P | RS_HIDDEN_TYPE_TOR;

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
const ServicePermissionFlags RS_NODE_PERM_DEFAULT    =  RS_NODE_PERM_DIRECT_DL ;
const ServicePermissionFlags RS_NODE_PERM_ALL        =  RS_NODE_PERM_DIRECT_DL | RS_NODE_PERM_ALLOW_PUSH | RS_NODE_PERM_REQUIRE_WL;

// ...

/* Connect state */
const uint32_t RS_PEER_CONNECTSTATE_OFFLINE           = 0;
const uint32_t RS_PEER_CONNECTSTATE_TRYING_TCP        = 2;
const uint32_t RS_PEER_CONNECTSTATE_TRYING_UDP        = 3;
const uint32_t RS_PEER_CONNECTSTATE_CONNECTED_TCP     = 4;
const uint32_t RS_PEER_CONNECTSTATE_CONNECTED_UDP     = 5;
const uint32_t RS_PEER_CONNECTSTATE_CONNECTED_TOR     = 6;
const uint32_t RS_PEER_CONNECTSTATE_CONNECTED_I2P     = 7;
const uint32_t RS_PEER_CONNECTSTATE_CONNECTED_UNKNOWN = 8;

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
static const RsNodeGroupId RS_GROUP_ID_FRIENDS   ("00000000000000000000000000000001");
static const RsNodeGroupId RS_GROUP_ID_FAMILY    ("00000000000000000000000000000002");
static const RsNodeGroupId RS_GROUP_ID_COWORKERS ("00000000000000000000000000000003");
static const RsNodeGroupId RS_GROUP_ID_OTHERS    ("00000000000000000000000000000004");
static const RsNodeGroupId RS_GROUP_ID_FAVORITES ("00000000000000000000000000000005");

#define RS_GROUP_DEFAULT_NAME_FRIENDS    "Friends"
#define RS_GROUP_DEFAULT_NAME_FAMILY     "Family"
#define RS_GROUP_DEFAULT_NAME_COWORKERS  "Co-Workers"
#define RS_GROUP_DEFAULT_NAME_OTHERS     "Other Contacts"
#define RS_GROUP_DEFAULT_NAME_FAVORITES  "Favorites"

const uint32_t RS_GROUP_FLAG_STANDARD = 0x0001;

/* A couple of helper functions for translating the numbers games */

std::string RsPeerTrustString(uint32_t trustLvl);
std::string RsPeerNetModeString(uint32_t netModel);
std::string RsPeerLastConnectString(uint32_t lastConnect);


/* We should definitely split this into 2 sub-structures:
 *    PGP info (or profile info) with all info related to PGP keys
 *    peer info:  all network related information
 *
 *   Plus top level information:
 *    isOnlyPgpDetail  (this could be obsolete if the methods to query about PGP info is a different function)
 *    peer Id
 */
struct RsPeerDetails : RsSerializable
{
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
	std::string authcode; 	// TODO: 2015/12/31 (cyril) what is this used for ?????
	std::list<RsPgpId> gpgSigners;

	uint32_t trustLvl;
	uint32_t validLvl;

    bool skip_signature_validation;
	bool ownsign; /* we have signed the remote peer GPG key */
	bool hasSignedMe; /* the remote peer has signed my GPG key */

	bool accept_connection;

    /* Peer permission flags. What services the peer can use (Only valid if friend).*/
    ServicePermissionFlags service_perm_flags ;

    /* Network details (only valid if friend) */
	uint32_t state;
	bool actAsServer;

	// TODO: 2015/12/31 to take advantage of multiple connection this must be
	// replaced by a set of addresses
	std::string connectAddr ; // current address if connected.
	uint16_t connectPort ;

	// Hidden Node details.
	bool isHiddenNode;
	std::string hiddenNodeAddress;
	uint16_t hiddenNodePort;
	uint32_t hiddenType;

	// Filled in for Standard Node.
	std::string localAddr;
	uint16_t localPort;
	std::string extAddr;
	uint16_t extPort;
	std::string dyndns;
	std::list<std::string> ipAddressList;

	uint32_t netMode;
	/* vis State */
	uint16_t vs_disc;
	uint16_t vs_dht;

	/* basic stats */
	uint32_t lastConnect;           /* how long ago */
	uint32_t lastUsed;              /* how long ago since last used: signature verif, connect attempt, etc */
	uint32_t connectState;          /* RS_PEER_CONNECTSTATE_... */
	std::string connectStateString; /* Additional string like ip address */
	uint32_t connectPeriod;
	bool foundDHT;

	/* have we been denied */
	bool wasDeniedConnection;
	rstime_t deniedTS;

	/* linkType */
	uint32_t linkType;

	/// @see RsSerializable
	virtual void serial_process( RsGenericSerializer::SerializeJob j,
	                             RsGenericSerializer::SerializeContext& ctx )
	{
		RS_SERIAL_PROCESS(isOnlyGPGdetail);
		RS_SERIAL_PROCESS(id);
		RS_SERIAL_PROCESS(gpg_id);
		RS_SERIAL_PROCESS(name);
		RS_SERIAL_PROCESS(email);
		RS_SERIAL_PROCESS(location);
		RS_SERIAL_PROCESS(org);
		RS_SERIAL_PROCESS(issuer);
		RS_SERIAL_PROCESS(fpr);
		RS_SERIAL_PROCESS(authcode);
		RS_SERIAL_PROCESS(gpgSigners);
		RS_SERIAL_PROCESS(trustLvl);
		RS_SERIAL_PROCESS(validLvl);
		RS_SERIAL_PROCESS(ownsign);
		RS_SERIAL_PROCESS(hasSignedMe);
		RS_SERIAL_PROCESS(accept_connection);
		RS_SERIAL_PROCESS(service_perm_flags);
		RS_SERIAL_PROCESS(state);
		RS_SERIAL_PROCESS(actAsServer);
		RS_SERIAL_PROCESS(connectAddr);
		RS_SERIAL_PROCESS(connectPort);
		RS_SERIAL_PROCESS(isHiddenNode);
		RS_SERIAL_PROCESS(hiddenNodeAddress);
		RS_SERIAL_PROCESS(hiddenNodePort);
		RS_SERIAL_PROCESS(hiddenType);
		RS_SERIAL_PROCESS(localAddr);
		RS_SERIAL_PROCESS(localPort);
		RS_SERIAL_PROCESS(extAddr);
		RS_SERIAL_PROCESS(extPort);
		RS_SERIAL_PROCESS(dyndns);
		RS_SERIAL_PROCESS(ipAddressList);
		RS_SERIAL_PROCESS(netMode);
		RS_SERIAL_PROCESS(vs_disc);
		RS_SERIAL_PROCESS(vs_dht);
		RS_SERIAL_PROCESS(lastConnect);
		RS_SERIAL_PROCESS(lastUsed);
		RS_SERIAL_PROCESS(connectState);
		RS_SERIAL_PROCESS(connectStateString);
		RS_SERIAL_PROCESS(connectPeriod);
		RS_SERIAL_PROCESS(foundDHT);
		RS_SERIAL_PROCESS(wasDeniedConnection);
		RS_SERIAL_PROCESS(deniedTS);
		RS_SERIAL_PROCESS(linkType);
	}
};

// This class is used to get info about crytographic algorithms used with a
// particular peer.
struct RsPeerCryptoParams
{
	int connexion_state;
	std::string cipher_name;
};

struct RsGroupInfo : RsSerializable
{
    RsGroupInfo();

    RsNodeGroupId   id;
    std::string     name;
    uint32_t        flag;

    std::set<RsPgpId> peerIds;

	/// @see RsSerializable
	void serial_process(
	        RsGenericSerializer::SerializeJob j,
	        RsGenericSerializer::SerializeContext &ctx)
	{
		RS_SERIAL_PROCESS(id);
		RS_SERIAL_PROCESS(name);
		RS_SERIAL_PROCESS(flag);
		RS_SERIAL_PROCESS(peerIds);
	}
};

/** Event emitted when a peer change state */
struct RsPeerStateChangedEvent : RsEvent
{
	/// @param[in] sslId is of the peer which changed state
	RsPeerStateChangedEvent(RsPeerId sslId);

	/// Storage fot the id of the peer that changed state
	RsPeerId mSslId;

	void serial_process( RsGenericSerializer::SerializeJob j,
	                     RsGenericSerializer::SerializeContext& ctx) override
	{
		RsEvent::serial_process(j, ctx);
		RS_SERIAL_PROCESS(mSslId);
	}
};

/** The Main Interface Class - for information about your Peers
 * A peer is another RS instance, means associated with an SSL certificate
 * A same GPG person can have multiple peer running with different SSL certs
 * signed by the same GPG key
 * Thus a peer have SSL cert details, and also the parent GPG details
 */
class RsPeers
{
public:

	RsPeers() {}
	virtual ~RsPeers() {}

	/**
	 * @brief Get own SSL peer id
	 * @return own peer id
	 */
	virtual const RsPeerId& getOwnId() = 0;

	virtual bool haveSecretKey(const RsPgpId& gpg_id) = 0 ;

	/**
	 * @brief Get trusted peers list
	 * @jsonapi{development}
	 * @param[out] sslIds storage for the trusted peers
	 * @return false if error occurred, true otherwise
	 */
	virtual bool getFriendList(std::list<RsPeerId>& sslIds) = 0;

	/**
	 * @brief Get connected peers list
	 * @jsonapi{development}
	 * @param[out] sslIds storage for the peers
	 * @return false if error occurred, true otherwise
	 */
	virtual bool getOnlineList(std::list<RsPeerId> &sslIds) = 0;

	/**
	 * @brief Get peers count
	 * @jsonapi{development}
	 * @param[out] peersCount storage for trusted peers count
	 * @param[out] onlinePeersCount storage for online peers count
	 * @param[in] countLocations true to count multiple locations of same owner
	 * @return false if error occurred, true otherwise
	 */
	virtual bool getPeersCount(
	        uint32_t& peersCount, uint32_t& onlinePeersCount,
	        bool countLocations = true ) = 0;

	RS_DEPRECATED
	virtual bool getPeerCount(unsigned int *pnFriendCount, unsigned int *pnnOnlineCount, bool ssl) = 0;

	/**
	 * @brief Check if there is an established connection to the given peer
	 * @jsonapi{development}
	 * @param[in] sslId id of the peer to check
	 * @return true if the connection is establisced, false otherwise
	 */
	virtual bool isOnline(const RsPeerId &sslId) = 0;

	/**
	 * @brief Check if given peer is a trusted node
	 * @jsonapi{development}
	 * @param[in] sslId id of the peer to check
	 * @return true if the node is trusted, false otherwise
	 */
	virtual bool isFriend(const RsPeerId& sslId) = 0;

	/**
	 * @brief Check if given PGP id is trusted
	 * @jsonapi{development}
	 * @param[in] pgpId PGP id to check
	 * @return true if the PGP id is trusted, false otherwise
	 */
	virtual bool isPgpFriend(const RsPgpId& pgpId) = 0;

	/**
	 * @brief Check if given peer is a trusted SSL node pending PGP approval
	 * Peers added through short invite remain in this state as long as their
	 * PGP key is not received and verified/approved by the user.
	 * @jsonapi{development}
	 * @param[in] sslId id of the peer to check
	 * @return true if the node is trusted, false otherwise
	 */
	virtual bool isSslOnlyFriend(const RsPeerId& sslId) = 0;

	virtual std::string getPeerName(const RsPeerId &ssl_id) = 0;
	virtual std::string getGPGName(const RsPgpId& gpg_id) = 0;

	/**
	 * @brief Get details details of the given peer
	 * @jsonapi{development}
	 * @param[in] sslId id of the peer
	 * @param[out] det storage for the details of the peer
	 * @return false if error occurred, true otherwise
	 */
	virtual bool getPeerDetails(const RsPeerId& sslId, RsPeerDetails& det) = 0;

	virtual bool getGPGDetails(const RsPgpId& gpg_id, RsPeerDetails &d) = 0;

	/* Using PGP Ids */
	virtual const RsPgpId& getGPGOwnId() = 0;

	/**
	 * @brief Get PGP id for the given peer
	 * @jsonapi{development}
	 * @param[in] sslId SSL id of the peer
	 * @return PGP id of the peer
	 */
	virtual RsPgpId getGPGId(const RsPeerId& sslId) = 0;
	virtual bool isKeySupported(const RsPgpId& gpg_ids) = 0;
	virtual bool getGPGAcceptedList(std::list<RsPgpId> &gpg_ids) = 0;
	virtual bool getGPGSignedList(std::list<RsPgpId> &gpg_ids) = 0;//friends that we accpet to connect with but we don't want to sign their gpg key
	virtual bool getGPGValidList(std::list<RsPgpId> &gpg_ids) = 0;
	virtual bool getGPGAllList(std::list<RsPgpId> &gpg_ids) = 0;
	virtual bool getAssociatedSSLIds(const RsPgpId& gpg_id, std::list<RsPeerId>& ids) = 0;
	virtual bool gpgSignData(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen, std::string reason = "") = 0;

	/**
	 * @brief Add trusted node
	 * @jsonapi{development}
	 * @param[in] sslId SSL id of the node to add
	 * @param[in] gpgId PGP id of the node to add
	 * @param[in] flags service permissions flag
	 * @return false if error occurred, true otherwise
	 */
	virtual bool addFriend(
	        const RsPeerId& sslId, const RsPgpId& gpgId,
	        ServicePermissionFlags flags = RS_NODE_PERM_DEFAULT ) = 0;

	/**
	 * @brief Add SSL-only trusted node
	 * When adding an SSL-only node, it is authorized to connect. Every time a
	 * connection is established the user is notified about the need to verify
	 * the PGP fingerprint, until she does, at that point the node become a full
	 * SSL+PGP friend.
	 * @jsonapi{development}
	 * @param[in] sslId SSL id of the node to add
	 * @param[in] details Optional extra details known about the node to add
	 * @return false if error occurred, true otherwise
	 */
	virtual bool addSslOnlyFriend(
	        const RsPeerId& sslId,
	        const RsPgpId& pgp_id,
	        const RsPeerDetails& details = RsPeerDetails() ) = 0;

	/**
	 * @brief Revoke connection trust from to node
	 * @jsonapi{development}
	 * @param[in] pgpId PGP id of the node
	 * @return false if error occurred, true otherwise
	 */
	virtual bool removeFriend(const RsPgpId& pgpId) = 0;

	/**
	 * @brief Remove location of a trusted node, useful to prune old unused
	 *	locations of a trusted peer without revoking trust
	 * @jsonapi{development}
	 * @param[in] sslId SSL id of the location to remove
	 * @return false if error occurred, true otherwise
	 */
	virtual bool removeFriendLocation(const RsPeerId& sslId) = 0;

	/* keyring management */
	virtual bool removeKeysFromPGPKeyring(
	        const std::set<RsPgpId>& pgpIds, std::string& backupFile,
	        uint32_t& errorCode ) = 0;

	/* Network Stuff */

	/**
	 * @brief Trigger connection attempt to given node
	 * @jsonapi{development}
	 * @param[in] sslId SSL id of the node to connect
	 * @return false if error occurred, true otherwise
	 */
	virtual bool connectAttempt(const RsPeerId& sslId) = 0;

	virtual bool setLocation(const RsPeerId &ssl_id, const std::string &location) = 0; // location is shown in the gui to differentiate ssl certs

	virtual bool setHiddenNode(const RsPeerId &id, const std::string &hidden_node_address) = 0;
	virtual bool setHiddenNode(const RsPeerId &id, const std::string &address, uint16_t port) = 0;
	virtual bool isHiddenNode(const RsPeerId &id) = 0;

	/**
	 * @brief Add URL locator for given peer
	 * @jsonapi{development}
	 * @param[in] sslId SSL id of the peer, own id is accepted too
	 * @param[in] locator peer url locator
	 * @return false if error occurred, true otherwise
	 */
	virtual bool addPeerLocator(const RsPeerId& sslId, const RsUrl& locator) = 0;

	/**
	 * @brief Set local IPv4 address for the given peer
	 * @jsonapi{development}
	 * @param[in] sslId SSL id of the peer, own id is accepted too
	 * @param[in] addr string representation of the local IPv4 address
	 * @param[in] port local listening port
	 * @return false if error occurred, true otherwise
	 */
	virtual bool setLocalAddress(
	        const RsPeerId& sslId, const std::string& addr, uint16_t port ) = 0;

	/**
	 * @brief Set external IPv4 address for given peer
	 * @jsonapi{development}
	 * @param[in] sslId SSL id of the peer, own id is accepted too
	 * @param[in] addr string representation of the external IPv4 address
	 * @param[in] port external listening port
	 * @return false if error occurred, true otherwise
	 */
	virtual bool setExtAddress(
	        const RsPeerId& sslId, const std::string &addr, uint16_t port ) = 0;

	/**
	 * @brief Set (dynamical) domain name associated to the given peer
	 * @jsonapi{development}
	 * @param[in] sslId SSL id of the peer, own id is accepted too
	 * @param[in] addr domain name string representation
	 * @return false if error occurred, true otherwise
	 */
	virtual bool setDynDNS(const RsPeerId& sslId, const std::string& addr) = 0;

	/**
	 * @brief Set network mode of the given peer
	 * @jsonapi{development}
	 * @param[in] sslId SSL id of the peer, own id is accepted too
	 * @param[in] netMode one of RS_NETMODE_*
	 * @return false if error occurred, true otherwise
	 */
	virtual bool setNetworkMode(const RsPeerId &sslId, uint32_t netMode) = 0;

	/**
	 * @brief set DHT and discovery modes
	 * @jsonapi{development}
	 * @param[in] sslId SSL id of the peer, own id is accepted too
	 * @param[in] vsDisc one of RS_VS_DISC_*
	 * @param[in] vsDht one of RS_VS_DHT_*
	 * @return false if error occurred, true otherwise
	 */
	virtual bool setVisState( const RsPeerId& sslId,
	                          uint16_t vsDisc, uint16_t vsDht ) = 0;

	virtual bool getProxyServer(const uint32_t type, std::string &addr, uint16_t &port,uint32_t& status_flags) = 0;
	virtual bool setProxyServer(const uint32_t type, const std::string &addr, const uint16_t port) = 0;

	virtual void getIPServersList(std::list<std::string>& ip_servers) = 0;
	virtual void allowServerIPDetermination(bool) = 0;
	virtual bool resetOwnExternalAddressList() = 0;
	virtual bool getAllowServerIPDetermination() = 0 ;

	/**
	 * @brief Get RetroShare invite of the given peer
	 * @jsonapi{development}
	 * @param[in] sslId Id of the peer of which we want to generate an invite,
	 *	a null id (all 0) is passed, an invite for own node is returned.
	 * @param[in] includeSignatures true to add key signatures to the invite
	 * @param[in] includeExtraLocators false to avoid to add extra locators
	 * @return invite string
	 */
	virtual std::string GetRetroshareInvite(
	        const RsPeerId& sslId = RsPeerId(),
	        bool includeSignatures = false,
	        bool includeExtraLocators = true ) = 0;

	/**
	 * @brief Get RetroShare short invite of the given peer
	 * @jsonapi{development}
	 * @param[out] invite storage for the generated invite
	 * @param[in] sslId Id of the peer of which we want to generate an invite,
	 *	a null id (all 0) is passed, an invite for own node is returned.
	 * @param[in] formatRadix true to get in base64 format false to get URL.
	 * @param[in] bareBones true to get smallest invite, which miss also
	 *	the information necessary to attempt an outgoing connection, but still
	 *	enough to accept an incoming one.
	 * @param[in] baseUrl URL into which to sneak in the RetroShare invite
	 *	radix, this is primarly useful to trick other applications into making
	 *	the invite clickable, or to disguise the RetroShare invite into a
	 *	"normal" looking web link. Used only if formatRadix is false.
	 * @return false if error occurred, true otherwise
	 */
	virtual bool getShortInvite(
	        std::string& invite, const RsPeerId& sslId = RsPeerId(),
	        bool formatRadix = false, bool bareBones = false,
	        const std::string& baseUrl = "https://retroshare.me/" ) = 0;

	/**
	 * @brief Parse the give short invite to extract contained information
	 * @jsonapi{development}
	 * @param[in] invite string containing the short invite to parse
	 * @param[out] details storage for the extracted information, consider it
	 *	valid only if the function return true
	 * @return false if error occurred, true otherwise
	 */
	virtual bool parseShortInvite(
	        const std::string& invite, RsPeerDetails& details ) = 0;

	/**
	 * @brief Add trusted node from invite
	 * @jsonapi{development}
	 * @param[in] invite invite string being it in cert or URL format
	 * @param[in] flags service permissions flag
	 * @return false if error occurred, true otherwise
	 */
	virtual bool acceptInvite(
	        const std::string& invite,
	        ServicePermissionFlags flags = RS_NODE_PERM_DEFAULT ) = 0;


	/* Auth Stuff */
	virtual	std::string getPGPKey(const RsPgpId& pgp_id,bool include_signatures) = 0;
	virtual bool GetPGPBase64StringAndCheckSum(const RsPgpId& gpg_id,std::string& gpg_base64_string,std::string& gpg_base64_checksum) = 0;
	virtual  bool hasExportMinimal() = 0;

	/**
	 * @brief Import certificate into the keyring
	 * @jsonapi{development}
	 * @param[in] cert string representation of the certificate
	 * @param[out] sslId storage for the SSL id of the certificate
	 * @param[out] pgpId storage for the PGP id of the certificate
	 * @param[out] errorString storage for the possible error string
	 * @return false if error occurred, true otherwise
	 */
	virtual bool loadCertificateFromString(
	        const std::string& cert, RsPeerId& sslId, RsPgpId& pgpId,
	        std::string& errorString) = 0;

	/**
	 * @brief Examine certificate and get details without importing into
	 *	the keyring
	 * @jsonapi{development}
	 * @param[in] cert string representation of the certificate
	 * @param[out] certDetails storage for the certificate details
	 * @param[out] errorCode storage for possible error number
	 * @return false if error occurred, true otherwise
	 */
	virtual bool loadDetailsFromStringCert(
	        const std::string& cert, RsPeerDetails& certDetails,
	        uint32_t& errorCode ) = 0;

	// Certificate utils
	virtual	bool cleanCertificate(const std::string &certstr, std::string &cleanCert,bool& is_short_format,int& error_code) = 0;
	virtual	bool saveCertificateToFile(const RsPeerId& id, const std::string &fname) = 0;
	virtual	std::string saveCertificateToString(const RsPeerId &id) = 0;

	virtual	bool signGPGCertificate(const RsPgpId &gpg_id) = 0;
	virtual	bool trustGPGCertificate(const RsPgpId &gpg_id, uint32_t trustlvl) = 0;

	/* Group Stuff */
	/**
	 * @brief addGroup create a new group
	 * @jsonapi{development}
	 * @param[in] groupInfo
	 * @return
	 */
    virtual bool addGroup(RsGroupInfo& groupInfo) = 0;

	/**
	 * @brief editGroup edit an existing group
	 * @jsonapi{development}
	 * @param[in] groupId
	 * @param[in] groupInfo
	 * @return
	 */
    virtual bool editGroup(const RsNodeGroupId& groupId, RsGroupInfo& groupInfo) = 0;

	/**
	 * @brief removeGroup remove a group
	 * @jsonapi{development}
	 * @param[in] groupId
	 * @return
	 */
    virtual bool removeGroup(const RsNodeGroupId& groupId) = 0;

	/**
	 * @brief getGroupInfo get group information to one group
	 * @jsonapi{development}
	 * @param[in] groupId
	 * @param[out] groupInfo
	 * @return
	 */
    virtual bool getGroupInfo(const RsNodeGroupId& groupId, RsGroupInfo& groupInfo) = 0;

	/**
	 * @brief getGroupInfoByName get group information by group name
	 * @jsonapi{development}
	 * @param[in] groupName
	 * @param[out] groupInfo
	 * @return
	 */
	virtual bool getGroupInfoByName(const std::string& groupName, RsGroupInfo& groupInfo) = 0;

	/**
	 * @brief getGroupInfoList get list of all groups
	 * @jsonapi{development}
	 * @param[out] groupInfoList
	 * @return
	 */
    virtual bool getGroupInfoList(std::list<RsGroupInfo>& groupInfoList) = 0;

	// groupId == "" && assign == false -> remove from all groups
	/**
	 * @brief assignPeerToGroup add a peer to a group
	 * @jsonapi{development}
	 * @param[in] groupId
	 * @param[in] peerId
	 * @param[in] assign true to assign a peer, false to remove a peer
	 * @return
	 */
    virtual bool assignPeerToGroup(const RsNodeGroupId& groupId, const RsPgpId& peerId, bool assign) = 0;

	/**
	 * @brief assignPeersToGroup add a list of peers to a group
	 * @jsonapi{development}
	 * @param[in] groupId
	 * @param[in] peerIds
	 * @param[in] assign true to assign a peer, false to remove a peer
	 * @return
	 */
    virtual bool assignPeersToGroup(const RsNodeGroupId& groupId, const std::list<RsPgpId>& peerIds, bool assign) = 0;

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
	virtual FileSearchFlags computePeerPermissionFlags(
			const RsPeerId& peer_id, FileStorageFlags file_sharing_flags,
            const std::list<RsNodeGroupId>& file_parent_groups) = 0;

	/* Service permission flags */

	virtual ServicePermissionFlags servicePermissionFlags(const RsPgpId& gpg_id) = 0;
	virtual ServicePermissionFlags servicePermissionFlags(const RsPeerId& ssl_id) = 0;
	virtual void setServicePermissionFlags(const RsPgpId& gpg_id,const ServicePermissionFlags& flags) = 0;
    
    	virtual bool setPeerMaximumRates(const RsPgpId& pid,uint32_t maxUploadRate,uint32_t maxDownloadRate) =0;
    	virtual bool getPeerMaximumRates(const RsPeerId& pid,uint32_t& maxUploadRate,uint32_t& maxDownloadRate) =0;
    	virtual bool getPeerMaximumRates(const RsPgpId& pid,uint32_t& maxUploadRate,uint32_t& maxDownloadRate) =0;

	RS_DEPRECATED_FOR(isPgpFriend)
	virtual bool isGPGAccepted(const RsPgpId &gpg_id_is_friend) = 0;
};
