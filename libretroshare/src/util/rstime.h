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
