/*******************************************************************************
 * libretroshare/src/services: p3banlist.cc                                    *
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
#include "pqi/p3servicecontrol.h"
#include "pqi/p3netmgr.h"
#include "pqi/p3cfgmgr.h"

#include "util/rsnet.h"

#include "services/p3banlist.h"
#include "retroshare/rsdht.h"
#include "retroshare/rsbanlist.h"

#include "rsitems/rsbanlistitems.h"
#include "rsitems/rsconfigitems.h"

#include <sys/time.h>
#include <sstream>

/****
 * #define DEBUG_BANLIST		1
 ****/
// #define DEBUG_BANLIST		1
//#define DEBUG_BANLIST_CONDENSE		1


/* DEFINE INTERFACE POINTER! */
//RsBanList *rsBanList = NULL;

#define RSBANLIST_ENTRY_MAX_AGE				(60 * 60 * 24) // 24 HOURS
#define RSBANLIST_SEND_PERIOD				600	// 10 Minutes.

#define RSBANLIST_DELAY_BETWEEN_TALK_TO_DHT 		240	// every 4 mins.

/************ IMPLEMENTATION NOTES *********************************
 *
 * Get Bad Peers passed to us (from DHT mainly).
 * we distribute and track the network list of bad peers.
 *
 */
RsBanList *rsBanList = NULL ;

p3BanList::p3BanList(p3ServiceControl *sc, p3NetMgr */*nm*/)
  : p3Service(), mBanMtx("p3BanList"), mServiceCtrl(sc)
  , mSentListTime(0), mLastDhtInfoRequest(0)
  // default number of IPs in same range to trigger a complete IP /24 filter.
  , mAutoRangeLimit(2), mAutoRangeIps(true)
  , mIPFilteringEnabled(true)
  , mIPFriendGatheringEnabled(false)
  , mIPDHTGatheringEnabled(false)
{ addSerialType(new RsBanListSerialiser()); }

const std::string BANLIST_APP_NAME = "banlist";
const uint16_t BANLIST_APP_MAJOR_VERSION  =       1;
const uint16_t BANLIST_APP_MINOR_VERSION  =       0;
const uint16_t BANLIST_MIN_MAJOR_VERSION  =       1;
const uint16_t BANLIST_MIN_MINOR_VERSION  =       0;

RsServiceInfo p3BanList::getServiceInfo()
{
        return RsServiceInfo(RS_SERVICE_TYPE_BANLIST,
                BANLIST_APP_NAME,
                BANLIST_APP_MAJOR_VERSION,
                BANLIST_APP_MINOR_VERSION,
                BANLIST_MIN_MAJOR_VERSION,
                             BANLIST_MIN_MINOR_VERSION);
}

bool p3BanList::ipFilteringEnabled() { return mIPFilteringEnabled ; }
void p3BanList::enableIPFiltering(bool b) { mIPFilteringEnabled = b ; }
void p3BanList::enableIPsFromFriends(bool b) { mIPFriendGatheringEnabled = b; mLastDhtInfoRequest=0;}
void p3BanList::enableIPsFromDHT(bool b)
{
    mIPDHTGatheringEnabled = b;
    mLastDhtInfoRequest=0;

    IndicateConfigChanged();
}
void p3BanList::enableAutoRange(bool b)
{
    mAutoRangeIps = b;
    autoFigureOutBanRanges() ;

    IndicateConfigChanged();
}
void p3BanList::setAutoRangeLimit(int n)
{
    mAutoRangeLimit = n;
    autoFigureOutBanRanges();

    IndicateConfigChanged();
}

namespace services {

class ZeroedInt
{
    public:
        ZeroedInt() { n = 0 ; }
    uint32_t n ;
};

}

BanListPeer::BanListPeer()
{
    memset(&addr, 0, sizeof(addr));
    masked_bytes=0;
    reason=RSBANLIST_REASON_UNKNOWN ;
    level=RSBANLIST_ORIGIN_UNKNOWN ;
    state = true ;
    connect_attempts=0;
    mTs=0;
}

void BanListPeer::toRsTlvBanListEntry(RsTlvBanListEntry &e) const
{
    e.addr.addr = addr;
    e.masked_bytes = masked_bytes;
    e.reason = reason;
    e.level = level;
    e.comment = comment;
    e.age = time(NULL) - mTs;
}

void BanListPeer::fromRsTlvBanListEntry(const RsTlvBanListEntry &e)
{
    addr = e.addr.addr;
    masked_bytes = e.masked_bytes;		// 0 = []/32. 1=[]/24, 2=[]/16
    reason = e.reason; 			// User, DHT
    level = e.level; 			// LOCAL, FRIEND, FoF.
    state = true; 				// true=>active, false=>just stored but inactive
    connect_attempts = 0; 			// recorded by the BanList service
    comment = e.comment; 			// recorded by the BanList service
    mTs = time(NULL) - e.age;
}

static uint32_t getBitRange(const sockaddr_storage& addr)
{
    sockaddr_storage s ;
    sockaddr_storage_clear(s) ;
    sockaddr_storage_copyip(s,addr) ;

    sockaddr_in *ad = (sockaddr_in*)(&s) ;

    if( (ad->sin_addr.s_addr & 0xff000000) != 0xff000000)
        return 0 ;

    if( (ad->sin_addr.s_addr & 0x00ff0000) != 0x00ff0000)
        return 1 ;

    if( (ad->sin_addr.s_addr & 0x0000ff00) != 0x0000ff00)
        return 2 ;

    if( (ad->sin_addr.s_addr & 0x000000ff) != 0x000000ff)
        return 3 ;

    return 4 ;
}
static sockaddr_storage makeBitsRange(const sockaddr_storage& addr,int masked_bytes)
{
    sockaddr_storage s ;
    sockaddr_storage_clear(s) ;
    sockaddr_storage_copyip(s,addr) ;

    sockaddr_in *ad = (sockaddr_in*)(&s) ;

    if(masked_bytes == 1)
        ad->sin_addr.s_addr |= 0xff000000 ;
    else if(masked_bytes == 2)
        ad->sin_addr.s_addr |= 0xffff0000 ;
    else if(masked_bytes != 0)
        std::cerr << "Warning: unhandled mask size for IP range: " << masked_bytes << std::endl;

    return s ;
}

