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

#include "rsrandom.h"

#include <string>
#include <openssl/rand.h>
#include <time.h>
#include <pthread.h>

#define RS_RANDOM_STATE_SIZE 1024

uint32_t RSRandom::index = RS_RANDOM_STATE_SIZE;
std::vector<uint32_t> RSRandom::MT(RS_RANDOM_STATE_SIZE, 0u);
RsMutex RSRandom::rndMtx("RSRandom");


void RSRandom::seed(uint32_t s)
{
	RS_STACK_MUTEX(rndMtx);

	MT[0]= s & 0xffffffffUL;
	for (uint32_t j=1; j<RS_RANDOM_STATE_SIZE; j++ )
		MT[j] = (1812433253UL * (MT[j-1] ^ (MT[j-1] >> 30)) + j) & 0xffffffffUL;

	RAND_seed( reinterpret_cast<unsigned char *>(&MT[0]),
	           RS_RANDOM_STATE_SIZE*sizeof(uint32_t) );
	locked_next_state();
}

void RSRandom::random_bytes(unsigned char *data, uint32_t size)
{
	/* According to our tests (cyril+thunder), on both Windows and Linux
	 * RAND_bytes does init itself automatically at first call, from
	 * system-based unpredictable values, so that seeding is not even needed.
	 * This call still adds some randomness (not much actually, but it's always
	 * good to have anyway) */
	static bool first_time_seed = true;
	if (first_time_seed)
	{
#if (!defined(WINDOWS_SYS) || defined(WIN_PTHREADS_H))
		RSRandom::seed((time(NULL) + ((uint32_t) pthread_self())*0x1293fe)^0x18e34a12);
#else
		RSRandom::seed((time(NULL) + ((uint32_t) pthread_self().p)*0x1293fe)^0x18e34a12);
#endif
		first_time_seed = false;
	}

	RAND_bytes(data,size);
}

void RSRandom::locked_next_state() 
{
	RAND_bytes( reinterpret_cast<unsigned char *>(&MT[0]),
	            RS_RANDOM_STATE_SIZE*sizeof(uint32_t) );
	index = 0 ;
}

uint32_t RSRandom::random_u32() 
{
	uint32_t y;

	{
		RS_STACK_MUTEX(rndMtx);
		++index;
		if(index >= RS_RANDOM_STATE_SIZE) locked_next_state();
		y = MT[index];
	}

	// Tempering
	y ^= (y >> 11);
	y ^= (y << 7 ) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return y;
}

uint64_t RSRandom::random_u64() 
{
	return ((uint64_t)random_u32() << 32ul) + random_u32();
}

float RSRandom::random_f32() 
{
	return random_u32() / (float)(~(uint32_t)0);
}

double RSRandom::random_f64() 
{
	return random_u64() / (double)(~(uint64_t)0);
}

void RSRandom::random_alphaNumericString(std::string &str, uint32_t len)
{
	str.clear();
	for(uint32_t i=0; i<len; ++i)
		str += (char)((random_u32()%94) + 33);
}
