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
	
	struct sockaddr_storage addr;
	uint32_t reason; // Dup Self, Dup Friend
	int level; // LOCAL, FRIEND, FoF.
	time_t mTs;
};

class BanList
{
	public:
	
	RsPeerId mPeerId; /* from */
	time_t mLastUpdate;
	std::map<struct sockaddr_storage, BanListPeer> mBanPeers;
};



//!The RS BanList service.
 /**
  *
  * Exchange list of Banned IP addresses with peers.
  */

class p3BanList: /* public RsBanList, */ public p3Service, public pqiNetAssistPeerShare /* , public p3Config, public pqiMonitor */
{
	public:
		p3BanList(p3LinkMgr *lm, p3NetMgr *nm);

		/***** overloaded from RsBanList *****/

		/***** overloaded from pqiNetAssistPeerShare *****/

		virtual void    updatePeer(const RsPeerId& id, const struct sockaddr_storage &addr, int type, int reason, int age);


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
		bool addBanEntry(const RsPeerId &peerId, const struct sockaddr_storage &addr, 
			int level, uint32_t reason, uint32_t age);
		void sendBanLists();
		int sendBanSet(const RsPeerId& peerid);


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
		int printBanSources_locked(std::ostream &out);
		int printBanSet_locked(std::ostream &out);

		time_t mSentListTime;
		std::map<RsPeerId, BanList> mBanSources;
		std::map<struct sockaddr_storage, BanListPeer> mBanSet;

		p3LinkMgr *mLinkMgr;
		p3NetMgr *mNetMgr;

};

#endif // SERVICE_RSBANLIST_HEADER

