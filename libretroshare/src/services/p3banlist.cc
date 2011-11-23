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

#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"

#include "util/rsnet.h"

#include "services/p3banlist.h"
#include "serialiser/rsbanlistitems.h"

#include <sys/time.h>

/****
 * #define DEBUG_BANLIST		1
 ****/


/* DEFINE INTERFACE POINTER! */
//RsBanList *rsBanList = NULL;

#define RSBANLIST_ENTRY_MAX_AGE		(60 * 60 * 1) // 1 HOURS
#define RSBANLIST_SEND_PERIOD	600		// 10 Minutes.

#define RSBANLIST_SOURCE_SELF		0
#define RSBANLIST_SOURCE_FRIEND		1
#define RSBANLIST_SOURCE_FOF		2


/************ IMPLEMENTATION NOTES *********************************
 * 
 * Get Bad Peers passed to us (from DHT mainly).
 * we distribute and track the network list of bad peers.
 *
 */


p3BanList::p3BanList(p3LinkMgr *lm, p3NetMgr *nm)
	:p3Service(RS_SERVICE_TYPE_BANLIST), mBanMtx("p3BanList"), mLinkMgr(lm), mNetMgr(nm) 
{
	addSerialType(new RsBanListSerialiser());

	mSentListTime = 0;
}


int	p3BanList::tick()
{
	processIncoming();
	sendPackets();

	return 0;
}

int	p3BanList::status()
{
	return 1;
}


/***** Implementation ******/

