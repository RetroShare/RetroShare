#include "dnsresolver.h"

#include "pqi/pqinetwork.h"
#include "util/rsnet.h"


#ifndef WIN32
#include <netdb.h>
#endif

#include <string.h>
#include <string>
#include <iostream>
#include <set>
#include <vector>
#include <algorithm>
#include <stdio.h>

const time_t MAX_TIME_BEFORE_RETRY 	=	300 ; /* seconds before retrying an ip address */
const time_t MAX_KEEP_DNS_ENTRY 		= 3600 ; /* seconds during which a DNS entry is considered valid */

static const std::string ADDR_AGENT  = "Mozilla/5.0";

void *solveDNSEntries(void *p)
{
	bool more_to_go = true ;
	DNSResolver *dnsr = (DNSResolver*)p ;

	while(more_to_go)
	{
		// get an address request
		time_t now = time(NULL) ;
		more_to_go = false ;

		std::string next_call = "" ;

		{
			RsStackMutex mut(dnsr->_rdnsMtx) ;

			if(dnsr->_addr_map != NULL)
				for(std::map<std::string, DNSResolver::AddrInfo>::iterator it(dnsr->_addr_map->begin());it!=dnsr->_addr_map->end() && next_call.empty();++it)
				{
					switch(it->second.state)
					{
						case DNSResolver::DNS_SEARCHING: 		
						case DNSResolver::DNS_HAVE:		  		break ;

						case DNSResolver::DNS_LOOKUP_ERROR:		if(it->second.last_lookup_time + MAX_TIME_BEFORE_RETRY > now)
																				continue ;

						case DNSResolver::DNS_DONT_HAVE: 		next_call = it->first ;
																			it->second.state = DNSResolver::DNS_SEARCHING ;
																			it->second.last_lookup_time = now ;
																			more_to_go = true ;
																			break ;
					}
				}
			else
				return NULL; // the thread has been deleted. Return.
		}

		if(!next_call.empty())
		{
			hostent *pHost = gethostbyname(next_call.c_str());

			{
				RsStackMutex mut(dnsr->_rdnsMtx) ;

				DNSResolver::AddrInfo &info = (*dnsr->_addr_map)[next_call];

				if(pHost) 
				{
					info.state = DNSResolver::DNS_HAVE ;
					// IPv4 for the moment.
					struct sockaddr_in *addrv4p = (struct sockaddr_in *) &(info.addr);
					addrv4p->sin_family = AF_INET;
					addrv4p->sin_addr.s_addr  = *(unsigned long*) (pHost->h_addr);
					addrv4p->sin_port = htons(0);
				}
				else
					info.state = DNSResolver::DNS_LOOKUP_ERROR ;
			}
		}
	}

	{
		RsStackMutex mut(dnsr->_rdnsMtx) ;
		*(dnsr->_thread_running) = false ;
	}
	return NULL ;
}

void DNSResolver::start_request()
{
	{
		RsStackMutex mut(_rdnsMtx) ;
		*_thread_running = true ;
	}

	void *data = (void *)this;
	pthread_t tid ;
	pthread_create(&tid, 0, &solveDNSEntries, data);
	pthread_detach(tid); /* so memory is reclaimed in linux */
}

void DNSResolver::reset()
{
	RsStackMutex mut(_rdnsMtx) ;

	*_thread_running = false ;
	_addr_map->clear();
}

bool DNSResolver::getIPAddressFromString(const std::string& server_name,struct sockaddr_storage &addr) 
{
	sockaddr_storage_clear(addr);
	bool running = false;
	{
		RsStackMutex mut(_rdnsMtx) ;

		std::map<std::string, AddrInfo>::iterator it(_addr_map->find(server_name)) ;
		time_t now = time(NULL) ;
		AddrInfo *addr_info ;

		if(it != _addr_map->end())
		{
			// check that the address record is not too old

			if(it->second.last_lookup_time + MAX_KEEP_DNS_ENTRY > now && it->second.state == DNSResolver::DNS_HAVE)
			{
				addr = it->second.addr ;
				return true ;
			}
			else
				addr_info = &it->second ;
		}
		else
			addr_info = &(*_addr_map)[server_name] ;

		// We don't have it. Let's push it into the names to lookup for, except if we're already into it.

		if(addr_info->state != DNSResolver::DNS_SEARCHING)
			addr_info->state = DNSResolver::DNS_DONT_HAVE ;

		running = *_thread_running ;
	}

	if(!running)
		start_request();

	return false ;
}

DNSResolver::~DNSResolver()
{
	RsStackMutex mut(_rdnsMtx) ;

	delete _addr_map ;
	_addr_map = NULL ;
	delete _thread_running ;
}

DNSResolver::DNSResolver() : _rdnsMtx("DNSResolver")
{
	RsStackMutex mut(_rdnsMtx) ;

	_addr_map = new std::map<std::string, AddrInfo>() ;
	_thread_running = new bool ;
	*_thread_running = false ;
}

