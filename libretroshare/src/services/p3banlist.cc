/*
 * libretroshare/src/services p3banlist.cc
 *
 * Ban List Service  for RetroShare.
 *
 * Copyright 2011-2011 by Robert Fernie.
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

#include "pqi/p3servicecontrol.h"
#include "pqi/p3netmgr.h"

#include "util/rsnet.h"

#include "services/p3banlist.h"
#include "serialiser/rsbanlistitems.h"
#include "retroshare/rsdht.h"

#include <sys/time.h>

/****
 * #define DEBUG_BANLIST		1
 ****/
 #define DEBUG_BANLIST		1


/* DEFINE INTERFACE POINTER! */
//RsBanList *rsBanList = NULL;

#define RSBANLIST_ENTRY_MAX_AGE		(60 * 60 * 1) // 1 HOURS
#define RSBANLIST_SEND_PERIOD	600		// 10 Minutes.

#define RSBANLIST_SOURCE_SELF		0
#define RSBANLIST_SOURCE_FRIEND		1
#define RSBANLIST_SOURCE_FOF		2

#define RSBANLIST_DELAY_BETWEEN_TALK_TO_DHT 60	// should be more: e.g. 600 secs.


/************ IMPLEMENTATION NOTES *********************************
 * 
 * Get Bad Peers passed to us (from DHT mainly).
 * we distribute and track the network list of bad peers.
 *
 */
RsBanList *rsBanList = NULL ;

p3BanList::p3BanList(p3ServiceControl *sc, p3NetMgr *nm)
	:p3Service(), mBanMtx("p3BanList"), mServiceCtrl(sc), mNetMgr(nm) 
{
	addSerialType(new RsBanListSerialiser());

    mSentListTime = 0;
    mLastDhtInfoRequest = 0 ;
}


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

bool p3BanList::isAddressAccepted(const sockaddr_storage &addr)
{
    // we should normally work this including entire ranges of IPs. For now, just check the exact IPs.

    if(mBanSet.find(addr) != mBanSet.end())
        return false ;

    return true ;
}

void p3BanList::getListOfBannedIps(std::list<BanListPeer> &lst)
{
    for(std::map<sockaddr_storage,BanListPeer>::const_iterator it(mBanSet.begin());it!=mBanSet.end();++it)
        lst.push_back(it->second) ;
}

int	p3BanList::tick()
{
	processIncoming();
	sendPackets();

    time_t now = time(NULL) ;

    if(mLastDhtInfoRequest + RSBANLIST_DELAY_BETWEEN_TALK_TO_DHT < now)
    {
        getDhtInfo() ;
        mLastDhtInfoRequest = now;
    }

	return 0;
}

int	p3BanList::status()
{
	return 1;
}

void p3BanList::getDhtInfo()
{
    // Get the list of masquerading peers from the DHT. Add them as potential IPs to be banned.
    // Don't make them active. Just insert them in the list.

    std::list<RsDhtFilteredPeer> filtered_peers ;

    rsDht->getListOfBannedIps(filtered_peers) ;

    std::cerr << "p3BanList::getDhtInfo() Got list of banned IPs." << std::endl;
    RsPeerId ownId = mServiceCtrl->getOwnId();

    for(std::list<RsDhtFilteredPeer>::const_iterator it(filtered_peers.begin());it!=filtered_peers.end();++it)
    {
        std::cerr << "  filtered peer: " << rs_inet_ntoa((*it).mAddr.sin_addr) << std::endl;

        int int_reason = 0 ;
        int age = 0 ;
        sockaddr_storage ad = *(sockaddr_storage*)&(*it).mAddr ;

        addBanEntry(ownId, ad, RSBANLIST_SOURCE_SELF, int_reason, age);
    }

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

			mBanSet.clear();
			condenseBanSources_locked();
		}

		/* pass list to NetAssist */

	}

	return true ;
} 
	

bool p3BanList::recvBanItem(RsBanListItem *item)
{
	bool updated = false;

	std::list<RsTlvBanListEntry>::const_iterator it;
	//for(it = item->peerList.entries.begin(); it != item->peerList.entries.end(); ++it)
	for(it = item->peerList.mList.begin(); it != item->peerList.mList.end(); ++it)
	{
		// Order is important!.	
		updated = (addBanEntry(item->PeerId(), it->addr.addr, it->level, 
			it->reason, it->age) || updated);
	}
	return updated;
}

/* overloaded from pqiNetAssistSharePeer */
void p3BanList::updatePeer(const RsPeerId& /*id*/, const struct sockaddr_storage &addr, int /*type*/, int /*reason*/, int age)
{
	RsPeerId ownId = mServiceCtrl->getOwnId();

	int int_reason = 0;
	addBanEntry(ownId, addr, RSBANLIST_SOURCE_SELF, int_reason, age);

	/* process */
	{
		RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/

		mBanSet.clear();
		condenseBanSources_locked();
	}
}