void p3BanList::autoFigureOutBanRanges()
{
    RS_STACK_MUTEX(mBanMtx) ;

    // clear automatic ban ranges

	for(std::map<sockaddr_storage,BanListPeer>::iterator it(mBanRanges.begin());
	    it!=mBanRanges.end(); )
	{
        if(it->second.reason == RSBANLIST_REASON_AUTO_RANGE)
        {
            std::map<sockaddr_storage,BanListPeer>::iterator it2=it ;
            ++it2 ;
            mBanRanges.erase(it) ;
            it=it2 ;
        }
		else ++it;
	}

    IndicateConfigChanged();

	if(!mAutoRangeIps) return;

#ifdef DEBUG_BANLIST
    std::cerr << "Automatically figuring out IP ranges from banned IPs." << std::endl;
#endif

    std::map<sockaddr_storage, services::ZeroedInt> range_map ;

    for(std::map<sockaddr_storage,BanListPeer>::iterator it(mBanSet.begin());it!=mBanSet.end();++it)
        ++range_map[makeBitsRange(it->first,1)].n ;

    rstime_t now = time(NULL) ;

    for(std::map<sockaddr_storage, services::ZeroedInt>::const_iterator it=range_map.begin();it!=range_map.end();++it)
    {
#ifdef DEBUG_BANLIST
        std::cerr << "Ban range: " << sockaddr_storage_iptostring(it->first) << " : " << it->second.n << std::endl;
#endif

        if(it->second.n >= mAutoRangeLimit)
        {
#ifdef DEBUG_BANLIST
           std::cerr << " --> creating new ban range." << std::endl;
#endif
           BanListPeer& peer(mBanRanges[it->first]) ;

           if (peer.reason == RSBANLIST_REASON_USER)
               continue;

           peer.addr = it->first ;
           peer.masked_bytes = 1 ;
           peer.reason = RSBANLIST_REASON_AUTO_RANGE ;
           peer.level = RSBANLIST_ORIGIN_SELF ;
           peer.state = true  ;
           peer.mTs = now ;
           peer.connect_attempts = 0 ;
           peer.connect_attempts = it->second.n;
        }
    }

    condenseBanSources_locked() ;
}

bool p3BanList::acceptedBanSet_locked(const BanListPeer& blp)
{
    //                         REASON_USER    REASON_DHT   REASON_AUTO
    //         ---------------------------------------------------------
    //   	       banDHT	      Y/Y/N         Y/Y/Y
    //         ---------------------------------------------------------
    //	   banFriends
    //         ---------------------------------------------------------
    //	banAutoRanges
    //

    if(blp.level > 1 && mIPFriendGatheringEnabled)
        return true ;

    switch(blp.reason)
    {
        case RSBANLIST_REASON_USER: 		return true ;

        case RSBANLIST_REASON_DHT:  		return mIPDHTGatheringEnabled && blp.level==1;

        default:
        case RSBANLIST_REASON_AUTO_RANGE:  	std::cerr << "(EE) Shouldn't find an AUTO RANGE in BanSet. Wrong call?" << std::endl;
                            return false ;
    }
    return false ;
}
bool p3BanList::acceptedBanRanges_locked(const BanListPeer& blp)
{
    //                         REASON_USER    REASON_DHT   REASON_AUTO
    //         ---------------------------------------------------------
    //   	       banDHT	      Y/Y/N         Y/Y/Y
    //         ---------------------------------------------------------
    //	   banFriends
    //         ---------------------------------------------------------
    //	banAutoRanges
    //

    switch(blp.reason)
    {
        case RSBANLIST_REASON_USER: 		return true ;

        default:
        case RSBANLIST_REASON_DHT:  		std::cerr << "(EE) Shouldn't find a DHT ip in BanRange. Wrong call?" << std::endl;
                            return false ;

        case RSBANLIST_REASON_AUTO_RANGE:  	return mAutoRangeIps ;
    }
    return false ;
}

