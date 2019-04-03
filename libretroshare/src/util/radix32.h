/*******************************************************************************
 * libretroshare/src/util: radix32.h                                           *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2015 Retroshare Team <retroshare.project@gmail.com>           *
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
#ifndef RADIX32_H
#define RADIX32_H

#include <string>
#include <string.h>
#include <vector>
#include <stdint.h>

class Radix32
{
public:
	static std::string encode(const std::vector<uint8_t> &in) {
		return encode(in.data(), in.size());
	}

	static std::string encode(const uint8_t *data, size_t len) {
		std::string out = "";

		size_t pos = 1;
		uint8_t bits = 8, index;
		uint16_t tmp = data[0]; // need min. 16 bits here
		while (bits > 0 || pos < len)	{
			if (bits < 5) {
				if (pos < len) {
					tmp <<= 8;
					tmp |= data[pos++] & 0xFF;
					bits += 8;
				} else { // last byte
					tmp <<= (5 - bits);
					bits = 5;
				}
			}

			bits -= 5;
			index = (tmp >> bits) & 0x1F;
			out.push_back(bintoasc()[index]);
		}

		// append padding
		while(out.length() % 4 != 0)
			out.push_back('=');

		return out;
	}

private:
	static const inline char *bintoasc() { static const char bta[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"; return bta ; }
};

#endif // RADIX32_H
