
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

#include <sys/time.h>
#include <time.h>

RsServer::RsServer(RsIface &i, NotifyBase &callback)
	:RsControl(i, callback)
{
	return;
}

RsServer::~RsServer()
{
	return;
}

        /* General Internal Helper Functions 
	  ----> MUST BE LOCKED! 
         */

#if 0
cert   *RsServer::intFindCert(RsCertId id)
{
	certsign cs;
	convert_to_certsign(id, cs);
	return sslr -> findcertsign(cs);
}

RsCertId RsServer::intGetCertId(cert *c)
{
	certsign cs;
	sslr -> getcertsign(c, cs);
	RsCertId id = convert_to_str(cs);
	return id;
}
#endif

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
    double maxTimeDelta = 2.0;
    double kickLimit = 0.5;

    double avgTickRate = timeDelta;

    double lastts, ts;
    lastts = ts = getCurrentTS();

    long   lastSec = 0; /* for the slower ticked stuff */

    int min = 0;
    int loop = 0;

    while(1)
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
		//std::cerr << "Delta: " << delta << std::endl;
		//std::cerr << "Time Delta: " << timeDelta << std::endl;
		//std::cerr << "Avg Tick Rate: " << avgTickRate << std::endl;

		lastts = ts;

/******************************** RUN SERVER *****************/
		lockRsCore();

	//	std::cerr << "RsServer::run() Lock() -> Run()" << std::endl;
		int moreToTick = server -> tick();

		unlockRsCore();

		/* tick the connection Manager */
		mConnMgr->tick();
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

			/* Chat needs to be quick too! */
			//std::cerr << "RsServer::run() UpdateAllChat()" << std::endl;
			UpdateAllChat();

	
			// every five loops (> 5 secs)
			if (loop % 5 == 0)
			{
				//	update_quick_stats();

				// Update All Every 5 Seconds.
				// These Update Functions do the locking themselves.
				//std::cerr << "RsServer::run() Updates()" << std::endl;
			
				// These two have been completed!	
				//std::cerr << "RsServer::run() UpdateAllCerts()" << std::endl;
				//UpdateAllCerts();
				//std::cerr << "RsServer::run() UpdateAllNetwork()" << std::endl;
				//UpdateAllNetwork();

				// currently Dummy Functions.
				//std::cerr << "RsServer::run() UpdateAllTransfers()" << std::endl;
				UpdateAllTransfers();

				//std::cerr << "RsServer::run() UpdateAllMsgs()" << std::endl;
				UpdateAllMsgs();
				//std::cerr << "RsServer::run() UpdateAllChannels()" << std::endl;
				UpdateAllChannels();

			        //std::cerr << "RsServer::run() ";
				//std::cerr << "UpdateRemotePeople()"<<std::endl;
				//UpdateRemotePeople();
	
				//std::cerr << "RsServer::run() UpdateAllFiles()" << std::endl;
				//UpdateAllFiles();

				//std::cerr << "RsServer::run() UpdateAllConfig()" << std::endl;
				UpdateAllConfig();



				//std::cerr << "RsServer::run() CheckDHT()" << std::endl;
				//CheckNetworking();

			}
	
			// every 60 loops (> 1 min)
			if (++loop >= 60)
			{
				loop = 0;

				// save the config every 5 minutes.
				if (min % 5 == 1)
				{
					//ConfigSave();
#ifdef PQI_USE_CHANNELS
					/* hack to update for now 
					 * Only occassionally - cos disabled
					 */
					// channel_list_ok = false;
					// update_channels();
#endif


					//std::cerr << "RsServer::run() UpdateAllFiles()" << std::endl;
					//UpdateAllFiles();
				}
	
				/* hour loop */
				if (++min >= 60)
				{
					min = 0;
				}
				// update_dirlist();
			}

			// slow update tick as well.
			// update();
		} // end of slow tick.

	} // end of only once a second.

	// update graphics.. ( but only if gui is visible )
	// This is also triggered in slow tick...
	//if ((ui->main_win->shown()) || (ui->chatter_window->shown()))
	{
	//	update();
	}
     }
     return;
}


