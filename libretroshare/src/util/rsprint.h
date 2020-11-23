/*******************************************************************************
 * libretroshare/src/util: rsprint.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2008-2008 Robert Fernie <retroshare@lunamutt.com>                 *
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
#ifndef RSUTIL_PRINTFNS_H
#define RSUTIL_PRINTFNS_H

#include <inttypes.h>
#include <string>
#include <vector>

namespace RsUtil {

std::string BinToHex(const std::string &bin);
std::string BinToHex(const char *arr, const uint32_t len);

// proxy function. When max_len>0 and len>max_len, only the first "max_len" bytes are writen to the string and "..." is happened.

std::string BinToHex(const unsigned char *arr, const uint32_t len, uint32_t max_len=0);
bool HexToBin(const std::string& input,unsigned char *data, const uint32_t len);
std::string NumberToString(uint64_t n, bool hex=false);
std::string HashId(const std::string &id, bool reverse = false);
std::vector<uint8_t> BinToSha256(const std::vector<uint8_t> &in);

//std::string AccurateTimeString();

}
	
#endif
