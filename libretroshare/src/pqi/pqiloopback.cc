/*******************************************************************************
 * libretroshare/src/pqi: pqiloopback.cc                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2004-2008 by Robert Fernie <retroshare@lunamutt.com>              *
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
#include "pqi/pqiloopback.h"

#include <stddef.h>  // for NULL

struct RsItem;

/***
#define LOOPBACK_DEBUG 1
***/

pqiloopback::pqiloopback(const RsPeerId& id)
	:PQInterface(id)
{
	setMaxRate(true, 0);
        setMaxRate(false, 0);
	setRate(true, 0);
	setRate(false, 0);

	return;
}

pqiloopback::~pqiloopback()
{
	return;
}

int	pqiloopback::SendItem(RsItem *i)
{

#ifdef  LOOPBACK_DEBUG 
	std::cerr << "pqiloopback::SendItem()";
	std::cerr << std::endl;
	i->print(std::cerr);
	std::cerr << std::endl;
#endif
	objs.push_back(i);
	return 1;
}

RsItem * 	pqiloopback::GetItem()
{
	if (objs.size() > 0)
	{
		RsItem *pqi = objs.front();
		objs.pop_front();
#ifdef  LOOPBACK_DEBUG 
		std::cerr << "pqiloopback::GetItem()";
		std::cerr << std::endl;
		pqi->print(std::cerr);
		std::cerr << std::endl;
#endif
		return pqi;
	}
	return NULL;
}

// PQI interface.
int 	pqiloopback::tick()
{
	return 0;
}

int 	pqiloopback::status()
{
	return 0;
}


