/*******************************************************************************
 * libretroshare/src/util: rsthreads.h                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare@lunamutt.com>              *
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
#pragma once

#include <pthread.h>
#include <inttypes.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <semaphore.h>
#include <thread>
#include <functional>

#include <util/rsmemory.h>
#include "util/rstime.h"

/* RsIface Thread Wrappers */

#undef RSTHREAD_SELF_LOCKING_GUARD
//#define RSMUTEX_DEBUG 300 // Milliseconds for print in the stderr
//#define RSMUTEX_DEBUG 

class RsMutex
{
	public:

	RsMutex(const std::string& name)
	{
		/* remove unused parameter warnings */

		pthread_mutex_init(&realMutex, NULL);
		_thread_id = 0 ;
#ifdef RSMUTEX_DEBUG
		this->_name = name;
#else
		(void) name;
#endif
	}
	~RsMutex() 
	{ 
		pthread_mutex_destroy(&realMutex); 
	}

	inline const pthread_t& owner() const { return _thread_id ; }
#ifdef RSMUTEX_DEBUG
	void setName(const std::string &name)
	{
		this->_name = name;
	}
#endif

	void	lock();
	void	unlock();
	bool	trylock() { return (0 == pthread_mutex_trylock(&realMutex)); }

#ifdef RSMUTEX_DEBUG
	const std::string& name() const { return _name ; }
#endif

	private:
		pthread_mutex_t  realMutex;
		pthread_t _thread_id ;
#ifdef RSTHREAD_SELF_LOCKING_GUARD
		uint32_t _cnt ;
#endif
#ifdef RSMUTEX_DEBUG
		std::string _name;
#endif
};

class RsStackMutex
{
	public:

		RsStackMutex(RsMutex &mtx)
			: mMtx(mtx) 
		{ 
			mMtx.lock(); 
#ifdef RSMUTEX_DEBUG
			double ts = getCurrentTS() ;
			_time_stamp = ts ;
			_lineno = 0 ;
			_info = "[no info]" ;
#endif
		}
		RsStackMutex(RsMutex &mtx,const char *function_name,const char *file_name,int lineno)
			: mMtx(mtx)
#ifdef RSMUTEX_DEBUG
			, _info(std::string(function_name)+" in file "+file_name),_lineno(lineno)
#endif
		{ 
#ifdef RSMUTEX_DEBUG
			double ts = getCurrentTS() ;
			_time_stamp = ts ;
			pthread_t owner = mMtx.owner() ;
#else
			/* remove unused parameter warnings */
			(void) function_name;
			(void) file_name;
			(void) lineno;
#endif

			mMtx.lock(); 

#ifdef RSMUTEX_DEBUG
			ts = getCurrentTS() ;

			if(ts - _time_stamp > 1.0)
				std::cerr << "Mutex " << (void*)&mMtx << " \"" << mtx.name() << "\"" 
					<< " waited for " << ts - _time_stamp 
					<< " seconds in thread " << pthread_self() 
					<< " for locked thread " << owner << ". in " << _info << ":" << _lineno << std::endl;

			_time_stamp = ts ;	// This is to re-init the locking time without accounting for how much we waited.
#endif
		}

		~RsStackMutex() 
		{ 
			mMtx.unlock(); 
#ifdef RSMUTEX_DEBUG
			double ts = getCurrentTS() ;

			if(ts - _time_stamp > 1.0)
				std::cerr << "Mutex " << (void*)&mMtx << " \"" << mMtx.name() << "\"" 
					<< " locked for " << ts - _time_stamp 
					<< " seconds in thread " << pthread_self() 
					<< ". in " << _info << ":" << _lineno << std::endl;
#endif
		}

	private:
		RsMutex &mMtx;

#ifdef RSMUTEX_DEBUG
		static double getCurrentTS() ;
		double _time_stamp ;
		std::string _info ;
		int _lineno ;
#endif
};

