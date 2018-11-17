/*******************************************************************************
 * libretroshare/src/util: dnsresolver.h                                       *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright (C) 2017 Retroshare Team <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Lesser General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Lesser General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/
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
			rstime_t last_lookup_time ;	// last lookup time
			struct sockaddr_storage addr ;
		};
		friend void *solveDNSEntries(void *p) ;

		RsMutex _rdnsMtx ;
		bool *_thread_running ;
		std::map<std::string, AddrInfo> *_addr_map ;
};
