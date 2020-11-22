/*******************************************************************************
 * libretroshare/src/rsserver: p3peers.h                                       *
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
#ifndef RETROSHARE_P3_PEER_INTERFACE_H
#define RETROSHARE_P3_PEER_INTERFACE_H
/* get OS-specific definitions for:
 * 	struct sockaddr_storage
 */
#ifndef WINDOWS_SYS
	#include <sys/socket.h>
#else
	#include <winsock2.h>
#endif

#include "retroshare/rspeers.h"
#include "util/rsurl.h"
#include "util/rsdeprecate.h"
#include "util/rsdebug.h"

class p3LinkMgr;
class p3PeerMgr;
class p3NetMgr;


class p3Peers: public RsPeers 
{
public:

	p3Peers(p3LinkMgr *lm, p3PeerMgr *pm, p3NetMgr *nm);
	virtual ~p3Peers() {}

	/* Updates ... */
	virtual bool FriendsChanged(bool add);
	virtual bool OthersChanged();

	/* Peer Details (Net & Auth) */
    virtual const RsPeerId& getOwnId() override;

    virtual bool haveSecretKey(const RsPgpId& gpg_id)  override;

    virtual bool getOnlineList(std::list<RsPeerId> &ids) override;
    virtual bool getFriendList(std::list<RsPeerId> &ids) override;
	virtual bool getPeersCount(
	        uint32_t& peersCount, uint32_t& onlinePeersCount,
            bool countLocations ) override;

	RS_DEPRECATED
    virtual bool getPeerCount (unsigned int *friendCount, unsigned int *onlineCount, bool ssl) override;

    virtual bool isOnline(const RsPeerId &id) override;
    virtual bool isFriend(const RsPeerId &id) override;
    virtual bool isPgpFriend(const RsPgpId& pgpId) override;

	/// @see RsPeers
	bool isSslOnlyFriend(const RsPeerId& sslId) override;

	RS_DEPRECATED_FOR(isPgpFriend)
    virtual bool isGPGAccepted(const RsPgpId &gpg_id_is_friend) override;

    virtual std::string getGPGName(const RsPgpId &gpg_id) override;
    virtual std::string getPeerName(const RsPeerId& ssl_or_gpg_id) override;
    virtual bool getPeerDetails(const RsPeerId& ssl_or_gpg_id, RsPeerDetails &d) override;

	/* Using PGP Ids */
    virtual const RsPgpId& getGPGOwnId() override;
    virtual RsPgpId getGPGId(const RsPeerId &ssl_id) override;
    virtual bool isKeySupported(const RsPgpId& ids) override;

	/// @see RsPeers
	bool getPgpFriendList(std::vector<RsPgpId>& pgpIds) override;

	RS_DEPRECATED_FOR(getPgpFriendList)
    virtual bool getGPGAcceptedList(std::list<RsPgpId> &ids) override;
    virtual bool getGPGSignedList(std::list<RsPgpId> &ids) override;
    virtual bool getGPGValidList(std::list<RsPgpId> &ids) override;
    virtual bool getGPGAllList(std::list<RsPgpId> &ids) override;
    virtual bool getGPGDetails(const RsPgpId &id, RsPeerDetails &d) override;
    virtual bool getAssociatedSSLIds(const RsPgpId& gpg_id, std::list<RsPeerId> &ids) override;
    virtual bool gpgSignData(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen, std::string reason = "")  override;

	virtual RsPgpId pgpIdFromFingerprint(const RsPgpFingerprint& fpr) override;

	/* Add/Remove Friends */
    virtual	bool addFriend(const RsPeerId &ssl_id, const RsPgpId &gpg_id,ServicePermissionFlags flags = RS_NODE_PERM_DEFAULT) override;

	/// @see RsPeers
	bool addSslOnlyFriend(
	            const RsPeerId& sslId,
	            const RsPgpId& pgp_id,
	            const RsPeerDetails& details = RsPeerDetails() ) override;

    virtual	bool removeFriend(const RsPgpId& gpgid) override;
    virtual bool removeFriendLocation(const RsPeerId& sslId) override;

	/* keyring management */
    virtual bool removeKeysFromPGPKeyring(const std::set<RsPgpId> &pgp_ids,std::string& backup_file,uint32_t& error_code) override;

	/* Network Stuff */
    virtual	bool connectAttempt(const RsPeerId &id) override;
    virtual bool setLocation(const RsPeerId &ssl_id, const std::string &location) override;//location is shown in the gui to differentiate ssl certs
    virtual bool setHiddenNode(const RsPeerId &id, const std::string &hidden_node_address) override;
    virtual bool setHiddenNode(const RsPeerId &id, const std::string &address, uint16_t port) override;
    virtual bool isHiddenNode(const RsPeerId &id) override;

    virtual bool addPeerLocator(const RsPeerId &ssl_id, const RsUrl& locator) override;
    virtual bool setLocalAddress(const RsPeerId &id, const std::string &addr, uint16_t port) override;
    virtual	bool setExtAddress(const RsPeerId &id, const std::string &addr, uint16_t port) override;
    virtual	bool setDynDNS(const RsPeerId &id, const std::string &dyndns) override;
    virtual	bool setNetworkMode(const RsPeerId &id, uint32_t netMode) override;
    virtual bool setVisState(const RsPeerId &id, uint16_t vs_disc, uint16_t vs_dht) override;

