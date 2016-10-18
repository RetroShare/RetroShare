
/*
 * "$Id: p3face-server.cc,v 1.5 2007-04-15 18:45:23 rmf24 Exp $"
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


#include "rsserver/p3face.h"
#include "retroshare/rsplugin.h"

#include "tcponudp/tou.h"
#include <unistd.h>

#include "pqi/authssl.h"
#include <sys/time.h>
#include <time.h>

#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"

int rsserverzone = 101;

#include "util/rsdebug.h"


/****
#define DEBUG_TICK 1
****/

#define WARN_BIG_CYCLE_TIME	(0.2)
#ifdef WINDOWS_SYS
#include <time.h>
#include <sys/timeb.h>
#endif


// These values should be tunable from the GUI, to offer a compromise between speed and CPU use.
// In some cases (VOIP) it's likely that we will need to set them temporarily to a very low 
// value, in order to favor a fast feedback

RsServer::RsServer()
    : coreMutex("RsServer")
{
	// This is needed asap.
	//
	mNotify = new p3Notify() ;
	rsNotify = mNotify ;

	mPeerMgr = NULL;
	mLinkMgr = NULL;
	mNetMgr = NULL;
	mHistoryMgr = NULL;

	pqih = NULL;

	mPluginsManager = NULL;

	/* services */
	mHeart = NULL;
	mDisc = NULL;
	msgSrv = NULL;
	chatSrv = NULL;
	mStatusSrv = NULL;
    mGxsTunnels = NULL;

    mMin = 0;
    mLoop = 0;


    mLastts = getCurrentTS();
    mLastSec = 0; /* for the slower ticked stuff */
    mTimeDelta = 0.25 ;

    mAvgTickRate = mTimeDelta;

    /* caches (that need ticking) */

	/* Config */
	mConfigMgr = NULL;
	mGeneralConfig = NULL;
}

RsServer::~RsServer()
{
	return;
}

        /* General Internal Helper Functions 
	  ----> MUST BE LOCKED! 
         */



        /* Thread Fn: Run the Core */
int	RsServer::tick()
{
    int moreToTick ;

    {
        RS_STACK_MUTEX(coreMutex) ;
        moreToTick = pqih->tick();
    }

    /* tick the Managers */
    mPeerMgr->tick();
    mLinkMgr->tick();
    mNetMgr->tick();

    time_t ts = time(NULL) ;

    /* now we have the slow ticking stuff */
    /* stuff ticked once a second (but can be slowed down) */
    if ((int) ts > mLastSec)
    {
        mLastSec = (int) ts;

        // Every second! (UDP keepalive).
        //tou_tick_stunkeepalive();

        // every five loops (> 5 secs)
        if (mLoop % 5 == 0)
        {
            // Update All Every 5 Seconds.
            // These Update Functions do the locking themselves.
#ifdef	DEBUG_TICK
            std::cerr << "RsServer::run() Updates()" << std::endl;
#endif
            mConfigMgr->tick(); /* saves stuff */
        }

        /* Tick slow services */
        if(rsPlugins)
            rsPlugins->slowTickPlugins((time_t)ts);
    }

#ifdef	DEBUG_TICK
    double endCycleTs = getCurrentTS();
    double cycleTime = endCycleTs - ts;
    if (cycleTime > WARN_BIG_CYCLE_TIME)
    {
        std::string out;
        rs_sprintf(out, "RsServer::run() WARNING Excessively Long Cycle Time: %g secs => Please DEBUG", cycleTime);
        std::cerr << out << std::endl;

        rslog(RSL_ALERT, rsserverzone, out);
    }
#endif

    return moreToTick ;
}
