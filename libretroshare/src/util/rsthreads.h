#ifndef RSIFACE_THREADS_H
#define RSIFACE_THREADS_H

/*
 * "$Id: rsthreads.h,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
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


#ifdef WINDOWS_SYS
#include "util/rswin.h"
#endif

#include <pthread.h>
#include <inttypes.h>
#include <string>

/* RsIface Thread Wrappers */

#undef RSTHREAD_SELF_LOCKING_GUARD
//#define RSMUTEX_DEBUG 300 // Milliseconds for print in the stderr

class RsMutex
{
	public:

	RsMutex(const std::string& name)
	{
		/* remove unused parameter warnings */
		(void) name;

		pthread_mutex_init(&realMutex, NULL);
#ifdef RSTHREAD_SELF_LOCKING_GUARD
		_thread_id = 0 ;
#endif
#ifdef RSMUTEX_DEBUG
		this->name = name;
#endif
	}
	~RsMutex() 
	{ 
		pthread_mutex_destroy(&realMutex); 
	}

#ifdef RSMUTEX_DEBUG
	void setName(const std::string &name)
	{
		this->name = name;
	}
#endif

	void	lock();
	void	unlock() 
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
	bool	trylock() { return (0 == pthread_mutex_trylock(&realMutex)); }

	private:
		pthread_mutex_t  realMutex;
		pthread_t _thread_id ;
#ifdef RSTHREAD_SELF_LOCKING_GUARD
		uint32_t _cnt ;
#endif
#ifdef RSMUTEX_DEBUG
		std::string name;
#endif
};

class RsStackMutex
{
	public:

	RsStackMutex(RsMutex &mtx): mMtx(mtx) { mMtx.lock(); }
	~RsStackMutex() { mMtx.unlock(); }

	private:
	RsMutex &mMtx;
};

class RsThread;

/* to create a thread! */
pthread_t  createThread(RsThread &thread);

class RsThread
{
	public:
	RsThread();
virtual ~RsThread() {}

virtual void start() { createThread(*this); }
virtual void run() = 0; /* called once the thread is started */
virtual	void join(); /* waits for the the mTid thread to stop */
virtual	void stop(); /* calls pthread_exit() */

bool isRunning();

	pthread_t mTid;
	RsMutex   mMutex;

private:
	bool mIsRunning;
};


class RsQueueThread: public RsThread
{
	public:

	RsQueueThread(uint32_t min, uint32_t max, double relaxFactor );
virtual ~RsQueueThread() { return; }

virtual void run();

	protected:

virtual bool workQueued() = 0;
virtual bool doWork() = 0;

	private:
	uint32_t mMinSleep; /* ms */
	uint32_t mMaxSleep; /* ms */
	uint32_t mLastSleep; /* ms */
	time_t   mLastWork;  /* secs */
	float    mRelaxFactor; 
};

#endif
