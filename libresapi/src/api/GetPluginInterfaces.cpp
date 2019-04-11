/*******************************************************************************
 * libresapi/api/GetPluginInterfaces.cpp                                       *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright 2018 by Retroshare Team <retroshare.project@gmail.com>            *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include "GetPluginInterfaces.h"

#include <retroshare/rsplugin.h>

#include <retroshare/rsmsgs.h>
#include <retroshare/rsturtle.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsdht.h>
#include <retroshare/rsnotify.h>
#include <retroshare/rsservicecontrol.h>

#include <retroshare/rsidentity.h>
#include <retroshare/rsgxscircles.h>
#include <retroshare/rsgxsforums.h>
#include <retroshare/rsgxschannels.h>

namespace resource_api{

bool getPluginInterfaces(RsPlugInInterfaces& interfaces)
{
    // when rsPlugins is null, then rs was not started
    if(rsPlugins == 0)
        return false;

    interfaces.mFiles  = rsFiles;
    interfaces.mPeers  = rsPeers;
    interfaces.mMsgs   = rsMsgs;
    interfaces.mTurtle = rsTurtle;
    interfaces.mDisc   = rsDisc;
    interfaces.mDht    = rsDht;
    interfaces.mNotify = rsNotify;
    interfaces.mServiceControl = rsServiceControl;
    interfaces.mPluginHandler  = rsPlugins;

    // gxs
    interfaces.mGxsDir          = "";
    interfaces.mIdentity        = rsIdentity;
    // not exposed with global variable, can't get it
    interfaces.mRsNxsNetMgr     = 0;
    // same as identity service, but different interface
    interfaces.mGxsIdService    = 0;
    //
    interfaces.mGxsCirlces      = 0;
    // not exposed with global variable
    interfaces.mPgpAuxUtils     = 0;
    interfaces.mGxsForums       = rsGxsForums;
    interfaces.mGxsChannels     = rsGxsChannels;
    return true;
}

} // namespace resource_api
