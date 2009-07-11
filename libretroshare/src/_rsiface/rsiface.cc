/*
 * "$Id: rsiface.cc,v 1.6 2007-04-15 18:45:23 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2007 by Robert Fernie.
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



#include "rsiface.h"
#include "util/rsdir.h"

RsIface::RsIface(NotifyBase &callback) :
        cb(callback)
{
}

RsIface::~RsIface()
{
}

const std::list<FileInfo>& RsIface::getRecommendList()
{
    return mRecommendList;
}

const RsConfig& RsIface::getConfig()
{
    return mConfig;
}

/* set to true */
bool RsIface::setChanged(DataFlags set)
{
    if ((int) set < (int) NumOfFlags)
    {
        /* go for it */
        mChanged[(int) set ] = true;
        return true;
    }
    return false;
}


/* leaves it */
bool    RsIface::getChanged(DataFlags set)
{
    if ((int) set < (int) NumOfFlags)
    {
        /* go for it */
        return mChanged[(int) set ];
    }
    return false;
}

/* resets it */
bool    RsIface::hasChanged(DataFlags set)
{
    if ((int) set < (int) NumOfFlags)
    {
        /* go for it */
        if (mChanged[(int) set ])
        {
            mChanged[(int) set ] = false;
            return true;
        }
    }
    return false;
}

RsIface *createRsIface(NotifyBase &cb)
{
    return new RsIfaceReal(cb);
}

