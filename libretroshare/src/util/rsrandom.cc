#include <stdlib.h>
#include <string>
#include <unistd.h>
#include "rsrandom.h"

#define RSRANDOM_USE_SSL

#ifdef RSRANDOM_USE_SSL
#include <openssl/rand.h>
#endif
uint32_t RSRandom::index = RSRandom::N ;
std::vector<uint32_t> RSRandom::MT(RSRandom::N,0u) ;
RsMutex RSRandom::rndMtx("RSRandom") ;

// Random seed is called according to the following rules:
// 	OpenSSL random bytes:
// 			- on systems that only have /dev/urandom (linux, BSD, MacOS), we don't need to call the seed
// 			- on windows, we need to
// 	MT19937 pseudo random
// 			- always seed.
//
#ifdef WINDOWS_SYS
static bool auto_seed = RSRandom::seed( (time(NULL) + ((uint32_t) pthread_self().p)*0x1293fe)^0x18e34a12 ) ;
#else
#ifndef RSRANDOM_USE_SSL
  #ifdef __APPLE__
	static bool auto_seed = RSRandom::seed( (time(NULL) + pthread_mach_thread_np(pthread_self())*0x1293fe + (getpid()^0x113ef76b))^0x18e34a12 ) ;
  #elif defined(__FreeBSD__)
    // since this is completely insecure anyway, just kludge for now
    static bool auto_seed = RSRandom::seed(time(NULL));
  #else
    static bool auto_seed = RSRandom::seed( (time(NULL) + pthread_self()*0x1293fe + (getpid()^0x113ef76b))^0x18e34a12 ) ;
  #endif
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

#ifdef RSRANDOM_USE_SSL
	RAND_seed((unsigned char *)&MT[0],N*sizeof(uint32_t)) ;
#endif
	return true ;
}

void RSRandom::locked_next_state() 
{
#ifdef RSRANDOM_USE_SSL
	RAND_bytes((unsigned char *)&MT[0],N*sizeof(uint32_t)) ;
#else
	for(uint32_t i=0;i<N;++i)
	{
		uint32_t y = ((MT[i]) & UMASK) | ((MT[(i+1)%(int)N]) & LMASK) ;

		MT[i] = MT[(i + M) % (int)N] ^ (y >> 1) ;

		if((y & 1) == 1) 
			MT[i] = MT[i] ^ 0x9908b0df ;
	}
#endif
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

