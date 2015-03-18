#include "GetPluginInterfaces.h"

#include <retroshare/rsplugin.h>

#include <retroshare/rsmsgs.h>
#include <retroshare/rsturtle.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rsdht.h>
#include <retroshare/rsnotify.h>

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
