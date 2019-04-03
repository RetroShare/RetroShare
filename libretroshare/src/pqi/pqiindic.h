/*******************************************************************************
 * libretroshare/src/pqi: pqiindic.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2006 by Robert Fernie <retroshare@lunamutt.com>              *
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
#ifndef MRK_PQI_INDICATOR_HEADER
#define MRK_PQI_INDICATOR_HEADER

#include <vector>

// This will indicate to num different sources
// when the event has occured.

class Indicator
{
	public:
	explicit Indicator(uint16_t n = 1)
	:num(n), changeFlags(n) {IndicateChanged();}
void	IndicateChanged()
	{
		for(uint16_t i = 0; i < num; i++)
			changeFlags[i]=true;
	}

bool	Changed(uint16_t idx = 0)
	{
		/* catch overflow */
		if (idx > num - 1)
			return false;

		bool ans = changeFlags[idx];
		changeFlags[idx] = false;
		return ans;
	}

	private:
	uint16_t num;
	std::vector<bool> changeFlags;
};



#endif