bool p3BanList::isAddressAccepted(
        const sockaddr_storage& dAddr, uint32_t checking_flags,
        uint32_t& check_result )
{
	check_result = RSBANLIST_CHECK_RESULT_NOCHECK;

	sockaddr_storage addr; sockaddr_storage_copy(dAddr, addr);

	if(!sockaddr_storage_ipv6_to_ipv4(addr)) return true;
	if(sockaddr_storage_isLoopbackNet(addr)) return true;


	RS_STACK_MUTEX(mBanMtx);

	if(!mIPFilteringEnabled) return true;

#ifdef DEBUG_BANLIST
    std::cerr << "isAddressAccepted(): tested addr=" << sockaddr_storage_iptostring(addr) << ", checking flags=" << checking_flags ;
#endif

    // we should normally work this including entire ranges of IPs. For now, just check the exact IPs.

    sockaddr_storage addr_32 = makeBitsRange(addr,0) ;	// this is necessay because it cleans the address
    sockaddr_storage addr_24 = makeBitsRange(addr,1) ;
    sockaddr_storage addr_16 = makeBitsRange(addr,2) ;

    bool white_list_found = false ;

    white_list_found = white_list_found || (mWhiteListedRanges.find(addr_16) != mWhiteListedRanges.end()) ;
    white_list_found = white_list_found || (mWhiteListedRanges.find(addr_24) != mWhiteListedRanges.end()) ;
    white_list_found = white_list_found || (mWhiteListedRanges.find(addr_32) != mWhiteListedRanges.end()) ;

    if(white_list_found)
	{
		check_result = RSBANLIST_CHECK_RESULT_ACCEPTED;
#ifdef DEBUG_BANLIST
      std::cerr << ". Address is in whitelist. Accepting" << std::endl;
#endif
        return true ;
    }

    if(checking_flags & RSBANLIST_CHECKING_FLAGS_WHITELIST)
	{
		check_result = RSBANLIST_CHECK_RESULT_NOT_WHITELISTED;
#ifdef DEBUG_BANLIST
      std::cerr << ". Address is not whitelist, and whitelist is required. Rejecting" << std::endl;
#endif
        return false ;
    }

    if(!(checking_flags & RSBANLIST_CHECKING_FLAGS_BLACKLIST))
    {
#ifdef DEBUG_BANLIST
      std::cerr << ". No blacklisting required. Accepting." << std::endl;
#endif
	    check_result = RSBANLIST_CHECK_RESULT_ACCEPTED;
        return true;
    }

    std::map<sockaddr_storage,BanListPeer>::iterator it ;

    if(((it=mBanRanges.find(addr_16)) != mBanRanges.end()) && acceptedBanRanges_locked(it->second))
    {
        ++it->second.connect_attempts;
#ifdef DEBUG_BANLIST
      std::cerr << " found in blacklisted range " << sockaddr_storage_iptostring(it->first) << "/16. returning false. attempts=" << it->second.connect_attempts << std::endl;
#endif
	    check_result = RSBANLIST_CHECK_RESULT_BLACKLISTED;
      return false ;
  }

  if(((it=mBanRanges.find(addr_24)) != mBanRanges.end()) && acceptedBanRanges_locked(it->second))
  {
      ++it->second.connect_attempts;
#ifdef DEBUG_BANLIST
      std::cerr << " found in blacklisted range " << sockaddr_storage_iptostring(it->first) << "/24.  returning false. attempts=" << it->second.connect_attempts << std::endl;
#endif
	    check_result = RSBANLIST_CHECK_RESULT_BLACKLISTED;
      return false ;
  }

  if(((it=mBanRanges.find(addr_32)) != mBanRanges.end()) && acceptedBanRanges_locked(it->second))
  {
      ++it->second.connect_attempts;
#ifdef DEBUG_BANLIST
      std::cerr << " found in blacklisted range " << sockaddr_storage_iptostring(it->first) << "/32.  returning false. attempts=" << it->second.connect_attempts << std::endl;
#endif
	    check_result = RSBANLIST_CHECK_RESULT_BLACKLISTED;
        return false ;
    }

    if((it=mBanSet.find(addr_32)) != mBanSet.end() && acceptedBanSet_locked(it->second))
    {
        ++it->second.connect_attempts;
#ifdef DEBUG_BANLIST
      std::cerr << "found as blacklisted address " << sockaddr_storage_iptostring(it->first) << ".  returning false. attempts=" << it->second.connect_attempts << std::endl;
#endif
	    check_result = RSBANLIST_CHECK_RESULT_BLACKLISTED;
      return false ;
  }

#ifdef DEBUG_BANLIST
  std::cerr << " not blacklisted. Accepting." << std::endl;
#endif
        check_result = RSBANLIST_CHECK_RESULT_ACCEPTED;
    return true ;
}

void p3BanList::getWhiteListedIps(std::list<BanListPeer> &lst)
{
    RS_STACK_MUTEX(mBanMtx) ;

    lst.clear() ;
    for(std::map<sockaddr_storage,BanListPeer>::const_iterator it(mWhiteListedRanges.begin());it!=mWhiteListedRanges.end();++it)
        lst.push_back(it->second) ;
}
void p3BanList::getBannedIps(std::list<BanListPeer> &lst)
{
    RS_STACK_MUTEX(mBanMtx) ;


    lst.clear() ;
    for(std::map<sockaddr_storage,BanListPeer>::const_iterator it(mBanSet.begin());it!=mBanSet.end();++it)
    {
        if(!acceptedBanSet_locked(it->second))
            continue ;

        std::map<sockaddr_storage,BanListPeer>::const_iterator found1 = mBanRanges.find(makeBitsRange(it->first,1)) ;

        if(found1!=mBanRanges.end() && acceptedBanRanges_locked(found1->second))
            continue ;

        std::map<sockaddr_storage,BanListPeer>::const_iterator found2 = mBanRanges.find(makeBitsRange(it->first,2)) ;

        if(found2!=mBanRanges.end() && acceptedBanRanges_locked(found2->second))
            continue ;

           lst.push_back(it->second) ;
    }

    for(std::map<sockaddr_storage,BanListPeer>::const_iterator it(mBanRanges.begin());it!=mBanRanges.end();++it)
        if(acceptedBanRanges_locked(it->second))
            lst.push_back(it->second) ;
}

