/*
 * libretroshare/src/util: rsscopetimer.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2013-     by Cyril Soler
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

#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/unistd.h>
#include <unistd.h>
#include "rstime.h"

namespace rstime {

int rs_usleep(uint32_t micro_seconds)
{
	while(micro_seconds >= 1000000)
	{
		// usleep cannot be called with 1000000 or more.

		usleep(500000) ;
		usleep(500000) ;

		micro_seconds -= 1000000 ;
	}
	usleep(micro_seconds) ;

	return 0 ;
}

RsScopeTimer::RsScopeTimer(const std::string& name)
{
	_name = name ;
	start();
}

RsScopeTimer::~RsScopeTimer()
{
	if (!_name.empty())
	{
		std::cerr << "Time for \"" << _name << "\": " << duration() << std::endl;
	}
}

double RsScopeTimer::currentTime()
{
	timeval tv ;
	gettimeofday(&tv,NULL) ;
	return (tv.tv_sec % 10000) + tv.tv_usec/1000000.0f ;	// the %1000 is here to allow double precision to cover the decimals.
}

void RsScopeTimer::start()
{
	_seconds = currentTime();
}

double RsScopeTimer::duration()
{
	return currentTime() - _seconds;
}

}
