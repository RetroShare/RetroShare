/*******************************************************************************
 * libretroshare/src/pqi: pqithreadstreamer.cc                                 *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2013 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include "pqi/pqithreadstreamer.h"
#include <unistd.h>

#define DEFAULT_STREAMER_TIMEOUT	  10000 // 10 ms.
#define DEFAULT_STREAMER_SLEEP		   1000 // 1 ms.
#define DEFAULT_STREAMER_IDLE_SLEEP	1000000 // 1 sec

//#define PQISTREAMER_DEBUG

pqithreadstreamer::pqithreadstreamer(PQInterface *parent, RsSerialiser *rss, const RsPeerId& id, BinInterface *bio_in, int bio_flags_in)
:pqistreamer(rss, id, bio_in, bio_flags_in), mParent(parent), mTimeout(0), mThreadMutex("pqithreadstreamer")
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
//	pqithreadstreamer mutex lock is not needed here
//	we are only checking if the connection is active, and if not active we will try to establish it
//	RsStackMutex stack(mThreadMutex);
	tick_bio();

	return 0;
}

void	pqithreadstreamer::threadTick()
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
    
    updateRates() ;

    if (!isactive)
    {
        rstime::rs_usleep(DEFAULT_STREAMER_IDLE_SLEEP);
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
        rstime::rs_usleep(sleep_period);
    }
}