bool p3BanList::removeIpRange( const struct sockaddr_storage& dAddr,
                               int masked_bytes, uint32_t list_type )
{
	sockaddr_storage addr; sockaddr_storage_copy(dAddr, addr);
	if(!sockaddr_storage_ipv6_to_ipv4(addr))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Cannot handle "
		          << sockaddr_storage_tostring(dAddr)
		          << " IPv6 not implemented yet!"
		          << std::endl;
		return false;
	}

	RS_STACK_MUTEX(mBanMtx);

    bool changed = false;
    std::map<sockaddr_storage,BanListPeer>::iterator it ;

    if(list_type == RSBANLIST_TYPE_BLACKLIST)
    {
        if( mBanRanges.end() != (it = mBanRanges.find(makeBitsRange(addr,masked_bytes))))
        {
            mBanRanges.erase(it) ;
            IndicateConfigChanged();
            changed = true;
        }
    }
    else if(list_type == RSBANLIST_TYPE_WHITELIST)
    {
        if( mWhiteListedRanges.end() != (it = mWhiteListedRanges.find(makeBitsRange(addr,masked_bytes))))
        {
            mWhiteListedRanges.erase(it) ;
            IndicateConfigChanged();
            changed = true;
        }
    }
    else
        std::cerr << "(EE) Only whitelist or blacklist ranges can be removed." << std::endl;

    condenseBanSources_locked() ;
    return changed;
}

bool p3BanList::addIpRange( const sockaddr_storage &dAddr, int masked_bytes,
                            uint32_t list_type, const std::string& comment )
{
	sockaddr_storage addr; sockaddr_storage_copy(dAddr, addr);
	if(!sockaddr_storage_ipv6_to_ipv4(addr))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Cannot handle "
		          << sockaddr_storage_tostring(dAddr)
		          << " IPv6 not implemented yet!"
		          << std::endl;
		return false;
	}

	RS_STACK_MUTEX(mBanMtx);

    if(getBitRange(addr) > uint32_t(masked_bytes))
    {
        std::cerr << "(EE) Input to p3BanList::addIpRange is inconsistent: ip=" << sockaddr_storage_iptostring(addr) << "/" << 32-8*masked_bytes << std::endl;
        return false ;
    }

    BanListPeer blp ;
    blp.level = RSBANLIST_ORIGIN_SELF ;
    blp.connect_attempts = 0 ;
    blp.addr = addr ;
    blp.masked_bytes = masked_bytes ;
    blp.mTs = time(NULL) ;
    blp.reason = RSBANLIST_REASON_USER;
    blp.comment = comment ;

    if(masked_bytes != 0 && masked_bytes != 1 && masked_bytes != 2)
    {
        std::cerr << "Unhandled masked byte size " << masked_bytes << ". Should be 0,1 or 2" << std::endl;
        return false;
    }
    if(list_type != RSBANLIST_TYPE_BLACKLIST && list_type != RSBANLIST_TYPE_WHITELIST)
    {
        std::cerr << "(EE) Cannot add IP range. Bad list_type. Should be eiter RSBANLIST_TYPE_BLACKLIST or RSBANLIST_TYPE_WHITELIST" << std::endl;
    return false ;
    }

    // Remove all existing ranges that are included into current range.

    std::map<sockaddr_storage,BanListPeer>& banlist( (list_type == RSBANLIST_TYPE_BLACKLIST)?mBanRanges:mWhiteListedRanges) ;

    for(int i=0;i<masked_bytes;++i)
    {
        std::map<sockaddr_storage,BanListPeer>::iterator it = banlist.find(makeBitsRange(addr,i)) ;

        if(it != banlist.end())
            banlist.erase(it) ;
    }
    // Add range to list.

    banlist[makeBitsRange(addr,masked_bytes)] = blp ;

    IndicateConfigChanged() ;
    condenseBanSources_locked() ;

    return true;
}

int p3BanList::tick()
{
    processIncoming();
    sendPackets();

    rstime_t now = time(NULL) ;

    if(mLastDhtInfoRequest + RSBANLIST_DELAY_BETWEEN_TALK_TO_DHT < now)
    {
        getDhtInfo() ;	// This is always done, since these IPs are also sent to friends.

        mLastDhtInfoRequest = now;

        if(mAutoRangeIps)
            autoFigureOutBanRanges() ;
    }

#ifdef DEBUG_BANLIST
    static rstime_t last_print = 0 ;

    if(now > 10+last_print)
    {
        RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/
        printBanSet_locked(std::cerr);
        last_print = now ;
    }
#endif

    return 0;
}

void p3BanList::getDhtInfo()
{
    // Get the list of masquerading peers from the DHT. Add them as potential IPs to be banned.
    // Don't make them active. Just insert them in the list.

    std::list<RsDhtFilteredPeer> filtered_peers ;

    rsDht->getListOfBannedIps(filtered_peers) ;

#ifdef DEBUG_BANLIST
    std::cerr << "p3BanList::getDhtInfo() Got list of banned IPs." << std::endl;
#endif
    RsPeerId ownId = mServiceCtrl->getOwnId();

    for(std::list<RsDhtFilteredPeer>::const_iterator it(filtered_peers.begin());it!=filtered_peers.end();++it)
    {
#ifdef DEBUG_BANLIST
        std::cerr << "  filtered peer: " << rs_inet_ntoa((*it).mAddr.sin_addr) << std::endl;
#endif

        int int_reason = RSBANLIST_REASON_DHT ;
        int time_stamp = (*it).mLastSeen ;

        sockaddr_storage ad ;
    sockaddr_storage_setipv4(ad,&(*it).mAddr) ;

        addBanEntry(ownId, ad, RSBANLIST_ORIGIN_SELF, int_reason, time_stamp);
    }

    RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/
    condenseBanSources_locked() ;
}

/***** Implementation ******/

