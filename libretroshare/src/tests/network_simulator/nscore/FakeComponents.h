#pragma once

#include <pqi/p3linkmgr.h>
#include <pqi/p3peermgr.h>
#include <ft/ftserver.h>

class FakeLinkMgr: public p3LinkMgrIMPL
{
	public:
		FakeLinkMgr(const RsPeerId& own_id,const std::list<RsPeerId>& friends)
			: p3LinkMgrIMPL(NULL,NULL),_own_id(own_id),_friends(friends)
		{
		}

		virtual const RsPeerId& getOwnId() { return _own_id ; }
		virtual void getOnlineList(std::list<RsPeerId>& lst) { lst = _friends ; }
		virtual uint32_t getLinkType(const RsPeerId&) { return RS_NET_CONN_TCP_ALL | RS_NET_CONN_SPEED_NORMAL; }

		virtual bool getPeerName(const RsPeerId &ssl_id, std::string &name) { name = ssl_id.toStdString() ; return true ;}

	private:
		RsPeerId _own_id ;
		std::list<RsPeerId> _friends ;
};

class FakePublisher: public pqiPublisher
{
	public:
		virtual bool sendItem(RsRawItem *item) 
		{
			_item_queue.push_back(item) ;
			return true ;
		}

		RsRawItem *outgoing() 
		{
            if(_item_queue.empty())
                return NULL ;

            RsRawItem *item = _item_queue.front() ;
			_item_queue.pop_front() ;
			return item ;
		}

	private:
		std::list<RsRawItem*> _item_queue ;
};

class FakePeerMgr: public p3PeerMgrIMPL
{
	public:
		FakePeerMgr(const RsPeerId& own,const std::list<RsPeerId>& ids)
			: p3PeerMgrIMPL(own,RsPgpId(),"no name","location name")
		{
			for(std::list<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
				_ids.insert(*it) ;
		}

		virtual bool idFriend(const RsPeerId& ssl_id) { return _ids.find(ssl_id) != _ids.end() ; }

        virtual ServicePermissionFlags servicePermissionFlags(const RsPeerId& ssl_id)
        {
            return ~ServicePermissionFlags(0) ;
        }
        std::set<RsPeerId> _ids ;
};

class FakeServiceControl: public p3ServiceControl
{
    public:
        FakeServiceControl(p3LinkMgr *lm)
            : p3ServiceControl(lm),mLink(lm)
        {
        }

        virtual void getPeersConnected(const uint32_t serviceId, std::set<RsPeerId> &peerSet)
        {
            std::list<RsPeerId> ids ;
            mLink->getOnlineList(ids) ;

            for(std::list<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
                peerSet.insert(*it) ;
        }

    virtual bool checkFilter(uint32_t,const RsPeerId& id)
    {
        return true ;
    }
    p3LinkMgr *mLink;
};

