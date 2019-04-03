/*******************************************************************************
 * util/bdrandom.cc                                                            *
 *                                                                             *
 * BitDHT: An Flexible DHT library.                                            *
 *                                                                             *
 * Copyright (C) 2010 Cyril Soler <csoler@users.sourceforge.net>               *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <time.h>
#include "util/bdrandom.h"

uint32_t bdRandom::index = 0 ;
std::vector<uint32_t> bdRandom::MT(bdRandom::N,0u) ;
bdMutex bdRandom::rndMtx ;

#if (defined(_WIN32) || defined(__MINGW32__)) && !defined(WIN_PTHREADS_H)
static bool auto_seed = bdRandom::seed( (time(NULL) + ((uint32_t) pthread_self().p)*0x1293fe)^0x18e34a12 ) ;
#else
  #ifdef __APPLE__
	static bool auto_seed = bdRandom::seed( (time(NULL) + pthread_mach_thread_np(pthread_self())*0x1293fe + (getpid()^0x113ef76b))^0x18e34a12 ) ;
  #elif defined(__FreeBSD__) || (__HAIKU__)
    // since this is completely insecure anyway, just kludge for now
    static bool auto_seed = bdRandom::seed(time(NULL));
  #elif defined(__OpenBSD__)
    static bool auto_seed = bdRandom::seed(arc4random());
  #else
    static bool auto_seed = bdRandom::seed( (time(NULL) + pthread_self()*0x1293fe + (getpid()^0x113ef76b))^0x18e34a12 ) ;
  #endif
#endif
bool bdRandom::seed(uint32_t s) 
{
	bdStackMutex mtx(rndMtx) ;

	MT.resize(N,0) ;	// because MT might not be already resized

	uint32_t j ;
	MT[0]= s & 0xffffffffUL;
	for (j=1; j<N; j++) 
		MT[j] = (1812433253UL * (MT[j-1] ^ (MT[j-1] >> 30)) + j) & 0xffffffffUL ;

	return true ;
}

void bdRandom::locked_next_state() 
{
	for(uint32_t i=0;i<N;++i)
	{
		uint32_t y = ((MT[i]) & UMASK) | ((MT[(i+1)%(int)N]) & LMASK) ;

		MT[i] = MT[(i + M) % (int)N] ^ (y >> 1) ;

		if((y & 1) == 1) 
			MT[i] = MT[i] ^ 0x9908b0df ;
	}
	index = 0 ;
}

uint32_t bdRandom::random_u32() 
{
	uint32_t y;

	{
		bdStackMutex mtx(rndMtx) ;

		y = MT[index++] ;

		if(index == N)
			locked_next_state();
	}

	// Tempering
	y ^= (y >> 11);
	y ^= (y << 7 ) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return y;
}

uint64_t bdRandom::random_u64() 
{
	return ((uint64_t)random_u32() << 32ul) + random_u32() ;
}

float bdRandom::random_f32() 
{
	return random_u32() / (float)(~(uint32_t)0) ;
}

double bdRandom::random_f64() 
{
	return random_u64() / (double)(~(uint64_t)0) ;
}

std::string bdRandom::random_alphaNumericString(uint32_t len)
{
	std::string s = "" ;

	for(uint32_t i=0;i<len;++i)
		s += (char)( (random_u32()%94) + 33) ;

	return s ;
}

