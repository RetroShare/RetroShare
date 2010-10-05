/*
 * util/bdthreads.cc
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



#include "bdthreads.h"
#include <unistd.h>    /* for usleep() */

/*******
 * #define DEBUG_THREADS 1
 *******/

#ifdef DEBUG_THREADS
	#include <iostream>
#endif

extern "C" void* bdthread_init(void* p)
{
  bdThread *thread = (bdThread *) p;
  if (!thread)
  {
    return 0;
  }
  thread -> run();
  return 0;
}


pthread_t  createThread(bdThread &thread)
{
    pthread_t tid;
    void  *data = (void *) (&thread);

    thread.mMutex.lock();
    {
      	pthread_create(&tid, 0, &bdthread_init, data);
      	thread.mTid = tid;
    }
    thread.mMutex.unlock();

    return tid;

}

bdThread::bdThread()
{
#if defined(_WIN32) || defined(__MINGW32__)
	memset (&mTid, 0, sizeof(mTid));
#else
	mTid = 0;
#endif
}

void bdThread::join() /* waits for the the mTid thread to stop */
{
	pthread_join(mTid, NULL);
}

void bdThread::stop() 
{
	pthread_exit(NULL);
}



