/*
 * libretroshare/src/serialiser: rsserviceserialiser.h
 *
 * 3P/PQI network interface for RetroShare.
 *
 * Copyright 2013-2013 by Cyril Soler & Robert Fernie
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

#pragma once

#include "rsserial.h"

class RsServiceSerialiser: public RsSerialType
{
	public:
		RsServiceSerialiser() :RsSerialType(RS_PKT_VERSION_SERVICE, 0, 0) { }
		virtual     ~RsServiceSerialiser() { }

		virtual	uint32_t    size(RsItem *);
		virtual	bool        serialise  (RsItem *item, void *data, uint32_t *size);
		virtual	RsItem *    deserialise(void *data, uint32_t *size);
};

