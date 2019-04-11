/*******************************************************************************
 * libretroshare/src/retroshare: rsbanlist.h                                   *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2011 by Robert Fernie <retroshare@lunamutt.com>              *
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

#include "util/rsnet.h"
#include "util/rstime.h"

class RsBanList;
extern RsBanList *rsBanList ;

#define RSBANLIST_ORIGIN_UNKNOWN	0
#define RSBANLIST_ORIGIN_SELF		1
#define RSBANLIST_ORIGIN_FRIEND		2
#define RSBANLIST_ORIGIN_FOF		3

#define RSBANLIST_REASON_UNKNOWN	0
#define RSBANLIST_REASON_USER		1
#define RSBANLIST_REASON_DHT		2
#define RSBANLIST_REASON_AUTO_RANGE     3

// These are flags. Can be combined.

#define RSBANLIST_CHECKING_FLAGS_NONE   	0x00
#define RSBANLIST_CHECKING_FLAGS_BLACKLIST   	0x01
#define RSBANLIST_CHECKING_FLAGS_WHITELIST   	0x02

// These are not flags. Cannot be combined. Used to give the reson for acceptance/denial of connections.

#define RSBANLIST_CHECK_RESULT_UNKNOWN  	0x00
#define RSBANLIST_CHECK_RESULT_NOCHECK  	0x01
#define RSBANLIST_CHECK_RESULT_BLACKLISTED  	0x02
#define RSBANLIST_CHECK_RESULT_NOT_WHITELISTED  0x03
#define RSBANLIST_CHECK_RESULT_ACCEPTED     	0x04

#define RSBANLIST_TYPE_PEERLIST	  		1
#define RSBANLIST_TYPE_BLACKLIST		2
#define RSBANLIST_TYPE_WHITELIST		3

class RsTlvBanListEntry ;

class BanListPeer
{
public:
    BanListPeer() ;

    void toRsTlvBanListEntry(RsTlvBanListEntry& e) const ;
    void fromRsTlvBanListEntry(const RsTlvBanListEntry& e) ;

    struct sockaddr_storage addr;
    uint8_t masked_bytes ;		// 0 = []/32. 1=[]/24, 2=[]/16
    uint32_t reason; 			 // User, DHT
    uint32_t level; 			 // LOCAL, FRIEND, FoF.
    bool state ; 			// true=>active, false=>just stored but inactive
    int  connect_attempts ; 	// recorded by the BanList service
    rstime_t mTs;
    std::string comment ;		//
};

class RsBanList
{
public:
    virtual void enableIPFiltering(bool b) =0;
    virtual bool ipFilteringEnabled() =0;

    // addIpRange()/removeIpRange()
    // 	addr: 		full IPv4 address. Port is ignored.
    // 	masked_bytes: 	0=full IP, 1="/24", 2="/16"
    // 	list_type: 	RSBANLIST_TYPE_WHITELIST or RSBANLIST_TYPE_BLACKLIST
    // 	comment: 	anything, user-based.

    virtual bool addIpRange(const struct sockaddr_storage& addr,int masked_bytes,uint32_t list_type,const std::string& comment) =0;
    virtual bool removeIpRange(const struct sockaddr_storage& addr,int masked_bytes,uint32_t list_type) =0;

    // isAddressAccepted()
    // 	addr: 		full IPv4 address. Port is ignored.
    // 	checking flags: any combination of  RSBANLIST_CHECKING_FLAGS_BLACKLIST and RSBANLIST_CHECKING_FLAGS_WHITELIST
    // 	check_result: 	returned result of the check in RSBANLIST_CHECK_RESULT_*
    // 	returned value: true=address is accepted, false=address is rejected.

    virtual bool isAddressAccepted(const struct sockaddr_storage& addr,uint32_t checking_flags,uint32_t *check_result=NULL) =0;

    virtual void getBannedIps(std::list<BanListPeer>& list) =0;
    virtual void getWhiteListedIps(std::list<BanListPeer>& list) =0;

    virtual bool autoRangeEnabled() =0;
    virtual void enableAutoRange(bool b) =0 ;

    virtual int  autoRangeLimit() =0;
    virtual void setAutoRangeLimit(int n)=0;

    virtual void enableIPsFromFriends(bool b) =0;
    virtual bool IPsFromFriendsEnabled() =0;

    virtual void enableIPsFromDHT(bool b) =0;
    virtual bool iPsFromDHTEnabled() =0;

};



