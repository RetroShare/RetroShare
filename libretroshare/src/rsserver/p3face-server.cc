
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


RsServer::RsServer(NotifyBase &callback)
	:RsControl(callback), coreMutex("RsServer")
{
	//ftserver = NULL;

	mPeerMgr = NULL;
	mLinkMgr = NULL;
	mNetMgr = NULL;

	pqih = NULL;

	mPluginsManager = NULL;

	/* services */
	mHeart = NULL;
	mDisc = NULL;
	msgSrv = NULL;
	chatSrv = NULL;
	mStatusSrv = NULL;
	/* caches (that need ticking) */

	/* Config */
	mConfigMgr = NULL;
	mGeneralConfig = NULL;

        /* GXS - Amazingly we can still initialise these
         * even without knowing the data-types (they are just pointers???)
	 */
        mPhoto = NULL;
        mWiki = NULL;
        mPosted = NULL;
        mGxsCircles = NULL;
        mGxsIdService = NULL;
        mGxsForums = NULL;
        mWire = NULL;

}

RsServer::~RsServer()
{
	return;
}

        /* General Internal Helper Functions 
	  ----> MUST BE LOCKED! 
         */

#ifdef WINDOWS_SYS
#include <time.h>
#include <sys/timeb.h>
#endif

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


        /* Thread Fn: Run the Core */
void 	RsServer::run()
{

	double timeDelta = 0.25;
	double minTimeDelta = 0.1; // 25;
	double maxTimeDelta = 0.5;
	double kickLimit = 0.15;

	double avgTickRate = timeDelta;

	double lastts, ts;
	lastts = ts = getCurrentTS();

	long   lastSec = 0; /* for the slower ticked stuff */

	int min = 0;
	int loop = 0;

	while(isRunning())
	{
#ifndef WINDOWS_SYS
                usleep((int) (timeDelta * 1000000));
#else
                Sleep((int) (timeDelta * 1000));
#endif

		ts = getCurrentTS();
		double delta = ts - lastts;

		/* for the fast ticked stuff */
		if (delta > timeDelta) 
		{
#ifdef	DEBUG_TICK
			std::cerr << "Delta: " << delta << std::endl;
			std::cerr << "Time Delta: " << timeDelta << std::endl;
			std::cerr << "Avg Tick Rate: " << avgTickRate << std::endl;
#endif

			lastts = ts;

			/******************************** RUN SERVER *****************/
			lockRsCore();

			//int moreToTick = ftserver -> tick();
			int moreToTick = pqih->tick();

#ifdef	DEBUG_TICK
			std::cerr << "RsServer::run() ftserver->tick(): moreToTick: " << moreToTick << std::endl;
#endif

			unlockRsCore();

			/* tick the Managers */
			mPeerMgr->tick();
			mLinkMgr->tick();
			mNetMgr->tick();
			/******************************** RUN SERVER *****************/

			/* adjust tick rate depending on whether there is more.
			 */

			avgTickRate = 0.2 * timeDelta + 0.8 * avgTickRate;

			if (1 == moreToTick)
			{
				timeDelta = 0.9 * avgTickRate;
				if (timeDelta > kickLimit)
				{
					/* force next tick in one sec
					 * if we are reading data.
					 */
					timeDelta = kickLimit;
					avgTickRate = kickLimit;
				}
			}
			else
			{
				timeDelta = 1.1 * avgTickRate;
			}

			/* limiter */
			if (timeDelta < minTimeDelta)
			{
				timeDelta = minTimeDelta;
			}
			else if (timeDelta > maxTimeDelta)
			{
				timeDelta = maxTimeDelta;
			}

			/* Fast Updates */


			/* now we have the slow ticking stuff */
			/* stuff ticked once a second (but can be slowed down) */
			if ((int) ts > lastSec)
			{
				lastSec = (int) ts;

				// Every second! (UDP keepalive).
				//tou_tick_stunkeepalive();

				// every five loops (> 5 secs)
				if (loop % 5 == 0)
				{
					//	update_quick_stats();

					// Update All Every 5 Seconds.
					// These Update Functions do the locking themselves.
#ifdef	DEBUG_TICK
					std::cerr << "RsServer::run() Updates()" << std::endl;
#endif

					mConfigMgr->tick(); /* saves stuff */

				}

				// every 60 loops (> 1 min)
				if (++loop >= 60)
				{
					loop = 0;

					/* force saving FileTransferStatus TODO */
					//ftserver->saveFileTransferStatus();

					/* see if we need to resave certs */
					//AuthSSL::getAuthSSL()->CheckSaveCertificates();

					/* hour loop */
					if (++min >= 60)
					{
						min = 0;
					}
				}

				/* Tick slow services */
				if(rsPlugins)
					rsPlugins->slowTickPlugins((time_t)ts);

				// slow update tick as well.
				// update();
			} // end of slow tick.

		} // end of only once a second.

		double endCycleTs = getCurrentTS();
		double cycleTime = endCycleTs - ts;
		if (cycleTime > WARN_BIG_CYCLE_TIME)
		{
			std::string out;
			rs_sprintf(out, "RsServer::run() WARNING Excessively Long Cycle Time: %g secs => Please DEBUG", cycleTime);
			std::cerr << out << std::endl;

			rslog(RSL_ALERT, rsserverzone, out);
		}
	}
	return;
}
