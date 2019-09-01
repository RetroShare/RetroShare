/*******************************************************************************
 * libretroshare/src/util: rsthreads.cc                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2007 by Robert Fernie <retroshare@lunamutt.com>              *
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

#include "rsthreads.h"
#include <unistd.h>    // for usleep()
#include <errno.h>    // for errno
#include <iostream>
#include "util/rstime.h"
#include "util/rsdebug.h"

#include "util/rstime.h"

#ifdef __APPLE__
int __attribute__((weak)) pthread_setname_np(const char *__buf) ;
int RS_pthread_setname_np(pthread_t /*__target_thread*/, const char *__buf) {
	return pthread_setname_np(__buf);
}
#else
int __attribute__((weak)) pthread_setname_np(pthread_t __target_thread, const char *__buf) ;
int RS_pthread_setname_np(pthread_t __target_thread, const char *__buf) {
	return pthread_setname_np(__target_thread, __buf);
}
#endif

#ifdef RSMUTEX_DEBUG
#include <stdio.h>
#include <sys/time.h>
#endif

/*******
 * #define DEBUG_THREADS 1
 * #define RSMUTEX_ABORT 1  // Catch wrong pthreads mode.
 *******/
#define THREAD_DEBUG std::cerr << "[this=" << (void*)this << ", caller thread ID: " << std::hex << pthread_self() << ", thread ID: " << mTid << std::dec << "] "

#ifdef RSMUTEX_ABORT
	#include <stdlib.h>
#endif

#ifdef DEBUG_THREADS
	#include <iostream>
#endif

void RsThread::go()
{
    mShouldStopSemaphore.set(0) ;
    mHasStoppedSemaphore.set(0) ;

    runloop();

    mShouldStopSemaphore.set(0);
    mHasStoppedSemaphore.set(1);	// last value that we modify because this is interpreted as a signal that the object can be deleted.
}
void *RsThread::rsthread_init(void* p)
{
  RsThread *thread = (RsThread *) p;
  if (!thread)
  {
    return NULL;
  }
    // tell the OS to free the thread resources when this function exits
    // it is a replacement for pthread_join()
    pthread_detach(pthread_self());

#ifdef DEBUG_THREADS
    std::cerr << "[Thread ID:" << std::hex << pthread_self() << std::dec << "] thread is started. Calling runloop()..." << std::endl;
#endif
    
  thread->go();
  return NULL;
}
RsThread::RsThread() 
{
#ifdef WINDOWS_SYS
    memset (&mTid, 0, sizeof(mTid));
#else
    mTid = 0;
#endif
    // The thread is certainly not running. This avoids to lock down when calling shutdown on a thread that has never started.

#ifdef DEBUG_THREADS
    THREAD_DEBUG << "[Thread ID:" << std::hex << pthread_self() << std::dec << "] thread object created. Initing stopped=1, should_stop=0" << std::endl;
#endif
    mHasStoppedSemaphore.set(1) ;
    mShouldStopSemaphore.set(0) ;
}

RsThread::~RsThread()
{
	if(isRunning())
    {
		RsErr() << "Deleting a thread that is still running! Something is very wrong here and Retroshare is likely to crash because of this." << std::endl;
        print_stacktrace();
    }
}

bool RsThread::isRunning()
{
    // do we need a mutex for this ?
    int sval = mHasStoppedSemaphore.value() ;

    return !sval ;
}

bool RsThread::shouldStop()
{
        int sval = mShouldStopSemaphore.value() ;
        return sval > 0;
}

void RsTickingThread::shutdown()
{
#ifdef DEBUG_THREADS
    THREAD_DEBUG << "pqithreadstreamer::shutdown()" << std::endl;
#endif

    int sval = mHasStoppedSemaphore.value() ;

    if(sval > 0)
    {
#ifdef DEBUG_THREADS
        THREAD_DEBUG << "  thread not running. Quit." << std::endl;
#endif
        return ;
    }

    ask_for_stop() ;
}

void RsThread::ask_for_stop()
{
#ifdef DEBUG_THREADS
    THREAD_DEBUG << "  calling stop" << std::endl;
#endif
    mShouldStopSemaphore.set(1);
}

