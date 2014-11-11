
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
#include <unistd.h>    // for usleep()
#include <errno.h>    // for errno
#include <iostream>
#include <time.h>

#ifdef RSMUTEX_DEBUG
#include <stdio.h>
#include <sys/time.h>
#endif

/*******
 * #define DEBUG_THREADS 1
 * #define RSMUTEX_ABORT 1  // Catch wrong pthreads mode.
 *******/

#ifdef RSMUTEX_ABORT
	#include <stdlib.h>
#endif

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

#if 0
	int ret;
	ret = pthread_attr_init(&tattr);
	if (doDetached)
	{
		ret = pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED);
	}
	else
	{
		ret = pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_JOINABLE);
	}

      	pthread_create(&tid, &tattr, &rsthread_init, data);
#endif

			int err ;

			if( 0 == (err=pthread_create(&tid, 0, &rsthread_init, data))) 
				thread.mTid = tid;
			else
				std::cerr << "Fatal error: pthread_create could not create a thread. Error returned: " << err << " !!!!!!!" << std::endl;
    }
    thread.mMutex.unlock();

    return tid;

}

RsThread::RsThread () : mMutex("RsThread")
{
	mIsRunning = true;

#ifdef WINDOWS_SYS
    memset (&mTid, 0, sizeof(mTid));
#else
    mTid = 0;
#endif
}

void RsThread::join() /* waits for the the mTid thread to stop */
{
	// do we need a mutex for this ?
	mIsRunning = false;

	void *ptr;
	pthread_join(mTid, &ptr);
}

void RsThread::stop() 
{
	pthread_exit(NULL);
}

bool RsThread::isRunning()
{
	// do we need a mutex for this ?
	return mIsRunning;
}

RsQueueThread::RsQueueThread(uint32_t min, uint32_t max, double relaxFactor )
	:mMinSleep(min), mMaxSleep(max), mRelaxFactor(relaxFactor)
{
	mLastSleep = (uint32_t)mMinSleep ;
	mLastWork = time(NULL) ;
}

void RsQueueThread::run()
{
	while(isRunning())
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
			mLastSleep = (uint32_t) (mMinSleep + (mLastSleep - mMinSleep) / 2.0);
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
		usleep(mLastSleep * 1000); // mLastSleep msec
	}
}

void RsMutex::unlock()
{ 
#ifdef RSTHREAD_SELF_LOCKING_GUARD
	if(--_cnt == 0)
	{
#endif
#ifndef WIN32
		_thread_id = 0 ;
#endif
		pthread_mutex_unlock(&realMutex); 

#ifdef RSTHREAD_SELF_LOCKING_GUARD
	}
#endif
}

void RsMutex::lock()
{
#ifdef RSMUTEX_DEBUG
	pthread_t owner = _thread_id ;
#endif

	int retval = 0;
#ifdef RSTHREAD_SELF_LOCKING_GUARD
	if(!trylock())
		if(!pthread_equal(_thread_id,pthread_self()))
#endif
			retval = pthread_mutex_lock(&realMutex);

	switch(retval)
	{
		case 0:
			break;

		case EINVAL:
			std::cerr << "RsMutex::lock() pthread_mutex_lock returned EINVAL";
			std::cerr << std::endl;
			break;
 
		case EBUSY:
			std::cerr << "RsMutex::lock() pthread_mutex_lock returned EBUSY";
			std::cerr << std::endl;
			break;
 
		case EAGAIN:
			std::cerr << "RsMutex::lock() pthread_mutex_lock returned EAGAIN";
			std::cerr << std::endl;
			break;
 
		case EDEADLK:
			std::cerr << "RsMutex::lock() pthread_mutex_lock returned EDEADLK";
			std::cerr << std::endl;
			break;
 
		case EPERM:
			std::cerr << "RsMutex::lock() pthread_mutex_lock returned EPERM";
			std::cerr << std::endl;
			break;

		default:
			std::cerr << "RsMutex::lock() pthread_mutex_lock returned UNKNOWN ERROR";
			std::cerr << std::endl;
			break;
	}

	/* Here is some debugging code - to catch failed locking attempts.
	 * Major bug is it is ever triggered.
	 */
#ifdef RSMUTEX_ABORT

	if (retval != 0)
	{
#ifdef RSMUTEX_DEBUG
		std::cerr << "RsMutex::lock() name: " << name << std::endl;
#endif
		std::cerr << "RsMutex::lock() pthread_mutex_lock returned an Error. Aborting()";
		std::cerr << std::endl;
		abort();
	}
#endif

 
	_thread_id = pthread_self() ;
#ifdef RSTHREAD_SELF_LOCKING_GUARD
	++_cnt ;
#endif
}
#ifdef RSMUTEX_DEBUG
double RsStackMutex::getCurrentTS()
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
#endif


