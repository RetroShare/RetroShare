#pragma once

#include <list>
#include <retroshare/rsids.h>
#include <pqi/p3peermgr.h>

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

        virtual ServicePermissionFlags servicePermissionFlags(const RsPeerId& /*ssl_id*/)
        {
            return ~ServicePermissionFlags(0) ;
        }
        std::set<RsPeerId> _ids ;
};


