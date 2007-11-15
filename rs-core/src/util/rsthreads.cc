
/*
 * "$Id: rsthreads.cc,v 1.1 2007-02-19 20:08:30 rmf24 Exp $"
 *
 * RetroShare C++ Interface.
 *
 * Copyright 2004-2007 by Robert Fernie.
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


#include "rsthreads.h"

extern "C" void* rsthread_init(void* p)
{
  RsThread *thread = (RsThread *) p;
  if (!thread)
  {
    return 0;
  }
  thread -> run();
  return 0;
}


pthread_t  createThread(RsThread &thread)
{
    pthread_t tid;
    void  *data = (void *) (&thread);

    thread.mMutex.lock();
    {
      pthread_create(&tid, 0, &rsthread_init, data);
      thread.mTid = tid;
    }
    thread.mMutex.unlock();

    return tid;

}



