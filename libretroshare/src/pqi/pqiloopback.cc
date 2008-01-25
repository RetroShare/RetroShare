/*
 * libretroshare/src/pqi: pqiloopback.cc
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2004-2008 by Robert Fernie.
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

#include "pqi/pqi.h"
#include "pqi/pqiloopback.h"

pqiloopback::pqiloopback(std::string id)
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
	objs.push_back(i);
	return 1;
}

RsItem * 	pqiloopback::GetItem()
{
	if (objs.size() > 0)
	{
		RsItem *pqi = objs.front();
		objs.pop_front();
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