    virtual bool getProxyServer(const uint32_t type, std::string &addr, uint16_t &port,uint32_t& status) override;
    virtual bool setProxyServer(const uint32_t type, const std::string &addr, const uint16_t port) override;
	virtual bool isProxyAddress(const uint32_t type, const sockaddr_storage &addr);

    virtual void getIPServersList(std::list<std::string>& ip_servers) override;
    virtual void allowServerIPDetermination(bool) override;
    virtual bool getAllowServerIPDetermination() override;
    virtual bool resetOwnExternalAddressList() override;

	/* Auth Stuff */
	// Get the invitation (GPG cert + local/ext address + SSL id for the given peer)
    virtual	std::string GetRetroshareInvite(const RsPeerId& ssl_id = RsPeerId(), RetroshareInviteFlags invite_flags = RetroshareInviteFlags::DNS | RetroshareInviteFlags::CURRENT_IP ) override;

	RS_DEPRECATED /// @see RsPeers
	std::string getPGPKey(const RsPgpId& pgp_id,bool include_signatures) override;

	virtual bool GetPGPBase64StringAndCheckSum(const RsPgpId& gpg_id,std::string& gpg_base64_string,std::string& gpg_base64_checksum);

	/// @see RsPeers
    bool getShortInvite(std::string& invite, const RsPeerId& sslId = RsPeerId(),
            RetroshareInviteFlags invite_flags = RetroshareInviteFlags::CURRENT_IP | RetroshareInviteFlags::DNS,
            const std::string& baseUrl = "https://retroshare.me/" ) override;

	/// @see RsPeers
	bool parseShortInvite(const std::string& invite, RsPeerDetails& details, uint32_t &err_code ) override;

	/// @see RsPeers::acceptInvite
	virtual bool acceptInvite(
	        const std::string& invite,
            ServicePermissionFlags flags = RS_NODE_PERM_DEFAULT ) override;

    virtual	bool loadCertificateFromString(const std::string& cert, RsPeerId& ssl_id,RsPgpId& pgp_id, std::string& error_string) override;
    virtual bool loadPgpKeyFromBinaryData( const unsigned char *bin_key_data,uint32_t bin_key_len, RsPgpId& gpg_id, std::string& error_string ) override;
    virtual	bool loadDetailsFromStringCert(const std::string &cert, RsPeerDetails &pd, uint32_t& error_code) override;

	virtual	bool cleanCertificate(const std::string &certstr, std::string &cleanCert, bool &is_short_format, uint32_t& error_code) override;
    virtual	std::string saveCertificateToString(const RsPeerId &id) override;

    virtual	bool signGPGCertificate(const RsPgpId &id) override;
    virtual	bool trustGPGCertificate(const RsPgpId &id, uint32_t trustlvl) override;

	/* Group Stuff */
    virtual bool addGroup(RsGroupInfo &groupInfo) override;
    virtual bool editGroup(const RsNodeGroupId &groupId, RsGroupInfo &groupInfo) override;
    virtual bool removeGroup(const RsNodeGroupId &groupId) override;
    virtual bool getGroupInfo(const RsNodeGroupId &groupId, RsGroupInfo &groupInfo) override;
    virtual bool getGroupInfoByName(const std::string& groupName, RsGroupInfo& groupInfo) override;
    virtual bool getGroupInfoList(std::list<RsGroupInfo> &groupInfoList) override;
    virtual bool assignPeerToGroup(const RsNodeGroupId &groupId, const RsPgpId &peerId, bool assign) override;
    virtual bool assignPeersToGroup(const RsNodeGroupId &groupId, const std::list<RsPgpId>& peerIds, bool assign) override;

    virtual FileSearchFlags computePeerPermissionFlags(const RsPeerId& peer_id, FileStorageFlags share_flags, const std::list<RsNodeGroupId> &parent_groups) override;

	// service permission stuff

    virtual ServicePermissionFlags servicePermissionFlags(const RsPgpId& gpg_id) override;
    virtual ServicePermissionFlags servicePermissionFlags(const RsPeerId & ssl_id) override;
    virtual void setServicePermissionFlags(const RsPgpId& gpg_id,const ServicePermissionFlags& flags) override;

        virtual bool setPeerMaximumRates(const RsPgpId& pid,uint32_t maxUploadRate,uint32_t maxDownloadRate) override;
        virtual bool getPeerMaximumRates(const RsPgpId& pid,uint32_t& maxUploadRate,uint32_t& maxDownloadRate) override;
        virtual bool getPeerMaximumRates(const RsPeerId& pid,uint32_t& maxUploadRate,uint32_t& maxDownloadRate) override;
private:

	p3LinkMgr *mLinkMgr;
	p3PeerMgr *mPeerMgr;
	p3NetMgr *mNetMgr;

	RS_SET_CONTEXT_DEBUG_LEVEL(1)
};

#endif
