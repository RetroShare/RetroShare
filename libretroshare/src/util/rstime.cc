/*******************************************************************************
 * libretroshare/src/util: rstime.cc                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2013-2013 by Cyril Soler <csoler@users.sourceforge.net>           *
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
#include <iostream>
#include <thread>
#include <sys/time.h>
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
