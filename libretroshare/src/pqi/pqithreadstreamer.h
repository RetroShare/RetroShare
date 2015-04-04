/*
 * libretroshare/src/pqi pqithreadstreamer.h
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

#ifndef MRK_PQI_THREAD_STREAMER_HEADER
#define MRK_PQI_THREAD_STREAMER_HEADER

#include <semaphore.h>
#include "pqi/pqistreamer.h"
#include "util/rsthreads.h"

class pqithreadstreamer: public pqistreamer, private RsThread
{
	public:
		pqithreadstreamer(PQInterface *parent, RsSerialiser *rss, const RsPeerId& peerid, BinInterface *bio_in, int bio_flagsin);

virtual void start(); 
virtual void shutdown();
virtual void fullstop(); 

virtual bool RecvItem(RsItem *item);
virtual int  tick();

protected:
    // from RsThread
    virtual void run();

    int  data_tick();

	PQInterface *mParent;
	uint32_t mTimeout;
	uint32_t mSleepPeriod;

private:
	/* thread variables */
	RsMutex mThreadMutex;
    sem_t mShouldStopSemaphore;
    sem_t mHasStoppedSemaphore;
};

#endif //MRK_PQI_THREAD_STREAMER_HEADER
