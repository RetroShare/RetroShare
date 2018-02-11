/*
 * libretroshare/src/util: rsscopetimer.h
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

#include <string>

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
