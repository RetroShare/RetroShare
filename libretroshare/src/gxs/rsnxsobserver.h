#ifndef RSNXSOBSERVER_H
#define RSNXSOBSERVER_H

/*
 * libretroshare/src/gxp: gxp.h
 *
 * Observer, interface for RetroShare.
 *
 * Copyright 2011-2012 by Robert Fernie, Evi-Parker Christopher
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

#include <set>
#include "serialiser/rsnxsitems.h"


class RsNxsObserver
{
public:

    RsNxsObserver() {}


public:

    /*!
     * @param messages messages are deleted after function returns
     */
    virtual void notifyNewMessages(std::vector<RsNxsMsg*> messages) = 0;

    /*!
     * @param messages messages are deleted after function returns
     */
    virtual void notifyNewGroups(std::vector<RsNxsGrp*>& groups) = 0;



};

#endif // RSNXSOBSERVER_H
