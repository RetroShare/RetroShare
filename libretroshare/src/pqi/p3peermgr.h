/*******************************************************************************
 * libretroshare/src/pqi: p3peermgr.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2007-2011 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef MRK_PQI_PEER_MANAGER_HEADER
#define MRK_PQI_PEER_MANAGER_HEADER

#include <retroshare/rspeers.h>
#include "pqi/pqimonitor.h"
#include "pqi/pqiipset.h"
#include "pqi/pqiassist.h"

#include "pqi/p3cfgmgr.h"

#include "util/rsthreads.h"

/* RS_VIS_STATE -> specified in rspeers.h
 */

	/* Startup Modes (confirmed later) */
const uint32_t RS_NET_MODE_TRYMODE    =   0xff00;

const uint32_t RS_NET_MODE_TRY_EXT    =   0x0100;
const uint32_t RS_NET_MODE_TRY_UPNP   =   0x0200;
const uint32_t RS_NET_MODE_TRY_UDP    =   0x0400;
const uint32_t RS_NET_MODE_TRY_LOOPBACK = 0x0800;

	/* Actual State */
const uint32_t RS_NET_MODE_ACTUAL =      0x00ff;

const uint32_t RS_NET_MODE_UNKNOWN =     0x0000;
const uint32_t RS_NET_MODE_EXT =         0x0001;
const uint32_t RS_NET_MODE_UPNP =        0x0002;
const uint32_t RS_NET_MODE_UDP =         0x0004;
const uint32_t RS_NET_MODE_HIDDEN =      0x0008;
const uint32_t RS_NET_MODE_UNREACHABLE = 0x0010;


/* flags of peerStatus */
const uint32_t RS_NET_FLAGS_USE_DISC		= 0x0001;
const uint32_t RS_NET_FLAGS_USE_DHT		= 0x0002;
const uint32_t RS_NET_FLAGS_ONLINE		= 0x0004;
const uint32_t RS_NET_FLAGS_EXTERNAL_ADDR	= 0x0008;
const uint32_t RS_NET_FLAGS_STABLE_UDP		= 0x0010;
const uint32_t RS_NET_FLAGS_TRUSTS_ME 		= 0x0020;

/*
 * remove locations offline since 90 days
 * stopt sending locations via discovery when offline for +30 days
 */
const rstime_t RS_PEER_OFFLINE_DELETE  = (90 * 24 * 3600);
const rstime_t RS_PEER_OFFLINE_NO_DISC = (30 * 24 * 3600);

class peerState
{
	public:
	peerState(); /* init */

	RsPeerId id;
	RsPgpId gpg_id;

    // This flag is used when adding a single SSL cert as friend without adding its PGP key in the friend list. This allows to
    // have short invites. However, because this represent a significant security risk, we perform multiple consistency checks
    // whenever we use this flag, in particular:
    //    flat is true  <==>   friend SSL cert is in the friend list, but PGP id is not in the friend list
    //                         PGP id is undefined and therefore set to null

    bool skip_pgp_signature_validation;

	uint32_t netMode; /* EXT / UPNP / UDP / HIDDEN / INVALID */
	/* visState */
	uint16_t vs_disc;
	uint16_t vs_dht;

        struct sockaddr_storage localaddr;
        struct sockaddr_storage serveraddr;
        std::string dyndns;

        rstime_t lastcontact;

	/* list of addresses from various sources */
	pqiIpAddrSet ipAddrs;

	bool hiddenNode; /* all IP addresses / dyndns must be blank */
	std::string hiddenDomain;
	uint16_t    hiddenPort;
	uint32_t    hiddenType;

	std::string location;
	std::string name;

    	uint32_t maxUpRate ;
    	uint32_t maxDnRate ;
};

class RsNodeGroupItem;
struct RsGroupInfo;

std::string textPeerState(peerState &state);

class p3LinkMgr;
class p3NetMgr;

class p3LinkMgrIMPL;
class p3NetMgrIMPL;

class p3PeerMgr
{
public:

	p3PeerMgr() {}
	virtual ~p3PeerMgr() {}

