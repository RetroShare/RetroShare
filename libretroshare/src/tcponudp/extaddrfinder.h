#pragma once

#include "util/rsthreads.h"

struct sockaddr ;

class ExtAddrFinder
{
	public:
		ExtAddrFinder() ;
		~ExtAddrFinder() ;

		bool hasValidIP(struct sockaddr_in *addr) ;

		void start_request() ;

		RsMutex _addrMtx ;
		struct sockaddr_in *_addr ;
		bool *_found ;
		bool *_searching ;
};
