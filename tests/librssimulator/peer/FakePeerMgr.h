/*******************************************************************************
 * librssimulator/peer/: FakePeerMgr.h                                         *
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


