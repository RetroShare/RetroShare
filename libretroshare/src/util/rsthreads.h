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
#include <string>
#include <iostream>

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
#ifdef RSTHREAD_SELF_LOCKING_GUARD
		_thread_id = 0 ;
#endif
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

class RsThread;

/* to create a thread! */
pthread_t  createThread(RsThread &thread);

class RsThread
{
	public:
	RsThread();
virtual ~RsThread() {}

virtual void start() { mIsRunning = true; createThread(*this); }
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
