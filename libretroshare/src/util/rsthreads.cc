/*******************************************************************************
 * libretroshare/src/util: rsthreads.cc                                        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2007  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2016-2020  Gioacchino Mazzurco <gio@eigenlab.org>             *
 * Copyright (C) 2019-2020  Asociación Civil Altermundi <info@altermundi.net>  *
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

#include "util/rsdebug.h"
#include "util/rserrno.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <thread>

#ifdef RS_MUTEX_DEBUG
#include <cstdio>
#include <sys/time.h>
#endif

#ifdef __APPLE__
int __attribute__((weak)) pthread_setname_np(const char *__buf) ;
int RS_pthread_setname_np(pthread_t /*__target_thread*/, const char *__buf) {
	return pthread_setname_np(__buf);
}
#else
#ifndef __WIN64__
int __attribute__((weak)) pthread_setname_np(pthread_t __target_thread, const char *__buf) ;
#endif //__WIN64__
int RS_pthread_setname_np(pthread_t __target_thread, const char *__buf) {
	return pthread_setname_np(__target_thread, __buf);
}
#endif //__APPLE__


/*******
 * #define DEBUG_THREADS 1
 * #define RSMUTEX_ABORT 1  // Catch wrong pthreads mode.
 *******/
#define THREAD_DEBUG RsDbg() << "[this=" << static_cast<void*>(this) \
	<< ", caller thread ID: " << std::hex << pthread_self() << ", thread ID: " \
	<< mTid << std::dec << "] "

#ifdef RSMUTEX_ABORT
	#include <stdlib.h>
#endif

#ifdef DEBUG_THREADS
	#include <iostream>
#endif

/*static*/ void* RsThread::rsthread_init(void* p)
{
	RsThread* thread = reinterpret_cast<RsThread *>(p);
	if(!thread) return nullptr;

	/* Using pthread_detach(...) the thread resources will be automatically
	 * freed when this function return, so there is no need for pthread_join()
	 * later. */
	pthread_detach(pthread_self());

#ifdef DEBUG_THREADS
	std::cerr << "[Thread ID:" << std::hex << pthread_self() << std::dec
	          << "] thread is started. Calling wrapRun()..." << std::endl;
#endif

	thread->wrapRun();
	return nullptr;
}

void RsThread::resetTid()
{
#ifdef WINDOWS_SYS
	memset (&mTid, 0, sizeof(mTid));
#else
	mTid = pthread_t(); //Thread identifiers should be considered opaque and can be null.
#endif
}

RsThread::RsThread() : mInitMtx("RsThread"), mHasStopped(true), mShouldStop(false), mLastTid()
#ifdef RS_THREAD_FORCE_STOP
  , mStopTimeout(0)
#endif
{ resetTid(); }

bool RsThread::isRunning() { return !mHasStopped; }

bool RsThread::shouldStop() { return mShouldStop; }

void RsThread::askForStop()
{
	/* Call onStopRequested() only once even if askForStop() is called multiple
	 * times */
	if(!mShouldStop.exchange(true)) onStopRequested();
}

void RsThread::wrapRun()
{
	{RS_STACK_MUTEX(mInitMtx);} // Waiting Init done.
	run();
	resetTid();
	mHasStopped = true;
}

