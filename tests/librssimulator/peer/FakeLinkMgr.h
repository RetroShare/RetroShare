#pragma once

#include <iostream>
#include <list>

#include <retroshare/rsids.h>
#include <pqi/p3linkmgr.h>

class FakePeerListStatus
{
	public:
	bool mOnline;
};


class FakeLinkMgr: public p3LinkMgrIMPL
{
	public:
		FakeLinkMgr(const RsPeerId& own_id,const std::list<RsPeerId>& friends, bool peersOnline)
			: p3LinkMgrIMPL(NULL,NULL), mOwnId(own_id), mFriends()
		{
			std::list<RsPeerId>::const_iterator it;
			for(it = friends.begin(); it != friends.end(); it++)
			{
				setOnlineStatus(*it, peersOnline);
			}
		}

		virtual const RsPeerId& getOwnId() { return mOwnId; }
		virtual void getOnlineList(std::list<RsPeerId>& lst)
		{ 
			std::map<RsPeerId, FakePeerListStatus>::iterator it;
			for(it = mFriends.begin(); it != mFriends.end(); it++)
			{
				if (it->second.mOnline)
				{
					lst.push_back(it->first);
				}
			}
		}

		virtual void  getFriendList(std::list<RsPeerId> &ssl_peers)
		{
			std::map<RsPeerId, FakePeerListStatus>::iterator it;
			for(it = mFriends.begin(); it != mFriends.end(); it++)
			{
				ssl_peers.push_back(it->first);
			}
		}


		virtual uint32_t getLinkType(const RsPeerId&) { return RS_NET_CONN_TCP_ALL | RS_NET_CONN_SPEED_NORMAL; }

		virtual bool getPeerName(const RsPeerId &ssl_id, std::string &name) { name = ssl_id.toStdString() ; return true ;}


		// functions to manipulate status.
		virtual void setOnlineStatus(RsPeerId id, bool online)
		{
			FakePeerListStatus status;
			status.mOnline = online;
			mFriends[id] = status;
		}
			
	private:
		RsPeerId mOwnId;
		std::map<RsPeerId, FakePeerListStatus> mFriends;
};


