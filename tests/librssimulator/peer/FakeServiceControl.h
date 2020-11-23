/*******************************************************************************
 * librssimulator/peer/: FakeServiceControl.h                                  *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2018, Retroshare team <retroshare.team@gmailcom>              *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
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
 ******************************************************************************/
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

	virtual void getPeersConnected(uint32_t serviceId, std::set<RsPeerId> &peerSet)
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

