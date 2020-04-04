/*******************************************************************************
 *                                                                             *
 * libretroshare base64 encoding utilities                                     *
 *                                                                             *
 * Copyright (C) 2015  Retroshare Team <contact@retroshare.cc>                 *
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

#include <cmath>

#include "util/rsbase64.h"
#include "util/rsdebug.h"

#if __cplusplus < 201703L
/* Solve weird undefined reference error with C++ < 17 see:
 * https://stackoverflow.com/questions/8016780/undefined-reference-to-static-constexpr-char
 */
/*static*/ decltype(RsBase64::bDict) constexpr RsBase64::bDict;
/*static*/ decltype(RsBase64::uDict) constexpr RsBase64::uDict;
/*static*/ decltype(RsBase64::rDict) constexpr RsBase64::rDict;
/*static*/ decltype(RsBase64::sPad)  constexpr RsBase64::sPad;
#endif

/*static*/ void RsBase64::encode(
        rs_view_ptr<const uint8_t> data, size_t len, std::string& outString,
        bool padding, bool urlSafe )
{
	const char* sDict = urlSafe ? uDict : bDict;

	// Workaround if input and output are the same buffer.
	bool inplace = (outString.data() == reinterpret_cast<const char*>(data));
	std::string tBuff;
	std::string& outStr = inplace ? tBuff : outString;

	auto encSize = encodedSize(len, padding);
	outStr.resize(encSize);
	char* p = &outStr[0];

	for (; len >= 3; len -= 3, data += 3)
	{
		*p++ = sDict[ (data[0] >> 2) & 077 ];
		*p++ = sDict[
		        (((data[0] << 4) & 060) | ((data[1] >> 4) & 017)) & 077 ];
		*p++ = sDict[
		        (((data[1] << 2) & 074) | ((data[2] >> 6) & 03)) & 077 ];
		*p++ = sDict[ data[2] & 077 ];
	}
	if (len == 2)
	{
		*p++ = sDict[ (data[0] >> 2) & 077 ];
		*p++ = sDict[
		        (((data[0] << 4) & 060) | ((data[1] >> 4) & 017)) & 077 ];
		*p++ = sDict[ ((data[1] << 2) & 074) ];
		if(padding) *p++ = sPad;
	}
	else if (len == 1)
	{
		*p++ = sDict[ (data[0] >> 2) & 077 ];
		*p++ = sDict[ (data[0] << 4) & 060 ];
		if(padding) { *p++ = sPad; *p++ = sPad; }
	}

	if(inplace) outString = tBuff;
}

/*static*/ std::error_condition RsBase64::decode(
        const std::string& encoded, std::vector<uint8_t>& decoded )
{
	size_t decSize; std::error_condition ec;
	std::tie(decSize, ec) = decodedSize(encoded);
	if(!decSize || ec) return ec;

	size_t encSize = encoded.size();
	decoded.resize(decSize);

	for (size_t i = 0, o = 0; i < encSize; i += 4, o += 3)
	{
		char input0 = encoded[i + 0];
		char input1 = encoded[i + 1];

		/* At the end of the string, missing bytes 2 and 3 are considered
		 * padding '=' */
		char input2 = i + 2 < encoded.size() ? encoded[i + 2] : sPad;
		char input3 = i + 3 < encSize ? encoded[i + 3] : sPad;

		// If any unknown characters appear, it's an error.
		if(!( isBase64Char(input0) && isBase64Char(input1) &&
		      isBase64Char(input2) && isBase64Char(input3) ))
			    return std::errc::argument_out_of_domain;

		/* If padding appears anywhere but the last 1 or 2 characters, or if
		 * it appears but encoded.size() % 4 != 0, it's an error. */
		bool at_end = (i + 4 >= encSize);
		if ( (input0 == sPad) || (input1 == sPad) ||
		     ( input2 == sPad && !at_end ) ||
		     ( input2 == sPad && input3 != sPad ) ||
		     ( input3 == sPad && !at_end) )
			return  std::errc::illegal_byte_sequence;

		uint32_t b0 = rDict[static_cast<uint8_t>(input0)] & 0x3f;
		uint32_t b1 = rDict[static_cast<uint8_t>(input1)] & 0x3f;
		uint32_t b2 = rDict[static_cast<uint8_t>(input2)] & 0x3f;
		uint32_t b3 = rDict[static_cast<uint8_t>(input3)] & 0x3f;

		uint32_t stream = (b0 << 18) | (b1 << 12) | (b2 << 6) | b3;
		decoded[o + 0] = (stream >> 16) & 0xFF;
		if (input2 != sPad) decoded[o + 1] = (stream >> 8) & 0xFF;
		/* If there are any stale bits in this from input1, the text is
		 * malformed. */
		else if (((stream >> 8) & 0xFF) != 0)
			return std::errc::invalid_argument;

		if (input3 != sPad) decoded[o + 2] = (stream >> 0) & 0xFF;
		/* If there are any stale bits in this from input2, the text is
		 * malformed. */
		else if (((stream >> 0) & 0xFF) != 0)
			return std::errc::invalid_argument;
	}

	return std::error_condition();
}

/*static*/ size_t RsBase64::encodedSize(size_t decodedSize, bool padding)
{
	if(padding) return 4 * (decodedSize + 2) / 3;
	return static_cast<size_t>(
	            std::ceil(4L * static_cast<double>(decodedSize) / 3L) );
}

/*static*/ std::tuple<size_t, std::error_condition> RsBase64::decodedSize(
        const std::string& input )
{
	const auto success = [](size_t val)
	{ return std::make_tuple(val, std::error_condition()); };

	if(input.empty()) return success(0);

	auto mod = input.size() % 4;
	if(mod == 1) std::make_tuple(0, std::errc::invalid_argument);

	size_t padded_size = ((input.size() + 3) / 4) * 3;
	if (mod >= 2 || (mod == 0 && input[input.size() - 1] == sPad))
	{
		/* If the last byte is '=', or the input size % 4 is 2 or 3 (thus
		 * there are implied '='s), then the actual size is 1-2 bytes
		 * smaller. */
		if ( mod == 2 || (mod == 0 && input[input.size() - 2] == sPad) )
		{
			/* If the second-to-last byte is also '=', or the input
			 * size % 4 is 2 (implying a second '='), then the actual size
			 * is 2 bytes smaller. */
			return success(padded_size - 2);
		}
		else
		{
			/* Otherwise it's just the last character and the actual size is
			 * 1 byte smaller. */
			return success(padded_size - 1);
		}
	}
	return success(padded_size);
}

/*static*/ size_t RsBase64::stripInvalid(
        const std::string& in, std::string& out )
{
	size_t strippedCnt = 0;
	auto inSize = in.size();
	out.resize(inSize);
	for(size_t i = 0; i < inSize; ++i)
	{
		if(isBase64Char(in[i])) out[i-strippedCnt] = in[i];
		else ++strippedCnt;
	}
	out.resize(inSize-strippedCnt);
	return strippedCnt;
}
