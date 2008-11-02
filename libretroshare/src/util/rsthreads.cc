
/*
 * "$Id: rsthreads.cc,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2007 by Robert Fernie.
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


#include "rsthreads.h"
#include <unistd.h>    /* for usleep() */

/*******
 * #define DEBUG_THREADS 1
 *******/

#ifdef DEBUG_THREADS
	#include <iostream>
#endif

extern "C" void* rsthread_init(void* p)
{
  RsThread *thread = (RsThread *) p;
  if (!thread)
  {
    return 0;
  }
  thread -> run();
  return 0;
}


pthread_t  createThread(RsThread &thread)
{
    pthread_t tid;
    void  *data = (void *) (&thread);

    thread.mMutex.lock();
    {
      pthread_create(&tid, 0, &rsthread_init, data);
      thread.mTid = tid;
    }
    thread.mMutex.unlock();

    return tid;

}


RsQueueThread::RsQueueThread(uint32_t min, uint32_t max, double relaxFactor )
	:mMinSleep(min), mMaxSleep(max), mRelaxFactor(relaxFactor)
{


}

void RsQueueThread::run()
{
	while(1)
	{
		bool doneWork = false;
		while(workQueued() && doWork())
		{
			doneWork = true;
		}
		time_t now = time(NULL);
		if (doneWork)
		{
			mLastWork = now;
			mLastSleep = (uint32_t) 
				(mMinSleep + (mLastSleep - mMinSleep) / 2.0);
#ifdef DEBUG_THREADS
			std::cerr << "RsQueueThread::run() done work: sleeping for: " << mLastSleep;
			std::cerr << " ms";
			std::cerr << std::endl;
#endif

		}
		else
		{
			uint32_t deltaT = now - mLastWork;
			double frac = deltaT / mRelaxFactor;
			
			mLastSleep += (uint32_t) 
				((mMaxSleep-mMinSleep) * (frac + 0.05));
			if (mLastSleep > mMaxSleep)
			{
				mLastSleep = mMaxSleep;
			}
#ifdef DEBUG_THREADS
			std::cerr << "RsQueueThread::run() no work: sleeping for: " << mLastSleep;
			std::cerr << " ms";
			std::cerr << std::endl;
#endif
		}
#ifdef WIN32
		Sleep(mLastSleep);
#else
		usleep(1000 * mLastSleep);
#endif
	}
}