bool p3BanList::processIncoming()
{
	/* for each packet - pass to specific handler */
	RsItem *item = NULL;
	bool updated = false;
	while(NULL != (item = recvItem()))
	{
#ifdef DEBUG_BANLIST
		std::cerr << "p3BanList::processingIncoming() Received Item:";
		std::cerr << std::endl;
		item->print(std::cerr);
		std::cerr << std::endl;
#endif
		switch(item->PacketSubType())
		{
			default:
				break;
			case RS_PKT_SUBTYPE_BANLIST_ITEM:
			{
				// Order is important!.
				updated = (recvBanItem((RsBanListItem *) item) || updated);
			}
				break;
		}

		/* clean up */
		delete item;
	}

	if (updated)
	{
		{
			RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/

			condenseBanSources_locked();
		}

		/* pass list to NetAssist */

	}

	return true ;
}


bool p3BanList::recvBanItem(RsBanListItem *item)
{
	bool updated = false;

    rstime_t now = time(NULL) ;
    std::list<RsTlvBanListEntry>::const_iterator it;

	for(it = item->peerList.mList.begin(); it != item->peerList.mList.end(); ++it)
	{
		// Order is important!.
        updated = (addBanEntry(item->PeerId(), it->addr.addr, it->level,  it->reason, now - it->age) || updated);
	}
	return updated;
}

/* overloaded from pqiNetAssistSharePeer */
void p3BanList::updatePeer( const RsPeerId& /*id*/,
                            const sockaddr_storage &dAddr,
                            int /*type*/, int /*reason*/, int time_stamp )
{
	sockaddr_storage addr; sockaddr_storage_copy(dAddr, addr);
	if(!sockaddr_storage_ipv6_to_ipv4(addr))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Cannot handle "
		          << sockaddr_storage_tostring(dAddr)
		          << " IPv6 not implemented yet!"
		          << std::endl;
		return;
	}

	RsPeerId ownId = mServiceCtrl->getOwnId();

	int int_reason = RSBANLIST_REASON_DHT;

	addBanEntry(ownId, addr, RSBANLIST_ORIGIN_SELF, int_reason, time_stamp);

	/* process */
	{
		RS_STACK_MUTEX(mBanMtx);
		condenseBanSources_locked();
	}
}

RsSerialiser *p3BanList::setupSerialiser()
{
    RsSerialiser *rss = new RsSerialiser ;

    rss->addSerialType(new RsBanListSerialiser()) ;
    rss->addSerialType(new RsGeneralConfigSerialiser());

    return rss ;
}

bool p3BanList::saveList(bool &cleanup, std::list<RsItem*>& itemlist)
{
    RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/

    cleanup = true ;

    for(std::map<RsPeerId,BanList>::const_iterator it(mBanSources.begin());it!=mBanSources.end();++it)
    {
        RsBanListConfigItem *item = new RsBanListConfigItem ;

        item->banListType   = RSBANLIST_TYPE_PEERLIST ;
        item->banListPeerId = it->second.mPeerId ;
        item->update_time   = it->second.mLastUpdate ;
        item->banned_peers.TlvClear() ;

        for(std::map<sockaddr_storage,BanListPeer>::const_iterator it2 = it->second.mBanPeers.begin();it2!=it->second.mBanPeers.end();++it2)
        {
            RsTlvBanListEntry e ;
            it2->second.toRsTlvBanListEntry(e) ;

            item->banned_peers.mList.push_back(e) ;
        }

        itemlist.push_back(item) ;
    }

    // Add  whitelist
    RsBanListConfigItem *item = new RsBanListConfigItem ;

    item->banListType = RSBANLIST_TYPE_WHITELIST ;
    item->banListPeerId.clear() ;
    item->update_time  = 0 ;
    item->banned_peers.TlvClear() ;

    for(std::map<sockaddr_storage,BanListPeer>::const_iterator it2 = mWhiteListedRanges.begin();it2!=mWhiteListedRanges.end();++it2)
    {
        RsTlvBanListEntry e ;
        it2->second.toRsTlvBanListEntry(e) ;

        item->banned_peers.mList.push_back(e) ;
    }

    itemlist.push_back(item) ;

    // addblacklist

    item = new RsBanListConfigItem ;

    item->banListType = RSBANLIST_TYPE_BLACKLIST ;
    item->banListPeerId.clear();
    item->update_time  = 0 ;
    item->banned_peers.TlvClear() ;

    for(std::map<sockaddr_storage,BanListPeer>::const_iterator it2 = mBanRanges.begin();it2!=mBanRanges.end();++it2)
    {
        RsTlvBanListEntry e ;
        it2->second.toRsTlvBanListEntry(e) ;

        item->banned_peers.mList.push_back(e) ;
    }

    itemlist.push_back(item) ;

    // Other variables

    RsConfigKeyValueSet *vitem = new RsConfigKeyValueSet ;

    RsTlvKeyValue kv;

    kv.key = "IP_FILTERING_ENABLED";
    kv.value = mIPFilteringEnabled?"TRUE":"FALSE" ;
    vitem->tlvkvs.pairs.push_back(kv) ;

    kv.key = "IP_FILTERING_AUTORANGE_IPS";
    kv.value = mAutoRangeIps?"TRUE":"FALSE" ;
    vitem->tlvkvs.pairs.push_back(kv) ;

    kv.key = "IP_FILTERING_FRIEND_GATHERING_ENABLED";
    kv.value = mIPFriendGatheringEnabled?"TRUE":"FALSE" ;
    vitem->tlvkvs.pairs.push_back(kv) ;

    kv.key = "IP_FILTERING_DHT_GATHERING_ENABLED";
    kv.value = mIPDHTGatheringEnabled?"TRUE":"FALSE" ;
    vitem->tlvkvs.pairs.push_back(kv) ;

    kv.key = "IP_FILTERING_AUTORANGE_IPS_LIMIT" ;
    std::ostringstream os ;
    os << mAutoRangeLimit ;
    os.flush() ;
    kv.value = os.str() ;
    vitem->tlvkvs.pairs.push_back(kv) ;

    itemlist.push_back(vitem) ;

    return true ;
}

