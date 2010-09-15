#include <stdlib.h>
#include "rsrandom.h"

uint32_t RSRandom::index = 0 ;
std::vector<uint32_t> RSRandom::MT(RSRandom::N,0u) ;
RsMutex RSRandom::rndMtx ;
static bool auto_seed = RSRandom::seed(time(NULL)) ;

bool RSRandom::seed(uint32_t s) 
{
	RsStackMutex mtx(rndMtx) ;

	MT.resize(N,0) ;	// because MT might not be already resized

	uint32_t j ;
	MT[0]= s & 0xffffffffUL;
	for (j=1; j<N; j++) 
		MT[j] = (1812433253UL * (MT[j-1] ^ (MT[j-1] >> 30)) + j) & 0xffffffffUL ;

	return true ;
}

void RSRandom::locked_next_state() 
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

uint32_t RSRandom::random_u32() 
{
	uint32_t y;

	{
		RsStackMutex mtx(rndMtx) ;

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

uint64_t RSRandom::random_u64() 
{
	return ((uint64_t)random_u32() << 32ul) + random_u32() ;
}

float RSRandom::random_f32() 
{
	return random_u32() / (float)(~(uint32_t)0) ;
}

double RSRandom::random_f64() 
{
	return random_u64() / (double)(~(uint64_t)0) ;
}

