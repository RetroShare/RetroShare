/*******************************************************************************
 * libretroshare/src/util: rsthreads.h                                         *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2004-2006  Robert Fernie <retroshare@lunamutt.com>            *
 * Copyright (C) 2016-2019  Gioacchino Mazzurco <gio@eigenlab.org>             *
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
#include <atomic>
#include <thread>
#include <functional>

#include "util/rsmemory.h"
#include "util/rsdeprecate.h"


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
		pthread_mutex_init(&realMutex, nullptr);

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
 * @def RS_STACK_MUTEX(m)
 * This macro allows you to trace which mutex in the code is locked and for how
 * much time. You can use this as follows:
 * @code
 * {
 * 	RS_STACK_MUTEX(myMutex);
 * 	do_something();
 * }
 * @endcode
 */
#define RS_STACK_MUTEX(m) \
	RsStackMutex __local_retroshare_stack_mutex_##m( \
	m, __PRETTY_FUNCTION__, __FILE__, __LINE__ )

/**
 * Provide mutexes that automatically lock/unlock on creation/destruction and
 * have powerfull debugging facilities (if RSMUTEX_DEBUG is defined at
 * compiletime).
 * In most of the cases you should not use this directly instead
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



/// @brief Offer basic threading functionalities.
class RsThread
{
public:
	RsThread();
	virtual ~RsThread();

	/**
	 * @brief start the thread and call run() on it.
	 * @param threadName string containing the name of the thread used for
	 *                   debugging purposes, it is truncated to 16 characters
	 *                   including \0 at the end of the string.
	 * @return false on error, true otherwise
	 */
	bool start(const std::string& threadName = "");

	/**
	 * @brief Check if thread is running.
	 * @return true if the thread is still running, false otherwise.
	 */
	bool isRunning();

	/**
	 * @brief Check if the thread should stop.
	 * Expecially useful for subclasses which implement a @see run() method
	 * which may take lot of time before returning when not asked, to check if
	 * stop has been requested and therefore interrupting the execution ASAP
	 * returning in a coherent state.
	 * @return true if the thread received a stopping order.
	 */
	bool shouldStop();

	/**
	 * @brief Asyncronously ask the thread to stop.
	 * The real stop will happen when the @see run() method finish.
	 */
	void askForStop();

	/**
	 * Call @see askForStop() then wait it has really stopped before returning.
	 * It must not be called in the same thread, as it would not wait for the
	 * effective stop to occur as it would cause a deadlock.
	 */
	void fullstop();

	/**
	 * Execute given function on a detached thread without blocking the caller
	 * execution.
	 * This can be generalized with variadic template, ATM it is enough to wrap
	 *	any kind of function call or job into a lambda which get no paramethers
	 *	and return nothing but can capture
	 * This can be easly optimized later by using a thread pool
	 */
	static void async(const std::function<void()>& fn)
	{ std::thread(fn).detach(); }

protected:
	/**
	 * This method must be implemented by sublasses, will be called once the
	 * thread is started. Should return on request, use @see shouldStop() to
	 * check if stop has been requested.
	 */
	virtual void run() = 0;

	/**
	 * This method is meant to be overridden by subclasses with long running
	 * @see run() method and is executed asyncronously when @see askForStop()
	 * is called, any task necessary to stop the thread (aka inducing @see run()
	 * to return in a coherent state) should be done in the overridden version
	 * of this method, @see JsonApiServer for an usage example. */
	virtual void onStopRequested() {}

private:
	/** Call @see run() setting the appropriate flags around it*/
	void wrapRun();

	/// True if thread is stopped, false otherwise
	std::atomic<bool> mHasStopped;

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

	/**
	 * Subclasses must implement this method, it will be called in a loop once
	 * the thread is started, so repetitive work (like checking if data is
	 * available on a socket) should be done here, at the end of this method
	 * sleep_for(...) or similar function should be called or the CPU will
	 * be used as much as possible also if there is nothing to do.
	 */
	virtual void threadTick() = 0;

private:
	/// Implement the run loop and continuously call threadTick() in it
	void run() override { while(!shouldStop()) threadTick(); }
};

// TODO: Used just one time, is this really an useful abstraction?
class RsQueueThread: public RsTickingThread
{
public:
	RsQueueThread(uint32_t min, uint32_t max, double relaxFactor);
	~RsQueueThread() override;

protected:
	virtual bool workQueued() = 0;
	virtual bool doWork() = 0;

	void threadTick() override; /// @see RsTickingThread

private:
    uint32_t mMinSleep; /* ms */
    uint32_t mMaxSleep; /* ms */
    uint32_t mLastSleep; /* ms */
    time_t   mLastWork;  /* secs */
    float    mRelaxFactor;
};