bool p3BanList::loadList(std::list<RsItem*>& load)
{
    RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/

    for(std::list<RsItem*>::const_iterator it(load.begin());it!=load.end();++it)
    {
        RsConfigKeyValueSet *vitem = dynamic_cast<RsConfigKeyValueSet*>( *it ) ;

        if(vitem != NULL)
            for(std::list<RsTlvKeyValue>::const_iterator it2(vitem->tlvkvs.pairs.begin());it2!=vitem->tlvkvs.pairs.end();++it2)
            {
                if(it2->key == "IP_FILTERING_ENABLED") mIPFilteringEnabled = (it2->value=="TRUE") ;
                if(it2->key == "IP_FILTERING_AUTORANGE_IPS") mAutoRangeIps = (it2->value=="TRUE") ;
                if(it2->key == "IP_FILTERING_FRIEND_GATHERING_ENABLED") mIPFriendGatheringEnabled = (it2->value=="TRUE") ;
                if(it2->key == "IP_FILTERING_DHT_GATHERING_ENABLED") mIPDHTGatheringEnabled = (it2->value=="TRUE") ;

                if(it2->key == "IP_FILTERING_AUTORANGE_IPS_LIMIT")
        {
            int val ;
            if(sscanf(it2->value.c_str(),"%d",&val) == 1)
                mAutoRangeLimit = val ;
        }
            }

        RsBanListConfigItem *citem = dynamic_cast<RsBanListConfigItem*>( *it ) ;

        if(citem != NULL)
        {
            if(citem->banListType == RSBANLIST_TYPE_PEERLIST)
            {
                BanList& bl(mBanSources[citem->banListPeerId]) ;

                bl.mPeerId = citem->banListPeerId ;
                bl.mLastUpdate = citem->update_time ;

                bl.mBanPeers.clear() ;

                for(std::list<RsTlvBanListEntry>::const_iterator it2(citem->banned_peers.mList.begin());it2!=citem->banned_peers.mList.end();++it2)
                {
                    BanListPeer blp ;
                    blp.fromRsTlvBanListEntry(*it2) ;

                    if(sockaddr_storage_isValidNet(blp.addr))
                        bl.mBanPeers[blp.addr] = blp ;
                    else
                        std::cerr << "(WW) removed wrong address " << sockaddr_storage_iptostring(blp.addr) << std::endl;
                }
            }
            else if(citem->banListType == RSBANLIST_TYPE_BLACKLIST)
            {
                mBanRanges.clear() ;

                for(std::list<RsTlvBanListEntry>::const_iterator it2(citem->banned_peers.mList.begin());it2!=citem->banned_peers.mList.end();++it2)
                {
                    BanListPeer blp ;
                    blp.fromRsTlvBanListEntry(*it2) ;

                    if(sockaddr_storage_isValidNet(blp.addr))
                        mBanRanges[makeBitsRange(blp.addr,blp.masked_bytes)] = blp ;
                    else
                        std::cerr << "(WW) removed wrong address " << sockaddr_storage_iptostring(blp.addr) << std::endl;
                }
            }
            else if(citem->banListType == RSBANLIST_TYPE_WHITELIST)
            {
                mWhiteListedRanges.clear() ;

                for(std::list<RsTlvBanListEntry>::const_iterator it2(citem->banned_peers.mList.begin());it2!=citem->banned_peers.mList.end();++it2)
                {
                    BanListPeer blp ;
                    blp.fromRsTlvBanListEntry(*it2) ;

                    if(sockaddr_storage_isValidNet(blp.addr))
                        mWhiteListedRanges[makeBitsRange(blp.addr,blp.masked_bytes)] = blp ;
                    else
                        std::cerr << "(WW) removed wrong address " << sockaddr_storage_iptostring(blp.addr) << std::endl;

                    std::cerr << "Read whitelisted range " << sockaddr_storage_iptostring(blp.addr) << "/" << 32 - 8*(blp.masked_bytes) << std::endl;
                }
            }
            else
                std::cerr << "(EE) BanList item unknown type " << citem->banListType << ". This is a bug." << std::endl;
        }

        delete *it ;
    }

    load.clear() ;
    return true ;
}

