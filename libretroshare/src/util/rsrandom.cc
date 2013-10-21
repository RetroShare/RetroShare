#include <stdlib.h>
#include <string>
#include <unistd.h>
#include "rsrandom.h"

#include <openssl/rand.h>

uint32_t RSRandom::index = RSRandom::N ;
std::vector<uint32_t> RSRandom::MT(RSRandom::N,0u) ;
RsMutex RSRandom::rndMtx("RSRandom") ;

// According to our tests (cyril+thunder), on both Windows and Linux does
// RAND_bytes init itself automatically at first call, from system-based
// unpredictable values, so that seeding is not even needed.
// This call still adds some randomness (not much actually, but it's always good to
// have anyway)
//
#ifdef WINDOWS_SYS
#include <time.h>
#ifdef WIN_PTHREADS_H
static bool auto_seed = RSRandom::seed( (time(NULL) + ((uint32_t) pthread_self())*0x1293fe)^0x18e34a12 ) ;
#else
static bool auto_seed = RSRandom::seed( (time(NULL) + ((uint32_t) pthread_self().p)*0x1293fe)^0x18e34a12 ) ;
#endif
#endif

bool RSRandom::seed(uint32_t s) 
{
	RsStackMutex mtx(rndMtx) ;

	MT.resize(N,0) ;	// because MT might not be already resized

	uint32_t j ;
	MT[0]= s & 0xffffffffUL;
	for (j=1; j<N; j++) 
		MT[j] = (1812433253UL * (MT[j-1] ^ (MT[j-1] >> 30)) + j) & 0xffffffffUL ;

	RAND_seed((unsigned char *)&MT[0],N*sizeof(uint32_t)) ;
	locked_next_state() ;

	return true ;
}

void RSRandom::random_bytes(unsigned char *data,uint32_t size) 
{
	RAND_bytes(data,size) ;
}
void RSRandom::locked_next_state() 
{
	RAND_bytes((unsigned char *)&MT[0],N*sizeof(uint32_t)) ;
	index = 0 ;
}

uint32_t RSRandom::random_u32() 
{
	uint32_t y;

	{
		RsStackMutex mtx(rndMtx) ;

		index++ ;

		if(index >= N)
			locked_next_state();

		y = MT[index] ;
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

std::string RSRandom::random_alphaNumericString(uint32_t len)
{
	std::string s = "" ;

	for(uint32_t i=0;i<len;++i)
		s += (char)( (random_u32()%94) + 33) ;

	return s ;
}

