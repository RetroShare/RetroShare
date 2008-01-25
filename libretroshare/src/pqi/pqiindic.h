/*
 * "$Id: pqiindic.h,v 1.3 2007-02-18 21:46:49 rmf24 Exp $"
 *
 * 3P/PQI network interface for RetroShare.
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



#ifndef MRK_PQI_INDICATOR_HEADER
#define MRK_PQI_INDICATOR_HEADER

#include <vector>

// This will indicate to num different sources
// when the event has occured.

class Indicator
{
	public:
	Indicator(int n = 1)
	:num(n), changeFlags(n) {IndicateChanged();}
void	IndicateChanged()
	{
		for(int i = 0; i < num; i++)
			changeFlags[i]=true;
	}

bool	Changed(int idx = 0)
	{
		/* catch overflow */
		if (idx > num - 1)
			return false;

		bool ans = changeFlags[idx];
		changeFlags[idx] = false;
		return ans;
	}

	private:
	int num;
	std::vector<bool> changeFlags;
};



#endif