bool p3BanList::addBanEntry( const RsPeerId &peerId,
                             const sockaddr_storage &dAddr,
                             int level, uint32_t reason, rstime_t time_stamp )
{
	sockaddr_storage addr; sockaddr_storage_copy(dAddr, addr);
	if(!sockaddr_storage_ipv6_to_ipv4(addr))
	{
		std::cerr << __PRETTY_FUNCTION__ << " Cannot handle "
		          << sockaddr_storage_tostring(dAddr)
		          << " IPv6 not implemented yet!"
		          << std::endl;
		return false;
	}

	RS_STACK_MUTEX(mBanMtx);

	rstime_t now = time(NULL);
	bool updated = false;

#ifdef DEBUG_BANLIST
    std::cerr << "p3BanList::addBanEntry() Addr: " << sockaddr_storage_iptostring(addr) << " Origin: " << level;
    std::cerr << " Reason: " << reason << " Age: " << now - time_stamp;
	std::cerr << std::endl;
#endif

	/* Only Accept it - if external address */
	if (!sockaddr_storage_isExternalNet(addr))
	{
#ifdef DEBUG_BANLIST
		std::cerr << "p3BanList::addBanEntry() Ignoring Non External Addr: " << sockaddr_storage_iptostring(addr);
		std::cerr << std::endl;
#endif
		return false;
        }


	std::map<RsPeerId, BanList>::iterator it;
	it = mBanSources.find(peerId);
	if (it == mBanSources.end())
	{
		BanList bl;
		bl.mPeerId = peerId;
		bl.mLastUpdate = now;
		mBanSources[peerId] = bl;

		it = mBanSources.find(peerId);
		updated = true;
	}

	// index is FAMILY + IP - the rest should be Zeros..
	struct sockaddr_storage bannedaddr;
	sockaddr_storage_clear(bannedaddr);
    bannedaddr.ss_family = AF_INET ;
    sockaddr_storage_copyip(bannedaddr, addr);
	sockaddr_storage_setport(bannedaddr, 0);

	std::map<struct sockaddr_storage, BanListPeer>::iterator mit;
	mit = it->second.mBanPeers.find(bannedaddr);
	if (mit == it->second.mBanPeers.end())
	{
		/* add in */
		BanListPeer blp;
		blp.addr = addr;
		blp.reason = reason;
        blp.level = level;
        blp.mTs = time_stamp ;
        blp.masked_bytes = 0 ;

		it->second.mBanPeers[bannedaddr] = blp;
		it->second.mLastUpdate = now;
		updated = true;
	}
	else
	{
		/* see if it needs an update */
		if ((mit->second.reason != reason) ||
            (mit->second.level != uint32_t(level)) ||
            (mit->second.mTs < time_stamp))
		{
			/* update */
			mit->second.addr = addr;
			mit->second.reason = reason;
            mit->second.level = level;
            mit->second.mTs = time_stamp;
            mit->second.masked_bytes = 0 ;

			it->second.mLastUpdate = now;
			updated = true;
		}
	}

	if (updated)
		IndicateConfigChanged() ;

	return updated;
}

bool p3BanList::isWhiteListed_locked(const sockaddr_storage& addr)
{
    if(mWhiteListedRanges.find(makeBitsRange(addr,0)) != mWhiteListedRanges.end())
        return true ;

    if(mWhiteListedRanges.find(makeBitsRange(addr,1)) != mWhiteListedRanges.end())
        return true ;

    if(mWhiteListedRanges.find(makeBitsRange(addr,2)) != mWhiteListedRanges.end())
        return true ;

    return false ;

}

/***
 * EXTRA DEBUGGING.
 * #define DEBUG_BANLIST_CONDENSE		1
 ***/

int p3BanList::condenseBanSources_locked()
{
        mBanSet.clear();

    rstime_t now = time(NULL);
	RsPeerId ownId = mServiceCtrl->getOwnId();

#ifdef DEBUG_BANLIST
	std::cerr << "p3BanList::condenseBanSources_locked()";
	std::cerr << std::endl;
#endif

	std::map<RsPeerId, BanList>::const_iterator it;
	for(it = mBanSources.begin(); it != mBanSources.end(); ++it)
	{
        if (now - it->second.mLastUpdate > RSBANLIST_ENTRY_MAX_AGE)
        {
#ifdef DEBUG_BANLIST_CONDENSE
        std::cerr << std::dec ;
            std::cerr << "p3BanList::condenseBanSources_locked()";
            std::cerr << " Ignoring Out-Of-Date peer: " << it->first;
            std::cerr << std::endl;
#endif
            continue;
        }

#ifdef DEBUG_BANLIST_CONDENSE
        std::cerr << std::dec ;
        std::cerr << "p3BanList::condenseBanSources_locked()";
		std::cerr << " Condensing Info from peer: " << it->first;
		std::cerr << std::endl;
#endif

		std::map<struct sockaddr_storage, BanListPeer>::const_iterator lit;
        for(lit = it->second.mBanPeers.begin(); lit != it->second.mBanPeers.end(); ++lit)
    {
        /* check timestamp */
                    if (now > RSBANLIST_ENTRY_MAX_AGE + lit->second.mTs)
                    {
#ifdef DEBUG_BANLIST_CONDENSE
        std::cerr << std::dec ;
                        std::cerr << "p3BanList::condenseBanSources_locked()";
                        std::cerr << " Ignoring Out-Of-Date Entry for: ";
                        std::cerr << sockaddr_storage_iptostring(lit->second.addr);
                        std::cerr << " time stamp= " << lit->second.mTs << ", age=" << now - lit->second.mTs;
                        std::cerr << std::endl;
#endif
                        continue;
                    }

        uint32_t lvl = lit->second.level;
        if (it->first != ownId)
        {
            /* as from someone else, increment level */
            lvl++;
        }

        struct sockaddr_storage bannedaddr;
        sockaddr_storage_clear(bannedaddr);
        bannedaddr.ss_family = AF_INET;
        sockaddr_storage_copyip(bannedaddr, lit->second.addr);
        sockaddr_storage_setport(bannedaddr, 0);

        if (isWhiteListed_locked(bannedaddr))
            continue;

        /* check if it exists in the Set already */
        std::map<struct sockaddr_storage, BanListPeer>::iterator sit;
        sit = mBanSet.find(bannedaddr);

        if ((sit == mBanSet.end()) || (lvl < sit->second.level))
        {
            BanListPeer bp = lit->second;
            bp.level = lvl;
        bp.masked_bytes = 0 ;
            sockaddr_storage_setport(bp.addr, 0);
            mBanSet[bannedaddr] = bp;
#ifdef DEBUG_BANLIST_CONDENSE
            std::cerr << "p3BanList::condenseBanSources_locked()";
            std::cerr << " Added New Entry for: ";
            std::cerr << sockaddr_storage_iptostring(bannedaddr);
            std::cerr << std::endl;
#endif
        }
        else
        {
#ifdef DEBUG_BANLIST_CONDENSE
            std::cerr << "p3BanList::condenseBanSources_locked()";
            std::cerr << " Merging Info for: ";
            std::cerr << sockaddr_storage_iptostring(bannedaddr);
            std::cerr << std::endl;
#endif
            /* update if necessary */
            if (lvl == sit->second.level)
            {
                sit->second.reason |= lit->second.reason;
                if (sit->second.mTs < lit->second.mTs)
                {
                    sit->second.mTs = lit->second.mTs;
                }
            }
        }
    }
	}


#ifdef DEBUG_BANLIST
	std::cerr << "p3BanList::condenseBanSources_locked() Printing New Set:";
	std::cerr << std::endl;

	printBanSet_locked(std::cerr);
#endif

	return true ;
}



