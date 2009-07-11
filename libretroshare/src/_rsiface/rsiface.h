#ifndef RETROSHARE_GUI_INTERFACE_H
#define RETROSHARE_GUI_INTERFACE_H

/*
 * "$Id: rsiface.h,v 1.9 2007-04-21 19:08:51 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2006 by Robert Fernie.
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


#include "rstypes.h"
#include "rscontrol.h"

#include <map>

class NotifyBase;
class RsIface;
class RsControl;
class RsInit;
struct TurtleFileInfo ;

/* declare single RsIface for everyone to use! */

// if it's declared here why does have the extern.TODO: To see it later

extern RsIface   *rsiface;
extern RsControl *rsicontrol;


RsIface   *createRsIface  (NotifyBase &notify);



class RsIface /* The Main Interface Class - create a single one! */
{
public:
    RsIface(NotifyBase &callback);
    virtual ~RsIface();

    /****************************************/

    /* Stubs for Very Important Fns -> Locking Functions */
    virtual void lockData() = 0;
    virtual void unlockData() = 0;

    const std::list<FileInfo> &getRecommendList();
    const RsConfig &getConfig();
    /****************************************/


    /* Flags to indicate used or not */
    enum DataFlags
    {
        Neighbour = 0,
        Friend = 1,
        DirLocal = 2,  /* Not Used - QModel instead */
        DirRemote = 3, /* Not Used - QModel instead */
        Transfer = 4,
        Message = 5,
        Channel = 6,
        Chat = 7,
        Recommend = 8,
        Config = 9,
        NumOfFlags = 10
    };


    /*
     * Operations for flags
     */

    bool setChanged(DataFlags set); /* set to true */
    bool getChanged(DataFlags set); /* leaves it */
    bool hasChanged(DataFlags set); /* resets it */

private:

    void fillLists(); /* create some dummy data to display */

    /* Internals */
    std::list<FileInfo> mRecommendList;

    bool mChanged[NumOfFlags];
    RsConfig mConfig;

    NotifyBase &cb;

    /* Classes which can update the Lists! */
    friend class RsControl;
    friend class RsServer;
};




#endif
