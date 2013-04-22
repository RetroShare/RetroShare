/*
 * RetroShare External Interface.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#include "rpc/proto/rpcprotoutils.h"
#include <sys/time.h>


double getTimeStamp()
{
	struct timeval tv;
	double ts = 0;
	if (0 == gettimeofday(&tv, NULL))
	{
		ts = tv.tv_sec + (tv.tv_usec / 1000000.0);
	}
	return ts;
}

bool setTimeStamp(rsctrl::core::Timestamp *ts)
{
	struct timeval tv;
	if (0 != gettimeofday(&tv, NULL))
	{
		ts->set_secs(tv.tv_sec);
		ts->set_microsecs(tv.tv_usec);
		return true;
	}
	else
	{
		ts->set_secs(0);
		ts->set_microsecs(0);
		return false;
	}
	return false;
}