	virtual bool addFriend( const RsPeerId &ssl_id, const RsPgpId &gpg_id,
	                        uint32_t netMode = RS_NET_MODE_UDP,
	                        uint16_t vsDisc = RS_VS_DISC_FULL,
	                        uint16_t vsDht = RS_VS_DHT_FULL,
	                        rstime_t lastContact = 0,
	                        ServicePermissionFlags = ServicePermissionFlags(RS_NODE_PERM_DEFAULT) ) = 0;

	virtual bool addSslOnlyFriend(
	        const RsPeerId& sslId,
	        const RsPgpId& pgpId,
	        const RsPeerDetails& details = RsPeerDetails() ) = 0;

	virtual bool removeFriend(const RsPeerId &ssl_id, bool removePgpId) = 0;
	virtual bool isFriend(const RsPeerId& ssl_id) = 0;
    virtual bool isSslOnlyFriend(const RsPeerId &ssl_id)=0;

virtual bool 	getAssociatedPeers(const RsPgpId &gpg_id, std::list<RsPeerId> &ids) = 0;
virtual bool 	removeAllFriendLocations(const RsPgpId &gpgid) = 0;


	/******************** Groups **********************/
	/* This is solely used by p3peers - makes sense   */

virtual bool    addGroup(RsGroupInfo &groupInfo) = 0;
virtual bool    editGroup(const RsNodeGroupId &groupId, RsGroupInfo &groupInfo) = 0;
virtual bool    removeGroup(const RsNodeGroupId &groupId) = 0;
virtual bool    getGroupInfo(const RsNodeGroupId &groupId, RsGroupInfo &groupInfo) = 0;
virtual bool    getGroupInfoByName(const std::string& groupName, RsGroupInfo &groupInfo) = 0;
virtual bool    getGroupInfoList(std::list<RsGroupInfo> &groupInfoList) = 0;
virtual bool    assignPeersToGroup(const RsNodeGroupId &groupId, const std::list<RsPgpId> &peerIds, bool assign) = 0;

	virtual bool resetOwnExternalAddressList() = 0 ;

	virtual ServicePermissionFlags servicePermissionFlags(const RsPgpId& gpg_id) =0;
	virtual ServicePermissionFlags servicePermissionFlags(const RsPeerId& ssl_id) =0;
	virtual void setServicePermissionFlags(const RsPgpId& gpg_id,const ServicePermissionFlags& flags) =0;

	/**************** Set Net Info ****************/
	/*
	 * These functions are used by:
	 * 1) p3linkmgr 
	 * 2) p3peers - reasonable
	 * 3) p3disc  - reasonable
	 */

	virtual bool addPeerLocator(const RsPeerId &ssl_id, const RsUrl& locator) = 0;
virtual bool 	setLocalAddress(const RsPeerId &id, const struct sockaddr_storage &addr) = 0;
virtual bool 	setExtAddress(const RsPeerId &id, const struct sockaddr_storage &addr) = 0;
virtual bool    setDynDNS(const RsPeerId &id, const std::string &dyndns) = 0;
virtual bool 	addCandidateForOwnExternalAddress(const RsPeerId& from, const struct sockaddr_storage &addr) = 0;
virtual bool 	getExtAddressReportedByFriends(struct sockaddr_storage& addr,uint8_t& isstable) = 0;

virtual bool 	setNetworkMode(const RsPeerId &id, uint32_t netMode) = 0;
virtual bool 	setVisState(const RsPeerId &id, uint16_t vs_disc, uint16_t vs_dht) = 0;

virtual bool    setLocation(const RsPeerId &pid, const std::string &location) = 0;
virtual bool    setHiddenDomainPort(const RsPeerId &id, const std::string &domain_addr, const uint16_t domain_port) = 0;
virtual bool    isHiddenNode(const RsPeerId& id) = 0 ;

virtual bool    updateCurrentAddress(const RsPeerId& id, const pqiIpAddress &addr) = 0;
virtual bool    updateLastContact(const RsPeerId& id) = 0;
virtual bool    updateAddressList(const RsPeerId& id, const pqiIpAddrSet &addrs) = 0;


