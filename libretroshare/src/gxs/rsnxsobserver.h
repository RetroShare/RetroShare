/*******************************************************************************
 * libretroshare/src/gxs: rsgxsobserver.h                                      *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2011-2012 by Robert Fernie, Evi-Parker Christopher                *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
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
 *******************************************************************************/
#ifndef RSNXSOBSERVER_H
#define RSNXSOBSERVER_H

#include <set>
#include "rsitems/rsnxsitems.h"

typedef uint32_t TurtleRequestId ;

class RsNxsObserver
{
public:

    RsNxsObserver() {}


public:

    /*!
     * @param messages messages are deleted after function returns
     */
    virtual void receiveNewMessages(std::vector<RsNxsMsg*>& messages) = 0;

    /*!
     * @param groups groups are deleted after function returns
     */
    virtual void receiveNewGroups(std::vector<RsNxsGrp*>& groups) = 0;

    /*!
     * \brief receiveDistantSearchResults
     * 				Called when new distant search result arrive.
     * \param grpId
     */
    virtual void receiveDistantSearchResults(TurtleRequestId /*id*/,const RsGxsGroupId& /*grpId*/)
    {
        std::cerr << __PRETTY_FUNCTION__ << ": not overloaded but still called. Nothing will happen." << std::endl;
    }

    /*!
     * @param grpId group id
     */
    virtual void notifyReceivePublishKey(const RsGxsGroupId &grpId) = 0;

    /*!
     * @param grpId group id
     */
    virtual void notifyChangedGroupStats(const RsGxsGroupId &grpId) = 0;
};

#endif // RSNXSOBSERVER_H
