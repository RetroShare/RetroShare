/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2010 Cyril Soler <csoler@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
 
#pragma once

// RSRandom contains a random number generator that is
// - thread safe
// - system independant
// - fast
// - cryptographically safe
//
// The implementation is adapted from the Mersenne Twister page of Wikipedia.
//
// 		http://en.wikipedia.org/wiki/Mersenne_twister

#include <vector>
#include <util/rsthreads.h>

class RSRandom
{
	public:
		static uint32_t random_u32() ;
		static uint64_t random_u64() ;
		static float 	 random_f32() ;
		static double	 random_f64() ;

		static bool     seed(uint32_t s) ;

		static std::string random_alphaNumericString(uint32_t length) ; 

	private:
		static RsMutex rndMtx ;

		static const uint32_t N = 624;
		static const uint32_t M = 397;

		static const uint32_t MATRIX_A 	= 0x9908b0dfUL;
		static const uint32_t UMASK 		= 0x80000000UL;
		static const uint32_t LMASK 		= 0x7fffffffUL;

		static void locked_next_state() ;
		static uint32_t index ;
		static std::vector<uint32_t> MT ;
};