		// THIS MUST ONLY BE CALLED BY NETMGR!!!!
virtual bool    UpdateOwnAddress(const struct sockaddr_storage &local_addr, const struct sockaddr_storage &ext_addr) = 0;

	/**************** Net Status Info ****************/
	/*
	 * MUST RATIONALISE THE DATA FROM THESE FUNCTIONS
	 * These functions are used by:
	 * 1) p3face-config ... to remove!
	 * 2) p3peers - reasonable
	 * 3) p3disc  - reasonable
	 */

virtual bool	getOwnNetStatus(peerState &state) = 0;
virtual bool	getFriendNetStatus(const RsPeerId &id, peerState &state) = 0;
virtual bool	getOthersNetStatus(const RsPeerId &id, peerState &state) = 0;

virtual bool    getPeerName(const RsPeerId &ssl_id, std::string &name) = 0;
virtual bool	getGpgId(const RsPeerId &sslId, RsPgpId &gpgId) = 0;
virtual uint32_t getConnectionType(const RsPeerId &sslId) = 0;

virtual bool    setProxyServerAddress(const uint32_t type, const struct sockaddr_storage &proxy_addr) = 0;
virtual bool    getProxyServerAddress(const uint32_t type, struct sockaddr_storage &proxy_addr) = 0;
virtual bool    getProxyServerStatus(const uint32_t type, uint32_t& status) = 0;
virtual bool    isHidden() = 0;
virtual bool    isHidden(const uint32_t type) = 0;
virtual bool    isHiddenPeer(const RsPeerId &ssl_id) = 0;
virtual bool    isHiddenPeer(const RsPeerId &ssl_id, const uint32_t type) = 0;
virtual bool    getProxyAddress(const RsPeerId &ssl_id, struct sockaddr_storage &proxy_addr, std::string &domain_addr, uint16_t &domain_port) = 0;
virtual uint32_t hiddenDomainToHiddenType(const std::string &domain) = 0;
virtual uint32_t getHiddenType(const RsPeerId &ssl_id) = 0;


virtual int 	getFriendCount(bool ssl, bool online) = 0;
virtual bool 	setMaxRates(const RsPgpId&  pid,uint32_t  maxUp,uint32_t  maxDn)=0;
virtual bool 	getMaxRates(const RsPgpId&  pid,uint32_t& maxUp,uint32_t& maxDn)=0;
virtual bool 	getMaxRates(const RsPeerId& pid,uint32_t& maxUp,uint32_t& maxDn)=0;

        /************* DEPRECIATED FUNCTIONS (TO REMOVE) ********/

		// Single Use Function... shouldn't be here. used by p3serverconfig.cc
virtual bool 	haveOnceConnected() = 0;

virtual bool   locked_computeCurrentBestOwnExtAddressCandidate(sockaddr_storage &addr, uint32_t &count)=0;

/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/
/*************************************************************************************************/


};


class p3PeerMgrIMPL: public p3PeerMgr, public p3Config
{
public:

    /************************************************************************************************/
    /* EXTERNAL INTERFACE */
    /************************************************************************************************/

    virtual bool addFriend(const RsPeerId&ssl_id, const RsPgpId&gpg_id, uint32_t netMode = RS_NET_MODE_UDP,
                              uint16_t vsDisc = RS_VS_DISC_FULL, uint16_t vsDht = RS_VS_DHT_FULL,
                              rstime_t lastContact = 0,ServicePermissionFlags = ServicePermissionFlags(RS_NODE_PERM_DEFAULT));

	bool addSslOnlyFriend(const RsPeerId& sslId, const RsPgpId &pgp_id, const RsPeerDetails& details = RsPeerDetails() ) override;

    virtual bool	removeFriend(const RsPeerId &ssl_id, bool removePgpId);
    virtual bool	removeFriend(const RsPgpId &pgp_id);

    virtual bool	isFriend(const RsPeerId &ssl_id);
    virtual bool	isSslOnlyFriend(const RsPeerId &ssl_id);

    virtual bool    getAssociatedPeers(const RsPgpId &gpg_id, std::list<RsPeerId> &ids);
    virtual bool    removeAllFriendLocations(const RsPgpId &gpgid);


    /******************** Groups **********************/
    /* This is solely used by p3peers - makes sense   */

