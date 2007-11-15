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

/* RsIface Thread Wrappers */

class RsMutex
{
	public:

	RsMutex() { pthread_mutex_init(&realMutex, NULL); }
        ~RsMutex() { pthread_mutex_destroy(&realMutex); }
void	lock() { pthread_mutex_lock(&realMutex); }
void	unlock() { pthread_mutex_unlock(&realMutex); }
bool	trylock() { return (0 == pthread_mutex_trylock(&realMutex)); }

	private:
	pthread_mutex_t  realMutex;
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

	pthread_t mTid;
        RsMutex   mMutex;
};


#endif
