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

// Use this class to measure and display time duration of a given environment:
//
// {
//     RsScopeTimer timer("callToMeasure()") ;
//
//     callToMeasure() ;
// }
//
class RsScopeTimer
{
	public:
		RsScopeTimer(const std::string& name)
		{
			_t = clock() ;
			_name = name ;
		}

		~RsScopeTimer()
		{
			clock_t s = clock() ;
			std::cerr << "Time for \"" << _name << "\": " << (s-_t)/(float)CLOCKS_PER_SEC << " secs" << std::endl;
		}

	private:
		clock_t _t ;
		std::string _name ;
};
