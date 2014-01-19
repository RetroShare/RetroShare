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

#include "retroshare/rspeers.h"
class p3LinkMgr;
class p3PeerMgr;
class p3NetMgr;


class p3Peers: public RsPeers 
{
	public:

        p3Peers(p3LinkMgr *lm, p3PeerMgr *pm, p3NetMgr *nm);
virtual ~p3Peers() { return; }

	/* Updates ... */
virtual bool FriendsChanged();
virtual bool OthersChanged();

	/* Peer Details (Net & Auth) */
virtual std::string getOwnId();

		virtual bool   haveSecretKey(const std::string& gpg_id) ;


virtual bool	getOnlineList(std::list<std::string> &ids);
virtual bool	getFriendList(std::list<std::string> &ids);
//virtual bool	getOthersList(std::list<std::string> &ids);
virtual bool    getPeerCount (unsigned int *friendCount, unsigned int *onlineCount, bool ssl);

virtual bool    isOnline(const std::string &id);
virtual bool    isFriend(const std::string &id);
virtual bool    isGPGAccepted(const std::string &gpg_id_is_friend); //
virtual std::string getGPGName(const std::string &gpg_id);
virtual std::string getPeerName(const std::string &ssl_or_gpg_id);
virtual bool	getPeerDetails(const std::string &ssl_or_gpg_id, RsPeerDetails &d);

                /* Using PGP Ids */
virtual std::string getGPGOwnId();
virtual std::string getGPGId(const std::string &ssl_id);
virtual bool    isKeySupported(const std::string& ids);
virtual bool    getGPGAcceptedList(std::list<std::string> &ids);
virtual bool    getGPGSignedList(std::list<std::string> &ids);
virtual bool    getGPGValidList(std::list<std::string> &ids);
virtual bool    getGPGAllList(std::list<std::string> &ids);
virtual bool	getGPGDetails(const std::string &id, RsPeerDetails &d);
virtual bool	getAssociatedSSLIds(const std::string &gpg_id, std::list<std::string> &ids);
virtual bool    gpgSignData(const void *data, const uint32_t len, unsigned char *sign, unsigned int *signlen) ;

	/* Add/Remove Friends */
virtual	bool addFriend(const std::string &ssl_id, const std::string &gpg_id,ServicePermissionFlags flags = RS_SERVICE_PERM_ALL);
virtual	bool removeFriend(const std::string &ssl_or_gpgid);
virtual bool removeFriendLocation(const std::string &sslId);

		/* keyring management */
		virtual bool removeKeysFromPGPKeyring(const std::list<std::string>& pgp_ids,std::string& backup_file,uint32_t& error_code);

	/* Network Stuff */
virtual	bool connectAttempt(const std::string &id);
virtual bool setLocation(const std::string &ssl_id, const std::string &location);//location is shown in the gui to differentiate ssl certs
virtual bool setHiddenNode(const std::string &id, const std::string &hidden_node_address);
virtual bool setHiddenNode(const std::string &id, const std::string &address, uint16_t port);

virtual	bool setLocalAddress(const std::string &id, const std::string &addr, uint16_t port);
virtual	bool setExtAddress(const std::string &id, const std::string &addr, uint16_t port);
virtual	bool setDynDNS(const std::string &id, const std::string &dyndns);
virtual	bool setNetworkMode(const std::string &id, uint32_t netMode);
virtual bool setVisState(const std::string &id, uint16_t vs_disc, uint16_t vs_dht);

virtual bool getProxyServer(std::string &addr, uint16_t &port);
virtual bool setProxyServer(const std::string &addr, const uint16_t port);

virtual void getIPServersList(std::list<std::string>& ip_servers) ;
virtual void allowServerIPDetermination(bool) ;
virtual bool getAllowServerIPDetermination() ;

	/* Auth Stuff */
// Get the invitation (GPG cert + local/ext address + SSL id for the given peer)
virtual	std::string GetRetroshareInvite(const std::string& ssl_id,bool include_signatures);
// same but for own id
virtual	std::string GetRetroshareInvite(bool include_signatures);
virtual bool GetPGPBase64StringAndCheckSum(const std::string& gpg_id,std::string& gpg_base64_string,std::string& gpg_base64_checksum) ;

virtual bool hasExportMinimal() ;

virtual	bool loadCertificateFromString(const std::string& cert, std::string& ssl_id,std::string& pgp_id, std::string& error_string);
virtual	bool loadDetailsFromStringCert(const std::string &cert, RsPeerDetails &pd, uint32_t& error_code);

virtual	bool cleanCertificate(const std::string &certstr, std::string &cleanCert,int& error_code);
virtual	bool saveCertificateToFile(const std::string &id, const std::string &fname);
virtual	std::string saveCertificateToString(const std::string &id);

virtual	bool signGPGCertificate(const std::string &id);
virtual	bool trustGPGCertificate(const std::string &id, uint32_t trustlvl);

	/* Group Stuff */
virtual bool addGroup(RsGroupInfo &groupInfo);
virtual bool editGroup(const std::string &groupId, RsGroupInfo &groupInfo);
virtual bool removeGroup(const std::string &groupId);
virtual bool getGroupInfo(const std::string &groupId, RsGroupInfo &groupInfo);
virtual bool getGroupInfoList(std::list<RsGroupInfo> &groupInfoList);
virtual bool assignPeerToGroup(const std::string &groupId, const std::string &peerId, bool assign);
virtual bool assignPeersToGroup(const std::string &groupId, const std::list<std::string> &peerIds, bool assign);

virtual FileSearchFlags computePeerPermissionFlags(const std::string& peer_id,FileStorageFlags share_flags,const std::list<std::string>& parent_groups) ;

// service permission stuff

	virtual ServicePermissionFlags servicePermissionFlags(const std::string& gpg_id) ;
	virtual ServicePermissionFlags servicePermissionFlags_sslid(const std::string& ssl_id) ;
	virtual void setServicePermissionFlags(const std::string& gpg_id,const ServicePermissionFlags& flags) ;

	private:

	p3LinkMgr *mLinkMgr;
	p3PeerMgr *mPeerMgr;
	p3NetMgr *mNetMgr;
	
};

#endif