int	p3BanList::sendPackets()
{
	rstime_t now = time(NULL);
	rstime_t pt;
	{
		RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/
		pt = mSentListTime;
	}

	if (now - pt > RSBANLIST_SEND_PERIOD)
	{
		sendBanLists();

		RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/

#ifdef DEBUG_BANLIST
		std::cerr << "p3BanList::sendPackets() Regular Broadcast";
		std::cerr << std::endl;

		printBanSources_locked(std::cerr);
		printBanSet_locked(std::cerr);
#endif

		mSentListTime = now;
	}
	return true ;
}

void p3BanList::sendBanLists()
{

	/* we ping our peers */
	/* who is online? */
	std::set<RsPeerId> idList;

	mServiceCtrl->getPeersConnected(getServiceInfo().mServiceType, idList);

#ifdef DEBUG_BANLIST
	std::cerr << "p3BanList::sendBanList()";
	std::cerr << std::endl;
#endif

	/* prepare packets */
	std::set<RsPeerId>::iterator it;
	for(it = idList.begin(); it != idList.end(); ++it)
	{
#ifdef DEBUG_BANLIST
		std::cerr << "p3BanList::sendBanList() To: " << *it;
		std::cerr << std::endl;
#endif
		sendBanSet(*it);
	}
}

// Send all manually banned ranges to friends

int p3BanList::sendBanSet(const RsPeerId& peerid)
{
    /* */
    RsBanListItem *item = new RsBanListItem();
    item->PeerId(peerid);

    //rstime_t now = time(NULL);

    {
        RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/
        std::map<struct sockaddr_storage, BanListPeer>::iterator it;
        for(it = mBanSet.begin(); it != mBanSet.end(); ++it)
        {
            if (it->second.level >= RSBANLIST_ORIGIN_FRIEND)
                continue; // only share OWN for the moment.

            RsTlvBanListEntry bi;
            it->second.toRsTlvBanListEntry(bi) ;

        bi.masked_bytes = 0 ;
        bi.comment.clear() ;	// don't send comments.
            item->peerList.mList.push_back(bi);
        }
    }

    sendItem(item);
    return 1;
}


int p3BanList::printBanSet_locked(std::ostream &out)
{
    rstime_t now = time(NULL);

    out << "p3BanList::printBanSet_locked()";
    out << "  Only printing active filters (due to user options).";
    out << std::endl;

    std::map<struct sockaddr_storage, BanListPeer>::iterator it;
    for(it = mBanSet.begin(); it != mBanSet.end(); ++it)
        if(acceptedBanSet_locked(it->second))
    {
        out << "Ban: " << sockaddr_storage_iptostring(it->second.addr);
        out << " Reason: " << it->second.reason;
        out << " Level: " << it->second.level;
        if (it->second.level > RSBANLIST_ORIGIN_FRIEND)
        {
            out << " (unused)";
        }

        out << " Age: " << now - it->second.mTs;
        out << std::endl;
    }

    std::cerr << "Current IP black list (only showing manual ranges, not automatically banned IPs): " << std::dec << std::endl;

    for(std::map<sockaddr_storage,BanListPeer>::const_iterator it(mBanRanges.begin());it!=mBanRanges.end();++it)
        if(acceptedBanRanges_locked(it->second))
        std::cerr << "  " << sockaddr_storage_iptostring(it->first) << ". masked_bytes=" << (int)it->second.masked_bytes
              << ", IP=" << sockaddr_storage_iptostring(it->second.addr) << "/" << ((int)32 - 8*(int)(it->second.masked_bytes)) << std::endl;

    std::cerr << "Current IP white list: " << std::endl;

    for(std::map<sockaddr_storage,BanListPeer>::const_iterator it(mWhiteListedRanges.begin());it!=mWhiteListedRanges.end();++it)
        if(acceptedBanRanges_locked(it->second))
        std::cerr << "  " << sockaddr_storage_iptostring(it->first) << ". masked_bytes=" << (int)it->second.masked_bytes
              << ", IP=" << sockaddr_storage_iptostring(it->second.addr) << "/" << ((int)32 - 8*(int)(it->second.masked_bytes)) << std::endl;

    return true ;
}



int p3BanList::printBanSources_locked(std::ostream &out)
{
	rstime_t now = time(NULL);

	std::map<RsPeerId, BanList>::const_iterator it;
	for(it = mBanSources.begin(); it != mBanSources.end(); ++it)
	{
		out << "BanList from: " << it->first;
		out << " LastUpdate: " << now - it->second.mLastUpdate;
		out << std::endl;

		std::map<struct sockaddr_storage, BanListPeer>::const_iterator lit;
		for(lit = it->second.mBanPeers.begin();
			lit != it->second.mBanPeers.end(); ++lit)
		{
			out << "\t";
			out << "Ban: " << sockaddr_storage_iptostring(lit->second.addr);
			out << " Reason: " << lit->second.reason;
            out << " Level: " << lit->second.level;
			out << " Age: " << now - lit->second.mTs;
			out << std::endl;
		}
    }

    return true ;
}

RsBanList::~RsBanList() = default;
