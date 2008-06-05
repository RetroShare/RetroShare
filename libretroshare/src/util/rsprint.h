
/*
 * libretroshare/src/util: rsprint.h
 *
 * RetroShare Utilities
 *
 * Copyright 2008 by Robert Fernie.
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


#ifndef RSUTIL_PRINTFNS_H
#define RSUTIL_PRINTFNS_H

#include <inttypes.h>
#include <string>

namespace RsUtil {

std::string BinToHex(std::string bin);
std::string BinToHex(const char *arr, const uint32_t len);
std::string HashId(std::string id, bool reverse = false);

}
	
#endif
