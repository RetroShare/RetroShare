#ifndef RS_TLV_MAPS_H
#define RS_TLV_MAPS_H

/*
 * libretroshare/src/serialiser: rstlvmaps.h
 *
 * RetroShare Serialiser.
 *
 * Copyright 2014 by Robert Fernie
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
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

#if 0
#include "serialiser/rstlvgenericmaps.h"

#endif


class RsTlvOpinionMapRef: public RsTlvGenericMapRef<std::string, uint32_t>
{
public:
	RsTlvOpinionMapRef(std::map<std::string, uint32_t> &refmap)
	:RsTlvGenericMapRef(OPINION, STRING_INT_PAIR, refmap) { return; }
};


#endif

