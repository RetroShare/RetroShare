/*
 * libretroshare/src/dht: opendhtstr.h
 *
 * Interface with OpenDHT for RetroShare.
 *
 * Copyright 2007-2008 by Robert Fernie.
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

#ifndef OPENDHT_STRING_CODE_H
#define OPENDHT_STRING_CODE_H

#include <inttypes.h>
#include <string>

std::string createHttpHeader(std::string host, uint16_t port, 
					std::string agent, uint32_t length);

std::string createHttpHeaderGET(std::string host, uint16_t port, 
			std::string page, std::string agent, uint32_t length);

std::string createOpenDHT_put(std::string key, std::string value, 
					uint32_t ttl, std::string client);

std::string createOpenDHT_get(std::string key, 
				uint32_t maxresponse, std::string client);
		
#endif

