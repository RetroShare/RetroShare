/*******************************************************************************
 * libretroshare/src/services: p3banlist.h                                     *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011 by Robert Fernie <retroshare@lunamutt.com>                   *
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
#ifndef SERVICE_RSBANLIST_HEADER
#define SERVICE_RSBANLIST_HEADER

#include <string>
#include <list>
#include <map>

#include "rsitems/rsbanlistitems.h"
#include "services/p3service.h"
#include "retroshare/rsbanlist.h"

class p3ServiceControl;
class p3NetMgr;

class BanList
{
	public:
	
	RsPeerId mPeerId; /* from */
	rstime_t mLastUpdate;
	std::map<struct sockaddr_storage, BanListPeer> mBanPeers;
};

/**
 * The RS BanList service.
 * Exchange list of Banned IPv4 addresses with peers.
 *
 * @warning IPv4 only, IPv6 not supported yet!
 */
class p3BanList: public RsBanList, public p3Service, public pqiNetAssistPeerShare, public p3Config /*, public pqiMonitor */
{
public:
    p3BanList(p3ServiceControl *sc, p3NetMgr *nm);
    virtual RsServiceInfo getServiceInfo();

	/***** overloaded from RsBanList *****/

	/// @see RsBanList
	virtual bool isAddressAccepted(
	        const sockaddr_storage& addr, uint32_t checking_flags,
	        uint32_t& check_result = RS_DEFAULT_STORAGE_PARAM(uint32_t)
	        ) override;

    virtual void getBannedIps(std::list<BanListPeer>& list) ;
    virtual void getWhiteListedIps(std::list<BanListPeer>& list) ;

	virtual bool addIpRange( const sockaddr_storage& addr, int masked_bytes,
	                         uint32_t list_type, const std::string& comment );
	virtual bool removeIpRange( const sockaddr_storage &addr, int masked_bytes,
	                            uint32_t list_type );

    virtual void enableIPFiltering(bool b) ;
    virtual bool ipFilteringEnabled() ;

    virtual bool autoRangeEnabled() { return mAutoRangeIps ; }
    virtual void enableAutoRange(bool b) ;

    virtual int  autoRangeLimit()   { return mAutoRangeLimit ; }
    virtual void setAutoRangeLimit(int b) ;

    virtual void enableIPsFromFriends(bool b) ;
    virtual bool IPsFromFriendsEnabled() { return mIPFriendGatheringEnabled ;}

    virtual void enableIPsFromDHT(bool b) ;
    virtual bool iPsFromDHTEnabled() { return mIPDHTGatheringEnabled ;}

    /***** overloaded from pqiNetAssistPeerShare *****/

	virtual void updatePeer( const RsPeerId& id, const sockaddr_storage &addr,
	                         int type, int reason, int time_stamp );

    /***********************  p3config  ******************************/
    virtual RsSerialiser *setupSerialiser();
    virtual bool saveList(bool &cleanup, std::list<RsItem *>& itemlist);
    virtual bool loadList(std::list<RsItem *>& load);

    /***** overloaded from p3Service *****/
    /*!
     * This retrieves all chat msg items and also (important!)
     * processes chat-status items that are in service item queue. chat msg item requests are also processed and not returned
     * (important! also) notifications sent to notify base  on receipt avatar, immediate status and custom status
     * : notifyCustomState, notifyChatStatus, notifyPeerHasNewAvatar
     * @see NotifyBase

     */
    virtual int   tick();
    virtual int   status();

    int     sendPackets();
    bool 	processIncoming();

	bool recvBanItem(RsBanListItem *item);
	bool addBanEntry( const RsPeerId &peerId,
	                  const sockaddr_storage &addr, int level, uint32_t reason,
	                  rstime_t time_stamp );
    void sendBanLists();
    int  sendBanSet(const RsPeerId& peerid);


    /*!
     * Interface stuff.
     */

    /*************** pqiMonitor callback ***********************/
    //virtual void statusChange(const std::list<pqipeer> &plist);


    /************* from p3Config *******************/
    //virtual RsSerialiser *setupSerialiser() ;
    //virtual bool saveList(bool& cleanup, std::list<RsItem*>&) ;
    //virtual void saveDone();
    //virtual bool loadList(std::list<RsItem*>& load) ;

private:
    void getDhtInfo() ;

    RsMutex mBanMtx;

    bool acceptedBanSet_locked(const BanListPeer &blp);
    bool acceptedBanRanges_locked(const BanListPeer &blp);
    void autoFigureOutBanRanges();
    int condenseBanSources_locked();
    int printBanSources_locked(std::ostream &out);
    int printBanSet_locked(std::ostream &out);
    bool isWhiteListed_locked(const sockaddr_storage &addr);

    p3ServiceControl *mServiceCtrl;
    //p3NetMgr *mNetMgr;
    rstime_t mSentListTime;
    std::map<RsPeerId, BanList> mBanSources;
    std::map<struct sockaddr_storage, BanListPeer> mBanSet;
    std::map<struct sockaddr_storage, BanListPeer> mBanRanges;
    std::map<struct sockaddr_storage, BanListPeer> mWhiteListedRanges;

    rstime_t mLastDhtInfoRequest ;

    uint32_t mAutoRangeLimit ;
    bool mAutoRangeIps ;

    bool mIPFilteringEnabled ;
    bool mIPFriendGatheringEnabled ;
    bool mIPDHTGatheringEnabled ;
};

#endif // SERVICE_RSBANLIST_HEADER

