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


#include <pthread.h>
#include <inttypes.h>

/* RsIface Thread Wrappers */

#undef RSTHREAD_SELF_LOCKING_GUARD

class RsMutex
{
	public:

	RsMutex() 
	{ 
		pthread_mutex_init(&realMutex, NULL); 
#ifdef RSTHREAD_SELF_LOCKING_GUARD
		_thread_id = 0 ;
#endif
	}
	~RsMutex() 
	{ 
		pthread_mutex_destroy(&realMutex); 
	}

	void	lock() 
	{ 
#ifdef RSTHREAD_SELF_LOCKING_GUARD
		if(!trylock())
			if(!pthread_equal(_thread_id,pthread_self()))
#endif
				pthread_mutex_lock(&realMutex); 

		_thread_id = pthread_self() ;
#ifdef RSTHREAD_SELF_LOCKING_GUARD
		++_cnt ;
#endif
	}
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
	RsThread() { return; }
virtual ~RsThread() { return; }

virtual void start() { createThread(*this); }
virtual void run() = 0; /* called once the thread is started */
virtual	void join(); /* waits for the the mTid thread to stop */
virtual	void stop(); /* calls pthread_exit() */

	pthread_t mTid;
        RsMutex   mMutex;
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
