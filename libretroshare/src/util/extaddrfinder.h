#pragma once

#include <list>
#include <string>
#include "util/rsthreads.h"
#include "util/rsnet.h"

struct sockaddr ;

class ExtAddrFinder
{
	public:
		ExtAddrFinder() ;
		~ExtAddrFinder() ;

		bool hasValidIP(struct sockaddr_storage &addr) ;
		void getIPServersList(std::list<std::string>& ip_servers) { ip_servers = _ip_servers ; }

		void start_request() ;

		void reset() ;

	private:
		friend void* doExtAddrSearch(void *p) ;

		RsMutex mAddrMtx ;
		time_t   mFoundTS;
		struct sockaddr_storage mAddr;
		bool mFound ;
		bool mSearching ;
		std::list<std::string> _ip_servers ;
};