bool p3BanList::addBanEntry(const RsPeerId &peerId, const struct sockaddr_storage &addr, int level, uint32_t reason, uint32_t age)
{
	RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/

	time_t now = time(NULL);
	bool updated = false;

#ifdef DEBUG_BANLIST
    std::cerr << "p3BanList::addBanEntry() Addr: " << sockaddr_storage_iptostring(addr) << " Level: " << level;
	std::cerr << " Reason: " << reason << " Age: " << age;
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
		blp.mTs = now - age;

		
		it->second.mBanPeers[bannedaddr] = blp;
		it->second.mLastUpdate = now;
		updated = true;
	}
	else
	{
		/* see if it needs an update */
		if ((mit->second.reason != reason) ||
			(mit->second.level != level) ||
			(mit->second.mTs < (time_t) (now - age)))
		{
			/* update */
			mit->second.addr = addr;
			mit->second.reason = reason;
			mit->second.level = level;
			mit->second.mTs = now - age;

			it->second.mLastUpdate = now;
			updated = true;
		}
	}
	return updated;
}

/***
 * EXTRA DEBUGGING.
 * #define DEBUG_BANLIST_CONDENSE		1
 ***/

int p3BanList::condenseBanSources_locked()
{
	time_t now = time(NULL);
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
			std::cerr << "p3BanList::condenseBanSources_locked()";
			std::cerr << " Ignoring Out-Of-Date peer: " << it->first;
			std::cerr << std::endl;
#endif
			continue;
		}

#ifdef DEBUG_BANLIST_CONDENSE
		std::cerr << "p3BanList::condenseBanSources_locked()";
		std::cerr << " Condensing Info from peer: " << it->first;
		std::cerr << std::endl;
#endif
		
		std::map<struct sockaddr_storage, BanListPeer>::const_iterator lit;
		for(lit = it->second.mBanPeers.begin();
			lit != it->second.mBanPeers.end(); ++lit)
		{
			/* check timestamp */
			if (now - lit->second.mTs > RSBANLIST_ENTRY_MAX_AGE)
			{
#ifdef DEBUG_BANLIST_CONDENSE
				std::cerr << "p3BanList::condenseBanSources_locked()";
				std::cerr << " Ignoring Out-Of-Date Entry for: ";
				std::cerr << sockaddr_storage_iptostring(lit->second.addr);
				std::cerr << std::endl;
#endif
				continue;
			}

			int lvl = lit->second.level;
			if (it->first != ownId)	
			{
				/* as from someone else, increment level */
				lvl++;
			}

			struct sockaddr_storage bannedaddr;
			sockaddr_storage_clear(bannedaddr);
			sockaddr_storage_copyip(bannedaddr, lit->second.addr);
			sockaddr_storage_setport(bannedaddr, 0);


			/* check if it exists in the Set already */
			std::map<struct sockaddr_storage, BanListPeer>::iterator sit;
			sit = mBanSet.find(bannedaddr);
			if ((sit == mBanSet.end()) || (lvl < sit->second.level))
			{
				BanListPeer bp = lit->second;
				bp.level = lvl;
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
	time_t now = time(NULL);
	time_t pt;
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



int p3BanList::sendBanSet(const RsPeerId& peerid)
{
	/* */
	RsBanListItem *item = new RsBanListItem();
	item->PeerId(peerid);
	
	time_t now = time(NULL);

	{
		RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/
		std::map<struct sockaddr_storage, BanListPeer>::iterator it;
		for(it = mBanSet.begin(); it != mBanSet.end(); ++it)
		{
			if (it->second.level >= RSBANLIST_SOURCE_FRIEND)
			{
				continue; // only share OWN for the moment.
			}
	
			RsTlvBanListEntry bi;
			bi.addr.addr = it->second.addr;
			bi.reason = it->second.reason;
			bi.level = it->second.level;
			bi.age = now - it->second.mTs;
	
			//item->peerList.entries.push_back(bi);
			item->peerList.mList.push_back(bi);
		}
	}			

	sendItem(item);
	return 1;
}


int p3BanList::printBanSet_locked(std::ostream &out)
{
	out << "p3BanList::printBanSet_locked()";
	out << std::endl;
	
	time_t now = time(NULL);

	std::map<struct sockaddr_storage, BanListPeer>::iterator it;
	for(it = mBanSet.begin(); it != mBanSet.end(); ++it)
	{
		out << "Ban: " << sockaddr_storage_iptostring(it->second.addr);
		out << " Reason: " << it->second.reason;
		out << " Level: " << it->second.level;
		if (it->second.level > RSBANLIST_SOURCE_FRIEND)
		{
			out << " (unused)";
		}
		
		out << " Age: " << now - it->second.mTs;
		out << std::endl;
	}			
	return true ;
}



int p3BanList::printBanSources_locked(std::ostream &out)
{
	time_t now = time(NULL);
	
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


