#pragma once

#include <iostream>
#include <list>

#include <retroshare/rsids.h>
#include <pqi/p3linkmgr.h>
#include <pqi/p3servicecontrol.h>

class FakeServiceControl: public p3ServiceControl
{
    public:
        FakeServiceControl(p3LinkMgr *lm)
            : p3ServiceControl(lm),mLink(lm)
        {
        }

        virtual void getPeersConnected(const uint32_t serviceId, std::set<RsPeerId> &peerSet)
        {
	    (void) serviceId;
            std::list<RsPeerId> ids ;
            mLink->getOnlineList(ids) ;

            for(std::list<RsPeerId>::const_iterator it(ids.begin());it!=ids.end();++it)
                peerSet.insert(*it) ;
        }

    virtual bool checkFilter(uint32_t,const RsPeerId& id)
    {
	(void) id;
        return true ;
    }
    p3LinkMgr *mLink;
};