void RsTickingThread::fullstop()
{
    shutdown() ;

#ifdef DEBUG_THREADS
    THREAD_DEBUG << "  waiting stop" << std::endl;
#endif
    if(pthread_equal(mTid,pthread_self()))
    {
        THREAD_DEBUG << "(WW) RsTickingThread::fullstop() called by same thread. This is unexpected." << std::endl;
        return ;
    }
    
    mHasStoppedSemaphore.wait_no_relock(); // Wait for semaphore value to become 1, but does not decrement it when obtained.
#ifdef DEBUG_THREADS
    THREAD_DEBUG << "  finished!" << std::endl;
#endif
}

void RsThread::start(const std::string &threadName)
{
	if(isRunning())
	{
		std::cerr << "(EE) RsThread \"" << threadName
		          << "\" is already running. Will not start twice!"
		          << std::endl;
		print_stacktrace();
		return;
	}
    pthread_t tid;
    void  *data = (void *)this ;

#ifdef DEBUG_THREADS
    THREAD_DEBUG << "pqithreadstreamer::start() initing should_stop=0" << std::endl;
#endif
    mShouldStopSemaphore.set(0) ;
	mHasStoppedSemaphore.set(0) ;

    int err ;

    // pthread_create is a memory barrier
    // -> the new thread will see mIsRunning() = true

    if( 0 == (err=pthread_create(&tid, 0, &rsthread_init, data)))
    {
        mTid = tid;

        // set name

        if(pthread_setname_np)
        {
            if(!threadName.empty())
            {
                // thread names are restricted to 16 characters including the terminating null byte
                if(threadName.length() > 15)
                {
#ifdef DEBUG_THREADS
                    THREAD_DEBUG << "RsThread::start called with to long name '" << threadName << "' truncating..." << std::endl;
#endif
                    RS_pthread_setname_np(mTid, threadName.substr(0, 15).c_str());
                } else {
                    RS_pthread_setname_np(mTid, threadName.c_str());
                }
            }
        }
    }
    else
    {
        THREAD_DEBUG << "Fatal error: pthread_create could not create a thread. Error returned: " << err << " !!!!!!!" << std::endl;
        mHasStoppedSemaphore.set(1) ;
    }
}



RsTickingThread::RsTickingThread()
{
#ifdef DEBUG_THREADS
    THREAD_DEBUG << "RsTickingThread::RsTickingThread()" << std::endl;
#endif
}

RsTickingThread::~RsTickingThread()
{
    fullstop();
}
void RsSingleJobThread::runloop()
{
    run() ;
}

void RsTickingThread::runloop()
{
#ifdef DEBUG_THREADS
    THREAD_DEBUG << "RsTickingThread::runloop(). Setting stopped=0" << std::endl;
#endif

    while(1)
    {
        if(shouldStop())
        {
#ifdef DEBUG_THREADS
            THREAD_DEBUG << "pqithreadstreamer::runloop(): asked to stop. setting hasStopped=1, and returning. Thread ends." << std::endl;
#endif
            return ;
        }

        data_tick();
    }
}

RsQueueThread::RsQueueThread(uint32_t min, uint32_t max, double relaxFactor )
    :mMinSleep(min), mMaxSleep(max), mRelaxFactor(relaxFactor)
{
    mLastSleep = (uint32_t)mMinSleep ;
    mLastWork = time(NULL) ;
}

void RsQueueThread::data_tick()
{
    bool doneWork = false;
    while(workQueued() && doWork())
    {
        doneWork = true;
    }
    rstime_t now = time(NULL);
    if (doneWork)
    {
        mLastWork = now;
        mLastSleep = (uint32_t) (mMinSleep + (mLastSleep - mMinSleep) / 2.0);
#ifdef DEBUG_TICKING
        THREAD_DEBUG << "RsQueueThread::data_tick() done work: sleeping for: " << mLastSleep << " ms" << std::endl;
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
#ifdef DEBUG_TICKING
        THREAD_DEBUG << "RsQueueThread::data_tick() no work: sleeping for: " << mLastSleep << " ms" << std::endl;
#endif
    }
    rstime::rs_usleep(mLastSleep * 1000); // mLastSleep msec
}

void RsMutex::unlock()
{ 
#ifdef RSTHREAD_SELF_LOCKING_GUARD
	if(--_cnt == 0)
	{
#endif
		_thread_id = 0 ;
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