bool p3BanList::processIncoming()
{
	/* for each packet - pass to specific handler */
	RsItem *item = NULL;
	bool updated = false;
	while(NULL != (item = recvItem()))
	{
		switch(item->PacketSubType())
		{
			default:
				break;
			case RS_PKT_SUBTYPE_BANLIST_ITEM:
			{
				updated = (updated || recvBanItem((RsBanListItem *) item));
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
	for(it = item->peerList.entries.begin(); it != item->peerList.entries.end(); it++)
	{
		updated = (updated || addBanEntry(item->PeerId(), 
				it->addr, it->level, it->reason, it->age));		
	}
	return updated;
}

bool p3BanList::addBanEntry(const std::string &peerId, const struct sockaddr_in &addr, uint32_t level, uint32_t reason, uint32_t age)
{
	RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/

	time_t now = time(NULL);
	bool updated = false;

	std::map<std::string, BanList>::iterator it;
	it = mBanSources.find(peerId);
	if (it == mBanSources.end())
	{
		BanList bl;
		bl.mPeerId = peerId;
		bl.mLastUpdate = 0;
		mBanSources[peerId] = bl;

		it = mBanSources.find(peerId);
		updated = true;
	}
	
	std::map<uint32_t, BanListPeer>::iterator mit;
	mit = it->second.mBanPeers.find(addr.sin_addr.s_addr);
	if (mit == it->second.mBanPeers.end())
	{
		/* add in */
		BanListPeer blp;
		blp.addr = addr;
		blp.reason = reason;
		blp.level = level;
		blp.mTs = now - age;
		updated = true;
	}
	else
	{
		/* see if it needs an update */
		if ((mit->second.reason != reason) ||
			(mit->second.level != level) ||
			(mit->second.mTs < now - age))
		{
			/* update */
			mit->second.addr = addr;
			mit->second.reason = reason;
			mit->second.level = level;
			mit->second.mTs = now - age;
			updated = true;
		}
	}
	return updated;
}


int p3BanList::condenseBanSources_locked()
{
	time_t now = time(NULL);
	std::string ownId = mLinkMgr->getOwnId();
	
	std::map<std::string, BanList>::const_iterator it;
	for(it = mBanSources.begin(); it != mBanSources.end(); it++)
	{
		if (now - it->second.mLastUpdate > RSBANLIST_ENTRY_MAX_AGE)
		{
			continue;
		}
		
		std::map<uint32_t, BanListPeer>::const_iterator lit;
		for(lit = it->second.mBanPeers.begin();
			lit != it->second.mBanPeers.end(); lit++)
		{
			/* check timestamp */
			if (now - lit->second.mTs > RSBANLIST_ENTRY_MAX_AGE)
			{
				continue;
			}

			int lvl = lit->second.level;
			if (it->first != ownId)	
			{
				/* as from someone else, increment level */
				lvl++;
			}

			/* check if it exists in the Set already */
			std::map<uint32_t, BanListPeer>::iterator sit;
			sit = mBanSet.find(lit->second.addr.sin_addr.s_addr);
			if ((sit == mBanSet.end()) || (lvl < sit->second.level))
			{
				BanListPeer bp = lit->second;
				bp.level = lvl;
				bp.addr.sin_port = 0;
				mBanSet[lit->second.addr.sin_addr.s_addr] = bp;
			}
			else
			{
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
		printBanSources(std::cerr);
		printBanSet(std::cerr);

		RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/
		mSentListTime = now;
	}
	return true ;
}

void p3BanList::sendBanLists()
{

	/* we ping our peers */
	/* who is online? */
	std::list<std::string> idList;

	mLinkMgr->getOnlineList(idList);

#ifdef DEBUG_BANLIST
	std::cerr << "p3BanList::sendBanList()";
	std::cerr << std::endl;
#endif

	/* prepare packets */
	std::list<std::string>::iterator it;
	for(it = idList.begin(); it != idList.end(); it++)
	{
#ifdef DEBUG_BANLIST
		std::cerr << "p3VoRS::sendBanList() To: " << *it;
		std::cerr << std::endl;
#endif
		sendBanSet(*it);
	}
}



int p3BanList::sendBanSet(std::string peerid)
{
	/* */
	RsBanListItem *item = new RsBanListItem();
	item->PeerId(peerid);
	
	time_t now = time(NULL);

	{
		RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/
		std::map<uint32_t, BanListPeer>::iterator it;
		for(it = mBanSet.begin(); it != mBanSet.end(); it++)
		{
			if (it->second.level > RSBANLIST_SOURCE_FRIEND)
			{
				continue; // only share OWN and FRIENDS.
			}
	
			RsTlvBanListEntry bi;
			bi.addr = it->second.addr;
			bi.reason = it->second.reason;
			bi.level = it->second.level;
			bi.age = now - it->second.mTs;
	
			item->peerList.entries.push_back(bi);
		}
	}			

	sendItem(item);
	return 1;
}


int p3BanList::printBanSet(std::ostream &out)
{
	RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/

	out << "p3BanList::printBanSet";
	out << std::endl;
	
	time_t now = time(NULL);

	std::map<uint32_t, BanListPeer>::iterator it;
	for(it = mBanSet.begin(); it != mBanSet.end(); it++)
	{
		out << "Ban: " << rs_inet_ntoa(it->second.addr.sin_addr);
		out << " Reason: " << it->second.reason;
		out << " Level: " << it->second.level;
		if (it->second.level > RSBANLIST_SOURCE_FRIEND)
		{
			out << " (unused)";
		}
		
		out << " Age: " << now - it->second.mTs;
		out << std::endl;
	}			
}



int p3BanList::printBanSources(std::ostream &out)
{
	RsStackMutex stack(mBanMtx); /****** LOCKED MUTEX *******/

	time_t now = time(NULL);
	
	std::map<std::string, BanList>::const_iterator it;
	for(it = mBanSources.begin(); it != mBanSources.end(); it++)
	{
		out << "BanList from: " << it->first;
		out << " LastUpdate: " << now - it->second.mLastUpdate;
		out << std::endl;

		std::map<uint32_t, BanListPeer>::const_iterator lit;
		for(lit = it->second.mBanPeers.begin();
			lit != it->second.mBanPeers.end(); lit++)
		{
			out << "\t";
			out << "Ban: " << rs_inet_ntoa(lit->second.addr.sin_addr);
			out << " Reason: " << lit->second.reason;
			out << " Level: " << lit->second.level;
			out << " Age: " << now - lit->second.mTs;
			out << std::endl;
		}
	}
}


