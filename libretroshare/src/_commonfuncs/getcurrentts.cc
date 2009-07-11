#include "getcurrentts.h"

/*
 * "$Id: getcurrentts.cc,v 1.5 2007-04-15 18:45:23 rmf24 Exp $"
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

#ifdef WINDOWS_SYS
#include <time.h>
#include <sys/timeb.h>
#endif
#else
#include <sys/time.h>
#include <time.h>
#endif

static double getCurrentTS()
{

#ifndef WINDOWS_SYS
    struct timeval cts_tmp;
    gettimeofday(&cts_tmp, NULL);
    double cts =  (cts_tmp.tv_sec) + ((double) cts_tmp.tv_usec) / 1000000.0;
#else
    struct _timeb timebuf;
    _ftime( &timebuf);
    double cts =  (timebuf.time) + ((double) timebuf.millitm) / 1000.0;
#endif
    return cts;
}


