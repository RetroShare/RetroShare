/*******************************************************************************
 * libretroshare/src/util: rsrandom.h                                          *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 *  Copyright (C) 2010 Cyril Soler <csoler@users.sourceforge.net>              *
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


#include <vector>
#include <cstdint>

#include "util/rsthreads.h"
#include "util/rsdeprecate.h"

/**
 * RsRandom provide a random number generator that is
 * - thread safe
 * - platform independent
 * - fast
 * - CRYPTOGRAPHICALLY SAFE, because it is based on openssl random number
 *   generator
 */
class RsRandom
{
public:
	static uint32_t random_u32();
	static uint64_t random_u64();
	static float    random_f32();
	static double   random_f64();

	static bool     seed(uint32_t s);

	static std::string random_alphaNumericString(uint32_t length);
	static void        random_bytes(uint8_t* data, uint32_t length);

private:
	static RsMutex rndMtx;

	static const uint32_t N = 1024;

	static void locked_next_state();
	static uint32_t index;
	static std::vector<uint32_t> MT;
};

/// @deprecated this alias is provided only for code retro-compatibility
using RSRandom RS_DEPRECATED_FOR(RsRandom) = RsRandom;