    virtual bool    addGroup(RsGroupInfo &groupInfo);
    virtual bool    editGroup(const RsNodeGroupId &groupId, RsGroupInfo &groupInfo);
    virtual bool    removeGroup(const RsNodeGroupId &groupId);
    virtual bool    getGroupInfo(const RsNodeGroupId &groupId, RsGroupInfo &groupInfo);
    virtual bool    getGroupInfoByName(const std::string& groupName, RsGroupInfo &groupInfo) ;
    virtual bool    getGroupInfoList(std::list<RsGroupInfo> &groupInfoList);
    virtual bool    assignPeersToGroup(const RsNodeGroupId &groupId, const std::list<RsPgpId> &peerIds, bool assign);

    virtual ServicePermissionFlags servicePermissionFlags(const RsPgpId& gpg_id) ;
    virtual ServicePermissionFlags servicePermissionFlags(const RsPeerId& ssl_id) ;
    virtual void setServicePermissionFlags(const RsPgpId& gpg_id,const ServicePermissionFlags& flags) ;

    /**************** Set Net Info ****************/
    /*
     * These functions are used by:
     * 1) p3linkmgr
     * 2) p3peers - reasonable
     * 3) p3disc  - reasonable
     */

	virtual bool addPeerLocator(const RsPeerId &ssl_id, const RsUrl& locator);
    virtual bool 	setLocalAddress(const RsPeerId &id, const struct sockaddr_storage &addr);
    virtual bool 	setExtAddress(const RsPeerId &id, const struct sockaddr_storage &addr);
    virtual bool    setDynDNS(const RsPeerId &id, const std::string &dyndns);
    virtual bool 	addCandidateForOwnExternalAddress(const RsPeerId& from, const struct sockaddr_storage &addr) ;
    virtual bool 	getExtAddressReportedByFriends(struct sockaddr_storage& addr, uint8_t &isstable) ;

    virtual bool 	setNetworkMode(const RsPeerId &id, uint32_t netMode);
    virtual bool 	setVisState(const RsPeerId &id, uint16_t vs_disc, uint16_t vs_dht);

    virtual bool    setLocation(const RsPeerId &pid, const std::string &location);
    virtual bool    setHiddenDomainPort(const RsPeerId &id, const std::string &domain_addr, const uint16_t domain_port);
	virtual bool    isHiddenNode(const RsPeerId& id);

    virtual bool    updateCurrentAddress(const RsPeerId& id, const pqiIpAddress &addr);
    virtual bool    updateLastContact(const RsPeerId& id);
    virtual bool    updateAddressList(const RsPeerId& id, const pqiIpAddrSet &addrs);

    virtual bool resetOwnExternalAddressList() ;

    // THIS MUST ONLY BE CALLED BY NETMGR!!!!
    virtual bool    UpdateOwnAddress(const struct sockaddr_storage &local_addr, const struct sockaddr_storage &ext_addr);
    /**************** Net Status Info ****************/
    /*
     * MUST RATIONALISE THE DATA FROM THESE FUNCTIONS
     * These functions are used by:
     * 1) p3face-config ... to remove!
     * 2) p3peers - reasonable
     * 3) p3disc  - reasonable
     */

    virtual bool	getOwnNetStatus(peerState &state);
    virtual bool	getFriendNetStatus(const RsPeerId &id, peerState &state);
    virtual bool	getOthersNetStatus(const RsPeerId &id, peerState &state);

    virtual bool    getPeerName(const RsPeerId& ssl_id, std::string& name);
    virtual bool	getGpgId(const RsPeerId& sslId, RsPgpId& gpgId);
    virtual uint32_t getConnectionType(const RsPeerId& sslId);

