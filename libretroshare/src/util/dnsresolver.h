#pragma once

#include "pqi/pqinetwork.h"

#ifndef WIN32
#include <netdb.h>
#endif

#include <map>
#include <string>
#include "util/rsthreads.h"

struct sockaddr ;

class DNSResolver
{
	public:
		DNSResolver() ;
		~DNSResolver() ;

		bool getIPAddressFromString(const std::string& server_name,struct sockaddr_storage &addr) ;

		void start_request() ;
		void reset() ;

	private:
		enum { DNS_DONT_HAVE,DNS_SEARCHING, DNS_HAVE, DNS_LOOKUP_ERROR } ;

		struct AddrInfo
		{
			uint32_t state ; 				// state: Looked-up, not found, have
			time_t last_lookup_time ;	// last lookup time
			struct sockaddr_storage addr ;
		};
		friend void *solveDNSEntries(void *p) ;

		RsMutex _rdnsMtx ;
		bool *_thread_running ;
		std::map<std::string, AddrInfo> *_addr_map ;
};