// This macro allows you to trace which mutex in the code is locked for how much time.
// se this as follows:
//
// {
// 	RS_STACK_MUTEX(myMutex) ;
//
// 	do_something() ;
// }
//
#define RS_STACK_MUTEX(m) RsStackMutex __local_retroshare_mutex(m,__PRETTY_FUNCTION__,__FILE__,__LINE__) 

// This class handles a Mutex-based semaphore, that makes it cross plateform.
class RsSemaphore
{
    class RsSemStruct
    {
    public:
        RsSemStruct() : mtx("Semaphore mutex"), val(0) {}

        RsMutex mtx ;
        uint32_t val ;
    };

public:
    RsSemaphore()
    {
        s = new RsSemStruct ;
    }

    ~RsSemaphore()
    {
        delete s ;
    }

    void set(uint32_t i)
    {
        RS_STACK_MUTEX(s->mtx) ;
        s->val = i ;
    }

    void post()
    {
        RS_STACK_MUTEX(s->mtx) ;
        ++(s->val) ;
    }

    uint32_t value()
    {
        RS_STACK_MUTEX(s->mtx) ;
        return s->val ;
    }

    // waits but does not re-locks the semaphore
    
    void wait_no_relock()
    {
        static const uint32_t max_waiting_time_before_warning=1000 *5 ;	// 5 secs
        uint32_t tries=0;

        while(true)
        {
            usleep(1000) ;
            if(++tries >= max_waiting_time_before_warning)
                std::cerr << "(EE) Semaphore waiting for too long. Something is probably wrong in the code." << std::endl;

            RS_STACK_MUTEX(s->mtx) ;
            if(s->val > 0)
		    return ;
        }

    }
private:
    RsSemStruct *s ;
};

class RsThread;

/* to create a thread! */
pthread_t  createThread(RsThread &thread);

class RsThread
{
public:
    RsThread();
    virtual ~RsThread() {}

    void start(const std::string &threadName = "");

    // Returns true of the thread is still running.

    bool isRunning();

    // Returns true if the thread received a stopping order and hasn't yet stopped.

    bool shouldStop();

    // Can be called to set the stopping flags. The stop will not be handled
    // by RsThread itself, but in subclasses. If you derive your own subclass,
    // you need to call shouldStop() in order to check for a possible stopping order.

    void ask_for_stop();

	/**
	 * Execute given function on another thread without blocking the caller
	 * execution.
	 * This can be generalized with variadic template, ATM it is enough to wrap
	 *	any kind of function call or job into a lambda which get no paramethers
	 *	and return nothing but can capture
	 * This can be easly optimized later by using a thread pool
	 */
	static void async(const std::function<void()>& fn)
	{ std::thread(fn).detach(); }

protected:
    virtual void runloop() =0; /* called once the thread is started. Should be overloaded by subclasses. */
    void go() ;	// this one calls runloop and also sets the flags correctly when the thread is finished running.

    RsSemaphore mHasStoppedSemaphore;
    RsSemaphore mShouldStopSemaphore;

    static void *rsthread_init(void*) ;
    pthread_t mTid;
};

class RsTickingThread: public RsThread
{
public:
    RsTickingThread();

    void shutdown();
    void fullstop();
    void join() { fullstop() ; }	// used for compatibility

    virtual void data_tick() =0;

private:
    virtual void runloop() ; /* called once the thread is started. Should be overloaded by subclasses. */
};

class RsSingleJobThread: public RsThread
{
public:
    virtual void run() =0;

protected:
    virtual void runloop() ;
};

class RsQueueThread: public RsTickingThread
{
public:

    RsQueueThread(uint32_t min, uint32_t max, double relaxFactor );
    virtual ~RsQueueThread() { return; }

protected:

    virtual bool workQueued() = 0;
    virtual bool doWork() = 0;
    virtual void data_tick() ;

private:
    uint32_t mMinSleep; /* ms */
    uint32_t mMaxSleep; /* ms */
    uint32_t mLastSleep; /* ms */
    rstime_t   mLastWork;  /* secs */
    float    mRelaxFactor;
};