void RsThread::fullstop()
{
#ifdef RS_THREAD_FORCE_STOP
	const rstime_t stopRequTS = time(nullptr);
#endif

	askForStop();

	const pthread_t callerTid = pthread_self();
	if(pthread_equal(mTid, callerTid))
	{
		RsErr() << __PRETTY_FUNCTION__ << " called by same thread. This should "
		        << "never happen! this: " << static_cast<void*>(this)
		        << std::hex << ", callerTid: " << callerTid
		        << ", mTid: " << mTid << std::dec
		        << ", mFullName: " << mFullName << std::endl;
		print_stacktrace();
		return;
	}

	// Wait for the thread being stopped
	auto i = 1;
	while(!mHasStopped)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
		++i;
		if(!(i%5))
			RsDbg() << __PRETTY_FUNCTION__ << " " << i*0.2 << " seconds passed"
			        << " waiting for thread: " << std::hex << mLastTid
			        << std::dec << " " << mFullName << " to stop" << std::endl;

#ifdef RS_THREAD_FORCE_STOP
		if(mStopTimeout && time(nullptr) > stopRequTS + mStopTimeout)
		{
			RsErr() << __PRETTY_FUNCTION__ << " thread mLastTid: " << std::hex
			         << mLastTid << " mTid: " << mTid << std::dec << " "
			         << mFullName
			         << " ignored our nice stop request for more then "
			         << mStopTimeout
			         << " seconds, will be forcefully stopped. "
			         << "Please submit a report to RetroShare developers"
			         << std::endl;

			const auto terr = pthread_cancel(mTid);
			if(terr == 0) mHasStopped = true;
			else
			{
				RsErr() << __PRETTY_FUNCTION__ << " pthread_cancel("
				        << std::hex << mTid << std::dec <<") returned "
				        << terr << " " << rsErrnoName(terr) << std::endl;
				print_stacktrace();
			}

			break;
		}
#endif // def RS_THREAD_FORCE_STOP
	}
}

bool RsThread::start(const std::string& threadName)
{
	// Atomically check if the thread was already started and set it as running
	if(mHasStopped.exchange(false))
	{
		RS_STACK_MUTEX(mInitMtx); // Block thread starting to run

		mShouldStop = false;
		int pError = pthread_create(
		            &mTid, nullptr, &rsthread_init, static_cast<void*>(this) );
		if(pError)
		{
			RsErr() << __PRETTY_FUNCTION__ << " pthread_create could not create"
			        << " new thread: " << threadName << " pError: " << pError
			        << std::endl;
			mHasStopped = true;
			print_stacktrace();
			return false;
		}

		/* Store an extra copy of thread id for debugging */
		mLastTid = mTid;

		/* Store thread full name as PThread is not able to keep it entirely */
		mFullName = threadName;

		/* Set PThread thread name which is restricted to 16 characters
		 * including the terminating null byte */
		if(pthread_setname_np && !threadName.empty())
			RS_pthread_setname_np(mTid, threadName.substr(0, 15).c_str());

		return true;
	}

	RsErr() << __PRETTY_FUNCTION__ << " attempt to start already running thread"
	        << std::endl;
	print_stacktrace();
	return false;
}

RsQueueThread::RsQueueThread(uint32_t min, uint32_t max, double relaxFactor )
    :mMinSleep(min), mMaxSleep(max), mRelaxFactor(relaxFactor)
{
    mLastSleep = (uint32_t)mMinSleep ;
    mLastWork = time(NULL) ;
}

void RsQueueThread::threadTick()
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

	std::this_thread::sleep_for(std::chrono::milliseconds(mLastSleep));
}

void RsMutex::unlock()
{
	_thread_id = 0;
	pthread_mutex_unlock(&realMutex);
}

void RsMutex::lock()
{
	int err = pthread_mutex_lock(&realMutex);
	if( err != 0)
	{
		RsErr() << __PRETTY_FUNCTION__ << "pthread_mutex_lock returned: "
		        << rsErrnoName(err)
#ifdef RS_MUTEX_DEBUG
		        << " name: " << name()
#endif
		       << std::endl;

		print_stacktrace();

#ifdef RSMUTEX_ABORT
		abort();
#endif
	}
 
	_thread_id = pthread_self();
}

#ifdef RS_MUTEX_DEBUG
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


RsThread::~RsThread()
{
	if(!mHasStopped)
	{
		RsErr() << __PRETTY_FUNCTION__ << " deleting thread: " << mLastTid
		        << " " << mFullName << " that is still "
		        << "running! Something seems very wrong here and RetroShare is "
		        << "likely to crash because of this." << std::endl;
		print_stacktrace();

		fullstop();
	}
}

RsQueueThread::~RsQueueThread() = default;
