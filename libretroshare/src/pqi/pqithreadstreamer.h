/*******************************************************************************
 * libretroshare/src/pqi: pqithreadstreamer.h                                  *
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
#ifndef MRK_PQI_THREAD_STREAMER_HEADER
#define MRK_PQI_THREAD_STREAMER_HEADER

#include "pqi/pqistreamer.h"
#include "util/rsthreads.h"

class pqithreadstreamer: public pqistreamer, public RsTickingThread
{
public:
    pqithreadstreamer(PQInterface *parent, RsSerialiser *rss, const RsPeerId& peerid, BinInterface *bio_in, int bio_flagsin);

    // from pqistreamer
    virtual bool RecvItem(RsItem *item) override;
    virtual int  tick() override;

protected:
	void threadTick() override; /// @see RsTickingThread

    PQInterface *mParent;
    uint32_t mTimeout;
    uint32_t mSleepPeriod;

private:
    /* thread variables */
    RsMutex mThreadMutex;
};

#endif //MRK_PQI_THREAD_STREAMER_HEADER
