#ifndef BITDHT_THREADS_H
#define BITDHT_THREADS_H

/*
 * util/bdthreads.h
 *
 * BitDHT: An Flexible DHT library.
 *
 * Copyright 2004-2010 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 3 as published by the Free Software Foundation.
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
 * Please report all bugs and problems to "bitdht@lunamutt.com".
 *
 */


#include <pthread.h>
#include <inttypes.h>

/* Thread Wrappers */

class bdMutex
{
	public:

	bdMutex() { pthread_mutex_init(&realMutex, NULL); }
        ~bdMutex() { pthread_mutex_destroy(&realMutex); }
void	lock() { pthread_mutex_lock(&realMutex); }
void	unlock() { pthread_mutex_unlock(&realMutex); }
bool	trylock() { return (0 == pthread_mutex_trylock(&realMutex)); }

	private:
	pthread_mutex_t  realMutex;
};

class bdStackMutex
{
	public:

	bdStackMutex(bdMutex &mtx): mMtx(mtx) { mMtx.lock(); }
        ~bdStackMutex() { mMtx.unlock(); }

	private:
	bdMutex &mMtx;
};

class bdThread;

/* to create a thread! */
pthread_t  createThread(bdThread &thread);

class bdThread
{
	public:
	bdThread();
virtual ~bdThread() { return; }

virtual void start() { createThread(*this); }
virtual void run() = 0; /* called once the thread is started */
virtual	void join(); /* waits for the mTid thread to stop */
virtual	void stop(); /* calls pthread_exit() */

	pthread_t mTid;
        bdMutex   mMutex;
};


#endif
