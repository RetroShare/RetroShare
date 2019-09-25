/*******************************************************************************
 * RetroShare gossip discovery - discovery2 retro-compatibility include        *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2019  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include "retroshare/rsgossipdiscovery.h"
#include "util/rsdeprecate.h"

#warning "Including retroshare/rsdisc.h is deprecated, \
use retroshare/rsgossipdiscovery.h instead"

using RsDisc RS_DEPRECATED_FOR("RsGossipDiscovery") = RsGossipDiscovery;

#define rsDisc rsGossipDiscovery.get()
