/*
 * pqithreadstreamer.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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


#include "pqi/pqithreadstreamer.h"
#include <unistd.h>

#define DEFAULT_STREAMER_TIMEOUT	  10000 // 10 ms.
#define DEFAULT_STREAMER_SLEEP		   1000 // 1 ms.
#define DEFAULT_STREAMER_IDLE_SLEEP	1000000 // 1 sec

//#define PQISTREAMER_DEBUG

pqithreadstreamer::pqithreadstreamer(PQInterface *parent, RsSerialiser *rss, const RsPeerId& id, BinInterface *bio_in, int bio_flags_in)
:pqistreamer(rss, id, bio_in, bio_flags_in), mParent(parent), mThreadMutex("pqithreadstreamer"),  mTimeout(0)
{
	mTimeout = DEFAULT_STREAMER_TIMEOUT;
    mSleepPeriod = DEFAULT_STREAMER_SLEEP;

    sem_init(&mShouldStopSemaphore,0,0) ;
    sem_init(&mHasStoppedSemaphore,0,1) ;
}

bool pqithreadstreamer::RecvItem(RsItem *item)
{
	return mParent->RecvItem(item);
}

int	pqithreadstreamer::tick()
{
        RsStackMutex stack(mThreadMutex);
    tick_bio();

	return 0;
}

void pqithreadstreamer::start()
{
//    mToRun = true;

#ifdef PQISTREAMER_DEBUG
    std::cerr << "pqithreadstreamer::run()" << std::endl;
    std::cerr << "  initing should_stop=0" << std::endl;
    std::cerr << "  initing has_stopped=1" << std::endl;
#endif

    sem_init(&mShouldStopSemaphore,0,0) ;
    sem_init(&mHasStoppedSemaphore,0,0) ;

    RsThread::start();
}

void pqithreadstreamer::run()
{
#ifdef PQISTREAMER_DEBUG
    std::cerr << "pqithreadstream::run()";
    std::cerr << std::endl;
#endif

    // tell the OS to free the thread resources when this function exits
    // it is a replacement for pthread_join()
    pthread_detach(pthread_self());

    while(1)
    {
        int sval =0;
        sem_getvalue(&mShouldStopSemaphore,&sval) ;

        if(sval > 0)
        {
#ifdef PQISTREAMER_DEBUG
            std::cerr << "pqithreadstreamer::run(): asked to stop." << std::endl;
            std::cerr << "  setting hasStopped=1" << std::endl;
#endif
            sem_post(&mHasStoppedSemaphore) ;
            return ;
        }

        data_tick();
    }
}

void pqithreadstreamer::shutdown()
{
#ifdef PQISTREAMER_DEBUG
    std::cerr << "pqithreadstreamer::stop()" << std::endl;
#endif

    int sval =0;
    sem_getvalue(&mHasStoppedSemaphore,&sval) ;

    if(sval > 0)
    {
#ifdef PQISTREAMER_DEBUG
        std::cerr << "  thread not running. Quit." << std::endl;
#endif
        return ;
    }

#ifdef PQISTREAMER_DEBUG
    std::cerr << "  calling stop" << std::endl;
#endif
    sem_post(&mShouldStopSemaphore) ;
}

void pqithreadstreamer::fullstop()
{
    shutdown() ;

#ifdef PQISTREAMER_DEBUG
    std::cerr << "  waiting stop" << std::endl;
#endif
    sem_wait(&mHasStoppedSemaphore) ;
#ifdef PQISTREAMER_DEBUG
    std::cerr << "  finished!" << std::endl;
#endif
}

int	pqithreadstreamer::data_tick()
{
	uint32_t recv_timeout = 0;
	uint32_t sleep_period = 0;
	bool isactive = false;
	{
		RsStackMutex stack(mStreamerMtx);
		recv_timeout = mTimeout;
		sleep_period = mSleepPeriod;
        	isactive = mBio->isactive();
	}

	if (!isactive)
	{
		usleep(DEFAULT_STREAMER_IDLE_SLEEP);
		return 0;
	}

    {
        RsStackMutex stack(mThreadMutex);
    tick_recv(recv_timeout);
    }

	// Push Items, Outside of Mutex.
	RsItem *incoming = NULL;
	while((incoming = GetItem()))
	{
		RecvItem(incoming);
	}

    {
        RsStackMutex stack(mThreadMutex);
    tick_send(0);
    }

	if (sleep_period)
	{
		usleep(sleep_period);
	}
	return 1;
}




