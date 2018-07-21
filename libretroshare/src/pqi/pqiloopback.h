/*******************************************************************************
 * libretroshare/src/pqi: pqiloopback.h                                        *
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
#ifndef MRK_PQI_LOOPBACK_HEADER
#define MRK_PQI_LOOPBACK_HEADER

#include <list>                  // for list

#include "pqi/pqi_base.h"        // for NetInterface (ptr only), PQInterface
#include "retroshare/rstypes.h"  // for RsPeerId

struct RsItem;

class pqiloopback: public PQInterface
{
public:
	pqiloopback(const RsPeerId& id);
virtual ~pqiloopback();

// search Interface.
virtual int	SendItem(RsItem *item);
virtual RsItem *GetItem();

// PQI interface.
virtual int	tick();
virtual int	status();

private:
	std::list<RsItem *> objs;
};

#endif //MRK_PQI_LOOPBACK_HEADER
