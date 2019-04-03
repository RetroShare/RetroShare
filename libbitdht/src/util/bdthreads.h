/*******************************************************************************
 * util/bdthread.cc                                                            *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright (C) 2004-2010 Robert Fernie <bitdht@lunamutt.com>                 *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#ifndef BITDHT_THREADS_H
#define BITDHT_THREADS_H

#include <iostream>
#include <pthread.h>
#include <inttypes.h>

/* Thread Wrappers */

class bdMutex
{
	public:

		bdMutex(bool recursive = false) 
		{
			/* remove unused parameter warnings */
			(void) recursive;

#if 0 // TESTING WITHOUT RECURSIVE
			if(recursive)
			{
				pthread_mutexattr_t att ;
				pthread_mutexattr_init(&att) ;
				pthread_mutexattr_settype(&att,PTHREAD_MUTEX_RECURSIVE) ;

				if( pthread_mutex_init(&realMutex, &att))
					std::cerr << "ERROR: Could not initialize mutex !" << std::endl ;
			}
			else
#endif
				if( pthread_mutex_init(&realMutex, NULL))
					std::cerr << "ERROR: Could not initialize mutex !" << std::endl ;
		}

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