    virtual bool    setProxyServerAddress(const uint32_t type, const struct sockaddr_storage &proxy_addr);
    virtual bool    getProxyServerAddress(const uint32_t type, struct sockaddr_storage &proxy_addr);
    virtual bool    getProxyServerStatus(const uint32_t type, uint32_t &proxy_status);
    virtual bool    isHidden();
    virtual bool    isHidden(const uint32_t type);
    virtual bool    isHiddenPeer(const RsPeerId &ssl_id);
    virtual bool    isHiddenPeer(const RsPeerId &ssl_id, const uint32_t type);
    virtual bool    getProxyAddress(const RsPeerId& ssl_id, struct sockaddr_storage &proxy_addr, std::string &domain_addr, uint16_t &domain_port);
    virtual uint32_t hiddenDomainToHiddenType(const std::string &domain);
    virtual uint32_t getHiddenType(const RsPeerId &ssl_id);

    virtual int 	getFriendCount(bool ssl, bool online);

    /************* DEPRECIATED FUNCTIONS (TO REMOVE) ********/

    // Single Use Function... shouldn't be here. used by p3serverconfig.cc
    virtual bool 	haveOnceConnected();

    virtual bool 	setMaxRates(const RsPgpId&  pid,uint32_t  maxUp,uint32_t  maxDn);
    virtual bool 	getMaxRates(const RsPgpId&  pid,uint32_t& maxUp,uint32_t& maxDn);
    virtual bool 	getMaxRates(const RsPeerId& pid,uint32_t& maxUp,uint32_t& maxDn);

    /************************************************************************************************/
    /* Extra IMPL Functions (used by p3LinkMgr, p3NetMgr + Setup) */
    /************************************************************************************************/

    p3PeerMgrIMPL(	const RsPeerId& ssl_own_id,
                    const RsPgpId& gpg_own_id,
                    const std::string& gpg_own_name,
                    const std::string& ssl_own_location) ;

    void	setManagers(p3LinkMgrIMPL *linkMgr, p3NetMgrIMPL *netMgr);

    bool 	forceHiddenNode();
    bool 	setupHiddenNode(const std::string &hiddenAddress, const uint16_t hiddenPort);

    void 	tick();

    const RsPeerId& getOwnId();
    bool 	setOwnNetworkMode(uint32_t netMode);
    bool 	setOwnVisState(uint16_t vs_disc, uint16_t vs_dht);

	int getConnectAddresses( const RsPeerId &id, sockaddr_storage &lAddr,
	                         sockaddr_storage &eAddr, pqiIpAddrSet &histAddrs,
	                         std::string &dyndns );


protected:
    /* Internal Functions */

    bool 	removeUnusedLocations();
    bool 	removeBannedIps();

    void    printPeerLists(std::ostream &out);

    virtual bool   locked_computeCurrentBestOwnExtAddressCandidate(sockaddr_storage &addr, uint32_t &count);

protected:
    /*****************************************************************/
    /***********************  p3config  ******************************/
    /* Key Functions to be overloaded for Full Configuration */
    virtual RsSerialiser *setupSerialiser();
    virtual bool saveList(bool &cleanup, std::list<RsItem *>&);
    virtual void saveDone();
    virtual bool    loadList(std::list<RsItem *>& load);
    /*****************************************************************/

    /* other important managers */

    p3LinkMgrIMPL *mLinkMgr;
    p3NetMgrIMPL  *mNetMgr;

private:
    RsMutex mPeerMtx; /* protects below */

    bool     mStatusChanged;

    std::list<pqiMonitor *> clients;

    peerState mOwnState;

    std::map<RsPeerId, peerState> mFriendList;	// <SSLid , peerState>
    std::map<RsPeerId, peerState> mOthersList;

    std::map<RsPeerId,sockaddr_storage> mReportedOwnAddresses ;

    std::map<RsNodeGroupId,RsGroupInfo> groupList;
    //uint32_t lastGroupId;

    std::list<RsItem *> saveCleanupList; /* TEMPORARY LIST WHEN SAVING */

    std::map<RsPgpId, ServicePermissionFlags> mFriendsPermissionFlags ; // permission flags for each gpg key
    std::map<RsPgpId, PeerBandwidthLimits> mPeerBandwidthLimits ; // bandwidth limits for each gpg key

    struct sockaddr_storage mProxyServerAddressTor;
    struct sockaddr_storage mProxyServerAddressI2P;
    uint32_t mProxyServerStatusTor ;
    uint32_t mProxyServerStatusI2P ;

};

#endif // MRK_PQI_PEER_MANAGER_HEADER
