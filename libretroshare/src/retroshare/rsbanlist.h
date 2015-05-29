/*
 * libretroshare/src/services/p3banlist.h
 *
 * Exchange list of Peers for Banning / Whitelisting.
 *
 * Copyright 2011 by Robert Fernie.
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

#pragma once

#include "util/rsnet.h"

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

class BanListPeer
{
public:
    BanListPeer() ;


    struct sockaddr_storage addr;
    uint8_t masked_bytes ;		// 0 = []/32. 1=[]/24, 2=[]/16
    uint32_t reason; 			 // User, DHT
    uint32_t level; 			 // LOCAL, FRIEND, FoF.
    bool state ; 			// true=>active, false=>just stored but inactive
    int  connect_attempts ; 	// recorded by the BanList service
    time_t mTs;
    std::string comment ;		//
};

class RsBanList
{
public:
    virtual void enableIPFiltering(bool b) =0;
    virtual bool ipFilteringEnabled() =0;

    virtual void addIpRange(const struct sockaddr_storage& addr,int masked_bytes,const std::string& comment) =0;

    virtual bool isAddressAccepted(const struct sockaddr_storage& addr) =0;
    virtual void getListOfBannedIps(std::list<BanListPeer>& list) =0;

    virtual bool autoRangeEnabled() =0;
    virtual void enableAutoRange(bool b) =0 ;

    virtual int  autoRangeLimit() =0;
    virtual void setAutoRangeLimit(int n)=0;

    virtual void enableIPsFromFriends(bool b) =0;
    virtual bool IPsFromFriendsEnabled() =0;

    virtual void enableIPsFromDHT(bool b) =0;
    virtual bool iPsFromDHTEnabled() =0;

};



