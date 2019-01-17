/*******************************************************************************
 * libretroshare/src/util: rstime.h                                            *
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
#pragma once

#include <string>
#ifdef WINDOWS_SYS
#include <stdint.h>
#else
#include <cstdint>
#endif
#include <ctime> // Added for comfort of users of this util header


/**
 * Safer alternative to time_t.
 * As time_t have not same lenght accross platforms, even though representation
 * is not guaranted to be the same but we found it being number of seconds since
 * the epoch for time points in all platforms we could test, or plain seconds
 * for intervals.
 * Still in some platforms it's 32bit long and in other 64bit long.
 * To avoid uncompatibility due to different serialzation format use this
 * reasonably safe alternative instead.
 */
typedef int64_t rstime_t;

// Do we really need this? Our names have rs prefix to avoid pollution already!
namespace rstime {

	/*!
	 * \brief This is a cross-system definition of usleep, which accepts any 32 bits number of micro-seconds.
	 */

	int rs_usleep(uint32_t micro_seconds);

	/* Use this class to measure and display time duration of a given environment:

	 {
	     RsScopeTimer timer("callToMeasure()") ;

	     callToMeasure() ;
	 }

	*/

	class RsScopeTimer
	{
	public:
		RsScopeTimer(const std::string& name);
		~RsScopeTimer();

		void start();
		double duration();

		static double currentTime();

	private:
		std::string _name ;
		double _seconds ;
	};

}
