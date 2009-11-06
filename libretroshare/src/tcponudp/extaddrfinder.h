#pragma once

#include <list>
#include <string>
#include "util/rsthreads.h"

struct sockaddr ;

class ExtAddrFinder
{
	public:
		ExtAddrFinder() ;
		~ExtAddrFinder() ;

		bool hasValidIP(struct sockaddr_in *addr) ;
		void getIPServersList(std::list<std::string>& ip_servers) { ip_servers = _ip_servers ; }

		void start_request() ;

		void reset() ;

	private:
		friend void* doExtAddrSearch(void *p) ;

		time_t   *mFoundTS;
		RsMutex _addrMtx ;
		struct sockaddr_in *_addr ;
		bool *_found ;
		bool *_searching ;
		std::list<std::string> _ip_servers ;
};
