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

void	pqithreadstreamer::data_tick()
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
        return ;
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
}




