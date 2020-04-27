/*******************************************************************************
 * libretroshare/src/rsserver: p3face-server.cc                                *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2015 by Robert Fernie   <retroshare.project@gmail.com>            *
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

#include "util/rstime.h"
#include "rsserver/p3face.h"
#include "retroshare/rsplugin.h"

#include "tcponudp/tou.h"
#include <unistd.h>

#include "pqi/authssl.h"
#include <sys/time.h>
#include "util/rstime.h"

#include "pqi/p3peermgr.h"
#include "pqi/p3linkmgr.h"
#include "pqi/p3netmgr.h"

#include "util/rsdebug.h"

#include "retroshare/rsevents.h"
#include "services/rseventsservice.h"


/****
#define DEBUG_TICK 1
****/

#define WARN_BIG_CYCLE_TIME	(0.2)
#ifdef WINDOWS_SYS
#include "util/rstime.h"
#include <sys/timeb.h>
#endif


/*extern*/ RsControl* rsControl = nullptr;

static double getCurrentTS()
{

#ifndef WINDOWS_SYS
        struct timeval cts_tmp;
        gettimeofday(&cts_tmp, NULL);
        double cts =  (cts_tmp.tv_sec) + ((double) cts_tmp.tv_usec) / 1000000.0;
#else
        struct _timeb timebuf;
        _ftime( &timebuf);
        double cts =  (timebuf.time) + ((double) timebuf.millitm) / 1000.0;
#endif
        return cts;
}

// These values should be tunable from the GUI, to offer a compromise between speed and CPU use.
// In some cases (VOIP) it's likely that we will need to set them temporarily to a very low 
// value, in order to favor a fast feedback

const double RsServer::minTickInterval = 0.05;
const double RsServer::maxTickInterval = 0.2;


RsServer::RsServer() :
	coreMutex("RsServer"), mShutdownCallback([](int){}),
	coreReady(false)
{
	{
		RsEventsService* tmpRsEvtPtr = new RsEventsService();
		rsEvents = tmpRsEvtPtr;
		startServiceThread(tmpRsEvtPtr, "RsEventsService");
	}

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

	mLastts = getCurrentTS();
	mTickInterval = maxTickInterval ;
	mAvgRunDuration = 0;
	mLastRunDuration = 0;

	/* caches (that need ticking) */

	/* config */
	mConfigMgr = NULL;
	mGeneralConfig = NULL;
}

RsServer::~RsServer()
{
	delete mGxsTrans;
}

// General Internal Helper Functions  ----> MUST BE LOCKED! 

void RsServer::threadTick()
{
RsDbg() << "DEBUG_TICK" << std::endl;
RsDbg() << "DEBUG_TICK ticking interval "<< mTickInterval << std::endl;

// we try to tick at a regular interval which depends on the load 
// if there is time left, we sleep
        double timeToSleep = mTickInterval - mAvgRunDuration;

	if (timeToSleep > 0)
	{
RsDbg() << "DEBUG_TICK will sleep " << timeToSleep << " ms" << std::endl;
		rstime::rs_usleep(timeToSleep * 1000000);
	}

	double ts = getCurrentTS();
	double delta = ts - mLastts;
	mLastts = ts;

// stuff we do always
RsDbg() << "DEBUG_TICK ticking server" << std::endl;
	lockRsCore();
	int moreToTick = pqih->tick();
	unlockRsCore();

// tick the managers
RsDbg() << "DEBUG_TICK ticking mPeerMgr" << std::endl;
        mPeerMgr->tick();
RsDbg() << "DEBUG_TICK ticking mLinkMgr" << std::endl;
        mLinkMgr->tick();
RsDbg() << "DEBUG_TICK ticking mNetMgr" << std::endl;
        mNetMgr->tick();


// stuff we do every second
        if (delta > 1)
        {
RsDbg() << "DEBUG_TICK every second" << std::endl;
		// slow services
		if (rsPlugins)
			rsPlugins->slowTickPlugins((rstime_t)ts);
		// UDP keepalive
		// tou_tick_stunkeepalive();
		// other stuff to tick
		// update();
	}

// stuff we do every five seconds
	if (delta > 5)
	{
RsDbg() << "DEBUG_TICK every 5 seconds" << std::endl;
		// save stuff
		mConfigMgr->tick();
	}

// stuff we do every minute
	if (delta > 60)
	{
RsDbg() << "DEBUG_TICK 60 seconds" << std::endl;
		// force saving FileTransferStatus TODO
                // ftserver->saveFileTransferStatus();
		// see if we need to resave certs
                // AuthSSL::getAuthSSL()->CheckSaveCertificates();
	}

// stuff we do every hour
	if (delta > 3600)
	{
RsDbg() << "DEBUG_TICK every hour" << std::endl;
	}

// ticking is done, now compute new values of mLastRunDuration, mAvgRunDuration and mTickInterval
        ts = getCurrentTS();
        mLastRunDuration = ts - mLastts;  
        mAvgRunDuration = 0.1 * mLastRunDuration + 0.9 * mAvgRunDuration;

RsDbg() << "DEBUG_TICK new mLastRunDuration " << mLastRunDuration << " mAvgRunDuration " << mAvgRunDuration << std::endl;
        if (mLastRunDuration > WARN_BIG_CYCLE_TIME)
                RsDbg() << "DEBUG_TICK excessively long lycle time " << mLastRunDuration << std::endl;
	
// if the core has returned that there is more to tick we decrease the ticking interval, else we increse it
RsDbg() << "DEBUG_TICK moreToTick " << moreToTick << std::endl;
	if (moreToTick == 1)
		mTickInterval = 0.9 * mTickInterval;
        else
		mTickInterval = 1.1 * mTickInterval;
RsDbg() << "DEBUG_TICK new tick interval " << mTickInterval << std::endl;

// keep the tick interval within allowed limits
        if (mTickInterval < minTickInterval)
		mTickInterval = minTickInterval;
        else if (mTickInterval > maxTickInterval)
		mTickInterval = maxTickInterval;
RsDbg() << "DEBUG_TICK new tick interval after limiter " << mTickInterval << std::endl;
}

