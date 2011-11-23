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


#ifndef SERVICE_RSBANLIST_HEADER
#define SERVICE_RSBANLIST_HEADER

#include <string>
#include <list>
#include <map>

#include "serialiser/rsbanlistitems.h"
#include "services/p3service.h"
//#include "retroshare/rsbanlist.h"

class p3LinkMgr;
class p3NetMgr;

class BanListPeer
{
	public:
	
	struct sockaddr_in addr;
	uint32_t reason; // Dup Self, Dup Friend
	int level; // LOCAL, FRIEND, FoF.
	time_t mTs;
};

class BanList
{
	public:
	
	std::string mPeerId; /* from */
	time_t mLastUpdate;
	std::map<uint32_t, BanListPeer> mBanPeers;
};



//!The RS BanList service.
 /**
  *
  * Exchange list of Banned IP addresses with peers.
  */

class p3BanList: /* public RsBanList, */ public p3Service /* , public p3Config, public pqiMonitor */
{
	public:
		p3BanList(p3LinkMgr *lm, p3NetMgr *nm);

		/***** overloaded from RsBanList *****/

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
		bool addBanEntry(const std::string &peerId, const struct sockaddr_in &addr, 
			uint32_t level, uint32_t reason, uint32_t age);
		void sendBanLists();
		int sendBanSet(std::string peerid);

		int printBanSources(std::ostream &out);
		int printBanSet(std::ostream &out);

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
		RsMutex mBanMtx;

		int condenseBanSources_locked();

		time_t mSentListTime;
		std::map<std::string, BanList> mBanSources;
		std::map<uint32_t, BanListPeer> mBanSet;

		p3LinkMgr *mLinkMgr;
		p3NetMgr *mNetMgr;

};

#endif // SERVICE_RSBANLIST_HEADER

