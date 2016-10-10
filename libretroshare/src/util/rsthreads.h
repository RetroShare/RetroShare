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

#pragma once

#include <pthread.h>
#include <inttypes.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <util/rsmemory.h>


//#define RSMUTEX_DEBUG

/**
 * @brief Provide mutexes that keep track of the owner. Based on pthread mutex.
 */
class RsMutex
{
public:

	RsMutex(const std::string& name) : _thread_id(0)
#ifdef RSMUTEX_DEBUG
	  , _name(name)
#endif
	{
		pthread_mutex_init(&realMutex, NULL);

#ifndef RSMUTEX_DEBUG
		(void) name; // remove unused parameter warnings
#endif
	}

	~RsMutex() { pthread_mutex_destroy(&realMutex); }

	inline const pthread_t& owner() const { return _thread_id; }

	void lock();
	void unlock();
	bool trylock() { return (0 == pthread_mutex_trylock(&realMutex)); }

#ifdef RSMUTEX_DEBUG
	const std::string& name() const { return _name ; }
#endif

private:
	pthread_mutex_t realMutex;
	pthread_t _thread_id;

#ifdef RSMUTEX_DEBUG
	std::string _name;
#endif
};

/**
 * @brief Provide mutexes that automatically lock/unlock on creation/destruction
 *        and have powerfull debugging facilities (if RSMUTEX_DEBUG is defined
 *        at compiletime )
 * @see RS_STACK_MUTEX(m)
 */
class RsStackMutex
{
public:

	RsStackMutex(RsMutex &mtx) : mMtx(mtx)
	{
		mMtx.lock();
#ifdef RSMUTEX_DEBUG
		double ts = getCurrentTS();
		_time_stamp = ts;
		_lineno = 0;
		_info = "[no info]";
#endif
	}

	RsStackMutex(RsMutex &mtx, const char *function_name, const char *file_name,
	             int lineno) : mMtx(mtx)
#ifdef RSMUTEX_DEBUG
	  , _info(std::string(function_name)+" in file "+file_name), _lineno(lineno)
#endif
	{
#ifdef RSMUTEX_DEBUG
		double ts = getCurrentTS();
		_time_stamp = ts;
		pthread_t owner = mMtx.owner();
#else
		// remove unused parameter warnings
		(void) function_name; (void) file_name; (void) lineno;
#endif

		mMtx.lock();

#ifdef RSMUTEX_DEBUG
		ts = getCurrentTS();

		if(ts - _time_stamp > 1.0)
			std::cerr << "Mutex " << (void*)&mMtx << " \"" << mtx.name() << "\""
			          << " waited for " << ts - _time_stamp
			          << " seconds in thread " << pthread_self()
			          << " for locked thread " << owner << ". in " << _info
			          << ":" << _lineno << std::endl;
		_time_stamp = ts ;	// This is to re-init the locking time without accounting for how much we waited.
#endif
	}

	~RsStackMutex()
	{
		mMtx.unlock();

#ifdef RSMUTEX_DEBUG
		double ts = getCurrentTS();

		    if(ts - _time_stamp > 1.0)
				std::cerr << "Mutex " << (void*)&mMtx << " \"" << mMtx.name()
				          << "\"" << " locked for " << ts - _time_stamp
				          << " seconds in thread " << pthread_self()
				          << ". in " << _info << ":" << _lineno << std::endl;
#endif
	}

private:
	RsMutex &mMtx;

#ifdef RSMUTEX_DEBUG
	static double getCurrentTS();
	double _time_stamp;
	std::string _info;
	int _lineno;
#endif
};


/**
 * @def RS_STACK_MUTEX(m)
 * This macro allows you to trace which mutex in the code is locked and for how
 * much time. You can use this as follows:
 * @code
 * {
 * 	RS_STACK_MUTEX(myMutex) ;
 * 	do_something() ;
 * }
 * @endcode
 */
#define RS_STACK_MUTEX(m) RsStackMutex __local_retroshare_mutex(m,__PRETTY_FUNCTION__,__FILE__,__LINE__) 


/**
 * @brief Offer basic threading functionalities, to execute code in parallel in
 * a thread you should inerith from this class.
 */
class RsThread
{
public:
	RsThread();
	virtual ~RsThread() {}

	/**
	 * @brief start the thread and call run() on it.
	 * @param threadName string containing the name of the thread used for
	 *                   debugging purposes, it is truncated to 16 characters
	 *                   including \0 at the end of the string.
	 */
	void start(const std::string &threadName = "");

	/**
	 * @brief Check if thread is running.
	 * @return true if the thread is still running, false otherwise.
	 */
	bool isRunning();

	/**
	 * @brief Check if the thread should stop.
	 * @return true if the thread received a stopping order but hasn't stopped yet.
	 */
	bool shouldStop();

	/**
	 * @brief Ask the thread to stop.
	 * Set the mShouldStop flag. The real stop will not be handled by RsThread
	 * itself, but must be implemented in subclasses @see run().
	 * If you derive your own subclass, you need to call shouldStop() in order
	 * to check for a possible stopping order.
	 */
	void askForStop();


	/**
	 * Ask the thread to stop, and wait it has really stopped before returning.
	 * It must not be called in the same thread, as it would not wait for the
	 * effective stop to occur as it would cause a deadlock.
	 */
	void fullstop();

protected:

	/**
	 * This method must be implemented by sublasses, will be called once the
	 * thread is started. Should return and properly set the stopping flags once
	 * requested, shuold use shouldStop() to check that.
	 */
	virtual void run() = 0;

	/**
	 * @brief flag to indicate if thread is stopped or not
	 * Subclasses must take care of setting this to false when the execution
	 * start, and to true when execution is stopped.
	 */
	std::atomic<bool> mHasStopped;

private:
	/// True if stop has been requested
	std::atomic<bool> mShouldStop;

	/// Passed as argument for pthread_create(), call start()
	static void *rsthread_init(void*);

	/// Store the id of the corresponding pthread
	pthread_t mTid;
};

/**
 * Provide a detached execution loop that continuously call data_tick() once the
 * thread is started
 */
class RsTickingThread: public RsThread
{
public:
	RsTickingThread();

	/**
	 * Subclasses must implement this method, it will be called in a loop once
	 * the thread is started, so repetitive work (like checking if data is
	 * available on a socket) should be done here, at the end of this method
	 * @see rs_usleep(...) or similar function should be called or the CPU will
	 * be used as much as possible also if there is nothing to do.
	 */
	virtual void data_tick() = 0;

	/// @deprecated Provided for compatibility @see RsThread::fullstop()
	inline void join() { fullstop(); }

private:
	/// Implement the run loop and continuously call data_tick() in it
	virtual void run();
};

// TODO: Used just one time, is this really an useful abstraction?
class RsQueueThread: public RsTickingThread
{
public:

	RsQueueThread(uint32_t min, uint32_t max, double relaxFactor);
	virtual ~RsQueueThread() {}

protected:

    virtual bool workQueued() = 0;
    virtual bool doWork() = 0;
    virtual void data_tick() ;

private:
    uint32_t mMinSleep; /* ms */
    uint32_t mMaxSleep; /* ms */
    uint32_t mLastSleep; /* ms */
    time_t   mLastWork;  /* secs */
    float    mRelaxFactor;
};
