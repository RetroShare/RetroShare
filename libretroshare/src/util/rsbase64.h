/*******************************************************************************
 *                                                                             *
 * libretroshare base64 encoding utilities                                     *
 *                                                                             *
 * Copyright (C) 2020  Gioacchino Mazzurco <gio@eigenlab.org>                  *
 * Copyright (C) 2020  Asociación Civil Altermundi <info@altermundi.net>       *
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

#include <string>
#include <vector>
#include <cstdint>
#include <system_error>
#include <tuple>

#include "util/rsmemory.h"

/**
 * Implement methods to encode e decode to base64 format as per RFC 4648
 * This implementation support also the file name and URL safe base64url format
 * @see https://tools.ietf.org/html/rfc4648#section-5
 */
class RsBase64
{
public:
	/// Enable base64url by default
	static constexpr bool DEFAULT_URL_SAFE = true;

	/// Disable padding by default
	static constexpr bool DEFAULT_PADDING = false;

	/**
	 * @brief Encode arbitrary data to base64
	 * @param[in] data pointer to the input data buffer
	 * @param[in] len lenght of the input buffer
	 * @param[out] outString storage for the resulting base64 encoded string
	 * @param[in] padding set to true to enable padding to 32 bits
	 * @param[in] urlSafe pass true for base64url format, false for base64 format
	 */
	static void encode(
	        rs_view_ptr<const uint8_t> data, size_t len,
	        std::string& outString,
	        bool padding = DEFAULT_PADDING, bool urlSafe = DEFAULT_URL_SAFE );

	/**
	 * @brief Decode data from a base64 encoded string
	 * @param[in] encoded encoded string
	 * @param[out] decoded storage for decoded data
	 * @return success or error details
	 */
	static std::error_condition decode(
	        const std::string& encoded, std::vector<uint8_t>& decoded );

	/**
	 * Remove invalid characters from base64 encoded string.
	 * Often when copy and pasting from one progam to another long base64
	 * strings, new lines, spaces or other characters end up polluting the
	 * original text. This function is useful to cleanup the pollution before
	 * attempting to decode the message.
	 * @param in input string
	 * @param out storage for cleaned string. In-place operation in supported so
	 * the same input string may be passed.
	 * @return count of stripped invalid characters
	 */
	static size_t stripInvalid(const std::string& in, std::string& out);

	/**
	 * Calculate how much bytes are needed to store the base64 encoded version
	 * of some data.
	 * @param decodedSize size of the original decoded data
	 * @param padding true to enable base64 padding
	 * @return how much bytes would take to store the encoded version
	 */
	static size_t encodedSize(
	        size_t decodedSize, bool padding = DEFAULT_PADDING );

	/**
	 * @brief Calculate how much space is needed to store the decoded version of
	 * a base64 encoded string
	 * @param input encoded string
	 * @return decoded size, plus error information on failure
	 */
	static std::tuple<size_t, std::error_condition> decodedSize(
	        const std::string& input );

private:
	/// base64 conversion table
	static constexpr char bDict[] =
	        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	/// base64url conversion table
	static constexpr char uDict[] =
	        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

	/// This reverse table supports both base64 and base64url
	static constexpr int8_t rDict[256] = {
	/* index  +0  +1  +2  +3  +4  +5  +6  +7  +8  +9 +10 +11 +12 +13 +14 +15 */
	/*   0 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/*  16 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/*  32 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, 62, -1, 63,
	/*  48 */ 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1,
	/*  64 */ -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
	/*  80 */ 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, 63,
	/*  96 */ -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	/* 112 */ 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
	/* 128 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 144 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 160 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 176 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 192 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 208 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 224 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	/* 240 */ -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	};

	/// base64 padding character
	static constexpr char sPad = '=';

	/** Check if given character is valid either for base64 or for base64url
	 * @param c character to check
	 * @return true if valid false otherwise
	 */
	static inline bool isBase64Char(char c)
	{ return rDict[static_cast<uint8_t>(c)] >= 0; }
};
