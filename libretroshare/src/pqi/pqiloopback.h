/*
 * libretroshare/src/pqi: pqiloopback.h
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
