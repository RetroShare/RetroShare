/*******************************************************************************
 * libretroshare/src/util: extaddrfinder.cc                                    *
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
#include "extaddrfinder.h"

#include "pqi/pqinetwork.h"
#include "util/rsstring.h"
#include "util/rsmemory.h"

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
#include "util/rstime.h"

const uint32_t MAX_IP_STORE = 300; /* seconds ip address timeout */

//#define EXTADDRSEARCH_DEBUG

void* doExtAddrSearch(void *p)
{
	
	std::vector<std::string> res ;

	ExtAddrFinder *af = (ExtAddrFinder*)p ;

	for(std::list<std::string>::const_iterator it(af->_ip_servers.begin());it!=af->_ip_servers.end();++it)
	{
		std::string ip = "";
		rsGetHostByNameSpecDNS(*it,"myip.opendns.com",ip);
		if(ip != "")
			res.push_back(ip) ;
#ifdef EXTADDRSEARCH_DEBUG
		std::cout << "ip found through DNS " << *it << ": \"" << ip << "\"" << std::endl ;
#endif
	}

	if(res.empty())
	{
		// thread safe copy results.
		//
		{
			RsStackMutex mtx(af->mAddrMtx) ;

			af->mFound = false ;
			af->mFoundTS = time(NULL) ;
			af->mSearching = false ;
		}
		return NULL ;
	}

	sort(res.begin(),res.end()) ; // eliminates outliers.



	if(!sockaddr_storage_ipv4_aton(af->mAddr, res[res.size()/2].c_str()))
	{
		std::cerr << "ExtAddrFinder: Could not convert " << res[res.size()/2] << " into an address." << std::endl ;
		{
			RsStackMutex mtx(af->mAddrMtx) ;
			af->mFound = false ;
			af->mFoundTS = time(NULL) ;
			af->mSearching = false ;
		}
		return NULL ;
	}

	{
		RsStackMutex mtx(af->mAddrMtx) ;
		af->mFound = true ;
		af->mFoundTS = time(NULL) ;
		af->mSearching = false ;
	}

	return NULL ;
}


void ExtAddrFinder::start_request()
{
	void *data = (void *)this;
	pthread_t tid ;
    
    	if(! pthread_create(&tid, 0, &doExtAddrSearch, data))
		pthread_detach(tid); /* so memory is reclaimed in linux */
        else
            	std::cerr << "(EE) Could not start ExtAddrFinder thread." << std::endl;
}

bool ExtAddrFinder::hasValidIP(struct sockaddr_storage &addr)
{
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Getting ip." << std::endl ;
#endif

	{
		RsStackMutex mut(mAddrMtx) ;
		if(mFound)
		{
#ifdef EXTADDRSEARCH_DEBUG
			std::cerr << "ExtAddrFinder: Has stored ip: responding with this ip." << std::endl ;
#endif
            sockaddr_storage_copyip(addr,mAddr);	// just copy the IP so we dont erase the port.
		}
	}
	rstime_t delta;
	{
		RsStackMutex mut(mAddrMtx) ;
		//timeout the current ip
		delta = time(NULL) - mFoundTS;
	}
	if((uint32_t)delta > MAX_IP_STORE) {//launch a research
		if( mAddrMtx.trylock())
		{
			if(!mSearching)
			{
#ifdef EXTADDRSEARCH_DEBUG
				std::cerr << "ExtAddrFinder: No stored ip: Initiating new search." << std::endl ;
#endif
				mSearching = true ;
				start_request() ;
			}
#ifdef EXTADDRSEARCH_DEBUG
			else
				std::cerr << "ExtAddrFinder: Already searching." << std::endl ;
#endif
			mAddrMtx.unlock();
		}
#ifdef EXTADDRSEARCH_DEBUG
		else
			std::cerr << "ExtAddrFinder: (Note) Could not acquire lock. Busy." << std::endl ;
#endif
	}

	RsStackMutex mut(mAddrMtx) ;
	return mFound ;
}

void ExtAddrFinder::reset()
{
	RsStackMutex mut(mAddrMtx) ;

	mFound = false ;
	mSearching = false ;
	mFoundTS = time(NULL) - MAX_IP_STORE;
}

ExtAddrFinder::~ExtAddrFinder()
{
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Deleting ExtAddrFinder." << std::endl ;
#endif

}

ExtAddrFinder::ExtAddrFinder() : mAddrMtx("ExtAddrFinder")
{
#ifdef EXTADDRSEARCH_DEBUG
	std::cerr << "ExtAddrFinder: Creating new ExtAddrFinder." << std::endl ;
#endif
	RsStackMutex mut(mAddrMtx) ;

	mFound = false;
	mSearching = false;
	mFoundTS = time(NULL) - MAX_IP_STORE;
	sockaddr_storage_clear(mAddr);

//https://unix.stackexchange.com/questions/22615/how-can-i-get-my-external-ip-address-in-a-shell-script
	//Enter direct ip so local DNS cannot change it.
	//DNS servers must recognize "myip.opendns.com"
	_ip_servers.push_back(std::string( "208.67.222.222" )) ;//resolver1.opendns.com
	_ip_servers.push_back(std::string( "208.67.220.220" )) ;//resolver2.opendns.com
	_ip_servers.push_back(std::string( "208.67.222.220" )) ;//resolver3.opendns.com
	_ip_servers.push_back(std::string( "208.67.220.222" )) ;//resolver4.opendns.com
	//Ipv6 server disabled as Current ip only manage ipv4 for now.
	//_ip_servers.push_back(std::string( "2620:119:35::35" )) ;//resolver1.opendns.com
	//_ip_servers.push_back(std::string( "2620:119:53::53" )) ;//resolver2.opendns.com
}

